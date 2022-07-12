//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{

#include "iatinject.hpp"

extern "C"
NTKERNELAPI
PVOID
NTAPI
PsGetProcessSectionBaseAddress(
	__in PEPROCESS Process
);

typedef struct _CLR_HEADER
{
	// Header versioning
	ULONG cb;
	USHORT MajorRuntimeVersion;
	USHORT MinorRuntimeVersion;

	// Symbol table and startup information
	IMAGE_DATA_DIRECTORY    MetaData;
	ULONG                   Flags;

	// Followed by the rest of the IMAGE_COR20_HEADER
} CLR_HEADER, *PCLR_HEADER;

static_assert(FIELD_OFFSET(CLR_HEADER, Flags) == FIELD_OFFSET(IMAGE_COR20_HEADER, Flags));

namespace cmd {
namespace kernelInjectLib {

bool IatInjector::initialize(ULONG Flags)
{
	return Injector::initialize(Flags);
}

void IatInjector::onProcessCreate(ULONG ProcessId, ULONG ParentId)
{
	UNREFERENCED_PARAMETER(ParentId);

	IFERR_LOG(addNewImportedDlls(ProcessId), "Failed to add new imported Dlls\r\n");
	return;
}

NTSTATUS IatInjector::getImageInfo(tools::ImageInfo& ImageInfo)
{
	NT_ASSERT(ImageInfo.m_ImageBase < MM_HIGHEST_USER_ADDRESS);
	NT_ASSERT(ImageInfo.m_ImageBase > MM_LOWEST_USER_ADDRESS);

	__try 
	{
		PIMAGE_NT_HEADERS ntHeaders = RtlImageNtHeader(ImageInfo.m_ImageBase);
		if (nullptr == ntHeaders)
			return STATUS_INVALID_IMAGE_FORMAT;

		switch (ntHeaders->OptionalHeader.Subsystem)
		{
		case IMAGE_SUBSYSTEM_NATIVE:
		case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION:
			return STATUS_NOT_SUPPORTED;
		default:
			break;
		}

		ImageInfo.m_Is32bitImage = (IMAGE_NT_OPTIONAL_HDR64_MAGIC != ntHeaders->OptionalHeader.Magic);

		ImageInfo.m_OldImports = (PIMAGE_IMPORT_DESCRIPTOR)RtlImageDirectoryEntryToData(ImageInfo.m_ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImageInfo.m_DescriptorsSize);
		ImageInfo.m_OldImportsSize = ImageInfo.m_DescriptorsSize;
		
		if (nullptr == ImageInfo.m_OldImports)
			return STATUS_INVALID_IMAGE_FORMAT;

		ImageInfo.m_ImageSize = ntHeaders->OptionalHeader.BaseOfCode +
			ntHeaders->OptionalHeader.SizeOfCode + 
			ntHeaders->OptionalHeader.SizeOfInitializedData + 
			ntHeaders->OptionalHeader.SizeOfUninitializedData;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LOGERROR(GetExceptionCode(), "Exception on user VA image access, <%p>\r\n", ImageInfo.m_ImageBase);
		return GetExceptionCode();
	}

#if defined (_WIN64)
	const ULONG DllCount = static_cast<ULONG>(ImageInfo.m_Is32bitImage ? Injector::m_DllList32.getSize() : Injector::m_DllList64.getSize());
#else
	const ULONG DllCount = Injector::m_DllList32.getSize();
#endif

	/// Size of new IMAGE_THUNK_DATA data array for one DLL (2 IMAGE_THUNK_DATA and 2 original IMAGE_THUNK_DATA, the last is zero)
	const USHORT ImageThunkDataArraySize = ImageInfo.m_Is32bitImage ? (4 * sizeof(IMAGE_THUNK_DATA32)) : (4 * sizeof(IMAGE_THUNK_DATA64));

	/// [IMAGE_IMPORT_DESCRIPTOR array + old IMAGE_IMPORT_DESCRIPTOR array][ALIGNMENT][new IMAGE_THUNK_DATA array][new DLL names]
	
	/// total size of new structs IMAGE_IMPORT_DESCRIPTOR, the last is zero 
	ImageInfo.m_DescriptorsSize = ImageInfo.m_DescriptorsSize + (DllCount * sizeof(IMAGE_IMPORT_DESCRIPTOR));

	/// total size of new structs IMAGE_IMPORT_DESCRIPTOR plus new IMAGE_THUNK_DATA array
	ImageInfo.m_AllocationSpaceSize = ALIGN_UP_BY(ImageInfo.m_DescriptorsSize, MEMORY_ALLOCATION_ALIGNMENT) + (DllCount * static_cast<size_t>(ImageThunkDataArraySize));
	
	/// offset to Dll names
	ImageInfo.m_DllNamesLocalOffset = ImageInfo.m_AllocationSpaceSize;

	NT_ASSERT(IS_ALIGNED(ImageInfo.m_DllNamesLocalOffset, MEMORY_ALLOCATION_ALIGNMENT));

#if defined (_WIN64)
	const auto& DllList = ImageInfo.m_Is32bitImage ? Injector::m_DllList32 : Injector::m_DllList64;
#else
	const auto& DllList = Injector::m_DllList32;
#endif

	ImageInfo.m_Dlls.clear();

	SharedLock lock(Injector::m_dllListLock);
	for (const auto& dllName : DllList)
	{
		Blob buffer;
		IFERR_RET_NOLOG(buffer.alloc(static_cast<size_t>(RtlUnicodeStringToAnsiSize(dllName)) + 1));

		ANSI_STRING ansiName = { 0 };
		RtlInitEmptyAnsiString(&ansiName, reinterpret_cast<PCHAR>(buffer.getData()), static_cast<USHORT>(buffer.getSize()));
		IFERR_RET_NOLOG(RtlUnicodeStringToAnsiString(&ansiName, dllName, FALSE));

		ansiName.Buffer[ansiName.Length] = '\0';
		IFERR_RET_NOLOG(ImageInfo.m_Dlls.pushBack(std::move(buffer)));

		ImageInfo.m_AllocationSpaceSize += ALIGN_UP_BY(static_cast<size_t>(ansiName.Length) + 1, MEMORY_ALLOCATION_ALIGNMENT);
	}
	return STATUS_SUCCESS;
}

NTSTATUS IatInjector::allocUserSpaceNearBase32(HANDLE ProcessHandle, PVOID NearBase, PVOID& AllocatedMemory, SIZE_T Size)
{
	MEMORY_BASIC_INFORMATION memInfo = { 0 };
	for (PVOID ptr = NearBase; ; ptr = Add2Ptr(memInfo.BaseAddress, memInfo.RegionSize))
	{
		if ((ptr >= MM_HIGHEST_USER_ADDRESS)
#if defined (_WIN64)
			|| ((reinterpret_cast<ULONG_PTR>(ptr) - reinterpret_cast<ULONG_PTR>(NearBase)) > MAXULONG32)
#endif
			)
		{
			return STATUS_NO_MEMORY;
		}

		IFERR_RET(ZwQueryVirtualMemory(ProcessHandle, ptr, MemoryBasicInformation, &memInfo, sizeof(memInfo), nullptr), "ZwQueryVirtualMemory failed\r\n");
		if (MEM_FREE != memInfo.State)
		{
			memInfo.RegionSize = memInfo.RegionSize ? memInfo.RegionSize : PAGE_SIZE;
			continue;
		}

		if (memInfo.RegionSize != 0 && memInfo.RegionSize < Size)
			continue;

		NT_ASSERT(IS_ALIGNED(memInfo.BaseAddress, PAGE_SIZE));

		for (ptr = memInfo.BaseAddress; ptr < Add2Ptr(memInfo.BaseAddress, memInfo.RegionSize - Size); ptr = Add2Ptr(ptr, PAGE_SIZE))
		{
			NTSTATUS status = ZwAllocateVirtualMemory(ProcessHandle, &ptr, 0, &Size, MEM_RESERVE, PAGE_READWRITE);
			if (!NT_SUCCESS(status))
				continue;

			status = ZwAllocateVirtualMemory(ProcessHandle, &ptr, 0, &Size, MEM_COMMIT, PAGE_READWRITE);
			if (!NT_SUCCESS(status))
			{
				ZwFreeVirtualMemory(ProcessHandle, &ptr, &Size, MEM_RELEASE);
				return status;
			}
			AllocatedMemory = ptr;
			return STATUS_SUCCESS;
		}
	}
}

template<typename Ty_>
NTSTATUS IatInjector::patchNtHeaders(ULONG NewImportsOffset, const tools::ImageInfo& ImageInfo)
{
	/// Make local copy of IMAGE_NT_HEADERS
	Ty_ ntHeaders;
	memcpy(&ntHeaders, RtlImageNtHeader(ImageInfo.m_ImageBase), sizeof(Ty_));

	/// Don't use bound imports
	PIMAGE_DATA_DIRECTORY boundImportDirecory = &ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];
	boundImportDirecory->VirtualAddress = 0;
	boundImportDirecory->Size = 0;

