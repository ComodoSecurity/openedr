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
#include "common.h"
#include "osutils.h"
#include <ntimage.h>
#include "filemon.h"
#include "procmon.h"
#include "petools.hpp"
#include "apcqueue.hpp"
//#include "threadqueue.hpp"
#include "..\kernelinjectlib.hpp"

namespace cmd {
namespace kernelInjectLib {
namespace tools {

//
// Cpmmon helper routines
//
inline NTSTATUS getObjectNamePathFinalComponent(PCUNICODE_STRING NamePath, PUNICODE_STRING FinalComponent)
{
	if (!ARGUMENT_PRESENT(NamePath) || NamePath->Length < sizeof(WCHAR))
		return STATUS_OBJECT_NAME_INVALID;

	USHORT charsMaximumLength = NamePath->Length / sizeof(WCHAR) - 1;
	for (USHORT charsNameLength = 0;
		charsNameLength < charsMaximumLength;
		charsNameLength++)
	{
		PWCH buffer = &NamePath->Buffer[charsMaximumLength - charsNameLength];

		if (OBJ_NAME_PATH_SEPARATOR == (*buffer))
		{
			FinalComponent->Buffer = buffer + 1;
			FinalComponent->Length = charsNameLength * sizeof(WCHAR);
			FinalComponent->MaximumLength = FinalComponent->Length;

			return STATUS_SUCCESS;
		}
	}
	return STATUS_OBJECT_NAME_INVALID;
}

inline NTSTATUS getObjectNamePathParentComponent(PCUNICODE_STRING NamePath, PUNICODE_STRING ParentComponent, PUNICODE_STRING FinalComponent = nullptr)
{
	UNICODE_STRING finalComponent = {0};
	NTSTATUS status = getObjectNamePathFinalComponent(NamePath, &finalComponent);
	if (!NT_SUCCESS(status))
		return status;

	if (NamePath->Length > finalComponent.Length)
	{
		ParentComponent->Buffer = NamePath->Buffer;
		ParentComponent->Length = NamePath->Length - finalComponent.Length - sizeof(OBJ_NAME_PATH_SEPARATOR);
		ParentComponent->MaximumLength = ParentComponent->Length;
		if (ARGUMENT_PRESENT(FinalComponent))
		{
			*FinalComponent = finalComponent;
		}
		return STATUS_SUCCESS;
	}
	return STATUS_OBJECT_NAME_INVALID;
}

inline bool isNameInExpression(const UNICODE_STRING& Expression, const UNICODE_STRING& Name)
{
	bool result = false;
	__try
	{
		result = !!FsRtlIsNameInExpression(const_cast<PUNICODE_STRING>(&Expression), const_cast<PUNICODE_STRING>(&Name), TRUE, NULL);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return result;
}

class UserSpaceMemory
{
	HANDLE m_processHandle = ZwCurrentProcess();
	PVOID m_baseAddress = nullptr;
	SIZE_T m_size = 0;
	ULONG m_currentOffset = 0;
public:
	UserSpaceMemory() = default;
	UserSpaceMemory(UserSpaceMemory& src) = delete;
	UserSpaceMemory(UserSpaceMemory&& src)
		: m_processHandle(src.m_processHandle)
		, m_baseAddress(src.m_baseAddress)
		, m_size(src.m_size)
		, m_currentOffset(src.m_currentOffset)
	{
		src.m_processHandle = nullptr;
		src.m_baseAddress = nullptr;
		src.m_currentOffset = 0;
		src.m_size = 0;
	};
	explicit UserSpaceMemory(HANDLE ProcessHandle);
	~UserSpaceMemory();

	PVOID alloc(SIZE_T Size);
	const PVOID getPtr() const { return m_baseAddress;  };
	const SIZE_T getSize() const { return m_size; };
	PVOID assign(PVOID BaseAddress, SIZE_T Size = 0);
#if defined (_WIN64)
	PVOID64 getUlong64Ptr(ULONG64 Data);
	PUNICODE_STRING64 getUnicodeString64(PCUNICODE_STRING SourceString);
#endif
	PUNICODE_STRING32 getUnicodeString32(PCUNICODE_STRING SourceString);
	PVOID32 getUlong32Ptr(ULONG32 Data);
	PVOID detach()
	{
		PVOID ptr = m_baseAddress;
		m_baseAddress = nullptr;
		return ptr;
	}
private:
	void free();
};

} // namespace tools

namespace imageInfo
{

[[nodiscard]] inline bool isImportantSystemDll(const UNICODE_STRING& ImageName)
{
	constexpr UNICODE_STRING systemDllList[] =
	{
		RTL_CONSTANT_STRING(L"NTDLL.DLL"),
		RTL_CONSTANT_STRING(L"KERNEL32.DLL"),
		RTL_CONSTANT_STRING(L"USER32.DLL"),
	};

	for (const auto& systemDll : systemDllList)
	{
		if (RtlEqualUnicodeString(&ImageName, &systemDll, TRUE))
			return true;
	}
	return false;
}

class MappedImageInfo
{
	DynUnicodeString m_imageName;
	PVOID  m_imageBase = nullptr;
	SIZE_T m_imageSize = 0;
	bool m_isWow64Image = false;
public:
	MappedImageInfo() = default;
	~MappedImageInfo() = default;
	MappedImageInfo(MappedImageInfo& src) = delete;
	MappedImageInfo(MappedImageInfo&& src) 
		: m_imageName(std::move(src.m_imageName))
		, m_imageBase(src.m_imageBase)
		, m_imageSize(src.m_imageSize)
		, m_isWow64Image(src.m_isWow64Image)
	{
		src.m_imageBase = nullptr;
		src.m_imageSize = 0;
	};

