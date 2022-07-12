//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{

#include "apcinjector.hpp"

namespace cmd {
namespace kernelInjectLib {

bool ApcInjector::initialize(ULONG Flags)
{
	return Injector::initialize(Flags);
}

void ApcInjector::onProcessCreate(ULONG ProcessId, ULONG ParentId)
{
	Injector::onProcessCreate(ProcessId, ParentId);
}

void ApcInjector::onProcessTerminate(ULONG ProcessId)
{
	Injector::onProcessTerminate(ProcessId);

	freeMemory(ProcessId);
	return;
}

void ApcInjector::freeMemory(ULONG ProcessId)
{
	UniqueLock memoryLock(m_memoryListLock);
	if (m_pendingFreelist.isEmpty())
		return;

	for (auto memDescriptor = m_pendingFreelist.begin(); memDescriptor != m_pendingFreelist.end(); ++memDescriptor)
	{
		if (memDescriptor->GetProcessId() != ProcessId)
			continue;

		m_pendingFreelist.remove(memDescriptor);
		break;
	}
	return;
}

void ApcInjector::doInject(PRKTHREAD Thread)
{
	PEPROCESS process = IoThreadToProcess(Thread);
#if defined (_WIN64)
	if (!isWow64Process(IoThreadToProcess(Thread)))
	{
		apcQueue::PAPC_CALLBACK fnLoadLibraryExW = (apcQueue::PAPC_CALLBACK)Injector::getProcAddress(HandleToUlong(PsGetProcessId(process)),
			RTL_CONSTANT_STRING(L"KERNEL32.DLL"), RTL_CONSTANT_STRING("LoadLibraryExW"));

		if (nullptr == fnLoadLibraryExW)
		{
			LOGERROR(STATUS_PROCEDURE_NOT_FOUND, "Failed to find LoadLibraryW address\r\n");
			return;
		}
		
		SharedLock lock(Injector::m_dllListLock);
		for (auto& dll : Injector::m_DllList64)
		{
			tools::UserSpaceMemory memory;
			if (nullptr == memory.alloc(dll.getLengthCb() + sizeof(UNICODE_NULL)))
				continue;

			if (!NT_SUCCESS(RtlStringCbCopyNW((PWSTR)memory.getPtr(), memory.getSize(), dll.getBuffer(), dll.getLengthCb())))
				continue;

			apcQueue::ApcQueue apcQueue;
			apcQueue.initialize(Thread, fnLoadLibraryExW, memory.getPtr());
			if (apcQueue.insert(nullptr, UlongToPtr(LOAD_IGNORE_CODE_AUTHZ_LEVEL | LOAD_LIBRARY_SEARCH_SYSTEM32)))
			{
				ProcessMemoryDescriptor memDescriptor(HandleToUlong(PsGetProcessId(process)), memory);
				UniqueLock memoryLock(m_memoryListLock);
				IFERR_LOG(m_pendingFreelist.pushBack(std::move(memDescriptor)));
			}
		}
	}
	else
#endif
	{
		apcQueue::PAPC_CALLBACK fnLoadLibraryExW = (apcQueue::PAPC_CALLBACK)PtrToPtr32(Injector::getProcAddress(HandleToUlong(PsGetProcessId(process)),
			RTL_CONSTANT_STRING(L"KERNEL32.DLL"), RTL_CONSTANT_STRING("LoadLibraryExW")));

		if (nullptr == fnLoadLibraryExW)
		{
			LOGERROR(STATUS_PROCEDURE_NOT_FOUND, "Failed to find LoadLibraryW address\r\n");
			return;
		}
		
		SharedLock lock(Injector::m_dllListLock);
		for (auto& dll : Injector::m_DllList32)
		{
			tools::UserSpaceMemory memory;
			if (nullptr == memory.alloc(dll.getLengthCb() + sizeof(UNICODE_NULL)))
				continue;

			if (!NT_SUCCESS(RtlStringCbCopyNW((PWSTR)memory.getPtr(), memory.getSize(), dll.getBuffer(), dll.getLengthCb())))
				continue;

			PVOID fnLoadLibraryWow64ExW = fnLoadLibraryExW;
			PVOID apcContext = memory.getPtr();

			apcQueue::ApcQueue apcQueue;

			wrapApcWow64Thread(&apcContext, &fnLoadLibraryWow64ExW);
			apcQueue.initialize(Thread, (apcQueue::PAPC_CALLBACK)fnLoadLibraryWow64ExW, apcContext);
			if (apcQueue.insert(nullptr, UlongToPtr(LOAD_IGNORE_CODE_AUTHZ_LEVEL | LOAD_LIBRARY_SEARCH_SYSTEM32)))
			{
				ProcessMemoryDescriptor memDescriptor(HandleToUlong(PsGetProcessId(process)), memory);
				UniqueLock memoryLock(m_memoryListLock);
				IFERR_LOG(m_pendingFreelist.pushBack(std::move(memDescriptor)));
			}
		}
	}
}

void ApcInjector::onCreateThread(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ThreadId);
	UNREFERENCED_PARAMETER(Create);

	if (!Create)
	{
		freeMemory(HandleToUlong(ProcessId));
	}
}

void ApcInjector::onImageLoad(PDEVICE_OBJECT DeviceObject, HANDLE ProcessId, PUNICODE_STRING FullImageName, PIMAGE_INFO ImageInfo)
{
	Injector::onImageLoad(DeviceObject, ProcessId, FullImageName, ImageInfo);

	if (!Injector::isDllMapped(HandleToUlong(ProcessId), RTL_CONSTANT_STRING(L"KERNEL32.DLL")))
		return;

	ThreadObjectPtr threadObject;
	if (!Injector::shouldDoInject((PETHREAD&)threadObject, ProcessId, PsGetCurrentThreadId(), TRUE))
		return;

	this->doInject(threadObject);
}

} // namespace kernelInjectLib
} // namespace cmd

/// @}