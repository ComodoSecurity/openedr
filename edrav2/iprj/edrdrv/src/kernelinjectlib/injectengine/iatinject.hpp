//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{

#pragma once
#include "injector.hpp"

namespace cmd {
namespace kernelInjectLib {
namespace tools {

using DllStrings = List<Blob<PoolType::NonPaged> >;

struct ImageInfo
{
	bool m_Is32bitImage = true;
	PVOID m_ImageBase = nullptr;
	ULONG m_ImageSize = 0;
	ULONG m_AllocationSpaceSize = 0;
	ULONG m_DescriptorsSize = 0;
	ULONG m_DllNamesLocalOffset = 0;
	PIMAGE_IMPORT_DESCRIPTOR m_OldImports = nullptr;
	ULONG m_OldImportsSize = 0;
	DllStrings m_Dlls;
};

} // namespace tools

class IatInjector 
	: public Injector
{
public:
	bool initialize(ULONG Flags = 0) override;
	void onProcessCreate(ULONG ProcessId, ULONG ParentId) override;

private:
	NTSTATUS getImageInfo(tools::ImageInfo& ImageInfo);
	NTSTATUS allocUserSpaceNearBase32(HANDLE ProcessHandle, PVOID NearBase, PVOID& AllocatedMemory, SIZE_T Size);
	NTSTATUS addNewImportedDlls(HANDLE ProcessHandle, PVOID ImageBase);
	NTSTATUS addNewImportedDlls(ULONG ProcessId);
	/// 
	/// [IMPORTANT]
	/// Do not use this method if it's necessary to keep the Copy-On-Write mechanism
	/// 
	NTSTATUS writeProcessMemory(PVOID BaseAddress, PVOID Buffer, SIZE_T Size);

	template<typename Ty_, auto Ordinal>
	void writeThunkData(PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor, const tools::ImageInfo& ImageInfo, ULONG& ThunkDataLocalOffset, ULONG NewImportsOffset);

	template<typename Ty_>
	NTSTATUS patchNtHeaders(ULONG NewImportsOffset, const tools::ImageInfo& ImageInfo);
};

} // namespace kernelInjectLib
} // namespace cmd

/// @}