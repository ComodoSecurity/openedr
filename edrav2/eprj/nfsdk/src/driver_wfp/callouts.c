//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "callouts.h"
#include "tcpctx.h"
#include "ipctx.h"
#include "devctrl.h"
#include "rules.h"

#ifdef _WPPTRACE
#include "callouts.tmh"
#endif

#define NFSDK_FLOW_ESTABLISHED_CALLOUT_DESCRIPTION L"NFSDK Flow Established Callout"
#define NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME L"Flow Established Callout"

#define NFSDK_STREAM_CALLOUT_DESCRIPTION L"NFSDK Stream Callout"
#define NFSDK_STREAM_CALLOUT_NAME L"Stream Callout"

#define NFSDK_SUBLAYER_NAME L"NFSDK Sublayer"
#define NFSDK_RECV_SUBLAYER_NAME L"NFSDK Recv Sublayer"

#define NFSDK_PROVIDER_NAME L"NFSDK Provider"

enum CALLOUT_GUIDS
{
	CG_TCP_FLOW_ESTABLISHED_CALLOUT_V4,
	CG_TCP_FLOW_ESTABLISHED_CALLOUT_V6,
	CG_SEND_STREAM_CALLOUT_V4,
	CG_RECV_STREAM_CALLOUT_V4,
	CG_RECV_PROT_STREAM_CALLOUT_V4,
	CG_SEND_STREAM_CALLOUT_V6,
	CG_RECV_STREAM_CALLOUT_V6,
	CG_RECV_PROT_STREAM_CALLOUT_V6,
	CG_UDP_FLOW_ESTABLISHED_CALLOUT_V4,
	CG_UDP_FLOW_ESTABLISHED_CALLOUT_V6,
	CG_DATAGRAM_DATA_CALLOUT_V4,
	CG_DATAGRAM_DATA_CALLOUT_V6,
	CG_OUTBOUND_TRANSPORT_V4,
	CG_OUTBOUND_TRANSPORT_V6,
	CG_INBOUND_TRANSPORT_V4,
	CG_INBOUND_TRANSPORT_V6,
	CG_ALE_CONNECT_REDIRECT_V4,
	CG_ALE_CONNECT_REDIRECT_V6,
	CG_ALE_ENDPOINT_CLOSURE_V4,
	CG_ALE_ENDPOINT_CLOSURE_V6,
	CG_UDP_ALE_CONNECT_REDIRECT_V4,
	CG_UDP_ALE_CONNECT_REDIRECT_V6,
	CG_UDP_ALE_ENDPOINT_CLOSURE_V4,
	CG_UDP_ALE_ENDPOINT_CLOSURE_V6,
	CG_OUTBOUND_IPPACKET_V4,
	CG_OUTBOUND_IPPACKET_V6,
	CG_INBOUND_IPPACKET_V4,
	CG_INBOUND_IPPACKET_V6,
	CG_ALE_BIND_REDIRECT_V4,
	CG_ALE_BIND_REDIRECT_V6,
	CG_MAX
};

static GUID		g_sublayerGuid;
static GUID		g_recvSublayerGuid;
static GUID		g_recvProtSublayerGuid;
static GUID		g_ipSublayerGuid;
static GUID		g_calloutGuids[CG_MAX];
static UINT32	g_calloutIds[CG_MAX];
static HANDLE	g_engineHandle = NULL;
static GUID		g_providerGuid;
static BOOLEAN	g_blockRST = TRUE;
static BOOLEAN	g_blockUnexpectedRecvDisconnects = TRUE;
static BOOLEAN	g_fastRecvDisconnectOnFinWithData = TRUE;
static BOOLEAN	g_bypassConnectRedirectWithoutActionWrite = TRUE;

static BOOLEAN	g_initialized = FALSE;

NTSTATUS callouts_flowEstablishedNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

NTSTATUS callouts_streamNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE         notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_streamFlowDeletion(
   IN UINT16 layerId,
   IN UINT32 calloutId,
   IN UINT64 flowContext);

VOID callouts_recvStreamFlowDeletion(
   IN UINT16 layerId,
   IN UINT32 calloutId,
   IN UINT64 flowContext);

VOID callouts_flowEstablishedCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_recvStreamCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_sendStreamCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_streamCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut,
   BOOLEAN bSend);

NTSTATUS callouts_udpFlowEstablishedNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

NTSTATUS callouts_udpNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE         notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_udpFlowEstablishedCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_udpCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS callouts_tcpPacketNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_tcpPacketFlowDeletion(
   IN UINT16 layerId,
   IN UINT32 calloutId,
   IN UINT64 flowContext);

VOID callouts_tcpOutboundPacketCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_tcpInboundPacketCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_connectRedirectCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS callouts_connectRedirectNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_endpointClosureCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS callouts_endpointClosureNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_connectRedirectUdpCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER * filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS callouts_connectRedirectUdpNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_endpointClosureUdpCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS callouts_endpointClosureUdpNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);


VOID callouts_streamCalloutRecvProt(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);


NTSTATUS callouts_IPPacketNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

VOID callouts_IPPacketCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

VOID callouts_bindRedirectCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut);

NTSTATUS callouts_bindRedirectNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter);

void
callouts_unregisterCallouts()
{
	NTSTATUS status;
	int i;

	tcpctx_removeFromFlows();

	devctrl_sleep(2000);

	for (i=0; i<CG_MAX; i++)
	{
		status = FwpsCalloutUnregisterByKey(&g_calloutGuids[i]);
		if (!NT_SUCCESS(status))
		{
			ASSERT(0);
		}
	}

	// EDR_FIX: Register additional callouts
	cmdedr_unregisterCallouts();

	devctrl_sleep(3000);
}

