//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#include "StdAfx.h"

#define _NF_INTERNALS

#include "nfapi.h"
#include "nfscm.h"
#include "sync.h"
#include "hashtable.h"
#include "mempools.h"
#include "linkedlist.h"
#include <tchar.h>
#include <psapi.h>
#include <Winternl.h>
#include "timers.h"

// EDR_FIX: Send data to user mode API
#include "cmdedr_main.h"


#pragma comment(lib, "psapi.lib")

using namespace nfapi;

// Prime number as a size for hashes
#define DEFAULT_HASH_SIZE			12289

#define MAX_BUFFERED_BYTES			NF_TCP_PACKET_BUF_SIZE

#define MAX_SEND_QUEUE_BYTES		32 * NF_TCP_PACKET_BUF_SIZE
#define MAX_QUEUE_BYTES				8 * 1024 * 1024
#define MAX_OUT_QUEUE_BYTES			8 * 1024 * 1024

// API closes the connections with no IO during 
// the specified number of milliseconds
#define DEFAULT_TCP_TIMEOUT			120 * 60 * 1000

// The minimum delay in milliseconds between checking 
// for timeouted connections
#define TCP_TIMEOUT_CHECK_PERIOD	5 * 1000

#define TCP_TIMEOUT_CHECK_PERIOD_LONG	30 * 60 * 1000

#define DISABLE_FILTERING_TIMEOUT	30 * 1000

typedef struct _NF_DATA_ENTRY
{
	LIST_ENTRY	entry;
	NF_DATA		data;
} NF_DATA_ENTRY, *PNF_DATA_ENTRY;

typedef struct _CONNOBJ
{
	LIST_ENTRY		entry;				// List linkage
	ENDPOINT_ID		id;					// Connection id
	PHASH_TABLE_ENTRY id_next;			// Hash linkage

	LIST_ENTRY		receivesIn;	// Receive packets
	unsigned int	receivesInBytes;	// Number of buffered receive bytes
	LIST_ENTRY		sendsIn;		// Send packets
	unsigned int	sendsInBytes;		// Number of buffered send bytes

	LIST_ENTRY		receivesOut;	// Receive packets
	unsigned int	receivesOutBytes;	// Number of buffered receive bytes
	LIST_ENTRY		sendsOut;		// Send packets
	unsigned int	sendsOutBytes;		// Number of buffered send bytes

	bool			canSendCalled;
	bool			canReceiveCalled;

	NF_FLOWCTL_STAT	flowStat;

	DWORD			lastIoTime;			// Last send or receive time
	NF_TCP_CONN_INFO	connInfo;		// Connection parameters
	bool			suspended;			// true if connection is suspended
	bool			wantSuspend;		// Requested suspend state
	bool			disconnected;
	bool			filteringDisabled;
	bool			disableFiltering;
	bool			indicateReceiveInProgress;
	bool			connected;
	bool			closed;
	bool			deferredDisconnect;
	bool			disconnectedIn;
	bool			disconnectedOut;
} CONNOBJ, *PCONNOBJ;

typedef struct _ADDROBJ
{
	LIST_ENTRY		entry;				// List linkage
	ENDPOINT_ID		id;					// Socket id
	PHASH_TABLE_ENTRY id_next;			// Hash linkage

	LIST_ENTRY		receivesIn;	// Receive packets
	unsigned int	receivesInBytes;	// Number of buffered receive bytes
	LIST_ENTRY		sendsIn;		// Send packets
	unsigned int	sendsInBytes;		// Number of buffered send bytes

	LIST_ENTRY		receivesOut;	// Receive packets
	unsigned int	receivesOutBytes;	// Number of buffered receive bytes
	LIST_ENTRY		sendsOut;		// Send packets
	unsigned int	sendsOutBytes;		// Number of buffered send bytes

	NF_FLOWCTL_STAT	flowStat;

	NF_UDP_CONN_INFO	connInfo;		// Socket parameters
	NF_UDP_CONN_REQUEST connRequest;
	bool			suspended;			// true if socket is suspended
	bool			wantSuspend;		// Requested suspend state
	bool			filteringDisabled;
	bool			indicateReceiveInProgress;
} ADDROBJ, *PADDROBJ;

typedef struct _IPOBJ
{
	LIST_ENTRY		receivesIn;	// Receive packets
	unsigned int	receivesInBytes;	// Number of buffered receive bytes
	LIST_ENTRY		sendsIn;		// Send packets
	unsigned int	sendsInBytes;		// Number of buffered send bytes

	LIST_ENTRY		receivesOut;	// Receive packets
	unsigned int	receivesOutBytes;	// Number of buffered receive bytes
	LIST_ENTRY		sendsOut;		// Send packets
	unsigned int	sendsOutBytes;		// Number of buffered send bytes
} IPOBJ, *PIPOBJ;

#pragma pack(push, 1)

// WinSock types

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned long   u_long;
typedef unsigned int    u_int;

struct in6_addr 
{
    union {
        u_char Byte[16];
        u_short Word[8];
    } u;
};

struct sockaddr_in6 
{
    short   sin6_family;        /* AF_INET6 */
    u_short sin6_port;          /* Transport level port number */
    u_long  sin6_flowinfo;      /* IPv6 flow information */
    struct in6_addr sin6_addr;  /* IPv6 address */
    u_long sin6_scope_id;       /* set of interfaces for a scope */
};

typedef union _sockaddr_gen 
{
    struct sockaddr_in  AddressIn;
    struct sockaddr_in6 AddressIn6;
} sockaddr_gen;

typedef union _gen_addr
{
    struct in_addr sin_addr;
    struct in6_addr sin6_addr;
} gen_addr;

#pragma pack(pop)

// Variables
static bool g_initialized = false;

static LIST_ENTRY			g_lConnections;
static PHASH_TABLE			g_phtConnById = NULL;
static DWORD				g_tcpTimeout = 0;//DEFAULT_TCP_TIMEOUT; 
static DWORD				g_lastCheckForTcpTimeout = 0; 
static LIST_ENTRY			g_lAddresses;
static PHASH_TABLE			g_phtAddrById = NULL;
static IPOBJ				g_ipObj;

static AutoCriticalSection	g_cs;

static AutoEventHandle		g_workThreadStartedEvent;
static AutoEventHandle		g_workThreadStoppedEvent;
static AutoEventHandleM		g_stopEvent;
static AutoCriticalSection	g_csPost;
static AutoEventHandle		g_ioPostEvent;
static AutoEventHandle		g_ioEvent;
static AutoHandle			g_hDevice;
static AutoHandle			g_hWorkThread;
static NF_EventHandler *	g_pEventHandler = NULL;
static char	g_driverName[MAX_PATH] = {0};
static NF_BUFFERS			g_nfBuffers;
static DWORD				g_nThreads = 1;
DWORD	g_flags = 0;
static NF_IPEventHandler *	g_pIPEventHandler = NULL;
static bool					g_useNagle = true;

static Timers				g_timers;

// Forwards
static NF_STATUS nf_tcpSetConnectionStateInternal(PCONNOBJ pConn, BOOL suspend);
static NF_STATUS nf_udpSetConnectionStateInternal(PADDROBJ pAddr, BOOL suspend);

static NF_STATUS nf_start(NF_EventHandler * pHandler);
static void nf_stop();

static NF_STATUS nf_getLocalAddress(ENDPOINT_ID id, unsigned char * addr, int len);

static PCONNOBJ connobj_alloc(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);
static void connobj_free(PCONNOBJ pConn);
static PADDROBJ addrobj_alloc(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);
static void addrobj_free(PADDROBJ pAddr);
static NF_STATUS nf_postData(PNF_DATA pData);

#include "EventQueue.h"
static EventQueue<NFEvent> g_eventQueue;
static EventQueue<NFEventOut> g_eventQueueOut;

bool nfapi::nf_pushInEvent(ENDPOINT_ID id, int code)
{
	bool res;
	NF_DATA data;

	data.id = id;
	data.code = code;
	data.bufferSize = 0;
	
	res = g_eventQueue.push(&data);
	g_eventQueue.processEvents();
	return res;
}

bool nfapi::nf_pushOutEvent(ENDPOINT_ID id, int code)
{
	bool res;
	NF_DATA data;

	data.id = id;
	data.code = code;
	data.bufferSize = 0;
	
	res = g_eventQueueOut.push(&data);
	g_eventQueueOut.processEvents();
	return res;
}

/**
* Allocate and initialize new CONNOBJ
* @return PCONNOBJ 
* @param ENDPOINT_ID id 
* @param PNF_TCP_CONN_INFO pConnInfo 
**/
static PCONNOBJ connobj_alloc(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	PCONNOBJ pConn;

	pConn = (PCONNOBJ)mp_alloc(sizeof(CONNOBJ));
	if (!pConn)
		return NULL;

	memset(pConn, 0, sizeof(CONNOBJ));

	pConn->id = id;
	pConn->connInfo = *pConnInfo;
	pConn->lastIoTime = GetTickCount();

	InitializeListHead(&pConn->receivesIn);
	InitializeListHead(&pConn->sendsIn);
	InitializeListHead(&pConn->receivesOut);
	InitializeListHead(&pConn->sendsOut);

	ht_add_entry(g_phtConnById, (PHASH_TABLE_ENTRY)&pConn->id);

	InsertTailList(&g_lConnections, &pConn->entry);

	return pConn;
}

/**
* Free CONNOBJ
* @return void 
* @param PCONNOBJ pConn 
**/
static void connobj_free(PCONNOBJ pConn)
{
	PNF_DATA_ENTRY pData;

	if (!pConn)
		return;

	while (!IsListEmpty(&pConn->receivesIn))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pConn->receivesIn);        
		mp_free(pData);
	}

	while (!IsListEmpty(&pConn->sendsIn))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pConn->sendsIn);        
		mp_free(pData);
	}

	while (!IsListEmpty(&pConn->receivesOut))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pConn->receivesOut);        
		mp_free(pData);
	}

	while (!IsListEmpty(&pConn->sendsOut))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pConn->sendsOut);        
		mp_free(pData);
	}

	ht_remove_entry(g_phtConnById, pConn->id);

	RemoveEntryList(&pConn->entry);

	mp_free(pConn);
}


