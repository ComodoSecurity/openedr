//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (30.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Disk utils (for filemon)
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "osutils.h"
#include "diskutils.h"

#include <Ntddstor.h>

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// USB Utils
//
//////////////////////////////////////////////////////////////////////////

//
//
//
NTSTATUS fillUsbInfoFromContainerId(PCUNICODE_STRING pusContainerId, 
	UsbInfo* pResult)
{
	HANDLE hRegKey = nullptr;
	PVOID pBuffer = nullptr;
	RtlZeroMemory(pResult, sizeof(*pResult));

	__try
	{
		// +FIXME: Why you don't use stack for variable
		// Strong recommended use no more 1 kb of stack in function

		// Alloc string for further operations
		const USHORT nBufferSize = 0x1000;
		pBuffer = ExAllocatePoolWithTag(NonPagedPool, nBufferSize, c_nAllocTag);
		if (pBuffer == NULL) 
			return LOGERROR(STATUS_NO_MEMORY);

		//
		// Open registery key
		//
		UNICODE_STRING usKeyName;
		usKeyName.Buffer = reinterpret_cast<PWCH>(pBuffer);
		usKeyName.MaximumLength = nBufferSize;
		usKeyName.Length = 0;

		// +FIXME: Why do you use g_* prefix for local variables?
		// It is static variables
		//g_usPrefix + pusContainerId + g_usInfix + pusContainerId
		PRESET_STATIC_UNICODE_STRING(g_usPrefix, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\DeviceContainers\\");
		PRESET_STATIC_UNICODE_STRING(g_usInfix, L"\\BaseContainers\\");

		IFERR_RET(RtlUnicodeStringCopy(&usKeyName, &g_usPrefix));
		IFERR_RET(RtlUnicodeStringCat(&usKeyName, pusContainerId));
		IFERR_RET(RtlUnicodeStringCat(&usKeyName, &g_usInfix));
		IFERR_RET(RtlUnicodeStringCat(&usKeyName, pusContainerId));

		OBJECT_ATTRIBUTES Attributes;
		InitializeObjectAttributes(&Attributes, &usKeyName,
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);

		IFERR_RET(ZwOpenKey(&hRegKey, KEY_READ, &Attributes));

		// Get key info
		RtlZeroMemory(pBuffer, nBufferSize);
		ULONG nResultLen = 0;
		IFERR_RET(ZwQueryKey(hRegKey, KeyFullInformation, pBuffer, nBufferSize, &nResultLen));		
		auto pKeyFullInformation = reinterpret_cast<PKEY_FULL_INFORMATION>(pBuffer);
		ULONG nValues = pKeyFullInformation->Values;
		//ULONG nMaxValueNameLen = 0;
		//nMaxValueNameLen = pInfo->MaxValueNameLen;
	
		//
		// Find "USB" value
		//

		WCHAR* sUsbName = nullptr;
		ULONG nUsbNameLength = 0;
		for (ULONG i = 0; i < nValues; i++)
		{
			RtlZeroMemory(pBuffer, nBufferSize);
			nResultLen = 0;
			IFERR_RET(ZwEnumerateValueKey(hRegKey, i, KeyValueBasicInformation, 
				pBuffer, nBufferSize, &nResultLen));
			auto pInfo = reinterpret_cast<PKEY_VALUE_BASIC_INFORMATION>(pBuffer);
			//pInfo->Name
			PRESET_STATIC_UNICODE_STRING(g_usNameInfix, L"USB\\");
			UNICODE_STRING usName;
			usName.Buffer = pInfo->Name;
			#pragma warning(suppress : 26472)
			usName.Length = (USHORT)pInfo->NameLength;
			#pragma warning(suppress : 26472)
			usName.MaximumLength = (USHORT)pInfo->NameLength;
			if (!RtlPrefixUnicodeString(&g_usNameInfix, &usName, TRUE)) continue;

			nUsbNameLength = pInfo->NameLength / sizeof(WCHAR);
			pInfo->Name[nUsbNameLength] = 0;
			sUsbName = pInfo->Name;
			break;
		}

		//
		// Parse Reg value name
		// USB\VID_0BDA&PID_0129\20100201396000000
		//

		// Check format
		if (sUsbName == nullptr ||
			nUsbNameLength < 8 /*USB\VID_*/ + 4 /* %VID% */ + 5 /*&PID_*/ + 4  /* %PID% */ + 1 /* \ */)
			return LOGERROR(STATUS_UNSUCCESSFUL);

		//
		// Extract PID:VID
		//
		UNICODE_STRING usVID;
		usVID.Length = 4 * sizeof(WCHAR);
		usVID.MaximumLength = usVID.Length;
		usVID.Buffer = &sUsbName[8];
		ULONG nVID = 0;
		IFERR_RET(RtlUnicodeStringToInteger(&usVID, 16, &nVID));
		pResult->nVid = static_cast<UINT16>(nVID);

		UNICODE_STRING usPID;
		usPID.Length = 4 * sizeof(WCHAR);
		usPID.MaximumLength = usVID.Length;
		usPID.Buffer = &sUsbName[8 + 4 + 5];
		ULONG nPID = 0;
		IFERR_RET(RtlUnicodeStringToInteger(&usPID, 16, &nPID));
		pResult->nPid = (UINT16)nPID;

		//
		// Extract SN
		//
		static const size_t c_nSnOffset = 8 + 4 + 5 + 4 + 1;

		UNICODE_STRING usSN;
		usSN.Buffer = &sUsbName[c_nSnOffset];
		usSN.Length = (USHORT)((nUsbNameLength - c_nSnOffset) * sizeof(WCHAR));
		usSN.MaximumLength = usSN.Length;

		ANSI_STRING asSN;
		asSN.Buffer = &(pResult->sSN[0]);
		asSN.Length = 0;
		asSN.MaximumLength = sizeof(pResult->sSN) - 1;

		IFERR_RET(RtlUnicodeStringToAnsiString(&asSN, &usSN, FALSE));
		pResult->sSN[asSN.Length] = 0;
	}
	__finally
	{
		if (hRegKey != nullptr)
			ZwClose(hRegKey);

		if (pBuffer != nullptr)
			ExFreePoolWithTag(pBuffer, c_nAllocTag);
	}

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS fillUsbInfo(PCUNICODE_STRING /*pusDiskPdoName*/, 
	PCUNICODE_STRING pusContainerId, UsbInfo* pUsbInfo)
{
	// Win7 - Get via UserMode
	if (!g_pCommonData->fIsWin8orHigher)
	{
		return STATUS_UNSUCCESSFUL;
		// TODO: need request user mode
		//if (pusDiskPdoName == nullptr || pusDiskPdoName->Length == 0) return STATUS_UNSUCCESSFUL;
		//static const LONG c_nGetUsbInfoTimeoutMillisec = 1000; // 1 sec
		//IFERR_RET_NOLOG(fltport::GetUsbInfo(pusDiskPdoName, pUsbInfo, c_nGetUsbInfoTimeoutMillisec));
		//return STATUS_SUCCESS;
	}

	// Win8+ - Get via ContainerId
	if (pusContainerId == nullptr || pusContainerId->Length == 0) return STATUS_UNSUCCESSFUL;
	IFERR_RET(fillUsbInfoFromContainerId(pusContainerId, pUsbInfo), "can't fillUsbInfo");

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Common Utils
//
//////////////////////////////////////////////////////////////////////////

#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS CTL_CODE(0x00000056, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
//
//
typedef struct _DISK_EXTENT
{
	//
	// Specifies the storage device number of
	// the disk on which this extent resides.
	//
	ULONG DiskNumber;

	//
	// Specifies the offset and length of this
	// extent relative to the beginning of the
	// disk.
	//
	LARGE_INTEGER StartingOffset;
	LARGE_INTEGER ExtentLength;

} DISK_EXTENT, *PDISK_EXTENT;

//
//
//
typedef struct _VOLUME_DISK_EXTENTS {

	//
	// Specifies one or more contiguous range
	// of sectors that make up this volume.
	//
	ULONG NumberOfDiskExtents;
	DISK_EXTENT Extents[ANYSIZE_ARRAY];

} VOLUME_DISK_EXTENTS, *PVOLUME_DISK_EXTENTS;

//
//
//
NTSTATUS getVolumePdoObject(PDEVICE_OBJECT pVolumeObject, PDEVICE_OBJECT* ppPDO)
{
	UINT8 Buffer[0x400];
	IFERR_RET(callKernelDeviceIoControl(pVolumeObject, 
		IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, nullptr, 0, Buffer, sizeof(Buffer)));
	auto pVolumeExtents = (PVOLUME_DISK_EXTENTS)Buffer;
	if (pVolumeExtents->NumberOfDiskExtents == 0) return STATUS_UNSUCCESSFUL;

	const ULONG DiskNumber = pVolumeExtents->Extents[0].DiskNumber;
	IFERR_RET(RtlStringCbPrintfW((NTSTRSAFE_PWSTR)Buffer, sizeof(Buffer), 
		L"\\GLOBAL??\\PhysicalDrive%d", DiskNumber));

	UNICODE_STRING strDeviceName;
	RtlInitUnicodeString(&strDeviceName, (NTSTRSAFE_PWSTR)Buffer);

	PFILE_OBJECT pFileObject = nullptr;
	PDEVICE_OBJECT pDeviceObject = nullptr;
	IFERR_RET(IoGetDeviceObjectPointer(&strDeviceName, 0, &pFileObject, &pDeviceObject));

	PDEVICE_OBJECT pLowerDevice = IoGetDeviceAttachmentBaseRef(pDeviceObject);
	ObDereferenceObject(pFileObject);

	*ppPDO = pLowerDevice;
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS getDeviceName(PDEVICE_OBJECT pDeviceObject, PUNICODE_STRING pusResultPdoName)
{
	UINT8 Buffer[0x400];

	//
	// Way 1: IoGetDeviceProperty
	//
	ULONG nResultLength = 0;
	*(WCHAR*)Buffer = 0;
	NTSTATUS ns = IoGetDeviceProperty(pDeviceObject, 
		DevicePropertyPhysicalDeviceObjectName, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
	if (NT_SUCCESS(ns))
	{
		UNICODE_STRING usPdoName;
		RtlInitUnicodeString(&usPdoName, (const WCHAR*)Buffer);
		IFERR_RET(RtlUnicodeStringCopy(pusResultPdoName, &usPdoName));
		return STATUS_SUCCESS;
	}

	//
	// Way 2: ObQueryNameString
	//
	auto pObjectNameInfo = (POBJECT_NAME_INFORMATION)&Buffer[0];
	pObjectNameInfo->Name.MaximumLength = 0;
	pObjectNameInfo->Name.Length = 0;

	ULONG nSize = 0;
	ns = ObQueryNameString(pDeviceObject, pObjectNameInfo, sizeof(Buffer), &nSize);
	if (NT_SUCCESS(ns))
	{
		IFERR_RET(RtlUnicodeStringCopy(pusResultPdoName, &pObjectNameInfo->Name));
		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}

//
// ppBuffer ???
//
NTSTATUS getDiskPdoName(PDEVICE_OBJECT pVolumeDeviceObject, UNICODE_STRING* pDst, 
	PVOID* ppBuffer)
{
	PDEVICE_OBJECT pStoragePdo = nullptr;
	__try
	{
		IFERR_RET(getVolumePdoObject(pVolumeDeviceObject, &pStoragePdo));
	
		UNICODE_STRING usPdoName;
		UINT8 Buffer[0x400] = {};
		usPdoName.Buffer = (PWCH)Buffer;
		usPdoName.MaximumLength = sizeof(Buffer);
		usPdoName.Length = 0;
		IFERR_RET(getDeviceName(pStoragePdo, &usPdoName));
		IFERR_RET(cloneUnicodeString(&usPdoName, pDst, ppBuffer));
	}
	__finally
	{
		if (pStoragePdo != nullptr)
			ObDereferenceObject(pStoragePdo);
	}

	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS getContainerId(PDEVICE_OBJECT pVolumeDeviceObject, UNICODE_STRING* pDst, 
	PVOID* ppBuffer)
{
	UINT8 Buffer[0x400] = {};
	ULONG nResultLength = 0;
	IFERR_RET(IoGetDeviceProperty(pVolumeDeviceObject, DevicePropertyContainerID, 
		sizeof(Buffer), &Buffer, &nResultLength));
	
	UNICODE_STRING usContainerId;
	RtlInitUnicodeString(&usContainerId, (WCHAR*)Buffer);
	if (usContainerId.Length == 0) return STATUS_UNSUCCESSFUL;

	IFERR_RET(cloneUnicodeString(&usContainerId, pDst, ppBuffer)); // &pInstanceContext->usDiskPdoName, pInstanceContext->pDiskPdoNameBuffer));

	return STATUS_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//
// Print info (for research)
//
//////////////////////////////////////////////////////////////////////////

//
//
//
void printDeviceInfo(PDEVICE_OBJECT pDeviceObject)
{
	UINT8 Buffer[0x400];

	// pDeviceObject1
	{
		LOGINFO2("DeviceObject: %p\r\n", pDeviceObject);
		#pragma warning(suppress : 28175)
		LOGINFO2("DriverName: \"%wZ\"\r\n", &pDeviceObject->DriverObject->DriverName);
	}

	// ObQueryNameString
	{
		auto pObjectNameInfo = (POBJECT_NAME_INFORMATION)&Buffer[0];
		pObjectNameInfo->Name.MaximumLength = 0;
		pObjectNameInfo->Name.Length = 0;

		ULONG nSize;
		NTSTATUS ns;
		ns = ObQueryNameString(pDeviceObject, pObjectNameInfo, sizeof(Buffer), &nSize);
		if (NT_ERROR(ns)) { IFERR_LOG(ns, "ObQueryNameString\r\n"); }
		else
		{
			LOGINFO2("ObQueryNameString: \"%wZ\"\r\n", &pObjectNameInfo->Name);
		}
	}

	// pDeviceObject2
	{
		LOGINFO2("Flags:0x%08X Characteristics:0x%08X  DeviceType:0x%08X\r\n",
			pDeviceObject->Flags, /*DO_VERIFY_VOLUME*/
			pDeviceObject->Characteristics, /*FILE_READ_ACCESS*/
			pDeviceObject->DeviceType /*FILE_DEVICE_BEEP*/);
	}

	// IoGetDeviceProperty
	{
		ULONG nResultLength;
		NTSTATUS ns;

		//DevicePropertyHardwareID
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyHardwareID, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			while (*pStr != 0)
			{
				UNICODE_STRING usStr;
				RtlInitUnicodeString(&usStr, &pStr[0]);
				LOGINFO2("DevicePropertyHardwareID: %wZ\r\n", &usStr);
				for (; *pStr != 0; ++pStr); ++pStr;
			}
		}

		//DevicePropertyCompatibleIDs
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyCompatibleIDs, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			while (*pStr != 0)
			{
				UNICODE_STRING usStr;
				RtlInitUnicodeString(&usStr, &pStr[0]);
				LOGINFO2("DevicePropertyCompatibleIDs: %wZ\r\n", &usStr);
				for (; *pStr != 0; ++pStr); ++pStr;
			}
		}

		//DevicePropertyLocationInformation
		*(WCHAR*)Buffer = 0;
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyLocationInformation, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			UNICODE_STRING usStr;
			RtlInitUnicodeString(&usStr, &pStr[0]);
			LOGINFO2("DevicePropertyLocationInformation: %wZ\r\n", &usStr);
		}

		
		//DevicePropertyPhysicalDeviceObjectName
		*(WCHAR*)Buffer = 0;
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyPhysicalDeviceObjectName, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			UNICODE_STRING usStr;
			RtlInitUnicodeString(&usStr, &pStr[0]);
			LOGINFO2("DevicePropertyPhysicalDeviceObjectName: %wZ\r\n", &usStr);
		}


		//DevicePropertyManufacturer
		*(WCHAR*)Buffer = 0;
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyManufacturer, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			UNICODE_STRING usStr;
			RtlInitUnicodeString(&usStr, &pStr[0]);
			LOGINFO2("DevicePropertyManufacturer: %wZ\r\n", &usStr);
		}

		
		//DevicePropertyContainerID
		*(WCHAR*)Buffer = 0;
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyContainerID, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			UNICODE_STRING usStr;
			RtlInitUnicodeString(&usStr, &pStr[0]);
			LOGINFO2("DevicePropertyContainerID: %wZ\r\n", &usStr);
		}

		//DevicePropertyDriverKeyName
		*(WCHAR*)Buffer = 0;
		ns = IoGetDeviceProperty(pDeviceObject, DevicePropertyDriverKeyName, ARRAYSIZE(Buffer), &Buffer, &nResultLength);
		if (NT_ERROR(ns)) { IFERR_LOG(ns); }
		else
		{
			auto pStr = (const WCHAR*)Buffer;
			UNICODE_STRING usStr;
			RtlInitUnicodeString(&usStr, &pStr[0]);
			LOGINFO2("DevicePropertyDriverKeyName: %wZ\r\n", &usStr);
		}
		
	}

	// Get pDeviceDescriptor
	{
		PSTORAGE_DEVICE_DESCRIPTOR pDeviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)&Buffer[0];

		STORAGE_PROPERTY_QUERY PropertyQuery;
		PropertyQuery.PropertyId = StorageDeviceProperty;
		PropertyQuery.QueryType = PropertyStandardQuery;
		const NTSTATUS ns = callKernelDeviceIoControl(pDeviceObject, IOCTL_STORAGE_QUERY_PROPERTY, &PropertyQuery, sizeof(PropertyQuery), pDeviceDescriptor, sizeof(Buffer));
		if (NT_ERROR(ns)) { IFERR_LOG(ns, "Can't get DeviceDescriptor.\r\n"); }
		else
		{
			// Exract USB info
			const CCHAR* pPid = "";
			const CCHAR* pVid = "";
			const CCHAR* pSn = "";
			{
				if (pDeviceDescriptor->VendorIdOffset != 0)
					pVid = (const CCHAR*)pDeviceDescriptor + pDeviceDescriptor->VendorIdOffset;
				if (pDeviceDescriptor->ProductIdOffset != 0)
					pPid = (const CCHAR*)pDeviceDescriptor + pDeviceDescriptor->ProductIdOffset;
				if (pDeviceDescriptor->SerialNumberOffset != 0)
					pSn = (const CCHAR*)pDeviceDescriptor + pDeviceDescriptor->SerialNumberOffset;
			}
			LOGINFO2("DeviceDescriptor: VID:\"%s\" PID:\"%s\" SN:\"%s\"\r\n", pVid, pPid, pSn);
		}
	}

	//// Device stack
	//{
	//	auto pBaseDeviceObject = IoGetDeviceAttachmentBaseRef(pDeviceObject);
	//	if (pBaseDeviceObject != NULL && pBaseDeviceObject != pDeviceObject)
	//	{
	//		LOGINFO2("--- BaseDeviceObject -------\r\n");
	//		printDeviceInfo(pBaseDeviceObject);
	//	}
	//	ObDereferenceObject(pBaseDeviceObject);
	//}

}

//
//
//
void printVolumeInfo(PCFLT_RELATED_OBJECTS FltObjects)
{
	LOGINFO2("=== VolumeInfo =============================================\r\n");

	PDEVICE_OBJECT pDeviceObject = nullptr;
	__try
	{
		// Get pDeviceObject
		{
			IFERR_LOG(FltGetDiskDeviceObject(FltObjects->Volume, &pDeviceObject));
			if (pDeviceObject == nullptr) return;
		}

		LOGINFO2("--- VolumeDevice -----------------\r\n");
		printDeviceInfo(pDeviceObject);

		LOGINFO2("--- StorageDevice ----------------\r\n");
		PDEVICE_OBJECT pPDO = nullptr;
		IFERR_LOG(getVolumePdoObject(pDeviceObject, &pPDO));
		if (pPDO != nullptr)
		{
			printDeviceInfo(pPDO);
		}

	}
	__finally
	{
		if (pDeviceObject != nullptr)
			ObDereferenceObject(pDeviceObject);
	}

	LOGINFO2("===========================================================\r\n");
}


} // namespace cmd
/// @}

