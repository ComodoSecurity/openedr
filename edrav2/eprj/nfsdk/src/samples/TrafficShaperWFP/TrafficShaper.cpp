/**
*	Limits the bandwidth for the specified process
*	Usage: TrafficShaperWFP.exe <process name> <limit>
*	<process name> - short process name, e.g. firefox.exe
*	<limit> - network IO limit in bytes per second for all instances of the specified process
*
*	Examples: 
*
*	TrafficShaperWFP.exe firefox.exe 10000
*	Limits TCP/UDP traffic to 10000 bytes per second for all instances of Firefox
*
*	TrafficShaperWFP.exe firefox.exe 0
*	Counts TCP/UDP traffic for all instances of Firefox without limitations
**/

#include "stdafx.h"
#include <crtdbg.h>
#include <set>
#include "nfapi.h"
#include "sync.h"

using namespace nfapi;

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

wchar_t g_processName[_MAX_PATH] = L"";
size_t g_processNameLen = 0;
unsigned int g_flowControlHandle = 0;

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
		wcscpy(processNameLong, processName);
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
		if (checkProcessName(pConnInfo->processId))
		{
			nf_setTCPFlowCtl(id, g_flowControlHandle);
//			printf("tcpConnected[%I64u] set flow limit\n", id);
			return;
		}
	}

	virtual void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
	}

	virtual void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		nf_tcpPostReceive(id, buf, len);
	}

	virtual void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
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
		if (checkProcessName(pConnInfo->processId))
		{
			nf_setUDPFlowCtl(id, g_flowControlHandle);
//			printf("udpCreated[%I64u] set flow limit\n", id);
			return;
		}
	}

	virtual void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
	}

	virtual void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
	}

	virtual void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{	
		// Send the packet to application
		nf_udpPostReceive(id, remoteAddress, buf, len, options);
	}

	virtual void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		// Send the packet to server
		nf_udpPostSend(id, remoteAddress, buf, len, options);
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
	printf("Usage: TrafficShaper.exe <process name> <limit>\n" \
			"\t<process name> - short process name, e.g. firefox.exe\n" \
			"\t<limit> - network IO limit in bytes per second for all instances of the specified process");
}

void logStatistics()
{
	NF_FLOWCTL_STAT stat = {0}, oldStat = {0};
	DWORD ts = 0, tsSpeed = 0;
	const int scd = 4;
	unsigned __int64  inSpeed = 0, outSpeed = 0;

	printf("Statistics (in bytes [speed], out bytes [speed]):\n");

	for (;;)
	{
		Sleep(100);
		
		if (kbhit())
			return;

		if ((GetTickCount() - ts) < 1000)
			continue;

		ts = GetTickCount();

		if (nf_getFlowCtlStat(g_flowControlHandle, &stat) == NF_STATUS_SUCCESS)
		{
			if ((ts - tsSpeed) >= scd * 1000)
			{
				inSpeed = (stat.inBytes - oldStat.inBytes) / scd;
				outSpeed = (stat.outBytes - oldStat.outBytes) / scd;
				tsSpeed = ts;
				oldStat = stat;
			}

			printf("in %I64u [%I64u], out %I64u [%I64u] \t\t\t\t\r", 
				stat.inBytes, inSpeed, 
				stat.outBytes, outSpeed);
		}
	}
}


int _tmain(int argc, wchar_t* argv[])
{
	EventHandler eh;
	NF_RULE rule;
	WSADATA wsaData;
	unsigned int ioLimit;

	if (argc < 2)
	{
		usage();
		return -1;
	}

	wcscpy(g_processName, argv[1]);
	g_processNameLen = wcslen(g_processName);
	ioLimit = _wtoi(argv[2]);

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

	NF_FLOWCTL_DATA data;
	data.inLimit = ioLimit;
	data.outLimit = ioLimit;

	if (nf_addFlowCtl(&data, &g_flowControlHandle) != NF_STATUS_SUCCESS)
	{
		printf("Unable to add flow control context");
		return -1;
	}

	// Bypass local traffic
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_ALLOW;
	rule.ip_family = AF_INET;
	*((unsigned long*)rule.remoteIpAddress) = inet_addr("127.0.0.1");
	*((unsigned long*)rule.remoteIpAddressMask) = inet_addr("255.0.0.0");
	nf_addRule(&rule, FALSE);

	// Control flow for other TCP/UDP traffic
	memset(&rule, 0, sizeof(rule));
	rule.filteringFlag = NF_CONTROL_FLOW;
	nf_addRule(&rule, FALSE);

	printf("Press any key to stop...\n\n");

	// Display statistics and wait for any key
	logStatistics();

	// Free the library
	nf_free();

	::WSACleanup();

	return 0;
}