/**
* Allocate and initialize new ADDROBJ 
* @return PADDROBJ 
* @param ENDPOINT_ID id 
* @param PNF_UDP_CONN_INFO pConnInfo 
**/
static PADDROBJ addrobj_alloc(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	PADDROBJ pAddr;

	pAddr = (PADDROBJ)mp_alloc(sizeof(ADDROBJ));
	if (!pAddr)
		return NULL;

	memset(pAddr, 0, sizeof(ADDROBJ));

	pAddr->id = id;
	pAddr->connInfo = *pConnInfo;

	InitializeListHead(&pAddr->receivesIn);
	InitializeListHead(&pAddr->sendsIn);
	InitializeListHead(&pAddr->receivesOut);
	InitializeListHead(&pAddr->sendsOut);

	ht_add_entry(g_phtAddrById, (PHASH_TABLE_ENTRY)&pAddr->id);

	InsertTailList(&g_lAddresses, &pAddr->entry);

	return pAddr;
}

/**
* Free ADDROBJ
* @return void 
* @param PADDROBJ pAddr 
**/
static void addrobj_free(PADDROBJ pAddr)
{
	PNF_DATA_ENTRY pData;

	if (!pAddr)
		return;

	while (!IsListEmpty(&pAddr->receivesIn))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pAddr->receivesIn);
		mp_free(pData);
	}

	while (!IsListEmpty(&pAddr->sendsIn))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pAddr->sendsIn);
		mp_free(pData);
	}

	while (!IsListEmpty(&pAddr->receivesOut))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pAddr->receivesOut);
		mp_free(pData);
	}

	while (!IsListEmpty(&pAddr->sendsOut))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&pAddr->sendsOut);
		mp_free(pData);
	}

	ht_remove_entry(g_phtAddrById, pAddr->id);

	RemoveEntryList(&pAddr->entry);

	mp_free(pAddr);
}


static void ipobj_init()
{
	memset(&g_ipObj, 0, sizeof(g_ipObj));

	InitializeListHead(&g_ipObj.receivesIn);
	InitializeListHead(&g_ipObj.sendsIn);
	InitializeListHead(&g_ipObj.receivesOut);
	InitializeListHead(&g_ipObj.sendsOut);
}

static void ipobj_free()
{
	PNF_DATA_ENTRY pData;

	while (!IsListEmpty(&g_ipObj.receivesIn))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&g_ipObj.receivesIn);
		mp_free(pData);
	}

	while (!IsListEmpty(&g_ipObj.sendsIn))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&g_ipObj.sendsIn);
		mp_free(pData);
	}

	while (!IsListEmpty(&g_ipObj.receivesOut))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&g_ipObj.receivesOut);
		mp_free(pData);
	}

	while (!IsListEmpty(&g_ipObj.sendsOut))
	{
		pData = (PNF_DATA_ENTRY)RemoveHeadList(&g_ipObj.sendsOut);
		mp_free(pData);
	}
}


/**
* Send IO packet to driver
* @return NF_STATUS 
* @param PNF_DATA pData 
**/
static NF_STATUS nf_postData(PNF_DATA pData)
{
	OVERLAPPED ol;
	DWORD dwRes;
	DWORD dwWritten = 0;
	HANDLE events[] = { g_ioPostEvent, g_stopEvent };
	NF_READ_RESULT rr;

	rr.length = sizeof(NF_DATA) + pData->bufferSize - 1;

	if (rr.length > g_nfBuffers.outBufLen)
	{
		return NF_STATUS_IO_ERROR;
	}

	if (g_hDevice == INVALID_HANDLE_VALUE || !g_nfBuffers.outBuf)
		return NF_STATUS_NOT_INITIALIZED;

	AutoLock lock(g_csPost);

	memcpy((void*)g_nfBuffers.outBuf, pData, (size_t)rr.length);

	memset(&ol, 0, sizeof(ol));

	ol.hEvent = g_ioPostEvent;

	if (!WriteFile(g_hDevice, &rr, sizeof(rr), NULL, &ol))
	{
		if (GetLastError() != ERROR_IO_PENDING)
			return NF_STATUS_IO_ERROR;
	}

	// Wait for completion
	for (;;)
	{
		dwRes = WaitForMultipleObjects(
						sizeof(events) / sizeof(events[0]),
						events, 
						FALSE,
						INFINITE);

		if (dwRes != WAIT_OBJECT_0)
		{
			CancelIo(g_hDevice);
			return NF_STATUS_FAIL;
		}

		dwRes = WaitForSingleObject(g_stopEvent, 0);
		if (dwRes == WAIT_OBJECT_0)
		{
			CancelIo(g_hDevice);
			return NF_STATUS_FAIL;
		}

		if (!GetOverlappedResult(g_hDevice, &ol, &dwWritten, FALSE))
		{
			return NF_STATUS_FAIL;
		}

		break;
	}

	return (dwWritten)? NF_STATUS_SUCCESS : NF_STATUS_FAIL;
}

void timerTimeout(ENDPOINT_ID id)
{
	nf_pushInEvent(id, NF_TCP_RECEIVE_PUSH);
}


