//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "tdiutil.h"

FILE_FULL_EA_INFORMATION UNALIGNED * // Returns: requested attribute or NULL.
tu_findEA(
    PFILE_FULL_EA_INFORMATION startEA,  // First extended attribute in list.
    CHAR * targetName,                   // Name of target attribute.
    USHORT targetNameLength)            // Length of above.
{
   USHORT i;
   BOOLEAN found;
   FILE_FULL_EA_INFORMATION UNALIGNED *currentEA;

   if (!startEA)
   {
      return NULL;
   }

   do
   {
      found = TRUE;

      currentEA = startEA;
      startEA += currentEA->NextEntryOffset;

      if (currentEA->EaNameLength != targetNameLength)
      {
         continue;
      }

      for (i=0; i < currentEA->EaNameLength; i++)
      {
         if (currentEA->EaName[i] == targetName[i])
         {
            continue;
         }
         found = FALSE;
         break;
      }

      if (found)
      {
         return currentEA;
      }
   
   } while (currentEA->NextEntryOffset != 0);

   return NULL;
}

int tu_getListSize(PLIST_ENTRY pList)
{
	int listSize;
	PLIST_ENTRY e;

	listSize = 0;
	e = pList->Flink;
	while (e != pList)
	{
		listSize++;
		e = e->Flink;
	}
	return listSize;
}

unsigned long tu_copyMdlToBuffer(
				PMDL mdl, 
				unsigned long mdlBytes,
				unsigned long mdlOffset,
				char * buffer, 
				unsigned long bufferSize)
{
	PUCHAR			pVA;
	ULONG			nVARemaining;
	ULONG			offset;

	KdPrint((DPREFIX"tu_copyMdlToBuffer\n"));

	if ((mdlBytes == 0) || (bufferSize < mdlBytes))
	{
		return 0;
	}

	offset = 0;

	while (mdl)
	{
		nVARemaining = MmGetMdlByteCount(mdl);

		if (nVARemaining <= mdlOffset)
		{
			mdlOffset -= nVARemaining;
			mdl = mdl->Next;
			continue;
		}

		if (nVARemaining > (mdlBytes + mdlOffset))
		{
			nVARemaining = (mdlBytes + mdlOffset);
		}

		nVARemaining -= mdlOffset;

		pVA = MmGetSystemAddressForMdlSafe(mdl, HighPagePriority);
		if (pVA == NULL)
		{
			break;
		}

		RtlCopyMemory(buffer + offset, pVA + mdlOffset, nVARemaining);

		if (mdlOffset > 0)
		{
			mdlOffset = 0;
		}

		offset += nVARemaining;
		mdlBytes -= nVARemaining;

        if (mdlBytes == 0)
		{
			break;
		}
        
		mdl = mdl->Next;
    }

	return offset;
}


NTSTATUS tu_attachDeviceInternal(PDRIVER_OBJECT DriverObject, 
										  wchar_t * szDevName,
										  PUNICODE_STRING pFilterDevName,
										  PFILE_OBJECT * ppFileObject,
										  PDEVICE_OBJECT * ppDeviceObject,
										  PDEVICE_OBJECT * ppFilterDeviceObject)
{
	NTSTATUS status;
	UNICODE_STRING ustrDevice;
	BOOLEAN bRestoreFlag = FALSE;

	RtlInitUnicodeString(&ustrDevice, szDevName);

	status = IoGetDeviceObjectPointer(
		&ustrDevice,
        FILE_ALL_ACCESS,
        ppFileObject,
        ppDeviceObject);
	
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if ((*ppDeviceObject)->Flags & DO_DEVICE_INITIALIZING)
	{
		bRestoreFlag = TRUE;

		(*ppDeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;
	}
	else
	{
		bRestoreFlag = FALSE;
	}

	status = IoCreateDevice(
		DriverObject,
        0,
		pFilterDevName,
		(*ppDeviceObject)->DeviceType,
		(*ppDeviceObject)->Characteristics,
		FALSE,
		ppFilterDeviceObject);
	
	if (!NT_SUCCESS(status))
	{
		ObDereferenceObject(*ppFileObject);

		*ppFilterDeviceObject = NULL;
		*ppDeviceObject = NULL;

		return status;
	}

	*ppDeviceObject = IoAttachDeviceToDeviceStack(
		*ppFilterDeviceObject,
		*ppDeviceObject);

	if (*ppDeviceObject == NULL)
	{
		ObDereferenceObject(*ppFileObject);
		IoDeleteDevice(*ppFilterDeviceObject);

		*ppFilterDeviceObject = NULL;
		*ppDeviceObject = NULL;

		return STATUS_UNSUCCESSFUL;
	}

	(*ppFilterDeviceObject)->Flags |= (*ppDeviceObject)->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO);
	(*ppFilterDeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;

	if (bRestoreFlag)
	{
		(*ppDeviceObject)->Flags |= DO_DEVICE_INITIALIZING;
	}

	return STATUS_SUCCESS;
}