struct NF_CALLOUT
{
   FWPS_CALLOUT_CLASSIFY_FN classifyFunction;
   FWPS_CALLOUT_NOTIFY_FN notifyFunction;
   FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN flowDeleteFunction;
   GUID const* calloutKey;
   UINT32 flags;
   UINT32* calloutId;
} g_callouts[] = {
	{
        (FWPS_CALLOUT_CLASSIFY_FN)callouts_flowEstablishedCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_flowEstablishedNotify,
        NULL,
        &g_calloutGuids[CG_TCP_FLOW_ESTABLISHED_CALLOUT_V4],
        0, // No flags.
        &g_calloutIds[CG_TCP_FLOW_ESTABLISHED_CALLOUT_V4]
	},
	{
        (FWPS_CALLOUT_CLASSIFY_FN)callouts_flowEstablishedCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_flowEstablishedNotify,
        NULL, // We don't need a flow delete function at this layer.
        &g_calloutGuids[CG_TCP_FLOW_ESTABLISHED_CALLOUT_V6],
        0, // No flags.
        &g_calloutIds[CG_TCP_FLOW_ESTABLISHED_CALLOUT_V6]
	},
	{
        (FWPS_CALLOUT_CLASSIFY_FN)callouts_sendStreamCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_streamNotify,
		(FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_streamFlowDeletion,
        &g_calloutGuids[CG_SEND_STREAM_CALLOUT_V4],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        &g_calloutIds[CG_SEND_STREAM_CALLOUT_V4]
	},
	{
        (FWPS_CALLOUT_CLASSIFY_FN)callouts_recvStreamCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_streamNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_recvStreamFlowDeletion,
        &g_calloutGuids[CG_RECV_STREAM_CALLOUT_V4],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        &g_calloutIds[CG_RECV_STREAM_CALLOUT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_streamCalloutRecvProt,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_streamNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_recvStreamFlowDeletion,
        &g_calloutGuids[CG_RECV_PROT_STREAM_CALLOUT_V4],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        &g_calloutIds[CG_RECV_PROT_STREAM_CALLOUT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_sendStreamCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_streamNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_streamFlowDeletion,
        &g_calloutGuids[CG_SEND_STREAM_CALLOUT_V6],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        &g_calloutIds[CG_SEND_STREAM_CALLOUT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_recvStreamCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_streamNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_recvStreamFlowDeletion,
        &g_calloutGuids[CG_RECV_STREAM_CALLOUT_V6],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        &g_calloutIds[CG_RECV_STREAM_CALLOUT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_streamCalloutRecvProt,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_streamNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_recvStreamFlowDeletion,
        &g_calloutGuids[CG_RECV_PROT_STREAM_CALLOUT_V6],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
        &g_calloutIds[CG_RECV_PROT_STREAM_CALLOUT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_udpFlowEstablishedCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_udpFlowEstablishedNotify,
        NULL, // We don't need a flow delete function at this layer.
        &g_calloutGuids[CG_UDP_FLOW_ESTABLISHED_CALLOUT_V4],
        0, // No flags.
        &g_calloutIds[CG_UDP_FLOW_ESTABLISHED_CALLOUT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_udpFlowEstablishedCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_udpFlowEstablishedNotify,
        NULL, // We don't need a flow delete function at this layer.
        &g_calloutGuids[CG_UDP_FLOW_ESTABLISHED_CALLOUT_V6],
        0, // No flags.
        &g_calloutIds[CG_UDP_FLOW_ESTABLISHED_CALLOUT_V6]
	},
	{
        (FWPS_CALLOUT_CLASSIFY_FN)callouts_udpCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_udpNotify,
        NULL,
		&g_calloutGuids[CG_DATAGRAM_DATA_CALLOUT_V4],
        0, // No flags
		&g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_udpCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_udpNotify,
        NULL,
		&g_calloutGuids[CG_DATAGRAM_DATA_CALLOUT_V6],
        0, // No flags
		&g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_tcpOutboundPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_tcpPacketNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_tcpPacketFlowDeletion,
		&g_calloutGuids[CG_OUTBOUND_TRANSPORT_V4],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
		&g_calloutIds[CG_OUTBOUND_TRANSPORT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_tcpOutboundPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_tcpPacketNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_tcpPacketFlowDeletion,
		&g_calloutGuids[CG_OUTBOUND_TRANSPORT_V6],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
		&g_calloutIds[CG_OUTBOUND_TRANSPORT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_tcpInboundPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_tcpPacketNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_tcpPacketFlowDeletion,
		&g_calloutGuids[CG_INBOUND_TRANSPORT_V4],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
		&g_calloutIds[CG_INBOUND_TRANSPORT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_tcpInboundPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_tcpPacketNotify,
        (FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN)callouts_tcpPacketFlowDeletion,
		&g_calloutGuids[CG_INBOUND_TRANSPORT_V6],
        FWP_CALLOUT_FLAG_CONDITIONAL_ON_FLOW,
		&g_calloutIds[CG_INBOUND_TRANSPORT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_connectRedirectCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_connectRedirectNotify,
        NULL,
		&g_calloutGuids[CG_ALE_CONNECT_REDIRECT_V4],
        0,
		&g_calloutIds[CG_ALE_CONNECT_REDIRECT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_connectRedirectCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_connectRedirectNotify,
        NULL,
		&g_calloutGuids[CG_ALE_CONNECT_REDIRECT_V6],
        0,
		&g_calloutIds[CG_ALE_CONNECT_REDIRECT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_endpointClosureCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_endpointClosureNotify,
        NULL,
		&g_calloutGuids[CG_ALE_ENDPOINT_CLOSURE_V4],
        0,
		&g_calloutIds[CG_ALE_ENDPOINT_CLOSURE_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_endpointClosureCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_endpointClosureNotify,
        NULL,
		&g_calloutGuids[CG_ALE_ENDPOINT_CLOSURE_V6],
        0,
		&g_calloutIds[CG_ALE_ENDPOINT_CLOSURE_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_connectRedirectUdpCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_connectRedirectUdpNotify,
        NULL,
		&g_calloutGuids[CG_UDP_ALE_CONNECT_REDIRECT_V4],
        0,
		&g_calloutIds[CG_UDP_ALE_CONNECT_REDIRECT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_connectRedirectUdpCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_connectRedirectUdpNotify,
        NULL,
		&g_calloutGuids[CG_UDP_ALE_CONNECT_REDIRECT_V6],
        0,
		&g_calloutIds[CG_UDP_ALE_CONNECT_REDIRECT_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_endpointClosureUdpCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_endpointClosureUdpNotify,
        NULL,
		&g_calloutGuids[CG_UDP_ALE_ENDPOINT_CLOSURE_V4],
        0,
		&g_calloutIds[CG_UDP_ALE_ENDPOINT_CLOSURE_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_endpointClosureUdpCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_endpointClosureUdpNotify,
        NULL,
		&g_calloutGuids[CG_UDP_ALE_ENDPOINT_CLOSURE_V6],
        0,
		&g_calloutIds[CG_UDP_ALE_ENDPOINT_CLOSURE_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_IPPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_IPPacketNotify,
        NULL,
		&g_calloutGuids[CG_INBOUND_IPPACKET_V4],
        0, // No flags
		&g_calloutIds[CG_INBOUND_IPPACKET_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_IPPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_IPPacketNotify,
        NULL,
		&g_calloutGuids[CG_OUTBOUND_IPPACKET_V4],
        0, // No flags
		&g_calloutIds[CG_OUTBOUND_IPPACKET_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_IPPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_IPPacketNotify,
        NULL,
		&g_calloutGuids[CG_INBOUND_IPPACKET_V6],
        0, // No flags
		&g_calloutIds[CG_INBOUND_IPPACKET_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_IPPacketCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_IPPacketNotify,
        NULL,
		&g_calloutGuids[CG_OUTBOUND_IPPACKET_V6],
        0, // No flags
		&g_calloutIds[CG_OUTBOUND_IPPACKET_V6]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_bindRedirectCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_bindRedirectNotify,
        NULL,
		&g_calloutGuids[CG_ALE_BIND_REDIRECT_V4],
        0,
		&g_calloutIds[CG_ALE_BIND_REDIRECT_V4]
	},
	{
		(FWPS_CALLOUT_CLASSIFY_FN)callouts_bindRedirectCallout,
        (FWPS_CALLOUT_NOTIFY_FN)callouts_bindRedirectNotify,
        NULL,
		&g_calloutGuids[CG_ALE_BIND_REDIRECT_V6],
        0,
		&g_calloutIds[CG_ALE_BIND_REDIRECT_V6]
	}

};

NTSTATUS
callouts_registerCallout(
   IN OUT void* deviceObject,
   IN  FWPS_CALLOUT_CLASSIFY_FN classifyFunction,
   IN  FWPS_CALLOUT_NOTIFY_FN notifyFunction,
   IN  FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN flowDeleteFunction,
   IN  GUID const* calloutKey,
   IN  UINT32 flags,
   OUT UINT32* calloutId
   )
{
    FWPS_CALLOUT sCallout;
    NTSTATUS status = STATUS_SUCCESS;

    memset(&sCallout, 0, sizeof(sCallout));

    sCallout.calloutKey = *calloutKey;
    sCallout.flags = flags;
    sCallout.classifyFn = classifyFunction;
    sCallout.notifyFn = notifyFunction;
    sCallout.flowDeleteFn = flowDeleteFunction;

    status = FwpsCalloutRegister1(deviceObject, (FWPS_CALLOUT1*)&sCallout, calloutId);

    return status;
}


NTSTATUS
callouts_registerCallouts(
   IN OUT void* deviceObject
   )
{
	NTSTATUS status = STATUS_SUCCESS;
	int i;

	status = FwpmTransactionBegin(g_engineHandle, 0);
	if (!NT_SUCCESS(status))
	{
		FwpmEngineClose(g_engineHandle);
        g_engineHandle = NULL;
		return status;
	}

	for(;;)
	{
		for (i=0; i<sizeof(g_callouts)/sizeof(g_callouts[0]); i++)
		{
			status = callouts_registerCallout(deviceObject,
				g_callouts[i].classifyFunction,
				g_callouts[i].notifyFunction,
				g_callouts[i].flowDeleteFunction,
				g_callouts[i].calloutKey,
				g_callouts[i].flags,
				g_callouts[i].calloutId);
			
			if (!NT_SUCCESS(status))
			{
				break;
			}
		}

		if (!NT_SUCCESS(status))
		{
			break;
		}

		// EDR_FIX: Register additional callouts
		status = cmdedr_registerCallouts(deviceObject);
		if (!NT_SUCCESS(status)) 
			break;

		status = FwpmTransactionCommit(g_engineHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		break;
	}

	if (!NT_SUCCESS(status))
	{
		FwpmTransactionAbort(g_engineHandle);
		FwpmEngineClose(g_engineHandle);
        g_engineHandle = NULL;
	}
	
	return status;
}

NTSTATUS
callouts_addFlowEstablishedFilter(const GUID * calloutKey, const GUID * layer, FWPM_SUBLAYER * subLayer)
{
	FWPM_CALLOUT callout;
	FWPM_DISPLAY_DATA displayData;
	FWPM_FILTER filter;
	FWPM_FILTER_CONDITION filterConditions[1];
	NTSTATUS status;

	for (;;)
	{
		RtlZeroMemory(&callout, sizeof(FWPM_CALLOUT));
		displayData.description = NFSDK_FLOW_ESTABLISHED_CALLOUT_DESCRIPTION;
		displayData.name = NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME;

		callout.calloutKey = *calloutKey;
		callout.displayData = displayData;
		callout.applicableLayer = *layer;
		callout.flags = 0; 

		status = FwpmCalloutAdd(g_engineHandle, &callout, NULL, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

		filter.layerKey = *layer;
		filter.displayData.name = NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME;
		filter.displayData.description = NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME;
		filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		filter.action.calloutKey = *calloutKey;
		filter.filterCondition = filterConditions;
		filter.subLayerKey = subLayer->subLayerKey;
		filter.weight.type = FWP_EMPTY; // auto-weight.
		  
		filter.numFilterConditions = 1;

		RtlZeroMemory(filterConditions, sizeof(filterConditions));

		filterConditions[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
		filterConditions[0].matchType = FWP_MATCH_EQUAL;
		filterConditions[0].conditionValue.type = FWP_UINT8;
		filterConditions[0].conditionValue.uint8 = IPPROTO_TCP;

		status = FwpmFilterAdd(g_engineHandle,
						   &filter,
						   NULL,
						   NULL);

		if (!NT_SUCCESS(status))
		{
			break;
		}
	
		break;
	} 

	return status;
}

NTSTATUS
callouts_addStreamFilter(const GUID * calloutKey, const GUID * layer, FWPM_SUBLAYER * subLayer)
{
	FWPM_CALLOUT callout;
	FWPM_DISPLAY_DATA displayData;
	FWPM_FILTER filter;
	NTSTATUS status;

	for (;;)
	{
		RtlZeroMemory(&callout, sizeof(FWPM_CALLOUT));
		displayData.description = NFSDK_STREAM_CALLOUT_DESCRIPTION;
		displayData.name = NFSDK_STREAM_CALLOUT_NAME;

		callout.calloutKey = *calloutKey;
		callout.displayData = displayData;
		callout.applicableLayer = *layer;
		callout.flags = 0; 

		status = FwpmCalloutAdd(g_engineHandle, &callout, NULL, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

		filter.layerKey = *layer;
		filter.displayData.name = NFSDK_STREAM_CALLOUT_NAME;
		filter.displayData.description = NFSDK_STREAM_CALLOUT_NAME;
		filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		filter.action.calloutKey = *calloutKey;
		filter.filterCondition = NULL;
		filter.subLayerKey = subLayer->subLayerKey;
		filter.weight.type = FWP_EMPTY; // auto-weight.
		filter.numFilterConditions = 0;

		status = FwpmFilterAdd(g_engineHandle,
						   &filter,
						   NULL,
						   NULL);

		if (!NT_SUCCESS(status))
		{
			break;
		}
	
		break;
	} 

	return status;
}


NTSTATUS
callouts_addUdpFlowEstablishedFilter(const GUID * calloutKey, const GUID * layer, FWPM_SUBLAYER * subLayer)
{
	FWPM_CALLOUT callout;
	FWPM_DISPLAY_DATA displayData;
	FWPM_FILTER filter;
	FWPM_FILTER_CONDITION filterConditions[1];
	NTSTATUS status;

	for (;;)
	{
		RtlZeroMemory(&callout, sizeof(FWPM_CALLOUT));
		displayData.description = NFSDK_FLOW_ESTABLISHED_CALLOUT_DESCRIPTION;
		displayData.name = NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME;

		callout.calloutKey = *calloutKey;
		callout.displayData = displayData;
		callout.applicableLayer = *layer;
		callout.flags = 0; 

		status = FwpmCalloutAdd(g_engineHandle, &callout, NULL, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

		filter.layerKey = *layer;
		filter.displayData.name = NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME;
		filter.displayData.description = NFSDK_FLOW_ESTABLISHED_CALLOUT_NAME;
		filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		filter.action.calloutKey = *calloutKey;
		filter.filterCondition = filterConditions;
		filter.subLayerKey = subLayer->subLayerKey;
		filter.weight.type = FWP_EMPTY; // auto-weight.
		  
		filter.numFilterConditions = 1;

		RtlZeroMemory(filterConditions, sizeof(filterConditions));

		filterConditions[0].fieldKey = FWPM_CONDITION_IP_PROTOCOL;
		filterConditions[0].matchType = FWP_MATCH_EQUAL;
		filterConditions[0].conditionValue.type = FWP_UINT8;
		filterConditions[0].conditionValue.uint8 = IPPROTO_UDP;

		status = FwpmFilterAdd(g_engineHandle,
						   &filter,
						   NULL,
						   NULL);

		if (!NT_SUCCESS(status))
		{
			break;
		}
	
		break;
	} 

	return status;
}

NTSTATUS
callouts_addDatagramDataFilter(const GUID * calloutKey, const GUID * layer, FWPM_SUBLAYER * subLayer)
{
	FWPM_CALLOUT callout;
	FWPM_DISPLAY_DATA displayData;
	FWPM_FILTER filter;
	NTSTATUS status;

	for (;;)
	{
		RtlZeroMemory(&callout, sizeof(FWPM_CALLOUT));
		displayData.description = NFSDK_STREAM_CALLOUT_DESCRIPTION;
		displayData.name = NFSDK_STREAM_CALLOUT_NAME;

		callout.calloutKey = *calloutKey;
		callout.displayData = displayData;
		callout.applicableLayer = *layer;
		callout.flags = 0; 

		status = FwpmCalloutAdd(g_engineHandle, &callout, NULL, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&filter, sizeof(FWPM_FILTER));

		filter.layerKey = *layer;
		filter.displayData.name = NFSDK_STREAM_CALLOUT_NAME;
		filter.displayData.description = NFSDK_STREAM_CALLOUT_NAME;
		filter.action.type = FWP_ACTION_CALLOUT_TERMINATING;
		filter.action.calloutKey = *calloutKey;
		filter.filterCondition = NULL;
		filter.subLayerKey = subLayer->subLayerKey;
		filter.weight.type = FWP_EMPTY; // auto-weight.
		filter.numFilterConditions = 0;

		status = FwpmFilterAdd(g_engineHandle,
						   &filter,
						   NULL,
						   NULL);

		if (!NT_SUCCESS(status))
		{
			break;
		}
	
		break;
	} 

	return status;
}

BOOLEAN callouts_isException()
{
	wchar_t * flt_services[] = {
//		L"epfwwfp", L"epfwwfpr",	// NOD32
		L"nisdrv", L"symnets",		// Symantec
//		L"klwfp",					// Kaspersky
		L"amoncdw8", L"amoncdw7",	// AhnLab
		NULL
	};
	wchar_t ** psvc_name;
	wchar_t tmpbuf[100];

	for (psvc_name = &flt_services[0]; *psvc_name != NULL; psvc_name++)
	{
		swprintf_s(tmpbuf, sizeof(tmpbuf)/2, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\services\\%s", *psvc_name);

		if (regPathExists(tmpbuf))
			return TRUE;
	}

	return FALSE;
}

BOOLEAN callouts_isDriverRegistered(wchar_t * svc_name)
{
	wchar_t tmpbuf[100];

	swprintf_s(tmpbuf, sizeof(tmpbuf)/2, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\services\\%s", svc_name);

	if (regPathExists(tmpbuf))
		return TRUE;

	return FALSE;
}

UINT16 callouts_getSublayerWeight()
{
	HANDLE hEnum = NULL;
	UINT16 minWeight;
	UINT16 weightRange = 65000;

//	DbgBreakPoint();

	if (callouts_isException())
	{
		weightRange = 51;
	}

	minWeight = weightRange;

	if (FwpmSubLayerCreateEnumHandle(g_engineHandle, NULL, &hEnum) != 0)
		return minWeight;
	
	for (;;)
	{
		FWPM_SUBLAYER ** pSublayers;
		UINT32 nItems;
		
		if (FwpmSubLayerEnum(g_engineHandle, hEnum, 1, &pSublayers, &nItems) != 0)
			break;
		if (nItems < 1)
			break;

		if (pSublayers[0]->weight > (weightRange-50) &&
			pSublayers[0]->weight <= weightRange)
		{
			if (pSublayers[0]->weight < minWeight)
				minWeight = pSublayers[0]->weight;
		}

		KdPrint((DPREFIX"callouts_getSublayerWeight %d, %ws, %ws\n", 
			pSublayers[0]->weight,
			pSublayers[0]->displayData.name,
			pSublayers[0]->displayData.description));

		FwpmFreeMemory((void**)&pSublayers);
	} 
	FwpmSubLayerDestroyEnumHandle(g_engineHandle, hEnum);

	return minWeight - 1;
}

NTSTATUS
callouts_addFilters()
{
	FWPM_SUBLAYER subLayer, recvSubLayer, recvProtSubLayer, ipSubLayer;
	NTSTATUS status;
	USHORT subLayerWeight, recvSubLayerWeight;
	FWPM_SUBLAYER * pConnectRedirectSubLayer = NULL, 
		* pUdpSubLayer = NULL,
		* pUdpConnectRedirectSubLayer = NULL;
	DWORD disabledCallouts;
	
	disabledCallouts = devctrl_getDisabledCallouts();

	if (callouts_isDriverRegistered(L"swi_callout")) 
	{
		KdPrint((DPREFIX"callouts_addFilters: Sophos order for connect redirect sublayer\n"));
		pConnectRedirectSubLayer = &subLayer;
		pUdpConnectRedirectSubLayer = &subLayer;

		g_bypassConnectRedirectWithoutActionWrite = TRUE;
	} else
	{
		KdPrint((DPREFIX"callouts_addFilters: default order for sublayers\n"));
		pConnectRedirectSubLayer = &recvSubLayer;
		pUdpConnectRedirectSubLayer = &recvSubLayer;
	}

	// Workaround to avoid SSL filtering issues with antiviruses
	if (callouts_isDriverRegistered(L"klwfp")) 
	{
		KdPrint((DPREFIX"callouts_addFilters: Kaspersky weight for sublayers\n"));

		subLayerWeight = 1;
		recvSubLayerWeight = callouts_getSublayerWeight();

		pConnectRedirectSubLayer = &recvSubLayer;

		pUdpSubLayer = &recvSubLayer;
		pUdpConnectRedirectSubLayer = &subLayer;

		g_fastRecvDisconnectOnFinWithData = FALSE;
		g_bypassConnectRedirectWithoutActionWrite = TRUE;
	} else
	if (callouts_isDriverRegistered(L"fsni")) 
	{
		KdPrint((DPREFIX"callouts_addFilters: F-Secure weight for sublayers\n"));

		subLayerWeight = 1;
		recvSubLayerWeight = 1;

		g_blockUnexpectedRecvDisconnects = FALSE;

		pUdpSubLayer = &recvSubLayer;
		pUdpConnectRedirectSubLayer = &recvSubLayer;
	} else
	if (callouts_isDriverRegistered(L"bdfwfpf_pc")) 
	{
		KdPrint((DPREFIX"callouts_addFilters: BitDefender weight for sublayers\n"));

		pConnectRedirectSubLayer = &recvSubLayer;

		subLayerWeight = callouts_getSublayerWeight();
		recvSubLayerWeight = 1;

		pUdpSubLayer = &recvSubLayer;
		pUdpConnectRedirectSubLayer = &subLayer;

		g_blockUnexpectedRecvDisconnects = FALSE;
		g_fastRecvDisconnectOnFinWithData = FALSE;
	} else
	if (callouts_isDriverRegistered(L"epfwwfp") ||
		callouts_isDriverRegistered(L"epfwwfpr") ||
		callouts_isDriverRegistered(L"aswstm")) 
	{
		KdPrint((DPREFIX"callouts_addFilters: NOD32 weight for sublayers\n"));

		pConnectRedirectSubLayer = &recvSubLayer;

		subLayerWeight = callouts_getSublayerWeight();
		recvSubLayerWeight = 1;

		pUdpSubLayer = &subLayer;
		pUdpConnectRedirectSubLayer = &subLayer;

		g_blockUnexpectedRecvDisconnects = FALSE;
		g_fastRecvDisconnectOnFinWithData = FALSE;
//		g_bypassConnectRedirectWithoutActionWrite = FALSE;
	} else
	{
		KdPrint((DPREFIX"callouts_addFilters: default weight for sublayers\n"));

		subLayerWeight = 1;
		recvSubLayerWeight = callouts_getSublayerWeight();

		pUdpSubLayer = &recvSubLayer;
		pUdpConnectRedirectSubLayer = &subLayer;
	} 

	// Filter TCP sends before Cisco AnyConnect callout
	if (callouts_isDriverRegistered(L"acsock"))
	{
		KdPrint((DPREFIX"callouts_addFilters: Cisco AnyConnect weight for sublayers\n"));

		if (recvSubLayerWeight > subLayerWeight)
		{
			subLayerWeight = recvSubLayerWeight;
			recvSubLayerWeight = 1;

			g_blockUnexpectedRecvDisconnects = FALSE;
		}
	}


	status = FwpmTransactionBegin(g_engineHandle, 0);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	
	for (;;)
	{
		RtlZeroMemory(&subLayer, sizeof(FWPM_SUBLAYER)); 

		subLayer.subLayerKey = g_sublayerGuid;
		subLayer.displayData.name = NFSDK_SUBLAYER_NAME;
		subLayer.displayData.description = NFSDK_SUBLAYER_NAME;
		subLayer.flags = 0;
		subLayer.weight = subLayerWeight;

		status = FwpmSubLayerAdd(g_engineHandle, &subLayer, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&recvSubLayer, sizeof(FWPM_SUBLAYER)); 

		recvSubLayer.subLayerKey = g_recvSublayerGuid;
		recvSubLayer.displayData.name = NFSDK_RECV_SUBLAYER_NAME;
		recvSubLayer.displayData.description = NFSDK_RECV_SUBLAYER_NAME;
		recvSubLayer.flags = 0;
		recvSubLayer.weight = recvSubLayerWeight;

		status = FwpmSubLayerAdd(g_engineHandle, &recvSubLayer, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&recvProtSubLayer, sizeof(FWPM_SUBLAYER)); 

		recvProtSubLayer.subLayerKey = g_recvProtSublayerGuid;
		recvProtSubLayer.displayData.name = NFSDK_RECV_SUBLAYER_NAME L"PROT";
		recvProtSubLayer.displayData.description = NFSDK_RECV_SUBLAYER_NAME L"PROT";
		recvProtSubLayer.flags = 0;
		recvProtSubLayer.weight = recvSubLayerWeight+1;

		status = FwpmSubLayerAdd(g_engineHandle, &recvProtSubLayer, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		RtlZeroMemory(&ipSubLayer, sizeof(FWPM_SUBLAYER)); 

		ipSubLayer.subLayerKey = g_ipSublayerGuid;
		ipSubLayer.displayData.name = NFSDK_RECV_SUBLAYER_NAME L"IP";
		ipSubLayer.displayData.description = NFSDK_RECV_SUBLAYER_NAME L"IP";
		ipSubLayer.flags = 0;
		ipSubLayer.weight = 0xffff;

		status = FwpmSubLayerAdd(g_engineHandle, &ipSubLayer, NULL);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		if (!(disabledCallouts & NDC_TCP))
		{
			status = callouts_addFlowEstablishedFilter(
				&g_calloutGuids[CG_TCP_FLOW_ESTABLISHED_CALLOUT_V4], 
				&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addFlowEstablishedFilter(
				&g_calloutGuids[CG_TCP_FLOW_ESTABLISHED_CALLOUT_V6], 
				&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_SEND_STREAM_CALLOUT_V4], 
				&FWPM_LAYER_STREAM_V4,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_RECV_STREAM_CALLOUT_V4], 
				&FWPM_LAYER_STREAM_V4,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_SEND_STREAM_CALLOUT_V6], 
				&FWPM_LAYER_STREAM_V6,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_RECV_STREAM_CALLOUT_V6], 
				&FWPM_LAYER_STREAM_V6,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			if (g_fastRecvDisconnectOnFinWithData)
			{
				status = callouts_addStreamFilter(
					&g_calloutGuids[CG_RECV_PROT_STREAM_CALLOUT_V4], 
					&FWPM_LAYER_STREAM_V4,
					&recvProtSubLayer);
				if (!NT_SUCCESS(status))
				{
					break;
				}

				status = callouts_addStreamFilter(
					&g_calloutGuids[CG_RECV_PROT_STREAM_CALLOUT_V6], 
					&FWPM_LAYER_STREAM_V6,
					&recvProtSubLayer);
				if (!NT_SUCCESS(status))
				{
					break;
				}
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_OUTBOUND_TRANSPORT_V4], 
				&FWPM_LAYER_OUTBOUND_TRANSPORT_V4,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_OUTBOUND_TRANSPORT_V6], 
				&FWPM_LAYER_OUTBOUND_TRANSPORT_V6,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_INBOUND_TRANSPORT_V4], 
				&FWPM_LAYER_INBOUND_TRANSPORT_V4,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_INBOUND_TRANSPORT_V6], 
				&FWPM_LAYER_INBOUND_TRANSPORT_V6,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addFlowEstablishedFilter(
				&g_calloutGuids[CG_ALE_CONNECT_REDIRECT_V4], 
				&FWPM_LAYER_ALE_CONNECT_REDIRECT_V4,
				pConnectRedirectSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addFlowEstablishedFilter(
				&g_calloutGuids[CG_ALE_CONNECT_REDIRECT_V6], 
				&FWPM_LAYER_ALE_CONNECT_REDIRECT_V6,
				pConnectRedirectSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addFlowEstablishedFilter(
				&g_calloutGuids[CG_ALE_ENDPOINT_CLOSURE_V4], 
				&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addFlowEstablishedFilter(
				&g_calloutGuids[CG_ALE_ENDPOINT_CLOSURE_V6], 
				&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6,
				&recvSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}
		}

		if (!(disabledCallouts & NDC_UDP))
		{
			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_UDP_FLOW_ESTABLISHED_CALLOUT_V4], 
				&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V4,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_UDP_FLOW_ESTABLISHED_CALLOUT_V6], 
				&FWPM_LAYER_ALE_FLOW_ESTABLISHED_V6,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_DATAGRAM_DATA_CALLOUT_V4], 
				&FWPM_LAYER_DATAGRAM_DATA_V4,
				pUdpSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_DATAGRAM_DATA_CALLOUT_V6], 
				&FWPM_LAYER_DATAGRAM_DATA_V6,
				pUdpSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_UDP_ALE_CONNECT_REDIRECT_V4], 
				&FWPM_LAYER_ALE_CONNECT_REDIRECT_V4,
				pUdpConnectRedirectSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_UDP_ALE_CONNECT_REDIRECT_V6], 
				&FWPM_LAYER_ALE_CONNECT_REDIRECT_V6,
				pUdpConnectRedirectSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_UDP_ALE_ENDPOINT_CLOSURE_V4], 
				&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V4,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addUdpFlowEstablishedFilter(
				&g_calloutGuids[CG_UDP_ALE_ENDPOINT_CLOSURE_V6], 
				&FWPM_LAYER_ALE_ENDPOINT_CLOSURE_V6,
				&subLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}
		}

		if (!(disabledCallouts & NDC_IP))
		{
			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_OUTBOUND_IPPACKET_V4], 
				&FWPM_LAYER_OUTBOUND_IPPACKET_V4,
				&ipSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_INBOUND_IPPACKET_V4], 
				&FWPM_LAYER_INBOUND_IPPACKET_V4,
				&ipSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_OUTBOUND_IPPACKET_V6], 
				&FWPM_LAYER_OUTBOUND_IPPACKET_V6,
				&ipSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_INBOUND_IPPACKET_V6], 
				&FWPM_LAYER_INBOUND_IPPACKET_V6,
				&ipSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}
		}

		if (!(disabledCallouts & NDC_BIND))
		{
			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_ALE_BIND_REDIRECT_V4], 
				&FWPM_LAYER_ALE_BIND_REDIRECT_V4,
				pConnectRedirectSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}

			status = callouts_addStreamFilter(
				&g_calloutGuids[CG_ALE_BIND_REDIRECT_V6], 
				&FWPM_LAYER_ALE_BIND_REDIRECT_V6,
				pConnectRedirectSubLayer);
			if (!NT_SUCCESS(status))
			{
				break;
			}
		}

		// EDR_FIX: Add additional filters
		struct AddingFilterContext addingFilterContext;
		addingFilterContext.hEngine = g_engineHandle;
		addingFilterContext.pSubLayer = &subLayer;
		addingFilterContext.pRecvSubLayer = &recvSubLayer;
		status = cmdedr_addFilters(&addingFilterContext);
		if (!NT_SUCCESS(status))
			break;

		status = FwpmTransactionCommit(g_engineHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		break;
	} 
	
	if (!NT_SUCCESS(status))
	{
		FwpmTransactionAbort(g_engineHandle);
	}

	return status;
}

void dbgPrintAddress(int ipFamily, void * addr, char * name, UINT64 id)
{
	if (ipFamily == AF_INET)
	{
		struct sockaddr_in * pAddr = (struct sockaddr_in *)addr;

		KdPrint((DPREFIX"dbgPrintAddress[%I64u] %s=%x:%d\n", 
			id, name, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));
	} else
	{
		struct sockaddr_in6 * pAddr = (struct sockaddr_in6 *)addr;

		KdPrint((DPREFIX"dbgPrintAddress[%I64u] %s=[%x:%x:%x:%x:%x:%x:%x:%x]:%d\n", 
				id, name,
				pAddr->sin6_addr.u.Word[0], 
				pAddr->sin6_addr.u.Word[1], 
				pAddr->sin6_addr.u.Word[2], 
				pAddr->sin6_addr.u.Word[3], 
				pAddr->sin6_addr.u.Word[4], 
				pAddr->sin6_addr.u.Word[5], 
				pAddr->sin6_addr.u.Word[6], 
				pAddr->sin6_addr.u.Word[7], 
				htons(pAddr->sin6_port)));
	}
}


PTCPCTX
callouts_createFlowContext(
	PTCPCTX	pTcpCtx,
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues)
{
	if ((ULONG)inMetaValues->processId == devctrl_getProxyPid())
	{
		KdPrint((DPREFIX"callouts_createFlowContext: bypass own connection\n"));
		return NULL;
	}

	if (!pTcpCtx)
	{
		pTcpCtx = tcpctx_alloc();
		if (!pTcpCtx)
		{
			KdPrint((DPREFIX"callouts_createFlowContext: tcpctx_alloc failed\n"));
			return NULL;
		}
	}

	pTcpCtx->closed = FALSE;
	pTcpCtx->flowHandle = inMetaValues->flowHandle;
	pTcpCtx->processId = (ULONG)inMetaValues->processId;

	if (inFixedValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4)
	{
		struct sockaddr_in	*	pAddr;
	
		pTcpCtx->layerId = FWPS_LAYER_STREAM_V4;
		pTcpCtx->sendCalloutId = g_calloutIds[CG_SEND_STREAM_CALLOUT_V4];
		pTcpCtx->recvCalloutId = g_calloutIds[CG_RECV_STREAM_CALLOUT_V4];
		pTcpCtx->recvProtCalloutId = g_calloutIds[CG_RECV_PROT_STREAM_CALLOUT_V4];

		pTcpCtx->transportLayerIdOut = FWPS_LAYER_OUTBOUND_TRANSPORT_V4;
		pTcpCtx->transportCalloutIdOut = g_calloutIds[CG_OUTBOUND_TRANSPORT_V4];
		pTcpCtx->transportLayerIdIn = FWPS_LAYER_INBOUND_TRANSPORT_V4;
		pTcpCtx->transportCalloutIdIn = g_calloutIds[CG_INBOUND_TRANSPORT_V4];

		pTcpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value.uint8;
		pTcpCtx->ip_family = AF_INET;

		pAddr = (struct sockaddr_in*)pTcpCtx->localAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pTcpCtx->id);

		pAddr = (struct sockaddr_in*)pTcpCtx->remoteAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_REMOTE_PORT].value.uint16);

		dbgPrintAddress(AF_INET, pAddr, "remoteAddr", pTcpCtx->id);

		pTcpCtx->direction = (inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_DIRECTION].value.uint16 == FWP_DIRECTION_OUTBOUND)? NF_D_OUT : NF_D_IN;

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pTcpCtx->processName) - 2)
			{
				offset = len - (sizeof(pTcpCtx->processName) - 2);
				len = sizeof(pTcpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pTcpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

	} else
	if (inFixedValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)
	{
		struct sockaddr_in6	*	pAddr;
	
		pTcpCtx->layerId = FWPS_LAYER_STREAM_V6;
		pTcpCtx->sendCalloutId = g_calloutIds[CG_SEND_STREAM_CALLOUT_V6];
		pTcpCtx->recvCalloutId = g_calloutIds[CG_RECV_STREAM_CALLOUT_V6];
		pTcpCtx->recvProtCalloutId = g_calloutIds[CG_RECV_PROT_STREAM_CALLOUT_V6];

		pTcpCtx->transportLayerIdOut = FWPS_LAYER_OUTBOUND_TRANSPORT_V6;
		pTcpCtx->transportCalloutIdOut = g_calloutIds[CG_OUTBOUND_TRANSPORT_V6];
		pTcpCtx->transportLayerIdIn = FWPS_LAYER_INBOUND_TRANSPORT_V6;
		pTcpCtx->transportCalloutIdIn = g_calloutIds[CG_INBOUND_TRANSPORT_V6];

		pTcpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL].value.uint8;
		pTcpCtx->ip_family = AF_INET6;

		pAddr = (struct sockaddr_in6*)pTcpCtx->localAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET6, pAddr, "localAddr", pTcpCtx->id);

		pAddr = (struct sockaddr_in6*)pTcpCtx->remoteAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_REMOTE_PORT].value.uint16);

		dbgPrintAddress(AF_INET6, pAddr, "remoteAddr", pTcpCtx->id);

        if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
		{
       	 	pAddr->sin6_scope_id = inMetaValues->remoteScopeId.Value;
    	}		
			
		pTcpCtx->direction = (inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_DIRECTION].value.uint16 == FWP_DIRECTION_OUTBOUND)? NF_D_OUT : NF_D_IN;

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pTcpCtx->processName) - 2)
			{
				offset = len - (sizeof(pTcpCtx->processName) - 2);
				len = sizeof(pTcpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pTcpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

	} else
	if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4)
	{
		struct sockaddr_in	*	pAddr;
	
		pTcpCtx->layerId = FWPS_LAYER_STREAM_V4;
		pTcpCtx->sendCalloutId = g_calloutIds[CG_SEND_STREAM_CALLOUT_V4];
		pTcpCtx->recvCalloutId = g_calloutIds[CG_RECV_STREAM_CALLOUT_V4];
		pTcpCtx->recvProtCalloutId = g_calloutIds[CG_RECV_PROT_STREAM_CALLOUT_V4];

		pTcpCtx->transportLayerIdOut = FWPS_LAYER_OUTBOUND_TRANSPORT_V4;
		pTcpCtx->transportCalloutIdOut = g_calloutIds[CG_OUTBOUND_TRANSPORT_V4];
		pTcpCtx->transportLayerIdIn = FWPS_LAYER_INBOUND_TRANSPORT_V4;
		pTcpCtx->transportCalloutIdIn = g_calloutIds[CG_INBOUND_TRANSPORT_V4];

		pTcpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_PROTOCOL].value.uint8;
		pTcpCtx->ip_family = AF_INET;

		pAddr = (struct sockaddr_in*)pTcpCtx->localAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pTcpCtx->id);

		pAddr = (struct sockaddr_in*)pTcpCtx->remoteAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_PORT].value.uint16);

		dbgPrintAddress(AF_INET, pAddr, "remoteAddr", pTcpCtx->id);

		pTcpCtx->direction = NF_D_OUT;

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pTcpCtx->processName) - 2)
			{
				offset = len - (sizeof(pTcpCtx->processName) - 2);
				len = sizeof(pTcpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pTcpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}


	} else
	if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
	{
		struct sockaddr_in6	*	pAddr;
	
		pTcpCtx->layerId = FWPS_LAYER_STREAM_V6;
		pTcpCtx->sendCalloutId = g_calloutIds[CG_SEND_STREAM_CALLOUT_V6];
		pTcpCtx->recvCalloutId = g_calloutIds[CG_RECV_STREAM_CALLOUT_V6];
		pTcpCtx->recvProtCalloutId = g_calloutIds[CG_RECV_PROT_STREAM_CALLOUT_V6];

		pTcpCtx->transportLayerIdOut = FWPS_LAYER_OUTBOUND_TRANSPORT_V6;
		pTcpCtx->transportCalloutIdOut = g_calloutIds[CG_OUTBOUND_TRANSPORT_V6];
		pTcpCtx->transportLayerIdIn = FWPS_LAYER_INBOUND_TRANSPORT_V6;
		pTcpCtx->transportCalloutIdIn = g_calloutIds[CG_INBOUND_TRANSPORT_V6];

		pTcpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_PROTOCOL].value.uint8;
		pTcpCtx->ip_family = AF_INET6;

		pAddr = (struct sockaddr_in6*)pTcpCtx->localAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET6, pAddr, "localAddr", pTcpCtx->id);

		pAddr = (struct sockaddr_in6*)pTcpCtx->remoteAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_PORT].value.uint16);

		dbgPrintAddress(AF_INET6, pAddr, "remoteAddr", pTcpCtx->id);

        if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
		{
         	pAddr->sin6_scope_id = inMetaValues->remoteScopeId.Value;
   	    }		

		pTcpCtx->direction = NF_D_OUT;

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pTcpCtx->processName) - 2)
			{
				offset = len - (sizeof(pTcpCtx->processName) - 2);
				len = sizeof(pTcpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pTcpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

	}

	return pTcpCtx;
}

