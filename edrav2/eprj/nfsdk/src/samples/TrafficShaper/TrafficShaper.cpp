/**
*	Limits the bandwidth for the specified process
*	Usage: TrafficShaper.exe <process name> <limit>
*	<process name> - short process name, e.g. firefox.exe
*	<limit> - network IO limit in bytes per second for all instances of the specified process
*
*	Example: TrafficShaper.exe firefox.exe 10000
*	Limits TCP/UDP traffic to 10000 bytes per second for all instances of Firefox
**/

#include "stdafx.h"
#include <crtdbg.h>
#include <set>
#include "nfapi.h"
#include "sync.h"

using namespace nfapi;

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

struct NET_IO_COUNTERS
{
	DWORD	ioTime;
	__int64	bytesIn;
	__int64	bytesOut;
};

typedef std::set<ENDPOINT_ID> tIds;
tIds g_tcpSet;
tIds g_udpSet;
NET_IO_COUNTERS g_io;
AutoCriticalSection g_cs;

wchar_t g_processName[_MAX_PATH] = L"";
size_t g_processNameLen = 0;
__int64 g_ioLimit = 0;

bool checkProcessName(DWORD processId)
{
	wchar_t processName[MAX_PATH] = L"";
	
	if (!nf_getProcessNameW(processId, processName, sizeof(processName)/sizeof(processName[0])))
	{
		return false;
	}

	wchar_t processNameLong[MAX_PATH] = L"";
	if (!GetLongPathNameW(processName, processNameLong, sizeof(processNameLong)/sizeof(processNameLong[0])))
	{
		return false;
	}

	size_t processNameLongLen = wcslen(processNameLong);

	if (processNameLongLen < g_processNameLen)
		return false;

	return wcsicmp(g_processName, processNameLong + processNameLongLen - g_processNameLen) == 0;
}

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
		if (!checkProcessName(pConnInfo->processId))
		{
			// Do not filter other processes
			nf_tcpDisableFiltering(id);
			return;
		}
		
		AutoLock lock(g_cs);
		g_tcpSet.insert(id);
	}

	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		AutoLock lock(g_cs);
		g_tcpSet.erase(id);
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		// Send the packet to application
		nf_tcpPostReceive(id, buf, len);

		AutoLock lock(g_cs);
		tIds::iterator it = g_tcpSet.find(id);
		if (it != g_tcpSet.end())
		{
			g_io.bytesIn += len;

			if (g_io.bytesIn > g_ioLimit)
			{
				nf_tcpSetConnectionState(*it, TRUE);
			}
		}
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		// Send the packet to server
		nf_tcpPostSend(id, buf, len);

		AutoLock lock(g_cs);
		tIds::iterator it = g_tcpSet.find(id);
		if (it != g_tcpSet.end())
		{
			g_io.bytesOut += len;

			if (g_io.bytesOut > g_ioLimit)
			{
				nf_tcpSetConnectionState(*it, TRUE);
			}
		}
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
		if (!checkProcessName(pConnInfo->processId))
		{
			// Do not filter other processes
			nf_udpDisableFiltering(id);
			return;
		}

		AutoLock lock(g_cs);
		g_tcpSet.insert(id);
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		AutoLock lock(g_cs);
		g_udpSet.erase(id);
	}

	virtual void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{	
		// Send the packet to application
		nf_udpPostReceive(id, remoteAddress, buf, len, options);

		AutoLock lock(g_cs);
		tIds::iterator it = g_udpSet.find(id);
		if (it != g_udpSet.end())
		{
			g_io.bytesIn += len;

			if (g_io.bytesIn > g_ioLimit)
			{
				nf_udpSetConnectionState(*it, TRUE);
			}
		}
	}

	virtual void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		// Send the packet to server
		nf_udpPostSend(id, remoteAddress, buf, len, options);

		AutoLock lock(g_cs);
		tIds::iterator it = g_udpSet.find(id);
		if (it != g_udpSet.end())
		{
			g_io.bytesOut += len;

			if (g_io.bytesOut > g_ioLimit)
			{
				nf_tcpSetConnectionState(*it, TRUE);
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

void shape()
{
	for (;;)
	{
		Sleep(100);
		
		if (kbhit())
			return;

		AutoLock lock(g_cs);

		// Check the time
		if ((GetTickCount() - g_io.ioTime) < 1000)
			continue;

		g_io.ioTime = GetTickCount();

		// Update IO counters
		g_io.bytesIn = (g_io.bytesIn > g_ioLimit)? g_io.bytesIn - g_ioLimit : 0;
		g_io.bytesOut = (g_io.bytesOut > g_ioLimit)? g_io.bytesOut - g_ioLimit : 0;

		// Suspend or resume TCP/UDP sockets belonging to specified application
		BOOL suspend = (g_io.bytesIn > g_ioLimit || g_io.bytesOut > g_ioLimit);

		for (tIds::iterator it = g_tcpSet.begin();
			it != g_tcpSet.end(); it++)
		{
			nf_tcpSetConnectionState(*it, suspend);
		}

		for (tIds::iterator it = g_udpSet.begin();
			it != g_udpSet.end(); it++)
		{
			nf_udpSetConnectionState(*it, suspend);
		}
	}
}

int _tmain(int argc, wchar_t* argv[])
{
	EventHandler eh;
	NF_RULE rule;
	WSADATA wsaData;

	if (argc < 2)
	{
		printf("Usage: TrafficShaper.exe <process name> <limit>\n" \
			"\t<process name> - short process name, e.g. firefox.exe\n" \
			"\t<limit> - network IO limit in bytes per second for all instances of the specified process");
		return -1;
	}

	wcscpy(g_processName, argv[1]);
	g_processNameLen = wcslen(g_processName);
	g_ioLimit = _wtoi(argv[2]);
	
	memset(&g_io, 0, sizeof(g_io));

	// This call is required for WSAAddressToString
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);

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

	// Bypass local traffic
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_ALLOW;
	rule.ip_family = AF_INET;
	*((unsigned long*)rule.remoteIpAddress) = inet_addr("127.0.0.1");
	*((unsigned long*)rule.remoteIpAddressMask) = inet_addr("255.0.0.0");
	nf_addRule(&rule, FALSE);

	// Filter all TCP/UDP traffic
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_FILTER;
	nf_addRule(&rule, FALSE);

	printf("Press any key to stop...\n\n");

	// Limit the bandwidth and wait for any key
	shape();

	// Free the library
	nf_free();

	::WSACleanup();

	return 0;
}

