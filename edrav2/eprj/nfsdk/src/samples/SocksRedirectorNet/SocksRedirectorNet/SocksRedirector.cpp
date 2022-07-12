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

#define SRN_API __declspec(dllexport) 
#define SRN_EXT extern "C" 

using namespace nfapi;

enum eOPTION_TYPE
{
	OT_NONE,
	OT_DRIVER_NAME,
	OT_PROTOCOL,
	OT_PROCESS_NAME,
	OT_REMOTE_ADDRESS,
	OT_REMOTE_PORT,
	OT_ACTION,
	OT_PROXY_ADDRESS,
	OT_PROXY_USER_NAME,
	OT_PROXY_PASSWORD,
};

enum eACTION
{
	ACTION_BYPASS,
	ACTION_REDIRECT,
};

#if defined(_DEBUG) || defined(_RELEASE_LOG)
DBGLogger DBGLogger::dbgLog;
#endif

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

typedef std::vector<std::string> tStrings;

struct SR_NETWORK
{
	unsigned short	ip_family;	// AF_INET for IPv4 and AF_INET6 for IPv6
	unsigned char	addr[NF_MAX_IP_ADDRESS_LENGTH];	
	unsigned char	mask[NF_MAX_IP_ADDRESS_LENGTH];	
};

struct SR_PORT_RANGE
{
	unsigned short from;
	unsigned short to;
};

typedef std::vector<SR_NETWORK> tNetworks;
typedef std::vector<SR_PORT_RANGE> tPorts;

struct SR_REDIRECT_RULE
{
	SR_REDIRECT_RULE()
	{
		clear();
	}

	SR_REDIRECT_RULE(const SR_REDIRECT_RULE & v)
	{
		*this = v;
	}
	
	SR_REDIRECT_RULE & operator = (const SR_REDIRECT_RULE & v)
	{
		action = v.action;
		memcpy(proxyAddress, v.proxyAddress, sizeof(proxyAddress));
		userName = v.userName;
		userPassword = v.userPassword;
		processNames = v.processNames;
		remoteNetworks = v.remoteNetworks;
		remotePorts = v.remotePorts;
		protocol = v.protocol;
		return *this;
	}

	void clear()
	{
		action = ACTION_REDIRECT;
		memset(&proxyAddress, 0, sizeof(proxyAddress));
		userName = "";
		userPassword = "";
		processNames.clear();
		remoteNetworks.clear();
		remotePorts.clear();
		protocol = 0;
	}

	int			action;
	unsigned char	proxyAddress[NF_MAX_ADDRESS_LENGTH];
	std::string userName;
	std::string userPassword;
	tStrings	processNames;
	tNetworks	remoteNetworks;
	tPorts		remotePorts;
	int			protocol;
};

typedef std::vector<SR_REDIRECT_RULE> tRules;

static std::string g_driverName;
static SR_REDIRECT_RULE g_currentRule;
static tRules g_rules;
static AutoCriticalSection g_cs;


inline bool safe_iswhitespace(int c) { return (c == (int) ' ' || c == (int) '\t' || c == (int) '\r' || c == (int) '\n'); }

inline std::string trimWhitespace(std::string str)
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

static bool parseIPNetwork(const std::string & s, SR_NETWORK &ipNet)
{
	int err, addrLen;
	std::string sAddr, sMask;
	unsigned char sa[NF_MAX_ADDRESS_LENGTH];

	size_t pos = s.find(L'/');
	if (pos != std::string::npos)
	{
		sAddr = trimWhitespace(s.substr(0, pos));
		sMask = trimWhitespace(s.substr(pos+1));
	} else
	{
		sAddr = trimWhitespace(s);
	}

	memset(&ipNet, 0, sizeof(ipNet));

	addrLen = sizeof(sa);
	err = WSAStringToAddress((LPSTR)sAddr.c_str(), AF_INET, NULL, (LPSOCKADDR)&sa, &addrLen);
	if (err == 0)
	{
		ipNet.ip_family = AF_INET;
		memcpy(&ipNet.addr, &sa[4], 4);
	} else
	{
		addrLen = sizeof(sa);
		err = WSAStringToAddress((LPSTR)sAddr.c_str(), AF_INET6, NULL, (LPSOCKADDR)&sa, &addrLen);
		if (err == 0)
		{
			ipNet.ip_family = AF_INET6;
			memcpy(&ipNet.addr, &sa[8], 16);
		} else
		{
			return false;
		}
	}

	if (!sMask.empty())
	{
		int nBits = atoi(sMask.c_str());
		int maxBits = (ipNet.ip_family == AF_INET)? 32 : 128;
		
		if ((nBits > 0) && (nBits < maxBits))
		{
			for (int i=0; i < nBits/8; i++)
			{
				ipNet.mask[i] = 0xff;
			}
			if (nBits % 8)
			{
				unsigned char b = 0;
				for (int k = nBits % 8; k > 0; k--)
				{
					b |= 1 << (8 - k);
				}
				ipNet.mask[nBits/8] = b;
			}
		}
	}

	return true;
}

