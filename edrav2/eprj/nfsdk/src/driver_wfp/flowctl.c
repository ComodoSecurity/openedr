//
// 	NetFilterSDK 
// 	Copyright (C) 2017 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "nfdriver.h"
#include "flowctl.h"
#include "tcpctx.h"
#include "udpctx.h"

#ifdef _WPPTRACE
#include "flowctl.tmh"
#endif

#define MAX_FLOW_CTL_HANDLES	1024 * 1024

#define FLOW_LIMIT_TIMEOUT (1 * 10 * 1000 * 1000)
#define FLOW_LIMIT_MAX_TIMEOUT (10 * 10 * 1000 * 1000)

typedef struct _NF_FLOW_CTL
{
	LIST_ENTRY	entry;

	tFlowControlHandle	id;
	PHASH_TABLE_ENTRY	id_next;

	uint64_t	inLimit;
	uint64_t	outLimit;

	uint64_t	inBucket;
	uint64_t	outBucket;

	uint64_t	inCounter;
	uint64_t	outCounter;

	uint64_t	ts;
} NF_FLOW_CTL, *PNF_FLOW_CTL;

static PHASH_TABLE	g_phtFlowCtl = NULL;
static LIST_ENTRY g_lFlowCtl;
static NPAGED_LOOKASIDE_LIST g_flowCtlLAList;
static KSPIN_LOCK g_sl;
static tFlowControlHandle g_nextFlowCtlId;
static tFlowControlHandle g_nFlowCtl;

uint64_t flowctl_getTickCount()
{
	LARGE_INTEGER li;

	KeQuerySystemTime(&li);

	return li.QuadPart;
}

BOOLEAN flowctl_init()
{
	KdPrint((DPREFIX"flowctl_init\n"));

	ExInitializeNPagedLookasideList( &g_flowCtlLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_FLOW_CTL ),
									 MEM_TAG,
                                     0 );

	InitializeListHead(&g_lFlowCtl);
	
	KeInitializeSpinLock(&g_sl);

	g_nextFlowCtlId = 1;
	g_nFlowCtl = 0;

	g_phtFlowCtl = hash_table_new(DEFAULT_HASH_SIZE);
	if (!g_phtFlowCtl)
	{
		return FALSE;
	}

	return TRUE;
}

void flowctl_free()
{
	KLOCK_QUEUE_HANDLE lh;
	PNF_FLOW_CTL pFlowCtl;
	
	KdPrint((DPREFIX"flowctl_free\n"));

	sl_lock(&g_sl, &lh);

	while (!IsListEmpty(&g_lFlowCtl))
	{
		pFlowCtl = (PNF_FLOW_CTL)RemoveHeadList(&g_lFlowCtl);

		ht_remove_entry(g_phtFlowCtl, pFlowCtl->id);
		
		ExFreeToNPagedLookasideList( &g_flowCtlLAList, pFlowCtl );		
	}

	sl_unlock(&lh);

	if (g_phtFlowCtl)
	{
		hash_table_free(g_phtFlowCtl);
		g_phtFlowCtl = NULL;
	}

	ExDeleteNPagedLookasideList( &g_flowCtlLAList );
}

BOOLEAN flowctl_exists(tFlowControlHandle fcHandle)
{
	KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result;

	sl_lock(&g_sl, &lh);	
	
	if (fcHandle == 0)
	{
		result = (g_nFlowCtl > 0);
	} else
	{
		if (ht_find_entry(g_phtFlowCtl, fcHandle))
		{
			result = TRUE;
		} else
		{
			result = FALSE;
		}
	}
	
	sl_unlock(&lh);	

	return result;
}


tFlowControlHandle flowctl_add(uint64_t inLimit, uint64_t outLimit)
{
    KLOCK_QUEUE_HANDLE lh;
	PNF_FLOW_CTL pFlowCtl = NULL;
	tFlowControlHandle id = 0;

	KdPrint((DPREFIX"flowctl_add inLimit=%I64u, outLimit=%I64u\n", inLimit, outLimit));

	sl_lock(&g_sl, &lh);
	if (g_nFlowCtl > MAX_FLOW_CTL_HANDLES)
	{
		sl_unlock(&lh);	
		return 0;
	}
	sl_unlock(&lh);	

	pFlowCtl = (PNF_FLOW_CTL)ExAllocateFromNPagedLookasideList( &g_flowCtlLAList );
	if (!pFlowCtl)
		return 0;

	memset(pFlowCtl, 0, sizeof(NF_FLOW_CTL));
	pFlowCtl->inLimit = inLimit;
	pFlowCtl->outLimit = outLimit;

	sl_lock(&g_sl, &lh);

	id = g_nextFlowCtlId++;
	pFlowCtl->id = id;
	
	ht_add_entry(g_phtFlowCtl, (PHASH_TABLE_ENTRY)&pFlowCtl->id);
	
	InsertTailList(&g_lFlowCtl, &pFlowCtl->entry);

	g_nFlowCtl++;

	sl_unlock(&lh);	

	return id;
}

