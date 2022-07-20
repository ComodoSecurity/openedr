//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{

#include "common.h"
#include <ntimage.h>
#include "petools.hpp"

typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
	HANDLE Section;
	PVOID MappedBase;
	PVOID ImageBase;
	ULONG ImageSize;
	ULONG Flags;
	USHORT LoadOrderIndex;
	USHORT InitOrderIndex;
	USHORT LoadCount;
	USHORT OffsetToFileName;
	UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
	ULONG NumberOfModules;
	RTL_PROCESS_MODULE_INFORMATION Modules[ANYSIZE_ARRAY];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;

namespace cmd {
namespace petools {

[[nodiscard]] inline bool isAddressInRange(const PVOID Address, const PVOID RangeStart, const ULONG RangeSize)
{
	return (Address >= RangeStart) && (Address < Add2Ptr(RangeStart, RangeSize));
}

[[nodiscard]] inline bool equalStrings(__in_ecount_z(MaxCount) const char* String1, __in_ecount_z(MaxCount) const char* String2, __in size_t MaxCount)
{
	return (0 == _strnicmp(String1, String2, MaxCount));
}

[[nodiscard]] PVOID getSystemModuleHandle(const ANSI_STRING& ModuleName)
{
	UNREFERENCED_PARAMETER(ModuleName);

	PAGED_CODE();

	if (nullptr == g_pCommonData->fnZwQuerySystemInformation)
	{
		LOGERROR(STATUS_PROCEDURE_NOT_FOUND, "ZwQuerySystemInformation not found\r\n");
		return nullptr;
	}

	ULONG bufferLength = 0;
	NTSTATUS status = g_pCommonData->fnZwQuerySystemInformation(SystemModuleInformation, &bufferLength, 0, &bufferLength);
	if ((STATUS_BUFFER_TOO_SMALL != status && STATUS_INFO_LENGTH_MISMATCH != status) || (0 == bufferLength))
	{
		LOGERROR(status, "ZwQuerySystemInformation failed\r\n");
		return nullptr;
	}

	cmd::Blob buffer;
	status = buffer.alloc(bufferLength);
	if (!NT_SUCCESS(status))
	{
		LOGERROR(status, "Failed to allocate user-memory\r\n");
		return nullptr;
	}

	PRTL_PROCESS_MODULES modules = reinterpret_cast<PRTL_PROCESS_MODULES>(buffer.getData());
	status = g_pCommonData->fnZwQuerySystemInformation(SystemModuleInformation, modules, bufferLength, nullptr);
	if (!NT_SUCCESS(status))
	{
		LOGERROR(status, "ZwQuerySystemInformation failed\r\n");
		return nullptr;
	}

	PVOID imageBase = nullptr;
	PRTL_PROCESS_MODULE_INFORMATION moduleInfo = modules->Modules;
	for (ULONG i = 0; i < modules->NumberOfModules; i++)
	{
		PCCH dllName = (PCCH)Add2Ptr(moduleInfo[i].FullPathName, moduleInfo[i].OffsetToFileName);
		if (!equalStrings(dllName, ModuleName.Buffer, ModuleName.MaximumLength))
			continue;

		imageBase = moduleInfo[i].ImageBase;
		break;
	}

	return imageBase;
}

[[nodiscard]] PVOID getSystemModuleHandle(const UNICODE_STRING& ModuleName)
{
	ANSI_STRING ansiName = { 0 };
	SIZE_T size = RtlUnicodeStringToAnsiSize(&ModuleName);
	
	Blob buffer;
	NTSTATUS status = buffer.alloc(size + 1);
	if (!NT_SUCCESS(status))
	{
		LOGERROR(STATUS_INSUFFICIENT_RESOURCES, "Failed to allocate memory for %wZ\r\n", ModuleName);
		return nullptr;
	}

	RtlInitEmptyAnsiString(&ansiName, reinterpret_cast<PCHAR>(buffer.getData()), static_cast<USHORT>(buffer.getSize()));
	status = RtlUnicodeStringToAnsiString(&ansiName, &ModuleName, FALSE);
	if (!NT_SUCCESS(status))
	{
		LOGERROR(status, "Failed to convert DLL name: %wZ\r\n", ModuleName);
		return nullptr;
	}

	return getSystemModuleHandle(ansiName);
}

[[nodiscard]] PVOID getProcAddress(const PVOID ImageBase, const ANSI_STRING& ProcName)
{
	NT_ASSERT(ARGUMENT_PRESENT(ImageBase) && ProcName.Buffer != nullptr && ProcName.Length != 0);

	__try
	{
		ULONG exportSize = 0;
		const PIMAGE_EXPORT_DIRECTORY exportDirectory = static_cast<PIMAGE_EXPORT_DIRECTORY>(RtlImageDirectoryEntryToData(
			ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &exportSize));
		
		if (nullptr == exportDirectory)
		{
			LOGERROR(STATUS_INVALID_IMAGE_FORMAT, "Invalid export directory for image %p\r\n", ImageBase);
			return nullptr;
		}

		const PULONG nameTableBase = static_cast<PULONG>Add2Ptr(ImageBase, exportDirectory->AddressOfNames);
		NT_ASSERTMSG("Invalid names table address in exports", isAddressInRange(reinterpret_cast<PVOID>(nameTableBase), exportDirectory, exportSize));

		for (ULONG hintIndex = 0; hintIndex < exportDirectory->NumberOfNames; hintIndex++)
		{
			const PSTR currentName = static_cast<PSTR>(Add2Ptr(ImageBase, nameTableBase[hintIndex]));
			NT_ASSERTMSG("Invalid name address in exports", isAddressInRange(reinterpret_cast<PVOID>(currentName), exportDirectory, exportSize));

			const ULONG maxLength = PtrOffset(currentName, Add2Ptr(exportDirectory, exportSize));
			if (!equalStrings(currentName, ProcName.Buffer, min(maxLength, ProcName.MaximumLength)))
				continue;

			const PUSHORT ordinalsTableBase = static_cast<PUSHORT>(Add2Ptr(ImageBase, exportDirectory->AddressOfNameOrdinals));
			NT_ASSERTMSG("Invalid ordinals table address in exports", isAddressInRange(reinterpret_cast<PVOID>(ordinalsTableBase), exportDirectory, exportSize));
			
			const USHORT ordinal = ordinalsTableBase[hintIndex];
			const PULONG addressTableBase = static_cast<PULONG>Add2Ptr(ImageBase, exportDirectory->AddressOfFunctions);
			NT_ASSERTMSG("Invalid address table in exports", isAddressInRange(reinterpret_cast<PVOID>(addressTableBase), exportDirectory, exportSize));

			return Add2Ptr(ImageBase, addressTableBase[ordinal]);
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LOGERROR(GetExceptionCode(), "getProcAddress exception\r\n");
		return nullptr;
	}
	return nullptr;
}

} // namespace petools
} // namespace cmd

/// @}