static bool parsePortRange(const std::string & s, SR_PORT_RANGE & pr)
{
	std::string portFrom, portTo;

	size_t pos = s.find(L'-');
	if (pos != std::string::npos)
	{
		portFrom = trimWhitespace(s.substr(0, pos));
		portTo = trimWhitespace(s.substr(pos+1));

		if (portTo.empty())
		{
			portTo = "65535";
		}
	} else
	{
		portFrom = portTo = trimWhitespace(s);
	}

	pr.from = atoi(portFrom.c_str());
	pr.to = atoi(portTo.c_str());

	return true;
}

static std::string getProcessName(DWORD processId)
{
	char processName[_MAX_PATH] = "";

	if (!nf_getProcessNameA(processId, processName, sizeof(processName)/sizeof(processName[0])))
	{
		return "";
	}

	char processNameLong[_MAX_PATH] = "";
	if (!GetLongPathNameA(processName, processNameLong, sizeof(processNameLong)/sizeof(processNameLong[0])))
	{
		strlwr(processName);
		return processName;
	}

	strlwr(processNameLong);
	return processNameLong;
}

static bool findPort(unsigned short port, tPorts & ports)
{
	if (ports.empty())
		return true;

	for (tPorts::iterator it = ports.begin(); it != ports.end(); it++)
	{
		if ((port >= it->from) && (port <= it->to))
		{
			return true;
		}
	}
	return false;
}

static bool isZeroBuffer(const unsigned char * buf, int len)
{
	for (int i=0; i<len; i++)
	{
		if (buf[i] != 0)
			return false;
	}
	return true;
}

static bool isEqualIpAddresses(unsigned short family,
								const unsigned char * pRuleAddress, 
								const unsigned char * pRuleAddressMask,
								const unsigned char * pAddress)
{
	int i;
	int addrLen = (family == AF_INET)? 4 : 16;

	if (isZeroBuffer(pRuleAddress, addrLen))
		return true;

	if (isZeroBuffer(pRuleAddressMask, addrLen))
	{
		for (i=0; i<addrLen; i++)
		{
			if (pRuleAddress[i] != pAddress[i])
				return false;
		}
	} else
	{
		for (i=0; i<addrLen; i++)
		{
			if ((pRuleAddress[i] & pRuleAddressMask[i]) != (pAddress[i] & pRuleAddressMask[i]))
				return false;
		}
	}

	return true;
}


static bool findAddress(unsigned short ip_family, unsigned char * ipAddress, tNetworks & nets)
{
	if (nets.empty())
		return true;

	for (tNetworks::iterator it = nets.begin(); it != nets.end(); it++)
	{
		if (ip_family != it->ip_family)
			continue;

		if (isEqualIpAddresses(ip_family, it->addr, it->mask, ipAddress))
			return true;
	}

	return false;
}

static bool nameMatches(const char * mask, size_t maskLen, const char * name, size_t nameLen)
{
	const char *p, *s;

	s = mask;
	p = name;

	while (maskLen > 0 && nameLen > 0)
	{
		if (*s == '*')
		{
			s--;
			maskLen--;
			while (nameLen > 0)
			{
				if (nameMatches(s, maskLen, p, nameLen))
				{
					return TRUE;
				}
				p--;
				nameLen--;
			}
			return (maskLen == 0);
		}

		if (*s != *p)
			return false;

		s--;
		maskLen--;

		p--;
		nameLen--;
	}

	if (maskLen == 0)
	{
		return true;
	}

	if (maskLen == 1 && nameLen == 0 && *s == '*')
	{
		return true;
	}

	return false;
}

static bool findProcessName(const std::string & name, tStrings & names)
{
	if (names.empty())
		return true;

	for (tStrings::iterator it = names.begin(); it != names.end(); it++)
	{
		if (nameMatches(it->c_str()+it->length()-1, it->length(), name.c_str()+name.length()-1, name.length()))
			return true;
	}
	return false;
}

