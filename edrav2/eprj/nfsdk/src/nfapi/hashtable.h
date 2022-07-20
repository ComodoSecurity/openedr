//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _HASHTABLE_H
#define _HASHTABLE_H

namespace nfapi
{
	typedef unsigned __int64 HASH_ID;

	#pragma pack(push,1)

	typedef struct _HASH_TABLE_ENTRY
	{
		HASH_ID		id;
		struct _HASH_TABLE_ENTRY * pNext;
	} HASH_TABLE_ENTRY, *PHASH_TABLE_ENTRY;

	typedef struct _HASH_TABLE
	{
		unsigned int size;
		PHASH_TABLE_ENTRY pEntries[1];
	} HASH_TABLE, *PHASH_TABLE;

	#pragma pack(pop)


	PHASH_TABLE hash_table_new(unsigned int size);

	void hash_table_free(PHASH_TABLE pTable);

	int ht_add_entry(PHASH_TABLE pTable, PHASH_TABLE_ENTRY pEntry);

	PHASH_TABLE_ENTRY ht_find_entry(PHASH_TABLE pTable, HASH_ID id);

	int ht_remove_entry(PHASH_TABLE pTable, HASH_ID id);

}

#endif