//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _TDIUTIL_H
#define _TDIUTIL_H


FILE_FULL_EA_INFORMATION UNALIGNED * // Returns: requested attribute or NULL.
tu_findEA(
    PFILE_FULL_EA_INFORMATION startEA,  // First extended attribute in list.
    CHAR * targetName,                   // Name of target attribute.
    USHORT targetNameLength            // Length of above.
	);


/**
* Returns the number of elements in pList
* @return int 
* @param pList 
**/
int tu_getListSize(PLIST_ENTRY pList);

/**
* Copy the specified number of bytes from mdl to buffer
* @return int Number of bytes copied
* @param mdl Source mdl
* @param mdlBytes Number of bytes to copy
* @param mdlOffset Mdl offset
* @param buffer Destination
* @param bufferSize Size of buffer
**/
unsigned long tu_copyMdlToBuffer(
				PMDL mdl, 
				unsigned long mdlBytes,
				unsigned long mdlOffset,
				char * buffer, 
				unsigned long bufferSize);

/**
*	Creates a device named pFilterDevName and attaches it to device szDevName
**/
NTSTATUS tu_attachDeviceInternal(PDRIVER_OBJECT DriverObject, 
										  wchar_t * szDevName,
										  PUNICODE_STRING pFilterDevName,
										  PFILE_OBJECT * ppFileObject,
										  PDEVICE_OBJECT * ppDeviceObject,
										  PDEVICE_OBJECT * ppFilterDeviceObject);


#endif