VOID callouts_flowEstablishedCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	PTCPCTX  pTcpCtx;

	UNREFERENCED_PARAMETER(packet);
	UNREFERENCED_PARAMETER(flowContext);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(classifyContext);

	if (!devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	for (;;) 
	{
		pTcpCtx = tcpctx_findByHandle(inMetaValues->transportEndpointHandle);
		if (pTcpCtx)
		{
			tcpctx_removeFromHandleTable(pTcpCtx);
			pTcpCtx->transportEndpointHandle = 0;

			pTcpCtx = callouts_createFlowContext(pTcpCtx, inFixedValues, inMetaValues);
			
			tcpctx_release(pTcpCtx);
		} else
		{
			pTcpCtx = callouts_createFlowContext(NULL, inFixedValues, inMetaValues);
			if (!pTcpCtx)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				break;
			}

			pTcpCtx->filteringFlag = rules_findByTcpCtx(pTcpCtx);
		}

		KdPrint((DPREFIX"callouts_flowEstablishedCallout[%I64u] flags = %d\n", pTcpCtx->id, pTcpCtx->filteringFlag));
		
		if (pTcpCtx->filteringFlag & NF_BLOCK)
		{
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
			pTcpCtx->abortConnection = TRUE;
//			status = STATUS_UNSUCCESSFUL;
//			break;
		} else
		if (!(pTcpCtx->filteringFlag & (NF_FILTER | NF_CONTROL_FLOW)))
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		status = devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CONNECTED, pTcpCtx, NULL);
		if (!NT_SUCCESS(status))
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		status = FwpsFlowAssociateContext(pTcpCtx->flowHandle,
                                     pTcpCtx->layerId,
									 pTcpCtx->sendCalloutId,
                                     (UINT64)pTcpCtx);
		if (!NT_SUCCESS(status))
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		tcpctx_addRef(pTcpCtx);

		status = FwpsFlowAssociateContext(pTcpCtx->flowHandle,
									pTcpCtx->layerId,
									pTcpCtx->recvCalloutId,
									(UINT64)pTcpCtx);
		if (!NT_SUCCESS(status))
		{
			tcpctx_release(pTcpCtx);
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		tcpctx_addRef(pTcpCtx);

		status = FwpsFlowAssociateContext(pTcpCtx->flowHandle,
									pTcpCtx->layerId,
									pTcpCtx->recvProtCalloutId,
									(UINT64)pTcpCtx);
		if (!NT_SUCCESS(status))
		{
			tcpctx_release(pTcpCtx);
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		tcpctx_addRef(pTcpCtx);

		status = FwpsFlowAssociateContext(pTcpCtx->flowHandle,
									pTcpCtx->transportLayerIdOut,
									pTcpCtx->transportCalloutIdOut,
									(UINT64)pTcpCtx);
		if (!NT_SUCCESS(status))
		{
			tcpctx_release(pTcpCtx);
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		tcpctx_addRef(pTcpCtx);

		status = FwpsFlowAssociateContext(pTcpCtx->flowHandle,
									pTcpCtx->transportLayerIdIn,
									pTcpCtx->transportCalloutIdIn,
									(UINT64)pTcpCtx);
		if (!NT_SUCCESS(status))
		{
			tcpctx_release(pTcpCtx);
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		classifyOut->actionType = FWP_ACTION_PERMIT;
	
		KdPrint((DPREFIX"callouts_flowEstablishedCallout[%I64u] success\n", pTcpCtx->id));

		break;
	} 

	if (!NT_SUCCESS(status))
	{
		if (pTcpCtx)
		{
			tcpctx_release(pTcpCtx);
		}
	}
}

VOID callouts_sendStreamCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	callouts_streamCallout(
				inFixedValues, 
				inMetaValues, 
				packet, 
				classifyContext, 
				filter, 
				flowContext, 
				classifyOut, 
				TRUE);
}

VOID callouts_recvStreamCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	callouts_streamCallout(
				inFixedValues, 
				inMetaValues, 
				packet, 
				classifyContext, 
				filter, 
				flowContext, 
				classifyOut, 
				FALSE);
}


