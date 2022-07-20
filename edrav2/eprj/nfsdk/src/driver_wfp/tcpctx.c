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
#include "tcpctx.h"
#include "devctrl.h"

#ifdef _WPPTRACE
#include "tcpctx.tmh"
#endif

static LIST_ENTRY	g_lTcpCtx;
static KSPIN_LOCK	g_slTcpCtx;

static NPAGED_LOOKASIDE_LIST g_tcpCtxLAList;
static PHASH_TABLE	g_phtTcpCtxById = NULL;
static PHASH_TABLE	g_phtTcpCtxByHandle = NULL;
static ENDPOINT_ID	g_nextTcpCtxId;
static NPAGED_LOOKASIDE_LIST g_packetsLAList;

static BOOLEAN		g_initialized = FALSE;

BOOLEAN tcpctx_init()
{
	ExInitializeNPagedLookasideList( &g_tcpCtxLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( TCPCTX ),
									 MEM_TAG_TCP,
                                     0 );

    ExInitializeNPagedLookasideList( &g_packetsLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_PACKET ),
                                     MEM_TAG_TCP_PACKET,
                                     0 );

	InitializeListHead(&g_lTcpCtx);
	sl_init(&g_slTcpCtx);

	g_phtTcpCtxById = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_phtTcpCtxById)
	{
		return FALSE;
	}

	g_phtTcpCtxByHandle = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_phtTcpCtxByHandle)
	{
		return FALSE;
	}

	g_nextTcpCtxId = 1;

	g_initialized = TRUE;
	
	return TRUE;
}

void tcpctx_cleanupFlows()
{
	KLOCK_QUEUE_HANDLE lh, lhp;
	PTCPCTX pTcpCtx;
	NF_PACKET * pPacket;
	LIST_ENTRY packets;
	
	KdPrint((DPREFIX"tcpctx_cleanupFlows\n"));

	InitializeListHead(&packets);

	sl_lock(&g_slTcpCtx, &lh);

	pTcpCtx = (PTCPCTX)g_lTcpCtx.Flink;
	
	while (pTcpCtx != (PTCPCTX)&g_lTcpCtx)
	{
		sl_lock(&pTcpCtx->lock, &lhp);
		while (!IsListEmpty(&pTcpCtx->pendedPackets))
		{
			pPacket = (PNF_PACKET)RemoveHeadList(&pTcpCtx->pendedPackets);
			InsertTailList(&packets, &pPacket->entry);
			KdPrint((DPREFIX"tcpctx_cleanupFlows packet in TCPCTX %I64u\n", pTcpCtx->id));
		}
		while (!IsListEmpty(&pTcpCtx->injectPackets))
		{
			pPacket = (PNF_PACKET)RemoveHeadList(&pTcpCtx->injectPackets);
			InsertTailList(&packets, &pPacket->entry);
			KdPrint((DPREFIX"tcpctx_cleanupFlows reinject packet in TCPCTX %I64u\n", pTcpCtx->id));
		}
		sl_unlock(&lhp);

		pTcpCtx = (PTCPCTX)pTcpCtx->entry.Flink;
	}

	sl_unlock(&lh);

	while (!IsListEmpty(&packets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&packets);
		tcpctx_freePacket(pPacket);
	}
}

void NTAPI 
tcpctx_tcpInjectCompletion(
   IN void* context,
   IN OUT NET_BUFFER_LIST* netBufferList,
   IN BOOLEAN dispatchLevel
   )
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(netBufferList);
	UNREFERENCED_PARAMETER(dispatchLevel);
}