/**
* Pass the event to application
* @return void 
* @param PNF_DATA pData 
**/
static void handleEvent(PNF_DATA pData)
{
	AutoLock lock(g_cs);

	if (!g_pEventHandler)
		return;

	switch (pData->code)
	{

	// TCP events

	case NF_TCP_CONNECTED:
		{
			PCONNOBJ pConn = NULL;
			PHASH_TABLE_ENTRY phte;
	
			phte = ht_find_entry(g_phtConnById, pData->id);
			if (phte)
			{
				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);
				pConn->connInfo = *(PNF_TCP_CONN_INFO)pData->buffer;
			} else
			{
				pConn = connobj_alloc(pData->id, (PNF_TCP_CONN_INFO)pData->buffer);
				if (!pConn)
					return;
			}

			pConn->connected = TRUE;

			nf_getLocalAddress(pData->id, pConn->connInfo.localAddress, sizeof(pConn->connInfo.localAddress));

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;
	
	case NF_TCP_CONNECT_REQUEST:
		{
			connobj_alloc(pData->id, (PNF_TCP_CONN_INFO)pData->buffer);

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_TCP_CLOSED:
		{
			PCONNOBJ pConn = NULL;
			PHASH_TABLE_ENTRY phte;

			if (pData->bufferSize == sizeof(NF_FLOWCTL_STAT))
			{
				phte = ht_find_entry(g_phtConnById, pData->id);
				if (phte)
				{
					pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);
					pConn->flowStat = *(PNF_FLOWCTL_STAT)pData->buffer;
				}
			}

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_TCP_RECEIVE:
		{
			PCONNOBJ pConn = NULL;
			PHASH_TABLE_ENTRY phte;
	
			phte = ht_find_entry(g_phtConnById, pData->id);
			if (phte)
			{
				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);
			} else
			{
				nf_tcpPostReceive(pData->id, pData->buffer, pData->bufferSize);
				break;
			}

			pConn->lastIoTime = GetTickCount();
/*
			if (pConn->disableFiltering)
			{
				nf_tcpPostReceive(pData->id, pData->buffer, pData->bufferSize);
				break;
			}
*/
			PNF_DATA_ENTRY pDataEntry;
			PLIST_ENTRY pPacketList;
			bool packetAdded = false;
			unsigned int * pCounter;
			
			pCounter = &pConn->receivesInBytes;
			pPacketList = &pConn->receivesIn;

			if (pData->bufferSize > 0)
			{
				if (!IsListEmpty(pPacketList))
				{
					pDataEntry = (PNF_DATA_ENTRY)pPacketList->Blink;

					if ((pDataEntry->data.bufferSize + pData->bufferSize) <= MAX_BUFFERED_BYTES)
					{
						if (pData->bufferSize > 0)
						{
							memcpy(pDataEntry->data.buffer + pDataEntry->data.bufferSize, pData->buffer, pData->bufferSize);
						}

						pDataEntry->data.bufferSize += pData->bufferSize;
						(*pCounter) += pData->bufferSize;

						packetAdded = true;
					}
				}
			}

			if (!packetAdded)
			{
				pDataEntry = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + MAX_BUFFERED_BYTES - 1);
				if (!pDataEntry)
					break;

				pDataEntry->data.id = pData->id;
				pDataEntry->data.code = pData->code;
				pDataEntry->data.bufferSize = pData->bufferSize;
				
				if (pData->bufferSize > 0)
				{
					memcpy(pDataEntry->data.buffer, pData->buffer, pData->bufferSize);
				}

				(*pCounter) += pData->bufferSize;

				InsertTailList(pPacketList, &pDataEntry->entry);
			}

			if ((pConn->sendsOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pConn->receivesOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pConn->sendsInBytes >= MAX_QUEUE_BYTES) ||
				(pConn->receivesInBytes >= MAX_QUEUE_BYTES))
			{
				nf_tcpSetConnectionStateInternal(pConn, TRUE);
			}

			if (g_useNagle)
			{
				if (pData->bufferSize < 1400 ||
					pConn->receivesInBytes >= (8 * NF_TCP_PACKET_BUF_SIZE))
				{
					AutoUnlock unlock(g_cs);
					nf_pushInEvent(pData->id, NF_TCP_RECEIVE_PUSH);
				} else
				{
					g_timers.addTimer(pData->id, 1);
				}
			} else
			{
				AutoUnlock unlock(g_cs);
				g_eventQueue.push(pData);
			}
		}
		break;

	case NF_TCP_SEND:
		{
			PCONNOBJ pConn = NULL;
			PHASH_TABLE_ENTRY phte;
			
			phte = ht_find_entry(g_phtConnById, pData->id);
			if (phte)
			{
				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);
			} else
			{
				nf_tcpPostSend(pData->id, pData->buffer, pData->bufferSize);
				break;
			}

			PNF_DATA_ENTRY pDataEntry;
			PLIST_ENTRY pPacketList;
			bool packetAdded = false;
			unsigned int * pCounter;
			
			pCounter = &pConn->sendsInBytes;
			pPacketList = &pConn->sendsIn;

			pConn->lastIoTime = GetTickCount();
/*
			if (pConn->disableFiltering)
			{
				nf_tcpPostSend(pData->id, pData->buffer, pData->bufferSize);
				break;
			}
*/
			if (pData->bufferSize > 0)
			{
				if (!IsListEmpty(pPacketList))
				{
					pDataEntry = (PNF_DATA_ENTRY)pPacketList->Blink;

					if ((pDataEntry->data.bufferSize + pData->bufferSize) <= MAX_BUFFERED_BYTES)
					{
						if (pData->bufferSize > 0)
						{
							memcpy(pDataEntry->data.buffer + pDataEntry->data.bufferSize, pData->buffer, pData->bufferSize);
						}

						pDataEntry->data.bufferSize += pData->bufferSize;

						(*pCounter) += pData->bufferSize;

						packetAdded = true;
					}
				} 
			}

			if (!packetAdded)
			{
				pDataEntry = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + MAX_BUFFERED_BYTES - 1);
				if (!pDataEntry)
					break;

				pDataEntry->data.id = pData->id;
				pDataEntry->data.code = pData->code;
				pDataEntry->data.bufferSize = pData->bufferSize;
				
				if (pData->bufferSize > 0)
				{
					memcpy(pDataEntry->data.buffer, pData->buffer, pData->bufferSize);
				}

				(*pCounter) += pData->bufferSize;

				InsertTailList(pPacketList, &pDataEntry->entry);
			}

			if ((pConn->sendsOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pConn->receivesOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pConn->sendsInBytes >= MAX_SEND_QUEUE_BYTES) ||
				(pConn->receivesInBytes >= MAX_QUEUE_BYTES))
			{
				nf_tcpSetConnectionStateInternal(pConn, TRUE);
			}

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_TCP_CAN_RECEIVE:
		{
			AutoUnlock unlock(g_cs);
			g_eventQueueOut.push(pData);
		}
		break;

	case NF_TCP_CAN_SEND:
		{
			AutoUnlock unlock(g_cs);
			g_eventQueueOut.push(pData);
		}
		break;

	case NF_TCP_REINJECT:
		{
			AutoUnlock unlock(g_cs);
			nf_pushOutEvent(pData->id, NF_TCP_REINJECT);
		}
		break;

	case NF_TCP_DEFERRED_DISCONNECT:
		{
			PCONNOBJ pConn = NULL;
			PHASH_TABLE_ENTRY phte;
	
			phte = ht_find_entry(g_phtConnById, pData->id);
			if (phte)
			{
				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);
				pConn->deferredDisconnect = TRUE;
			}
		}
		break;


	// UDP events

	case NF_UDP_CREATED:
		{
			PADDROBJ pAddr;
			
			// Allocate new ADDROBJ
			pAddr = addrobj_alloc(pData->id, (PNF_UDP_CONN_INFO)pData->buffer);
			
			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;
	
	case NF_UDP_CONNECT_REQUEST:
		{
			PADDROBJ pAddr = NULL;
			PHASH_TABLE_ENTRY phte;
	
			// Find and free ADDROBJ
			phte = ht_find_entry(g_phtAddrById, pData->id);
			if (phte)
			{
				pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);
				pAddr->connRequest = *(PNF_UDP_CONN_REQUEST)pData->buffer;
			}
			
			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_UDP_CLOSED:
		{
			PADDROBJ pAddr = NULL;
			PHASH_TABLE_ENTRY phte;

			if (pData->bufferSize == sizeof(NF_FLOWCTL_STAT))
			{
				phte = ht_find_entry(g_phtAddrById, pData->id);
				if (phte)
				{
					pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);
					pAddr->flowStat = *(PNF_FLOWCTL_STAT)pData->buffer;
				}
			}

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_UDP_RECEIVE:
		{
			PADDROBJ pAddr = NULL;
			PHASH_TABLE_ENTRY phte;
			char * pRemoteAddr = pData->buffer; 
			PNF_DATA_ENTRY pDataEntry;
			PLIST_ENTRY pPacketList;
			unsigned int * pCounter;
			
			// Add new ADDROBJ if it doesn't exist
			phte = ht_find_entry(g_phtAddrById, pData->id);
			if (!phte)
			{
				NF_UDP_CONN_INFO ci;
				memset(&ci, 0, sizeof(ci));		
				ci.ip_family = ((sockaddr_in*)pRemoteAddr)->sin_family;

				pAddr = addrobj_alloc(pData->id, &ci);
				if (!pAddr)
					return;
			} else
			{
				pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);
			}

			pCounter = &pAddr->receivesInBytes;
			pPacketList = &pAddr->receivesIn;

			pDataEntry = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + NF_UDP_PACKET_BUF_SIZE - 1);
			if (!pDataEntry)
				return;

			pDataEntry->data.id = pData->id;
			pDataEntry->data.code = pData->code;
			pDataEntry->data.bufferSize = pData->bufferSize;
			
			if (pData->bufferSize > 0)
			{
				memcpy(pDataEntry->data.buffer, pData->buffer, pData->bufferSize);
			}

			(*pCounter) += pData->bufferSize;

			InsertTailList(pPacketList, &pDataEntry->entry);

			if ((pAddr->sendsOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pAddr->receivesOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pAddr->sendsInBytes >= MAX_QUEUE_BYTES) ||
				(pAddr->receivesInBytes >= MAX_QUEUE_BYTES))
			{
				nf_udpSetConnectionStateInternal(pAddr, TRUE);
			}

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_UDP_SEND:
		{
			PADDROBJ pAddr = NULL;
			PHASH_TABLE_ENTRY phte;
			char * pRemoteAddr = pData->buffer; 
			PNF_DATA_ENTRY pDataEntry;
			PLIST_ENTRY pPacketList;
			unsigned int * pCounter;
			
			// Add new ADDROBJ if it doesn't exist
			phte = ht_find_entry(g_phtAddrById, pData->id);
			if (!phte)
			{
				NF_UDP_CONN_INFO ci;
				memset(&ci, 0, sizeof(ci));		
				ci.ip_family = ((sockaddr_in*)pRemoteAddr)->sin_family;

				pAddr = addrobj_alloc(pData->id, &ci);
				if (!pAddr)
					return;
			} else
			{
				pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);
			}

			pCounter = &pAddr->sendsInBytes;
			pPacketList = &pAddr->sendsIn;

			pDataEntry = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + NF_UDP_PACKET_BUF_SIZE - 1);
			if (!pDataEntry)
				return;

			pDataEntry->data.id = pData->id;
			pDataEntry->data.code = pData->code;
			pDataEntry->data.bufferSize = pData->bufferSize;
			
			if (pData->bufferSize > 0)
			{
				memcpy(pDataEntry->data.buffer, pData->buffer, pData->bufferSize);
			}

			(*pCounter) += pData->bufferSize;

			InsertTailList(pPacketList, &pDataEntry->entry);

			if ((pAddr->sendsOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pAddr->receivesOutBytes >= MAX_OUT_QUEUE_BYTES) ||
				(pAddr->sendsInBytes >= MAX_QUEUE_BYTES) ||
				(pAddr->receivesInBytes >= MAX_QUEUE_BYTES))
			{
				nf_udpSetConnectionStateInternal(pAddr, TRUE);
			}

			AutoUnlock unlock(g_cs);
			g_eventQueue.push(pData);
		}
		break;

	case NF_UDP_CAN_RECEIVE:
		{
			AutoUnlock unlock(g_cs);
			g_eventQueueOut.push(pData);
		}
		break;

	case NF_UDP_CAN_SEND:
		{
			AutoUnlock unlock(g_cs);
			g_eventQueueOut.push(pData);
		}
		break;

	case NF_IP_RECEIVE:
		{
			PNF_IP_PACKET_OPTIONS pOptions;
			char * pPacketData; 
			unsigned long packetDataLen;

			pOptions = (PNF_IP_PACKET_OPTIONS)(pData->buffer);
			pPacketData = pData->buffer + sizeof(NF_IP_PACKET_OPTIONS); 
			packetDataLen = pData->bufferSize - sizeof(NF_IP_PACKET_OPTIONS);

			AutoUnlock unlock(g_cs);
			if (g_pIPEventHandler)
			{
				g_pIPEventHandler->ipReceive(pPacketData, 
					packetDataLen,
					pOptions);
			} else
			{
				nf_ipPostReceive(pPacketData, packetDataLen, pOptions);
			}
		}
		break;

	case NF_IP_SEND:
		{
			PNF_IP_PACKET_OPTIONS pOptions;
			char * pPacketData; 
			unsigned long packetDataLen;

			pOptions = (PNF_IP_PACKET_OPTIONS)(pData->buffer);
			pPacketData = pData->buffer + sizeof(NF_IP_PACKET_OPTIONS); 
			packetDataLen = pData->bufferSize - sizeof(NF_IP_PACKET_OPTIONS);

			AutoUnlock unlock(g_cs);
			if (g_pIPEventHandler)
			{
				g_pIPEventHandler->ipSend(pPacketData, 
					packetDataLen,
					pOptions);
			} else
			{
				nf_ipPostSend(pPacketData, packetDataLen, pOptions);
			}
		}
		break;

	default:
		{
			// EDR_FIX: Send data to user mode API
			if (pData->code & 0x10000)
				cmdedr::processCustomData(pData);
			break;
		}
	}
}

/**
* Close TCP connections having no IO for a long time
**/
static void nf_closeHangingConnections()
{
	PCONNOBJ	pConn, pNextConn;
	DWORD		dwTime;
	DWORD		dwCurrentTime = GetTickCount();

	AutoLock	lock(g_cs);

	if ((dwCurrentTime - g_lastCheckForTcpTimeout) < TCP_TIMEOUT_CHECK_PERIOD)
		return;

	g_lastCheckForTcpTimeout = dwCurrentTime;

	pConn = (PCONNOBJ)g_lConnections.Flink;

	while (pConn != (PCONNOBJ)&g_lConnections)
	{
		if (pConn->connected &&
			(pConn->connInfo.filteringFlag & NF_FILTER) &&
			!pConn->disconnected &&
			!pConn->filteringDisabled)
		{	
			dwTime = dwCurrentTime - pConn->lastIoTime;
			
			if (pConn->deferredDisconnect && 
				 ((pConn->receivesInBytes + pConn->receivesOutBytes) == 0) && 
				 (dwTime > TCP_TIMEOUT_CHECK_PERIOD))
			{
				pConn->deferredDisconnect = FALSE;
				nf_tcpPostReceive(pConn->id, NULL, 0);
			} else
			if (g_tcpTimeout && (dwTime > g_tcpTimeout))
			{
				nf_tcpPostSend(pConn->id, NULL, 0);
				
				pConn->disconnected = TRUE;

				pNextConn = (PCONNOBJ)pConn->entry.Flink;

				NF_DATA data;
				data.id = pConn->id;
				data.code = NF_TCP_CLOSED;
				data.bufferSize = 0;

				g_eventQueue.push(&data);

				g_eventQueue.processEvents();

				pConn = pNextConn;
				
				continue;
			}
		}

		pConn = (PCONNOBJ)pConn->entry.Flink;
	}
}

