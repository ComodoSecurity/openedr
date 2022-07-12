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
#include "udpctx.h"
#include "devctrl.h"

#ifdef _WPPTRACE
#include "udpctx.tmh"
#endif

static LIST_ENTRY	g_lUdpCtx;
static KSPIN_LOCK	g_slUdpCtx;

static NPAGED_LOOKASIDE_LIST g_udpCtxLAList;
static PHASH_TABLE	g_phtUdpCtxById = NULL;
static PHASH_TABLE	g_phtUdpCtxByHandle = NULL;
static ENDPOINT_ID	g_nextUdpCtxId;

static NPAGED_LOOKASIDE_LIST g_udpPacketsLAList;

static BOOLEAN		g_initialized = FALSE;

BOOLEAN udpctx_init()
{
	g_phtUdpCtxById = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_phtUdpCtxById)
	{
		return FALSE;
	}

	g_phtUdpCtxByHandle = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_phtUdpCtxByHandle)
	{
		return FALSE;
	}

	ExInitializeNPagedLookasideList( &g_udpCtxLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( UDPCTX ),
                                     MEM_TAG_UDP,
                                     0 );

    ExInitializeNPagedLookasideList( &g_udpPacketsLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_UDP_PACKET ),
									 MEM_TAG_UDP_PACKET,
                                     0 );

	InitializeListHead(&g_lUdpCtx);
	sl_init(&g_slUdpCtx);

	g_nextUdpCtxId = 1;

	g_initialized = TRUE;
	
	return TRUE;
}

void udpctx_cleanupFlows()
{
	KLOCK_QUEUE_HANDLE lh, lhd;
	PUDPCTX pUdpCtx;
	NF_UDP_PACKET * pPacket;
	
	KdPrint((DPREFIX"udpctx_cleanupFlows\n"));

	sl_lock(&g_slUdpCtx, &lh);

	pUdpCtx = (PUDPCTX)g_lUdpCtx.Flink;
	
	while (pUdpCtx != (PUDPCTX)&g_lUdpCtx)
	{
		sl_lock(&pUdpCtx->lock, &lhd);
		while (!IsListEmpty(&pUdpCtx->pendedSendPackets))
		{
			pPacket = (PNF_UDP_PACKET)RemoveHeadList(&pUdpCtx->pendedSendPackets);
			udpctx_freePacket(pPacket);
			KdPrint((DPREFIX"udpctx_cleanupFlows orphan send packet in UDPCTX %I64u\n", pUdpCtx->id));
		}
		while (!IsListEmpty(&pUdpCtx->pendedRecvPackets))
		{
			pPacket = (PNF_UDP_PACKET)RemoveHeadList(&pUdpCtx->pendedRecvPackets);
			udpctx_freePacket(pPacket);
			KdPrint((DPREFIX"udpctx_cleanupFlows orphan recv packet in UDPCTX %I64u\n", pUdpCtx->id));
		}
		sl_unlock(&lhd);
	
		pUdpCtx = (PUDPCTX)pUdpCtx->entry.Flink;
	}

	sl_unlock(&lh);
}

void udpctx_postCreateEvents()
{
	KLOCK_QUEUE_HANDLE lh;
	PUDPCTX pUdpCtx;
	LIST_ENTRY ctxList;
	PLIST_ENTRY pEntry;
	
	KdPrint((DPREFIX"udpctx_postCreateEvents\n"));

	InitializeListHead(&ctxList);

	sl_lock(&g_slUdpCtx, &lh);

	pUdpCtx = (PUDPCTX)g_lUdpCtx.Flink;
	
	while (pUdpCtx != (PUDPCTX)&g_lUdpCtx)
	{
		pUdpCtx->refCount++;
		InsertTailList(&ctxList, &pUdpCtx->auxEntry);
		pUdpCtx = (PUDPCTX)pUdpCtx->entry.Flink;
	}

	sl_unlock(&lh);

	while (!IsListEmpty(&ctxList))
	{
		pEntry = RemoveHeadList(&ctxList);
		pUdpCtx = (PUDPCTX)CONTAINING_RECORD(pEntry, UDPCTX, auxEntry);
		devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CREATED, pUdpCtx, NULL);
		udpctx_release(pUdpCtx);
	}
}

void udpctx_releaseFlows()
{
	KLOCK_QUEUE_HANDLE lh;
	PUDPCTX pUdpCtx;
	
	KdPrint((DPREFIX"udpctx_releaseFlows\n"));

	sl_lock(&g_slUdpCtx, &lh);

	while (!IsListEmpty(&g_lUdpCtx))
	{
		pUdpCtx = (PUDPCTX)g_lUdpCtx.Flink;

		sl_unlock(&lh);
		
		udpctx_release(pUdpCtx);
	
		sl_lock(&g_slUdpCtx, &lh);
	}

	sl_unlock(&lh);
}

