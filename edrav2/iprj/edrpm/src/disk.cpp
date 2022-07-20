//
// edrav2.edrpm project
//
// Author: Denis Kroshin (30.04.2019)
// Reviewer: Denis Bogdanov (05.05.2019)
//
///
/// @file Injection library
///
/// @addtogroup edrpm
/// 
/// Injection library for process monitoring.
/// 
/// @{
#include "pch.h"
#include "winapi.h"
#include "disk.h"

namespace cmd {
namespace edrpm {

#define ObjectNameInformation OBJECT_INFORMATION_CLASS(1)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
typedef struct _OBJECT_NAME_INFORMATION {
	UNICODE_STRING Name;
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

//
//
//
std::wstring getNameByHandleInt(HANDLE nHandle)
{
	DWORD reqSize = 0;
	uint8_t pBuffer[0x400]; // FIXME: Why 0x400?
	auto status = NtQueryObject(nHandle, ObjectNameInformation, pBuffer, DWORD(std::size(pBuffer)), &reqSize);
	if (NT_SUCCESS(status))
	{
		POBJECT_NAME_INFORMATION pNameInfo = POBJECT_NAME_INFORMATION(pBuffer);
		return pNameInfo->Name.Buffer;
	}

	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		// FIXME: is reqSize takes into account the last zero wchar?
		std::vector<uint8_t> pDynBuffer(reqSize, 0);
		if (NT_SUCCESS(NtQueryObject(nHandle, ObjectNameInformation, pDynBuffer.data(), reqSize, NULL)))
		{
			POBJECT_NAME_INFORMATION pNameInfo = POBJECT_NAME_INFORMATION(pDynBuffer.data());
			return pNameInfo->Name.Buffer;
		}
	}
	
	return L"";
}

//
//
//
std::wstring getNameByHandle(HANDLE nHandle)
{
	auto sName = getNameByHandleInt(nHandle);
	if (!sName.empty())
		return sName;

	ScopedHandle dupHandle;
	if (NT_SUCCESS(NtDuplicateObject(NtCurrentProcess(), nHandle, 
		NtCurrentProcess(), &dupHandle, FILE_ANY_ACCESS, FALSE, 0)))
		sName = getNameByHandleInt(nHandle);
	
	return sName;
}

//
//
//
std::wstring getNameByObjectAttributes(POBJECT_ATTRIBUTES pObjAttr)
{
	if (pObjAttr->RootDirectory == NULL && pObjAttr->ObjectName == NULL)
		return L"";
	
	std::wstring sObjName;
	if (pObjAttr->RootDirectory)
	{
		sObjName = getNameByHandle(pObjAttr->RootDirectory);
		sObjName += L"\\";
	}

	if (pObjAttr->ObjectName)
		sObjName += string::makeString(pObjAttr->ObjectName);

	return sObjName;
}

//
//
//
bool isFileAccessChange(ACCESS_MASK DesiredAccess)
{
	if (DesiredAccess & (FILE_WRITE_DATA | FILE_APPEND_DATA))
		return true;
	if (DesiredAccess & (DELETE | WRITE_OWNER | WRITE_DAC | GENERIC_WRITE))
		return true;
	return false;
}

//
//
//
enum class ObjectType : uint32_t
{
	Disk = 0,
	Volume = 1,
	Device = 2,

	Unknown
};

//
// Return object type by name
//
ObjectType getObjectType(const std::wstring& sObjName)
{
	if (string::matchWildcard(sObjName, L"\\Device\\Harddisk*\\dr?") ||
		string::matchWildcard(sObjName, L"\\Device\\PhysicalDrive?") ||
		string::matchWildcard(sObjName, L"\\??\\PhysicalDrive?"))
		//string::matchWildcard(sObjName, L"\\Device\\Harddisk?")
		return ObjectType::Disk;

	if (string::matchWildcard(sObjName, L"\\Device\\HarddiskVolume?") ||
		string::matchWildcard(sObjName, L"\\Device\\HarddiskVolume??") ||
		string::matchWildcard(sObjName, L"\\??\\?:"))
		return ObjectType::Volume;

	if (string::matchWildcard(sObjName, L"\\??\\SCSI#*") ||
		string::matchWildcard(sObjName, L"\\??\\IDE#*") ||
		string::matchWildcard(sObjName, L"\\??\\STORAGE#Volume#*"))
		return ObjectType::Device;

	return ObjectType::Unknown;
}

//
//
//
REGISTER_PRE_CALLBACK(NtCreateFile, [](PHANDLE FileHandle, ACCESS_MASK DesiredAccess, 
	POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions,
	PVOID EaBuffer, ULONG EaLength)
{
	//dbgPrint("Inside NtCreateFile");
	if (ObjectAttributes == NULL || !isFileAccessChange(DesiredAccess))
		return;

	auto sObjName = getNameByObjectAttributes(ObjectAttributes);
	if (sObjName.empty())
		return;

	auto eObjType = getObjectType(sObjName);
	if (eObjType == ObjectType::Unknown)
		return;

	sendEvent(RawEvent::PROCMON_STORAGE_RAW_ACCESS_WRITE, [eObjType, sObjName](auto serializer)
	{
		return (serializer->write(EventField::ObjectType, uint32_t(eObjType)) &&
			serializer->write(EventField::Path, sObjName));
	});
});

//
//
//
REGISTER_PRE_CALLBACK(NtCreateSymbolicLinkObject, [](OUT PHANDLE pHandle, IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes, IN PUNICODE_STRING DestinationName)
{
	std::wstring sDestName(string::makeString(DestinationName));

	auto eObjType = getObjectType(sDestName);
	if (eObjType == ObjectType::Unknown)
		return;

	auto sObjName = getNameByObjectAttributes(ObjectAttributes);
	if (sObjName.empty())
		return;

	sendEvent(RawEvent::PROCMON_STORAGE_RAW_LINK_CREATE, [eObjType, sObjName, sDestName](auto serializer)
	{
		return (serializer->write(EventField::ObjectType, uint32_t(eObjType)) &&
			serializer->write(EventField::Path, sObjName) &&
			serializer->write(EventField::Link, sDestName));
	});
});

} // namespace edrpm
} // namespace cmd