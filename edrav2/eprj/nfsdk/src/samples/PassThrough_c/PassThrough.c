/**
*	The sample filters all TCP connections and UDP datagrams in pass-through mode,
*	and prints the information about all called events to standard console output.
**/

#include "stdafx.h"
#include <crtdbg.h>
#include "nfapi.h"

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

// Forward declarations
void printConnInfo(BOOL connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);
void printAddrInfo(BOOL created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);

//
//	API event handlers
//

void threadStart()
{
	printf("threadStart\n");
	fflush(stdout);
	// Initialize thread specific stuff
}

void threadEnd()
{
	printf("threadEnd\n");
	// Uninitialize thread specific stuff
}
	
//
// TCP events
//

void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	printf("tcpConnectRequest id=%I64u\n", id);
}

void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	printConnInfo(TRUE, id, pConnInfo);
	fflush(stdout);
}

void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	printConnInfo(FALSE, id, pConnInfo);
	fflush(stdout);
}

void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
{	
	printf("tcpReceive id=%I64u len=%d\n", id, len);
	fflush(stdout);
	// Send the packet to application
	nf_tcpPostReceive(id, buf, len);
}

void tcpSend(ENDPOINT_ID id, const char * buf, int len)
{
	printf("tcpSend id=%I64u len=%d\n", id, len);
	fflush(stdout);

	// Send the packet to server
	nf_tcpPostSend(id, buf, len);
}

void tcpCanReceive(ENDPOINT_ID id)
{
	printf("tcpCanReceive id=%I64d\n", id);
	fflush(stdout);
}

void tcpCanSend(ENDPOINT_ID id)
{
	printf("tcpCanSend id=%I64d\n", id);
	fflush(stdout);
}
	
//
// UDP events
//

void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	printAddrInfo(TRUE, id, pConnInfo);
	fflush(stdout);
}

void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
{
	printf("udpConnectRequest id=%I64d\n", id);
	fflush(stdout);
}

void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	printAddrInfo(FALSE, id, pConnInfo);
	fflush(stdout);
}

void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
{	
	char remoteAddr[MAX_PATH];
	DWORD dwLen;
	
	dwLen = sizeof(remoteAddr);
	WSAAddressToString((struct sockaddr*)remoteAddress, 
			(((struct sockaddr*)remoteAddress)->sa_family == AF_INET6)? 
				sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in), 
			NULL, 
			remoteAddr, 
			&dwLen);

	printf("udpReceive id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);

	fflush(stdout);

	// Send the packet to application
	nf_udpPostReceive(id, remoteAddress, buf, len, options);
}

void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
{
	char remoteAddr[MAX_PATH];
	DWORD dwLen;
	
	dwLen = sizeof(remoteAddr);
	WSAAddressToString((struct sockaddr*)remoteAddress, 
			(((struct sockaddr*)remoteAddress)->sa_family == AF_INET6)? 
				sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in), 
			NULL, 
			remoteAddr, 
			&dwLen);

	printf("udpSend id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);
	
	fflush(stdout);

	// Send the packet to server
	nf_udpPostSend(id, remoteAddress, buf, len, options);
}

void udpCanReceive(ENDPOINT_ID id)
{
	printf("udpCanReceive id=%I64d\n", id);
	fflush(stdout);
}

void udpCanSend(ENDPOINT_ID id)
{
	printf("udpCanSend id=%I64d\n", id);
	fflush(stdout);
}

int main(int argc, char* argv[])
{
	NF_EventHandler eh = { 
		threadStart,
		threadEnd,
		tcpConnectRequest,
		tcpConnected,
		tcpClosed,
		tcpReceive,
		tcpSend,
		tcpCanReceive,
		tcpCanSend,
		udpCreated,
		udpConnectRequest,
		udpClosed,
		udpReceive,
		udpSend,
		udpCanReceive,
		udpCanSend
	};
	
	NF_RULE rule;
	WSADATA wsaData;

	// This call is required for WSAAddressToString
    WSAStartup(MAKEWORD(2, 2), &wsaData);

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, &eh) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver");
		return -1;
	}

	// Filter all TCP/UDP traffic
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_FILTER;
	nf_addRule(&rule, TRUE);

	printf("Press any key to stop...\n\n");

	// Wait for any key
	getchar();

	// Free the library
	nf_free();

	WSACleanup();

	return 0;
}

/**
* Print the connection information
**/
void printConnInfo(BOOL connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	char remoteAddr[MAX_PATH] = "";
	DWORD dwLen;
	struct sockaddr * pAddr;
	char processName[MAX_PATH] = "";
	
	pAddr = (struct sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? 
					sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in), 
				NULL, 
				localAddr, 
				&dwLen);

	pAddr = (struct sockaddr*)pConnInfo->remoteAddress;
	dwLen = sizeof(remoteAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? 
					sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in), 
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

/**
* Print the address information
**/
void printAddrInfo(BOOL created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	struct sockaddr * pAddr;
	DWORD dwLen;
	char processName[MAX_PATH] = "";
	
	pAddr = (struct sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToString((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? 
					sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in), 
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