	/// Assign new import directory offset and size
	PIMAGE_DATA_DIRECTORY importDirecory = &ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	importDirecory->VirtualAddress = NewImportsOffset;
	importDirecory->Size = ImageInfo.m_DescriptorsSize;

	/// Ignore CheckSum
	ntHeaders.OptionalHeader.CheckSum = 0;

	/// If the image doesn't have an IMAGE_DIRECTORY_ENTRY_IAT, we should assign it to avoid crash of OS loader.
	for (auto i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++)
	{
		PIMAGE_DATA_DIRECTORY iatDirectory = &ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
		if (iatDirectory->VirtualAddress != 0)
			break;

		IMAGE_SECTION_HEADER imageSectionHeader;
		PIMAGE_DOS_HEADER dosHeader = static_cast<PIMAGE_DOS_HEADER>(ImageInfo.m_ImageBase);

		ULONG sectionOffset = dosHeader->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS32, OptionalHeader) + ntHeaders.FileHeader.SizeOfOptionalHeader;
		RtlCopyMemory(&imageSectionHeader, Add2Ptr(ImageInfo.m_ImageBase, sectionOffset + sizeof(imageSectionHeader) * i), sizeof(imageSectionHeader));

		if (importDirecory->VirtualAddress >= imageSectionHeader.VirtualAddress &&
			importDirecory->VirtualAddress < imageSectionHeader.VirtualAddress + imageSectionHeader.SizeOfRawData)
		{
			iatDirectory->VirtualAddress = imageSectionHeader.VirtualAddress;
			iatDirectory->Size = imageSectionHeader.SizeOfRawData;
		}
	}

