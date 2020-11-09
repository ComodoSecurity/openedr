//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file String Utils
///
/// @addtoroup edrdrv
/// @{
#pragma once
#include "common.h"
#include <Ntstrsafe.h>

namespace openEdr {


namespace detail {

//
//
//
inline const wchar_t* wmemchr(const wchar_t* str, size_t strLenCh, wchar_t ch)
{
	const wchar_t* pEnd = str + strLenCh;
	for (const wchar_t* pCur = str; pCur < pEnd; ++pCur)
		if (*pCur == ch)
			return pCur;
	return nullptr;
}

//
//
//
inline const wchar_t* wmemmem(const wchar_t* str, size_t strLenCh, const wchar_t *subStr, size_t subStrLenCh)
{
	if (subStrLenCh == 0)
		return str;

	if (strLenCh < subStrLenCh)
		return nullptr;

	if (subStrLenCh == 1)
		return wmemchr(str, strLenCh, *subStr);

	const wchar_t* pEnd = str + strLenCh - subStrLenCh + 1;
	for (const wchar_t* pCur = str; pCur < pEnd; ++pCur)
		if (pCur[0] == subStr[0] && memcmp(pCur, subStr, subStrLenCh*2) == 0)
			return pCur;

	return nullptr;
}

}



//////////////////////////////////////////////////////////////////////////
//
// UNICODE_STRING
//
//////////////////////////////////////////////////////////////////////////

///
/// Declare UNICODE_STRING constant.
///
/// @param str - wchar_t string literal.
/// @see RTL_CONSTANT_STRING
///
#define U_STAT RTL_CONSTANT_STRING  

///
/// Declare UNICODE_STRING variable with initialization.
///
/// @param _symbol - variable name.
/// @param _buffer - wchar_t string literal.
///
#define PRESET_UNICODE_STRING(_symbol,_buffer)	\
	UNICODE_STRING _symbol = RTL_CONSTANT_STRING (_buffer);

///
/// Declare UNICODE_STRING static variable with initialization.
///
/// @param _symbol - variable name.
/// @param _buffer - wchar_t string literal.
///
#define PRESET_STATIC_UNICODE_STRING(_symbol,_buffer)	\
	static UNICODE_STRING _symbol = RTL_CONSTANT_STRING (_buffer);


// +FIXME: May we create string namespace here? And delete Str postfix in functions...

///
/// Creates a substring.
///
/// @param usStr - TBD.
/// @param nStartPosCb - TBD.
/// @param nSizeCb - TBD.
/// @return The function returns a UNICODE string.
///
inline UNICODE_STRING sliceStrCb(const UNICODE_STRING& usStr, size_t nStartPosCb, size_t nSizeCb)
{
	// Verification
	const size_t nStrLenCb = usStr.Length;
	if (nStartPosCb >= nStrLenCb)
		return usStr;

	const auto nNewSizeCb = (USHORT)min((nStrLenCb - nStartPosCb), nSizeCb);
	return UNICODE_STRING{ nNewSizeCb, nNewSizeCb, (wchar_t*) (((char*)usStr.Buffer) + nStartPosCb)};
}

///
/// Creates a substring.
///
/// @param usStr - TBD.
/// @param nStartPosCb - TBD.
/// @param nSizeCb - TBD.
/// @return The function returns a UNICODE string.
///
inline UNICODE_STRING sliceStrCch(const UNICODE_STRING& usStr, size_t nStartPosCch, size_t nSizeCch)
{
	return sliceStrCb(usStr, nStartPosCch / sizeof(wchar_t), nSizeCch / sizeof(wchar_t));
}

///
/// Compares zero-end ANSI string.
///
/// @param pStr1 - string #1.
/// @param pStr2 - string #2.
/// @param fCaseInSensitive - TBD.
/// @return The function returns `TRUE` if strings are equal.
///
// +FIXME: Why we can't use standard c++ types7 for Boolean?
//
inline BOOLEAN isEqualStr(const char* pStr1, const char* pStr2, BOOLEAN fCaseInSensitive)
{
	// +FIXME: Are two NULL strings equal?
	// !It is logical error
	if (pStr1 == nullptr) return false;
	if (pStr2 == nullptr) return false;

	STRING Str1;
	RtlInitAnsiString(&Str1, pStr1);

	STRING Str2;
	RtlInitAnsiString(&Str2, pStr2);

	return RtlEqualString(&Str1, &Str2, fCaseInSensitive);
}

///
/// Compares zero-end WCHAR string.
///
/// @param pStr1 - string #1.
/// @param pStr2 - string #2.
/// @param fCaseInSensitive - TBD.
/// @return The function returns `TRUE` if strings are equal.
///
inline BOOLEAN isEqualStr(PCWCHAR pStr1, PCWCHAR pStr2, BOOLEAN fCaseInSensitive)
{
	if (pStr1 == nullptr) return false;
	if (pStr2 == nullptr) return false;

	UNICODE_STRING Str1;
	RtlInitUnicodeString(&Str1, pStr1);

	UNICODE_STRING Str2;
	RtlInitUnicodeString(&Str2, pStr2);

	return RtlEqualUnicodeString(&Str1, &Str2, fCaseInSensitive);
}


///
/// Checking of string prefix (case insensitive).
///
/// @param pusString[in] - source string.
/// @param pusStartWith[in] - prefix for checking.
/// @return TRUE if pusString has prefix.
///
inline BOOLEAN isStartWith(PCUNICODE_STRING pusString, PCUNICODE_STRING pusStartWith)
{
	return RtlPrefixUnicodeString(pusStartWith, pusString, TRUE);
}

///
/// Checking of string prefix (case insensitive).
///
/// @param pusString[in] - source string.
/// @param pStr[in] - prefix for checking.
/// @return TRUE if pusString has prefix.
///
inline BOOLEAN isStartWith(PCUNICODE_STRING pusString, PWCHAR pStr)
{
	UNICODE_STRING uPref;
	RtlInitUnicodeString(&uPref, pStr);

	return isStartWith(pusString, &uPref);
}

//
//
//
inline void truncateLeft(PUNICODE_STRING pString, USHORT nSize)
{
	ASSERT(nSize % sizeof(WCHAR) == 0);
	ASSERT(pString != NULL);
	ASSERT(pString->Length >= nSize);

	pString->Buffer += (nSize / sizeof(WCHAR));
	pString->Length -= nSize;
	pString->MaximumLength -= nSize;
}

///
/// Checking of string postfix (case insensitive).
///
/// @param pusString[in] - source string.
/// @param pusPostfix[in] - postfix for checking.
/// @return TRUE if pusString has postfix.
///
inline BOOLEAN isEndedWith(PCUNICODE_STRING pusString, PCUNICODE_STRING pusPostfix)
{
	if (!pusString || !pusPostfix)
		return FALSE;
	if (pusString->Length < pusPostfix->Length)
		return FALSE;

	UNICODE_STRING uTemp = *pusString;
	if(uTemp.Length != pusPostfix->Length)
		truncateLeft(&uTemp, uTemp.Length - pusPostfix->Length);

	return RtlEqualUnicodeString(&uTemp, pusPostfix, TRUE);
}

///
/// Checking of string postfix (case insensitive).
///
/// @param pusString[in] - source string.
/// @param sPostfix[in] - postfix for checking.
/// @return TRUE if pusString has postfix.
///
inline BOOLEAN isEndedWith(PCUNICODE_STRING uPath, const WCHAR* sPostfix)
{
	UNICODE_STRING uPostfix;
	RtlInitUnicodeString(&uPostfix, sPostfix);

	return isEndedWith(uPath, &uPostfix);
}

///
/// wcsstr analog.
///
/// @param usStr - TBD.
/// @param usStrToFind - TBD.
/// @param nStartPosCch [opt] - TBD. (default: 0).
/// @return The function returns a position in wchar_t or SIZE_MAX if not found.
///
inline size_t findSubStrCch(PCUNICODE_STRING usStr, PCUNICODE_STRING usStrToFind, size_t nStartPosCch=0)
{
	const size_t nStrSizeCh = usStr->Length / sizeof(wchar_t);
	if (nStartPosCch >= nStrSizeCh)
		return SIZE_MAX;

	auto pResult = detail::wmemmem(usStr->Buffer + nStartPosCch, nStrSizeCh - nStartPosCch,
		usStrToFind->Buffer, usStrToFind->Length / sizeof(wchar_t));

	if (pResult == nullptr)
		return SIZE_MAX;
	return pResult - usStr->Buffer;
}

///
/// wcsstr analog.
///
/// @param usStr - TBD.
/// @param usStrToFind - TBD.
/// @param nStartPosCb [opt] - TBD. (default: 0).
/// @return The function returns a position in bytes or SIZE_MAX if not found.
///
inline size_t findSubStrCb(PCUNICODE_STRING usStr, PCUNICODE_STRING usStrToFind, size_t nStartPosCb = 0)
{
	const auto result = findSubStrCch(usStr, usStrToFind, nStartPosCb/sizeof(wchar_t));
	return result == SIZE_MAX ? SIZE_MAX : result * sizeof(wchar_t);
}

///
/// wcschr analog.
///
/// @param usStr - TBD.
/// @param ch - TBD.
/// @param nStartPosCch [opt] - TBD. (default: 0).
/// @return The function returns a position in wchar_t or SIZE_MAX if not found.
///
inline size_t findChrCch(PCUNICODE_STRING usStr, wchar_t ch, size_t nStartPosCch = 0)
{
	const size_t nStrSizeCh = usStr->Length / sizeof(wchar_t);
	if (nStartPosCch >= nStrSizeCh)
		return SIZE_MAX;

	auto pResult = detail::wmemchr(usStr->Buffer + nStartPosCch, nStrSizeCh - nStartPosCch, ch);
	if (pResult == nullptr)
		return SIZE_MAX;
	return pResult - usStr->Buffer;
}

///
/// wcschr analog.
///
/// @param usStr - TBD.
/// @param ch - TBD.
/// @param nStartPosCb - TBD. (default: 0).
/// @return The function returns a position in bytes or SIZE_MAX if not found.
///
inline size_t findChrCb(PCUNICODE_STRING usStr, wchar_t ch, size_t nStartPosCb = 0)
{
	const auto result = findChrCch(usStr, ch, nStartPosCb / sizeof(wchar_t));
	return result == SIZE_MAX ? SIZE_MAX : result * sizeof(WCHAR);
}

///
/// Replace a substring.
///
/// @param usDst - TBD.
/// @param usSubStr - TBD.
/// @param nReplacePosCb - TBD.
/// @param nReplaceSizeCb - TBD.
/// @return The function returns a boolean status.
///
inline bool replaceSubStrCb(PUNICODE_STRING usDst, PCUNICODE_STRING usSubStr, size_t nReplacePosCb, size_t nReplaceSizeCb)
{
	// Verification
	const size_t nDstLenCb = usDst->Length;
	const size_t nSubStrLenCb = usSubStr->Length;
	char* pDstBuffer = (char*)usDst->Buffer;

	// Check replace pos
	if (nDstLenCb < nReplacePosCb)
		return false;
	if (nDstLenCb - nReplacePosCb < nReplaceSizeCb)
		return false;

	// Check new size
	const size_t nNewDstSize = nDstLenCb - nReplaceSizeCb + nSubStrLenCb;
	if (nNewDstSize > (size_t) usDst->MaximumLength)
		return false;

	// move tail
	const size_t nTailOldPos = nReplacePosCb + nReplaceSizeCb;
	size_t nTailNewPos = nReplacePosCb + nSubStrLenCb;
	const size_t nTailSize = nDstLenCb - nTailOldPos;
	if (nTailOldPos != nTailNewPos && nTailSize != 0)
		memmove(pDstBuffer + nTailNewPos, pDstBuffer + nTailOldPos, nTailSize);
	
	// Copy substr
	if (nSubStrLenCb != 0)
		memcpy(pDstBuffer + nReplacePosCb, usSubStr->Buffer, nSubStrLenCb);

	usDst->Length = (USHORT)nNewDstSize;
	return true;
}

///
/// Cut a string.
///
/// @param usDst - TBD.
/// @param nPosCb - TBD.
/// @param nSizeCb - TBD.
///
inline void cutStrCb(PUNICODE_STRING usDst, size_t nPosCb, size_t nSizeCb)
{
	// Verification
	const size_t nDstLenCb = usDst->Length;
	char* pDstBuffer = (char*)usDst->Buffer;

	// out of size
	if (nDstLenCb <= nPosCb)
		return;

	// Cut tail
	if (nDstLenCb - nPosCb <= nSizeCb)
	{
		usDst->Length = (USHORT)nPosCb;
		return;
	}

	// Move tail
	size_t nTailNewPos = nPosCb;
	const size_t nTailOldPos = nPosCb + nSizeCb;
	const size_t nTailSize = nDstLenCb - nTailOldPos;
	memmove(pDstBuffer + nTailNewPos, pDstBuffer + nTailOldPos, nTailSize);

	usDst->Length = (USHORT)(nTailNewPos+nTailSize);
}


///
/// Duplicate a UNICODE_STRING info new allocated buffer.
///
/// Buffer is returned and must be free by ExFreePoolWithTag(, c_nAllocTag).
///
/// @param pSrc[in] - source UNICODE_STRING.
/// @param pDst[in] - destination UNICODE_STRING.
/// @param ppBuffer[out] - allocated buffer.
/// @return The function returns a NTSTATUS of the operation.
///
inline NTSTATUS cloneUnicodeString(PCUNICODE_STRING pSrc, UNICODE_STRING* pDst, PVOID* ppBuffer)
{
	const USHORT nStrBufferSize = pSrc->Length + sizeof(WCHAR);
	// FIXME: We shall check pDst parameter!
	// 
	// +FIXME: Just an advice: Make allocMem function and hide realization....
	// Then we just remove c_nAllocTag. But it is more readable.
	PVOID pStrBuffer = ExAllocatePoolWithTag(NonPagedPool, nStrBufferSize, c_nAllocTag);
	if (pStrBuffer == nullptr) { return LOGERROR(STATUS_NO_MEMORY); }

	memcpy(pStrBuffer, pSrc->Buffer, pSrc->Length);
	// Add zero end
	((UINT8*)pStrBuffer)[pSrc->Length] = 0;
	((UINT8*)pStrBuffer)[pSrc->Length+1] = 0;

	pDst->Buffer = (PWCH)pStrBuffer;
	pDst->Length = pSrc->Length;
	pDst->MaximumLength = nStrBufferSize;

	*ppBuffer = pStrBuffer;
	return STATUS_SUCCESS;
}

///
/// Dynamic allocated unicode string.
///
class DynUnicodeString
{
private:
	UNICODE_STRING m_us = {0,0,nullptr};
	UniquePtr<void, FreeDeleter> m_pBuffer;

public:
	DynUnicodeString() = default;
	~DynUnicodeString() = default;
	DynUnicodeString(const DynUnicodeString&) = delete;
	DynUnicodeString& operator=(const DynUnicodeString&) = delete;