VOID callouts_streamCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut,
   BOOLEAN bSend)
{
	FWPS_STREAM_CALLOUT_IO_PACKET* streamPacket;
	FWPS_STREAM_DATA* streamData;
	PTCPCTX pTcpCtx;
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN isSend;
	BOOLEAN bypassPacket = FALSE;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	streamPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*) packet;
	streamData = streamPacket->streamData;

	pTcpCtx = *(PTCPCTX*)(UINT64*) &flowContext;

	if (!devctrl_isProxyAttached() ||
		pTcpCtx->abortConnection ||
		devctrl_isShutdown())
	{
		KdPrint((DPREFIX"callouts_streamCallout [%I64u] drop connection\n", pTcpCtx->id));
        streamPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
        classifyOut->actionType = FWP_ACTION_NONE;
		pTcpCtx->abortConnection = TRUE;
		return;
	}

	isSend = (BOOLEAN) ((streamData->flags & FWPS_STREAM_FLAG_SEND) == FWPS_STREAM_FLAG_SEND);

	if (!(pTcpCtx->filteringFlag & NF_FILTER))
	{
		sl_lock(&pTcpCtx->lock, &lh);
		if (isSend && bSend)
		{
			pTcpCtx->outCounter += streamData->dataLength;
			pTcpCtx->outCounterTotal += streamData->dataLength;
		} else
		if (!isSend && !bSend)
		{
			pTcpCtx->inCounter += streamData->dataLength;
			pTcpCtx->inCounterTotal += streamData->dataLength;
		}
		sl_unlock(&lh);

		if (pTcpCtx->fcHandle != 0)
		{
			if (isSend && bSend)
			{
				flowctl_update(pTcpCtx->fcHandle, TRUE, streamData->dataLength);
				pTcpCtx->outLastTS = flowctl_getTickCount();
			} else
			if (!isSend && !bSend)
			{
				flowctl_update(pTcpCtx->fcHandle, FALSE, streamData->dataLength);
				pTcpCtx->inLastTS = flowctl_getTickCount();
			}
		}

		streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	sl_lock(&pTcpCtx->lock, &lh);

	if (bSend)
	{
		if (!pTcpCtx->sendCalloutInjectable)
		{
			pTcpCtx->sendCalloutInjectable = TRUE;
		}
		if ((pTcpCtx->sendCalloutBypass && isSend) || (!isSend && !pTcpCtx->recvCalloutBypass))
		{
			bypassPacket = TRUE;
		}
	} else
	{
		if (!pTcpCtx->recvCalloutInjectable)
		{
			pTcpCtx->recvCalloutInjectable = TRUE;
		}
		if ((pTcpCtx->recvCalloutBypass && !isSend) || (isSend && !pTcpCtx->sendCalloutBypass))
		{
			bypassPacket = TRUE;
		}
	}

	sl_unlock(&lh);

	if (bypassPacket || (pTcpCtx->filteringState == UMFS_DISABLED))
	{
		if (pTcpCtx->filteringState == UMFS_DISABLED)
		{
			KdPrint((DPREFIX"callouts_streamCallout [%I64u] filtering disabled, length=%d, flags=%x\n", 
				pTcpCtx->id, (UINT)streamData->dataLength, streamData->flags));
		} else 
		{
			KdPrint((DPREFIX"callouts_streamCallout [%I64u] wrong callout, length=%d, flags=%x\n", 
				pTcpCtx->id, (UINT)streamData->dataLength, streamData->flags));
		}

		if (g_blockUnexpectedRecvDisconnects &&
			(streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT) &&
			!pTcpCtx->recvDisconnectCalled)
		{
			KdPrint((DPREFIX"callouts_streamCallout [%I64u] unexpected disconnect, flags=%x\n", 
				pTcpCtx->id, streamData->flags));
			streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
			classifyOut->actionType = FWP_ACTION_BLOCK;
			return;
		}

		status = tcpctx_pushPacket(pTcpCtx, streamData, NF_TCP_REINJECT, bSend? pTcpCtx->sendCalloutId : pTcpCtx->recvCalloutId);
		if (status == STATUS_SUCCESS)
		{
			streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
			streamPacket->countBytesRequired = 0;
			streamPacket->countBytesEnforced = streamData->dataLength;
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
		} else
		{
			KdPrint((DPREFIX"callouts_streamCallout[%I64u] tcpctx_pushPacket failed, drop connection\n", pTcpCtx->id));

			streamPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
			classifyOut->actionType = FWP_ACTION_NONE;
		}

		return;
	}

	if (streamData->flags & (FWPS_STREAM_FLAG_SEND_EXPEDITED | 
				FWPS_STREAM_FLAG_RECEIVE_EXPEDITED | 
				FWPS_STREAM_FLAG_SEND_ABORT |
				FWPS_STREAM_FLAG_RECEIVE_ABORT))
	{
		streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		classifyOut->actionType = FWP_ACTION_PERMIT;
		
		if (streamData->flags & (FWPS_STREAM_FLAG_SEND_ABORT | FWPS_STREAM_FLAG_RECEIVE_ABORT))
		{
			sl_lock(&pTcpCtx->lock, &lh);
			pTcpCtx->abortConnection = TRUE;
			sl_unlock(&lh);
		}
		KdPrint((DPREFIX"callouts_streamCallout [%I64u] bypass packet\n", pTcpCtx->id));
		return;
	}

	KdPrint((DPREFIX"callouts_streamCallout[%I64u] stream data length=%d, flags=%x\n", 
		pTcpCtx->id, (UINT)streamData->dataLength, streamData->flags));

	sl_lock(&pTcpCtx->lock, &lh);

	if (!isSend &&
		streamData->dataLength > 0 &&
		(pTcpCtx->pendedRecvBytes > TCP_RECV_INJECT_LIMIT) ||
		(pTcpCtx->pendedRecvProtBytes > TCP_RECV_INJECT_LIMIT))
	{
		pTcpCtx->recvDeferred = TRUE;
	} else
	{
		pTcpCtx->recvDeferred = FALSE;
	}

	if (isSend)
	{
		if (streamData->dataLength == 0 &&
			pTcpCtx->sendDisconnectCalled)
		{
			sl_unlock(&lh);
			streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
			classifyOut->actionType = FWP_ACTION_BLOCK;
			KdPrint((DPREFIX"callouts_streamCallout [%I64u] repeated send disconnect\n", pTcpCtx->id));
			return;
		}

		pTcpCtx->pendedSendBytes += (ULONG)streamData->dataLength;
		KdPrint((DPREFIX"callouts_streamCallout[%I64u] pendedSendBytes=%u, packetLen=%u\n", 
			pTcpCtx->id, pTcpCtx->pendedSendBytes, (UINT)streamData->dataLength));
	} else
	{
		if (streamData->dataLength == 0 &&
			pTcpCtx->recvDisconnectCalled)
		{
			sl_unlock(&lh);
			streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
			classifyOut->actionType = FWP_ACTION_BLOCK;
			KdPrint((DPREFIX"callouts_streamCallout [%I64u] repeated recv disconnect\n", pTcpCtx->id));
			return;
		}

		pTcpCtx->pendedRecvBytes += (ULONG)streamData->dataLength;
		KdPrint((DPREFIX"callouts_streamCallout[%I64u] pendedRecvBytes=%u, packetLen=%u\n", 
			pTcpCtx->id, pTcpCtx->pendedRecvBytes, (UINT)streamData->dataLength));

		if (pTcpCtx->pendedRecvProtBytes >= streamData->dataLength)
		{
			pTcpCtx->pendedRecvProtBytes -= (ULONG)streamData->dataLength;
		} else
		{
			pTcpCtx->pendedRecvProtBytes = 0;
		}

		KdPrint((DPREFIX"callouts_streamCallout[%I64u] %d received bytes\n", pTcpCtx->id, pTcpCtx->pendedRecvProtBytes));

	}
	sl_unlock(&lh);

	status = tcpctx_pushPacket(pTcpCtx, streamData, isSend? NF_TCP_SEND : NF_TCP_RECEIVE, bSend? pTcpCtx->sendCalloutId : pTcpCtx->recvCalloutId);
	if (status == STATUS_SUCCESS)
	{
		streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		streamPacket->countBytesRequired = 0;
		streamPacket->countBytesEnforced = streamData->dataLength;
		classifyOut->actionType = FWP_ACTION_BLOCK;
	    classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

		sl_lock(&pTcpCtx->lock, &lh);
		if (pTcpCtx->filteringState == UMFS_DISABLED)
		{
			if (isSend)
			{
				pTcpCtx->pendedSendBytes -= (ULONG)streamData->dataLength;
			} else
			{
				pTcpCtx->pendedRecvBytes -= (ULONG)streamData->dataLength;
			}
		}
		sl_unlock(&lh);

		if (g_fastRecvDisconnectOnFinWithData &&
			!isSend && 
			(pTcpCtx->pendedRecvProtBytes == 0) && 
			pTcpCtx->finReceivedOnRecvProt)
		{
			FWPS_STREAM_DATA sd;
			memset(&sd, 0, sizeof(sd));
			sd.flags = FWPS_STREAM_FLAG_RECEIVE | FWPS_STREAM_FLAG_RECEIVE_DISCONNECT;

			tcpctx_pushPacket(pTcpCtx, &sd, NF_TCP_RECEIVE, pTcpCtx->recvCalloutId);

			KdPrint((DPREFIX"callouts_streamCallout[%I64u] all data received, push disconnect\n", pTcpCtx->id));
		}

	} else
	{
		KdPrint((DPREFIX"callouts_streamCallout[%I64u] tcpctx_pushPacket failed, drop connection\n", pTcpCtx->id));

		streamPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
        classifyOut->actionType = FWP_ACTION_NONE;

		sl_lock(&pTcpCtx->lock, &lh);
		if (isSend)
		{
			pTcpCtx->pendedSendBytes -= (ULONG)streamData->dataLength;
		} else
		{
			pTcpCtx->pendedRecvBytes -= (ULONG)streamData->dataLength;
		}
		sl_unlock(&lh);
	}
}


VOID callouts_streamCalloutRecvProt(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	FWPS_STREAM_CALLOUT_IO_PACKET* streamPacket;
	FWPS_STREAM_DATA* streamData;
	PTCPCTX pTcpCtx;

	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	if (!flowContext)
		return;

	streamPacket = (FWPS_STREAM_CALLOUT_IO_PACKET*) packet;
	streamData = streamPacket->streamData;

	pTcpCtx = *(PTCPCTX*)(UINT64*) &flowContext;

	if (!devctrl_isProxyAttached() ||
		pTcpCtx->abortConnection ||
		devctrl_isShutdown())
	{
		KdPrint((DPREFIX"callouts_streamCalloutRecvProt [%I64u] drop connection\n", pTcpCtx->id));
        streamPacket->streamAction = FWPS_STREAM_ACTION_DROP_CONNECTION;
        classifyOut->actionType = FWP_ACTION_NONE;
		pTcpCtx->abortConnection = TRUE;
		return;
	}

	if (!(pTcpCtx->filteringFlag & NF_FILTER))
	{
		streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	KdPrint((DPREFIX"callouts_streamCalloutRecvProt[%I64u] stream data length=%d, flags=%x, action=%x, rights=%x, classifyFlags=%x, withData=%d\n", 
		pTcpCtx->id, (UINT)streamData->dataLength, streamData->flags, classifyOut->actionType, classifyOut->rights, classifyOut->flags, pTcpCtx->finWithData));

	if (pTcpCtx->finWithData &&
		(streamData->flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT))
	{
		KLOCK_QUEUE_HANDLE lh;

		streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
		classifyOut->actionType = FWP_ACTION_BLOCK;

		sl_lock(&pTcpCtx->lock, &lh);

		pTcpCtx->finReceivedOnRecvProt = TRUE;

		if (g_fastRecvDisconnectOnFinWithData && pTcpCtx->pendedRecvProtBytes == 0)
		{
			FWPS_STREAM_DATA sd;
			memset(&sd, 0, sizeof(sd));
			sd.flags = FWPS_STREAM_FLAG_RECEIVE | FWPS_STREAM_FLAG_RECEIVE_DISCONNECT;

			sl_unlock(&lh);
			tcpctx_pushPacket(pTcpCtx, &sd, NF_TCP_RECEIVE, pTcpCtx->recvCalloutId);
			sl_lock(&pTcpCtx->lock, &lh);

			KdPrint((DPREFIX"callouts_streamCalloutRecvProt[%I64u] all data received, push disconnect\n", pTcpCtx->id));
		}

		sl_unlock(&lh);

		devctrl_pushTcpData(pTcpCtx->id, NF_TCP_DEFERRED_DISCONNECT, NULL, NULL);

		return;
	} 

	if (streamData->flags & FWPS_STREAM_FLAG_RECEIVE)
	{
		KLOCK_QUEUE_HANDLE lh;

		sl_lock(&pTcpCtx->lock, &lh);
		pTcpCtx->pendedRecvProtBytes += (ULONG)streamData->dataLength;
		sl_unlock(&lh);

		KdPrint((DPREFIX"callouts_streamCalloutRecvProt[%I64u] %d received bytes\n", pTcpCtx->id, pTcpCtx->pendedRecvProtBytes));
	}

	tcpctx_pushPacket(pTcpCtx, streamData, NF_TCP_REINJECT, pTcpCtx->recvProtCalloutId);

	streamPacket->streamAction = FWPS_STREAM_ACTION_NONE;
	streamPacket->countBytesRequired = 0;
	streamPacket->countBytesEnforced = 0;
	classifyOut->actionType = FWP_ACTION_BLOCK;
	classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
	classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

	return;
}


NTSTATUS callouts_flowEstablishedNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)

{
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to Flow Established layer.\n"));

       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from Flow Established layer.\n"));
       break;
    }

    return STATUS_SUCCESS;
}

VOID callouts_streamFlowDeletion(
   IN UINT16 layerId,
   IN UINT32 calloutId,
   IN UINT64 flowContext)
{
	TCPCTX *	pTcpCtx;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);

	pTcpCtx = *((TCPCTX**)&flowContext);

	sl_lock(&pTcpCtx->lock, &lh);
	pTcpCtx->closed = TRUE;
	sl_unlock(&lh);

	devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CLOSED, pTcpCtx, NULL);

    KdPrint((DPREFIX"callouts_streamFlowDeletion[%I64u] closed\n", pTcpCtx->id));

	tcpctx_release(pTcpCtx);
}

