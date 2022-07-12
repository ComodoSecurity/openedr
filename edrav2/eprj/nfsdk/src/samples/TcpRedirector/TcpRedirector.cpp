/**
	This sample allows to redirect TCP connections with given parameters to the specified endpoint.
	It also supports redirecting to HTTPS or SOCKS proxy.

	Usage: TcpRedirector.exe [-s|t] [-p Port] [-pid ProcessId] -r IP:Port 
		-t : redirect via HTTPS proxy at IP:Port. The proxy must support HTTP tunneling (CONNECT requests)
		-s : redirect via SOCKS4 proxy at IP:Port. 
		Port : remote port to intercept
		ProcessId : redirect connections of the process with given PID
		ProxyPid : it is necessary to specify proxy process id if the connection is redirected to local proxy
		IP:Port : redirect connections to the specified IP endpoint

	Example:
		
		TcpRedirector.exe -t -p 110 -r 163.15.64.8:3128

		The sample will redirect POP3 connections to HTTPS proxy at 163.15.64.8:3128 with tunneling to real server. 
**/

#include "stdafx.h"
#include <crtdbg.h>
#include "nfapi.h"
#include <map>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib")

using namespace nfapi;

#pragma pack(push, 1)

enum eSOCKS_VERSION
{
	SOCKS_4 = 4,
	SOCKS_5 = 5
};

enum eSOCKS4_COMMAND
{
	S4C_CONNECT = 1,
	S4C_BIND = 2
};

struct SOCKS4_REQUEST
{
	char	version;
	char	command;
	unsigned short port;
	unsigned int ip;
	char userid[1];
};

enum eSOCKS4_RESCODE
{
	S4RC_SUCCESS = 0x5a
};

struct SOCKS4_RESPONSE
{
	char	reserved;
	char	res_code;
	unsigned short reserved1;
	unsigned int reserved2;
};

#pragma pack(pop)

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

// Forward declarations
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);

unsigned char	redirectToAddress[NF_MAX_ADDRESS_LENGTH];
DWORD proxyPid = 0;

enum PROXY_TYPE
{
	PT_NONE,
	PT_HTTPS,
	PT_SOCKS
};

PROXY_TYPE g_proxyType = PT_NONE;

//
//	API events handler
//
class EventHandler : public NF_EventHandler
{
	struct ORIGINAL_CONN_INFO
	{
		unsigned char	remoteAddress[NF_MAX_ADDRESS_LENGTH];
		std::vector<char>	pendedSends;
	};

	typedef std::map<ENDPOINT_ID, ORIGINAL_CONN_INFO> tConnInfoMap;
	tConnInfoMap m_connInfoMap;

	virtual void threadStart()
	{
		printf("threadStart\n");
		fflush(stdout);

		// Initialize thread specific stuff
	}

	virtual void threadEnd()
	{
		printf("threadEnd\n");

		// Uninitialize thread specific stuff
	}
	
	//
	// TCP events
	//

	virtual void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		printf("tcpConnectRequest id=%I64u\n", id);