/**
* Main IO thread
**/
static unsigned WINAPI nf_workThread(void* )
{
	DWORD readBytes;
	PNF_DATA pData;
	OVERLAPPED ol;
	DWORD dwRes;
	NF_READ_RESULT rr;
	HANDLE events[] = { g_ioEvent, g_stopEvent };
	DWORD waitTimeout;
	bool abortBatch;
	int i;

	SetEvent(g_workThreadStartedEvent);

	g_eventQueue.init(g_nThreads);
	g_eventQueueOut.init(1);

	for (;;)
	{
		waitTimeout = 10;
		abortBatch = false;

		g_eventQueue.suspend(true);

		for (i=0; i<8; i++)
		{
			readBytes = 0;

			memset(&ol, 0, sizeof(ol));

			ol.hEvent = g_ioEvent;

			if (!ReadFile(g_hDevice, &rr, sizeof(rr), NULL, &ol))
			{
				if (GetLastError() != ERROR_IO_PENDING)
					goto finish;
			}

			for (;;)
			{
				dwRes = WaitForMultipleObjects(
								sizeof(events) / sizeof(events[0]),
								events, 
								FALSE,
								waitTimeout);
				
				if (dwRes == WAIT_TIMEOUT)
				{
					if (waitTimeout == TCP_TIMEOUT_CHECK_PERIOD)
					{
						nf_closeHangingConnections();
					}

					waitTimeout = TCP_TIMEOUT_CHECK_PERIOD;

					g_eventQueue.suspend(false);
					g_eventQueueOut.processEvents();
					g_eventQueue.processEvents();

					abortBatch = true;

					continue;
				} else
				if (dwRes != WAIT_OBJECT_0)
				{
					goto finish;
				}

				dwRes = WaitForSingleObject(g_stopEvent, 0);
				if (dwRes == WAIT_OBJECT_0)
				{
					goto finish;
				}

				if (!GetOverlappedResult(g_hDevice, &ol, &readBytes, FALSE))
				{
					goto finish;
				}

				break;
			}

			readBytes = (DWORD)rr.length;

			if (readBytes > g_nfBuffers.inBufLen)
			{
				readBytes = (DWORD)g_nfBuffers.inBufLen;
			}

			pData = (PNF_DATA)g_nfBuffers.inBuf;
			
			while (readBytes >= (sizeof(NF_DATA) - 1))
			{
				handleEvent(pData);

				if (!g_useNagle)
				{
					abortBatch = true;
				} else
				if ((pData->code == NF_TCP_RECEIVE ||
					pData->code == NF_TCP_SEND ||
					pData->code == NF_UDP_RECEIVE ||
					pData->code == NF_UDP_SEND)	&&
					pData->bufferSize < 1400)
				{
					abortBatch = true;
				}

				if (readBytes < (sizeof(NF_DATA) - 1 + pData->bufferSize))
				{
					break;
				}

				readBytes -= sizeof(NF_DATA) - 1 + pData->bufferSize;
				pData = (PNF_DATA)(pData->buffer + pData->bufferSize);
			}

			if (abortBatch)
				break;
		}

		g_eventQueue.suspend(false);
		g_eventQueueOut.processEvents();
		g_eventQueue.processEvents();

		nf_closeHangingConnections();

		g_eventQueue.wait(8000);
		g_eventQueueOut.wait(64000);
	}

finish:

	CancelIo(g_hDevice);

	g_eventQueue.free();
	g_eventQueueOut.free();

	SetEvent(g_workThreadStoppedEvent);

	return 0;
}


// TCP

/**
* Change suspended state for TCP connection
* @return NF_STATUS 
* @param ENDPOINT_ID id 
* @param BOOL suspend 
**/
static NF_STATUS nf_tcpSetConnectionStateInternal(PCONNOBJ pConn, BOOL suspend)
{
	PNF_DATA pData;
	NF_STATUS status = NF_STATUS_FAIL;
	AutoLock lock(g_cs);

	if ((BOOL)pConn->suspended == suspend)
		return NF_STATUS_SUCCESS;

	pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA));
	if (!pData)
		return NF_STATUS_FAIL;

	pData->id = pConn->id;
	pData->code = (suspend)? NF_TCP_REQ_SUSPEND : NF_TCP_REQ_RESUME;
	pData->bufferSize = 0;

	status = nf_postData(pData);
	if (status == NF_STATUS_SUCCESS)
	{
		pConn->suspended = (suspend != FALSE);
#ifdef _RELEASE_LOG
		printf("* DbgStatus id=%d, code=%d\n", (int)pConn->id, pData->code);
#endif
	}

	mp_free(pData);

	return status;
}


/**
* 
* @return NF_STATUS 
* @param ENDPOINT_ID id 
* @param BOOL suspend 
**/
NFAPI_API NF_STATUS NFAPI_NS nf_tcpSetConnectionState(ENDPOINT_ID id, BOOL suspend)
{
	NF_STATUS status = NF_STATUS_FAIL;
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);

	phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

	if ((BOOL)pConn->wantSuspend == suspend)
		return NF_STATUS_SUCCESS;

	pConn->wantSuspend = (suspend != FALSE);

	status = nf_tcpSetConnectionStateInternal(pConn, pConn->wantSuspend? TRUE : FALSE);

	return status;
}

/**
* Split the buffer to packets and post the first packet to driver
* @return NF_STATUS 
* @param NF_DATA_CODE code 
* @param ENDPOINT_ID id 
* @param const char * buf 
* @param int len 
**/
static NF_STATUS nf_tcpPostPacket(NF_DATA_CODE code, ENDPOINT_ID id, const char * buf, int len)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	// EDR_FIX: Fix compilation on vs2017
	PNF_DATA_ENTRY pData = NULL;
	PLIST_ENTRY pPacketList;
	unsigned int * pCounter;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!g_phtConnById)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
	{
		return NF_STATUS_INVALID_ENDPOINT_ID;
	}

	pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

	switch (code)
	{
	case NF_TCP_SEND:
		pPacketList = &pConn->sendsOut;
		pCounter = &pConn->sendsOutBytes;
		pConn->canSendCalled = FALSE;
		break;

	case NF_TCP_RECEIVE:
		pPacketList = &pConn->receivesOut;
		pCounter = &pConn->receivesOutBytes;
		pConn->canReceiveCalled = FALSE;
		break;

	default:
		return NF_STATUS_FAIL;
	}


	if (len <= MAX_BUFFERED_BYTES)
	{
		bool packetAdded = false;

		if ((len > 0) && !IsListEmpty(pPacketList))
		{
			pData = (PNF_DATA_ENTRY)pPacketList->Blink;

			if (pData->data.bufferSize > 0 &&
				(pData->data.bufferSize + len) <= MAX_BUFFERED_BYTES)
			{
				if (buf)
				{
					memcpy(pData->data.buffer + pData->data.bufferSize, buf, len);
				}

				pData->data.bufferSize += len;

				(*pCounter) += len;

				packetAdded = true;
			}
		} 

		if (!packetAdded)
		{
			pData = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + MAX_BUFFERED_BYTES - 1);
			if (!pData)
				return NF_STATUS_FAIL;

			pData->data.id = id;
			pData->data.code = code;
			pData->data.bufferSize = len;
			
			if (buf)
			{
				memcpy(pData->data.buffer, buf, len);
			}

			InsertTailList(pPacketList, &pData->entry);

			(*pCounter) += len;
		}
	} else
	{
		int offset;
		int packetLength;

		offset = 0;
		
		// Split the buffer to packets having NF_TCP_PACKET_BUF_SIZE or less bytes
		while (offset < len)
		{
			pData = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + MAX_BUFFERED_BYTES - 1);
			if (!pData)
				return NF_STATUS_FAIL;

			packetLength = len - offset;
								
			if (packetLength > MAX_BUFFERED_BYTES)
				packetLength = MAX_BUFFERED_BYTES;

			pData->data.id = id;
			pData->data.code = code;
			pData->data.bufferSize = packetLength;

			if (buf)
			{
				memcpy(pData->data.buffer, buf + offset, packetLength);
			}

			InsertTailList(pPacketList, &pData->entry);

			(*pCounter) += pData->data.bufferSize;

			offset += packetLength;
		}
	}

	// EDR_FIX: Fix compilation on vs2017
	if (pData != NULL && !IsListEmpty(pPacketList))// && ((code == NF_TCP_SEND) || !pConn->indicateReceiveInProgress))
	{
		nf_pushOutEvent(pData->data.id, (code == NF_TCP_RECEIVE)? NF_TCP_CAN_RECEIVE : NF_TCP_CAN_SEND);
	}

	return NF_STATUS_SUCCESS;
}

NFAPI_API NF_STATUS NFAPI_NS nf_tcpPostSend(ENDPOINT_ID id, const char * buf, int len)
{
	return nf_tcpPostPacket(NF_TCP_SEND, id, buf, len);
}

NFAPI_API NF_STATUS NFAPI_NS nf_tcpPostReceive(ENDPOINT_ID id, const char * buf, int len)
{
	return nf_tcpPostPacket(NF_TCP_RECEIVE, id, buf, len);
}

NFAPI_API NF_STATUS NFAPI_NS nf_tcpClose(ENDPOINT_ID id)
{
	DWORD	dwBytesReturned;
	NF_STATUS status = NF_STATUS_SUCCESS;

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_FAIL;

	// Posting zero length send prematurely instructs the driver
	// to abort the connection. If zero length send is already sent,
	// i.e. graceful disconnect is in progress, but the connection
	// is not yet closed, the driver aborts this process.
	
	nf_tcpPostSend(id, NULL, 0);

	if (!DeviceIoControl(g_hDevice,
		NF_REQ_TCP_ABORT,
		(LPVOID)&id, sizeof(id),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
	}

	return status;
}


// UDP

/**
* Change suspended state for the socket
* @return NF_STATUS 
* @param PADDROBJ pAddr 
* @param BOOL suspend 
**/
static NF_STATUS nf_udpSetConnectionStateInternal(PADDROBJ pAddr, BOOL suspend)
{
	PNF_DATA pData;
	NF_STATUS status = NF_STATUS_FAIL;
	AutoLock lock(g_cs);

	if ((BOOL)pAddr->suspended == suspend)
		return NF_STATUS_SUCCESS;

	pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA));
	if (!pData)
		return NF_STATUS_FAIL;

	pData->id = pAddr->id;
	pData->code = (suspend)? NF_UDP_REQ_SUSPEND : NF_UDP_REQ_RESUME;
	pData->bufferSize = 0;

	status = nf_postData(pData);
	if (status == NF_STATUS_SUCCESS)
	{
		pAddr->suspended = (suspend != FALSE);
	}

	mp_free(pData);

	return status;
}