VOID callouts_recvStreamFlowDeletion(
   IN UINT16 layerId,
   IN UINT32 calloutId,
   IN UINT64 flowContext)
{
	TCPCTX *	pTcpCtx;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);

	pTcpCtx = *((TCPCTX**)&flowContext);

	sl_lock(&pTcpCtx->lock, &lh);
	pTcpCtx->closed = TRUE;
	sl_unlock(&lh);

    KdPrint((DPREFIX"callouts_streamFlowDeletion[%I64u] closed (recv)\n", pTcpCtx->id));

	tcpctx_release(pTcpCtx);
}

NTSTATUS callouts_streamNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to Stream layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from Stream layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}


PUDPCTX
callouts_createUdpFlowContext(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues)
{
	PUDPCTX	pUdpCtx;

	KdPrint((DPREFIX"callouts_createUdpFlowContext\n"));

	pUdpCtx = udpctx_alloc(inMetaValues->transportEndpointHandle);
	if (!pUdpCtx)
	{
		KdPrint((DPREFIX"callouts_createUdpFlowContext: udpctx_alloc failed\n"));
		return NULL;
	}

	pUdpCtx->closed = FALSE;
	pUdpCtx->processId = (ULONG)inMetaValues->processId;

	if (inFixedValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V4)
	{
		struct sockaddr_in	*	pAddr;
	
		pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V4;
		pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V4];
		pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_PROTOCOL].value.uint8;
		pUdpCtx->ip_family = AF_INET;

		pAddr = (struct sockaddr_in*)pUdpCtx->localAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_IP_LOCAL_PORT].value.uint16);

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pUdpCtx->processName) - 2)
			{
				offset = len - (sizeof(pUdpCtx->processName) - 2);
				len = sizeof(pUdpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pUdpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V4_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pUdpCtx->id);

	} else
	if (inFixedValues->layerId == FWPS_LAYER_ALE_FLOW_ESTABLISHED_V6)
	{
		struct sockaddr_in6	*	pAddr;
	
		pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V6;
		pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V6];
		pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_PROTOCOL].value.uint8;
		pUdpCtx->ip_family = AF_INET6;

		pAddr = (struct sockaddr_in6*)pUdpCtx->localAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_IP_LOCAL_PORT].value.uint16);

		if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
		{
       	 	pAddr->sin6_scope_id = inMetaValues->remoteScopeId.Value;
    	}		

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pUdpCtx->processName) - 2)
			{
				offset = len - (sizeof(pUdpCtx->processName) - 2);
				len = sizeof(pUdpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pUdpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_FLOW_ESTABLISHED_V6_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pUdpCtx->id);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4)
	{
		struct sockaddr_in	*	pAddr;
	
		pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V4;
		pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V4];
		pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL].value.uint8;
		pUdpCtx->ip_family = AF_INET;

		pAddr = (struct sockaddr_in*)pUdpCtx->localAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pUdpCtx->id);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6)
	{
		struct sockaddr_in6	*	pAddr;
	
		pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V6;
		pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V6];
		pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_PROTOCOL].value.uint8;
		pUdpCtx->ip_family = AF_INET6;

		pAddr = (struct sockaddr_in6*)pUdpCtx->localAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_PORT].value.uint16);

		if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
		{
       	 	pAddr->sin6_scope_id = inMetaValues->remoteScopeId.Value;
    	}		

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pUdpCtx->id);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4)
	{
		struct sockaddr_in	*	pAddr;
	
		pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V4;
		pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V4];
		pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_PROTOCOL].value.uint8;
		pUdpCtx->ip_family = AF_INET;

		pAddr = (struct sockaddr_in*)pUdpCtx->localAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET, pAddr, "localAddr", pUdpCtx->id);

		pAddr = (struct sockaddr_in*)pUdpCtx->remoteAddr;
		pAddr->sin_family = AF_INET;
		pAddr->sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_ADDRESS].value.uint32);
		pAddr->sin_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_IP_REMOTE_PORT].value.uint16);

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pUdpCtx->processName) - 2)
			{
				offset = len - (sizeof(pUdpCtx->processName) - 2);
				len = sizeof(pUdpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pUdpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

		dbgPrintAddress(AF_INET, pAddr, "remoteAddr", pUdpCtx->id);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V6)
	{
		struct sockaddr_in6	*	pAddr;
	
		pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V6;
		pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V6];
		pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_PROTOCOL].value.uint8;
		pUdpCtx->ip_family = AF_INET6;

		pAddr = (struct sockaddr_in6*)pUdpCtx->localAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_LOCAL_PORT].value.uint16);

		dbgPrintAddress(AF_INET6, pAddr, "localAddr", pUdpCtx->id);

		pAddr = (struct sockaddr_in6*)pUdpCtx->remoteAddr;
		pAddr->sin6_family = AF_INET6;
		memcpy(&pAddr->sin6_addr, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
				NF_MAX_IP_ADDRESS_LENGTH);
		pAddr->sin6_port = 
			htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_IP_REMOTE_PORT].value.uint16);

		if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
		{
       	 	pAddr->sin6_scope_id = inMetaValues->remoteScopeId.Value;
    	}		

		if (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob)
		{
			int offset, len;
	
			len = inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob->size;
			if (len > sizeof(pUdpCtx->processName) - 2)
			{
				offset = len - (sizeof(pUdpCtx->processName) - 2);
				len = sizeof(pUdpCtx->processName) - 2;
			} else
			{
				offset = 0;
			}
			memcpy(pUdpCtx->processName, 
				inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V6_ALE_APP_ID].value.byteBlob->data + offset,
				len);
		}

		dbgPrintAddress(AF_INET6, pAddr, "remoteAddr", pUdpCtx->id);
	}

	return pUdpCtx;
}

VOID callouts_udpFlowEstablishedCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	PUDPCTX  pUdpCtx;

	UNREFERENCED_PARAMETER(packet);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	if (devctrl_isShutdown() || !devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	for (;;) 
	{
		if (inMetaValues->transportEndpointHandle)
		{
			KdPrint((DPREFIX"callouts_udpFlowEstablishedCallout transportEndpointHandle=%I64u\n", inMetaValues->transportEndpointHandle));

			pUdpCtx = udpctx_findByHandle(inMetaValues->transportEndpointHandle);
			if (pUdpCtx)
			{
				KdPrint((DPREFIX"callouts_udpFlowEstablishedCallout[%I64u] found connected socket\n", pUdpCtx->id));
				udpctx_release(pUdpCtx);
			} else
			{
				pUdpCtx = callouts_createUdpFlowContext(inFixedValues, inMetaValues);
				if (!pUdpCtx)
				{
					classifyOut->actionType = FWP_ACTION_PERMIT;
					break;
				}

				if ((ULONG)inMetaValues->processId != devctrl_getProxyPid())
				{
					devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CREATED, pUdpCtx, NULL);
				}

				udpctx_release(pUdpCtx);
			}

			KdPrint((DPREFIX"callouts_udpFlowEstablishedCallout[%I64u]\n", pUdpCtx->id));
		}

		classifyOut->actionType = FWP_ACTION_PERMIT;
	
		break;
	} 

}

NTSTATUS callouts_udpFlowEstablishedNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)

{
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to UDP Flow Established layer.\n"));

       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from UDP Flow Established layer.\n"));
       break;
    }

    return STATUS_SUCCESS;
}

static BOOLEAN callouts_copyBuffer(const FWPS_INCOMING_METADATA_VALUES* inMetaValues, NET_BUFFER* netBuffer, BOOLEAN isSend, PNF_UDP_PACKET pPacket, ULONG dataLength)
{
	void * buf = NULL;
	BOOLEAN result = TRUE;

	if (!isSend)
	{
		NdisRetreatNetBufferDataStart(
				netBuffer,
				inMetaValues->transportHeaderSize,
				0,
				NULL
				);
	}

	buf = NdisGetDataBuffer(
			netBuffer,
			dataLength,
			pPacket->dataBuffer,
			1,
			0);
			
	if (buf != NULL)
	{
		if (buf != (pPacket->dataBuffer))
		{
			memcpy(pPacket->dataBuffer, buf, dataLength);
		}
	} else
	{
		result = FALSE;
	}

	if (!isSend)
	{
		NdisAdvanceNetBufferDataStart(
				netBuffer,
				inMetaValues->transportHeaderSize,
				FALSE,
				NULL
				);
	}

	return result;
}

BOOLEAN callouts_pushUdpPacket(
	const FWPS_INCOMING_VALUES* inFixedValues,
	const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	PUDPCTX	pUdpCtx,
	NET_BUFFER* netBuffer,
	BOOLEAN isSend)
{
	PNF_UDP_PACKET pPacket = NULL;
	ULONG dataLength;
	KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result = FALSE;

	dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
	if (!isSend)
	{
		dataLength += inMetaValues->transportHeaderSize;
	}

	sl_lock(&pUdpCtx->lock, &lh);

	if (pUdpCtx->pendedRecvBytes > UDP_PEND_LIMIT ||
		pUdpCtx->pendedSendBytes > UDP_PEND_LIMIT)
	{
		sl_unlock(&lh);
		KdPrint((DPREFIX"callouts_udpCallout[%I64u] too large queue\n", pUdpCtx->id));
		return FALSE;
	}

	if (isSend)
	{
		pUdpCtx->pendedSendBytes += dataLength;
		KdPrint((DPREFIX"callouts_udpCallout[%I64u] pendedSendBytes=%u, packetsLen=%u\n", 
			pUdpCtx->id, pUdpCtx->pendedSendBytes, dataLength));
	} else
	{
		pUdpCtx->pendedRecvBytes += dataLength;
		KdPrint((DPREFIX"callouts_udpCallout[%I64u] pendedRecvBytes=%u, packetsLen=%u\n", 
			pUdpCtx->id, pUdpCtx->pendedRecvBytes, dataLength));
	}
	
	sl_unlock(&lh);

	for (;;)
	{
		pPacket = udpctx_allocPacket(dataLength);
		if (!pPacket)
		{
			KdPrint((DPREFIX"callouts_udpCallout[%I64u] unable to allocate packet\n", pUdpCtx->id));
			break;
		}
		
		pPacket->dataLength = dataLength;
		pPacket->options.compartmentId = (COMPARTMENT_ID)inMetaValues->compartmentId;
		pPacket->options.transportHeaderLength = inMetaValues->transportHeaderSize;
		pPacket->options.endpointHandle = inMetaValues->transportEndpointHandle;

		if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))		
			pPacket->options.remoteScopeId = inMetaValues->remoteScopeId;

		memcpy(&pPacket->options.localAddr, &pUdpCtx->localAddr, NF_MAX_ADDRESS_LENGTH);
		
		if (isSend)
		{
			if (FWPS_IS_METADATA_FIELD_PRESENT(
				inMetaValues, 
				FWPS_METADATA_FIELD_TRANSPORT_CONTROL_DATA))
			{
				 ASSERT(inMetaValues->controlDataLength > 0);

	#pragma warning(push)
	#pragma warning(disable: 28197) 
				 pPacket->controlData = (WSACMSGHDR*)
					 ExAllocatePoolWithTag(NonPagedPool, inMetaValues->controlDataLength, MEM_TAG_UDP_DATA);
	#pragma warning(pop)
				 
				 if (pPacket->controlData != NULL)
				 {
					memcpy(pPacket->controlData,
						inMetaValues->controlData,
						inMetaValues->controlDataLength);

					pPacket->options.controlDataLength = inMetaValues->controlDataLength;
				}
			}
		}

		if (!callouts_copyBuffer(inMetaValues, netBuffer, isSend, pPacket, dataLength))
		{
			break;
		}

		if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4)
		{
			struct sockaddr_in	*	pAddr;

			pAddr = (struct sockaddr_in*)pPacket->remoteAddr;
			pAddr->sin_family = AF_INET;
			pAddr->sin_addr.S_un.S_addr = 
				htonl(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_ADDRESS].value.uint32);
			pAddr->sin_port = 
				htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_PORT].value.uint16);

			pPacket->options.interfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_INTERFACE_INDEX].value.uint32;
			pPacket->options.subInterfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_SUB_INTERFACE_INDEX].value.uint32;
		} else
		{
			struct sockaddr_in6	*	pAddr;
		
			pAddr = (struct sockaddr_in6*)pPacket->remoteAddr;
			pAddr->sin6_family = AF_INET6;
			memcpy(&pAddr->sin6_addr, 
					inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);
			pAddr->sin6_port = 
				htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_PORT].value.uint16);

			pPacket->options.interfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_INTERFACE_INDEX].value.uint32;
			pPacket->options.subInterfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_SUB_INTERFACE_INDEX].value.uint32;
		}

		if (NT_SUCCESS(devctrl_pushUdpData(pUdpCtx->id, isSend? NF_UDP_SEND : NF_UDP_RECEIVE, pUdpCtx, pPacket)))
		{
			result = TRUE;
		}

		break;
	} 

	if (!result)
	{
		sl_lock(&pUdpCtx->lock, &lh);
		if (isSend)
		{
			pUdpCtx->pendedSendBytes -= dataLength;
		} else
		{
			pUdpCtx->pendedRecvBytes -= dataLength;
		}
		sl_unlock(&lh);
	}

	return result;
}