static bool findTCPRule(PNF_TCP_CONN_INFO pConnInfo, SR_REDIRECT_RULE & rule)
{
	AutoLock lock(g_cs);

	std::string processName;
	unsigned short port;
	unsigned char * ipAddr;

	processName = getProcessName(pConnInfo->processId);

	if (pConnInfo->ip_family == AF_INET)
	{
		sockaddr_in * sockAddr = (sockaddr_in *)pConnInfo->remoteAddress;
		port = ntohs(sockAddr->sin_port);
		ipAddr = (unsigned char *)&sockAddr->sin_addr;
	} else
	{
		sockaddr_in6 * sockAddr = (sockaddr_in6 *)pConnInfo->remoteAddress;
		port = ntohs(sockAddr->sin6_port);
		ipAddr = (unsigned char *)&sockAddr->sin6_addr;
	}

	for (tRules::iterator it = g_rules.begin(); it != g_rules.end(); it++)
	{
		if ((it->protocol != 0) && (it->protocol != IPPROTO_TCP))
			continue;

		if (!findPort(port, it->remotePorts))
			continue;

		if (!findAddress(pConnInfo->ip_family, ipAddr, it->remoteNetworks))
			continue;

		if (!findProcessName(processName, it->processNames))
			continue;

		rule = *it;
		return true;
	}

	return false;
}

