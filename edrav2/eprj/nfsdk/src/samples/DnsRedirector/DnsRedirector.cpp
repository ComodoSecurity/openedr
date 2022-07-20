/**
*	This sample redirects DNS requests to opendns.com server.
**/

#include "stdafx.h"
#include <crtdbg.h>
#include <process.h>
#include <map>
#include <queue>
#include "nfapi.h"
#include "sync.h"

using namespace nfapi;


// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

// Forward declarations
void printAddrInfo(bool created, ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);

#define REDIRECT_TO "208.67.222.220"

struct DNS_REQUEST
{
	DNS_REQUEST(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		m_id = id;
		
		memcpy(m_remoteAddress, remoteAddress, NF_MAX_ADDRESS_LENGTH);
		
		if (buf)
		{
			m_buf = new char[len];
			memcpy(m_buf, buf, len);
			m_len = len;
		} else
		{
			m_buf = NULL;
			len = 0;
		}

		if (options)
		{
			m_options = (PNF_UDP_OPTIONS)new char[sizeof(NF_UDP_OPTIONS) + options->optionsLength];
			memcpy(m_options, options, sizeof(NF_UDP_OPTIONS) + options->optionsLength - 1);
		} else
		{
			m_options = NULL;
		}
	}

	~DNS_REQUEST()
	{
		if (m_buf)
			delete[] m_buf;
		if (m_options)
			delete[] m_options;
	}
	
	ENDPOINT_ID m_id;
	unsigned char m_remoteAddress[NF_MAX_ADDRESS_LENGTH];
	char * m_buf;
	int m_len;
	PNF_UDP_OPTIONS m_options;
};

class DnsResolver
{
public:
	DnsResolver()
	{
		m_stopEvent.Attach(CreateEvent(NULL, TRUE, FALSE, NULL));
	}
	
	~DnsResolver()
	{
		free();
	}

	bool init(int threadCount)
	{
		HANDLE hThread;
		unsigned threadId;
		int i;

		ResetEvent(m_stopEvent);

		if (threadCount <= 0)
		{
			SYSTEM_INFO sysinfo;
			GetSystemInfo( &sysinfo );

			threadCount = sysinfo.dwNumberOfProcessors;
			if (threadCount == 0)
			{
				threadCount = 1;
			}
		}

		for (i=0; i<threadCount; i++)
		{
			hThread = (HANDLE)_beginthreadex(0, 0,
						 _threadProc,
						 (LPVOID)this,
						 0,
						 &threadId);

			if (hThread != 0 && hThread != (HANDLE)(-1L))
			{
				m_threads.push_back(hThread);
			}
		}

		return true;
	}

	void free()
	{
		SetEvent(m_stopEvent);

		for (tThreads::iterator it = m_threads.begin();
			it != m_threads.end();
			it++)
		{
			WaitForSingleObject(*it, INFINITE);
			CloseHandle(*it);
		}

		m_threads.clear();

		while (!m_dnsRequestQueue.empty())
		{
			DNS_REQUEST * p = m_dnsRequestQueue.front();
			delete p;
			m_dnsRequestQueue.pop();
		}
	}

	void addRequest(DNS_REQUEST * pRequest)
	{
		AutoLock lock(m_cs);
		m_dnsRequestQueue.push(pRequest);
		SetEvent(m_jobAvailableEvent);
	}

protected:

