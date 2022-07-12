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
#include "ipctx.h"
#include "devctrl.h"

#ifdef _WPPTRACE
#include "ipctx.tmh"
#endif

static IPCTX	g_ipCtx;

static NPAGED_LOOKASIDE_LIST g_ipPacketsLAList;

static BOOLEAN		g_initialized = FALSE;

PIPCTX ipctx_get()
{
	return &g_ipCtx;
}

BOOLEAN ipctx_init()
{
    ExInitializeNPagedLookasideList( &g_ipPacketsLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_IP_PACKET ),
									 MEM_TAG_IP_PACKET,
                                     0 );

	sl_init(&g_ipCtx.lock);
	InitializeListHead(&g_ipCtx.pendedPackets);

	g_initialized = TRUE;
	
	return TRUE;
}

void ipctx_cleanup()
{
	KLOCK_QUEUE_HANDLE lh;
	NF_IP_PACKET * pPacket;
	
	KdPrint((DPREFIX"ipctx_cleanup\n"));

	sl_lock(&g_ipCtx.lock, &lh);

	while (!IsListEmpty(&g_ipCtx.pendedPackets))
	{
		pPacket = (PNF_IP_PACKET)RemoveHeadList(&g_ipCtx.pendedPackets);
		ipctx_freePacket(pPacket);
		KdPrint((DPREFIX"ipctx_cleanup packet\n"));
	}

	sl_unlock(&lh);
}

void ipctx_free()
{
	KdPrint((DPREFIX"ipctx_free\n"));

	if (!g_initialized)
		return;

	ipctx_cleanup();

	ExDeleteNPagedLookasideList( &g_ipPacketsLAList );

	g_initialized = FALSE;
}

PNF_IP_PACKET ipctx_allocPacket(ULONG dataLength)
{
	PNF_IP_PACKET pPacket;

	pPacket = (PNF_IP_PACKET)ExAllocateFromNPagedLookasideList( &g_ipPacketsLAList );
	if (!pPacket)
		return NULL;

	memset(pPacket, 0, sizeof(NF_IP_PACKET));

	if (dataLength > 0)
	{
		pPacket->dataBuffer = (char*)ExAllocatePoolWithTag(NonPagedPool, dataLength, MEM_TAG_IP_DATA_COPY);
		if (!pPacket->dataBuffer)
		{
			ExFreeToNPagedLookasideList( &g_ipPacketsLAList, pPacket );
			return NULL;
		}
	}

	return pPacket;
}

void ipctx_freePacket(PNF_IP_PACKET pPacket)
{
	if (pPacket->dataBuffer)
	{
		free_np(pPacket->dataBuffer);
		pPacket->dataBuffer = NULL;
	}
	ExFreeToNPagedLookasideList( &g_ipPacketsLAList, pPacket );
}

ULONG ipctx_getIPHeaderLengthAndProtocol(NET_BUFFER * nb, int * pProtocol)
{
	PIP_HEADER_V4 pIPv4Header;
	ULONG ipHeaderSize = 0;
	ULONG packetLength;

	packetLength = NET_BUFFER_DATA_LENGTH(nb);

	if (packetLength < sizeof(struct iphdr))
	{
		KdPrint((DPREFIX"ipctx_getIPHeaderLength too small packet\n"));
		return 0;
	}

	pIPv4Header = (IP_HEADER_V4*)NdisGetDataBuffer(
			nb,
			sizeof(struct iphdr),
			0,
			1,
			0);
	if (!pIPv4Header)
	{
		KdPrint((DPREFIX"ipctx_getIPHeaderLength cannot get IPv4 header\n"));
		return 0;
	}

	if (pIPv4Header->version == 4)
	{
		ipHeaderSize = pIPv4Header->headerLength * 4;
		if (pProtocol)
		{
			*pProtocol = pIPv4Header->protocol;
		}
	} else
	if (pIPv4Header->version == 6)
	{
		PIP_HEADER_V6 pIPv6Header;
		UCHAR proto;
		UINT8 *ext_header;
		ULONG ext_header_len;
		BOOL isexthdr;
				
		pIPv6Header = (IP_HEADER_V6*)NdisGetDataBuffer(
				nb,
				sizeof(IP_HEADER_V6),
				NULL,
				1,
				0);
		if (!pIPv6Header)
		{
			KdPrint((DPREFIX"ipctx_getIPHeaderLength cannot get IPv6 header\n"));
			return 0;
		}

		ipHeaderSize = sizeof(IP_HEADER_V6);

		proto = pIPv6Header->nextHeader;

		NdisAdvanceNetBufferDataStart(nb, ipHeaderSize, FALSE, NULL);

		isexthdr = TRUE;

		for (;;)
		{
			ext_header = (UINT8 *)NdisGetDataBuffer(nb, 2, NULL, 1, 0);
			if (ext_header == NULL)
			{
				break;
			}

			ext_header_len = (size_t)ext_header[1];
			switch (proto)
			{
				case IPPROTO_FRAGMENT:
					{
						ext_header_len = 8;
					}
					break;
				case IPPROTO_AH:
					ext_header_len += 2;
					ext_header_len *= 4;
					break;
				case IPPROTO_HOPOPTS:
				case IPPROTO_DSTOPTS:
				case IPPROTO_ROUTING:
					ext_header_len++;
					ext_header_len *= 8;
					break;
				default:
					isexthdr = FALSE;
					break;
			}

			if (!isexthdr)
			{
				break;
			}

			proto = ext_header[0];
			ipHeaderSize += ext_header_len;

			NdisAdvanceNetBufferDataStart(nb, ext_header_len, FALSE, NULL);
		}

		NdisRetreatNetBufferDataStart(nb, ipHeaderSize,	0, NULL);

		if (pProtocol)
		{
			*pProtocol = proto;
		}
	}

	return ipHeaderSize;
}

