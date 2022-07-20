//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "mempools.h"

#define MAX_POOLS 100

typedef struct _MEM_BUFFER
{
	LIST_ENTRY		entry;
	unsigned int	size;
	char			buffer[1];
} MEM_BUFFER;

typedef MEM_BUFFER * PMEM_BUFFER;

typedef struct _MEM_POOL
{
	unsigned int	buffer_size;
	unsigned int	nBuffers;
	LIST_ENTRY		freeBuffers;
} MEM_POOL;

typedef MEM_POOL * PMEM_POOL;

static MEM_POOL		mem_pools[MAX_POOLS];
static int			nPools = 0;
static __SPIN_LOCK	mem_pools_lock;

void mempools_init()
{
	nPools = 0;
	sl_init(&mem_pools_lock);
}

void mempools_free()
{
	PMEM_BUFFER pMemBuffer, pNext;
	PLIST_ENTRY pEntry;
	KIRQL	irql;
	int i;
	
	sl_lock(&mem_pools_lock, &irql);

	for (i=0; i<nPools; i++)
	{
		while (!IsListEmpty(&mem_pools[i].freeBuffers))
		{
			pMemBuffer = (PMEM_BUFFER)RemoveHeadList(&mem_pools[i].freeBuffers);
			free_np(pMemBuffer);
		}
	}

	nPools = 0;

	sl_unlock(&mem_pools_lock, irql);

	sl_free(&mem_pools_lock);
}

/**
* Allocate the buffer of specified size, or get a pre-allocated buffer from pool.
* @return void * 
* @param size Buffer size
**/
void * mp_alloc(unsigned int size)
{
	PMEM_BUFFER pMemBuffer;
	KIRQL		irql;
	int			i;

	if (size == 0)
		return NULL;

	sl_lock(&mem_pools_lock, &irql);

	// Try to find a free buffer with required size
	for (i=0; i<nPools; i++)
	{
		if (mem_pools[i].buffer_size == size)
		{
			if (!IsListEmpty(&mem_pools[i].freeBuffers))
			{
				// Return a buffer from pool
				pMemBuffer = (PMEM_BUFFER)RemoveHeadList(&mem_pools[i].freeBuffers);
				
				mem_pools[i].nBuffers--;
			
				KdPrint(("[MP] mp_alloc: nBuffers[%d] = %d\n", size, mem_pools[i].nBuffers));
				sl_unlock(&mem_pools_lock, irql);
				
				return pMemBuffer->buffer;
			} else
			{
				// Buffer pool is empty
				break;
			}
		}
	}

	sl_unlock(&mem_pools_lock, irql);

	KdPrint(("[MP] mp_alloc: new buffer\n"));

	pMemBuffer = (PMEM_BUFFER)malloc_np(sizeof(MEM_BUFFER) + size - 1);
	if (!pMemBuffer)
	{
		return NULL;
	}
	
	pMemBuffer->size = size;

	return pMemBuffer->buffer;
}

/**
* Return the memory buffer to the pool, or free the buffer if pool is full.
* @return void 
* @param buffer PMEM_BUFFER->buffer
* @param maxPoolSize Maximum number of buffers in the pool
**/
void mp_free(void * buffer, unsigned int maxPoolSize)
{
	PMEM_BUFFER pMemBuffer;
	KIRQL		irql;
	int			i;
	
	if (!buffer)
		return;

	pMemBuffer = (PMEM_BUFFER)((char*)buffer - (char*)(&((PMEM_BUFFER)0)->buffer));

	sl_lock(&mem_pools_lock, &irql);

	for (i=0; i<nPools; i++)
	{
		if (mem_pools[i].buffer_size == pMemBuffer->size)
		{
			if ((maxPoolSize == 0) || (mem_pools[i].nBuffers < maxPoolSize))
			{
				InsertTailList(&mem_pools[i].freeBuffers, &pMemBuffer->entry);

				mem_pools[i].nBuffers++;
				
				KdPrint(("[MP] mp_free: nBuffers[%d] = %d\n", pMemBuffer->size, mem_pools[i].nBuffers));
			} else
			{
				// Pool is full. Add the buffer to tail and free the list head.
				// This ensures that the memory will be accessible for some time
				// after mp_free call.

				InsertTailList(&mem_pools[i].freeBuffers, &pMemBuffer->entry);

				pMemBuffer = (PMEM_BUFFER)RemoveHeadList(&mem_pools[i].freeBuffers);

				free_np(pMemBuffer);

				KdPrint(("[MP] mp_free: pool is full, free buffer\n"));
			}

			sl_unlock(&mem_pools_lock, irql);
			
			return;
		}
	}

	if (nPools < MAX_POOLS)
	{
		// Initialize a new pool

		mem_pools[nPools].buffer_size = pMemBuffer->size;
		
		InitializeListHead(&mem_pools[nPools].freeBuffers);
		
		InsertTailList(&mem_pools[nPools].freeBuffers, &pMemBuffer->entry);

		mem_pools[nPools].nBuffers = 1;

		KdPrint(("[MP] mp_free: nBuffers[%d] = %d\n", pMemBuffer->size, mem_pools[nPools].nBuffers));

		nPools++;
	
		sl_unlock(&mem_pools_lock, irql);
		
		return;
	}

	sl_unlock(&mem_pools_lock, irql);

	free_np(pMemBuffer);

	KdPrint(("[MP] mp_free: free buffer\n"));
}
