//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _TCPCTX_H
#define _TCPCTX_H

#include "hashtable.h"
#include "nfdriver.h"
#include "flowctl.h"

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef	u_long	tcp_seq;
typedef u_long	tcp_cc;			

#pragma pack(1) 

typedef struct _TCP_HEADER 
{
	u_short	th_sport;		/* source port */
	u_short	th_dport;		/* destination port */
	tcp_seq	th_seq;			/* sequence number */
	tcp_seq	th_ack;			/* acknowledgement number */
	u_char	th_off:4,			/* data offset */
		th_x2:4;
	u_char	th_flags;
#define	TH_FIN	0x01
#define	TH_SYN	0x02
#define	TH_RST	0x04
#define	TH_PUSH	0x08
#define	TH_ACK	0x10
#define	TH_URG	0x20
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG)

	u_short	th_win;			/* window */
	u_short	th_sum;			/* checksum */
	u_short	th_urp;			/* urgent pointer */
} TCP_HEADER, *PTCP_HEADER;

#pragma pack()


typedef struct _REDIRECT_INFO
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
} REDIRECT_INFO, *PREDIRECT_INFO;

typedef enum _UMT_FILTERING_STATE
{
	UMFS_NONE,
	UMFS_DISABLE,
	UMFS_DISABLED
} UMT_FILTERING_STATE;

typedef struct _TCPCTX
{
	LIST_ENTRY entry;
	LIST_ENTRY	injectQueueEntry; // Inject queue list entry

	LIST_ENTRY	pendedPackets;	// List of queued packets
	LIST_ENTRY	injectPackets;	// List of packets to inject

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
	UCHAR		direction;		// Connection direction (NF_D_IN or NF_D_OUT)
	USHORT      ipProto;		// protocol

	UINT64      flowHandle;		// WFP flow handle
	UINT16		layerId;		// WFP layer id
	UINT32		sendCalloutId;		// WFP send callout id
	UINT32		recvCalloutId;		// WFP receive callout id
	UINT32		recvProtCalloutId;		// WFP receive protection callout id

	BOOLEAN		recvCalloutInjectable;
	BOOLEAN		recvCalloutBypass;
	BOOLEAN		sendCalloutInjectable;
	BOOLEAN		sendCalloutBypass;

	UINT16		transportLayerIdOut;		// WFP outbound transport layer id
	UINT32		transportCalloutIdOut;		// WFP outbound transport callout id
	UINT16		transportLayerIdIn;		// WFP inbound transport layer id
	UINT32		transportCalloutIdIn;		// WFP inbound transport callout id 

	BOOLEAN		inInjectQueue;	// TRUE if the context is in inject queue

	ULONG		pendedSendBytes;	// Number of bytes in outbound packets from pendedSendPackets
	ULONG		pendedRecvBytes;	// Number of bytes in inbound packets from pendedRecvPackets
	
	ULONG		injectedSendBytes;	// Number of bytes in injected outbound packets
	ULONG		injectedRecvBytes;	// Number of bytes in injected inbound packets

	BOOLEAN		recvDeferred;		// TRUE if connection is deferred

	BOOLEAN		sendDisconnectPending;	// TRUE if outbound disconnect request is received
	BOOLEAN		recvDisconnectPending;	// TRUE if inbound disconnect request is received 
	BOOLEAN		abortConnection;		// TRUE if the connection must be aborted

	BOOLEAN		sendDisconnectCalled;	// TRUE if outbound disconnect request is sent
	BOOLEAN		recvDisconnectCalled;	// TRUE if inbound disconnect request is sent

	BOOLEAN		sendInProgress;		// TRUE if the number of injected outbound bytes reaches a limit
	BOOLEAN		recvInProgress;		// TRUE if the number of injected inbound bytes reaches a limit

	BOOLEAN		noDelay;		// TRUE if no delay flag must be applied to outbound packets

	BOOLEAN		finReceived;	// TRUE if FIN packet received
	BOOLEAN		finWithData;	// TRUE if FIN packet with non-zero length received

	ULONG		pendedRecvProtBytes;	// Number of bytes in inbound packets on the fly from recvProt sublayer
	BOOLEAN		finReceivedOnRecvProt;	// TRUE if FIN packet is received on recvProt sublayer

	tFlowControlHandle	fcHandle;
	uint64_t		inLastTS;
	uint64_t		outLastTS;
	
	uint64_t		inCounter;
	uint64_t		outCounter;

	uint64_t		inCounterTotal;
	uint64_t		outCounterTotal;

	REDIRECT_INFO	redirectInfo;

	UMT_FILTERING_STATE	filteringState;

	wchar_t			processName[MAX_PATH];

	ULONG		refCount;		// Reference counter

	KSPIN_LOCK	lock;			// Context spinlock
} TCPCTX, *PTCPCTX;

typedef struct _NF_PACKET
{	
	LIST_ENTRY			entry;
	PTCPCTX				pTcpCtx;
	UINT32				calloutId;
	BOOLEAN				isClone;
	FWPS_STREAM_DATA	streamData;			// Packet data
	char *				flatStreamData;		// Flat buffer for large packets
	UINT64				flatStreamOffset;	// Current offset in flatStreamData
} NF_PACKET, *PNF_PACKET;

BOOLEAN tcpctx_init();
void tcpctx_free();
PTCPCTX tcpctx_alloc();
void tcpctx_addRef(PTCPCTX pTcpCtx);
void tcpctx_release(PTCPCTX pTcpCtx);
PTCPCTX tcpctx_find(HASH_ID id);
void tcpctx_removeFromFlows();
NTSTATUS tcpctx_pushPacket(PTCPCTX pTcpCtx, FWPS_STREAM_DATA * pStreamData, int code, UINT32 calloutId);
PNF_PACKET tcpctx_allocPacket();
void tcpctx_freePacket(PNF_PACKET pPacket);
void tcpctx_closeConnections();
void tcpctx_purgeRedirectInfo(PTCPCTX pTcpCtx);
void tcpctx_addToHandleTable(PTCPCTX pTcpCtx);
void tcpctx_removeFromHandleTable(PTCPCTX pTcpCtx);
PTCPCTX tcpctx_findByHandle(HASH_ID handle);
BOOLEAN tcpctx_isProxy(ULONG processId);
BOOLEAN tcpctx_setFlowControlHandle(ENDPOINT_ID id, tFlowControlHandle fcHandle);
void tcpctx_disableFlowControlHandle(tFlowControlHandle fcHandle);
void tcpctx_releaseFlows();

#endif