void tcpctx_closeConnections()
{
	KLOCK_QUEUE_HANDLE lh;
	PTCPCTX pTcpCtx;
	NTSTATUS status;
	
	KdPrint((DPREFIX"tcpctx_closeConnections\n"));

	sl_lock(&g_slTcpCtx, &lh);

	pTcpCtx = (PTCPCTX)g_lTcpCtx.Flink;
	
	while (pTcpCtx != (PTCPCTX)&g_lTcpCtx)
	{
		status = FwpsStreamInjectAsync(
				  devctrl_getInjectionHandle(),
				  NULL,
				  0,
				  pTcpCtx->flowHandle,
				  pTcpCtx->sendCalloutId,
				  pTcpCtx->layerId,
				  FWPS_STREAM_FLAG_SEND | FWPS_STREAM_FLAG_SEND_DISCONNECT,
				  NULL,
				  0,
				  tcpctx_tcpInjectCompletion,
				  NULL
				  );
		
		pTcpCtx = (PTCPCTX)pTcpCtx->entry.Flink;
	}

	sl_unlock(&lh);
}

void tcpctx_removeFromFlows()
{
	KLOCK_QUEUE_HANDLE lh;
	PTCPCTX pTcpCtx;
	NTSTATUS status;

	KdPrint((DPREFIX"tcpctx_removeFromFlows\n"));
	
	tcpctx_cleanupFlows();

	sl_lock(&g_slTcpCtx, &lh);

	while (!IsListEmpty(&g_lTcpCtx))
	{
		pTcpCtx = (PTCPCTX)RemoveHeadList(&g_lTcpCtx);

		InitializeListHead(&pTcpCtx->entry);
	        
		KdPrint((DPREFIX"tcpctx_removeFromFlows(): TCPCTX [%I64d]\n", pTcpCtx->id));
	
		pTcpCtx->refCount++;

		sl_unlock(&lh);

		status = FwpsFlowRemoveContext(pTcpCtx->flowHandle,
                                      pTcpCtx->layerId,
                                      pTcpCtx->sendCalloutId);
		ASSERT(NT_SUCCESS(status));

		status = FwpsFlowRemoveContext(pTcpCtx->flowHandle,
                                      pTcpCtx->layerId,
                                      pTcpCtx->recvCalloutId);
		ASSERT(NT_SUCCESS(status));

		status = FwpsFlowRemoveContext(pTcpCtx->flowHandle,
                                      pTcpCtx->layerId,
                                      pTcpCtx->recvProtCalloutId);
		ASSERT(NT_SUCCESS(status));

		status = FwpsFlowRemoveContext(pTcpCtx->flowHandle,
									  pTcpCtx->transportLayerIdOut,
									  pTcpCtx->transportCalloutIdOut);
		ASSERT(NT_SUCCESS(status));

		status = FwpsFlowRemoveContext(pTcpCtx->flowHandle,
									  pTcpCtx->transportLayerIdIn,
									  pTcpCtx->transportCalloutIdIn);
		ASSERT(NT_SUCCESS(status));

		if (pTcpCtx->transportEndpointHandle != 0)
		{
			tcpctx_release(pTcpCtx);
		}

		tcpctx_release(pTcpCtx);

		sl_lock(&g_slTcpCtx, &lh);
	}
		
	sl_unlock(&lh);
}

void tcpctx_releaseFlows()
{
	KLOCK_QUEUE_HANDLE lh;
	PTCPCTX pTcpCtx;
	
	KdPrint((DPREFIX"tcpctx_releaseFlows\n"));

	sl_lock(&g_slTcpCtx, &lh);

	while (!IsListEmpty(&g_lTcpCtx))
	{
		pTcpCtx = (PTCPCTX)g_lTcpCtx.Flink;

		sl_unlock(&lh);
		
		tcpctx_release(pTcpCtx);
	
		sl_lock(&g_slTcpCtx, &lh);
	}

	sl_unlock(&lh);
}


void tcpctx_free()
{
	KdPrint((DPREFIX"tcpctx_free\n"));

	if (!g_initialized)
		return;

	tcpctx_removeFromFlows();
	
	tcpctx_releaseFlows();

	if (g_phtTcpCtxById)
	{
		hash_table_free(g_phtTcpCtxById);
		g_phtTcpCtxById = NULL;
	}

	if (g_phtTcpCtxByHandle)
	{
		hash_table_free(g_phtTcpCtxByHandle);
		g_phtTcpCtxByHandle = NULL;
	}

	ExDeleteNPagedLookasideList( &g_tcpCtxLAList );
	
	ExDeleteNPagedLookasideList( &g_packetsLAList );

	g_initialized = FALSE;
}