static bool findUDPRule(PNF_UDP_CONN_INFO pConnInfo, const unsigned char * remoteAddress, SR_REDIRECT_RULE & rule)
{
	AutoLock lock(g_cs);

	std::string processName;
	unsigned short port;
	unsigned char * ipAddr;

	processName = getProcessName(pConnInfo->processId);

	if (pConnInfo->ip_family == AF_INET)
	{
		sockaddr_in * sockAddr = (sockaddr_in *)remoteAddress;
		port = ntohs(sockAddr->sin_port);
		ipAddr = (unsigned char *)&sockAddr->sin_addr;
	} else
	{
		sockaddr_in6 * sockAddr = (sockaddr_in6 *)remoteAddress;
		port = ntohs(sockAddr->sin6_port);
		ipAddr = (unsigned char *)&sockAddr->sin6_addr;
	}

	for (tRules::iterator it = g_rules.begin(); it != g_rules.end(); it++)
	{
		if ((it->protocol != 0) && (it->protocol != IPPROTO_UDP))
			continue;

		if (!findPort(port, it->remotePorts))
			continue;

		if (!findAddress(pConnInfo->ip_family, ipAddr, it->remoteNetworks))
			continue;

		if (!findProcessName(processName, it->processNames))
			continue;

		rule = *it;
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

	typedef std::map<unsigned __int64, SR_REDIRECT_RULE> tUdpRedirectionMap;
	tUdpRedirectionMap m_udpRedirectionMap;

	UdpProxy::UDPProxy	m_udpProxy;

	AutoCriticalSection m_cs;
public:

	bool init()
	{
		bool result = false;
		
		for (;;)
		{
			if (!m_udpProxy.init(this))
			{
				DbgPrint("Unable to start UDP proxy");
				break;
			}

			if (!m_tcpProxy.init(htons(8888)))
			{
				DbgPrint("Unable to start TCP proxy");
				break;
			}

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
		m_udpRedirectionMap.clear();
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

		DbgPrint("onUdpReceiveComplete id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);

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
		DbgPrint("tcpConnectRequest id=%I64u\n", id);

		sockaddr * pAddr = (sockaddr*)pConnInfo->remoteAddress;
		int addrLen = (pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

		if (!m_tcpProxy.isIPFamilyAvailable(pAddr->sa_family))
		{
			DbgPrint("tcpConnectRequest id=%I64u bypass ipFamily %d\n", id, pAddr->sa_family);
			return;
		}

		if (g_rules.empty())
		{
			DbgPrint("tcpConnectRequest id=%I64u no rules\n", id);
			return;
		}

		{
			SR_REDIRECT_RULE rule;
			
			if (findTCPRule(pConnInfo, rule))
			{
				if (rule.action == ACTION_BYPASS)
				{
					DbgPrint("tcpConnectRequest id=%I64u action: bypass\n", id);
					return;
				}

				DbgPrint("tcpConnectRequest id=%I64u action: redirect\n", id);

				// Don't redirect the connection if it is already redirected
				if (memcmp(pAddr, rule.proxyAddress, addrLen) == 0)
				{
					DbgPrint("tcpConnectRequest id=%I64u bypass already redirected\n", id);
					return;
				}

				TcpProxy::LOCAL_PROXY_CONN_INFO lpci;

				lpci.connInfo = *pConnInfo;
				memcpy(lpci.proxyAddress, rule.proxyAddress, sizeof(lpci.proxyAddress));
				lpci.proxyAddressLen = (((sockaddr*)rule.proxyAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
				lpci.proxyType = TcpProxy::PROXY_SOCKS5;
				lpci.userName = rule.userName;
				lpci.userPassword = rule.userPassword;

				m_tcpProxy.setConnInfo(&lpci);
			} else
			{
				DbgPrint("tcpConnectRequest id=%I64u action: bypass (no rule)\n", id);
				return;
			}
		}

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
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		printConnInfo(true, id, pConnInfo);
#endif
	}

	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		printConnInfo(false, id, pConnInfo);
#endif
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		DbgPrint("tcpReceive id=%I64u len=%d\n", id, len);
		// Send the packet to application
		nf_tcpPostReceive(id, buf, len);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		DbgPrint("tcpSend id=%I64u len=%d\n", id, len);
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
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		printAddrInfo(true, id, pConnInfo);
#endif
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
		DbgPrint("udpConnectRequest id=%I64u\n", id);
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		printAddrInfo(false, id, pConnInfo);
#endif
		m_udpProxy.deleteProxyConnection(id);

		AutoLock lock(m_cs);

		tUdpCtxMap::iterator it = m_udpCtxMap.find(id);
		if (it != m_udpCtxMap.end())
		{
			delete it->second;
			m_udpCtxMap.erase(it);
		}
	
		m_udpRedirectionMap.erase(id);
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

		DbgPrint("udpReceive id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);

		// Send the packet to application
		nf_udpPostReceive(id, remoteAddress, buf, len, options);
	}

	virtual void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		char remoteAddr[MAX_PATH];
		DWORD dwLen;
		
		dwLen = sizeof(remoteAddr);
		WSAAddressToString((sockaddr*)remoteAddress, 
				(((sockaddr*)remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);

		DbgPrint("udpSend id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);
#endif

		{
			AutoLock lock(m_cs);

			tUdpRedirectionMap::iterator itid = m_udpRedirectionMap.find(id);
			if (itid == m_udpRedirectionMap.end())
			{
				SR_REDIRECT_RULE rule;
				NF_UDP_CONN_INFO ci;

				nf_getUDPConnInfo(id, &ci);
				
				if (findUDPRule(&ci, remoteAddress, rule))
				{
					if (rule.action == ACTION_BYPASS)
					{
						DbgPrint("udpSend id=%I64u action: bypass\n", id);
						nf_udpPostSend(id, remoteAddress, buf, len, options);
						return;
					}

					DbgPrint("udpSend id=%I64u action: redirect\n", id);

					sockaddr * pAddr = (sockaddr*)remoteAddress;
					int addrLen = (pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

					// Don't redirect the connection if it is already redirected
					if (memcmp(pAddr, rule.proxyAddress, addrLen) == 0)
					{
						DbgPrint("udpSend id=%I64u bypass already redirected\n", id);
						nf_udpPostSend(id, remoteAddress, buf, len, options);
						return;
					}

					if (!m_udpProxy.createProxyConnection(id, (char*)rule.proxyAddress,
							(((sockaddr*)rule.proxyAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in),
							rule.userName.c_str(), rule.userPassword.c_str()))
					{
						nf_udpPostSend(id, remoteAddress, buf, len, options);
						return;
					}

					m_udpCtxMap[id] = new UDP_CONTEXT(options);
				} else
				{
					DbgPrint("udpSend id=%I64u action: bypass (no rule)\n", id);
					nf_udpPostSend(id, remoteAddress, buf, len, options);
					return;
				}
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

		DbgPrint("udpCreated id=%I64u pid=%d local=%s\n\tProcess: %s\n",
			id,
			pConnInfo->processId, 
			localAddr, 
			processName);
	} else
	{
		DbgPrint("udpClosed id=%I64u pid=%d local=%s\n",
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

		DbgPrint("tcpConnected id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s (conn.table size %d)\n\tProcess: %s\n",
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
		DbgPrint("tcpClosed id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s (conn.table size %d)\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr,
			nf_getConnCount());
	}

}

static EventHandler g_eh;
static bool g_started = false;

SRN_EXT SRN_API void srn_startRule()
{
	DbgPrint("Start rule");

	AutoLock lock(g_cs);
	g_currentRule.clear();
}

SRN_EXT SRN_API void srn_endRule()
{
	DbgPrint("End rule");

	AutoLock lock(g_cs);
	g_rules.push_back(g_currentRule);
}

SRN_EXT SRN_API BOOL srn_addOption(int optionType, const char * optionValue)
{
	AutoLock lock(g_cs);

	std::string v = optionValue? optionValue : "";

	switch (optionType)
	{
	case OT_DRIVER_NAME:
		g_driverName = v;
		DbgPrint("Driver name: %s\n", v.c_str());
		break;
	
	case OT_PROXY_ADDRESS:
		{
			int err, addrLen;

			memset(&g_currentRule.proxyAddress, 0, sizeof(g_currentRule.proxyAddress));

			addrLen = sizeof(g_currentRule.proxyAddress);
			err = WSAStringToAddress((char*)v.c_str(), AF_INET, NULL, (LPSOCKADDR)&g_currentRule.proxyAddress, &addrLen);
			if (err < 0)
			{
				addrLen = sizeof(g_currentRule.proxyAddress);
				err = WSAStringToAddress((char*)v.c_str(), AF_INET6, NULL, (LPSOCKADDR)&g_currentRule.proxyAddress, &addrLen);
				if (err < 0)
				{
					return FALSE;
				}
			}

			DbgPrint("Redirect to: %s\n", v.c_str());
		}
		break;

	case OT_ACTION:
		{
			if (stricmp(v.c_str(), "redirect") == 0)
			{
				g_currentRule.action = ACTION_REDIRECT;
			} else
			if (stricmp(v.c_str(), "bypass") == 0)
			{
				g_currentRule.action = ACTION_BYPASS;
			} else
				return FALSE;

			DbgPrint("Action: %s\n", v.c_str());
		}
		break;

	case OT_PROCESS_NAME:
		if (!v.empty())
		{
			strlwr((char*)v.c_str());
			g_currentRule.processNames.push_back(v);
			DbgPrint("Process name: %s\n", v.c_str());
		}
		break;
	
	case OT_PROTOCOL:
		{
			if (stricmp(v.c_str(), "tcp") == 0)
			{
				g_currentRule.protocol = IPPROTO_TCP;
			} else
			if (stricmp(v.c_str(), "udp") == 0)
			{
				g_currentRule.protocol = IPPROTO_UDP;
			} else
			{
				g_currentRule.protocol = 0;
			}

			DbgPrint("Action: %s\n", v.c_str());
		}
		break;

	case OT_PROXY_USER_NAME:
		g_currentRule.userName = v;
		DbgPrint("Proxy user name: %s\n", v.c_str());
		break;
	
	case OT_PROXY_PASSWORD:
		g_currentRule.userPassword = v;
		DbgPrint("Proxy password: %s\n", v.c_str());
		break;
	
	case OT_REMOTE_ADDRESS:
		{
			SR_NETWORK net;

			if (parseIPNetwork(v, net))
			{
				DbgPrint("Add network: %s\n", v.c_str());
				g_currentRule.remoteNetworks.push_back(net);
				return TRUE;
			}
		}
		break;

	case OT_REMOTE_PORT:
		{
			SR_PORT_RANGE port;

			if (parsePortRange(v, port))
			{
				DbgPrint("Add port: %s\n", v.c_str());
				g_currentRule.remotePorts.push_back(port);
				return TRUE;
			}
		}
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

SRN_EXT SRN_API BOOL srn_clearOptions()
{
	AutoLock lock(g_cs);
	g_driverName = NFDRIVER_NAME;
	g_rules.clear();
	g_currentRule.clear();
	return TRUE;
}

SRN_EXT SRN_API BOOL srn_enable(BOOL start)
{
	if (start)
	{
		NF_RULE rule;

		if (g_started)
			return TRUE;
	
		if (!g_eh.init())
		{
			DbgPrint("Failed to initialize the event handler");
			return FALSE;
		}

		// Initialize the library and start filtering thread
		if (nf_init(g_driverName.c_str(), &g_eh) != NF_STATUS_SUCCESS)
		{
			DbgPrint("Failed to connect to driver");
			return FALSE;
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
		rule.protocol = IPPROTO_TCP;
		rule.direction = NF_D_OUT;
		rule.filteringFlag = NF_INDICATE_CONNECT_REQUESTS;
		nf_addRule(&rule, FALSE);

		g_started = true;

		DbgPrint("Redirecting...");
	} else
	{
		if (!g_started)
			return TRUE;

		nf_free();
		g_eh.free();

		g_started = false;

		DbgPrint("Stopped");
	}

	return TRUE;
}

SRN_EXT SRN_API BOOL srn_init()
{
	WSADATA wsaData;

	// This call is required for WSAAddressToString
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	DBGLogger::instance().init("SocksRedirectorNetLog.txt");
#endif
	srn_clearOptions();

	return TRUE;
}

SRN_EXT void SRN_API srn_free()
{
	srn_enable(FALSE);
	srn_clearOptions();
	::WSACleanup();
}
