//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#include "stdafx.h"
#include <crtdbg.h>
#include <process.h>
#include <map>
#include <queue>
#include "sync.h"
#include "SocksTcpProxy.h"

#if defined(_DEBUG) || defined(_RELEASE_LOG)
DBGLogger DBGLogger::dbgLog;
#endif

TcpProxyServer::TCPProxy g_proxy;

using namespace nfapi;

// Forward declarations
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);

//
//	API events handler
//
class EventHandler : public NF_EventHandler
{
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
//		fflush(stdout);

		// Send the packet to application
		g_proxy.tcpPostReceive(id, (char*)buf, len);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		printf("tcpSend id=%I64u len=%d\n", id, len);
//		fflush(stdout);

		// Send the packet to server
		g_proxy.tcpPostSend(id, (char*)buf, len);
	}

	virtual void tcpCanReceive(ENDPOINT_ID id)
	{
		printf("tcpCanReceive id=%I64d\n", id);
//		fflush(stdout);
	}

	virtual void tcpCanSend(ENDPOINT_ID id)
	{
		printf("tcpCanSend id=%I64d\n", id);
//		fflush(stdout);
	}
	
	//
	// UDP events
	//

	virtual void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		printAddrInfo(true, id, pConnInfo);
		fflush(stdout);
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
		printf("udpConnectRequest id=%I64u\n", id);
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		printAddrInfo(false, id, pConnInfo);
		fflush(stdout);
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

		g_proxy.udpPostReceive(id, remoteAddress, buf, len);
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

		g_proxy.udpPostSend(id, remoteAddress, buf, len);
	}

	virtual void udpCanReceive(ENDPOINT_ID id)
	{
		printf("udpCanReceive id=%I64d\n", id);
//		fflush(stdout);
	}

	virtual void udpCanSend(ENDPOINT_ID id)
	{
		printf("udpCanSend id=%I64d\n", id);
//		fflush(stdout);
	}
};

void usage()
{
	printf("Usage: SocksProxyServer.exe [-p port]\n" \
		"port : listen the specified port\n" \
		);
	exit(0);
}

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	EventHandler eh;

	// This call is required for WSAAddressToString
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	DBGLogger::Instance().PutFileName("SocksProxyServerLog.txt");
	DBGLogger::Instance().PutEnable(true);
#endif

	int port = 1080;

	for (int i=1; i < argc; i += 2)
	{
		if (stricmp(argv[i], "-p") == 0)
		{
			port = atoi(argv[i+1]);
			if (port <= 0)
			{
				printf("invalid port\n");
				usage();
			}
		}
	}

	printf("Listening on: %d\n", port);

	g_proxy.setEventHandler(&eh);

	if (!g_proxy.init(htons(port)))
	{
		printf("Cannot initialize proxy\n");
		return -1;
	}

	printf("Press enter to stop...\n\n");

	// Wait for enter
	getchar();

	g_proxy.free();

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

		printf("tcpConnected id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s\n\tProcess: %s\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr,
			processName);
	} else
	{
		printf("tcpClosed id=%I64u flag=%d pid=%d direction=%s local=%s remote=%s\n",
			id,
			pConnInfo->filteringFlag,
			pConnInfo->processId, 
			(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
			localAddr, 
			remoteAddr);
	}

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