void udpctx_free()
{
	KdPrint((DPREFIX"udpctx_free\n"));

	if (!g_initialized)
		return;

	udpctx_releaseFlows();

	if (g_phtUdpCtxById)
	{
		hash_table_free(g_phtUdpCtxById);
		g_phtUdpCtxById = NULL;
	}

	if (g_phtUdpCtxByHandle)
	{
		hash_table_free(g_phtUdpCtxByHandle);
		g_phtUdpCtxByHandle = NULL;
	}

	ExDeleteNPagedLookasideList( &g_udpCtxLAList );
	
	ExDeleteNPagedLookasideList( &g_udpPacketsLAList );

	g_initialized = FALSE;
}

PUDPCTX udpctx_alloc(UINT64 transportEndpointHandle)
{
	KLOCK_QUEUE_HANDLE lh;
	PUDPCTX pUdpCtx;

	pUdpCtx = (PUDPCTX)ExAllocateFromNPagedLookasideList( &g_udpCtxLAList );
	if (!pUdpCtx)
		return NULL;

	memset(pUdpCtx, 0, sizeof(UDPCTX));

	sl_init(&pUdpCtx->lock);
	pUdpCtx->refCount = 2;

	InitializeListHead(&pUdpCtx->pendedSendPackets);
	InitializeListHead(&pUdpCtx->pendedRecvPackets);

	if (transportEndpointHandle)
	{
		pUdpCtx->transportEndpointHandle = transportEndpointHandle;

		sl_lock(&g_slUdpCtx, &lh);

		pUdpCtx->id = g_nextUdpCtxId++;

		KdPrint((DPREFIX"udpctx_alloc id=[%I64u]\n", pUdpCtx->id));

		ht_add_entry(g_phtUdpCtxByHandle, (PHASH_TABLE_ENTRY)&pUdpCtx->transportEndpointHandle);
		ht_add_entry(g_phtUdpCtxById, (PHASH_TABLE_ENTRY)&pUdpCtx->id);

		InsertTailList(&g_lUdpCtx, &pUdpCtx->entry);

		sl_unlock(&lh);
	} else
	{
		pUdpCtx->refCount--;
	}

	return pUdpCtx;
}

void udpctx_addRef(PUDPCTX pUdpCtx)
{
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slUdpCtx, &lh);
	pUdpCtx->refCount++;
	sl_unlock(&lh);

	KdPrint((DPREFIX"udpctx_addRef[%I64u] %d\n", pUdpCtx->id, pUdpCtx->refCount));
}

void udpctx_release(PUDPCTX pUdpCtx)
{
	NF_UDP_PACKET * pPacket;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"udpctx_release[%I64u] %d\n", pUdpCtx->id, pUdpCtx->refCount));

	sl_lock(&g_slUdpCtx, &lh);

	if (pUdpCtx->refCount == 0)
	{
		sl_unlock(&lh);
		return;
	}
	
	pUdpCtx->refCount--;

	if (pUdpCtx->refCount > 0)
	{
		sl_unlock(&lh);
		return;
	}

	KdPrint((DPREFIX"udpctx_release delete UDPCTX %I64u\n", pUdpCtx->id));

	if (pUdpCtx->transportEndpointHandle)
	{
		ht_remove_entry(g_phtUdpCtxById, pUdpCtx->id);
		ht_remove_entry(g_phtUdpCtxByHandle, pUdpCtx->transportEndpointHandle);

		RemoveEntryList(&pUdpCtx->entry);
	}

	sl_unlock(&lh);

	sl_lock(&pUdpCtx->lock, &lh);
	while (!IsListEmpty(&pUdpCtx->pendedSendPackets))
	{
		pPacket = (PNF_UDP_PACKET)RemoveHeadList(&pUdpCtx->pendedSendPackets);
		sl_unlock(&lh);
		udpctx_freePacket(pPacket);
		sl_lock(&pUdpCtx->lock, &lh);
		KdPrint((DPREFIX"udpctx_release orphan send packet in UDPCTX %I64u\n", pUdpCtx->id));
	}
	while (!IsListEmpty(&pUdpCtx->pendedRecvPackets))
	{
		pPacket = (PNF_UDP_PACKET)RemoveHeadList(&pUdpCtx->pendedRecvPackets);
		sl_unlock(&lh);
		udpctx_freePacket(pPacket);
		sl_lock(&pUdpCtx->lock, &lh);
		KdPrint((DPREFIX"udpctx_release orphan recv packet in UDPCTX %I64u\n", pUdpCtx->id));
	}
	sl_unlock(&lh);

	udpctx_purgeRedirectInfo(pUdpCtx);

	ExFreeToNPagedLookasideList( &g_udpCtxLAList, pUdpCtx );
}

PUDPCTX udpctx_find(HASH_ID id)
{
	PUDPCTX pUdpCtx = NULL;
	PHASH_TABLE_ENTRY phte;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slUdpCtx, &lh);
	phte = ht_find_entry(g_phtUdpCtxById, id);
	if (phte)
	{
		pUdpCtx = (PUDPCTX)CONTAINING_RECORD(phte, UDPCTX, id);
		pUdpCtx->refCount++;
	}
	sl_unlock(&lh);

	return pUdpCtx;
}