	/// Patch .Net image flags
	ULONG size = 0;
	PIMAGE_COR20_HEADER clr = static_cast<PIMAGE_COR20_HEADER>(RtlImageDirectoryEntryToData(ImageInfo.m_ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &size));
	if (clr != nullptr && size >= sizeof(CLR_HEADER))
	{
		/// Make local copy of CLR_HEADER
		CLR_HEADER clrHeader;
		memcpy(&clrHeader, clr, sizeof(clrHeader));

		/// Clear COMIMAGE_FLAGS_ILONLY
		resetFlag<ULONG>(clrHeader.Flags, COMIMAGE_FLAGS_ILONLY);

		/// Write necessary changes to the original IMAGE_COR20_HEADER headers
		IFERR_RET_NOLOG(writeProcessMemory(clr, &clrHeader, sizeof(clrHeader)));
	}

	/// Write necessary changes to the original IMAGE_NT_HEADERS
	IFERR_RET_NOLOG(writeProcessMemory(RtlImageNtHeader(ImageInfo.m_ImageBase), &ntHeaders, sizeof(Ty_)));
	return STATUS_SUCCESS;
}

template<typename Ty_, auto Ordinal>
void IatInjector::writeThunkData(PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, const tools::ImageInfo& ImageInfo, ULONG& ThunkDataLocalOffset, ULONG NewImportsOffset)
{
	ImportDescriptor->OriginalFirstThunk = NewImportsOffset + ThunkDataLocalOffset;

	/// Get pointer to OriginalFirstThunk
	Ty_* thunkData = static_cast<Ty_*>(Add2Ptr(ImageInfo.m_ImageBase, ImportDescriptor->OriginalFirstThunk));
	thunkData[0].u1.Ordinal = Ordinal;
	thunkData[1].u1.AddressOfData = 0;

	ThunkDataLocalOffset += 2 * sizeof(Ty_);
	ImportDescriptor->FirstThunk = NewImportsOffset + ThunkDataLocalOffset;

	/// Get pointer to FirstThunk (IAT array)
	thunkData = static_cast<Ty_*>(Add2Ptr(ImageInfo.m_ImageBase, ImportDescriptor->FirstThunk));
	thunkData[0].u1.Ordinal = Ordinal;
	thunkData[1].u1.AddressOfData = 0;
}