	void handleRequest(DNS_REQUEST * pRequest, SOCKET s)
	{
		sockaddr_in addr;
		int len;

		printf("DnsResolver::handleRequest() id=%I64u\n", pRequest->m_id);

		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(53);
		addr.sin_addr.S_un.S_addr = inet_addr(REDIRECT_TO);

		len = sendto(s, pRequest->m_buf, pRequest->m_len, 0, (sockaddr*)&addr, sizeof(addr));
		if (len != SOCKET_ERROR)
		{
			fd_set fdr, fde;
			timeval tv;

			FD_ZERO(&fdr);
			FD_SET(s, &fdr);
			FD_ZERO(&fde);
			FD_SET(s, &fde);
			
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			len = select(1, &fdr, NULL, &fde, &tv);
			if (len != SOCKET_ERROR)
			{
				if (FD_ISSET(s, &fdr))
				{
					char result[1024];
					int fromLen;

					fromLen = sizeof(addr);
					len = recvfrom(s, result, (int)sizeof(result), 0, (sockaddr*)&addr, &fromLen);
					if (len != SOCKET_ERROR)
					{
						nf_udpPostReceive(pRequest->m_id, 
							pRequest->m_remoteAddress,
							result,
							len,
							pRequest->m_options);

						printf("DnsResolver::handleRequest() id=%I64u succeeded, len=%d\n", pRequest->m_id, len);
					} else
					{
						printf("DnsResolver::handleRequest() id=%I64u recvfrom error=%d\n", pRequest->m_id, GetLastError());
					}
				} else
				{
					printf("DnsResolver::handleRequest() id=%I64u no data\n", pRequest->m_id);
				}
			} else
			{
				printf("DnsResolver::handleRequest() id=%I64u select error=%d\n", pRequest->m_id, GetLastError());
			}
		} else
		{
			printf("DnsResolver::handleRequest() id=%I64u sendto error=%d\n", pRequest->m_id, GetLastError());
		}

	}

	void threadProc()
	{
		HANDLE handles[] = { m_jobAvailableEvent, m_stopEvent };
		DNS_REQUEST * pRequest;

		SOCKET s;

		s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET)
			return;

		for (;;)
		{
			DWORD res = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
			
			if (res == (WAIT_OBJECT_0+1))
				break;

			for (;;)
			{
				{
					AutoLock lock(m_cs);
					if (m_dnsRequestQueue.empty())
					{
						break;
					}

					pRequest = m_dnsRequestQueue.front();
					m_dnsRequestQueue.pop();
				}

				handleRequest(pRequest, s);

				delete pRequest;
			}
		}

		closesocket(s);
	}

	static unsigned WINAPI _threadProc(void* pData)
	{
		(reinterpret_cast<DnsResolver*>(pData))->threadProc();
		return 0;
	}

private:
	typedef std::vector<HANDLE> tThreads;
	tThreads m_threads;

	typedef std::queue<DNS_REQUEST*> tDnsRequestQueue;
	tDnsRequestQueue m_dnsRequestQueue;

	nfapi::AutoEventHandle m_jobAvailableEvent;
	nfapi::AutoHandle m_stopEvent;

	nfapi::AutoCriticalSection m_cs;
};

DnsResolver g_dnsResolver;


//
//	API events handler
//
class EventHandler : public NF_EventHandler
{

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
	}

	virtual void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
	}

	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
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

		printf("udpSend id=%I64u len=%d remoteAddress=%s\n", id, len, remoteAddr);
		fflush(stdout);

		if (((sockaddr_in*)remoteAddress)->sin_addr.S_un.S_addr == inet_addr(REDIRECT_TO))
		{
			nf_udpPostSend(id, remoteAddress, buf, len, options);
			return;
		}

		g_dnsResolver.addRequest(new DNS_REQUEST(id, remoteAddress, buf, len, options));
	}

	virtual void udpCanReceive(ENDPOINT_ID id)
	{
	}

	virtual void udpCanSend(ENDPOINT_ID id)
	{
	}
};

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

	nf_adjustProcessPriviledges();

	printf("Press enter to stop...\n\n");

	g_dnsResolver.init(10);

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, &eh) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver");
		return -1;
	}

	// Filter outgoing IPv4 UDP packets, directed to port 53 (DNS)
	memset(&rule, 0, sizeof(rule));
	rule.ip_family = AF_INET;
	rule.protocol = IPPROTO_UDP;
	rule.direction = NF_D_OUT;
	rule.remotePort = htons(53);
	rule.filteringFlag = NF_FILTER;
	nf_addRule(&rule, TRUE);

	// Wait for any key
	getchar();

	// Free the library
	nf_free();

	g_dnsResolver.free();

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