	//
	//
	//
	DynUnicodeString(DynUnicodeString&& other)
	{
		*this = std::move(other);
	}

	//
	//
	//
	DynUnicodeString& operator=(DynUnicodeString&& other)
	{
		m_pBuffer = std::move(other.m_pBuffer);
		m_us = std::move(other.m_us);
		other.clear();
		return *this;
	}

	//
	// Compares strings and returns
	//   < 0 if less,
	//   == 0 if equal,
	//   > 0 if more,
	//
	int compare(PCUNICODE_STRING other, bool fCaseInSensitive) const
	{
		return (int) RtlCompareUnicodeString(*this, other, fCaseInSensitive);
	}

	//
	// Compares strings
	//
	bool isEqual(PCUNICODE_STRING other, bool fCaseInSensitive) const
	{
		return compare(other, fCaseInSensitive) == 0;
	}

	//
	// Attention: Don't define operator==(), use isEqual.
	// because operator PCUNICODE_STRING() is called and pointers are compared
	// instead operator==(PCUNICODE_STRING).
	//
	// bool operator==(PCUNICODE_STRING other) const
	//

	//
	// Alloc empty string
	//
	NTSTATUS alloc(size_t nMaxSizeCb, POOL_TYPE ePoolType = NonPagedPool)
	{
		PVOID pStrBuffer = ExAllocatePoolWithTag(ePoolType, nMaxSizeCb, c_nAllocTag);
		if (pStrBuffer == nullptr)
			return STATUS_NO_MEMORY;

		UNICODE_STRING usRes;
		usRes.Buffer = (PWCH)pStrBuffer;
		usRes.Length = 0;
		constexpr size_t c_nUshortMax = 0xFFFF;
		usRes.MaximumLength = nMaxSizeCb <= c_nUshortMax ? (USHORT)nMaxSizeCb : c_nUshortMax;

		assign(pStrBuffer, usRes);
		return STATUS_SUCCESS;
	}

