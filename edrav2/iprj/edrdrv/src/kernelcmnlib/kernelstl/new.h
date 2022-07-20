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
#pragma once

///
/// The enum class of internal pool types for support POOL_NX_OPTIN.
///
/// For ability passing PoolType as template parameter.
///
enum class PoolType
{
	NonPaged,
	Paged,
	NonPagedExecute,
	NonPagedNx,
};

#if POOL_NX_OPTIN && !POOL_NX_OPTOUT
#define _constexpr_for_convetrToSystemPoolType
#else // POOL_NX_OPTIN && !POOL_NX_OPTOUT
#define _constexpr_for_convetrToSystemPoolType constexpr
#endif // POOL_NX_OPTIN && !POOL_NX_OPTOUT

///
/// Converts PoolType -> POOL_TYPE.
///
/// If POOL_NX_OPTIN is not enable, it is constexpr.
///
/// @param ePoolType - a type of the pool.
/// @return The function returns a POOL_TYPE value.
///
inline _constexpr_for_convetrToSystemPoolType POOL_TYPE convetrToSystemPoolType(PoolType ePoolType)
{
	switch (ePoolType)
	{
	case PoolType::NonPaged:
		return NonPagedPool;
	case PoolType::Paged:
		return PagedPool;
	case PoolType::NonPagedExecute:
		return NonPagedPoolExecute;
	case PoolType::NonPagedNx:
		return NonPagedPoolNx;
	default:
		return MaxPoolType;
	}
}



// placement new
void* __cdecl operator new (size_t, void* p);
// placement new
void* __cdecl operator new[](size_t, void* p);
// placement delete
void __cdecl operator delete (void*, void*);
// placement delete
void __cdecl operator delete[](void*, void*);


// allocation new
_When_((ePoolType& NonPagedPoolMustSucceed) != 0,
	__drv_reportError("Must succeed pool allocations are forbidden. "
		"Allocation failures cause a system crash"))
void* __cdecl operator new (size_t nSize, POOL_TYPE ePoolType);
// allocation new
_When_((ePoolType& NonPagedPoolMustSucceed) != 0,
	__drv_reportError("Must succeed pool allocations are forbidden. "
		"Allocation failures cause a system crash"))
void* __cdecl operator new[] (size_t nSize, POOL_TYPE ePoolType);
// allocation new
void* __cdecl operator new (size_t nSize, PoolType ePoolType);
// allocation new
void* __cdecl operator new[](size_t nSize, PoolType ePoolType);
// allocation delete
void __cdecl operator delete (void* pVoid);

namespace cmd {

/// Allocation tag
/// for ExAllocatePoolWithTag
constexpr ULONG c_nAllocTag = 'erdd';

}

/// @}

