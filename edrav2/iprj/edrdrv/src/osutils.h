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

namespace cmd {

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
/// Create security descriptor for everyone all access.
///
SECURITY_DESCRIPTOR* createFltPortEveryoneAllAccessAllowMandantorySD();

//
//
// 
__checkReturn
bool
isSystemLuidProcess(
	__in PEPROCESS Process
);

///
/// Signature verification API wrappers
/// 

//
//
// 
__checkReturn
LOGICAL
isProtectedProcess(
	__in PEPROCESS Process
);

//
//
// 
__checkReturn
LOGICAL
isProtectedProcessLight(
	__in PEPROCESS Process
);

//
//
// 
__checkReturn
bool
isWow64Process(
	__in ULONG ProcessId
);

NTSTATUS
wrapApcWow64Thread(
	__inout PVOID* ApcContext,
	__inout PVOID* ApcRoutine
);

//
//
// 
__checkReturn
bool
isWow64Process(
	__in PEPROCESS Process
);

// 
// 
// 
SE_SIGNING_LEVEL
getProcessSignatureLevel(
	__in PEPROCESS Process,
	__out PSE_SIGNING_LEVEL SectionSignatureLevel
);

// 
// 
// 
__drv_maxIRQL(APC_LEVEL)
__checkReturn
NTSTATUS
seGetCachedSigningLevel(
	__in PFILE_OBJECT FileObject,
	__out PULONG Flags,
	__out PSE_SIGNING_LEVEL SigningLevel,
	__reserved __out_ecount_full_opt(*ThumbprintSize) PUCHAR Thumbprint = nullptr,
	__reserved __out_opt PULONG ThumbprintSize = nullptr,
	__reserved __out_opt PULONG ThumbprintAlgorithm = nullptr
);

// 
// 
// 
__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
ntSetCachedSigningLevel(
	__in ULONG Flags,
	__in SE_SIGNING_LEVEL InputSigningLevel,
	__in_ecount(SourceFileCount) PHANDLE SourceFiles,
	__in ULONG SourceFileCount,
	__in HANDLE TargetFile
);

//
//
//
__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
getProcessModuleSignatureLevel(
	__in PEPROCESS Process,
	__in const PFILE_OBJECT FileObject,
	__out SE_SIGNING_LEVEL& SignatureLevel
);

//
//
//
__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
getProcessModuleSignatureLevel(
	__in PEPROCESS Process,
	__in PFLT_INSTANCE Instance,
	__in const UNICODE_STRING& FileName,
	__out SE_SIGNING_LEVEL& SignatureLevel
);

//
//
//
__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
getModuleSignatureLevel(
	__in const UNICODE_STRING& FileName,
	__out SE_SIGNING_LEVEL& SignatureLevel,
	__in_opt PFLT_INSTANCE Instance = nullptr
);

//
// ObCloseHandle deleter for SmartPtrT
//
template<auto V = MODE::KernelMode>
struct CloseDeleter
{
	void operator()(void* pData)
	{
		ObCloseHandle(pData, V);
	}
};

using KernelModeCloseDeleter = CloseDeleter<MODE::KernelMode>;
using UserModeCloseDeleter = CloseDeleter<MODE::UserMode>;

struct FltHandleCloseDeleter
{
	void operator()(void* FltHandle)
	{
		if (FltHandle)
			FltClose(FltHandle);
	}
};


//
// ObfDereferenceObject deleter for SmartPtrT
//
struct DerefDeleter
{
	void operator()(void* pData)
	{
		ObfDereferenceObject(pData);
	}
};

//
// FltObjectDereference deleter for SmartPtrT
//
struct FltDerefDeleter
{
	void operator()(void* pData)
	{
		FltObjectDereference(pData);
	}
};

//
// FltReleaseFileNameInformation deleter for SmartPtrT
//
struct FltReleaseFileNameInformationDeleter
{
	void operator()(PFLT_FILE_NAME_INFORMATION pData)
	{
		FltReleaseFileNameInformation(pData);
	}
};

///
/// Smart pointer for kernel mode.
///
/// @tparam T - data type.
/// @tparam fUseDeleter - way to free data Data type.
///
template<typename T, typename fUseDeleter = KernelModeCloseDeleter>
class SmartPtrT
{
public:
	using Pointer = typename GetPointer<T>::type;

private:
	Pointer m_pData; //< data

	//
	// call Deleter
	//
	void cleanup()
	{
		if (m_pData == nullptr)
			return;
		fUseDeleter()(m_pData);
		m_pData = nullptr;
	}

public:

	///
	/// Constructor.
	///
	SmartPtrT(Pointer pData = nullptr) : m_pData(pData)
	{
	}

	///
	/// Destructor.
	///
	~SmartPtrT()
	{
		reset();
	}

	//
	// deny coping
	//
	SmartPtrT(const SmartPtrT&) = delete;
	SmartPtrT<T>& operator= (const SmartPtrT&) = delete;