VOID callouts_udpCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	PUDPCTX	pUdpCtx = NULL;
	FWPS_PACKET_INJECTION_STATE packetState;
	BOOLEAN isSend;
    NET_BUFFER* netBuffer = NULL;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
	{
		KdPrint((DPREFIX"callouts_udpCallout no FWPS_RIGHT_ACTION_WRITE\n"));
		return;
	}

	if (devctrl_isShutdown() || !devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	packetState = FwpsQueryPacketInjectionState0(
						devctrl_getUdpNwV4InjectionHandle(),
						(NET_BUFFER_LIST*)packet,
						NULL
						);
	if (packetState != FWPS_PACKET_NOT_INJECTED)
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		KdPrint((DPREFIX"callouts_udpCallout injected by self or others on NwV4\n"));
		return;
	}

	packetState = FwpsQueryPacketInjectionState0(
						devctrl_getUdpNwV6InjectionHandle(),
						(NET_BUFFER_LIST*)packet,
						NULL
						);
	if (packetState != FWPS_PACKET_NOT_INJECTED)
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		KdPrint((DPREFIX"callouts_udpCallout injected by self or others on NwV6\n"));
		return;
	}
	
	if (!flowContext)
	{
		pUdpCtx = udpctx_findByHandle(inMetaValues->transportEndpointHandle);
		if (pUdpCtx)
		{
			KdPrint((DPREFIX"callouts_udpCallout[%I64u] found connected socket\n", pUdpCtx->id));
			if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4)
			{
				struct sockaddr_in	*	pAddr;

				pAddr = (struct sockaddr_in*)pUdpCtx->localAddr;

				if (pAddr->sin_addr.S_un.S_addr == 0)
				{
					pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V4;
					pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V4];
					pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_PROTOCOL].value.uint8;
					pUdpCtx->ip_family = AF_INET;

					pAddr->sin_family = AF_INET;
					pAddr->sin_addr.S_un.S_addr = 
						htonl(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS].value.uint32);
					pAddr->sin_port = 
						htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_PORT].value.uint16);
				}
			} else
			if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V6)
			{
				struct sockaddr_in6	*	pAddr;
				char zero[16] = {0};

				pAddr = (struct sockaddr_in6*)pUdpCtx->localAddr;

				if (memcmp(&pAddr->sin6_addr, zero, 16) == 0)
				{
					pUdpCtx->layerId = FWPS_LAYER_DATAGRAM_DATA_V6;
					pUdpCtx->calloutId = g_calloutIds[CG_DATAGRAM_DATA_CALLOUT_V6];
					pUdpCtx->ipProto = inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_PROTOCOL].value.uint8;
					pUdpCtx->ip_family = AF_INET6;

					pAddr->sin6_family = AF_INET6;
					memcpy(&pAddr->sin6_addr, 
							inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
							NF_MAX_IP_ADDRESS_LENGTH);
					pAddr->sin6_port = 
						htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_PORT].value.uint16);

					if (FWPS_IS_METADATA_FIELD_PRESENT(inMetaValues, FWPS_METADATA_FIELD_REMOTE_SCOPE_ID))
					{
       	 				pAddr->sin6_scope_id = inMetaValues->remoteScopeId.Value;
    				}		
				}
			}
		} else
		{
			pUdpCtx = callouts_createUdpFlowContext(inFixedValues, inMetaValues);
			if (!pUdpCtx)
			{
				KdPrint((DPREFIX"callouts_udpCallout unable to create context\n"));
				classifyOut->actionType = FWP_ACTION_PERMIT;
				return;
			}

			KdPrint((DPREFIX"callouts_udpCallout[%I64u] created new context\n", pUdpCtx->id));

			if ((ULONG)inMetaValues->processId != devctrl_getProxyPid())
			{
				devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CREATED, pUdpCtx, NULL);
			}
		}

	} else
	{
		pUdpCtx = *(PUDPCTX*)(UINT64*) &flowContext;

		// Add reference to keep the context in function scope
		udpctx_addRef(pUdpCtx);
	}

	for (;;)
	{
		struct sockaddr_in addrV4;
		struct sockaddr_in6	addrV6;
		char * pRemoteAddr = NULL;

		if (!devctrl_isProxyAttached() ||
			pUdpCtx->closed ||
			devctrl_isShutdown())
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		if (pUdpCtx->processId == devctrl_getProxyPid())
		{
			KdPrint((DPREFIX"callouts_udpCallout [%I64u] bypass own socket\n", pUdpCtx->id));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		packetState = FwpsQueryPacketInjectionState0(
						 devctrl_getUdpInjectionHandle(),
						 (NET_BUFFER_LIST*)packet,
						 NULL
						 );

		if (pUdpCtx->seenPackets)
		{
			if (packetState != FWPS_PACKET_NOT_INJECTED)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_udpCallout[%I64u] injected by self or others\n", pUdpCtx->id));
				break;
			}
		} else
		{
			pUdpCtx->seenPackets = TRUE;
		}

		if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4)
		{
			isSend = (FWP_DIRECTION_OUTBOUND == 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_DIRECTION].value.uint32);
		} else
		{
			isSend = (FWP_DIRECTION_OUTBOUND == 
				inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_DIRECTION].value.uint32);
		}

		if (pUdpCtx->filteringDisabled)
		{
			if (pUdpCtx->fcHandle != 0)
			{
				if (flowctl_mustSuspend(pUdpCtx->fcHandle, isSend, pUdpCtx->inLastTS))
				{
					KdPrint((DPREFIX"callouts_udpCallout[%I64u] datagram blocked by flow limit\n", pUdpCtx->id));

					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
					classifyOut->rights ^= FWPS_RIGHT_ACTION_WRITE;
					break;
				}

				if (isSend)
				{
					flowctl_update(pUdpCtx->fcHandle, TRUE, NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet)));
					pUdpCtx->outLastTS = flowctl_getTickCount();
				} else
				{
					flowctl_update(pUdpCtx->fcHandle, FALSE, NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet)));
					pUdpCtx->inLastTS = flowctl_getTickCount();
				}
			}

			sl_lock(&pUdpCtx->lock, &lh);
			if (isSend)
			{
				pUdpCtx->outCounter += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
				pUdpCtx->outCounterTotal += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
			} else
			{
				pUdpCtx->inCounter += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
				pUdpCtx->inCounterTotal += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
			}
			sl_unlock(&lh);

			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		if (inFixedValues->layerId == FWPS_LAYER_DATAGRAM_DATA_V4)
		{
			memset(&addrV4, 0, sizeof(addrV4));
			addrV4.sin_family = AF_INET;
			addrV4.sin_addr.S_un.S_addr = 
				htonl(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_ADDRESS].value.uint32);
			addrV4.sin_port = 
				htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_PORT].value.uint16);

			{
				struct sockaddr_in * pAddr = (struct sockaddr_in *)&addrV4;

				KdPrint((DPREFIX"callouts_udpCallout[%I64u] remoteAddr=%x:%d\n", 
					pUdpCtx->id, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));

				pAddr = (struct sockaddr_in *)pUdpCtx->localAddr;

				KdPrint((DPREFIX"callouts_udpCallout[%I64u] localAddr=%x:%d\n", 
					pUdpCtx->id, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));

//				if (pAddr->sin_addr.s_addr == 0)
				{
					pAddr->sin_addr.s_addr = htonl(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_ADDRESS].value.uint32);
					pAddr->sin_port = 
							htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_LOCAL_PORT].value.uint16);

					KdPrint((DPREFIX"callouts_udpCallout[%I64u] updated localAddr=%x:%d\n", 
						pUdpCtx->id, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));
				}
			}

			pRemoteAddr = (char*)&addrV4;
		} else
		{
			memset(&addrV6, 0, sizeof(addrV6));
			addrV6.sin6_family = AF_INET6;
			memcpy(&addrV6.sin6_addr, 
					inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);
			addrV6.sin6_port = 
				htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_PORT].value.uint16);

			{
				struct sockaddr_in6 * pAddr = (struct sockaddr_in6 *)&addrV6;

				KdPrint((DPREFIX"callouts_udpCallout[%I64u] remoteAddr=[%x:%x:%x:%x:%x:%x:%x:%x]:%d\n", 
					pUdpCtx->id, 
					pAddr->sin6_addr.u.Word[0], 
					pAddr->sin6_addr.u.Word[1], 
					pAddr->sin6_addr.u.Word[2], 
					pAddr->sin6_addr.u.Word[3], 
					pAddr->sin6_addr.u.Word[4], 
					pAddr->sin6_addr.u.Word[5], 
					pAddr->sin6_addr.u.Word[6], 
					pAddr->sin6_addr.u.Word[7], 
					htons(pAddr->sin6_port)));

				pAddr = (struct sockaddr_in6 *)pUdpCtx->localAddr;

				{
					memcpy(&pAddr->sin6_addr, 
							inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
							NF_MAX_IP_ADDRESS_LENGTH);
					pAddr->sin6_port = 
							htons(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_LOCAL_PORT].value.uint16);
				}

				KdPrint((DPREFIX"callouts_udpCallout[%I64u] localAddr=[%x:%x:%x:%x:%x:%x:%x:%x]:%d\n", 
					pUdpCtx->id, 
					pAddr->sin6_addr.u.Word[0], 
					pAddr->sin6_addr.u.Word[1], 
					pAddr->sin6_addr.u.Word[2], 
					pAddr->sin6_addr.u.Word[3], 
					pAddr->sin6_addr.u.Word[4], 
					pAddr->sin6_addr.u.Word[5], 
					pAddr->sin6_addr.u.Word[6], 
					pAddr->sin6_addr.u.Word[7], 
					htons(pAddr->sin6_port)));
			}

			pRemoteAddr = (char*)&addrV6;
		}

		flag = rules_findByUdpInfo(pUdpCtx, pRemoteAddr, isSend? NF_D_OUT : NF_D_IN);

		if (flag & NF_BLOCK)
		{
			KdPrint((DPREFIX"callouts_udpCallout[%I64u] datagram denied by rules\n", pUdpCtx->id));
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->rights ^= FWPS_RIGHT_ACTION_WRITE;
			break;
		}

		if (pUdpCtx->fcHandle)
		{
			if (flowctl_mustSuspend(pUdpCtx->fcHandle, isSend, pUdpCtx->inLastTS))
			{
				KdPrint((DPREFIX"callouts_udpCallout[%I64u] datagram blocked by flow limit\n", pUdpCtx->id));

				classifyOut->actionType = FWP_ACTION_BLOCK;
				classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
				classifyOut->rights ^= FWPS_RIGHT_ACTION_WRITE;
				break;
			}
		}

		if (!(flag & NF_FILTER))
		{
			sl_lock(&pUdpCtx->lock, &lh);
			if (isSend)
			{
				pUdpCtx->outCounter += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
				pUdpCtx->outCounterTotal += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
			} else
			{
				pUdpCtx->inCounter += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
				pUdpCtx->inCounterTotal += NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));
			}
			sl_unlock(&lh);

			if (pUdpCtx->fcHandle != 0)
			{
				if (isSend)
				{
					flowctl_update(pUdpCtx->fcHandle, TRUE, NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet)));
					pUdpCtx->outLastTS = flowctl_getTickCount();
				} else
				{
					flowctl_update(pUdpCtx->fcHandle, FALSE, NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet)));
					pUdpCtx->inLastTS = flowctl_getTickCount();
				}
			}

			KdPrint((DPREFIX"callouts_udpCallout[%I64u] datagram allowed by rules\n", pUdpCtx->id));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		} 

		for (netBuffer = NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet);
		   netBuffer != NULL;
		   netBuffer = NET_BUFFER_NEXT_NB(netBuffer))
		{
			if (!callouts_pushUdpPacket(inFixedValues, inMetaValues, pUdpCtx, netBuffer, isSend))
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				break;
			}
		}

		if (netBuffer == NULL)
		{
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
			classifyOut->rights ^= FWPS_RIGHT_ACTION_WRITE;
		}

		break;
	} 

	if (pUdpCtx)
	{
		udpctx_release(pUdpCtx);
	}
}

NTSTATUS callouts_udpNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to UDP layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from UDP layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}


VOID callouts_tcpOutboundPacketCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	PTCPCTX		pTcpCtx;
	PTCP_HEADER tcpHeader;
	ULONG		totalLen;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(inMetaValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	pTcpCtx = *(PTCPCTX*)(UINT64*) &flowContext;
	if (pTcpCtx)
	{
		BOOLEAN suspendIn, suspendOut;

		suspendIn = FALSE;
		suspendOut = FALSE;

		sl_lock(&pTcpCtx->lock, &lh);
		suspendIn = pTcpCtx->recvDeferred;
		if (!suspendIn)
		{
			suspendIn = (pTcpCtx->filteringFlag & NF_SUSPENDED)? TRUE : FALSE;
		}
		sl_unlock(&lh);

		// Block the unwanted RST, which is sent by TCP stack instead of FIN for outgoing connections, 
		// when there were no outgoing data packets
		if (g_blockRST)
		{
			tcpHeader = (PTCP_HEADER)NdisGetDataBuffer(
								NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet),
								sizeof(TCP_HEADER),
								NULL,
								sizeof(UINT16),
								0
								);

			if (tcpHeader)
			{
				KdPrint((DPREFIX"Flags %d for send [%I64u]\n", tcpHeader->th_flags, pTcpCtx->id));

				if ((tcpHeader->th_flags & TH_RST) && (pTcpCtx->finReceived))
				{
					KdPrint((DPREFIX"Blocked RST for id=%I64u\n", pTcpCtx->id));
					tcpHeader->th_flags = TH_FIN | TH_ACK;
					classifyOut->actionType = FWP_ACTION_PERMIT;
					return;
				} 
			} 
		}

		if (pTcpCtx->fcHandle)
		{
			if (!suspendIn)
			{
				suspendIn = flowctl_mustSuspend(pTcpCtx->fcHandle, FALSE, pTcpCtx->inLastTS);
			}

			suspendOut = flowctl_mustSuspend(pTcpCtx->fcHandle, TRUE, pTcpCtx->outLastTS);
		}

		if (suspendIn || suspendOut)
		{
			totalLen = NET_BUFFER_DATA_LENGTH(NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet));

			KdPrint((DPREFIX"Sent %u bytes for [%I64u]\n", totalLen, pTcpCtx->id));

			tcpHeader = (PTCP_HEADER)NdisGetDataBuffer(
								NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet),
								sizeof(TCP_HEADER),
								NULL,
								sizeof(UINT16),
								0
								);

			if (tcpHeader)
			{
				KdPrint((DPREFIX"Flags %d for send [%I64u]\n", tcpHeader->th_flags, pTcpCtx->id));

				if (totalLen == inMetaValues->transportHeaderSize)
				{
					if (suspendIn && (tcpHeader->th_flags == TH_ACK))
					{
						KdPrint((DPREFIX"Blocked ACK for id=%I64u\n", pTcpCtx->id));
						classifyOut->actionType = FWP_ACTION_BLOCK;
						classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
						classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
						return;
					} 
				} else
				{
					if (suspendOut && (tcpHeader->th_flags & TH_ACK))
					{
						KdPrint((DPREFIX"Blocked data for id=%I64u\n", pTcpCtx->id));
						classifyOut->actionType = FWP_ACTION_BLOCK;
						classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
						classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
						return;
					} 
				}
			} 
		}
	}

	classifyOut->actionType = FWP_ACTION_PERMIT;
}

VOID callouts_tcpInboundPacketCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	PTCPCTX		pTcpCtx;
	ULONG		totalLen;
	PTCP_HEADER tcpHeader;
    NET_BUFFER* netBuffer;
	ULONG		offset = 0;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(inFixedValues);
	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);

	pTcpCtx = *(PTCPCTX*)(UINT64*) &flowContext;
	if (pTcpCtx)
	{
		BOOLEAN disconnected;

		netBuffer = NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet);
		totalLen = NET_BUFFER_DATA_LENGTH(netBuffer);

		KdPrint((DPREFIX"Received %u bytes for [%I64u]\n", totalLen, pTcpCtx->id));

		if (g_blockRST)
		{
			offset = inMetaValues->transportHeaderSize;

			NdisRetreatNetBufferDataStart(
					netBuffer,
					offset,
					0,
					NULL
				  );

			tcpHeader = (PTCP_HEADER)NdisGetDataBuffer(
								netBuffer,
								sizeof(TCP_HEADER),
								NULL,
								sizeof(UINT16),
								0
								);

			NdisAdvanceNetBufferDataStart(
					netBuffer,
					offset,
					FALSE,
					NULL
				  );

			if (!tcpHeader)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				return;
			}

			KdPrint((DPREFIX"Flags %d for receive [%I64u]\n", tcpHeader->th_flags, pTcpCtx->id));

			if (tcpHeader->th_flags & TH_FIN)
			{
				KdPrint((DPREFIX"Inbound FIN for id=%I64u\n", pTcpCtx->id));
				pTcpCtx->finReceived = TRUE;

				if (pTcpCtx->finWithData)
				{
/*					FWPS_STREAM_DATA sd;
					memset(&sd, 0, sizeof(sd));
					sd.flags = FWPS_STREAM_FLAG_RECEIVE | FWPS_STREAM_FLAG_RECEIVE_DISCONNECT;

					tcpctx_pushPacket(pTcpCtx, &sd, NF_TCP_REINJECT, pTcpCtx->recvProtCalloutId);

					devctrl_pushTcpData(pTcpCtx->id, NF_TCP_DEFERRED_DISCONNECT, NULL, NULL);
*/
				} else
				{
					if (totalLen > 0)
					{
						KdPrint((DPREFIX"Inbound FIN with data for id=%I64u\n", pTcpCtx->id));
						pTcpCtx->finWithData = TRUE;
					}
				}
			} 

			if (tcpHeader->th_flags & TH_RST)
			{
				KdPrint((DPREFIX"Inbound RST for id=%I64u\n", pTcpCtx->id));
				if (pTcpCtx->finReceived)
				{
					classifyOut->actionType = FWP_ACTION_BLOCK;
					classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
					classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
					return;
				}
			} 
		}

		sl_lock(&pTcpCtx->lock, &lh);
		disconnected = pTcpCtx->recvDisconnectCalled;
		sl_unlock(&lh);

		if (!disconnected)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			return;
		}

		if (totalLen > 0)
		{
			KdPrint((DPREFIX"Blocked unexpected %u bytes for [%I64u]\n", totalLen, pTcpCtx->id));

			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
			return;
		}
	}

	classifyOut->actionType = FWP_ACTION_PERMIT;
}


NTSTATUS callouts_tcpPacketNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to TCP packet layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from TCP packet layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}

VOID callouts_tcpPacketFlowDeletion(
   IN UINT16 layerId,
   IN UINT32 calloutId,
   IN UINT64 flowContext)
{
	TCPCTX *	pTcpCtx;

	UNREFERENCED_PARAMETER(layerId);
	UNREFERENCED_PARAMETER(calloutId);

	pTcpCtx = *((TCPCTX**)&flowContext);

    KdPrint((DPREFIX"callouts_tcpPacketFlowDeletion[%I64u] closed\n", pTcpCtx->id));

	tcpctx_release(pTcpCtx);
}

typedef 
NTSTATUS
(NTAPI *t_FwpsPendClassify0)(
   UINT64 classifyHandle,
   UINT64 filterId,
   UINT32 flags,
   FWPS_CLASSIFY_OUT0* classifyOut
   );

