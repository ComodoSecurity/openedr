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
#include "hashtable.h"

PHASH_TABLE hash_table_new(unsigned int size)
{
	unsigned int memsize;
	PHASH_TABLE pTable;

	if (size < 1)
		return NULL;

	memsize = sizeof(HASH_TABLE) + sizeof(PHASH_TABLE_ENTRY) * (size - 1);

	pTable = malloc_np(memsize);
	if (!pTable)
		return NULL;
	
	memset(pTable, 0, memsize);

	pTable->size = size;

	return pTable;
}

void hash_table_free(PHASH_TABLE pTable)
{
	if (pTable)
	{
		free_np(pTable);
	}
}

int ht_add_entry(PHASH_TABLE pTable, PHASH_TABLE_ENTRY pEntry)
{
	HASH_ID hash = pEntry->id % pTable->size;

	if (ht_find_entry(pTable, pEntry->id))
		return 0;

	pEntry->pNext = pTable->pEntries[hash];
	pTable->pEntries[hash] = pEntry;

	return 1;
}


PHASH_TABLE_ENTRY ht_find_entry(PHASH_TABLE pTable, HASH_ID id)
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

int ht_remove_entry(PHASH_TABLE pTable, HASH_ID id)
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