PTCPCTX tcpctx_alloc()
{
	KLOCK_QUEUE_HANDLE lh;
	PTCPCTX pTcpCtx;

	pTcpCtx = (PTCPCTX)ExAllocateFromNPagedLookasideList( &g_tcpCtxLAList );
	if (!pTcpCtx)
		return NULL;

	memset(pTcpCtx, 0, sizeof(TCPCTX));

	sl_init(&pTcpCtx->lock);
	pTcpCtx->refCount = 1;

	InitializeListHead(&pTcpCtx->pendedPackets);
	InitializeListHead(&pTcpCtx->injectPackets);

	sl_lock(&g_slTcpCtx, &lh);
	pTcpCtx->id = g_nextTcpCtxId++;
	ht_add_entry(g_phtTcpCtxById, (PHASH_TABLE_ENTRY)&pTcpCtx->id);
	InsertTailList(&g_lTcpCtx, &pTcpCtx->entry);
	sl_unlock(&lh);

	return pTcpCtx;
}

void tcpctx_addRef(PTCPCTX pTcpCtx)
{
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slTcpCtx, &lh);
	pTcpCtx->refCount++;
	sl_unlock(&lh);
}

void tcpctx_release(PTCPCTX pTcpCtx)
{
	NF_PACKET * pPacket;
	KLOCK_QUEUE_HANDLE lh;

	if (!pTcpCtx)
		return;

	sl_lock(&g_slTcpCtx, &lh);

	if (pTcpCtx->refCount == 0)
	{
		sl_unlock(&lh);
		return;
	}
	
	pTcpCtx->refCount--;

	KdPrint((DPREFIX"tcpctx_release TCPCTX %I64u, refCount=%d\n", pTcpCtx->id, pTcpCtx->refCount));

	if (pTcpCtx->refCount > 0)
	{
		sl_unlock(&lh);
		return;
	}

	KdPrint((DPREFIX"tcpctx_release delete TCPCTX %I64u\n", pTcpCtx->id));

	ht_remove_entry(g_phtTcpCtxById, pTcpCtx->id);
	RemoveEntryList(&pTcpCtx->entry);
	
	sl_unlock(&lh);

	sl_lock(&pTcpCtx->lock, &lh);
	while (!IsListEmpty(&pTcpCtx->pendedPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pTcpCtx->pendedPackets);
		sl_unlock(&lh);
		tcpctx_freePacket(pPacket);
		sl_lock(&pTcpCtx->lock, &lh);
		KdPrint((DPREFIX"tcpctx_release orphan packet in TCPCTX %I64u\n", pTcpCtx->id));
	}
	while (!IsListEmpty(&pTcpCtx->injectPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pTcpCtx->injectPackets);
		sl_unlock(&lh);
		tcpctx_freePacket(pPacket);
		sl_lock(&pTcpCtx->lock, &lh);
		KdPrint((DPREFIX"tcpctx_release orphan inject packet in TCPCTX %I64u\n", pTcpCtx->id));
	}
	sl_unlock(&lh);

	if (pTcpCtx->transportEndpointHandle)
	{
		tcpctx_removeFromHandleTable(pTcpCtx);
		pTcpCtx->transportEndpointHandle = 0;
	}

	if (pTcpCtx->inInjectQueue)
	{
		ASSERT(0);
		KdPrint((DPREFIX"tcpctx_release orphan TCPCTX %I64u in inject queue\n", pTcpCtx->id));
	}

	tcpctx_purgeRedirectInfo(pTcpCtx);

	ExFreeToNPagedLookasideList( &g_tcpCtxLAList, pTcpCtx );
}