	//
	//
	//
	NTSTATUS assign(PCUNICODE_STRING pusSrc, POOL_TYPE ePoolType = NonPagedPool)
	{
		IFERR_RET_NOLOG(alloc(pusSrc->Length + sizeof(wchar_t), ePoolType));
		RtlCopyUnicodeString(&m_us, pusSrc);
		// Add zero end
		if(m_us.Length < m_us.MaximumLength)
			m_us.Buffer[(((size_t)m_us.Length)+1) / sizeof(wchar_t)] = 0;
		return STATUS_SUCCESS;
	}

	//
	//
	//
	void assign(void* pBuffer, UNICODE_STRING usSrc)
	{
		m_pBuffer.reset(pBuffer);
		m_us = usSrc;
	}

	//
	//
	//
	void clear()
	{
		m_us = UNICODE_STRING{ 0,0,nullptr };
		m_pBuffer.reset();
	}

	//
	//
	//
	size_t getLengthCb() const
	{
		return m_us.Length;
	}

	//
	//
	//
	wchar_t getCharCch(size_t nIndex) const
	{
		return m_us.Buffer[nIndex];
	}

	//
	//
	//
	wchar_t getCharCb(size_t nIndex) const
	{
		return m_us.Buffer[nIndex/sizeof(wchar_t)];
	}