	//
	// move constructor
	//
	SmartPtrT(SmartPtrT&& other) : m_pData(other.release())
	{
	}

	//
	// move assignment
	//
	SmartPtrT& operator= (SmartPtrT&& other)
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
		return m_pData != nullptr;
	}

	//
	// operator->
	//
	Pointer operator->() const
	{
		return m_pData;
	}

	///
	/// Get the data.
	///
	/// @return The function returns a smart pointer.
	///
	Pointer get() const
	{
		return m_pData;
	}

	///
	/// Release.
	///
	/// @return The function returns a smart pointer.
	///
	Pointer release()
	{
		T* pData = m_pData;
		m_pData = nullptr;
		return pData;
	}

	///
	/// Reset.
	///
	/// @param pData [opt] - a pointer to the data.
	///
	void reset(Pointer pData = nullptr)
	{
		cleanup();
		m_pData = pData;
	}
public:
	///
	/// Cleans resource and returns address of internal "empty" resource.
	///
	/// @return address of empty raw pointer.
	///
	T** getPtr()
	{
		reset();
		return &m_pData;
	}

	///
	/// Cleans resource and returns address of internal "empty" resource.
	///
	/// @return address of empty raw pointer.
	///
	T** operator&()
	{
		return getPtr();
	}

	///
	/// @return Cast object to pointer on stored object.
	///
	operator T* () { return m_pData; }
	operator const T* () const { return m_pData; }

	///
	/// @return ownership of raw pointer.
	///
	T* detach()
	{
		T* pOld = m_pData;
		m_pData = nullptr;
		return pOld;
	}
};

///
/// Smart pointers to HANDLE object.
///
using UniqueHandle = SmartPtrT<void, KernelModeCloseDeleter>;
using UniqueUserHandle = SmartPtrT<void, UserModeCloseDeleter>;

///
/// FltHandle smart pointer.
///
/// FltHandle released with FltClose.
///
using UniqueFltHandle = SmartPtrT<void, FltHandleCloseDeleter>; 

///
/// Kernel object dereference helper.
///
/// @tparam T - data type.
/// 
template<typename T>
using KrnlObjectDeref = SmartPtrT<T, DerefDeleter>;

///
/// Smart pointer to _FILE_OBJECT for kernel mode.
///
/// _FILE_OBJECT released with ObDereferenceObject.
///
using FileObjectPtr = KrnlObjectDeref<_FILE_OBJECT>;

///
/// Smart pointer to _EPROCESS for kernel mode.
///
/// _EPROCESS released with ObDereferenceObject.
///
using ProcObjectPtr = KrnlObjectDeref<_KPROCESS>;


///
/// Smart pointer to _ETHREAD for kernel mode.
///
/// _ETHREAD released with ObDereferenceObject.
///
using ThreadObjectPtr = KrnlObjectDeref<_KTHREAD>;

struct TokenDerefHelper
{
	void operator()(PACCESS_TOKEN Token)
	{
		if (Token)
			PsDereferencePrimaryToken(Token);
	}
};

///
/// Smart pointer to _ACCESS_TOKEN object.
///
/// _ACCESS_TOKEN released with PsDereferencePrimaryToken.
///
using AccessTokenRef = SmartPtrT<void, TokenDerefHelper>;

///
/// Flt objects dereference helper.
///
/// @tparam T - data type.
/// 
template<typename T>
using FltObjectDeref = SmartPtrT<T, FltDerefDeleter>;

///
/// Smart pointer to _FLT_VOLUME for kernel mode.
///
/// _FLT_VOLUME released with FltObjectDereference.
///
using FltVolumeDeref = FltObjectDeref<_FLT_VOLUME>;

///
/// Smart pointer to _FLT_INSTANCE for kernel mode.
///
/// _FLT_INSTANCE released with FltObjectDereference.
///
using FltInstanceDeref = FltObjectDeref<_FLT_INSTANCE>;

///
/// Smart pointer to _FLT_FILE_NAME_INFORMATION for kernel mode.
///
/// _FLT_VOLUME released with FltReleaseFileNameInformation.
///
using FltFileNameInfoDeref = SmartPtrT<_FLT_FILE_NAME_INFORMATION, FltReleaseFileNameInformationDeleter>;

///
/// Attach to process helper class.
///
class AttachToProcess
{
private:
	KAPC_STATE m_apcState = {0};
	bool m_fAttached = false;

public:

	///
	/// Constructor.
	///
	AttachToProcess(PEPROCESS pProcess)
	{
		KeStackAttachProcess(pProcess, &m_apcState);
		m_fAttached = true;
	}

	///
	/// Destructor.
	///
	~AttachToProcess()
	{
		detach();
	}

	///
	/// deny coping
	///
	AttachToProcess(const AttachToProcess&) = delete;
	AttachToProcess& operator= (const AttachToProcess&) = delete;

	///
	/// deny move constructor
	///
	AttachToProcess(AttachToProcess&& other) = delete;

	///
	/// deny move assignment
	///
	AttachToProcess& operator= (AttachToProcess&& other) = delete;