PTCPCTX tcpctx_find(HASH_ID id)
{
	PTCPCTX pTcpCtx = NULL;
	PHASH_TABLE_ENTRY phte;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slTcpCtx, &lh);
	phte = ht_find_entry(g_phtTcpCtxById, id);
	if (phte)
	{
		pTcpCtx = (PTCPCTX)CONTAINING_RECORD(phte, TCPCTX, id);
		pTcpCtx->refCount++;
	}
	sl_unlock(&lh);

	return pTcpCtx;
}

PTCPCTX tcpctx_findByHandle(HASH_ID handle)
{
	PTCPCTX pTcpCtx = NULL;
	PHASH_TABLE_ENTRY phte;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slTcpCtx, &lh);
	phte = ht_find_entry(g_phtTcpCtxByHandle, handle);
	if (phte)
	{
		pTcpCtx = (PTCPCTX)CONTAINING_RECORD(phte, TCPCTX, transportEndpointHandle);
		pTcpCtx->refCount++;
	}
	sl_unlock(&lh);

	return pTcpCtx;
}

NTSTATUS tcpctx_pushPacket(PTCPCTX pTcpCtx, FWPS_STREAM_DATA * pStreamData, int code, UINT32 calloutId)
{
	KLOCK_QUEUE_HANDLE lh;
	PNF_PACKET pPacket;
	NTSTATUS status = STATUS_SUCCESS;

	pPacket = tcpctx_allocPacket();
	if (!pPacket)
		return STATUS_NO_MEMORY;

	memset(pPacket, 0, sizeof(NF_PACKET));

	pPacket->calloutId = calloutId;
	pPacket->pTcpCtx = pTcpCtx;
	pPacket->streamData.dataLength = pStreamData->dataLength;
	pPacket->streamData.flags = pStreamData->flags;
	pPacket->isClone = TRUE;

	if (pStreamData->dataLength > 0 &&
		pStreamData->netBufferListChain) 
	{
		status = FwpsCloneStreamData(pStreamData, NULL, NULL, 0, &pPacket->streamData.netBufferListChain);
		if (NT_SUCCESS(status))
		{
			pPacket->streamData.dataOffset.netBufferList = pPacket->streamData.netBufferListChain;
			pPacket->streamData.dataOffset.netBuffer = 
				NET_BUFFER_LIST_FIRST_NB(pPacket->streamData.dataOffset.netBufferList);
			pPacket->streamData.dataOffset.mdl = 
				NET_BUFFER_CURRENT_MDL(pPacket->streamData.dataOffset.netBuffer);
			pPacket->streamData.dataOffset.mdlOffset = 
				NET_BUFFER_CURRENT_MDL_OFFSET(pPacket->streamData.dataOffset.netBuffer);
		} else
		{
			ASSERT(0);
			tcpctx_freePacket(pPacket);
			return STATUS_NO_MEMORY;
		}
	}

	if (code == NF_TCP_SEND)
	{
		status = devctrl_pushTcpData(pTcpCtx->id, NF_TCP_SEND, pTcpCtx, pPacket);

		if (pStreamData->dataLength == 0 &&
			NT_SUCCESS(status))
		{
			sl_lock(&pTcpCtx->lock, &lh);
			pTcpCtx->sendDisconnectPending = TRUE;
			sl_unlock(&lh);
		}
	} else
	if (code == NF_TCP_RECEIVE)
	{
		status = devctrl_pushTcpData(pTcpCtx->id, NF_TCP_RECEIVE, pTcpCtx, pPacket);

		if (pStreamData->dataLength == 0 &&
			NT_SUCCESS(status))
		{
			sl_lock(&pTcpCtx->lock, &lh);
			pTcpCtx->recvDisconnectPending = TRUE;
			sl_unlock(&lh);
		}
	} else
	if (code == NF_TCP_REINJECT)
	{
		status = devctrl_pushTcpData(pTcpCtx->id, NF_TCP_REINJECT, pTcpCtx, pPacket);
	}

	if (!NT_SUCCESS(status))
	{
		tcpctx_freePacket(pPacket);
	}

	return status;
}