/**
* Change suspended state for the socket identified by id
* @return NF_STATUS 
* @param ENDPOINT_ID id 
* @param BOOL suspend 
**/
NFAPI_API NF_STATUS NFAPI_NS nf_udpSetConnectionState(ENDPOINT_ID id, BOOL suspend)
{
	NF_STATUS status = NF_STATUS_FAIL;
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	phte = ht_find_entry(g_phtAddrById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

	if ((BOOL)pAddr->wantSuspend == suspend)
		return NF_STATUS_SUCCESS;

	pAddr->wantSuspend = (suspend != FALSE);

	status = nf_udpSetConnectionStateInternal(pAddr, pAddr->wantSuspend);

	return status;
}

/**
* Buffer the data and post the first packet to driver
* @return NF_STATUS 
* @param NF_DATA_CODE code 
* @param ENDPOINT_ID id 
* @param PNF_UDP_OPTIONS options
* @param const char * remoteAddress
* @param const char * buf 
* @param int len 
**/
static NF_STATUS nf_udpPostPacket(NF_DATA_CODE code, 
								  ENDPOINT_ID id, 
								  const unsigned char * remoteAddress, 
								  const char * buf, 
								  int len,
								  PNF_UDP_OPTIONS options)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	PNF_DATA_ENTRY pData;
	PLIST_ENTRY pPacketList;
	unsigned int *	pCounter;
	unsigned short	ip_family;
	int		addrLen;
	int		bufferSize;

	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!g_phtAddrById || !buf || !remoteAddress)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtAddrById, id);
	if (!phte)
	{
		return NF_STATUS_INVALID_ENDPOINT_ID;
	}

	pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

	switch (code)
	{
	case NF_UDP_SEND:
		pPacketList = &pAddr->sendsOut;
		pCounter = &pAddr->sendsOutBytes;
		break;

	case NF_UDP_RECEIVE:
		pPacketList = &pAddr->receivesOut;
		pCounter = &pAddr->receivesOutBytes;
		break;

	default:
		return NF_STATUS_FAIL;
	}

	ip_family = *(unsigned short*)remoteAddress;
	
	addrLen = (ip_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in);

	bufferSize = sizeof(NF_UDP_OPTIONS) - 1 + NF_MAX_ADDRESS_LENGTH + len;
	
	if (options)
	{
		bufferSize += options->optionsLength;
	}

	if (len > NF_UDP_PACKET_BUF_SIZE)
	{
		return NF_STATUS_FAIL;
	}

	pData = (PNF_DATA_ENTRY)mp_alloc(sizeof(NF_DATA_ENTRY) + bufferSize - 1, 4096);
	if (!pData)
		return NF_STATUS_FAIL;

	pData->data.id = id;
	pData->data.code = code;
	pData->data.bufferSize = bufferSize;
	
	char * pBuffer = pData->data.buffer;

	memcpy(pBuffer, remoteAddress, addrLen);
	pBuffer += NF_MAX_ADDRESS_LENGTH;

	if (options && options->optionsLength)
	{
		memcpy(pBuffer, options, sizeof(NF_UDP_OPTIONS)-1);
		memcpy(pBuffer + sizeof(NF_UDP_OPTIONS)-1, options->options, options->optionsLength);
		pBuffer += sizeof(NF_UDP_OPTIONS)-1 + options->optionsLength;
	} else
	{
		PNF_UDP_OPTIONS puo = (PNF_UDP_OPTIONS)pBuffer;
		memset(puo, 0, sizeof(NF_UDP_OPTIONS));
		pBuffer += sizeof(NF_UDP_OPTIONS)-1;
	}
	
	memcpy(pBuffer, buf, len);

	InsertTailList(pPacketList, &pData->entry);

	(*pCounter) += pData->data.bufferSize;

	if (!IsListEmpty(pPacketList) && ((code == NF_UDP_SEND) || !pAddr->indicateReceiveInProgress))
	{
		nf_pushOutEvent(pData->data.id, (code == NF_UDP_RECEIVE)? NF_UDP_CAN_RECEIVE : NF_UDP_CAN_SEND);
	}

	return NF_STATUS_SUCCESS;
}


NFAPI_API NF_STATUS NFAPI_NS nf_udpPostSend(ENDPOINT_ID id, 
										  const unsigned char * remoteAddress,
										  const char * buf, 
										  int len,
										  PNF_UDP_OPTIONS options)
{
	return nf_udpPostPacket(NF_UDP_SEND, id, remoteAddress, buf, len, options);
}

NFAPI_API NF_STATUS NFAPI_NS nf_udpPostReceive(ENDPOINT_ID id, 
										  const unsigned char * remoteAddress,
										  const char * buf, 
										  int len,
										  PNF_UDP_OPTIONS options)
{
	return nf_udpPostPacket(NF_UDP_RECEIVE, id, remoteAddress, buf, len, options);
}


static NF_STATUS nf_ipPostPacket( NF_DATA_CODE code, 
								  const char * buf, 
								  int len,
								  PNF_IP_PACKET_OPTIONS options)
{
	OVERLAPPED ol;
	DWORD dwRes;
	DWORD dwWritten = 0;
	HANDLE events[] = { g_ioPostEvent, g_stopEvent };
	NF_READ_RESULT rr;
	PNF_DATA pData;
	int		bufferSize;

	// Don't inject the packets indicated for monitoring in read-only mode
	if (options->flags & NFIF_READONLY)
		return NF_STATUS_SUCCESS;

	bufferSize = sizeof(NF_IP_PACKET_OPTIONS) + len;

	rr.length = sizeof(NF_DATA) + bufferSize - 1;

	if (rr.length > g_nfBuffers.outBufLen)
	{
		return NF_STATUS_IO_ERROR;
	}

	if (g_hDevice == INVALID_HANDLE_VALUE || !g_nfBuffers.outBuf)
		return NF_STATUS_NOT_INITIALIZED;

	AutoLock lock(g_csPost);

	pData = (PNF_DATA)g_nfBuffers.outBuf;

	pData->id = 0;
	pData->code = code;
	pData->bufferSize = bufferSize;
	memcpy(pData->buffer, options, sizeof(NF_IP_PACKET_OPTIONS));
	memcpy(pData->buffer + sizeof(NF_IP_PACKET_OPTIONS), buf, len);

	memset(&ol, 0, sizeof(ol));

	ol.hEvent = g_ioPostEvent;

	if (!WriteFile(g_hDevice, &rr, sizeof(rr), NULL, &ol))
	{
		if (GetLastError() != ERROR_IO_PENDING)
			return NF_STATUS_IO_ERROR;
	}

	// Wait for completion
	for (;;)
	{
		dwRes = WaitForMultipleObjects(
						sizeof(events) / sizeof(events[0]),
						events, 
						FALSE,
						INFINITE);

		if (dwRes != WAIT_OBJECT_0)
		{
			CancelIo(g_hDevice);
			return NF_STATUS_FAIL;
		}

		dwRes = WaitForSingleObject(g_stopEvent, 0);
		if (dwRes == WAIT_OBJECT_0)
		{
			CancelIo(g_hDevice);
			return NF_STATUS_FAIL;
		}

		if (!GetOverlappedResult(g_hDevice, &ol, &dwWritten, FALSE))
		{
			return NF_STATUS_FAIL;
		}

		break;
	}

	return (dwWritten)? NF_STATUS_SUCCESS : NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS nf_ipPostSend(const char * buf, 
										  int len,
										  PNF_IP_PACKET_OPTIONS options)
{
	return nf_ipPostPacket(NF_IP_SEND, buf, len, options);
}

NFAPI_API NF_STATUS NFAPI_NS nf_ipPostReceive(const char * buf, 
										  int len,
										  PNF_IP_PACKET_OPTIONS options)
{
	return nf_ipPostPacket(NF_IP_RECEIVE, buf, len, options);
}

// Rules

/**
* Add a rule to the head of driver list
* @return NF_STATUS 
* @param PNF_RULE pRule 
**/
NFAPI_API NF_STATUS NFAPI_NS nf_addRule(PNF_RULE pRule, int toHead)
{
	PNF_DATA pData;
	NF_STATUS status = NF_STATUS_FAIL;

	{
		AutoLock	lock(g_cs);
		if (g_hDevice == INVALID_HANDLE_VALUE)
			return NF_STATUS_NOT_INITIALIZED;
	}

	pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA) - 1 + sizeof(NF_RULE));
	if (!pData)
		return NF_STATUS_FAIL;

	pData->id = 0;
	pData->code = (toHead)? NF_REQ_ADD_HEAD_RULE : NF_REQ_ADD_TAIL_RULE;
	pData->bufferSize = sizeof(NF_RULE);

	memcpy(pData->buffer, pRule, sizeof(NF_RULE));

	status = nf_postData(pData);

	mp_free(pData);

	return status;
}

/**
* Remove all rules from driver list
* @return NF_STATUS 
**/
NFAPI_API NF_STATUS NFAPI_NS nf_deleteRules()
{
	PNF_DATA pData;
	NF_STATUS status = NF_STATUS_FAIL;

	{
		AutoLock	lock(g_cs);

		if (g_hDevice == INVALID_HANDLE_VALUE)
			return NF_STATUS_NOT_INITIALIZED;
	}

	pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA));
	if (!pData)
		return NF_STATUS_FAIL;

	pData->id = 0;
	pData->code = NF_REQ_DELETE_RULES;
	pData->bufferSize = 0;

	status = nf_postData(pData);

	mp_free(pData);

	return status;
}


