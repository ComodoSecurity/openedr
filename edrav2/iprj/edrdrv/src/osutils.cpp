//
// edrav2.edrdrv project
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
#include "osutils.h"
#include "filemon.h"

namespace cmd {

using cmd::filemon::tools::openFile;
using cmd::filemon::tools::getInstanceFromFileObject;

//
//
//
NTSTATUS callKernelDeviceIoControl(PDEVICE_OBJECT pDevObj, ULONG ControlCode, 
	PVOID pInBuf, ULONG InBufLen, PVOID pOutBuf, ULONG OutBufLen)
{
	KEVENT Event;
	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	IO_STATUS_BLOCK IoStatus = {};
	PIRP pIrp = IoBuildDeviceIoControlRequest(ControlCode, pDevObj, pInBuf, 
		InBufLen, pOutBuf, OutBufLen, FALSE, &Event, &IoStatus);
	if (pIrp == NULL) return LOGERROR(STATUS_INVALID_DEVICE_REQUEST);

	NTSTATUS Status = IoCallDriver(pDevObj, pIrp);
	if (NT_ERROR(Status)) return Status;

	if (Status == STATUS_PENDING)
	{
		IFERR_LOG(KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL));
	}

	// +FIXME: if Status == STATUS_PENDING then it returns here even if we wait
	// Why? IoStatus.Status will be changed.
	return IoStatus.Status;
}

