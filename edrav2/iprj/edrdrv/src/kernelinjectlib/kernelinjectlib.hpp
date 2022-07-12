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

namespace cmd {
namespace kernelInjectLib {

enum class eInjectorType 
{
	IatInjector = 1,
	ApcInjector,
	UsermodeCallbackInjector, // TODO
};

class IInjector
{
public:
	static IInjector* CreateInstance(eInjectorType Type) noexcept;

public:
	virtual bool initialize(ULONG Flags = 0) = 0;
	virtual void onProcessCreate(ULONG ProcessId, ULONG ParentId) = 0;
	virtual void onProcessTerminate(ULONG ProcessId) = 0;
	virtual void onCreateThread(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) = 0;
	virtual void onImageLoad(PDEVICE_OBJECT DeviceObject, HANDLE ProcessId, PUNICODE_STRING FullImageName, PIMAGE_INFO ImageInfo) = 0;

public:
	virtual void enableDllVerification(SE_SIGNING_LEVEL RequiredLevel) = 0;

	virtual NTSTATUS addSystemDll(const UNICODE_STRING& DllPath) = 0;
	virtual NTSTATUS addDll32(const UNICODE_STRING& DllName32) = 0;
#if defined (_WIN64)
	virtual NTSTATUS addDll64(const UNICODE_STRING& DllName64) = 0;
#endif
	virtual void cleanupDllList() = 0;

	virtual ~IInjector() = 0;
};

using IInjectorPtr = UniquePtr<IInjector>;

} // namespace kernelInjectLib
} // namespace cmd

/// @}