NTSTATUS IatInjector::addNewImportedDlls(HANDLE ProcessHandle, PVOID ImageBase)
{
	/// Query reqired information for new imports
	tools::ImageInfo imageInfo;

	imageInfo.m_ImageBase = ImageBase;
	IFERR_RET_NOLOG(getImageInfo(imageInfo));

	cmd::UserSpaceRegion* memRegion = new (NonPagedPool) cmd::UserSpaceRegion();
	if (nullptr == memRegion)
		return STATUS_INSUFFICIENT_RESOURCES;

	cmd::UserSpaceRegionPtr regionPtr(memRegion);

	memRegion->m_ProcessHandle = ProcessHandle;
	memRegion->m_Size = imageInfo.m_AllocationSpaceSize;

	IFERR_RET_NOLOG(allocUserSpaceNearBase32(ProcessHandle, Add2Ptr(imageInfo.m_ImageBase, imageInfo.m_ImageSize), memRegion->m_BaseAddress, memRegion->m_Size));

#if defined (_WIN64)
	if ((reinterpret_cast<ULONG_PTR>(Add2Ptr(memRegion->m_BaseAddress, imageInfo.m_AllocationSpaceSize)) - reinterpret_cast<ULONG_PTR>(imageInfo.m_ImageBase)) > MAXULONG32)
		return STATUS_NO_MEMORY;
#endif

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	__try
	{
		ProbeForWrite(memRegion->m_BaseAddress, memRegion->m_Size, 1);

		RtlZeroMemory(memRegion->m_BaseAddress, memRegion->m_Size);

		///
		/// [IMAGE_IMPORT_DESCRIPTOR array + old IMAGE_IMPORT_DESCRIPTOR array][ALIGNMENT][new IMAGE_THUNK_DATA array][new DLL names]
		///
		PIMAGE_IMPORT_DESCRIPTOR newImport = static_cast<PIMAGE_IMPORT_DESCRIPTOR>(memRegion->m_BaseAddress);
		const ULONG newImportsOffset = PtrOffset(imageInfo.m_ImageBase, newImport);

		ULONG thunkDataLocalOffset = ALIGN_UP_BY(imageInfo.m_DescriptorsSize, MEMORY_ALLOCATION_ALIGNMENT);

		for (const auto& dllName : imageInfo.m_Dlls)
		{
			NT_ASSERT(IS_ALIGNED(imageInfo.m_DllNamesLocalOffset, MEMORY_ALLOCATION_ALIGNMENT));

			/// Copy new imported DLL name
			status = RtlStringCchCopyA(reinterpret_cast<PSTR>(Add2Ptr(newImport, imageInfo.m_DllNamesLocalOffset)),
				imageInfo.m_AllocationSpaceSize - imageInfo.m_DllNamesLocalOffset, reinterpret_cast<PCSTR>(dllName.getData()));
			if (!NT_SUCCESS(status))
				__leave;

			/// Init IMAGE_IMPORT_DESCRIPTOR
			newImport->Name = newImportsOffset + imageInfo.m_DllNamesLocalOffset;
			newImport->TimeDateStamp = 0;
			
			/// no forwarders
			newImport->ForwarderChain = 0xFFFFFFFFUL;	
	
			/// Get next DLL name offset
			imageInfo.m_DllNamesLocalOffset += static_cast<ULONG>(dllName.getSize()) + 1;
			imageInfo.m_DllNamesLocalOffset = ALIGN_UP_BY(imageInfo.m_DllNamesLocalOffset, MEMORY_ALLOCATION_ALIGNMENT);

			if (imageInfo.m_Is32bitImage)
			{
				writeThunkData<IMAGE_THUNK_DATA32, IMAGE_ORDINAL_FLAG32 | 1>(newImport, imageInfo, thunkDataLocalOffset, newImportsOffset);
			}
#if defined (_WIN64)
			else
			{
				writeThunkData<IMAGE_THUNK_DATA64, IMAGE_ORDINAL_FLAG64 | 1>(newImport, imageInfo, thunkDataLocalOffset, newImportsOffset);
			}
#endif
			newImport++;
		}
		
		RtlCopyMemory(newImport, imageInfo.m_OldImports, imageInfo.m_OldImportsSize);

		if (imageInfo.m_Is32bitImage)
		{
			status = patchNtHeaders<IMAGE_NT_HEADERS32>(newImportsOffset, imageInfo);
			if (!NT_SUCCESS(status))
			{
				LOGERROR(status, "Failed to patch NtHeaders, ImageBase <%p>\r\n", ImageBase);
				__leave;
			}
		}
#if defined (_WIN64)
		else
		{
			status = patchNtHeaders<IMAGE_NT_HEADERS64>(newImportsOffset, imageInfo);
			if (!NT_SUCCESS(status))
			{
				LOGERROR(status, "Failed to patch NtHeaders, ImageBase <%p>\r\n", ImageBase);
				__leave;
			}
		}
#endif
		status = STATUS_SUCCESS;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		status = GetExceptionCode();
		LOGERROR(status, "Exception on user VA image access, <%p>\r\n", ImageBase);
	}

	if (NT_SUCCESS(status))
	{
		// Keep untouched allocated user space memory in case of success
		regionPtr.release();
	}
	return status;
}