/**
* Replace the rules in driver with the specified array.
* @param pRules Array of <tt>NF_RULE</tt> structures
* @param count Number of items in array
**/
NFAPI_API NF_STATUS NFAPI_NS 
nf_setRules(PNF_RULE pRules, int count)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!DeviceIoControl(g_hDevice,
		NF_REQ_CLEAR_TEMP_RULES,
		(LPVOID)NULL, 0,
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_FAIL;
	}

	for (int i=0; i<count; i++)
	{
		if (!DeviceIoControl(g_hDevice,
			NF_REQ_ADD_TEMP_RULE,
			(LPVOID)&pRules[i], sizeof(NF_RULE),
			(LPVOID)NULL, 0,
			&dwBytesReturned, NULL))
		{
			return NF_STATUS_FAIL;
		}
	}

	if (!DeviceIoControl(g_hDevice,
		NF_REQ_SET_TEMP_RULES,
		(LPVOID)NULL, 0,
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_FAIL;
	}

	return NF_STATUS_SUCCESS;
}

/**
* Replace the rules in driver with the specified array.
* @param pRules Array of <tt>NF_RULE</tt> structures
* @param count Number of items in array
**/
NFAPI_API NF_STATUS NFAPI_NS 
nf_setRulesEx(PNF_RULE_EX pRules, int count)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!DeviceIoControl(g_hDevice,
		NF_REQ_CLEAR_TEMP_RULES,
		(LPVOID)NULL, 0,
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_FAIL;
	}

	for (int i=0; i<count; i++)
	{
		if (!DeviceIoControl(g_hDevice,
			NF_REQ_ADD_TEMP_RULE_EX,
			(LPVOID)&pRules[i], sizeof(NF_RULE_EX),
			(LPVOID)NULL, 0,
			&dwBytesReturned, NULL))
		{
			return NF_STATUS_FAIL;
		}
	}

	if (!DeviceIoControl(g_hDevice,
		NF_REQ_SET_TEMP_RULES,
		(LPVOID)NULL, 0,
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_FAIL;
	}

	return NF_STATUS_SUCCESS;
}

// Init/free

static void nf_freeConnList()
{
	PCONNOBJ pConn = NULL;

	while (!IsListEmpty(&g_lConnections))
	{
		pConn = (PCONNOBJ)RemoveHeadList(&g_lConnections);
		connobj_free(pConn);
	}
}

static void nf_freeAddrList()
{
	PADDROBJ pAddr = NULL;

	while (!IsListEmpty(&g_lAddresses))
	{
		pAddr = (PADDROBJ)RemoveHeadList(&g_lAddresses);
		addrobj_free(pAddr);
	}
}

/**
* Initialize the driver with specified name and set the event handler
* @return NF_STATUS 
* @param const char * driverName 
* @param NF_EventHandler * pHandler 
**/
NFAPI_API NF_STATUS NFAPI_NS nf_init(const char * driverName, NF_EventHandler * pHandler)
{
	HANDLE hDevice;
	AutoLock lock(g_cs);

	if (g_initialized)
		return NF_STATUS_SUCCESS;

	if (!nf_applyFlags(true))
	{
		return NF_STATUS_REBOOT_REQUIRED;
	}

	hDevice = nf_openDevice(driverName);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return NF_STATUS_FAIL;
	} else
	{
		g_hDevice.Attach(hDevice);

		strncpy(g_driverName, driverName, sizeof(g_driverName));
	}

	DWORD dwBytesReturned = 0;
	memset(&g_nfBuffers, 0, sizeof(g_nfBuffers));

	OVERLAPPED ol;
	AutoEventHandle hEvent;

	memset(&ol, 0, sizeof(ol));
	ol.hEvent = hEvent;

	if (!DeviceIoControl(g_hDevice,
		IOCTL_DEVCTRL_OPEN,
		NULL, 0,
		(LPVOID)&g_nfBuffers, sizeof(g_nfBuffers),
		NULL, &ol))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			g_hDevice.Close();
			return NF_STATUS_FAIL;
		}
	}

	if (!GetOverlappedResult(g_hDevice, &ol, &dwBytesReturned, TRUE))
	{
		g_hDevice.Close();
		return NF_STATUS_FAIL;
	}

	if (dwBytesReturned != sizeof(g_nfBuffers))
	{
		g_hDevice.Close();
		return NF_STATUS_FAIL;
	}

	InitializeListHead(&g_lConnections);
	InitializeListHead(&g_lAddresses);

	g_phtConnById = hash_table_new(DEFAULT_HASH_SIZE);
	g_phtAddrById = hash_table_new(DEFAULT_HASH_SIZE);

	mempools_init();
	ipobj_init();

	g_initialized = true;

	if (nf_start(pHandler) != NF_STATUS_SUCCESS)
	{
		nf_free();
		return NF_STATUS_FAIL;
	}

	return NF_STATUS_SUCCESS;
}

/**
* Free the driver
* @return void 
**/
NFAPI_API void NFAPI_NS nf_free()
{
	nf_stop();

	g_hDevice.Close();

	AutoLock lock(g_cs);

	if (!g_initialized)
		return;

	g_initialized = false;

	// Free the lists
	nf_freeConnList();
	nf_freeAddrList();
	ipobj_free();

	if (g_phtConnById)
	{
		hash_table_free(g_phtConnById);
		g_phtConnById = NULL;
	}

	if (g_phtAddrById)
	{
		hash_table_free(g_phtAddrById);
		g_phtAddrById = NULL;
	}

	mempools_free();
}

/**
* Start IO thread
* @return static NF_STATUS 
* @param NF_EventHandler * pHandler 
**/
static NF_STATUS nf_start(NF_EventHandler * pHandler)
{
	unsigned int threadId;
	HANDLE hThread;

	AutoLock lock(g_cs);
	
	if ((g_hDevice == INVALID_HANDLE_VALUE) ||
		(g_hWorkThread != INVALID_HANDLE_VALUE) ||
		!pHandler)
		return NF_STATUS_FAIL;

	g_pEventHandler = pHandler;

	g_useNagle = false;//(nf_getDriverType() == DT_WFP);

	if (g_useNagle)
	if (!g_timers.init(timerTimeout))
		return NF_STATUS_FAIL;

	ResetEvent(g_stopEvent);
	ResetEvent(g_workThreadStoppedEvent);
	ResetEvent(g_workThreadStartedEvent);

	hThread = (HANDLE)_beginthreadex(0, 0,
                     nf_workThread,
                     (LPVOID)NULL,
                     0,
                     &threadId);
	
	if (hThread == 0)
	{
		SetEvent(g_workThreadStoppedEvent);
		g_hWorkThread.Attach(INVALID_HANDLE_VALUE);
		return NF_STATUS_FAIL;
	} else
	{
		g_hWorkThread.Attach(hThread);
		WaitForSingleObject(g_workThreadStartedEvent, INFINITE);
	}

	return NF_STATUS_SUCCESS;
}



/**
* Stop IO thread
* @return void 
**/
static void nf_stop()
{
	SetEvent(g_stopEvent);

	if (g_hWorkThread != INVALID_HANDLE_VALUE)
	{
		WaitForSingleObject(g_workThreadStoppedEvent.m_h, INFINITE);
		g_hWorkThread.Close();
	}

	if (g_useNagle)
	{
		g_timers.free();
	}

	g_pEventHandler = NULL;
}


/**
* Returns the number of active connections
* @return unsigned long
**/
NFAPI_API unsigned long NFAPI_NS nf_getConnCount()
{
	PLIST_ENTRY pEntry;
	unsigned long count = 0;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	pEntry = g_lConnections.Flink;

	while (pEntry != &g_lConnections)
	{
		count++;
		pEntry = pEntry->Flink;
	}

	return count;
}

/**
 *	Sets the timeout for TCP connections and returns old timeout.
 **/
NFAPI_API unsigned long NFAPI_NS 
nf_setTCPTimeout(unsigned long timeout)
{
	AutoLock lock(g_cs);
	DWORD oldTimeout = g_tcpTimeout;
	g_tcpTimeout = timeout;	
	return oldTimeout;
}

/**
 *	Disables indicating TCP packets to user mode for the specified endpoint
 *  @param id Socket identifier
 */
NFAPI_API NF_STATUS NFAPI_NS 
nf_tcpDisableFiltering(ENDPOINT_ID id)
{
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	PHASH_TABLE_ENTRY phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	PCONNOBJ pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

	if (pConn->filteringDisabled || 
		!(pConn->connInfo.filteringFlag & NF_FILTER) ||
		(pConn->connInfo.filteringFlag & NF_OFFLINE))
		return NF_STATUS_SUCCESS;

	// Start flushing the packets to driver
	pConn->disableFiltering = TRUE;
/*
	// Suspend the connection, to make sure that the buffers 
	// will be flushed from user mode in a timely manner
	nf_tcpSetConnectionStateInternal(pConn, TRUE);

	NF_DATA data;
	data.id = pConn->id;
	data.code = NF_TCP_CAN_SEND;
	data.bufferSize = 0;
	
	AutoUnlock unlock(g_cs);
	g_eventQueue.push(&data);
*/	
	return NF_STATUS_SUCCESS;
}

/**
 *	Disables indicating UDP packets to user mode for the specified endpoint
 *  @param id Socket identifier
 */
NFAPI_API NF_STATUS NFAPI_NS 
nf_udpDisableFiltering(ENDPOINT_ID id)
{
	NF_STATUS status = NF_STATUS_FAIL;
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	phte = ht_find_entry(g_phtAddrById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

	if (pAddr->filteringDisabled)
		return NF_STATUS_SUCCESS;

	pAddr->filteringDisabled = TRUE;

	PNF_DATA pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA));
	if (!pData)
		return NF_STATUS_FAIL;

	pData->id = id;
	pData->code = NF_UDP_DISABLE_USER_MODE_FILTERING;
	pData->bufferSize = 0;

	status = nf_postData(pData);

	mp_free(pData);

	return status;
}

#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED

#define	CL_TL_ENTITY				0x401
#define	INFO_CLASS_PROTOCOL			0x200
#define	INFO_TYPE_CONNECTION		0x300

//* Structure of an entity ID.
typedef struct TDIEntityID {
	ulong		tei_entity;
	ulong		tei_instance;
} TDIEntityID;

//* Structure of an object ID.
typedef struct TDIObjectID {
	TDIEntityID	toi_entity;
	ulong		toi_class;
	ulong		toi_type;
	ulong		toi_id;
} TDIObjectID;

//
// SetInformationEx IOCTL request structure. This structure is passed as the
// InputBuffer. The space allocated for the structure must be large enough
// to contain the structure and the set data buffer, which begins at the
// Buffer field. The OutputBuffer parameter in the DeviceIoControl is not used.
//
typedef struct tcp_request_set_information_ex {
	TDIObjectID     ID;             // object ID to set.
	unsigned int    BufferSize;     // size of the set data buffer in bytes
	unsigned char   Buffer[1];      // beginning of the set data buffer
} TCP_REQUEST_SET_INFORMATION_EX, *PTCP_REQUEST_SET_INFORMATION_EX;


