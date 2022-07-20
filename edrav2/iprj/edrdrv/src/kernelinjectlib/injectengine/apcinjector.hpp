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

class ApcInjector
	: public Injector
{
private:
	//
	// List of UserSpaceMemory, that should be freed after injection is done
	//
	class ProcessMemoryDescriptor
	{
		ULONG m_processId;
		tools::UserSpaceMemory m_memory;
	public:
		ProcessMemoryDescriptor() = delete;
		ProcessMemoryDescriptor(ProcessMemoryDescriptor& src) = delete;
		ProcessMemoryDescriptor(ProcessMemoryDescriptor&& src)
			: m_processId(src.m_processId)
			, m_memory(std::move(src.m_memory))
		{
		};
		ProcessMemoryDescriptor(ULONG ProcessId, tools::UserSpaceMemory& Memory)
			: m_processId(ProcessId)
			, m_memory(std::move(Memory))
		{
		};
		~ProcessMemoryDescriptor() {};
		const ULONG GetProcessId() const { return m_processId; };
	};

	using UserSpaceMemoryList = List<ProcessMemoryDescriptor>;

	FastMutex m_memoryListLock; ///< RW sync for m_pendingFreelist
	UserSpaceMemoryList m_pendingFreelist;

	void doInject(PRKTHREAD Thread);
	void freeMemory(ULONG ProcessId);

public:
	bool initialize(ULONG Flags = 0) override;
	void onProcessCreate(ULONG ProcessId, ULONG ParentId) override;
	void onProcessTerminate(ULONG ProcessId) override;
	void onCreateThread(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) override;
	void onImageLoad(PDEVICE_OBJECT DeviceObject, HANDLE ProcessId, PUNICODE_STRING FullImageName, PIMAGE_INFO ImageInfo) override;

}; //class ApcInjector

} // namespace kernelInjectLib
} // namespace cmd

/// @}