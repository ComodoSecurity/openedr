//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _UDPCTX_H
#define _UDPCTX_H

#include "hashtable.h"
#include "nfdriver.h"
#include "flowctl.h"

typedef struct UDP_HEADER_ 
{
    UINT16 srcPort;
    UINT16 destPort;
    UINT16 length;
    UINT16 checksum;
} UDP_HEADER, *PUDP_HEADER;


typedef struct _UDP_REDIRECT_INFO
{
	UINT64				classifyHandle;
	UINT64				filterId;
	FWPS_CLASSIFY_OUT	classifyOut;
	BOOLEAN				isPended;

#ifdef USE_NTDDI
#if(NTDDI_VERSION >= NTDDI_WIN8)
	HANDLE				redirectHandle;
#endif 
#endif
} UDP_REDIRECT_INFO, *PUDP_REDIRECT_INFO;

typedef struct _UDPCTX
{
	LIST_ENTRY entry;

	HASH_ID	id;
	PHASH_TABLE_ENTRY id_next;

	UINT64				transportEndpointHandle;
	PHASH_TABLE_ENTRY	transportEndpointHandle_next;

	BOOLEAN		closed;			// TRUE if the context is disassociated from WFP flow

	ULONG		filteringFlag;	// Values from NF_FILTERING_FLAG
	ULONG		processId;		// Process identifier
	USHORT		ip_family;		// AF_INET or AF_INET6
    UCHAR		localAddr[NF_MAX_ADDRESS_LENGTH];	// Local address
	UCHAR		remoteAddr[NF_MAX_ADDRESS_LENGTH];	// Remote address
	USHORT      ipProto;		// protocol

	UINT16		layerId;		// WFP layer id
	UINT32		calloutId;		// WFP callout id

	LIST_ENTRY	pendedSendPackets;	// List of outbound packets
	LIST_ENTRY	pendedRecvPackets;	// List of inbound packets

	ULONG		pendedSendBytes;	// Number of bytes in outbound packets from pendedSendPackets
	ULONG		pendedRecvBytes;	// Number of bytes in inbound packets from pendedRecvPackets
	
	ULONG		injectedSendBytes;	// Number of bytes in injected outbound packets
	ULONG		injectedRecvBytes;	// Number of bytes in injected inbound packets

	BOOLEAN		sendInProgress;		// TRUE if the number of injected outbound bytes reaches a limit
	BOOLEAN		recvInProgress;		// TRUE if the number of injected inbound bytes reaches a limit

	UDP_REDIRECT_INFO	redirectInfo;
	BOOLEAN		seenPackets;
	BOOLEAN		redirected;

	tFlowControlHandle	fcHandle;
	uint64_t		inLastTS;
	uint64_t		outLastTS;

	uint64_t		inCounter;
	uint64_t		outCounter;

	uint64_t		inCounterTotal;
	uint64_t		outCounterTotal;

	BOOLEAN			filteringDisabled;

	wchar_t			processName[MAX_PATH];

	ULONG		refCount;			// Reference counter

	LIST_ENTRY	auxEntry;			// List entry for adding context to temp lists

	KSPIN_LOCK	lock;				// Context spinlock
} UDPCTX, *PUDPCTX;

#pragma pack(push, 1)

typedef struct _NF_UDP_PACKET_OPTIONS
{	
	COMPARTMENT_ID		compartmentId;
	UINT64				endpointHandle;
	SCOPE_ID			remoteScopeId;
	IF_INDEX			interfaceIndex;
	IF_INDEX			subInterfaceIndex;
	ULONG				controlDataLength;
	UINT32				transportHeaderLength;
	unsigned char		localAddr[NF_MAX_ADDRESS_LENGTH]; 
} NF_UDP_PACKET_OPTIONS, *PNF_UDP_PACKET_OPTIONS;

#pragma pack(pop)

typedef struct _NF_UDP_PACKET
{	
	LIST_ENTRY			entry;
	unsigned char		remoteAddr[NF_MAX_ADDRESS_LENGTH]; 
	NF_UDP_PACKET_OPTIONS options;
	WSACMSGHDR*			controlData;
	char *				dataBuffer;
	ULONG				dataLength;
	FWPS_TRANSPORT_SEND_PARAMS0 sendArgs;
} NF_UDP_PACKET, *PNF_UDP_PACKET;

BOOLEAN udpctx_init();
void udpctx_free();
PUDPCTX udpctx_alloc(UINT64 transportEndpointHandle);
void udpctx_addRef(PUDPCTX pUdpCtx);
void udpctx_release(PUDPCTX pUdpCtx);
PUDPCTX udpctx_find(HASH_ID id);
PNF_UDP_PACKET udpctx_allocPacket(ULONG dataLength);
void udpctx_freePacket(PNF_UDP_PACKET pPacket);
void udpctx_cleanupFlows();
void udpctx_postCreateEvents();
void udpctx_purgeRedirectInfo(PUDPCTX pUdpCtx);
PUDPCTX udpctx_findByHandle(HASH_ID handle);
BOOLEAN udpctx_setFlowControlHandle(ENDPOINT_ID id, tFlowControlHandle fcHandle);
void udpctx_disableFlowControlHandle(tFlowControlHandle fcHandle);

#endif

