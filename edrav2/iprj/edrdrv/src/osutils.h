//
// EDRAv2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Common Utils
///
/// @addtogroup edrdrv
/// @{
#pragma once
#include "common.h"

namespace openEdr {

///
/// Maximum size of SID in string format.
///
/// In characters, including zero end.
///
// https://groups.google.com/forum/#!msg/microsoft.public.dotnet.security/NpIi7c2Toi8/31SVhcepY58J
inline constexpr size_t c_nMaxStringSidSizeCh = 200;

///
/// Kernel DeviceIoOperation.
///
/// @see DeviceIoOperation in WinAPI
///
NTSTATUS callKernelDeviceIoControl(PDEVICE_OBJECT pDevObj, ULONG ControlCode, 
	PVOID pInBuf, ULONG InBufLen, PVOID pOutBuf, ULONG OutBufLen);

///
/// Check OS Version.
///
/// Wrapper of RtlVerifyVersionInfo.
///
/// @param dwMajorVersion - major OS version.
/// @param dwMinorVersion - minor OS version.
/// @return The function returns a result of RtlVerifyVersionInfo call.
///
NTSTATUS checkOSVersionSupport(ULONG dwMajorVersion, ULONG dwMinorVersion);

///
/// Returns user name from token.
///
/// @param[in] hToken - access token.
/// @param[out] pusUserName - buffer for username 
///	  (recommended size: c_nMaxStringSidSizeCh)
/// @return The function returns a NTSTATUS of the operation.
/// 
NTSTATUS getTokenSid(HANDLE hToken, PUNICODE_STRING pusUserName);

///
/// Returns token elevation info.
///
/// @param[in] hToken - access token.
/// @param[out] pfElevated - TBD.
/// @return The function returns a NTSTATUS of the operation.
/// 
NTSTATUS checkTokenElevation(HANDLE hToken, bool* pfElevated);

///
/// Returns token elevation info.
///
/// @param[in] hToken - access token.
/// @param[out] peElevationType - TBD.
/// @return The function returns a NTSTATUS of the operation.
/// 
NTSTATUS getTokenElevationType(HANDLE hToken, TOKEN_ELEVATION_TYPE* peElevationType);

///
/// Kernel mode analog of GetTickCount64.
///
/// @return The function returns a value in milliseconds since system boot.
///
inline uint64_t getTickCount64()
{
	const ULONG64 nTime = KeQueryInterruptTime();
	return nTime / ((ULONG64)10 /*100 nano*/ * (ULONG64)1000 /*micro*/);
}

///
/// Kernel mode analog of GetSystemTimeAsFileTime.
///
/// @return The function retruns a UNIX-time in milliseconds.
///
inline uint64_t getSystemTime()
{
	// TODO: Possibly we must to use KeQuerySystemTimePrecise
	// Need to check system impact because of KeQueryPerformanceCounter call inside
	LARGE_INTEGER nSystemTime = { 0 };
	KeQuerySystemTime(&nSystemTime);
	// Convert to UNIX epoch. Since Jan 1, 1970
	return (uint64_t)(nSystemTime.QuadPart - 116444736000000000LL) /
		((ULONG64)10 /*100 nano*/ * (ULONG64)1000 /*micro*/);
}

///
/// Gets procedures address.
///
/// @param usFnName - TBD.
/// @param ppFn - pointer to storage variable.
/// @return The function returns a NTSTATUS of the operation.
///
template<typename Fn>
inline NTSTATUS getProcAddress(UNICODE_STRING usFnName, Fn** ppFn)
{
	*ppFn = (Fn*)MmGetSystemRoutineAddress(&usFnName);
	return (*ppFn == nullptr) ? STATUS_NOT_IMPLEMENTED : STATUS_SUCCESS;
}

///
/// Gets procedures address and log errors.
///
/// @param usFnName - TBD.
/// @param fCanAbsent - if can absent, just logging message is generated, else error is returned.
/// @param ppFn - pointer to storage variable.
/// @return The function returns a NTSTATUS of the operation.
///
template<typename Fn>
inline NTSTATUS getProcAddressAndLog(UNICODE_STRING usFnName, bool fCanAbsent, Fn** ppFn)
{
	NTSTATUS eStatus = getProcAddress(usFnName, ppFn);
	// If success
	if (NT_SUCCESS(eStatus)) return eStatus;

	// If error
	if (fCanAbsent)
	{
		LOGINFO1("Can't get API routine <%wZ>.\r\n", &usFnName);
		return STATUS_SUCCESS;
	}

	return LOGERROR(eStatus, "Can't get API routine <%wZ>.\r\n", &usFnName);
}

///
/// Wrapper of ObQueryNameString.
///
/// Alloc appropriate string and returns it.
/// If function is success, ppusName should be free with ExFreePoolWithTag(, c_nAllocTag).
/// @param pObject - object.
/// @param usDst - returned value.
/// @return The function returns a NTSTATUS of the operation.
///
inline NTSTATUS ObQueryNameString(PVOID pObject, DynUnicodeString& usDst)
{
	// get length
	ULONG nBufferSize = 0;
	const NTSTATUS eStatus = ::ObQueryNameString(pObject, nullptr, 0, &nBufferSize);
	if (!NT_SUCCESS(eStatus) && eStatus != STATUS_INFO_LENGTH_MISMATCH)
		return eStatus;
	nBufferSize += sizeof(WCHAR);

	UniquePtr<UINT8> pBuffer(new (NonPagedPool) UINT8[nBufferSize]);
	if (!pBuffer)
		return STATUS_NO_MEMORY;

	auto pNameInfo = (POBJECT_NAME_INFORMATION)pBuffer.get();
	IFERR_RET_NOLOG(::ObQueryNameString(pObject, pNameInfo, nBufferSize, &nBufferSize));

	usDst.assign(pBuffer.release(), pNameInfo->Name);
	return STATUS_SUCCESS;
}

///
/// Smart pointer to HANDLE for kernel mode.
///
/// HANDLE closed with ZwClose.
///
class UniqueHandle
{
private:
	HANDLE m_handle = nullptr;

public:

	///
	/// Constructor.
	///
	UniqueHandle(HANDLE h = nullptr) : m_handle(h)
	{
	}

	///
	/// Destructor.
	///
	~UniqueHandle()
	{
		reset();
	}

	//
	// deny coping
	//
	UniqueHandle(const UniqueHandle&) = delete;
	UniqueHandle& operator= (const UniqueHandle&) = delete;

	//
	// move constructor
	//
	UniqueHandle(UniqueHandle && other) : m_handle(other.release())
	{
	}

	//
	// move assignment
	//
	UniqueHandle& operator= (UniqueHandle && other)
	{
		if (this != &other)
			reset(other.release());
		return *this;
	}

	//
	// operator bool
	//
	operator bool() const
	{
		return m_handle != nullptr;
	}

	//
	// operator bool
	//
	operator HANDLE() const
	{
		return m_handle;
	}

	///
	/// Cleans resource and returns address of internal "empty" resource.
	///
	/// @return The function returns a raw pointer to HANDLE.
	///
	HANDLE* getPtr()
	{
		reset();
		return &m_handle;
	}


	///
	/// Get the handle.
	///
	/// @return The function returns a HANDLE value.
	///
	HANDLE get() const
	{
		return m_handle;
	}

	///
	/// Releases the handle.
	///
	/// @return The function returns a HANDLE value.
	///
	HANDLE release()
	{
		HANDLE h = m_handle;
		m_handle = nullptr;
		return h;
	}

	///
	/// Resets the handle.
	///
	/// @param h - a HANDLE value.
	///
	void reset(HANDLE h = nullptr)
	{
		if (m_handle != nullptr)
			ZwClose(m_handle);
		m_handle = h;
	}
};

} // namespace openEdr
/// @}
