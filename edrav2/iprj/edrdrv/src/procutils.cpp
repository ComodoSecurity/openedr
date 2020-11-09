//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Common Process information
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "procutils.h"

namespace openEdr {

//
// Get Current Process ImageName. Allocate buffer
//
NTSTATUS getProcessImageName(HANDLE hProcess, PUNICODE_STRING* ppProcessImageName)
{
	// +FIXME: Wrong return status STATUS_NOT_IMPLEMENTED
	// STATUS_NOT_SUPPORTED?
	if (g_pCommonData->fnZwQueryInformationProcess == NULL) 
		return LOGERROR(STATUS_NOT_IMPLEMENTED);

	PVOID pBuffer = NULL;
	__try
	{
		// Step one - get the size we need
		ULONG nBufferSize = 0;
		NTSTATUS ns = g_pCommonData->fnZwQueryInformationProcess(hProcess, 
			ProcessImageFileName, NULL, 0, &nBufferSize);
		if (NT_ERROR(ns))
		{
			if (ns != STATUS_INFO_LENGTH_MISMATCH) IFERR_RET(ns);
		}
	
		pBuffer = ExAllocatePoolWithTag(NonPagedPool, nBufferSize, c_nAllocTag);
		if (pBuffer == NULL) return LOGERROR(STATUS_NO_MEMORY);

		// Now lets get the data
		IFERR_RET(g_pCommonData->fnZwQueryInformationProcess(hProcess, 
			ProcessImageFileName, pBuffer, nBufferSize, &nBufferSize));

		// +FIXME: Can we return pointer to allocated memory?
		*ppProcessImageName = (PUNICODE_STRING)pBuffer;
		pBuffer = NULL;
	}
	__finally
	{
		if (pBuffer != NULL)
			ExFreePoolWithTag(pBuffer, c_nAllocTag);
	}

	return STATUS_SUCCESS;
}


//////////////////////////////////////////////////////////////////////////
//
// Process flags by name
//

struct SpecialProcFlags
{
	UNICODE_STRING usName;		// ImageName postfix
	UINT32 nFlags;				// Process Flags
};

//
//
//
static SpecialProcFlags g_specialProcFlags[] =
{
	{ U_STAT(L"\\system32\\csrss.exe"), (UINT32)ProcessInfoFlags::CsrssProcess },
	{ U_STAT(L"\\edrsvc.exe"), (UINT32)ProcessInfoFlags::ThisProductProcess },
	{ U_STAT(L"\\edrcon.exe"), (UINT32)ProcessInfoFlags::ThisProductProcess },
};

NTSTATUS getProcessFlagsByName(PUNICODE_STRING pusImageName, UINT32* pnFlags)
{
	for (size_t nIndex = 0; nIndex < ARRAYSIZE(g_specialProcFlags); ++nIndex)
	{
		SpecialProcFlags& pCur = g_specialProcFlags[nIndex];

		if (!isEndedWith(pusImageName, &pCur.usName))
			continue;

		setFlag(*pnFlags, pCur.nFlags);
	}
	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Collect process info
//
PRESET_STATIC_UNICODE_STRING(c_usSystem, L"System");
PUNICODE_STRING CommonProcessInfo::getPrintImageName()
{
	if (pusImageName != nullptr)
		return pusImageName;
	else
		return &c_usSystem;
}

//
//
//
_IRQL_requires_max_(PASSIVE_LEVEL)
void fillProcessInfoByHandle(HANDLE nPid, HANDLE hProcess, CommonProcessInfo* pProcessInfo, bool fPrintInfo)
{
	pProcessInfo->nPid = nPid;

	// +FIXME: Do we can continue without it?
	// Now - no. But it is possible. 
	// We collect different info and we don't force to use fnZwQueryInformationProcess only.
	if (g_pCommonData->fnZwQueryInformationProcess != nullptr)
	{
		PROCESS_BASIC_INFORMATION BasicInfo = {};
		NTSTATUS ns = g_pCommonData->fnZwQueryInformationProcess(hProcess, 
			ProcessBasicInformation, &BasicInfo, sizeof(BasicInfo), nullptr);
		if (!NT_SUCCESS(ns))
		{
			if (fPrintInfo) 
				IFERR_LOG(ns, "Can't get parent info for pid: %Iu.\n", (UINT_PTR)nPid);
		}
		else
		{
			pProcessInfo->nParentPid = (HANDLE)BasicInfo.InheritedFromUniqueProcessId;
			pProcessInfo->fIsTerminated = ((ULONG)BasicInfo.ExitStatus) != 259/*STILL_ACTIVE*/;
		}
	}

	if (g_pCommonData->fnZwQueryInformationProcess != nullptr)
	{
		PS_PROTECTION ProtectionInfo = {};
		NTSTATUS ns = g_pCommonData->fnZwQueryInformationProcess(hProcess,
			ProcessProtectionInformation, &ProtectionInfo, sizeof(ProtectionInfo), nullptr);
		if (!NT_SUCCESS(ns))
		{
			if (fPrintInfo)
				IFERR_LOG(ns, "Can't get protection info for pid: %Iu.\n", (UINT_PTR)nPid);
		}
		else
		{
			pProcessInfo->eProtectedType = (PS_PROTECTED_TYPE)ProtectionInfo.Type;
			pProcessInfo->eProtectedSigner = (PS_PROTECTED_SIGNER)ProtectionInfo.Signer;
		}
	}

	NTSTATUS ns = getProcessImageName(hProcess, &pProcessInfo->pusImageName);
	if (!NT_SUCCESS(ns))
	{
		if (fPrintInfo) IFERR_LOG(ns, "Can't getProcessImageName(%Iu)\n", (UINT_PTR)nPid);
		// +FIXME: Why do we clean uninitialized data?
		// What a problem? It is habit from external API usage.
		pProcessInfo->pusImageName = nullptr;
	}

	if (pProcessInfo->pusImageName != nullptr)
	{
		ns = getProcessFlagsByName(pProcessInfo->pusImageName, &pProcessInfo->nFlags);
		if (!NT_SUCCESS(ns))
		{
			if (fPrintInfo) IFERR_LOG(ns, "Can't getProcessFlagsByName(%Iu)\n", (UINT_PTR)nPid);
			pProcessInfo->nFlags = 0;
		}
	}

	if (fPrintInfo)
	{
		LOGINFO2("Process %wZ(%u), parent=%u, flags=0x%08X\n", 
			pProcessInfo->getPrintImageName(), pProcessInfo->nPid, 
			pProcessInfo->nParentPid, pProcessInfo->nFlags);
	}

}

//
//
//
_IRQL_requires_max_(PASSIVE_LEVEL)
void fillProcessInfoByPid(HANDLE nPid, CommonProcessInfo* pProcessInfo, bool fPrintInfo)
{
	HANDLE hProcess = nullptr;

	__try
	{
		pProcessInfo->free();
		pProcessInfo->nPid = nPid;

		// Fills info for SYSTEM(4) process or similar to it
		if (nPid == (HANDLE)c_nSystemProcessPid || nPid == (HANDLE)c_nIdleProcessPid
			|| nPid <= (HANDLE)c_nSystemProcessPid)
		{
			setFlag(pProcessInfo->nFlags, (UINT32)ProcessInfoFlags::SystemLikeProcess);
			return;
		}

		// Open process hProcess
		CLIENT_ID id = {};
		id.UniqueProcess = nPid;
		OBJECT_ATTRIBUTES objAttr = {};
		InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
		NTSTATUS ns = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &objAttr, &id);
		if (!NT_SUCCESS(ns))
		{
			IFERR_LOG(ns, "Can't ZwOpenProcess(%Iu)\n", (UINT_PTR)nPid);
			return;
		}

		fillProcessInfoByHandle(nPid, hProcess, pProcessInfo, fPrintInfo);
		pProcessInfo->nPid = nPid;
	}
	__finally
	{
		if (hProcess != nullptr)
			ZwClose(hProcess);
	}
}

//
//
//
NTSTATUS getProcessTimes(PEPROCESS pProcess, KERNEL_USER_TIMES* pTimes)
{
	if (pTimes == nullptr)
		return LOGERROR(STATUS_INVALID_PARAMETER);
	if (g_pCommonData->fnZwQueryInformationProcess == nullptr)
		return LOGERROR(STATUS_NOT_IMPLEMENTED);

	HANDLE hProcess = nullptr;

	__try
	{
		IFERR_RET(ObOpenObjectByPointer(pProcess, OBJ_KERNEL_HANDLE, NULL, 
			0, *PsProcessType, KernelMode, &hProcess));

		KERNEL_USER_TIMES Times = {};
		IFERR_RET(g_pCommonData->fnZwQueryInformationProcess(hProcess,
			ProcessTimes, &Times, sizeof(Times), nullptr));
		*pTimes = Times;
	}
	__finally
	{
		if (hProcess != nullptr)
			ZwClose(hProcess);
	}

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS getProcessList(HANDLE** ppProcessArray, size_t* pnSize)
{
	// +FIXME: Bad error
	// STATUS_NOT_SUPPORTED ?
	if (g_pCommonData->fnZwQueryInformationProcess == nullptr) 
		return LOGERROR(STATUS_NOT_IMPLEMENTED);
	if (g_pCommonData->fnZwQuerySystemInformation == nullptr) 
		return LOGERROR(STATUS_NOT_IMPLEMENTED);

	PSYSTEM_PROCESS_INFORMATION pSystemProcessInfo = nullptr;
	HANDLE* pProcessArray = nullptr;

	__try
	{
		//
		// Get pProcessInfo
		//

		// Get the size we need
		ULONG nBufferSize = 0;
		NTSTATUS ns = g_pCommonData->fnZwQuerySystemInformation(
			SystemProcessInformation, NULL, 0, &nBufferSize);
		// Must be error STATUS_INFO_LENGTH_MISMATCH
		if (NT_SUCCESS(ns)) 
			return LOGERROR(STATUS_UNSUCCESSFUL);
		if (ns != STATUS_INFO_LENGTH_MISMATCH) IFERR_RET(ns);
	
		// Alloc buffer (for current and possible new processes)
		// +FIXME: Magic values?
		// Yes. See commentary above
		nBufferSize = (nBufferSize + 0x100) * 2;
		pSystemProcessInfo = (PSYSTEM_PROCESS_INFORMATION)ExAllocatePoolWithTag(
			NonPagedPool, nBufferSize, c_nAllocTag);
		if (pSystemProcessInfo == nullptr) 
			return LOGERROR(STATUS_NO_MEMORY);

		// Get data
		// +FIXME: We can get error here it the processes list is changed
		// Can't understand a question.
		IFERR_RET(g_pCommonData->fnZwQuerySystemInformation(
			SystemProcessInformation, pSystemProcessInfo, nBufferSize, nullptr));

		// Get count of processes
		size_t nProcessCount = 0;
		for (PSYSTEM_PROCESS_INFORMATION pInfo = pSystemProcessInfo;
			pInfo != nullptr;
			pInfo = (PSYSTEM_PROCESS_INFORMATION)(pInfo->NextEntryOffset == 0 ? 
				nullptr : ((PCHAR)pInfo) + pInfo->NextEntryOffset))
		{
			nProcessCount++;
		}

		//
		// Create own process list
		//
		
		// Alloc memory for handles
		pProcessArray = (HANDLE*)ExAllocatePoolWithTag(NonPagedPool, 
			nProcessCount * sizeof(HANDLE), c_nAllocTag);
		if (pProcessArray == nullptr) 
			return LOGERROR(STATUS_NO_MEMORY);

		// Log process info
		size_t nIndex = 0;
		for (PSYSTEM_PROCESS_INFORMATION pInfo = pSystemProcessInfo;
			pInfo != nullptr;
			pInfo = (PSYSTEM_PROCESS_INFORMATION)(pInfo->NextEntryOffset == 0 ? 
				nullptr : ((PCHAR)pInfo) + pInfo->NextEntryOffset))
		{
			pProcessArray[nIndex] = pInfo->UniqueProcessId;
			nIndex++;
		}

		*ppProcessArray = pProcessArray;
		*pnSize = nProcessCount;
		pProcessArray = nullptr;
	}
	__finally
	{
		// +FIXME: Can we use RAII wrappers?
		if (pSystemProcessInfo != nullptr)
			ExFreePoolWithTag(pSystemProcessInfo, c_nAllocTag);
		if (pProcessArray != nullptr)
			ExFreePoolWithTag(pProcessArray, c_nAllocTag);
	}

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS getProcessToken(PEPROCESS pProcess, HANDLE* phToken)
{
	HANDLE hProcess = nullptr;

	__try
	{
		IFERR_RET(ObOpenObjectByPointer(pProcess, OBJ_KERNEL_HANDLE, NULL, 
			0, *PsProcessType, KernelMode, &hProcess));
		IFERR_RET(getProcessToken(hProcess, phToken));
	}
	__finally
	{
		if (hProcess != nullptr)
			ZwClose(hProcess);
	}

	return STATUS_SUCCESS;
}


//
//
//
NTSTATUS getProcessToken(HANDLE hProcess, HANDLE* phToken)
{
	return ZwOpenProcessTokenEx(hProcess, 0, OBJ_KERNEL_HANDLE, phToken);
}

} // namespace openEdr
/// @}
