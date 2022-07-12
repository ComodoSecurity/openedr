/**
*	The sample filters all TCP connections and UDP datagrams in pass-through mode,
*	and prints the information about all called events to standard console output.
**/

#include "stdafx.h"
#include <crtdbg.h>
#include <string>
#include <vector>
#include <Strsafe.h>
#include "nfapi.h"
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")

using namespace nfapi;

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

// Forward declarations
void printConnInfo(bool connected, ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);

DWORD getDefaultInterfaceIP()
{
	DWORD err;
	DWORD ifIndex = 0;
	sockaddr_in s;
	DWORD ip = 0;

	memset(&s, 0, sizeof(s));
	s.sin_family = AF_INET;
	s.sin_addr.S_un.S_addr = inet_addr("8.8.8.8");

	err = GetBestInterfaceEx((sockaddr*)&s, &ifIndex);
	if (err == NO_ERROR)
	{
		PIP_ADAPTER_ADDRESSES adapterAddresses = NULL;
		ULONG outBufferLength = 0;
		ULONG retVal = 0, i;

		for (i = 0; i < 5; i++) 
		{
			retVal = GetAdaptersAddresses(
					AF_UNSPEC, 
					0,
					NULL, 
					adapterAddresses, 
					&outBufferLength);
	        
			if (retVal != ERROR_BUFFER_OVERFLOW) 
			{
				break;
			}

			if (adapterAddresses != NULL) 
			{
				free(adapterAddresses);
			}
	        
			adapterAddresses = (PIP_ADAPTER_ADDRESSES)malloc(outBufferLength);
			if (adapterAddresses == NULL) 
			{
				retVal = GetLastError();
				break;
			}
		}

		if (retVal == NO_ERROR) 
		{
			PIP_ADAPTER_ADDRESSES adapterList = adapterAddresses;
			while (adapterList) 
			{
				if (adapterList->IfIndex == ifIndex)
				{
					PIP_ADAPTER_UNICAST_ADDRESS pAddr = (PIP_ADAPTER_UNICAST_ADDRESS)adapterList->FirstUnicastAddress;
					while (pAddr)
					{
						if (pAddr->Address.lpSockaddr->sa_family == AF_INET)
						{
							ip = ((sockaddr_in*)pAddr->Address.lpSockaddr)->sin_addr.S_un.S_addr;
							break;
						}
						pAddr = pAddr->Next;
					}
				}

				if (ip)
					break;

				adapterList = adapterList->Next;
			}
		} 

		if (adapterAddresses != NULL) 
		{
			free(adapterAddresses);
		}
	}

	return ip;
}

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
		fflush(stdout);

		// Send the packet to application
		nf_tcpPostReceive(id, buf, len);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		printf("tcpSend id=%I64u len=%d\n", id, len);
		fflush(stdout);

		// Send the packet to server
		nf_tcpPostSend(id, buf, len);
	}

	virtual void tcpCanReceive(ENDPOINT_ID id)
	{
		printf("tcpCanReceive id=%I64d\n", id);
		fflush(stdout);
	}

	virtual void tcpCanSend(ENDPOINT_ID id)
	{
		printf("tcpCanSend id=%I64d\n", id);
		fflush(stdout);
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

		printf("udpReceive id=%I64u len=%d remoteAddress=%s flags=%x optionsLen=%d\n", id, len, remoteAddr, options->flags, options->optionsLength);
		fflush(stdout);

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

		printf("udpSend id=%I64u len=%d remoteAddress=%s flags=%x optionsLen=%d\n", id, len, remoteAddr, options->flags, options->optionsLength);
		fflush(stdout);

		// Send the packet to server
		nf_udpPostSend(id, remoteAddress, buf, len, options);
	}

	virtual void udpCanReceive(ENDPOINT_ID id)
	{
		printf("udpCanReceive id=%I64d\n", id);
		fflush(stdout);
	}

	virtual void udpCanSend(ENDPOINT_ID id)
	{
		printf("udpCanSend id=%I64d\n", id);
		fflush(stdout);
	}
};

std::wstring toUtf16(std::string const & str)
{
    std::wstring RetVal;

    int SizeReq = MultiByteToWideChar(
        CP_ACP,
        0,
        str.c_str(),
        -1,
        NULL,
        0
        );
    if ( SizeReq )
    {
        std::vector<wchar_t> Buffer(SizeReq);
        SizeReq = MultiByteToWideChar(
            CP_ACP,
            0,
            str.c_str(),
            -1,
            &Buffer[0],
            SizeReq
            );
        if ( SizeReq )
        {
            RetVal = &Buffer[0];
        }
    }
    return RetVal;
}

void usage()
{
	printf("Usage: BindRedirector.exe [-process <process name>] [-pid <process id>] -r IP[:Port]\n" \
		"process name : tail part of the process path\n" \
		"process id : redirect binding for the process with given PID\n" \
		"IP[:Port] : redirect binding to the specified IP, and optionally port number\n" 
		);
	exit(0);
}


int main(int argc, char* argv[])
{
	EventHandler eh;
	NF_BINDING_RULE rule;
	unsigned char	redirectToAddress[NF_MAX_ADDRESS_LENGTH];
	bool redirectAddressSpecified = false;
	WSADATA wsaData;

    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	getDefaultInterfaceIP();

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

	memset(&rule, 0, sizeof(rule));

	for (int i=1; i < argc; i += 2)
	{
		if (stricmp(argv[i], "-process") == 0)
		{
			std::wstring name = toUtf16(argv[i+1]);
			StringCbCopyW(rule.processName, sizeof(rule.processName)/2, name.c_str());
			printf("Process: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-pid") == 0)
		{
			rule.processId = atoi(argv[i+1]);	
			printf("Process Id: %s\n", argv[i+1]);
		} else
		if (stricmp(argv[i], "-r") == 0)
		{
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
				} else
				{
					rule.ip_family = AF_INET6;
					memcpy(rule.newLocalIpAddress, &((sockaddr_in6*)&redirectToAddress)->sin6_addr, 16);
					if (((sockaddr_in6*)&redirectToAddress)->sin6_port != 0)
					{
						rule.newLocalPort = ((sockaddr_in6*)&redirectToAddress)->sin6_port;
					}
				}
			} else
			{
				rule.ip_family = AF_INET;
				memcpy(rule.newLocalIpAddress, &((sockaddr_in*)&redirectToAddress)->sin_addr, 4);
				if (((sockaddr_in*)&redirectToAddress)->sin_port != 0)
				{
					rule.newLocalPort = ((sockaddr_in*)&redirectToAddress)->sin_port;
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
		usage();

	printf("Press enter to stop...\n\n");

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, &eh) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver");
		return -1;
	}

	rule.filteringFlag = NF_FILTER;
	nf_addBindingRule(&rule, FALSE);

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