NFAPI_API NF_STATUS NFAPI_NS 
nf_tcpSetSockOpt(ENDPOINT_ID id, int optname, const char* optval, int optlen)
{
	NF_STATUS status = NF_STATUS_FAIL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!optval || optlen < sizeof(BOOL))
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	PNF_DATA pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA) - 1 + sizeof(TCP_REQUEST_SET_INFORMATION_EX) - 1 + optlen);
	if (!pData)
		return NF_STATUS_FAIL;

	pData->id = id;
	pData->code = NF_REQ_SET_TCP_OPT;
	pData->bufferSize = sizeof(TCP_REQUEST_SET_INFORMATION_EX) - 1 + optlen;

	PTCP_REQUEST_SET_INFORMATION_EX pInfo = (PTCP_REQUEST_SET_INFORMATION_EX)pData->buffer;

    pInfo->ID.toi_entity.tei_entity = CL_TL_ENTITY;   
    pInfo->ID.toi_entity.tei_instance = 0;   
    pInfo->ID.toi_class = INFO_CLASS_PROTOCOL;   
    pInfo->ID.toi_type = INFO_TYPE_CONNECTION;   
	pInfo->ID.toi_id = optname;

	memcpy(pInfo->Buffer, optval, optlen);
	pInfo->BufferSize = optlen;

	status = nf_postData(pData);

	mp_free(pData);

	return status;
}

NFAPI_API BOOL NFAPI_NS 
nf_tcpIsProxy(DWORD processId)
{
	NF_STATUS status = NF_STATUS_FAIL;
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	NF_DATA data;

	data.id = (ENDPOINT_ID)processId;
	data.code = NF_REQ_IS_PROXY;
	data.bufferSize = 0;

	status = nf_postData(&data);

	return (status == NF_STATUS_SUCCESS);
}

NF_STATUS nf_getLocalAddress(ENDPOINT_ID id, unsigned char * addr, int len)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (len < NF_MAX_ADDRESS_LENGTH)
		return NF_STATUS_FAIL;

	DWORD rlen = sizeof(NF_DATA) - 1 + NF_MAX_ADDRESS_LENGTH;
	PNF_DATA pData = (PNF_DATA)mp_alloc(rlen);
	if (!pData)
		return NF_STATUS_FAIL;

	memset(pData, 0, rlen);
	pData->id = id;
	pData->bufferSize = NF_MAX_ADDRESS_LENGTH;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_GET_ADDR_INFO,
		(LPVOID)pData, rlen,
		(LPVOID)pData, rlen,
		&dwBytesReturned, NULL) && (dwBytesReturned > 0))
	{
		memcpy(addr, pData->buffer, NF_MAX_ADDRESS_LENGTH);
		mp_free(pData);
		return NF_STATUS_SUCCESS;
	}

	mp_free(pData);
	return NF_STATUS_FAIL;
}

// EDR_FIX: Remove function declaration which exist
/*
typedef BOOL (WINAPI *tQueryFullProcessImageNameA)(
			HANDLE hProcess,
			DWORD dwFlags,
			LPSTR lpExeName,
			PDWORD lpdwSize
);

typedef BOOL (WINAPI *tQueryFullProcessImageNameW)(
			HANDLE hProcess,
			DWORD dwFlags,
			LPWSTR lpExeName,
			PDWORD lpdwSize
);

tQueryFullProcessImageNameW QueryFullProcessImageNameW =
			(tQueryFullProcessImageNameW)GetProcAddress(
							GetModuleHandle(_T("kernel32")), "QueryFullProcessImageNameW");

tQueryFullProcessImageNameA QueryFullProcessImageNameA =
			(tQueryFullProcessImageNameA)GetProcAddress(
							GetModuleHandle(_T("kernel32")), "QueryFullProcessImageNameA");
*/

NFAPI_API BOOL NFAPI_NS 
nf_getProcessNameA(DWORD processId, char * buf, DWORD len)
{
	BOOL res = FALSE;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (hProcess)
	{
		// EDR_FIX: Remove function declaration which exist
		//if (QueryFullProcessImageNameA)
		//{
			res = QueryFullProcessImageNameA(hProcess, 0, buf, &len);
		//} else
		//{
		//	res = GetModuleFileNameExA(hProcess, NULL, buf, len);
		//}

		CloseHandle(hProcess);
	}

	return res;
}

NFAPI_API BOOL NFAPI_NS 
nf_getProcessNameW(DWORD processId, wchar_t * buf, DWORD len)
{
	BOOL res = FALSE;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (hProcess)
	{
		// EDR_FIX: Remove function declaration which exist
		//if (QueryFullProcessImageNameW)
		//{
			res = QueryFullProcessImageNameW(hProcess, 0, buf, &len);
		//} else
		//{
		//	res = GetModuleFileNameExW(hProcess, NULL, buf, len);
		//}

		CloseHandle(hProcess);
	}

	return res;
}


NFAPI_API void NFAPI_NS 
nf_adjustProcessPriviledges()
{
	// Set the necessary privileges for accessing token info for all system processes
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	if (hProcess)
	{
		HANDLE hToken;
				
		if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
		{
			TOKEN_PRIVILEGES tp;
			LUID luid;

			if ( !LookupPrivilegeValue( 
					NULL,            // lookup privilege on local system
					SE_DEBUG_NAME,   // privilege to lookup 
					&luid ) )        // receives LUID of privilege
			{
				CloseHandle(hToken);
				CloseHandle(hProcess);
				return; 
			}

			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if ( !AdjustTokenPrivileges(
				   hToken, 
				   FALSE,  
				   &tp, 
				   sizeof(TOKEN_PRIVILEGES), 
				   (PTOKEN_PRIVILEGES) NULL, 
				   (PDWORD) NULL) )
			{ 
				  CloseHandle(hToken);
				  CloseHandle(hProcess);
				  return; 
			} 

			CloseHandle(hToken);
		}

		CloseHandle(hProcess);
	}
}

NFAPI_API void NFAPI_NS 
nf_setOptions(DWORD nThreads, DWORD flags)
{
	g_nThreads = nThreads;
	g_flags = flags;
}

NFAPI_API NF_STATUS NFAPI_NS 
nf_completeTCPConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	NF_STATUS status = NF_STATUS_FAIL;
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pConnInfo)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

	if (pConnInfo->filteringFlag & NF_BLOCK)
	{
		nf_tcpClose(id);
		connobj_free(pConn);
	} else
	if (!(pConnInfo->filteringFlag & NF_FILTER))
	{
		connobj_free(pConn);
	}

	PNF_DATA pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA) - 1 + sizeof(NF_TCP_CONN_INFO));
	if (!pData)
	{
		return NF_STATUS_FAIL;
	}

	pData->id = id;
	pData->code = NF_TCP_CONNECT_REQUEST;
	pData->bufferSize = sizeof(NF_TCP_CONN_INFO);

	memcpy(pData->buffer, pConnInfo, sizeof(NF_TCP_CONN_INFO));
	
	status = nf_postData(pData);

	mp_free(pData);

	return status;
}

NFAPI_API NF_STATUS NFAPI_NS 
nf_completeUDPConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnInfo)
{
	NF_STATUS status = NF_STATUS_FAIL;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pConnInfo)
		return NF_STATUS_FAIL;

	PNF_DATA pData = (PNF_DATA)mp_alloc(sizeof(NF_DATA) - 1 + sizeof(NF_UDP_CONN_REQUEST));
	if (!pData)
	{
		return NF_STATUS_FAIL;
	}

	pData->id = id;
	pData->code = NF_UDP_CONNECT_REQUEST;
	pData->bufferSize = sizeof(NF_UDP_CONN_REQUEST);

	memcpy(pData->buffer, pConnInfo, sizeof(NF_UDP_CONN_REQUEST));
	
	status = nf_postData(pData);
 
	mp_free(pData);

	return status;
}

NFAPI_API NF_STATUS NFAPI_NS 
nf_getTCPConnInfo(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pConnInfo)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

	memcpy(pConnInfo, &pConn->connInfo, sizeof(NF_TCP_CONN_INFO));

	return NF_STATUS_SUCCESS;
}

NFAPI_API NF_STATUS NFAPI_NS 
nf_getUDPConnInfo(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	PADDROBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pConnInfo)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtAddrById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pConn = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

	memcpy(pConnInfo, &pConn->connInfo, sizeof(NF_UDP_CONN_INFO));

	return NF_STATUS_SUCCESS;
}


struct DRIVE_INFO
{
	DRIVE_INFO()
	{
		deviceNameLen = 0;
	}
	DRIVE_INFO(const DRIVE_INFO & v)
	{
		*this = v;
	}
	DRIVE_INFO & operator = (const DRIVE_INFO & v)
	{
		deviceName = v.deviceName;
		driveName = v.driveName;
		deviceNameLen = v.deviceNameLen;
		return *this;
	}

	std::wstring deviceName;
	std::wstring driveName;
	size_t		deviceNameLen;
};

typedef std::vector<DRIVE_INFO> tDevInfo;
static tDevInfo	g_devInfo;
static AutoCriticalSection m_csDevMap;

typedef NTSTATUS (WINAPI * tNtQuerySymbolicLinkObject)(
  __in       HANDLE LinkHandle,
  __inout    PUNICODE_STRING LinkTarget,
  __out_opt  PULONG ReturnedLength
);

typedef NTSTATUS (WINAPI * tNtOpenSymbolicLinkObject)(
  __out  PHANDLE LinkHandle,
  __in   ACCESS_MASK DesiredAccess,
  __in   POBJECT_ATTRIBUTES ObjectAttributes
);

tNtQuerySymbolicLinkObject NtQuerySymbolicLinkObject = 
(tNtQuerySymbolicLinkObject)GetProcAddress(GetModuleHandleA("ntdll"), "NtQuerySymbolicLinkObject");

tNtOpenSymbolicLinkObject NtOpenSymbolicLinkObject = 
	(tNtOpenSymbolicLinkObject)GetProcAddress(GetModuleHandleA("ntdll"), "NtOpenSymbolicLinkObject");

typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name; 
	WCHAR NameBuffer[1];
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

