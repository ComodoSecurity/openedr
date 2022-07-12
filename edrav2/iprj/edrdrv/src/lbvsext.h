//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Lbvs extension
///
/// @addtogroup edrdrv
/// @{
#pragma once

#include "log.h"
#include "kernelcmnlib.hpp"

#define CMD_KERNEL_MODE
typedef uint64_t Enum;
// External libraries
#include <libcore\inc\variant\lbvs.hpp>


namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// 
//

//
// Kernel mode allocator for LbvsSerializer
//
template<PoolType c_ePoolType>
struct LbvsAllocator
{
	void* allocMem(size_t nSize)
	{
		return ExAllocatePoolWithTag(convetrToSystemPoolType(c_ePoolType), nSize, c_nAllocTag);
	}

	void freeMem(void* pPtr)
	{
		ExFreePoolWithTag(pPtr, c_nAllocTag);
	}
};

///
/// LbvsSerializer with paged pool.
///
template<typename IdType, size_t c_nBufferSize = 0x100 /* 256 byte */, 
	size_t c_nLimitSize = 100*0x400*0x400 /*100 mb*/ >
using PagedLbvsSerializer = variant::BasicLbvsSerializer<IdType, 
	LbvsAllocator<PoolType::Paged>, c_nBufferSize, c_nLimitSize>;

///
/// LbvsSerializer with non paged pool.
///
template<typename IdType, size_t c_nBufferSize = 0x100 /* 256 byte */, 
	size_t c_nLimitSize = 100 * 0x400 * 0x400 /*100 mb*/ >
using NonPagedLbvsSerializer = variant::BasicLbvsSerializer<IdType, 
	LbvsAllocator<PoolType::NonPaged>, c_nBufferSize, c_nLimitSize>;

///
/// Writes UNICODE_STRING into LbvsSerializer.
///
/// @param serializer - TBD.
/// @param eFieldId - TBD.
/// @param pusStr - TBD.
/// @return The function returns a boolean status of the operation.
///
template<typename IdType, typename MemAllocator, size_t c_nBufferSize, size_t c_nLimitSize>
[[nodiscard]] bool write(variant::BasicLbvsSerializer<IdType, MemAllocator, c_nBufferSize,
	c_nLimitSize>& serializer, IdType eFieldId, PCUNICODE_STRING pusStr)
{
	if (pusStr == nullptr) return false;

	const wchar_t* psBegin = pusStr->Buffer;
	size_t nSize = (size_t)pusStr->Length / sizeof(wchar_t);
	// Validate size (find internal zero)
	const wchar_t* psEnd = psBegin + nSize;
	for (const wchar_t* psCur = psBegin; psCur < psEnd; ++psCur)
		if (*psCur == 0)
		{
			nSize = psCur - psBegin;
			break;
		}

	return serializer.write(eFieldId, psBegin, nSize);
}

//
// Proxy to BasicLbvsDeserializer::getValue
//
template<typename IdType, typename StorageType>
[[nodiscard]] NTSTATUS getValue(const variant::BasicLbvsDeserializerField<IdType>& item, StorageType& storage, bool fLogError = true)
{
	if (!item.getValue(storage))
	{
		if (!fLogError)
			return STATUS_INVALID_PARAMETER;
		return LOGERROR(STATUS_INVALID_PARAMETER, "Invalid lbvs field: %u, type: %u.\r\n",
			(ULONG)item.id, (ULONG)item.type);
	}
	return STATUS_SUCCESS;
}

//
// Adapter of BasicLbvsDeserializer::getValue for AtomicInt
//
template<typename IdType, typename T>
[[nodiscard]] NTSTATUS getValue(const variant::BasicLbvsDeserializerField<IdType>& item, AtomicInt<T>& storage, bool fLogError = true)
{
	T value;
	IFERR_RET_NOLOG(getValue(item, value, fLogError));
	storage = value;
	return STATUS_SUCCESS;
}

//
// Adapter of BasicLbvsDeserializer::getValue for AtomicBool
//
template<typename IdType>
[[nodiscard]] NTSTATUS getValue(const variant::BasicLbvsDeserializerField<IdType>& item, AtomicBool& storage, bool fLogError = true)
{
	bool value;
	IFERR_RET_NOLOG(getValue(item, value, fLogError));
	storage = value;
	return STATUS_SUCCESS;
}


//
// Adapter of BasicLbvsDeserializer::getValue for UNICODE_STRING
//
template<typename IdType>
[[nodiscard]] NTSTATUS getValue(const variant::BasicLbvsDeserializerField<IdType>& item, UNICODE_STRING& storage, bool fLogError = true)
{
	const wchar_t* pStr;
	IFERR_RET_NOLOG(getValue(item, pStr, fLogError));
	RtlInitUnicodeString(&storage, pStr);
	return STATUS_SUCCESS;
}

//
// Adapter of BasicLbvsDeserializer::getValue for DynUnicodeString
//
template<typename IdType>
[[nodiscard]] NTSTATUS getValue(const variant::BasicLbvsDeserializerField<IdType>& item, DynUnicodeString& storage, bool fLogError = true)
{
	UNICODE_STRING usData = {};
	IFERR_RET_NOLOG(getValue(item, usData, fLogError));
	NTSTATUS eResult = storage.assign(&usData);
	if (NT_SUCCESS(eResult))
		return STATUS_SUCCESS;
	if (!fLogError)
		return eResult;
	return LOGERROR(eResult, "Invalid config field: %u, type: %u.\r\n", (ULONG)item.id, (ULONG)item.type);
}

//
// Adapter of BasicLbvsDeserializer::getValue for BOOLEAN
//
template<typename IdType>
[[nodiscard]] NTSTATUS getValue(const variant::BasicLbvsDeserializerField<IdType>& item, BOOLEAN& storage, bool fLogError = true)
{
	bool m_fValue;
	IFERR_RET_NOLOG(getValue(item, m_fValue, fLogError));
	storage = m_fValue;
	return STATUS_SUCCESS;
}

//
// Adapter of BasicLbvsDeserializer::getValue for enum
//
template<typename IdType, typename EnumType>
[[nodiscard]] NTSTATUS getEnumValue(const variant::BasicLbvsDeserializerField<IdType>& item, EnumType& storage, bool fLogError = true)
{
	uint64_t nValue;
	IFERR_RET_NOLOG(getValue(item, nValue, fLogError));
	storage = EnumType(nValue);
	return STATUS_SUCCESS;
}


//
// Update list according to eMode
// Note: src can be changed. (items are moved from it)
//
template<typename ListData>
void updateList(edrdrv::UpdateRulesMode eMode, List<ListData>& dst, List<ListData>& src)
{
	switch (eMode)
	{
	case edrdrv::UpdateRulesMode::Clear:
		dst.clear();
		break;
	case edrdrv::UpdateRulesMode::PushBack:
		dst.spliceBack(src);
		break;
	case edrdrv::UpdateRulesMode::PushFront:
		src.spliceBack(dst);
		dst = std::move(src);
		break;
	case edrdrv::UpdateRulesMode::Replace:
		dst = std::move(src);
		break;
	}
}

} // namespace cmd

/// @}