PNF_UDP_PACKET udpctx_allocPacket(ULONG dataLength)
{
	PNF_UDP_PACKET pPacket;

	pPacket = (PNF_UDP_PACKET)ExAllocateFromNPagedLookasideList( &g_udpPacketsLAList );
	if (!pPacket)
		return NULL;

	memset(pPacket, 0, sizeof(NF_UDP_PACKET));

	if (dataLength > 0)
	{
		pPacket->dataBuffer = (char*)ExAllocatePoolWithTag(NonPagedPool, dataLength, MEM_TAG_UDP_DATA_COPY);
		if (!pPacket->dataBuffer)
		{
			ExFreeToNPagedLookasideList( &g_udpPacketsLAList, pPacket );
			return NULL;
		}
	}

	return pPacket;
}

void udpctx_freePacket(PNF_UDP_PACKET pPacket)
{
	if (pPacket->dataBuffer)
	{
		free_np(pPacket->dataBuffer);
		pPacket->dataBuffer = NULL;
	}
	if (pPacket->controlData && pPacket->options.controlDataLength)
	{
		free_np(pPacket->controlData);
		pPacket->controlData = NULL;
	}
	ExFreeToNPagedLookasideList( &g_udpPacketsLAList, pPacket );
}

void udpctx_purgeRedirectInfo(PUDPCTX pUdpCtx)
{
	UINT64 classifyHandle = pUdpCtx->redirectInfo.classifyHandle;

	if (classifyHandle)
	{
		pUdpCtx->redirectInfo.classifyHandle = 0;

		if (pUdpCtx->redirectInfo.isPended)
		{
			FwpsCompleteClassify(classifyHandle,
                              0,
                              &pUdpCtx->redirectInfo.classifyOut);

			pUdpCtx->redirectInfo.isPended = FALSE;
		}

#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
		if (pUdpCtx->redirectInfo.redirectHandle)
		{
			FwpsRedirectHandleDestroy(pUdpCtx->redirectInfo.redirectHandle);
			pUdpCtx->redirectInfo.redirectHandle = 0;
		}
#endif
#endif

		FwpsReleaseClassifyHandle(classifyHandle);
	}
}

PUDPCTX udpctx_findByHandle(HASH_ID handle)
{
	PUDPCTX pUdpCtx = NULL;
	PHASH_TABLE_ENTRY phte;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slUdpCtx, &lh);
	phte = ht_find_entry(g_phtUdpCtxByHandle, handle);
	if (phte)
	{
		pUdpCtx = (PUDPCTX)CONTAINING_RECORD(phte, UDPCTX, transportEndpointHandle);
		pUdpCtx->refCount++;
	}
	sl_unlock(&lh);

	return pUdpCtx;
}

BOOLEAN udpctx_setFlowControlHandle(ENDPOINT_ID id, tFlowControlHandle fcHandle)
{
	KLOCK_QUEUE_HANDLE lh;
	PUDPCTX pUdpCtx;
	NF_FLOWCTL_STAT stat;
	
	KdPrint((DPREFIX"udpctx_setFlowControlHandle[%I64u] fcHandle=%u\n", id, fcHandle));

	if ((fcHandle != 0) &&
		!flowctl_exists(fcHandle))
	{
		KdPrint((DPREFIX"udpctx_setFlowControlHandle[%I64u]: fcHandle %u not found\n", id, fcHandle));
		return FALSE;
	}

	pUdpCtx = udpctx_find(id);
	if (!pUdpCtx)
	{
		KdPrint((DPREFIX"udpctx_setFlowControlHandle[%I64u]: UDPCTX not found\n", id));
		return FALSE;
	}

	sl_lock(&pUdpCtx->lock, &lh);
	
	pUdpCtx->fcHandle = fcHandle;
	pUdpCtx->inLastTS = 0;
	pUdpCtx->outLastTS = 0;

	if (fcHandle != 0)
	{
		stat.inBytes = pUdpCtx->inCounter;
		stat.outBytes = pUdpCtx->outCounter;

		flowctl_updateStat(fcHandle, &stat);
	} else
	{
		pUdpCtx->inCounter = 0;
		pUdpCtx->outCounter = 0;
	}
	sl_unlock(&lh);

	udpctx_release(pUdpCtx);

	return TRUE;
}

void udpctx_disableFlowControlHandle(tFlowControlHandle fcHandle)
{
	KLOCK_QUEUE_HANDLE lh;
	PUDPCTX pUdpCtx;
	
	KdPrint((DPREFIX"udpctx_disableFlowControlHandle fcHandle=%u\n", fcHandle));

	sl_lock(&g_slUdpCtx, &lh);

	pUdpCtx = (PUDPCTX)g_lUdpCtx.Flink;
	
	while (pUdpCtx != (PUDPCTX)&g_lUdpCtx)
	{
		if (fcHandle == 0 ||
			pUdpCtx->fcHandle == fcHandle)
		{
			pUdpCtx->fcHandle = 0;
		}
		
		pUdpCtx = (PUDPCTX)pUdpCtx->entry.Flink;
	}

	sl_unlock(&lh);
}