static t_FwpsPendClassify0 _FwpsPendClassify0 = FwpsPendClassify0;

VOID callouts_connectRedirectCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER * filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	PTCPCTX  pTcpCtx;

	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	if (g_bypassConnectRedirectWithoutActionWrite)
	if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
	{
		KdPrint((DPREFIX"callouts_connectRedirectCallout no FWPS_RIGHT_ACTION_WRITE\n"));
		return;
	}

	if (!devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	if (!packet || 
		!classifyContext ||
        (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE))
	{
		KdPrint((DPREFIX"callouts_connectRedirectCallout bypass\n"));
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}


	for (;;) 
	{
		pTcpCtx = callouts_createFlowContext(NULL, inFixedValues, inMetaValues);
		if (!pTcpCtx)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		pTcpCtx->filteringFlag = rules_findByTcpCtx(pTcpCtx);
			
		KdPrint((DPREFIX"callouts_connectRedirectCallout[%I64u] flags = %d\n", pTcpCtx->id, pTcpCtx->filteringFlag));

		if (!(pTcpCtx->filteringFlag & NF_DISABLE_REDIRECT_PROTECTION))
		{
			if (tcpctx_isProxy((ULONG)inMetaValues->processId))
			{
				KdPrint((DPREFIX"callouts_connectRedirectCallout bypass local proxy connection (pid=%d)\n", (ULONG)inMetaValues->processId));
				classifyOut->actionType = FWP_ACTION_PERMIT;
				status = STATUS_UNSUCCESSFUL;
				break;
			}
		}

		if (pTcpCtx->filteringFlag & NF_BLOCK)
		{
			classifyOut->actionType = FWP_ACTION_BLOCK;
			status = STATUS_UNSUCCESSFUL;
			KdPrint((DPREFIX"callouts_connectRedirectCallout block\n"));
			break;
		}

		if (pTcpCtx->filteringFlag & NF_INDICATE_CONNECT_REQUESTS)
		{
			memcpy(&pTcpCtx->redirectInfo.classifyOut, classifyOut, sizeof(FWPS_CLASSIFY_OUT));

			pTcpCtx->transportEndpointHandle = inMetaValues->transportEndpointHandle;

			KdPrint((DPREFIX"callouts_connectRedirectCallout[%I64u] endpoint %I64u\n", 
				pTcpCtx->id, inMetaValues->transportEndpointHandle));

			pTcpCtx->redirectInfo.filterId = filter->filterId;

#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
			status = FwpsRedirectHandleCreate(&g_providerGuid, 0, &pTcpCtx->redirectInfo.redirectHandle);
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_connectRedirectCallout FwpsRedirectHandleCreate failed, status=%x\n", status));
				break;
			}
#endif
#endif
			status = FwpsAcquireClassifyHandle((void*)classifyContext,
											  0,
											  &(pTcpCtx->redirectInfo.classifyHandle));
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_connectRedirectCallout FwpsAcquireClassifyHandle failed, status=%x\n", status));
				break;
			}

			status = _FwpsPendClassify0(pTcpCtx->redirectInfo.classifyHandle,
									 filter->filterId,
									 0,
									 &pTcpCtx->redirectInfo.classifyOut);
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_connectRedirectCallout FwpsPendClassify failed, status=%x\n", status));
				break;
		    }

			tcpctx_addToHandleTable(pTcpCtx);

			pTcpCtx->redirectInfo.isPended = TRUE;

			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

			status = devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CONNECT_REQUEST, pTcpCtx, NULL);
			if (!NT_SUCCESS(status))
			{
//				tcpctx_purgeRedirectInfo(pTcpCtx);
//				classifyOut->actionType = FWP_ACTION_PERMIT;
				break;
			}

			return;
		} else
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		break;
	} 

	if (!NT_SUCCESS(status))
	{
		if (pTcpCtx)
		{
			tcpctx_release(pTcpCtx);
		}
	}
}

NTSTATUS callouts_connectRedirectNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to connectRedirect layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from connectRedirect layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}


VOID callouts_endpointClosureCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	PTCPCTX  pTcpCtx;

    UNREFERENCED_PARAMETER(inFixedValues);
    UNREFERENCED_PARAMETER(packet);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

	if (!devctrl_isProxyAttached())
	{
		return;
	}

	KdPrint((DPREFIX"callouts_endpointClosureCallout endpoint %I64u\n", inMetaValues->transportEndpointHandle));

	pTcpCtx = tcpctx_findByHandle(inMetaValues->transportEndpointHandle);
	if (pTcpCtx)
	{
		KdPrint((DPREFIX"callouts_endpointClosureCallout[%I64u]\n", pTcpCtx->id));

		devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CLOSED, pTcpCtx, NULL);
	
		tcpctx_release(pTcpCtx);
		tcpctx_release(pTcpCtx);
	}
}

NTSTATUS callouts_endpointClosureNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to endpointClosure layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from endpointClosure layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}


VOID callouts_connectRedirectUdpCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER * filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	PUDPCTX  pUdpCtx = NULL;

	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
	{
		KdPrint((DPREFIX"callouts_connectRedirectUdpCallout no FWPS_RIGHT_ACTION_WRITE\n"));
		return;
	}

	if (!devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	KdPrint((DPREFIX"callouts_connectRedirectUdpCallout\n"));

	if (!packet || 
		!inMetaValues->transportEndpointHandle ||
		!classifyContext ||
        (inFixedValues->incomingValue[FWPS_FIELD_ALE_CONNECT_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE))
	{
		KdPrint((DPREFIX"callouts_connectRedirectUdpCallout bypass\n"));
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	if ((ULONG)inMetaValues->processId == devctrl_getProxyPid())
	{
		KdPrint((DPREFIX"callouts_connectRedirectUdpCallout: bypass own connection\n"));
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	if (inFixedValues->layerId == FWPS_LAYER_ALE_CONNECT_REDIRECT_V4)
	{
		if (inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V4_IP_REMOTE_ADDRESS].value.uint32 != 0)
		{
			KdPrint((DPREFIX"callouts_connectRedirectUdpCallout bypass generic send\n"));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			return;
		}
	} else
	{
		char zero[16] = {0};

		if (memcmp(inFixedValues->incomingValue[FWPS_FIELD_DATAGRAM_DATA_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16, zero, 16) != 0)
		{
			KdPrint((DPREFIX"callouts_connectRedirectUdpCallout bypass generic send\n"));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			return;
		}
	}

	for (;;) 
	{
		pUdpCtx = udpctx_findByHandle(inMetaValues->transportEndpointHandle);
		if (!pUdpCtx)
		{
			pUdpCtx = callouts_createUdpFlowContext(inFixedValues, inMetaValues);
			if (!pUdpCtx)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				break;
			}

			status = devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CREATED, pUdpCtx, NULL);
			if (!NT_SUCCESS(status))
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				break;
			}
		}

		pUdpCtx->filteringFlag = rules_findByUdpInfo(pUdpCtx, (char*)pUdpCtx->remoteAddr, NF_D_OUT);

		KdPrint((DPREFIX"callouts_connectRedirectUdpCallout[%I64u] flags = %d\n", pUdpCtx->id, pUdpCtx->filteringFlag));

		if (pUdpCtx->filteringFlag & NF_BLOCK)
		{
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
			status = STATUS_UNSUCCESSFUL;
			KdPrint((DPREFIX"callouts_connectRedirectUdpCallout block\n"));
			break;
		}

		if (pUdpCtx->filteringFlag & NF_INDICATE_CONNECT_REQUESTS)
		{
			memcpy(&pUdpCtx->redirectInfo.classifyOut, classifyOut, sizeof(FWPS_CLASSIFY_OUT));

			KdPrint((DPREFIX"callouts_connectRedirectUdpCallout[%I64u] endpoint %I64u\n", 
				pUdpCtx->id, inMetaValues->transportEndpointHandle));

			pUdpCtx->redirectInfo.filterId = filter->filterId;

#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
			status = FwpsRedirectHandleCreate(&g_providerGuid, 0, &pUdpCtx->redirectInfo.redirectHandle);
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_connectRedirectUdpCallout FwpsRedirectHandleCreate failed, status=%x\n", status));
				break;
			}
#endif
#endif

			status = FwpsAcquireClassifyHandle((void*)classifyContext,
											  0,
											  &(pUdpCtx->redirectInfo.classifyHandle));
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_connectRedirectUdpCallout FwpsAcquireClassifyHandle failed, status=%x\n", status));
				break;
			}

			status = FwpsPendClassify(pUdpCtx->redirectInfo.classifyHandle,
									 filter->filterId,
									 0,
									 &pUdpCtx->redirectInfo.classifyOut);
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_connectRedirectUdpCallout FwpsPendClassify failed, status=%x\n", status));
				break;
		    }

			pUdpCtx->redirectInfo.isPended = TRUE;

			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

			status = devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CONNECT_REQUEST, pUdpCtx, NULL);
			if (!NT_SUCCESS(status))
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				break;
			}
		} else
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		break;
	} 

	if (pUdpCtx)
	{
		udpctx_release(pUdpCtx);
	}
}

NTSTATUS callouts_connectRedirectUdpNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to connectRedirectUdp layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from connectRedirectUdp layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}

VOID callouts_endpointClosureUdpCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	PUDPCTX  pUdpCtx;

    UNREFERENCED_PARAMETER(inFixedValues);
    UNREFERENCED_PARAMETER(packet);
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(flowContext);

	if (devctrl_isShutdown())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	KdPrint((DPREFIX"callouts_endpointClosureUdpCallout endpoint %I64u\n", inMetaValues->transportEndpointHandle));

	pUdpCtx = udpctx_findByHandle(inMetaValues->transportEndpointHandle);
	if (pUdpCtx)
	{
		KdPrint((DPREFIX"callouts_endpointClosureUdpCallout[%I64u]\n", pUdpCtx->id));

		if (pUdpCtx->processId != devctrl_getProxyPid())
		{
			devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CLOSED, pUdpCtx, NULL);
		}
	
		udpctx_release(pUdpCtx);
		udpctx_release(pUdpCtx);
	}
}

NTSTATUS callouts_endpointClosureUdpNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to endpointClosureUdp layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from endpointClosureUdp layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}

BOOLEAN callouts_getPacketProtocolAndPorts(   
	IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	IN VOID* packet,
	BOOLEAN isSend,
	BOOLEAN isIPv4,
	OUT PACKET_INFO * pPacketInfo)
{
	PVOID pData = NULL;
	NET_BUFFER* netBuffer = NULL;
	BOOLEAN result = FALSE;
	UINT32 bytesRetreated = 0;
	NTSTATUS status;
	USHORT srcPort = 0, dstPort = 0;
	ULONG ipHeaderLength;

	netBuffer = NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet);

	if (!isSend)
	{
		status = NdisRetreatNetBufferDataStart(
				netBuffer,
				inMetaValues->ipHeaderSize,
				0,
				NULL
				);
		if (status == STATUS_SUCCESS)
		{
			bytesRetreated = inMetaValues->ipHeaderSize;
		}
	}

	pData = NdisGetDataBuffer(
			netBuffer,
			inMetaValues->ipHeaderSize,
			0,
			1,
			0);

	if (pData)
	{
		ipHeaderLength = ipctx_getIPHeaderLengthAndProtocol(netBuffer, &pPacketInfo->protocol);
		if (ipHeaderLength == 0)
			ipHeaderLength = inMetaValues->ipHeaderSize;

		result = TRUE;

		if (!isSend)
		{
			NdisAdvanceNetBufferDataStart(
					netBuffer,
					bytesRetreated,
					FALSE,
					NULL
					);

			bytesRetreated = 0;
		} else
		{
			NdisAdvanceNetBufferDataStart(
					netBuffer,
					ipHeaderLength,
					FALSE,
					NULL
					);
		}

		netBuffer = NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet);

		if (pPacketInfo->protocol == IPPROTO_TCP)
		{
			PTCP_HEADER pHeader;

			pData = NdisGetDataBuffer(
					netBuffer,
					sizeof(TCP_HEADER),
					0,
					1,
					0);

			if (pData)
			{
				pHeader = (PTCP_HEADER)pData;
				srcPort = pHeader->th_sport;
				dstPort = pHeader->th_dport;
			}
		} else
		if (pPacketInfo->protocol == IPPROTO_UDP)
		{
			PUDP_HEADER pHeader;

			pData = NdisGetDataBuffer(
					netBuffer,
					sizeof(UDP_HEADER),
					0,
					1,
					0);

			if (pData)
			{
				pHeader = (PUDP_HEADER)pData;
				srcPort = pHeader->srcPort;
				dstPort = pHeader->destPort;
			}
		}

		if (isSend)
		{
			NdisRetreatNetBufferDataStart(
					netBuffer,
					ipHeaderLength,
					0,
					NULL
					);
		}

		if (srcPort && dstPort)
		{
			if (isSend)
			{
				if (isIPv4)
				{
					pPacketInfo->localAddress.AddressIn.sin_port = srcPort;
					pPacketInfo->remoteAddress.AddressIn.sin_port = dstPort;
				} else
				{
					pPacketInfo->localAddress.AddressIn6.sin6_port = srcPort;
					pPacketInfo->remoteAddress.AddressIn6.sin6_port = dstPort;
				}
			} else
			{
				if (isIPv4)
				{
					pPacketInfo->localAddress.AddressIn.sin_port = dstPort;
					pPacketInfo->remoteAddress.AddressIn.sin_port = srcPort;
				} else
				{
					pPacketInfo->localAddress.AddressIn6.sin6_port = dstPort;
					pPacketInfo->remoteAddress.AddressIn6.sin6_port = srcPort;
				}
			}
		}
	}

	if (bytesRetreated)
	{
		NdisAdvanceNetBufferDataStart(
				netBuffer,
				bytesRetreated,
				FALSE,
				NULL
				);
	}

	return result;
}

BOOLEAN callouts_getPacketInfo(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   OUT PACKET_INFO * pPacketInfo)
{
	BOOLEAN result = FALSE;

	if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4)
	{
		memset(&pPacketInfo->localAddress, 0, sizeof(pPacketInfo->localAddress));
		pPacketInfo->localAddress.AddressIn.sin_family = AF_INET;
		pPacketInfo->localAddress.AddressIn.sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS].value.uint32);

		memset(&pPacketInfo->remoteAddress, 0, sizeof(pPacketInfo->remoteAddress));
		pPacketInfo->remoteAddress.AddressIn.sin_family = AF_INET;
		pPacketInfo->remoteAddress.AddressIn.sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_IP_REMOTE_ADDRESS].value.uint32);

		pPacketInfo->direction = NF_D_IN;

		result = callouts_getPacketProtocolAndPorts(inMetaValues, packet, FALSE, TRUE, pPacketInfo);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4)
	{
		memset(&pPacketInfo->localAddress, 0, sizeof(pPacketInfo->localAddress));
		pPacketInfo->localAddress.AddressIn.sin_family = AF_INET;
		pPacketInfo->localAddress.AddressIn.sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_LOCAL_ADDRESS].value.uint32);

		memset(&pPacketInfo->remoteAddress, 0, sizeof(pPacketInfo->remoteAddress));
		pPacketInfo->remoteAddress.AddressIn.sin_family = AF_INET;
		pPacketInfo->remoteAddress.AddressIn.sin_addr.S_un.S_addr = 
			htonl(inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_IP_REMOTE_ADDRESS].value.uint32);

		pPacketInfo->direction = NF_D_OUT;

		result = callouts_getPacketProtocolAndPorts(inMetaValues, packet, TRUE, TRUE, pPacketInfo);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
	{
		memset(&pPacketInfo->localAddress, 0, sizeof(pPacketInfo->localAddress));
		pPacketInfo->localAddress.AddressIn6.sin6_family = AF_INET6;
		memcpy(&pPacketInfo->localAddress.AddressIn6.sin6_addr, 
					inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);

		memset(&pPacketInfo->remoteAddress, 0, sizeof(pPacketInfo->remoteAddress));
		pPacketInfo->remoteAddress.AddressIn6.sin6_family = AF_INET6;
		memcpy(&pPacketInfo->remoteAddress.AddressIn6.sin6_addr, 
					inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);

		pPacketInfo->direction = NF_D_IN;

		result = callouts_getPacketProtocolAndPorts(inMetaValues, packet, FALSE, FALSE, pPacketInfo);
	} else
	if (inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
	{
		memset(&pPacketInfo->localAddress, 0, sizeof(pPacketInfo->localAddress));
		pPacketInfo->localAddress.AddressIn6.sin6_family = AF_INET6;
		memcpy(&pPacketInfo->localAddress.AddressIn6.sin6_addr, 
					inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);

		memset(&pPacketInfo->remoteAddress, 0, sizeof(pPacketInfo->remoteAddress));
		pPacketInfo->remoteAddress.AddressIn6.sin6_family = AF_INET6;
		memcpy(&pPacketInfo->remoteAddress.AddressIn6.sin6_addr, 
					inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V6_IP_REMOTE_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);

		pPacketInfo->direction = NF_D_OUT;

		result = callouts_getPacketProtocolAndPorts(inMetaValues, packet, TRUE, FALSE, pPacketInfo);
	} else
	{
		return FALSE;
	}


	return result;
}