		sockaddr * pAddr = (sockaddr*)pConnInfo->remoteAddress;
		int addrLen = (pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

		if (g_proxyType != PT_NONE)
		{
			ORIGINAL_CONN_INFO oci;
			memcpy(oci.remoteAddress, pConnInfo->remoteAddress, sizeof(oci.remoteAddress));

			// Save the original remote address
			m_connInfoMap[id] = oci;
		}

		// Redirect the connection if it is not already redirected
		if (memcmp(pAddr, redirectToAddress, addrLen) != 0 &&
			proxyPid != pConnInfo->processId)
		{
			// Change the remote address
			memcpy(pConnInfo->remoteAddress, redirectToAddress, sizeof(pConnInfo->remoteAddress));
			pConnInfo->processId = proxyPid;

			if (g_proxyType != PT_NONE)
			{
				// Filtering is required only for HTTP/SOCKS tunneling.
				// The first incoming packet will be a response from proxy that must be skipped.
				pConnInfo->filteringFlag |= NF_FILTER;
			}
		} 
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

		if (g_proxyType != PT_NONE)
		{
			m_connInfoMap.erase(id);
		}
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		printf("tcpReceive id=%I64u len=%d\n", id, len);
		fflush(stdout);

		if (g_proxyType != PT_NONE)
		{
			tConnInfoMap::iterator it;

			it = m_connInfoMap.find(id);
			if (it != m_connInfoMap.end())
			{
				if (!it->second.pendedSends.empty())
				{
					nf_tcpPostSend(id, &it->second.pendedSends[0], (int)it->second.pendedSends.size());
				}

				m_connInfoMap.erase(id);
				// The first packet is a response from proxy server.
				// Skip it.

				return;
			}
		}

		// Send the packet to application
		nf_tcpPostReceive(id, buf, len);

		// Don't filter the subsequent packets (optimization)
		nf_tcpDisableFiltering(id);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		printf("tcpSend id=%I64u len=%d\n", id, len);
		fflush(stdout);

		if (g_proxyType != PT_NONE)
		{
			tConnInfoMap::iterator it;

			it = m_connInfoMap.find(id);
			
			if (it != m_connInfoMap.end())
			{
				switch (g_proxyType)
				{
				case PT_NONE:
					nf_tcpDisableFiltering(id);
					break;

				case PT_HTTPS:
					{
						char request[200];
						char addrStr[MAX_PATH] = "";
						sockaddr * pAddr;
						DWORD dwLen;
						tConnInfoMap::iterator it;

						// Generate CONNECT request using saved original remote address

						it = m_connInfoMap.find(id);
						if (it == m_connInfoMap.end())
							return;
						
						pAddr = (sockaddr*)&it->second.remoteAddress;
						dwLen = sizeof(addrStr);

						WSAAddressToString((LPSOCKADDR)pAddr, 
									(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
									NULL, 
									addrStr, 
									&dwLen);

						_snprintf(request, sizeof(request), "CONNECT %s HTTP/1.0\r\n\r\n", addrStr);

						// Send the request first
						nf_tcpPostSend(id, request, (int)strlen(request));

						it->second.pendedSends.insert(it->second.pendedSends.end(), buf, buf+len);
						return;
					}
					break;

				case PT_SOCKS:
					{
						SOCKS4_REQUEST request;
						sockaddr_in * pAddr;
						tConnInfoMap::iterator it;

						// Generate SOCKS request using saved original remote address

						it = m_connInfoMap.find(id);
						if (it == m_connInfoMap.end())
							return;
						
						pAddr = (sockaddr_in*)&it->second.remoteAddress;
						if (pAddr->sin_family == AF_INET6)
						{
							return;
						}

						request.version = SOCKS_4;
						request.command = S4C_CONNECT;
						request.ip = pAddr->sin_addr.S_un.S_addr;
						request.port = pAddr->sin_port;
						request.userid[0] = '\0';

						// Send the request first
						nf_tcpPostSend(id, (char*)&request, (int)sizeof(request));

						it->second.pendedSends.insert(it->second.pendedSends.end(), buf, buf+len);

						return;
					}
					break;
				}
			}
		}

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
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
	}

	virtual void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{	
	}

	virtual void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
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
	printf("Usage: TcpRedirector.exe [-t|s] [-p Port] [-pid ProcessId] -r IP:Port\n" \
		"Port : remote port to intercept\n" \
		"ProcessId : redirect connections of the process with given PID\n" \
		"ProxyPid : it is necessary to specify proxy process id if the connection is redirected to local proxy\n" \
		"IP:Port : redirect connections to the specified IP endpoint\n" \
		"-t : turn on tunneling via proxy at IP:Port. The proxy must support HTTP tunneling (CONNECT requests)\n" \
		"-s : redirect via anonymous SOCKS4 proxy at IP:Port.\n "
		);
	exit(0);
}

int main(int argc, char* argv[])
{
	EventHandler eh;
	NF_RULE rule;
	WSADATA wsaData;
	bool redirectAddressSpecified = false;

	// This call is required for WSAAddressToString
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

	memset(&rule, 0, sizeof(rule));
	rule.protocol = IPPROTO_TCP;
	rule.direction = NF_D_OUT;
	rule.filteringFlag = NF_INDICATE_CONNECT_REQUESTS;

	for (int i=1; i < argc; i += 2)
	{
		if (stricmp(argv[i], "-p") == 0)
		{
			rule.remotePort = htons(atoi(argv[i+1]));	
			printf("Remote port: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-pid") == 0)
		{
			rule.processId = atoi(argv[i+1]);	
			printf("Process Id: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-proxypid") == 0)
		{
			proxyPid = atoi(argv[i+1]);	
			printf("Proxy process Id: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-t") == 0)
		{
			g_proxyType = PT_HTTPS;	
			i--;
			printf("Tunnel via HTTP proxy\n");
		} else
		if (stricmp(argv[i], "-s") == 0)
		{
			g_proxyType = PT_SOCKS;	
			i--;
			printf("Tunnel via SOCKS proxy\n");
		} else
		if (stricmp(argv[i], "-r") == 0)
		{
			char * p;
			
			if (!(p = strchr(argv[i+1], ':')))
			{
				usage();
			}

			int err, addrLen;

			memset(&redirectToAddress, 0, sizeof(redirectToAddress));
			
			addrLen = sizeof(redirectToAddress);
			err = WSAStringToAddress(argv[i+1], AF_INET, NULL, (LPSOCKADDR)&redirectToAddress, &addrLen);
			if (err < 0)
			{
				addrLen = sizeof(redirectToAddress);
				err = WSAStringToAddress(argv[i+1], AF_INET6, NULL, (LPSOCKADDR)&redirectToAddress, &addrLen);
				if (err < 0)
				{
					printf("WSAStringToAddress failed, err=%d", WSAGetLastError());
					usage();
				}
			}

			redirectAddressSpecified = true;

			printf("Redirecting to: %s\n", argv[i+1]);
		} else
		{
			usage();
		}
	}

	if (!redirectAddressSpecified)
	{
		usage();
	}

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, &eh) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver");
		return -1;
	}

	nf_addRule(&rule, TRUE);

	printf("Press any key to stop...\n\n");

	// Wait for any key
	getchar();

	// Free the library
	nf_free();

	::WSACleanup();

	return 0;
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