	[[nodiscard]] NTSTATUS init(const UNICODE_STRING& ImageName, const PVOID ImageBase, SIZE_T ImageSize, bool IsWow64Image = false);
	[[nodiscard]] operator const DynUnicodeString&() const { return m_imageName; }
	[[nodiscard]] const PVOID GetImageBase() const { return m_imageBase; };
	[[nodiscard]] const SIZE_T GetImageSize() const { return m_imageSize; };
	[[nodiscard]] const bool IsImageWow64() const { return m_isWow64Image; };
};

//
// Basic image information initialization
//
[[nodiscard]] inline NTSTATUS MappedImageInfo::init(const UNICODE_STRING& ImageName, const PVOID ImageBase, SIZE_T ImageSize, bool IsWow64Image)
{
	IFERR_RET_NOLOG(m_imageName.assign(&ImageName));
	m_imageBase = ImageBase;
	m_imageSize = ImageSize;
	m_isWow64Image = IsWow64Image;
	return STATUS_SUCCESS;
}

//
// List of MappedImageInfo for differnent image information in process
//
using MappedImageList = List<MappedImageInfo>;

//
// This class uses common mapped images information for differnent processes
//
class ProcessDllInfo
{
	ULONG m_processId;
	MappedImageList m_dllList;
	EResource m_dllListLock; ///< RW sync for m_dllList

	LONG m_firstThread = 0;
public:
	ProcessDllInfo() = delete;
	ProcessDllInfo(ProcessDllInfo& src) = delete;
	explicit ProcessDllInfo(ULONG ProcessId) : m_processId(ProcessId) {};
	ProcessDllInfo(ProcessDllInfo&& src)
		: m_processId(src.m_processId)
		, m_dllList(std::move(src.m_dllList))
	{
		src.m_dllList.clear();
		src.m_processId = 0;
	}
	~ProcessDllInfo() { m_dllList.clear(); };

	[[nodiscard]] NTSTATUS initialize() { return m_dllListLock.initialize(); };
	[[nodiscard]] const ULONG GetProcessId() const { return m_processId; };
	[[nodiscard]] const PVOID GetImageBase(const UNICODE_STRING& ImageName, bool IsWow64Image = false) const;
	[[nodiscard]] bool IsFirstThreadStarted() { return !!InterlockedCompareExchange(&m_firstThread, TRUE, FALSE); };

	NTSTATUS AddMappedImage(const PVOID ImageBase, const SIZE_T ImageSize, const UNICODE_STRING& ImagePath);
};

[[nodiscard]] inline const PVOID ProcessDllInfo::GetImageBase(const UNICODE_STRING& ImageName, bool IsWow64Image) const
{
	SharedLock lock(const_cast<cmd::EResource&>(m_dllListLock));
	for (auto& dll : m_dllList)
	{
		const DynUnicodeString& imageName = dll;
		if (!imageName.isEqual(&ImageName, true) || (dll.IsImageWow64() != IsWow64Image))
			continue;

		return dll.GetImageBase();
	}
	return nullptr;
}

inline NTSTATUS ProcessDllInfo::AddMappedImage(const PVOID ImageBase, const SIZE_T ImageSize, const UNICODE_STRING& ImagePath)
{
	MappedImageInfo imageInfo;
	IFERR_RET(imageInfo.init(ImagePath, ImageBase, ImageSize, isWow64Process(m_processId)), "Failded to regiter mapped image %wZ, process=%d\r\n", &ImagePath, m_processId);

	UniqueLock lock(m_dllListLock);
	IFERR_RET(m_dllList.pushBack(std::move(imageInfo)));
	return STATUS_SUCCESS;
}

//
// List of ProcessDllInfo for differnent processes
//
using ProcessList = List<ProcessDllInfo>;

} // end of namespace imageInfo

typedef List<DynUnicodeString> DllList;

//
// This class uses common routines for differnent injection methods
//
class Injector : public IInjector
{
protected:
	//
	// List of mapped system images, e.g. NTDLL, KERNEL32, USER32
	// 
	EResource m_processListLock; ///< RW sync for m_processList
	imageInfo::ProcessList m_processList;
	