//
//
//
NTSTATUS checkOSVersionSupport(ULONG dwMajorVersion, ULONG dwMinorVersion)
{
	RTL_OSVERSIONINFOEXW VersionInfo = { 0 };
	VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
	VersionInfo.dwMajorVersion = dwMajorVersion;
	VersionInfo.dwMinorVersion = dwMinorVersion;

	ULONGLONG ConditionMask = 0;
	VER_SET_CONDITION(ConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(ConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	return RtlVerifyVersionInfo(&VersionInfo, 
		VER_MAJORVERSION | VER_MINORVERSION, ConditionMask);
}

//
//
//
NTSTATUS getTokenSid(HANDLE hToken, PUNICODE_STRING pusUserName)
{
	TOKEN_USER* pTokenUser = nullptr;

	__try
	{
		// Query data size
		ULONG nTokenInfoSize = 0;
		NTSTATUS ns = ZwQueryInformationToken(hToken, TokenUser, NULL, 0, 
			&nTokenInfoSize);
		if (ns != STATUS_BUFFER_TOO_SMALL) IFERR_RET(ns);

		// Allocate memory and query information
		pTokenUser = (TOKEN_USER*)ExAllocatePoolWithTag(NonPagedPool, nTokenInfoSize, c_nAllocTag);
		if (pTokenUser == nullptr) return LOGERROR(STATUS_NO_MEMORY);
		IFERR_RET(ZwQueryInformationToken(hToken, TokenUser, pTokenUser, nTokenInfoSize, &nTokenInfoSize));
		
		//
		if (!RtlValidSid(pTokenUser->User.Sid)) return LOGERROR(STATUS_UNSUCCESSFUL);
		IFERR_RET(RtlConvertSidToUnicodeString(pusUserName, pTokenUser->User.Sid, FALSE));
	}
	__finally
	{
		if (pTokenUser != NULL)
			ExFreePoolWithTag(pTokenUser, c_nAllocTag);
	}

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS checkTokenElevation(HANDLE hToken, bool* pfElevated)
{
	TOKEN_ELEVATION tokenElevation = {};
	ULONG nTokenInfoSize = 0;
	IFERR_RET(ZwQueryInformationToken(hToken, TokenElevation, &tokenElevation, 
		sizeof(tokenElevation), &nTokenInfoSize));
	if (nTokenInfoSize != sizeof(tokenElevation)) 
		return LOGERROR(STATUS_UNSUCCESSFUL);

	*pfElevated = tokenElevation.TokenIsElevated != 0;
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS getTokenElevationType(HANDLE hToken, TOKEN_ELEVATION_TYPE* peElevationType)
{
	TOKEN_ELEVATION_TYPE tokenElevationType = {};
	ULONG nTokenInfoSize = 0;
	IFERR_RET(ZwQueryInformationToken(hToken, TokenElevationType, 
		&tokenElevationType, sizeof(tokenElevationType), &nTokenInfoSize));
	// +FIXME: Do we need this check?
	// Why not? It can be less.
	if (nTokenInfoSize != sizeof(tokenElevationType)) 
		return LOGERROR(STATUS_UNSUCCESSFUL);

	*peElevationType = tokenElevationType;
	return STATUS_SUCCESS;
}

//
//
//
HANDLE openProcess(PEPROCESS pProcessObject)
{
	UniqueHandle processHandle = nullptr;
	NTSTATUS eStatus = ObOpenObjectByPointer(pProcessObject, OBJ_KERNEL_HANDLE, NULL, PROCESS_ALL_ACCESS, *PsProcessType, KernelMode, &processHandle);
	if (!NT_SUCCESS(eStatus))
	{
		LOGERROR(eStatus, "Failed to open process: %X.\r\n", HandleToUlong(PsGetProcessId(pProcessObject)));
		return nullptr;
	}

	return processHandle.detach();
}

//
//
//
HANDLE openProcess(ULONG nProcessId)
{
	ProcObjectPtr processObject;

	NTSTATUS eStatus = PsLookupProcessByProcessId(UlongToHandle(nProcessId), &processObject);
	if (!NT_SUCCESS(eStatus))
	{
		LOGERROR(eStatus, "Failed to lookup process: %X.\r\n", nProcessId);
		return nullptr;
	}

	return openProcess(processObject.get());
}

//
//
//
SECURITY_DESCRIPTOR* createFltPortEveryoneAllAccessAllowMandantorySD()
{
	ULONG daclSize = 0;
	daclSize += RtlLengthSid(SeExports->SeWorldSid);
	daclSize += RtlLengthSid(SeExports->SeLocalSystemSid);
	daclSize += sizeof(ACCESS_ALLOWED_ACE) * 2;
	daclSize += sizeof(ACL);
	
	ULONG saclSize = 0;
	saclSize += RtlLengthSid(SeExports->SeLowMandatorySid);
	saclSize += sizeof(SYSTEM_MANDATORY_LABEL_ACE);
	saclSize += sizeof(ACL);

	size_t securityDescriptorAllocSize = sizeof(SECURITY_DESCRIPTOR) + static_cast<size_t>(daclSize) + static_cast<size_t>(saclSize);
	UniquePtr<SECURITY_DESCRIPTOR> securityDescriptor(reinterpret_cast<SECURITY_DESCRIPTOR*>(new (NonPagedPool) UINT8[securityDescriptorAllocSize]));
	if (!securityDescriptor)
		return nullptr;

	RtlZeroMemory(securityDescriptor.get(), securityDescriptorAllocSize);

	NTSTATUS status = RtlCreateSecurityDescriptor(securityDescriptor.get(), SECURITY_DESCRIPTOR_REVISION);
	if (!NT_SUCCESS(status))
		return nullptr;

	//Create SACL
	securityDescriptor->Control = SE_SACL_PRESENT;
	PACL  sacl = (PACL)Add2Ptr(securityDescriptor.get(), sizeof(SECURITY_DESCRIPTOR));
	status = RtlCreateAcl(sacl, saclSize, ACL_REVISION);
	if (!NT_SUCCESS(status))
		return nullptr;

	SYSTEM_MANDATORY_LABEL_ACE* mandatoryLabelAce = (SYSTEM_MANDATORY_LABEL_ACE*)Add2Ptr(sacl, sizeof(ACL));

	sacl->AceCount = 1;
	sacl->AclRevision = ACL_REVISION;
	sacl->AclSize = (USHORT)(sizeof(ACL) + sizeof(SYSTEM_MANDATORY_LABEL_ACE) + RtlLengthSid(SeExports->SeLowMandatorySid) - sizeof(ULONG));
	sacl->Sbz1 = 0;
	sacl->Sbz2 = 0;

	ACCESS_MASK	mandatoryPolicy = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP | SYSTEM_MANDATORY_LABEL_NO_READ_UP | SYSTEM_MANDATORY_LABEL_NO_EXECUTE_UP;
	mandatoryLabelAce->Header.AceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE;
	mandatoryLabelAce->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
	mandatoryLabelAce->Header.AceSize = (USHORT)sizeof(ACE_HEADER) + sizeof(ACCESS_MASK) + (USHORT)RtlLengthSid(SeExports->SeLowMandatorySid);
	mandatoryLabelAce->Mask = mandatoryPolicy;
	RtlCopyMemory(&mandatoryLabelAce->SidStart, SeExports->SeLowMandatorySid, RtlLengthSid(SeExports->SeLowMandatorySid));
	securityDescriptor->Sacl = sacl;

	//Create DACL
	PACL dacl = (PACL)Add2Ptr(securityDescriptor.get(), sizeof(SECURITY_DESCRIPTOR) + saclSize);
	status = RtlCreateAcl(dacl, daclSize, ACL_REVISION);
	if (!NT_SUCCESS(status))
		return nullptr;

	status = RtlSetOwnerSecurityDescriptor(securityDescriptor.get(), SeExports->SeLocalSystemSid, FALSE);
	if (!NT_SUCCESS(status))
		return nullptr;

	status = RtlSetDaclSecurityDescriptor(securityDescriptor.get(), TRUE, dacl, FALSE);
	if (!NT_SUCCESS(status))
		return nullptr;

	ACCESS_MASK	desiredAccess = FLT_PORT_ALL_ACCESS;

	// Add aces to the DACL
	status = RtlAddAccessAllowedAceEx(dacl,
		ACL_REVISION,
		OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
		desiredAccess,
		SeExports->SeWorldSid);

	if (!NT_SUCCESS(status))
		return nullptr;

	status = RtlAddAccessAllowedAce(dacl, ACL_REVISION, desiredAccess, SeExports->SeLocalSystemSid);
	if (!NT_SUCCESS(status))
		return nullptr;

	return securityDescriptor.release();
}

//
//
// 
__checkReturn
LOGICAL
isProtectedProcess(
	__in PEPROCESS Process
	)
{
	if (g_pCommonData->PsIsProtectedProcess != nullptr)
	{
		return g_pCommonData->PsIsProtectedProcess(Process);
	}
	return FALSE;
}

//
//
// 
__checkReturn
LOGICAL
isProtectedProcessLight(
	__in PEPROCESS Process
	)
{
	if (g_pCommonData->PsIsProtectedProcessLight != nullptr)
	{
		return g_pCommonData->PsIsProtectedProcessLight(Process);
	}
	return FALSE;
}

//
//
// 
__checkReturn
bool
isWow64Process(
	__in PEPROCESS Process
	)
{
#if defined (_WIN64)
	if (g_pCommonData->PsGetProcessWow64Process != nullptr)
	{
		return (g_pCommonData->PsGetProcessWow64Process(Process) != nullptr);
	}
#else
	UNREFERENCED_PARAMETER(Process);
#endif
	return false;
}

//
//
// 
__checkReturn
bool
isWow64Process(
	__in ULONG ProcessId
	)
{
#if defined (_WIN64)
	if (g_pCommonData->PsGetProcessWow64Process != nullptr)
	{
		ProcObjectPtr processObject;
		if (!NT_SUCCESS(PsLookupProcessByProcessId(UlongToHandle(ProcessId), &processObject)))
			return false;

		return isWow64Process(static_cast<PEPROCESS>(processObject));
	}
#else
	UNREFERENCED_PARAMETER(ProcessId);
#endif // defined (_WIN64)
	return false;
}

//
//
// 
NTSTATUS
wrapApcWow64Thread(
	__inout PVOID* ApcContext,
	__inout PVOID* ApcRoutine
	)
{
#if defined (_WIN64)
	if (g_pCommonData->PsWrapApcWow64Thread != nullptr)
	{
		return g_pCommonData->PsWrapApcWow64Thread(ApcContext, ApcRoutine);
	}
#else
	UNREFERENCED_PARAMETER(ApcContext);
	UNREFERENCED_PARAMETER(ApcRoutine);
#endif
	return STATUS_NOT_SUPPORTED;
}

//
//
//
__checkReturn
bool
isSystemLuidProcess(
	__in PEPROCESS Process
)
{
	AccessTokenRef token = PsReferencePrimaryToken(Process);
	if (nullptr == token)
	{
		NT_ASSERT(FALSE);
		return false;
	}
	
	LUID processLuid = { 0 };
	NTSTATUS status = SeQueryAuthenticationIdToken(token, &processLuid);
	if (NT_SUCCESS(status))
	{
		LUID systemLuid = SYSTEM_LUID;
		return !!RtlEqualLuid(&processLuid, &systemLuid);
	}
	LOGERROR(status, "isSystemLuidProcess => SeQueryAuthenticationIdToken failed\r\n");
	return false;
}

// 
// 
// 
SE_SIGNING_LEVEL
getProcessSignatureLevel(
	__in PEPROCESS Process,
	__out PSE_SIGNING_LEVEL SectionSignatureLevel
	)
{
	if (g_pCommonData->PsGetProcessSignatureLevel != nullptr)
	{
		return g_pCommonData->PsGetProcessSignatureLevel(Process, SectionSignatureLevel);
	}
	return SE_SIGNING_LEVEL_UNCHECKED;
}

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
	__reserved __out_ecount_full_opt(*ThumbprintSize) PUCHAR Thumbprint,
	__reserved __out_opt PULONG ThumbprintSize,
	__reserved __out_opt PULONG ThumbprintAlgorithm
	)
{
	if (g_pCommonData->SeGetCachedSigningLevel != nullptr)
	{
		return g_pCommonData->SeGetCachedSigningLevel(FileObject, Flags, SigningLevel, Thumbprint, ThumbprintSize, ThumbprintAlgorithm);
	}
	return STATUS_NOT_SUPPORTED;
}

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
	)
{
	if (g_pCommonData->NtSetCachedSigningLevel != nullptr)
	{
		return g_pCommonData->NtSetCachedSigningLevel(Flags, InputSigningLevel, SourceFiles, SourceFileCount, TargetFile);
	}
	return STATUS_NOT_SUPPORTED;
}

//
//
//
__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
getModuleSignatureLevel(
	__in const UNICODE_STRING& FileName,
	__out SE_SIGNING_LEVEL& SignatureLevel,
	__in_opt PFLT_INSTANCE Instance
	)
{
	if (KernelMode == ExGetPreviousMode())
		return LOGERROR(STATUS_NOT_SUPPORTED, "Unsupported request for signature verification <%wZ>\r\n", (PCUNICODE_STRING)&FileName);

	UniqueFltHandle fileHandle;
	FileObjectPtr fileObject;

	IFERR_RET(openFile(Instance, FileName, fileHandle, fileObject), "Failed to open file <%wZ>\r\n", (PCUNICODE_STRING)&FileName);

	UniqueUserHandle userFileHandle;
	IFERR_RET(ObOpenObjectByPointer(fileObject, 0, nullptr, FILE_READ_ACCESS, *IoFileObjectType, UserMode, &userFileHandle), "ObOpenObjectByPointer failed\r\n");

	cmd::UserSpaceRegionPtr regionPtr(new (NonPagedPool) cmd::UserSpaceRegion());
	if (nullptr == regionPtr.get())
		return STATUS_INSUFFICIENT_RESOURCES;

	SIZE_T regionSize = sizeof(HANDLE);
	regionPtr.get()->m_ProcessHandle = ZwCurrentProcess();
	regionPtr.get()->m_Size = regionSize;

	PHANDLE userBuffer = reinterpret_cast<PHANDLE>(regionPtr.get()->m_BaseAddress);
	IFERR_RET(ZwAllocateVirtualMemory(ZwCurrentProcess(), (PVOID*)&userBuffer, 0, &regionSize, MEM_COMMIT, PAGE_READWRITE), "ZwAllocateVirtualMemory failed\r\n");

	NTSTATUS status = STATUS_SUCCESS;
	__try
	{
		*userBuffer = userFileHandle;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		status = GetExceptionCode();
	}

	IFERR_RET(status, "Exception on user VA access\r\n");

	//
	// [IMPORTANT] 
	// This API required only user buffer and UserMode==ExGetPreviousMode()
	// This combination of flags perform signature checking in $Kernel.Purge.ESBCache and signature verification if cache is absent
	//
	status = ntSetCachedSigningLevel(UNPROTECTED_FLAG, SE_SIGNING_LEVEL_UNCHECKED, userBuffer, 1, userFileHandle);
	if (STATUS_INVALID_IMAGE_HASH == status)
	{
		SignatureLevel = SE_SIGNING_LEVEL_UNSIGNED;
		return STATUS_SUCCESS;
	}

	SignatureLevel = SE_SIGNING_LEVEL_UNCHECKED;
	IFERR_RET(status, "NtSetCachedSigningLevel failed\r\n");

	// Get actual signing level from EA cache after verification
	ULONG flags = 0;
	status = seGetCachedSigningLevel(fileObject, &flags, &SignatureLevel);
	if (NT_SUCCESS(status))
		return STATUS_SUCCESS;

	// Windows 8.0 does not support SeGetCachedSigningLevel API
	if (isProtectedProcess(PsGetCurrentProcess()))
	{
		SignatureLevel = SE_SIGNING_LEVEL_WINDOWS;
	}
	else
	{
		SignatureLevel = SE_SIGNING_LEVEL_AUTHENTICODE;
	}
	return STATUS_SUCCESS;
}

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
	)
{
	SignatureLevel = SE_SIGNING_LEVEL_UNCHECKED;
	if (KernelMode == ExGetPreviousMode())
	{
		//
		// [IMPORTANT]
		// Workaround for GUI drivers mapping by SMSS/CSRSS processes
		// L"*\\Windows\\System32\\win32k.sys"
		// L"*\\Windows\\System32\\win32kfull.sys"
		// L"*\\Windows\\System32\\win32kbase.sys"
		// L"*\\Windows\\System32\\drivers\dxgmms1.sys"
		// L"*\\Windows\\System32\\drivers\dxgmms2.sys"
		// L"*\\Windows\\System32\\cdd.dll"
		//
		LOGINFO4("getProcessModuleSignatureLevel request is not supported for PreviousMode == KernelMode, return ProcessSignatureLevel\r\n");

		if (isProtectedProcess(Process))
		{
			SignatureLevel = SE_SIGNING_LEVEL_WINDOWS;
			return STATUS_SUCCESS;
		}

		SE_SIGNING_LEVEL sectionSignatureLevel = 0;
		SignatureLevel = getProcessSignatureLevel(Process, &sectionSignatureLevel);
		return STATUS_SUCCESS;
	}

	IFERR_RET_NOLOG(getModuleSignatureLevel(FileName, SignatureLevel, Instance));
	return STATUS_SUCCESS;
}

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
	)
{
	ULONG flags = 0;
	if (NT_SUCCESS(cmd::seGetCachedSigningLevel(FileObject, &flags, &SignatureLevel)))
		return STATUS_SUCCESS;

	FltInstanceDeref instance;
	IFERR_RET(getInstanceFromFileObject(FileObject, &instance), "Failed to get Instance from FILE_OBJECT\r\n");

	FltFileNameInfoDeref fileNameInfo;
	IFERR_RET(FltGetFileNameInformationUnsafe(FileObject, instance, FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP | FLT_FILE_NAME_NORMALIZED, &fileNameInfo));
	IFERR_RET(getProcessModuleSignatureLevel(Process, instance, fileNameInfo->Name, SignatureLevel), "Failed to get ModuleSignatureLevel from FileName\r\n");

	return STATUS_SUCCESS;
}

} // namespace cmd
/// @}