BOOLEAN flowctl_modifyLimits(tFlowControlHandle fcHandle, uint64_t inLimit, uint64_t outLimit)
{
	PNF_FLOW_CTL pFlowCtl = NULL;
	PHASH_TABLE_ENTRY phte;
    KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result;

	KdPrint((DPREFIX"flowctl_update inLimit=%I64u, outLimit=%I64u\n", inLimit, outLimit));

	sl_lock(&g_sl, &lh);	

	phte = ht_find_entry(g_phtFlowCtl, fcHandle);
	if (phte)
	{
		pFlowCtl = (PNF_FLOW_CTL)CONTAINING_RECORD(phte, NF_FLOW_CTL, id);

		pFlowCtl->inLimit = inLimit;
		pFlowCtl->outLimit = outLimit;

		result = TRUE;
	} else
	{
		result = FALSE;
	}
	
	sl_unlock(&lh);	

	return TRUE;
}

BOOLEAN flowctl_delete(tFlowControlHandle fcHandle)
{
	PNF_FLOW_CTL pFlowCtl = NULL;
	PHASH_TABLE_ENTRY phte;
    KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result;

	KdPrint((DPREFIX"flowctl_delete id=%u\n", fcHandle));

	if (fcHandle == 0)
	{
		sl_lock(&g_sl, &lh);	
		while (!IsListEmpty(&g_lFlowCtl))
		{
			pFlowCtl = (PNF_FLOW_CTL)RemoveHeadList(&g_lFlowCtl);

			ht_remove_entry(g_phtFlowCtl, pFlowCtl->id);
		
			ExFreeToNPagedLookasideList( &g_flowCtlLAList, pFlowCtl );		
		}
		sl_unlock(&lh);	

		tcpctx_disableFlowControlHandle(0);
		udpctx_disableFlowControlHandle(0);

		g_nFlowCtl = 0;

		return TRUE;
	} 

    sl_lock(&g_sl, &lh);	

	phte = ht_find_entry(g_phtFlowCtl, fcHandle);
	if (phte)
	{
		pFlowCtl = (PNF_FLOW_CTL)CONTAINING_RECORD(phte, NF_FLOW_CTL, id);

		ht_remove_entry(g_phtFlowCtl, pFlowCtl->id);
		RemoveEntryList(&pFlowCtl->entry);
		
		ExFreeToNPagedLookasideList( &g_flowCtlLAList, pFlowCtl );		

		g_nFlowCtl--;

		result = TRUE;
	} else
	{
		result = FALSE;
	}

	sl_unlock(&lh);	

	tcpctx_disableFlowControlHandle(fcHandle);
	udpctx_disableFlowControlHandle(fcHandle);

	return result;
}

BOOLEAN flowctl_getStat(tFlowControlHandle fcHandle, PNF_FLOWCTL_STAT pStat)
{
	PNF_FLOW_CTL pFlowCtl = NULL;
	PHASH_TABLE_ENTRY phte;
    KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result;

	KdPrint((DPREFIX"flowctl_getStat %u\n", fcHandle));

	sl_lock(&g_sl, &lh);	

	phte = ht_find_entry(g_phtFlowCtl, fcHandle);
	if (phte)
	{
		pFlowCtl = (PNF_FLOW_CTL)CONTAINING_RECORD(phte, NF_FLOW_CTL, id);

		pStat->inBytes = pFlowCtl->inCounter;
		pStat->outBytes = pFlowCtl->outCounter;

		result = TRUE;
	} else
	{
		result = FALSE;
	}

	sl_unlock(&lh);	

	return result;
}

BOOLEAN flowctl_updateStat(tFlowControlHandle fcHandle, PNF_FLOWCTL_STAT pStat)
{
	PNF_FLOW_CTL pFlowCtl = NULL;
	PHASH_TABLE_ENTRY phte;
    KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result;

	KdPrint((DPREFIX"flowctl_setStat %u\n", fcHandle));

	sl_lock(&g_sl, &lh);	

	phte = ht_find_entry(g_phtFlowCtl, fcHandle);
	if (phte)
	{
		pFlowCtl = (PNF_FLOW_CTL)CONTAINING_RECORD(phte, NF_FLOW_CTL, id);

		pFlowCtl->inCounter += pStat->inBytes;
		pFlowCtl->outCounter += pStat->outBytes;

		result = TRUE;
	} else
	{
		result = FALSE;
	}

	sl_unlock(&lh);	

	return result;
}