	[[nodiscard]] const PVOID getProcAddress(const ULONG ProcessId, const UNICODE_STRING& ImageName, const ANSI_STRING& ProcName) const;
	[[nodiscard]] bool isDllMapped(const ULONG ProcessId, const UNICODE_STRING& ImageName) const;
public:
	//
	// Basic class initialization
	// 
	virtual bool initialize(ULONG Flags) override;

	//
	// Callback for CreateProcessNotification
	// 
	virtual void onProcessCreate(ULONG ProcessId, ULONG ParentId) override;

	//
	// Callback for CreateProcessNotification (process terminattion)
	// 
	virtual void onProcessTerminate(ULONG ProcessId) override;

	//
	// Callback for CreateThreadNotification 
	// 
	virtual void onCreateThread(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create) override;

	//
	// Callback for LoadImageNotification, should store information about system mapped images, such as NDLL, KERNEL32 and USER32
	// 
	virtual void onImageLoad(PDEVICE_OBJECT DeviceObject, HANDLE ProcessId, PUNICODE_STRING FullImageName, PIMAGE_INFO ImageInfo) override;

public:
	//
	// This method DLL verifocation for injected DLL list
	// 
	virtual void enableDllVerification(SE_SIGNING_LEVEL RequiredLevel) override;
	
	//
	// This method add DLL from WOW64 or SYSTEM32 DLL path to corresponding injected list
	// 
	virtual NTSTATUS addSystemDll(const UNICODE_STRING& DllPath) override;
	virtual NTSTATUS addDll32(const UNICODE_STRING& DllName32) override;
#if defined (_WIN64)
	virtual NTSTATUS addDll64(const UNICODE_STRING& DllName64) override;
#endif
	virtual void cleanupDllList() override;

	virtual ~Injector() override;

protected:
	EResource m_dllListLock; ///< RW sync for m_dllList
	//
	// This list contains 32 bit DLL list that to injected
	// 
	DllList m_DllList32;

	//
	// This list contains 64 bit DLL list that to injected
	// 
#if defined (_WIN64)
	DllList m_DllList64;
#endif
	//
	// This list contains full DLL path list for signing verification
	// 
	DllList m_DllPathList;
	
	bool m_DllVerified = false;
	bool m_DllSignedProperly = false;

	SE_SIGNING_LEVEL getRequiredSignatureLevel() const { return m_RequiredSignatureLevel; };
	NTSTATUS isDllVerified(bool &isSignedProperly);
	bool isAllowDllInjection(PEPROCESS Process, ULONG ProcessId) const;

	bool shouldDoInject(__pre_except_maybenull __out PETHREAD& ThreadObject, const HANDLE ProcessId, const HANDLE ThreadId, const BOOLEAN Create);
private:
	SE_SIGNING_LEVEL m_RequiredSignatureLevel = SE_SIGNING_LEVEL_UNSIGNED;

	NTSTATUS addDll(const UNICODE_STRING& DllName, DllList& DllList);
};

inline NTSTATUS Injector::addDll(const UNICODE_STRING& DllName, DllList& DllList)
{
	DynUnicodeString dynUnicodeString;
	IFERR_RET(dynUnicodeString.assign(&DllName), "Can't add Dll <%wZ>\r\n", &DllName);
	
	UniqueLock lock(m_dllListLock);
	IFERR_RET(DllList.pushBack(std::move(dynUnicodeString)));
	return STATUS_SUCCESS;
}

inline NTSTATUS Injector::addDll32(const UNICODE_STRING& DllName32)
{
	return this->addDll(DllName32, this->m_DllList32);
}

#if defined (_WIN64)
inline NTSTATUS Injector::addDll64(const UNICODE_STRING& DllName64)
{
	return this->addDll(DllName64, this->m_DllList64);
}
#endif

} // namespace kernelInjectLib
} // namespace cmd

/// @}