typedef struct _OBJECT_TYPE_INFORMATION {
    UNICODE_STRING TypeName;
    ULONG Reserved [22];    // reserved for internal use
} OBJECT_TYPE_INFORMATION, *POBJECT_TYPE_INFORMATION;


#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
#endif

static bool resolveLink(const std::wstring & linkName, std::wstring & devName)
{
	HANDLE hLink = NULL;
	UNICODE_STRING name, link;
	bool res = false;

	if (!NtOpenSymbolicLinkObject || !NtQuerySymbolicLinkObject)
		return false;

	name.Buffer = (PWSTR)linkName.c_str();
	name.MaximumLength = name.Length = (USHORT)linkName.length()*sizeof(linkName[0]);

	OBJECT_ATTRIBUTES oa;
	oa.Length=sizeof(oa);
	oa.RootDirectory=NULL;
	oa.ObjectName=&name;
	oa.Attributes=0x40;
	oa.SecurityDescriptor=NULL;
	oa.SecurityQualityOfService=NULL;

	NtOpenSymbolicLinkObject(&hLink, 1, &oa);
	if (hLink)
	{
		wchar_t targetName[_MAX_PATH];
		
		link.Buffer = targetName;
		link.MaximumLength = link.Length = sizeof(targetName);
		
		if (NT_SUCCESS(NtQuerySymbolicLinkObject(hLink, &link, NULL)))
		{
			*(PWORD)((PBYTE)link.Buffer+link.Length)=0;
			devName = link.Buffer;
			res = true;
		}

		CloseHandle(hLink);
	}

	return res;
}

static void updateDevInfo()
{
	wchar_t drives[MAX_PATH];
	wchar_t devname[MAX_PATH];
	tDevInfo devInfo;

	if (GetLogicalDriveStringsW(sizeof(drives)/2, drives))
	{
		for (wchar_t * p = drives; ((p-drives) < sizeof(drives)/2) && *p; p += wcslen(p)+1)
		{
			wchar_t driveName[3] = { *p, ':', 0 };

			if (QueryDosDeviceW(driveName, devname, sizeof(devname)/2))
			{
				std::wstring driveNameWithSlash = std::wstring(driveName); 
				DRIVE_INFO di;
				
				driveNameWithSlash += L"\\";

				di.deviceName = devname;
				di.driveName = driveName;
				di.deviceNameLen = wcslen(devname);

				devInfo.push_back(di);

				DWORD dt = GetDriveTypeW(driveNameWithSlash.c_str());

				if (dt != DRIVE_CDROM &&
					dt != DRIVE_REMOTE)
				{
					std::wstring name;

					if (resolveLink(devname, name))
					{
						di.deviceName = name;
						di.deviceNameLen = name.length();
						
						devInfo.push_back(di);
					}
				}
			}
		}
	}
	
	{
		AutoLock lock(m_csDevMap);
		g_devInfo = devInfo;
	}
}

static bool getDevInfo(const std::wstring & path, DRIVE_INFO & di)
{
	AutoLock lock(m_csDevMap);

	for (tDevInfo::iterator it = g_devInfo.begin(); 
		it != g_devInfo.end(); it++)
	{
		if (wcsncmp(path.c_str(), it->deviceName.c_str(), it->deviceName.length()) == 0)
		{
			di = *it;
			return true;
		}
	}

	return false;
}

NFAPI_API BOOL NFAPI_NS 
nf_getProcessNameFromKernel(DWORD processId, wchar_t * buf, DWORD len)
{
	DWORD	dwBytesReturned;

	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return FALSE;

	if (!buf || len < 260)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	if (DeviceIoControl(g_hDevice,
		NF_REQ_GET_PROCESS_NAME,
		(LPVOID)&processId, sizeof(ULONG),
		(LPVOID)buf, len*2,
		&dwBytesReturned, NULL) && (dwBytesReturned > 0))
	{
		DRIVE_INFO di;
		std::wstring name;
	
		if (getDevInfo(buf, di))
		{
			name = di.driveName;
			name += (buf + di.deviceNameLen);
		} else
		{
			updateDevInfo();

			if (getDevInfo(buf, di))
			{
				name = di.driveName;
				name += (buf + di.deviceNameLen);
			} else
			{
				name = buf;
			}
		}

		if (name.empty() || name.length() >= len)
			return FALSE;

		wcscpy(buf, name.c_str());

		return TRUE;
	}

	return FALSE;
}

NFAPI_API void NFAPI_NS
nf_setIPEventHandler(NF_IPEventHandler * pHandler)
{
	g_pIPEventHandler = pHandler;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_addFlowCtl(PNF_FLOWCTL_DATA pData, unsigned int * pFcHandle)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pFcHandle || !pData)
		return NF_STATUS_FAIL;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_ADD_FLOW_CTL,
		(LPVOID)pData, sizeof(*pData),
		(LPVOID)pFcHandle, sizeof(*pFcHandle),
		&dwBytesReturned, NULL) && (dwBytesReturned > 0))
	{
		if (*pFcHandle != 0)
		{
			return NF_STATUS_SUCCESS;
		}
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_deleteFlowCtl(unsigned int fcHandle)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_DELETE_FLOW_CTL,
		(LPVOID)&fcHandle, sizeof(fcHandle),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_setTCPFlowCtl(ENDPOINT_ID id, unsigned int fcHandle)
{
	DWORD	dwBytesReturned;
	NF_FLOWCTL_SET_DATA data;
	AutoLock lock(g_cs);

	data.endpointId = id;
	data.fcHandle = fcHandle;

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_SET_TCP_FLOW_CTL,
		(LPVOID)&data, sizeof(data),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_setUDPFlowCtl(ENDPOINT_ID id, unsigned int fcHandle)
{
	DWORD	dwBytesReturned;
	NF_FLOWCTL_SET_DATA data;
	AutoLock lock(g_cs);

	data.endpointId = id;
	data.fcHandle = fcHandle;

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_SET_UDP_FLOW_CTL,
		(LPVOID)&data, sizeof(data),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_modifyFlowCtl(unsigned int fcHandle, PNF_FLOWCTL_DATA pData)
{
	DWORD	dwBytesReturned;
	NF_FLOWCTL_MODIFY_DATA data;
	AutoLock lock(g_cs);

	if (!fcHandle || !pData)
		return NF_STATUS_FAIL;

	data.fcHandle = fcHandle;
	data.data = *pData;

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_MODIFY_FLOW_CTL,
		(LPVOID)&data, sizeof(data),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL)) 
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_getFlowCtlStat(unsigned int fcHandle, PNF_FLOWCTL_STAT pStat)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!fcHandle || !pStat)
		return NF_STATUS_FAIL;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_GET_FLOW_CTL_STAT,
		(LPVOID)&fcHandle, sizeof(fcHandle),
		(LPVOID)pStat, sizeof(*pStat),
		&dwBytesReturned, NULL) && (dwBytesReturned > 0))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS 
nf_getTCPStat(ENDPOINT_ID id, PNF_FLOWCTL_STAT pStat)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pStat)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtConnById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

	memcpy(pStat, &pConn->flowStat, sizeof(NF_FLOWCTL_STAT));

	return NF_STATUS_SUCCESS;
}

NFAPI_API NF_STATUS NFAPI_NS 
nf_getUDPStat(ENDPOINT_ID id, PNF_FLOWCTL_STAT pStat)
{
	PADDROBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	AutoLock lock(g_cs);
	
	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pStat)
		return NF_STATUS_FAIL;

	phte = ht_find_entry(g_phtAddrById, id);
	if (!phte)
		return NF_STATUS_INVALID_ENDPOINT_ID;

	pConn = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

	memcpy(pStat, &pConn->flowStat, sizeof(NF_FLOWCTL_STAT));

	return NF_STATUS_SUCCESS;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_addBindingRule(PNF_BINDING_RULE pRule, int toHead)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pRule)
		return NF_STATUS_FAIL;

	if (pRule->processName[0] != 0)
	{
		size_t len = wcslen((wchar_t*)pRule->processName);

		_wcslwr((wchar_t*)pRule->processName);
		if (len > 3)
		{
			if (pRule->processName[1] == L':')
			{
				memmove(pRule->processName, pRule->processName + 2, len*2 - 2);
			}
		}
	}

	if (DeviceIoControl(g_hDevice,
		toHead? NF_REQ_ADD_HEAD_BINDING_RULE : NF_REQ_ADD_TAIL_BINDING_RULE,
		(LPVOID)pRule, sizeof(*pRule),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API NF_STATUS NFAPI_NS
nf_deleteBindingRules()
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_DELETE_BINDING_RULES,
		(LPVOID)NULL, 0,
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

/**
* Add a rule to the head of driver list
* @return NF_STATUS 
* @param PNF_RULE pRule 
**/
NFAPI_API NF_STATUS NFAPI_NS nf_addRuleEx(PNF_RULE_EX pRule, int toHead)
{
	DWORD	dwBytesReturned;
	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return NF_STATUS_NOT_INITIALIZED;

	if (!pRule)
		return NF_STATUS_FAIL;

	if (pRule->processName[0] != 0)
	{
		size_t len = wcslen((wchar_t*)pRule->processName);

		_wcslwr((wchar_t*)pRule->processName);
		if (len > 3)
		{
			if (pRule->processName[1] == L':')
			{
				memmove(pRule->processName, pRule->processName + 2, len*2 - 2);
			}
		}
	}

	if (DeviceIoControl(g_hDevice,
		toHead? NF_REQ_ADD_HEAD_RULE_EX : NF_REQ_ADD_TAIL_RULE_EX,
		(LPVOID)pRule, sizeof(*pRule),
		(LPVOID)NULL, 0,
		&dwBytesReturned, NULL))
	{
		return NF_STATUS_SUCCESS;
	}

	return NF_STATUS_FAIL;
}

NFAPI_API unsigned long NFAPI_NS 
nf_getDriverType()
{
	DWORD	dwBytesReturned;
	ULONG	driverType = 0;

	AutoLock lock(g_cs);

	if (g_hDevice == INVALID_HANDLE_VALUE)
		return DT_UNKNOWN;

	if (DeviceIoControl(g_hDevice,
		NF_REQ_GET_DRIVER_TYPE,
		(LPVOID)NULL, 0,
		(LPVOID)&driverType, sizeof(driverType),
		&dwBytesReturned, NULL) && (dwBytesReturned > 0))
	{
		return DT_WFP;
	}

	return DT_TDI;
}
