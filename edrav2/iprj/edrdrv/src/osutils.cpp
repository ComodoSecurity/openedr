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

namespace openEdr {

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

} // namespace openEdr
/// @}