VOID callouts_IPPacketCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER* filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	FWPS_PACKET_INJECTION_STATE packetState;
	BOOLEAN isSend = FALSE;
	ULONG packetFlags = 0;
    NET_BUFFER* netBuffer;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	PACKET_INFO	packetInfo;
	HANDLE inject_context = NULL;

	UNREFERENCED_PARAMETER(classifyContext);
	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
	{
		KdPrint((DPREFIX"callouts_IPPacketCallout no FWPS_RIGHT_ACTION_WRITE\n"));
		return;
	}

	if (devctrl_isShutdown() || !devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	if (!(rules_getRulesMask() & RM_IP))
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		KdPrint((DPREFIX"callouts_IPPacketCallout !RM_IP\n"));
		return;
	}

	if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
		inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4)
	{
		packetState = FwpsQueryPacketInjectionState0(
							devctrl_getNwV4InjectionHandle(),
							(NET_BUFFER_LIST*)packet,
							&inject_context
							);
		
		if (packetState == FWPS_PACKET_INJECTED_BY_SELF ||
			packetState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
		{
			if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4)
			{
				KdPrint((DPREFIX"callouts_IPPacketCallout injected by self or others on NwV4, interface=%d, context=%d\n",
					inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX].value.uint32,
					*(ULONG*)&inject_context));

				if (inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX].value.uint32 == *(ULONG*)&inject_context)
				{
					classifyOut->actionType = FWP_ACTION_PERMIT;
					KdPrint((DPREFIX"callouts_IPPacketCallout injected by self or others on NwV4\n"));
					return;
				}
			} else
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_IPPacketCallout injected by self or others on NwV4\n"));
				return;
			}
		}
	} else
	if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
		inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
	{
		packetState = FwpsQueryPacketInjectionState0(
							devctrl_getNwV6InjectionHandle(),
							(NET_BUFFER_LIST*)packet,
							&inject_context
							);
		if (packetState == FWPS_PACKET_INJECTED_BY_SELF ||
			packetState == FWPS_PACKET_PREVIOUSLY_INJECTED_BY_SELF)
		{
			if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
			{
				KdPrint((DPREFIX"callouts_IPPacketCallout injected by self or others on NwV6, interface=%d, context=%d\n",
					inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_INTERFACE_INDEX].value.uint32,
					*(ULONG*)&inject_context));

				if (inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_INTERFACE_INDEX].value.uint32 == *(ULONG*)&inject_context)
				{
					classifyOut->actionType = FWP_ACTION_PERMIT;
					KdPrint((DPREFIX"callouts_IPPacketCallout injected by self or others on NwV6\n"));
					return;
				}
			} else
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_IPPacketCallout injected by self or others on NwV6\n"));
				return;
			}
		}
	}

	{
		UINT32 flags = 0;
		FWP_VALUE * pValue = NULL;

		if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4)
		{
			pValue = &(inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_FLAGS].value);
		} else
		if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6)
		{
			pValue = &(inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_FLAGS].value);
		} else
		if (inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4)
		{
			pValue = &(inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V4_FLAGS].value);
		} else
		if (inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
		{
			pValue = &(inFixedValues->incomingValue[FWPS_FIELD_OUTBOUND_IPPACKET_V6_FLAGS].value);
		}

		if (pValue && pValue->type == FWP_UINT32)
			flags = pValue->uint32;

		if (flags & FWP_CONDITION_FLAG_IS_LOOPBACK)
		{
			KdPrint((DPREFIX"callouts_IPPacketCallout bypass loopback\n"));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			return;
		}

		if (flags & FWP_CONDITION_FLAG_IS_IPSEC_SECURED)
		{
			KdPrint((DPREFIX"callouts_IPPacketCallout bypass IPSEC\n"));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			return;
		}
	}

	for (;;)
	{

		memset(&packetInfo, 0, sizeof(packetInfo));

		if (!callouts_getPacketInfo(inFixedValues, inMetaValues, packet, &packetInfo))
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
			KdPrint((DPREFIX"callouts_IPPacketCallout !callouts_getPacketInfo\n"));
			break;
		}

		KdPrint((DPREFIX"callouts_IPPacketCallout packet direction=%d, protocol=%d, localPort=%d, remotePort=%d\n", 
			packetInfo.direction,
			packetInfo.protocol,
			htons(packetInfo.localAddress.AddressIn.sin_port),
			htons(packetInfo.remoteAddress.AddressIn.sin_port)));

		flag = rules_findByIPInfo(&packetInfo);

		if (inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4 ||
			inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
		{
			isSend = TRUE;
		}

		if (flag & NF_BLOCK)
		{
			KdPrint((DPREFIX"callouts_IPPacketCallout packet denied by rules\n"));
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;
			break;
		} else
		if (!(flag & NF_FILTER_AS_IP_PACKETS))
		{
//			KdPrint((DPREFIX"callouts_IPPacketCallout packet allowed by rules\n"));
			classifyOut->actionType = FWP_ACTION_PERMIT;
			break;
		}

		if (flag & NF_READONLY)
		{
			packetFlags |= NFIF_READONLY;
		}

		for (netBuffer = NET_BUFFER_LIST_FIRST_NB((NET_BUFFER_LIST*)packet);
		   netBuffer != NULL;
		   netBuffer = NET_BUFFER_NEXT_NB(netBuffer))
		{
			if (!ipctx_pushPacket(inFixedValues, inMetaValues, netBuffer, isSend, packetFlags))
			{
				KdPrint((DPREFIX"callouts_IPPacketCallout !ipctx_pushPacket\n"));
				break;
			}
		}

		if (flag & NF_READONLY)
		{
			classifyOut->actionType = FWP_ACTION_PERMIT;
		} else
		{
			classifyOut->actionType = FWP_ACTION_BLOCK;
			classifyOut->flags |= FWPS_CLASSIFY_OUT_FLAG_ABSORB;
			classifyOut->rights ^= FWPS_RIGHT_ACTION_WRITE;
		}

		break;
	} 

}


NTSTATUS callouts_IPPacketNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to IP packet layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from IP packet layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}

VOID callouts_bindRedirectCallout(
   IN const FWPS_INCOMING_VALUES* inFixedValues,
   IN const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
   IN VOID* packet,
   IN const void* classifyContext,
   IN const FWPS_FILTER * filter,
   IN UINT64 flowContext,
   OUT FWPS_CLASSIFY_OUT* classifyOut)
{
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(filter);
	UNREFERENCED_PARAMETER(flowContext);

	if ((classifyOut->rights & FWPS_RIGHT_ACTION_WRITE) == 0)
	{
		KdPrint((DPREFIX"callouts_bindRedirectCallout no FWPS_RIGHT_ACTION_WRITE\n"));
		return;
	}

	if (!devctrl_isProxyAttached())
	{
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	if (!classifyContext ||
        (inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_FLAGS].value.uint32 & FWP_CONDITION_FLAG_IS_REAUTHORIZE))
	{
		KdPrint((DPREFIX"callouts_bindRedirectCallout bypass\n"));
		classifyOut->actionType = FWP_ACTION_PERMIT;
		return;
	}

	for (;;) 
	{
		NF_BINDING_RULE bindInfo;
		ULONG filteringFlag;
		int offset, len;

		memset(&bindInfo, 0, sizeof(bindInfo));

		bindInfo.processId = (ULONG)inMetaValues->processId;

		if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4)
		{
			bindInfo.protocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_PROTOCOL].value.uint8;
			bindInfo.ip_family = AF_INET;

			*(ULONG*)bindInfo.localIpAddress = 
				htonl(inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_ADDRESS].value.uint32);

			bindInfo.localPort = 
				htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_IP_LOCAL_PORT].value.uint16);

			if (inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_APP_ID].value.byteBlob)
			{
				len = inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_APP_ID].value.byteBlob->size;
				if (len > sizeof(bindInfo.processName) - 2)
				{
					offset = len - (sizeof(bindInfo.processName) - 2);
					len = sizeof(bindInfo.processName) - 2;
				} else
				{
					offset = 0;
				}
				memcpy(bindInfo.processName, 
					inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V4_ALE_APP_ID].value.byteBlob->data + offset,
					len);
			}

			KdPrint((DPREFIX"callouts_bindRedirectCallout protocol=%d, processId=%d, localAddr=%x:%d\n", 
				bindInfo.protocol, bindInfo.processId, *(ULONG*)bindInfo.localIpAddress, htons(bindInfo.localPort)));
		} else
		if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V6)
		{
			bindInfo.protocol = inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_IP_PROTOCOL].value.uint8;
			bindInfo.ip_family = AF_INET6;

			memcpy(bindInfo.localIpAddress, 
					inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_IP_LOCAL_ADDRESS].value.byteArray16->byteArray16,
					NF_MAX_IP_ADDRESS_LENGTH);

			bindInfo.localPort = 
				htons(inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_IP_LOCAL_PORT].value.uint16);

			if (inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_ALE_APP_ID].value.byteBlob)
			{
				len = inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_ALE_APP_ID].value.byteBlob->size;
				if (len > sizeof(bindInfo.processName) - 2)
				{
					offset = len - (sizeof(bindInfo.processName) - 2);
					len = sizeof(bindInfo.processName) - 2;
				} else
				{
					offset = 0;
				}
				memcpy(bindInfo.processName, 
					inFixedValues->incomingValue[FWPS_FIELD_ALE_BIND_REDIRECT_V6_ALE_APP_ID].value.byteBlob->data + offset,
					len);
			}

			KdPrint((DPREFIX"callouts_bindRedirectCallout protocol=%d, processId=%d, localAddr=[]:%d\n", 
				bindInfo.protocol, bindInfo.processId, htons(bindInfo.localPort)));
		}

		filteringFlag = rules_findByBindInfo(&bindInfo);

		if (filteringFlag & NF_FILTER)
		{
			UINT64 classifyHandle;
			FWPS_BIND_REQUEST * pBindRequest;

			status = FwpsAcquireClassifyHandle((void*)classifyContext,
											  0,
											  &classifyHandle);
			if (status != STATUS_SUCCESS)
			{
				classifyOut->actionType = FWP_ACTION_PERMIT;
				KdPrint((DPREFIX"callouts_bindRedirectCallout FwpsAcquireClassifyHandle failed, status=%x\n", status));
				break;
			}

			status = FwpsAcquireWritableLayerDataPointer(classifyHandle,
														filter->filterId,
														0,
														(PVOID*)&pBindRequest,
														classifyOut);
			if (status == STATUS_SUCCESS)
			{
				sockaddr_gen * pAddr = (sockaddr_gen *)&pBindRequest->localAddressAndPort;

				if (inFixedValues->layerId == FWPS_LAYER_ALE_BIND_REDIRECT_V4)
				{
					pAddr->AddressIn.sin_addr.S_un.S_addr = *(ULONG*)bindInfo.newLocalIpAddress;
					memset(pAddr->AddressIn.sin_zero, 0, sizeof(pAddr->AddressIn.sin_zero));

					if (bindInfo.newLocalPort != 0)
					{
						pAddr->AddressIn.sin_port = bindInfo.newLocalPort;
					}
				} else 
				{
					memcpy(&pAddr->AddressIn6.sin6_addr, bindInfo.newLocalIpAddress, NF_MAX_IP_ADDRESS_LENGTH);

					if (bindInfo.newLocalPort != 0)
					{
						pAddr->AddressIn6.sin6_port = bindInfo.newLocalPort;
					}
				}

				KdPrint((DPREFIX"callouts_bindRedirectCallout redirected\n"));

				FwpsApplyModifiedLayerData(classifyHandle,
										 pBindRequest,
										 0);
			} else
			{
				KdPrint((DPREFIX"callouts_bindRedirectCallout FwpsAcquireWritableLayerDataPointer failed, status=%x\n", status));
			}

		    FwpsReleaseClassifyHandle(classifyHandle);
		}

		classifyOut->actionType = FWP_ACTION_PERMIT;
		classifyOut->rights &= ~FWPS_RIGHT_ACTION_WRITE;

		break;
	}
}

NTSTATUS callouts_bindRedirectNotify(
    IN  FWPS_CALLOUT_NOTIFY_TYPE        notifyType,
    IN  const GUID*             filterKey,
    IN  const FWPS_FILTER*     filter)
{
    UNREFERENCED_PARAMETER(notifyType);
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    switch (notifyType)
    {
    case FWPS_CALLOUT_NOTIFY_ADD_FILTER:
        KdPrint((DPREFIX"Filter Added to bindRedirect layer.\n"));
       break;
    case FWPS_CALLOUT_NOTIFY_DELETE_FILTER:
        KdPrint((DPREFIX"Filter Deleted from bindRedirect layer.\n"));
       break;
    }
    return STATUS_SUCCESS;
}

NTSTATUS callouts_init(PDEVICE_OBJECT deviceObject)
{
	NTSTATUS status;
	DWORD dwStatus;
	FWPM_SESSION session = {0};
	int i;

	if (g_initialized)
		return STATUS_SUCCESS;

	ExUuidCreate(&g_providerGuid);
	ExUuidCreate(&g_sublayerGuid);
	ExUuidCreate(&g_recvSublayerGuid);
	ExUuidCreate(&g_recvProtSublayerGuid);
	
	for (i=0; i<CG_MAX; i++)
	{
		ExUuidCreate(&g_calloutGuids[i]);
	}

	session.flags = FWPM_SESSION_FLAG_DYNAMIC;

	status = FwpmEngineOpen(
                NULL,
                RPC_C_AUTHN_WINNT,
                NULL,
                &session,
                &g_engineHandle
                );
	if (!NT_SUCCESS(status))
	{
		KdPrint((DPREFIX"FwpmEngineOpen failed, status=%x\n", status));
		return status;
	}

	for (;;)
	{
		FWPM_PROVIDER provider;

		RtlZeroMemory(&provider, sizeof(provider));
		provider.displayData.description = NFSDK_PROVIDER_NAME;
		provider.displayData.name = NFSDK_PROVIDER_NAME;
		provider.providerKey = g_providerGuid;

		dwStatus = FwpmProviderAdd(g_engineHandle, &provider, NULL);
		if (dwStatus != 0)
		{
			KdPrint((DPREFIX"FwpmProviderAdd failed, status=%x\n", dwStatus));
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		status = callouts_registerCallouts(deviceObject);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"callouts_registerCallouts failed, status=%x\n", status));
			break;
		}

		status = callouts_addFilters();
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"callouts_addFilters failed, status=%x\n", status));
			break;
		}
	
		break;
	} 

	g_initialized = TRUE;

	if (!NT_SUCCESS(status))
	{
		callouts_free();
	}

	return status;
}

void callouts_free()
{
	KdPrint((DPREFIX"callouts_free\n"));

	if (!g_initialized)
		return;

	g_initialized = FALSE;

	callouts_unregisterCallouts();

	FwpmSubLayerDeleteByKey(g_engineHandle, &g_sublayerGuid);
	FwpmSubLayerDeleteByKey(g_engineHandle, &g_recvSublayerGuid);
	FwpmSubLayerDeleteByKey(g_engineHandle, &g_recvProtSublayerGuid);
	FwpmSubLayerDeleteByKey(g_engineHandle, &g_ipSublayerGuid);

	FwpmProviderContextDeleteByKey(g_engineHandle, &g_providerGuid);

	if (g_engineHandle)
	{
		FwpmEngineClose(g_engineHandle);
		g_engineHandle = NULL;
	}
}

