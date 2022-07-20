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
#include "hashtable.h"

using namespace nfapi;

PHASH_TABLE nfapi::hash_table_new(unsigned int size)
{
	unsigned int memsize;
	PHASH_TABLE pTable;

	if (size < 1)
		return NULL;

	memsize = sizeof(HASH_TABLE) + sizeof(PHASH_TABLE_ENTRY) * (size - 1);

	pTable = (PHASH_TABLE)malloc_np(memsize);
	if (!pTable)
		return NULL;
	
	memset(pTable, 0, memsize);

	pTable->size = size;

	return pTable;
}

void nfapi::hash_table_free(PHASH_TABLE pTable)
{
	if (pTable)
	{
		free_np(pTable);
	}
}

int nfapi::ht_add_entry(PHASH_TABLE pTable, PHASH_TABLE_ENTRY pEntry)
{
	HASH_ID hash = pEntry->id % pTable->size;

	if (ht_find_entry(pTable, pEntry->id))
		return 0;

	pEntry->pNext = pTable->pEntries[hash];
	pTable->pEntries[hash] = pEntry;

	return 1;
}


PHASH_TABLE_ENTRY nfapi::ht_find_entry(PHASH_TABLE pTable, HASH_ID id)
{
	PHASH_TABLE_ENTRY pEntry;

	pEntry = pTable->pEntries[id % pTable->size];

	while (pEntry)
	{
		if (pEntry->id == id)
		{
			return pEntry;
		}

		pEntry = pEntry->pNext;
	}

	return NULL;
}

int nfapi::ht_remove_entry(PHASH_TABLE pTable, HASH_ID id)
{
	PHASH_TABLE_ENTRY pEntry, * ppNext;

	ppNext = &pTable->pEntries[id % pTable->size];
	pEntry = *ppNext;

	while (pEntry)
	{
		if (pEntry->id == id)
		{
			*ppNext = pEntry->pNext;
			return 1;
		}

		ppNext = &pEntry->pNext;
		pEntry = *ppNext;
	}

	return 0;
}