PNF_PACKET tcpctx_allocPacket()
{
	PNF_PACKET pPacket;

	pPacket = (PNF_PACKET)ExAllocateFromNPagedLookasideList( &g_packetsLAList );

	KdPrint((DPREFIX"tcpctx_allocPacket %I64x\n", (unsigned __int64)pPacket));

	if (pPacket)
	{
		memset(pPacket, 0, sizeof(NF_PACKET));
	}

	return pPacket;
}

void tcpctx_freePacket(PNF_PACKET pPacket)
{
	KdPrint((DPREFIX"tcpctx_freePacket %I64x\n", (unsigned __int64)pPacket));

	if (pPacket->streamData.netBufferListChain)
	{
		if (pPacket->isClone)
		{
			BOOLEAN isDispatch = (KeGetCurrentIrql() == DISPATCH_LEVEL) ? TRUE : FALSE;
			FwpsDiscardClonedStreamData(pPacket->streamData.netBufferListChain, 0, isDispatch);
		} else
		{
			FwpsFreeNetBufferList(pPacket->streamData.netBufferListChain);

			if (pPacket->streamData.dataOffset.mdl != NULL)
			{
				free_np(pPacket->streamData.dataOffset.mdl->MappedSystemVa);
				IoFreeMdl(pPacket->streamData.dataOffset.mdl);
			}
		}
		pPacket->streamData.netBufferListChain = NULL;
	}
	if (pPacket->flatStreamData)
	{
		free_np(pPacket->flatStreamData);
		pPacket->flatStreamData = NULL;
	}

	ExFreeToNPagedLookasideList( &g_packetsLAList, pPacket );
}

void tcpctx_purgeRedirectInfo(PTCPCTX pTcpCtx)
{
	UINT64 classifyHandle = pTcpCtx->redirectInfo.classifyHandle;
	
	if (classifyHandle)
	{
		pTcpCtx->redirectInfo.classifyHandle = 0;

		if (pTcpCtx->redirectInfo.isPended)
		{
			FwpsCompleteClassify(classifyHandle,
                              0,
                              &pTcpCtx->redirectInfo.classifyOut);

			pTcpCtx->redirectInfo.isPended = FALSE;
		}

#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
		if (pTcpCtx->redirectInfo.redirectHandle)
		{
			FwpsRedirectHandleDestroy(pTcpCtx->redirectInfo.redirectHandle);
			pTcpCtx->redirectInfo.redirectHandle = 0;
		}
#endif
#endif

		FwpsReleaseClassifyHandle(classifyHandle);
	}
}

void tcpctx_addToHandleTable(PTCPCTX pTcpCtx)
{
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slTcpCtx, &lh);
	ht_add_entry(g_phtTcpCtxByHandle, (PHASH_TABLE_ENTRY)&pTcpCtx->transportEndpointHandle);
	sl_unlock(&lh);
}

void tcpctx_removeFromHandleTable(PTCPCTX pTcpCtx)
{
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slTcpCtx, &lh);
	ht_remove_entry(g_phtTcpCtxByHandle, pTcpCtx->transportEndpointHandle);
	sl_unlock(&lh);
}

