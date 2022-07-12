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
#pragma once

//////////////////////////////////////////////////////////////////////////
//
// USB Utils
//
//////////////////////////////////////////////////////////////////////////


namespace cmd {

// To unify alignment for kernel and user mode (1 byte)
#pragma pack(push, 1)

///
/// USB device information
///
struct UsbInfo
{
	static const size_t c_nMaxSerialNumber = 256;

	UINT16 nVid;
	UINT16 nPid;
	char sSN[c_nMaxSerialNumber];
};

#pragma pack( pop )

///
/// Get USB information.
///
/// @param pusDiskPdoName[in] - PDO name (can be NULL).
/// @param pusContainerId[in] - container Id (can be NULL).
/// @param pUsbInfo[out] - USB info.
/// @return The function returns NTSTATUS of the operation.
///
NTSTATUS fillUsbInfo(PCUNICODE_STRING pusDiskPdoName, 
	PCUNICODE_STRING pusContainerId, UsbInfo* pUsbInfo);


///
/// Get Disk PDO by VolumeObject.
///
/// @param pVolumeObject[in] - volume object.
/// @param ppPDO[out] - PDO. It must be released by ObDereferenceObject.
/// @return The function returns NTSTATUS of the operation.
///
NTSTATUS getVolumePdoObject(PDEVICE_OBJECT pVolumeObject, PDEVICE_OBJECT* ppPDO);

///
/// Get DiskPDO name (Allocate new buffer).
///
/// @param pVolumeObject[in] - volume object.
/// @param pDst[out] - result string.
/// @param ppBuffer[out] - memory allocated for result string. It must be free by ExFreePoolWith.
/// @return The function returns NTSTATUS of the operation.
///
NTSTATUS getDiskPdoName(PDEVICE_OBJECT pVolumeObject, UNICODE_STRING* pDst, 
	PVOID* ppBuffer);

///
/// Get ContainerId (Allocate new buffer).
///
/// @param pVolumeObject[in] - volume object.
/// @param pDst[out] - result string.
/// @param ppBuffer[out] - memory allocated for result string. It must be free by ExFreePoolWith.
/// @return The function returns NTSTATUS of the operation.
///
NTSTATUS getContainerId(PDEVICE_OBJECT pVolumeObject, UNICODE_STRING* pDst, 
	PVOID* ppBuffer);

///
/// Print volume information into the log.
///
/// @param FltObjects - TBD
///
void printVolumeInfo(PCFLT_RELATED_OBJECTS FltObjects);

} // namespace cmd
/// @}
