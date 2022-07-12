//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdafx.h"
#include "stdinc.h"
#include "mempools.h"

using namespace nfapi;

#define MAX_POOLS		100
#define MAX_POOL_SIZE	100

#pragma pack(push,1)

typedef struct _MEM_BUFFER
{
	struct _MEM_BUFFER * pNext;
	unsigned int size;
	char buffer[1];
} MEM_BUFFER, *PMEM_BUFFER;

typedef struct _MEM_POOL
{
	unsigned int buffer_size;
	unsigned int nBuffers;
	PMEM_BUFFER pFreeBuffers;
} MEM_POOL, *PMEM_POOL;

#pragma pack(pop)

static MEM_POOL mem_pools[MAX_POOLS];
static int nPools = 0;
static __SPIN_LOCK mem_pools_lock;

void nfapi::mempools_init()
{
	nPools = 0;
	sl_init(&mem_pools_lock);
}

void nfapi::mempools_free()
{
	PMEM_BUFFER pMemBuffer, pNext;
	int i;
	
	sl_lock(&mem_pools_lock);

	for (i=0; i<nPools; i++)
	{
		if (mem_pools[i].pFreeBuffers)
		{
			pMemBuffer = mem_pools[i].pFreeBuffers; 
			
			while (pMemBuffer != NULL)
			{
				pNext = pMemBuffer->pNext;
				free_np(pMemBuffer);
				pMemBuffer = pNext;
			}
		}
	}

	nPools = 0;

	sl_unlock(&mem_pools_lock);

	sl_free(&mem_pools_lock);
}

void * nfapi::mp_alloc(unsigned int size, int align)
{
	PMEM_BUFFER pMemBuffer;
	int			i;

	if (size == 0)
		return NULL;

	if (align > 0)
	{
		size = ((size / align) + 1) * align;
	}

	sl_lock(&mem_pools_lock);

	for (i=0; i<nPools; i++)
	{
		if (mem_pools[i].buffer_size == size)
		{
			if (mem_pools[i].pFreeBuffers)
			{
				pMemBuffer = mem_pools[i].pFreeBuffers;
				mem_pools[i].pFreeBuffers = pMemBuffer->pNext;
				mem_pools[i].nBuffers--;

				sl_unlock(&mem_pools_lock);
				return pMemBuffer->buffer;
			} else
			{
				break;
			}
		}
	}

	sl_unlock(&mem_pools_lock);

	pMemBuffer = (PMEM_BUFFER)malloc_np(sizeof(MEM_BUFFER) + size - 1);
	if (!pMemBuffer)
	{
		return NULL;
	}
	
	pMemBuffer->size = size;

	return pMemBuffer->buffer;
}

void nfapi::mp_free(void * buffer, unsigned int maxPoolSize)
{
	PMEM_BUFFER pMemBuffer;
	int	i;
	
	if (!buffer)
		return;

	pMemBuffer = (PMEM_BUFFER)((char*)buffer - (char*)(&((PMEM_BUFFER)0)->buffer));

	sl_lock(&mem_pools_lock);

	for (i=0; i<nPools; i++)
	{
		if (mem_pools[i].buffer_size == pMemBuffer->size)
		{
			if ((maxPoolSize == 0) || (mem_pools[i].nBuffers < maxPoolSize))
			{
				if (mem_pools[i].nBuffers < MAX_POOL_SIZE)
				{
					pMemBuffer->pNext = mem_pools[i].pFreeBuffers;
					mem_pools[i].pFreeBuffers = pMemBuffer;
					mem_pools[i].nBuffers++;
				} else
				{
					free_np(pMemBuffer);
				}
			} else
			{
				free_np(pMemBuffer);
			}

			sl_unlock(&mem_pools_lock);

			return;
		}
	}

	if (nPools < MAX_POOLS)
	{
		mem_pools[nPools].buffer_size = pMemBuffer->size;
		mem_pools[nPools].pFreeBuffers = pMemBuffer;
		pMemBuffer->pNext = NULL;

		mem_pools[nPools].nBuffers = 1;
	
		nPools++;
	
		sl_unlock(&mem_pools_lock);
		return;
	}

	sl_unlock(&mem_pools_lock);

	free_np(pMemBuffer);
}