BOOLEAN tcpctx_isProxy(ULONG processId)
{
	PTCPCTX pConnIn = NULL;
	PTCPCTX pConnOut = NULL;
	sockaddr_gen * pLocalAddrOut = NULL;
	sockaddr_gen * pRemoteAddrOut = NULL;
	sockaddr_gen * pLocalAddrIn = NULL;
	sockaddr_gen * pRemoteAddrIn = NULL;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"tcpctx_isProxy %d\n", processId));

	sl_lock(&g_slTcpCtx, &lh);

	pConnIn = (PTCPCTX)g_lTcpCtx.Flink;
	
	while (pConnIn != (PTCPCTX)&g_lTcpCtx)
	{
		// Try to find an inbound connection with the same processId
		if (pConnIn->processId == processId && pConnIn->direction == NF_D_IN)
		{
			pLocalAddrIn = (sockaddr_gen *)pConnIn->localAddr;
			pRemoteAddrIn = (sockaddr_gen *)pConnIn->remoteAddr;

			pConnOut = (PTCPCTX)g_lTcpCtx.Flink;
			
			while (pConnOut != (PTCPCTX)&g_lTcpCtx)
			{
				// Try to find an appropriate outgoing connection by local address
				if (pConnOut->direction == NF_D_OUT)
				{
					pLocalAddrOut = (sockaddr_gen *)pConnOut->localAddr;
					pRemoteAddrOut = (sockaddr_gen *)pConnOut->remoteAddr;

					if ((pLocalAddrOut->AddressIn.sin_family == pRemoteAddrIn->AddressIn.sin_family) &&
						(pLocalAddrOut->AddressIn.sin_port == pRemoteAddrIn->AddressIn.sin_port))
					{
						// If the connection was redirected from different process,
						// or the original remote address was changed,
						// the process that accepted a connection is a local proxy.
						if ((pConnOut->processId != pConnIn->processId) ||
							(pRemoteAddrOut->AddressIn.sin_port != pLocalAddrIn->AddressIn.sin_port))
						{
							KdPrint((DPREFIX"tcpctx_isProxy %d - is a local proxy\n", processId));
							sl_unlock(&lh);
							return TRUE;
						}
					}
				}

				pConnOut = (PTCPCTX)pConnOut->entry.Flink;
			}
		}
		
		pConnIn = (PTCPCTX)pConnIn->entry.Flink;
	}

	sl_unlock(&lh);

	return FALSE;
}

BOOLEAN tcpctx_setFlowControlHandle(ENDPOINT_ID id, tFlowControlHandle fcHandle)
{
	KLOCK_QUEUE_HANDLE lh;
	PTCPCTX pTcpCtx;
	NF_FLOWCTL_STAT stat;
	
	KdPrint((DPREFIX"tcpctx_setFlowControlHandle[%I64u] fcHandle=%u\n", id, fcHandle));

	if ((fcHandle != 0) &&
		!flowctl_exists(fcHandle))
	{
		KdPrint((DPREFIX"tcpctx_setFlowControlHandle[%I64u]: flowLimit %u not found\n", id, fcHandle));
		return FALSE;
	}

	pTcpCtx = tcpctx_find(id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"tcpctx_setFlowControlHandle[%I64u]: TCPCTX not found\n", id));
		return FALSE;
	}

	sl_lock(&pTcpCtx->lock, &lh);
	
	pTcpCtx->fcHandle = fcHandle;
	pTcpCtx->inLastTS = 0;
	pTcpCtx->outLastTS = 0;

	if (fcHandle != 0)
	{
		stat.inBytes = pTcpCtx->inCounter;
		stat.outBytes = pTcpCtx->outCounter;

		flowctl_updateStat(fcHandle, &stat);
	} else
	{
		pTcpCtx->inCounter = 0;
		pTcpCtx->outCounter = 0;
	}

	sl_unlock(&lh);

	tcpctx_release(pTcpCtx);

	return TRUE;
}

void tcpctx_disableFlowControlHandle(tFlowControlHandle fcHandle)
{
	KLOCK_QUEUE_HANDLE lh;
	PTCPCTX pTcpCtx;
	
	KdPrint((DPREFIX"tcpctx_disableFlowControlHandle\n"));

	sl_lock(&g_slTcpCtx, &lh);

	pTcpCtx = (PTCPCTX)g_lTcpCtx.Flink;
	
	while (pTcpCtx != (PTCPCTX)&g_lTcpCtx)
	{
		if (fcHandle == 0 ||
			pTcpCtx->fcHandle == fcHandle)
		{
			pTcpCtx->fcHandle = 0;
		}
		
		pTcpCtx = (PTCPCTX)pTcpCtx->entry.Flink;
	}

	sl_unlock(&lh);
}