NTSTATUS IatInjector::addNewImportedDlls(ULONG ProcessId)
{
	ProcObjectPtr processObject;
	IFERR_RET_NOLOG(PsLookupProcessByProcessId(UlongToHandle(ProcessId), &processObject));

	if (!isAllowDllInjection(processObject, ProcessId))
		return LOGERROR(STATUS_CONTENT_BLOCKED, "Disallow DLLs injection into process <%d>\r\n", ProcessId);

	bool dllSignedProperly = false;
	IFERR_RET(Injector::isDllVerified(dllSignedProperly));

	if (!dllSignedProperly)
		return LOGERROR(STATUS_CONTENT_BLOCKED, "Can't inject DLLs into process <%d>, invalid required signing level\r\n", ProcessId);

	AttachToProcess attach(processObject);
	IFERR_RET_NOLOG(addNewImportedDlls(ZwCurrentProcess(), PsGetProcessSectionBaseAddress(processObject)));

	return STATUS_SUCCESS;
}

/// 
/// [IMPORTANT]
/// Do not use this method if it's necessary to keep the Copy-On-Write mechanism
/// 
NTSTATUS IatInjector::writeProcessMemory(PVOID BaseAddress, PVOID Buffer, SIZE_T Size)
{
	AutoMdlForRead mdl;
	IFERR_RET_NOLOG(mdl.Init(BaseAddress, static_cast<ULONG>(Size)));

	NT_ASSERT(Buffer >= MM_SYSTEM_RANGE_START && Size > 0);

	PVOID buffer = mdl.GetBuffer();
	if (nullptr == buffer)
		return STATUS_UNSUCCESSFUL;

	NT_ASSERT(buffer > MM_SYSTEM_RANGE_START);
	IFERR_RET_NOLOG(MmProtectMdlSystemAddress(mdl, PAGE_READWRITE));

	RtlCopyMemory(buffer, Buffer, Size);
	return STATUS_SUCCESS;
}

} // namespace kernelInjectLib
} // namespace cmd 

/// @}