	//
	//
	//
	const wchar_t* getBuffer() const
	{
		return m_us.Buffer;
	}

	//
	//
	//
	bool isEmpty() const
	{
		return m_us.Length == 0;
	}

	//
	//
	//
	operator UNICODE_STRING() const
	{
		return m_us;
	}

	//
	//
	//
	operator PCUNICODE_STRING() const
	{
		return &m_us;
	}

	//
	// Convert to non-const is explicit only
	//
	explicit operator PUNICODE_STRING()
	{
		return &m_us;
	}
};

///
/// Converts to lower case.
///
/// Allocs new string.
///
/// @param usSrc - TBD.
/// @param usDst - TBD.
/// @return The function returns a NTSTATUS of the operation.
///
inline NTSTATUS convertToLowerCase(PCUNICODE_STRING usSrc, DynUnicodeString& usDst)
{
	DynUnicodeString usResult;
	IFERR_RET_NOLOG(usResult.alloc((((size_t)usSrc->Length) + sizeof(wchar_t) ) * 11/10 /*reserve 10% */));
	IFERR_RET_NOLOG(RtlDowncaseUnicodeString((PUNICODE_STRING)usResult, usSrc, FALSE));
	usDst = std::move(usResult);
	return STATUS_SUCCESS;
}

///
/// Converts to upper case.
///
/// Allocs new string.
///
/// @param usSrc - TBD.
/// @param usDst - TBD.
/// @return The function returns a NTSTATUS of the operation.
///
inline NTSTATUS convertToUpperCase(PCUNICODE_STRING usSrc, DynUnicodeString& usDst)
{
	DynUnicodeString usResult;
	IFERR_RET_NOLOG(usResult.alloc((((size_t)usSrc->Length) + sizeof(wchar_t)) * 11 / 10 /*reserve 10% */));
	IFERR_RET_NOLOG(RtlUpcaseUnicodeString((PUNICODE_STRING)usResult, usSrc, FALSE));
	usDst = std::move(usResult);
	return STATUS_SUCCESS;
}


///
/// Allocate UNICODE_STRING and its data buffer in one memory block
/// After usage new string must be free with ExFreePoolWithTag(, c_nAllocTag)
///
/// @param nSizeCb - data bufer size
/// @param ePoolType - Pool type for ExAllocatePoolWithTag
/// @return nullptr if error occured
///

//
// convert int (0-15) to hex digit
//
inline char getHexDigit(UINT8 nByte)
{
	if (nByte < 10)
		return nByte + '0';
	return nByte - 10  + 'A';
}


///
/// Convert binary data into hex ansi string.
///
/// @param pSrc - source data.
/// @param nSrcSize - source size.
/// @param psDst - destination buffer.
/// @param nDstSize - destination buffer size.
///
inline void convertBinToHex(const void* pSrc, size_t nSrcSize, char* psDst, size_t nDstSize)
{
	ASSERT(nDstSize >= nSrcSize * 2 + 1);

	// If buffer is very small, returns "".
	if (nDstSize < 2/*one byte*/ + 1 /*zero end*/)
	{
		if (nDstSize != 0)
			psDst[0] = 0;
		return;
	}

	size_t nConvertSize = min(nSrcSize, (nDstSize - 1) / 2);
	auto pBytes = (const UINT8*)pSrc;

	for (size_t i = 0; i < nConvertSize; ++i)
	{
		const UINT8 nByte = pBytes[i];
		psDst[i * 2] = getHexDigit(nByte >> 4);
		psDst[i * 2 + 1] = getHexDigit(nByte & 0x0F);
	}
	psDst[nConvertSize * 2] = '\0';
}

} // namespace openEdr
/// @}
