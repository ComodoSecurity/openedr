//
//  edrav2.libdllinj
//  Driver library for DLL injection
//
///
/// @file libdllinj API
///
#pragma once

#include "common.h"
#include <madchookdrv.h>
#include "dllinj.h"

namespace openEdr {
namespace dllinj {

//
//
//
static void InitLists()
{
	InitDllList();
	InitSessionList();
	InitProcessList();
	InitDelayedInjectList();
	InitInjectRestoreIatList();
}

//
//
//
static void FreeLists()
{
	FreeDllList();
	FreeSessionList();
	FreeProcessList();
	FreeDelayedInjectList();
	FreeInjectRestoreIatList();
}

//
//
//
static PDEVICE_OBJECT g_pDeviceObjectForWorkItem = nullptr;

//
// This callback is called whenever a new image is loaded
//
static VOID ImageCallback(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	DriverEvent_NewImage(g_pDeviceObjectForWorkItem, ProcessId, FullImageName, ImageInfo);
}

//
// Extracts the public key of the certificate our driver was signed with
//
static BOOLEAN ExtractPublicKey(PUNICODE_STRING RegistryPath)
{
	BOOLEAN result = FALSE;
	__try
	{
		if (InitCrypt())
		{
			// extract and remember our driver's authenticode certificate's public key
			OBJECT_ATTRIBUTES oa;
			HANDLE hkey;
			InitializeObjectAttributes(&oa, RegistryPath, 0, nullptr, nullptr);
			if (!ZwOpenKey(&hkey, KEY_READ, &oa))
			{
				ULONG size = 0;
				UNICODE_STRING imagePath;
				RtlInitUnicodeString(&imagePath, L"ImagePath");
				ZwQueryValueKey(hkey, &imagePath, KeyValuePartialInformation, NULL, 0, &size);
				if (size)
				{
					PKEY_VALUE_PARTIAL_INFORMATION vpi = static_cast<PKEY_VALUE_PARTIAL_INFORMATION>(ExAllocatePoolWithTag(PagedPool, size + sizeof(WCHAR), 'dinj'));
					if (vpi)
					{
						if (!ZwQueryValueKey(hkey, &imagePath, KeyValuePartialInformation, vpi, size, &size))
						{
							PWCHAR driverFile = NULL;
							// make sure the string is zero terminated, just to be safe
							((PWCHAR)vpi->Data)[vpi->DataLength / sizeof(WCHAR)] = 0;
							{
								// Does the image path start with "\SYSTEM32\..."?  (if it does we can't open the file)
								UNICODE_STRING usSystem32 = RTL_CONSTANT_STRING(L"SYSTEM32\\");
								UNICODE_STRING usImagePath;
								RtlInitUnicodeString(&usImagePath, (PWCHAR)vpi->Data);
								if (RtlPrefixUnicodeString(&usSystem32, &usImagePath, TRUE))
								{
									// It does, so we need to prepend "\SYSTEMROOT\" to the image path.
									UNICODE_STRING usSystemRoot = RTL_CONSTANT_STRING(L"\\SYSTEMROOT\\");
									driverFile = static_cast<PWCHAR>(ExAllocatePoolWithTag(PagedPool, ((size_t)usSystemRoot.Length) + ((size_t)usImagePath.Length) + sizeof(WCHAR), 'dinj'));
									if (driverFile)
									{
										RtlCopyMemory(driverFile, usSystemRoot.Buffer, usSystemRoot.Length);
										RtlCopyMemory(driverFile + (usSystemRoot.Length / sizeof(WCHAR)), usImagePath.Buffer, usImagePath.Length + sizeof(WCHAR));
									}
								}
							}
							{
								// finally, string troubles solved, let's do the real work
								PVOID publicKeyExp[4], publicKeyMod[4];
								int publicKeyExpLen[4], publicKeyModLen[4];
								if (ExtractAuthenticodePublicKey((driverFile) ? (driverFile) : ((PWCHAR)vpi->Data), publicKeyExp, publicKeyExpLen, publicKeyMod, publicKeyModLen))
								{
									SetPublicKeys(publicKeyExp, publicKeyExpLen, publicKeyMod, publicKeyModLen);
									result = TRUE;
								}
							}
							if (driverFile)
								ExFreePool(driverFile);
						}
						ExFreePool(vpi);
					}
				}
				ZwClose(hkey);
			}
		}
	}
	__except (1)
	{
		// ooops!
		//LogUlonglong(L"\\??\\C:\\ExtractPublicKey crash", 0);
		result = FALSE;
	}
	return result;
}


// ********************************************************************

//
//
//
constexpr ULONG c_nDllItemSize = sizeof(DllItem) + 0x1000 /*possible filter size*/;

//
// DllInfo
//
struct DllInfo
{
	UNICODE_STRING usPath = {}; // full dll file path.
	BOOLEAN fAlreadyRegistered = FALSE; // Dll is already registered
	explicit DllInfo(UNICODE_STRING _usPath) : usPath(_usPath) {}
};

typedef List<DllInfo> DllInfoList;

//
// Remove already registered Dll which are not specified in dllInfoList
//
NTSTATUS removeUnspecifiedDlls(DllInfoList& dllInfoList)
{
	PDllItem pDllItem = nullptr;

	__try
	{
		// Iterate registered Dlls
		for (ULONG nOldDllIndex = 0;;)
		{
			// We should allocate Item for every DLL because 
			// dll is released in DriverEvent_UninjectionRequest
			if (pDllItem == nullptr)
			{
				pDllItem = static_cast<PDllItem>(ExAllocatePoolWithTag(NonPagedPool, c_nDllItemSize, c_nAllocTag));
				if (pDllItem == nullptr)
					return LOGERROR(STATUS_NO_MEMORY);
				RtlZeroMemory(pDllItem, c_nDllItemSize);
			}

			if (!DriverEvent_EnumInjectRequest(nOldDllIndex, pDllItem, c_nDllItemSize))
				break;
			UNICODE_STRING usOldDllName = {};
			RtlInitUnicodeString(&usOldDllName, pDllItem->Name);

			// Search same DLL in new DLLs
			bool fRemoveDll = true;
			for (auto& dllInfo : dllInfoList)
			{
				if (!RtlEqualUnicodeString(&dllInfo.usPath, &usOldDllName, TRUE))
					continue;
				dllInfo.fAlreadyRegistered = TRUE;
				fRemoveDll = false;
			}

			// Remove old DLL
			if (!fRemoveDll)
			{
				++nOldDllIndex;
				continue;
			}

			BOOLEAN fAllowUnload = FALSE;
			DriverEvent_UninjectionRequest(pDllItem, nullptr, &fAllowUnload);
			// reset pointer because DriverEvent_UninjectionRequest has released buffer
			pDllItem = nullptr;
			// reset index after removing DLL
			nOldDllIndex = 0;
		}
	}
	__finally
	{
		if (pDllItem != nullptr)
			ExFreePoolWithTag(pDllItem, c_nAllocTag);
	}

	return STATUS_SUCCESS;
}

//
// Register new Dlls from dllInfoList (except DLLs which is already registered)
//
NTSTATUS addNewDlls(DllInfoList& dllInfoList)
{
	for (auto& dllInfo : dllInfoList)
	{
		if (dllInfo.fAlreadyRegistered)
			continue;

		// We should allocate Item for every DLL because 
		// "dll is released in DriverEvent_InjectionRequest"
		PDllItem pDllItem = static_cast<PDllItem>(ExAllocatePoolWithTag(NonPagedPool, c_nDllItemSize, c_nAllocTag));
		if (pDllItem == nullptr)
			return LOGERROR(STATUS_NO_MEMORY);
		RtlZeroMemory(pDllItem, c_nDllItemSize);

		pDllItem->Size = sizeof(*pDllItem) - sizeof(pDllItem->Data) +
			2 * sizeof(WCHAR) /*2 empty zero-end string*/;
		pDllItem->Session = ULONG(-1); // Inject into all sessions
		pDllItem->Flags = INJECT_SYSTEM_PROCESSES | INJECT_METRO_APPS /*| INJECT_VIA_IAT_PATCHING*/;
		memcpy(pDllItem->Name, dllInfo.usPath.Buffer, dllInfo.usPath.Length);

		BOOLEAN fAllowUnload = FALSE;
		if (!DriverEvent_InjectionRequest(pDllItem, nullptr, &fAllowUnload))
		{
			LOGERROR(STATUS_UNSUCCESSFUL, "Can't add DLL for injection. path: <%wZ>.\r\n", &dllInfo.usPath);
			continue;
		}
		
		LOGINFO2("Add DLL for injection. path: <%wZ>.\r\n", &dllInfo.usPath);
	}

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Inject callbacks
//

//
// This callback is called just before injection the specified dll
// If the callback return FALSE, injection of this DLL is rejected
//
BOOLEAN approveDllInject(PDllItem pDll, HANDLE /*hTargetProcessHandle*/,
	HANDLE hTargetProcessId, HANDLE /*hParentProcessId*/, BOOLEAN /*f64Process*/, BOOLEAN /*fSysProcess*/)
{
	LOGINFO2("Inject DLL into process. pid: %Iu. dll: %S.\r\n", (ULONG_PTR)hTargetProcessId,
		pDll->Name);
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// External API
//

//
//
//
NTSTATUS setInjectedDllList(List<DynUnicodeString>& dllList)
{
	// Fill DllInfo list
	DllInfoList dllInfoList;
	for (const auto& usName : dllList)
		IFERR_RET(dllInfoList.pushBack(DllInfo(usName)));

	IFERR_RET(removeUnspecifiedDlls(dllInfoList));
	IFERR_RET(addNewDlls(dllInfoList));

	return STATUS_SUCCESS;
}

//
//
//
void enableInjectedDllVerification(bool fEnable)
{
	enableDllAuthenticodeVerification(fEnable);
}


//
//
//
NTSTATUS initialize()
{
	SetDriverName(CMD_EDRDRV_FILE_NAME);

	// first init all the stuff that doesn't need finalization
	if ((!IsValidConfig()) || (!InitToolFuncs()) || (!InitInjectLibrary(nullptr)))
		return LOGERROR(STATUS_UNSUCCESSFUL);

	// init all lists
	InitLists();

	// set callback
	setDllInjectApprover(approveDllInject);

	// Init notification about newly loaded images.
	g_pDeviceObjectForWorkItem = g_pCommonData->pIoctlDeviceObject;
	if (PsSetLoadImageNotifyRoutine(ImageCallback))
	{
		FreeLists();
		return LOGERROR(STATUS_UNSUCCESSFUL);
	}

	// extract the public key of the certificate our driver was signed with
	if (!Config_GetPublicKey(nullptr, nullptr, nullptr, nullptr))
	{
		UNICODE_STRING usData = g_pCommonData->usRegistryPath;
		ExtractPublicKey(&usData);
	}

	g_pCommonData->fDllInjectorIsInitialized = true;

	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (!g_pCommonData->fDllInjectorIsInitialized)
		return;
	g_pCommonData->fDllInjectorIsInitialized = false;

	// unregister the notification about images
	PsRemoveLoadImageNotifyRoutine(ImageCallback);

	// free everything
	FreeLists();
}


namespace detail {

//
//
//
void notifyOnProcessCreation(procmon::Context* pNewProcessCtx, procmon::Context* pParentProcessCtx)
{
	if (!g_pCommonData->fDllInjectorIsInitialized)
		return;

	if (!g_pCommonData->fEnableDllInjection)
		return;

	// Injection filtration
	if (!pNewProcessCtx->fEnableInject)
	{
		LOGINFO3("Skip injection of DLL into process. pid: %Iu.\r\n", 
			(ULONG_PTR)pNewProcessCtx->processInfo.nPid);
		return;
	}

	LOGINFO3("Try to inject DLL into process. pid: %Iu.\r\n", 
		(ULONG_PTR)pNewProcessCtx->processInfo.nPid);
	DriverEvent_NewProcess(pNewProcessCtx->processInfo.nPid, pParentProcessCtx->processInfo.nPid);
}

//
//
//
void notifyOnProcessTermination(procmon::Context* pProcessCtx)
{
	if (!g_pCommonData->fDllInjectorIsInitialized)
		return;

	DriverEvent_ProcessGone(pProcessCtx->processInfo.nPid);
}

} // namespace detail

} // namespace dllinj
} // namespace openEdr