BOOLEAN ipctx_pushPacket(
	const FWPS_INCOMING_VALUES* inFixedValues,
	const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
	NET_BUFFER* netBuffer,
	BOOLEAN isSend,
	ULONG flags)
{
	PNF_IP_PACKET pPacket = NULL;
	ULONG dataLength;
	KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result = FALSE;
	ULONG ipHeaderLength;

	dataLength = NET_BUFFER_DATA_LENGTH(netBuffer);
	if (!isSend)
	{
		dataLength += inMetaValues->ipHeaderSize;
	}

	sl_lock(&g_ipCtx.lock, &lh);

	if (g_ipCtx.pendedRecvBytes > TCP_RECV_INJECT_LIMIT ||
		// EDR_FIX: FIXME: TCP_SEND_INJECT_LIMIT need to use?
		g_ipCtx.pendedSendBytes > TCP_RECV_INJECT_LIMIT)
	{
		sl_unlock(&lh);
		KdPrint((DPREFIX"ipctx_pushPacket too large queue\n"));
		return FALSE;
	}

	if (isSend)
	{
		g_ipCtx.pendedSendBytes += dataLength;
		KdPrint((DPREFIX"ipctx_pushPacket pendedSendBytes=%u, packetsLen=%u\n", 
			g_ipCtx.pendedSendBytes, dataLength));
	} else
	{
		g_ipCtx.pendedRecvBytes += dataLength;
		KdPrint((DPREFIX"ipctx_pushPacket pendedRecvBytes=%u, packetsLen=%u\n", 
			g_ipCtx.pendedRecvBytes, dataLength));
	}
	
	sl_unlock(&lh);

	for (;;)
	{
		pPacket = ipctx_allocPacket(dataLength);
		if (!pPacket)
		{
			KdPrint((DPREFIX"ipctx_pushPacket unable to allocate packet\n"));
			break;
		}
		
		{
			void * buf = NULL;

			if (!isSend)
			{
				NdisRetreatNetBufferDataStart(
						netBuffer,
						inMetaValues->ipHeaderSize,
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
				ASSERT(0);
			}

			KdPrint((DPREFIX"ipctx_pushPacket ipHeaderSize=%d\n", inMetaValues->ipHeaderSize));

			ipHeaderLength = ipctx_getIPHeaderLengthAndProtocol(netBuffer, NULL);
			if (ipHeaderLength == 0)
				ipHeaderLength = inMetaValues->ipHeaderSize;

			if (!isSend)
			{
				NdisAdvanceNetBufferDataStart(
						netBuffer,
						inMetaValues->ipHeaderSize,
						FALSE,
						NULL
					  );
			}
		}

		pPacket->dataLength = dataLength;
		pPacket->isSend = isSend;

		pPacket->options.ipHeaderSize = ipHeaderLength;
		pPacket->options.compartmentId = (ULONG)inMetaValues->compartmentId;
		pPacket->options.flags = flags;

		if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V4 ||
			inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V4)
		{
			pPacket->options.ip_family = AF_INET;
			pPacket->options.interfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_INTERFACE_INDEX].value.uint32;
			pPacket->options.subInterfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V4_SUB_INTERFACE_INDEX].value.uint32;
		} else
		if (inFixedValues->layerId == FWPS_LAYER_INBOUND_IPPACKET_V6 ||
			inFixedValues->layerId == FWPS_LAYER_OUTBOUND_IPPACKET_V6)
		{
			pPacket->options.ip_family = AF_INET6;
			pPacket->options.interfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_INTERFACE_INDEX].value.uint32;
			pPacket->options.subInterfaceIndex = 
				inFixedValues->incomingValue[FWPS_FIELD_INBOUND_IPPACKET_V6_SUB_INTERFACE_INDEX].value.uint32;
		}
		
		sl_lock(&g_ipCtx.lock, &lh);
		InsertTailList(&g_ipCtx.pendedPackets, &pPacket->entry);
		sl_unlock(&lh);

		if (NT_SUCCESS(devctrl_pushIPData(isSend? NF_IP_SEND : NF_IP_RECEIVE)))
		{
			result = TRUE;
		}
		
		break;
	}

	if (!result)
	{
		sl_lock(&g_ipCtx.lock, &lh);
		if (isSend)
		{
			g_ipCtx.pendedSendBytes -= dataLength;
		} else
		{
			g_ipCtx.pendedRecvBytes -= dataLength;
		}
		sl_unlock(&lh);
	}

	return result;
}

