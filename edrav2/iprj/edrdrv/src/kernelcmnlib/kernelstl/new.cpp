//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (19.03.2019)
// Reviewer: 
//
///
/// @file Crypto utils (hash, etc.)
///
/// @addtogroup edrdrv
/// @{

// Standard header has 4-th level warnings in C++
#pragma warning(push, 3)
#include <wdm.h>
#pragma warning(pop)

#include "new.h"

//
// placement new
//
void* __cdecl operator new (size_t, void * p)
{ 
	return p; 
}

//
// placement new
//
void* __cdecl operator new[](size_t, void * p)
{
	return p; 
}

//
// placement delete
//
void __cdecl operator delete (void *, void *)
{
}

//
// placement delete
//
void __cdecl operator delete[](void *, void *)
{
}

//
// allocation new
//
_Use_decl_annotations_
void* __cdecl operator new (size_t nSize, POOL_TYPE ePoolType)
{
	nSize = (nSize != 0) ? nSize : 1;
	PVOID result = ExAllocatePoolWithTag(ePoolType, nSize, cmd::c_nAllocTag);
	if (result)
		RtlZeroMemory(result, nSize);
	return result;
}

//
// allocation new
//
_Use_decl_annotations_
void* __cdecl operator new[] (size_t nSize, POOL_TYPE ePoolType)
{
	nSize = (nSize != 0) ? nSize : 1;
	PVOID result = ExAllocatePoolWithTag(ePoolType, nSize, cmd::c_nAllocTag);
	if (result)
		RtlZeroMemory(result, nSize);
	return result;
}

//
// allocation new
//
void* __cdecl operator new (size_t nSize, PoolType ePoolType)
{
	nSize = (nSize != 0) ? nSize : 1;
	PVOID result = ExAllocatePoolWithTag(convetrToSystemPoolType(ePoolType), nSize, cmd::c_nAllocTag);
	if (result)
		RtlZeroMemory(result, nSize);
	return result;
}

//
// allocation new
//
void* __cdecl operator new[] (size_t nSize, PoolType ePoolType)
{
	nSize = (nSize != 0) ? nSize : 1;
	PVOID result = ExAllocatePoolWithTag(convetrToSystemPoolType(ePoolType), nSize, cmd::c_nAllocTag);
	if (result)
		RtlZeroMemory(result, nSize);
	return result;
}


//
// allocation delete
//
void __cdecl operator delete (void* pObject)
{
	if (pObject)
		ExFreePool(pObject);
}

void __cdecl operator delete[](void* pObject)
{
	if (pObject != NULL)
		ExFreePool(pObject);
}

//
// size_t version is needed for VS2015(C++ 14).  
// 
void __cdecl operator delete(void* pObject, size_t /*s*/)
{
	::operator delete(pObject);
}

//
// size_t version is needed for VS2015(C++ 14).  
// 
void __cdecl operator delete[](void* pObject, size_t /*s*/)
{
	::operator delete[](pObject);
}

/// @}
