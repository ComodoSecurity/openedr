/**
*	This sample redirects TCP/UDP traffic to the specified SOCKS5 proxy.
**/

#include "stdafx.h"
#include <crtdbg.h>
#include <process.h>
#include <map>
#include <queue>
#include "nfapi.h"
#include "sync.h"
#include "UdpProxy.h"
#include "TcpProxy.h"

using namespace nfapi;

#if defined(_DEBUG) || defined(_RELEASE_LOG)
DBGLogger DBGLogger::dbgLog;
#endif

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

typedef std::vector<std::string> tStrings;

unsigned char	g_proxyAddress[NF_MAX_ADDRESS_LENGTH];
tStrings	g_processNames;
std::string g_userName;
std::string g_userPassword;

inline bool safe_iswhitespace(int c) { return (c == (int) ' ' || c == (int) '\t' || c == (int) '\r' || c == (int) '\n'); }

std::string trimWhitespace(std::string str)
{
	while (str.length() > 0 && safe_iswhitespace(str[0]))
	{
		str.erase(0, 1);
	}

	while (str.length() > 0 && safe_iswhitespace(str[str.length()-1]))
	{
		str.erase(str.length()-1, 1);
	}

	return str;
}

bool parseValue(const std::string & _s, tStrings & v)
{
	std::string sPart;
	size_t pos;
	std::string s = _s;

	v.clear();

	while (!s.empty()) 
	{
		pos = s.find(",");
	
		if (pos == std::string::npos)
		{
			sPart = trimWhitespace(s);
			s.erase();
		} else
		{
			sPart = trimWhitespace(s.substr(0, pos));
			s.erase(0, pos+1);
		}
		
		if (!sPart.empty())
		{
			v.push_back(sPart);
		}
	}

	return true;
}


bool checkProcessName(DWORD processId)
{
	char processName[_MAX_PATH] = "";
	char processNameLong[_MAX_PATH] = "";

	if (processId == 4)
	{
		strcpy(processNameLong, "System");
	} else
	{	
		if (!nf_getProcessNameA(processId, processName, sizeof(processName)/sizeof(processName[0])))
		{
			return false;
		}

		if (!GetLongPathName(processName, processNameLong, sizeof(processNameLong)/sizeof(processNameLong[0])))
		{
			return false;
		}
	}

	size_t processNameLongLen = strlen(processNameLong);

	for (size_t i=0; i<g_processNames.size(); i++)
	{
		if (g_processNames[i].length() > processNameLongLen)
			continue;

		if (stricmp(g_processNames[i].c_str(), processNameLong + processNameLongLen - g_processNames[i].length()) == 0)
			return true;
	}

	return false;
}

// Forward declarations
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);

struct UDP_CONTEXT
{
	UDP_CONTEXT(PNF_UDP_OPTIONS options)
	{
		if (options)
		{
			m_options = (PNF_UDP_OPTIONS)new char[sizeof(NF_UDP_OPTIONS) + options->optionsLength];
			memcpy(m_options, options, sizeof(NF_UDP_OPTIONS) + options->optionsLength - 1);
		} else
		{
			m_options = NULL;
		}
	}

	~UDP_CONTEXT()
	{
		if (m_options)
			delete[] m_options;
	}
	
	PNF_UDP_OPTIONS m_options;
};



//
//	API events handler
//
class EventHandler : public NF_EventHandler, public UdpProxy::UDPProxyHandler
{
	TcpProxy::LocalTCPProxy m_tcpProxy;

	typedef std::map<unsigned __int64, UDP_CONTEXT*> tUdpCtxMap;
	tUdpCtxMap m_udpCtxMap;

	UdpProxy::UDPProxy	m_udpProxy;

	typedef std::set<unsigned __int64> tIdSet;
	tIdSet m_filteredUdpIds;

	AutoCriticalSection m_cs;


public:

	bool init()
	{
		bool result = false;
		
		for (;;)
		{
			if (!m_udpProxy.init(this, 
				(char*)g_proxyAddress, 
				(((sockaddr*)g_proxyAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in),
				g_userName.empty()? NULL : g_userName.c_str(),
				g_userPassword.empty()? NULL : g_userPassword.c_str()))
			{
				printf("Unable to start UDP proxy");
				break;
			}

			if (!m_tcpProxy.init(htons(8888)))
			{
				printf("Unable to start TCP proxy");
				break;
			}

			m_tcpProxy.setProxy(0, TcpProxy::PROXY_SOCKS5, 
				(char*)g_proxyAddress, 
				(((sockaddr*)g_proxyAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in),
				g_userName.empty()? NULL : g_userName.c_str(),
				g_userPassword.empty()? NULL : g_userPassword.c_str());
			
			result = true;

			break;
		}

		if (!result)
		{
			free();
		}

		return result;
	}

	void free()
	{
		m_udpProxy.free();
		m_tcpProxy.free();

		AutoLock lock(m_cs);
		while (!m_udpCtxMap.empty())
		{
			tUdpCtxMap::iterator it = m_udpCtxMap.begin();
			delete it->second;
			m_udpCtxMap.erase(it);
		}
		m_filteredUdpIds.clear();
	}

	virtual void onUdpReceiveComplete(unsigned __int64 id, char * buf, int len, char * remoteAddress, int remoteAddressLen)
	{
		AutoLock lock(m_cs);
		
		tUdpCtxMap::iterator it = m_udpCtxMap.find(id);
		if (it == m_udpCtxMap.end())
			return;

		char remoteAddr[MAX_PATH];
		DWORD dwLen;
		
		dwLen = sizeof(remoteAddr);
		WSAAddressToString((sockaddr*)remoteAddress, 
				(((sockaddr*)remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);

		printf("onUdpReceiveComplete id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);
//		fflush(stdout);

		nf_udpPostReceive(id, (const unsigned char*)remoteAddress, buf, len, it->second->m_options);
	}

	virtual void threadStart()
	{
	}

	virtual void threadEnd()
	{
	}
	
	//
	// TCP events
	//
	virtual void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printf("tcpConnectRequest id=%I64u\n", id);

		sockaddr * pAddr = (sockaddr*)pConnInfo->remoteAddress;
		int addrLen = (pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

		// Don't redirect the connection if it is already redirected
		if (memcmp(pAddr, g_proxyAddress, addrLen) == 0)
		{
			printf("tcpConnectRequest id=%I64u bypass already redirected\n", id);
			return;
		}

		if (g_processNames.size() > 0)
		{
			if (!checkProcessName(pConnInfo->processId))
			{
				printf("tcpConnectRequest id=%I64u bypass wrong process\n", id);
				return;
			}
		}

		if (!m_tcpProxy.isIPFamilyAvailable(pAddr->sa_family))
		{
			printf("tcpConnectRequest id=%I64u bypass ipFamily %d\n", id, pAddr->sa_family);
			return;
		}

		m_tcpProxy.setConnInfo(pConnInfo);

		// Redirect the connection
		if (pAddr->sa_family == AF_INET)
		{
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			addr.sin_port = m_tcpProxy.getPort();

			memcpy(pConnInfo->remoteAddress, &addr, sizeof(addr));
		} else
		{
			sockaddr_in6 addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			addr.sin6_addr.u.Byte[15] = 1;
			addr.sin6_port = m_tcpProxy.getPort();

			memcpy(pConnInfo->remoteAddress, &addr, sizeof(addr));
		}

		// Specify current process id to avoid blocking connection redirected to local proxy
		pConnInfo->processId = GetCurrentProcessId();
	}

	virtual void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printConnInfo(true, id, pConnInfo);
		fflush(stdout);
	}

	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printConnInfo(false, id, pConnInfo);
		fflush(stdout);
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		printf("tcpReceive id=%I64u len=%d\n", id, len);
		// Send the packet to application
		nf_tcpPostReceive(id, buf, len);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		printf("tcpSend id=%I64u len=%d\n", id, len);
		// Send the packet to server
		nf_tcpPostSend(id, buf, len);
	}

	virtual void tcpCanReceive(ENDPOINT_ID id)
	{
	}

	virtual void tcpCanSend(ENDPOINT_ID id)
	{
	}
	
	//
	// UDP events
	//

	virtual void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		printAddrInfo(true, id, pConnInfo);
		fflush(stdout);

		if (g_processNames.size() > 0)
		{
			if (checkProcessName(pConnInfo->processId))
			{
				AutoLock lock(m_cs);
				m_filteredUdpIds.insert(id);
			}
		} else
		{
			AutoLock lock(m_cs);
			m_filteredUdpIds.insert(id);
		}
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
		printf("udpConnectRequest id=%I64u\n", id);
		fflush(stdout);
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		printAddrInfo(false, id, pConnInfo);
		fflush(stdout);

		m_udpProxy.deleteProxyConnection(id);

		AutoLock lock(m_cs);

		tUdpCtxMap::iterator it = m_udpCtxMap.find(id);
		if (it != m_udpCtxMap.end())
		{
			delete it->second;
			m_udpCtxMap.erase(it);
		}
	
		m_filteredUdpIds.erase(id);
	}

	virtual void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{	
		char remoteAddr[MAX_PATH];
		DWORD dwLen;
		
		dwLen = sizeof(remoteAddr);
		WSAAddressToString((sockaddr*)remoteAddress, 
				(((sockaddr*)remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);

		printf("udpReceive id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);
//		fflush(stdout);

		// Send the packet to application
		nf_udpPostReceive(id, remoteAddress, buf, len, options);
	}

	virtual void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		char remoteAddr[MAX_PATH];
		DWORD dwLen;
		
		dwLen = sizeof(remoteAddr);
		WSAAddressToString((sockaddr*)remoteAddress, 
				(((sockaddr*)remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);

		printf("udpSend id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);
//		fflush(stdout);


		{
			AutoLock lock(m_cs);

			tIdSet::iterator itid = m_filteredUdpIds.find(id);
			if (itid == m_filteredUdpIds.end())
			{
				nf_udpPostSend(id, remoteAddress, buf, len, options);
				return;
			}

			tUdpCtxMap::iterator it = m_udpCtxMap.find(id);
			if (it == m_udpCtxMap.end())
			{
				if (!m_udpProxy.createProxyConnection(id))
					return;

				m_udpCtxMap[id] = new UDP_CONTEXT(options);
			}
		}

		{
			int addrLen = (((sockaddr*)remoteAddress)->sa_family == AF_INET)? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
			if (!m_udpProxy.udpSend(id, (char*)buf, len, (char*)remoteAddress, addrLen))
			{
				nf_udpPostSend(id, remoteAddress, buf, len, options);
			}
		}
	}

	virtual void udpCanReceive(ENDPOINT_ID id)
	{
	}

	virtual void udpCanSend(ENDPOINT_ID id)
	{
	}
};

void usage()
{
	printf("Usage: SocksRedirector.exe -r IP:port [-p \"<process names>\"] [-user <proxy user name>] [-password <proxy user password>]\n" \
		"IP:port : tunnel TCP/UDP traffic via SOCKS proxy using specified IP:port\n" \
		"<process names> : redirect the traffic of the specified processes (it is possible to specify multiple names divided by ',')\n" \
		);
	exit(0);
}

int main(int argc, char* argv[])
{
	EventHandler eh;
	NF_RULE rule;
	WSADATA wsaData;

	// This call is required for WSAAddressToString
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	DBGLogger::instance().init("SocksRedirectorLog.txt");
#endif

	memset(&g_proxyAddress, 0, sizeof(g_proxyAddress));

	if (argc < 2)
		usage();

	for (int i=1; i < argc; i += 2)
	{
		if (stricmp(argv[i], "-r") == 0)
		{
			int err, addrLen;

			addrLen = sizeof(g_proxyAddress);
			err = WSAStringToAddress(argv[i+1], AF_INET, NULL, (LPSOCKADDR)&g_proxyAddress, &addrLen);
			if (err < 0)
			{
				addrLen = sizeof(g_proxyAddress);
				err = WSAStringToAddress(argv[i+1], AF_INET6, NULL, (LPSOCKADDR)&g_proxyAddress, &addrLen);
				if (err < 0)
				{
					printf("WSAStringToAddress failed, err=%d", WSAGetLastError());
					usage();
				}
			}

			printf("Redirect to: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-p") == 0)
		{
			parseValue(argv[i+1], g_processNames);

			printf("Process name(s): %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-user") == 0)
		{
			g_userName = argv[i+1];

			printf("User name: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-password") == 0)
		{
			g_userPassword = argv[i+1];

			printf("User password: %s\n", argv[i+1]);
		} else
		{
			usage();
		}
	}

	printf("Press enter to stop...\n\n");

	if (!eh.init())
	{
		printf("Failed to initialize the event handler");
		return -1;
	}

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, &eh) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver");
		return -1;
	}

	// Bypass local traffic
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_ALLOW;
	rule.ip_family = AF_INET;
	*((unsigned long*)rule.remoteIpAddress) = inet_addr("127.0.0.1");
	*((unsigned long*)rule.remoteIpAddressMask) = inet_addr("255.0.0.0");
	nf_addRule(&rule, FALSE);

	// Filter UDP packets
	memset(&rule, 0, sizeof(rule));
	rule.ip_family = AF_INET;
	rule.protocol = IPPROTO_UDP;
	rule.filteringFlag = NF_FILTER;
	nf_addRule(&rule, FALSE);


	// Filter TCP connect requests
	memset(&rule, 0, sizeof(rule));
//	rule.ip_family = AF_INET;
	rule.protocol = IPPROTO_TCP;
	rule.direction = NF_D_OUT;
//	rule.remotePort = htons(443);
	rule.filteringFlag = NF_INDICATE_CONNECT_REQUESTS;
	nf_addRule(&rule, FALSE);

	// Wait for enter
	getchar();

	// Free the library
	nf_free();

	eh.free();

	::WSACleanup();

	return 0;
}


/**
* Print the address information
**/
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	sockaddr * pAddr;
	DWORD dwLen;
	char processName[MAX_PATH] = "";
	
	pAddr = (sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				localAddr, 
				&dwLen);
		
	if (created)
	{
		if (!nf_getProcessName(pConnInfo->processId, processName, sizeof(processName)/sizeof(processName[0])))
		{
			processName[0] = '\0';
		}

		printf("udpCreated id=%I64u pid=%d local=%s\n\tProcess: %s\n",
			id,
			pConnInfo->processId, 
			localAddr, 
			processName);
	} else
	{
		printf("udpClosed id=%I64u pid=%d local=%s\n",
			id,
			pConnInfo->processId, 
			localAddr);
	}

}

/**
* Print the connection information
**/
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	char remoteAddr[MAX_PATH] = "";
	DWORD dwLen;
	sockaddr * pAddr;
	char processName[MAX_PATH] = "";
	
	pAddr = (sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				localAddr, 
				&dwLen);

	pAddr = (sockaddr*)pConnInfo->remoteAddress;
	dwLen = sizeof(remoteAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);
	
	if (connected)
	{
		if (!nf_getProcessName(pConnInfo->processId, processName, sizeof(processName)/sizeof(processName[0])))
		{
			processName[0] = '\0';
		}

		printf("tcpConnected id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s (conn.table size %d)\n\tProcess: %s\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr,
			nf_getConnCount(),
			processName);
	} else
	{
		printf("tcpClosed id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s (conn.table size %d)\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr,
			nf_getConnCount());
	}

}