	///
	/// detach.
	///
	void detach()
	{
		if (m_fAttached)
		{
			m_fAttached = false;
			KeUnstackDetachProcess(&m_apcState);
		}
	}
};

///
/// Returns kernel handle with PROCESS_ALL_ACCESS access rights.
///
/// @param[in] nProcessId - process id.
/// @param[out] kernel handle.
/// 
HANDLE openProcess(ULONG nProcessId);


class IoMdl
{
	PMDL m_Mdl;
public:
	IoMdl(const IoMdl&) = delete;
	IoMdl& operator=(const IoMdl&) = delete;

	IoMdl() : m_Mdl(nullptr) {}
	~IoMdl() { Free(); }

	operator PMDL() const { return m_Mdl; }

	bool Allocate(PVOID VirtualAdderss, ULONG Length, BOOLEAN SecondaryBuffer = FALSE, BOOLEAN ChargeQuota = FALSE);
	void Free();

	NTSTATUS LockPages(KPROCESSOR_MODE AccessMode, LOCK_OPERATION Operation);
	PVOID GetSystemAddress(MM_PAGE_PRIORITY Priority = NormalPagePriority);
	PMDL Detach();
};

inline bool IoMdl::Allocate(PVOID VirtualAdderss, ULONG Length, BOOLEAN SecondaryBuffer /*= FALSE*/, BOOLEAN ChargeQuota /*= FALSE */)
{
	Free();
	m_Mdl = IoAllocateMdl(VirtualAdderss, Length, SecondaryBuffer, ChargeQuota, nullptr);
	return (m_Mdl != nullptr);
}

inline void IoMdl::Free()
{
	PMDL mdl = m_Mdl, nextMdl;
	m_Mdl = nullptr;

	while (mdl)
	{
		nextMdl = mdl->Next;

		if (FlagOn(mdl->MdlFlags, MDL_PAGES_LOCKED))
		{
			MmUnlockPages(mdl);
		}
		IoFreeMdl(mdl);
		mdl = nextMdl;
	}
}

inline NTSTATUS IoMdl::LockPages(KPROCESSOR_MODE AccessMode, LOCK_OPERATION Operation)
{
	if (nullptr == m_Mdl)
		return STATUS_INSUFFICIENT_RESOURCES;

	if (!FlagOn(m_Mdl->MdlFlags, MDL_PAGES_LOCKED))
	{
		__try
		{
			MmProbeAndLockPages(m_Mdl, AccessMode, Operation);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return GetExceptionCode();
		}
	}
	return STATUS_SUCCESS;
}

inline PMDL IoMdl::Detach()
{
	PMDL tmp = m_Mdl;

	m_Mdl = nullptr;
	return tmp;
}

inline PVOID IoMdl::GetSystemAddress(MM_PAGE_PRIORITY Priority /*= NormalPagePriority*/)
{
	if (nullptr == m_Mdl)
		return nullptr;

	ASSERT(FlagOn(m_Mdl->MdlFlags, MDL_PAGES_LOCKED));

	return MmGetSystemAddressForMdlSafe(m_Mdl, Priority);
}

template<auto LockOperation = IoReadAccess>
class AutoMdl : public IoMdl
{
	PVOID m_Buffer;
	ULONG m_Length;
public:
	NTSTATUS Init(PVOID Buffer, ULONG Length)
	{
		m_Buffer = Buffer;
		m_Length = Length;

		if (!m_Buffer || !m_Length)
			return STATUS_ACCESS_VIOLATION;
		
		IoMdl::Allocate(m_Buffer, m_Length);

		KPROCESSOR_MODE accessMode = static_cast<KPROCESSOR_MODE>((m_Buffer >= MM_SYSTEM_RANGE_START) ? KernelMode : UserMode);
		return IoMdl::LockPages(accessMode, LockOperation);
	}

	PVOID GetBuffer()
	{
		return IoMdl::GetSystemAddress();
	}

	ULONG GetLength()
	{
		return m_Length;
	}
};

using AutoMdlForWrite = AutoMdl<IoWriteAccess>;
using AutoMdlForRead = AutoMdl<IoReadAccess>;
using AutoMdlForModify = AutoMdl<IoModifyAccess>;

struct UserSpaceRegion
{
	HANDLE m_ProcessHandle = nullptr;
	PVOID m_BaseAddress = nullptr;
	SIZE_T m_Size = 0;
};

//
// custom deleter for UserSpaceRegionPtr
//
struct UserSpaceRegionDeleter
{
	void operator()(UserSpaceRegion* Data)
	{
		if (Data->m_BaseAddress)
		{
			ZwFreeVirtualMemory(Data->m_ProcessHandle, &Data->m_BaseAddress, &Data->m_Size, MEM_FREE);
			Data->m_BaseAddress = nullptr;
		}
	}
};

using UserSpaceRegionPtr = UniquePtr<UserSpaceRegion, UserSpaceRegionDeleter>;
} // namespace cmd
/// @}