static void flowctl_updateInternal(PNF_FLOW_CTL pFlowCtl)
{
 	uint64_t	ts;
	uint64_t	nBytes;

	ts = flowctl_getTickCount();

	if ((ts - pFlowCtl->ts) > 0)
	{
		if (pFlowCtl->outLimit > 0)
		{
			nBytes = (pFlowCtl->outLimit * (ts - pFlowCtl->ts)) / (10 * 1000 * 1000);

			KdPrint((DPREFIX"flowctl_updateInternal[%d] out, outBucket=%I64u, nBytes=%I64u\n", 
				pFlowCtl->id, pFlowCtl->outBucket, nBytes));
			
			if (pFlowCtl->outBucket > nBytes)
			{
				pFlowCtl->outBucket -= nBytes;
			} else
			{
				pFlowCtl->outBucket = 0;
			}
		}

		if (pFlowCtl->inLimit > 0)
		{
			nBytes = (pFlowCtl->inLimit * (ts - pFlowCtl->ts)) / (10 * 1000 * 1000);
			
			KdPrint((DPREFIX"flowctl_updateInternal[%d] in, inBucket=%I64u, nBytes=%I64u\n", 
				pFlowCtl->id, pFlowCtl->inBucket, nBytes));

			if (pFlowCtl->inBucket > nBytes)
			{
				pFlowCtl->inBucket -= nBytes;
			} else
			{
				pFlowCtl->inBucket = 0;
			}
		}

		pFlowCtl->ts = ts;
	}

}

BOOLEAN flowctl_mustSuspendInternal(PNF_FLOW_CTL pFlowCtl, BOOLEAN isOut, uint64_t lastIoTime)
{
 	uint64_t	ts;

	ts = flowctl_getTickCount();

	if ((ts - lastIoTime) > FLOW_LIMIT_MAX_TIMEOUT)
	{
		return FALSE;
	}

	flowctl_updateInternal(pFlowCtl);

	if (isOut)
	{
		if ((pFlowCtl->outLimit > 0) && (pFlowCtl->outBucket > pFlowCtl->outLimit))
			return TRUE;
	} else
	{
		if ((pFlowCtl->inLimit > 0) && (pFlowCtl->inBucket > pFlowCtl->inLimit))
			return TRUE;
	}

	return FALSE;
}

BOOLEAN flowctl_mustSuspend(tFlowControlHandle fcHandle, BOOLEAN isOut, uint64_t lastIoTime)
{
	PNF_FLOW_CTL pFlowCtl = NULL;
	PHASH_TABLE_ENTRY phte;
    KLOCK_QUEUE_HANDLE lh;
	BOOLEAN result;

	sl_lock(&g_sl, &lh);	

	phte = ht_find_entry(g_phtFlowCtl, fcHandle);
	if (phte)
	{
		pFlowCtl = (PNF_FLOW_CTL)CONTAINING_RECORD(phte, NF_FLOW_CTL, id);

		result = flowctl_mustSuspendInternal(pFlowCtl, isOut, lastIoTime);
	} else
	{
		result = FALSE;
	}

	sl_unlock(&lh);	

	KdPrint((DPREFIX"flowctl_mustSuspend [%d], isOut=%d, result=%d\n", 
		fcHandle, isOut, result));

	return result;
}

void flowctl_update(tFlowControlHandle fcHandle, BOOLEAN isOut, uint64_t nBytes)
{
	PNF_FLOW_CTL pFlowCtl = NULL;
	PHASH_TABLE_ENTRY phte;
    KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"flowctl_update [%d], isOut=%d, nBytes=%I64u\n", 
		fcHandle, isOut, nBytes));

    sl_lock(&g_sl, &lh);	

	phte = ht_find_entry(g_phtFlowCtl, fcHandle);
	if (phte)
	{
		pFlowCtl = (PNF_FLOW_CTL)CONTAINING_RECORD(phte, NF_FLOW_CTL, id);

		if (isOut)
		{
			if (pFlowCtl->outLimit > 0)
			{
				flowctl_updateInternal(pFlowCtl);
				pFlowCtl->outBucket += nBytes;
			}

			pFlowCtl->outCounter += nBytes;
		} else
		{
			if (pFlowCtl->inLimit > 0)
			{
				flowctl_updateInternal(pFlowCtl);
				pFlowCtl->inBucket += nBytes;
			}

			pFlowCtl->inCounter += nBytes;
		}
	}

	sl_unlock(&lh);	
}