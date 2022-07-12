//
// edrav2.edrdrv project
//
// Author: Denis Shavrovskiy (15.03.2021)
// Reviewer:
//
///
/// @addtogroup kernelinjectlib
/// @{
/// 

#include "iatinject.hpp"
//#include "usercallback.hpp"
#include "apcinjector.hpp"

namespace cmd {
namespace kernelInjectLib {
namespace tools {

UserSpaceMemory::UserSpaceMemory(HANDLE ProcessHandle)
	: m_processHandle(ProcessHandle)
{
}

UserSpaceMemory::~UserSpaceMemory()
{
	free();
}

PVOID UserSpaceMemory::alloc(SIZE_T Size)
{
	NT_ASSERT(Size);
	NT_ASSERT(m_processHandle);

	free();
	IFERR_LOG(ZwAllocateVirtualMemory(m_processHandle, &m_baseAddress, 0, &Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE), "Failed to allocate user memory\r\n");
	m_size = Size;
	return m_baseAddress;
}

void UserSpaceMemory::free()
{
	if (m_baseAddress)
	{
		NT_ASSERT(m_processHandle);
		ZwFreeVirtualMemory(m_processHandle, &m_baseAddress, &m_size, MEM_RELEASE);
		m_baseAddress = nullptr;
	}
}
#if defined (_WIN64)
PVOID64 UserSpaceMemory::getUlong64Ptr(ULONG64 Data)
{
	__try
	{
		PVOID64 pointer64 = Add2Ptr(m_baseAddress, m_currentOffset);
		memcpy(pointer64, &Data, sizeof(Data));
		m_currentOffset += sizeof(ULONG64);
		return pointer64;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LOGERROR(GetExceptionCode(), "Exception during access user VA memory\r\n");
	}
	return nullptr;
}

PUNICODE_STRING64 UserSpaceMemory::getUnicodeString64(PCUNICODE_STRING SourceString)
{
	__try
	{
		PUNICODE_STRING result = (PUNICODE_STRING)Add2Ptr(m_baseAddress, m_currentOffset);
		result->MaximumLength = SourceString->Length + sizeof(UNICODE_NULL);
		result->Buffer = (PWCH)(result + 1);

		m_currentOffset += (result->MaximumLength + sizeof(UNICODE_STRING64));

		RtlCopyUnicodeString(result, SourceString);
		return (PUNICODE_STRING64)result;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LOGERROR(GetExceptionCode(), "Exception during access user VA memory\r\n");
	}
	return nullptr;
}
#endif

PUNICODE_STRING32 UserSpaceMemory::getUnicodeString32(PCUNICODE_STRING SourceString)
{
	__try
	{
		PUNICODE_STRING32 result = (PUNICODE_STRING32)PtrToPtr32(Add2Ptr(m_baseAddress, m_currentOffset));
		result->MaximumLength = SourceString->Length + sizeof(UNICODE_NULL);
		result->Length = SourceString->Length;

		PWCH buffer = (PWCH)(result + 1);
		result->Buffer = (ULONG)PtrToPtr32(buffer);
		RtlStringCbCopyNW(buffer, result->MaximumLength, SourceString->Buffer, SourceString->Length);

		m_currentOffset += (result->MaximumLength + sizeof(UNICODE_STRING32));
		return result;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LOGERROR(GetExceptionCode(), "Exception during access user VA memory\r\n");
	}
	return nullptr;
}

PVOID32 UserSpaceMemory::getUlong32Ptr(ULONG32 Data)
{
	__try
	{
		PVOID32 pointer32 = PtrToPtr32(Add2Ptr(m_baseAddress, m_currentOffset));
		memcpy(pointer32, &Data, sizeof(Data));
		m_currentOffset += sizeof(ULONG32);
		return pointer32;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		LOGERROR(GetExceptionCode(), "Exception during access user VA memory\r\n");
	}
	return nullptr;
}

PVOID UserSpaceMemory::assign(PVOID BaseAddress, SIZE_T Size)
{
	free();

	this->m_baseAddress = BaseAddress;
	this->m_size = Size;
	return this->m_baseAddress;
}

} //namespace tools

bool Injector::initialize(ULONG Flags)
{
	UNREFERENCED_PARAMETER(Flags);

	NTSTATUS status = m_processListLock.initialize();
	if (!NT_SUCCESS(status))
	{
		LOGERROR(status, "Failed to initialize synch\r\n");
		return false;
	}
	
	status = m_dllListLock.initialize();
	if (!NT_SUCCESS(status))
	{
		LOGERROR(status, "Failed to initialize synch\r\n");
		return false;
	}
	return true;
}

void Injector::onProcessCreate(ULONG ProcessId, ULONG ParentId)
{
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ParentId);
	return;
}

void Injector::onProcessTerminate(ULONG ProcessId)
{
	UniqueLock lock(m_processListLock);
	for (auto process = m_processList.begin(); process != m_processList.end(); ++process)
	{
		if (process->GetProcessId() != ProcessId)
			continue;

		m_processList.remove(process);
		break;
	}
	return;
}

bool Injector::shouldDoInject(__pre_except_maybenull __out PETHREAD& ThreadObject, const HANDLE ProcessId, const HANDLE ThreadId, const BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ThreadId);
	UNREFERENCED_PARAMETER(ProcessId);

	if (!Create)
		return false;

	ThreadObjectPtr threadObject;
	NTSTATUS status = PsLookupThreadByThreadId(ThreadId, &threadObject);
	if (!NT_SUCCESS(status))
	{
		LOGERROR(status, "Failed to lookup thread object for thread id %x\r\n", HandleToUlong(ThreadId));
		return false;
	}

	PEPROCESS processObject = IoThreadToProcess(threadObject);
	if (!isAllowDllInjection(processObject, HandleToUlong(PsGetProcessId(processObject))))
		return false;

	const ULONG threadProcessId = HandleToUlong(PsGetProcessId(processObject));
	
	SharedLock lock(m_processListLock);
	for (auto& process : m_processList)
	{
		if (process.GetProcessId() != threadProcessId)
			continue;

		if (process.IsFirstThreadStarted())
			return false;
		
		break;
	}

#if (DBG)
	//if (isWow64Process(processObject))
	{
		const UNICODE_STRING masks[] =
		{
			RTL_CONSTANT_STRING(L"*\\TASKMGR.EXE"),
			//RTL_CONSTANT_STRING(L"*\\EXCEL.EXE"),
		};

		procmon::ContextPtr context;
		if (NT_SUCCESS(procmon::getProcessContext(ProcessId, context)) &&
			context->processInfo.pusImageName)
		{
			for (const auto& mask : masks)
			{
				if (tools::isNameInExpression(mask, *context->processInfo.pusImageName))
				{
					ThreadObject = (PETHREAD)threadObject.detach();
					return true;
				}
			}
			return false;
		}
	}
#else
	ThreadObject = (PETHREAD)threadObject.detach();
	return true;
#endif
	return false;
}

void Injector::onCreateThread(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ProcessId);
	UNREFERENCED_PARAMETER(ThreadId);
	UNREFERENCED_PARAMETER(Create);
	return;
}

void Injector::onImageLoad(PDEVICE_OBJECT DeviceObject, HANDLE ProcessId, PUNICODE_STRING FullImageName, PIMAGE_INFO ImageInfo)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(FullImageName);

	NT_ASSERT(ImageInfo->ExtendedInfoPresent);
	const auto imageInfoEx = CONTAINING_RECORD(ImageInfo, IMAGE_INFO_EX, ImageInfo);

	NT_ASSERT(HandleToUlong(ProcessId) != 0 && ProcessId != PsGetProcessId(PsInitialSystemProcess));

	FltInstanceDeref instance;
	IFERR_LOG_RET(filemon::tools::getInstanceFromFileObject(imageInfoEx->FileObject, &instance), "Failed to get Instance from FILE_OBJECT\r\n");

	FltFileNameInfoDeref fileNameInfo;
	IFERR_LOG_RET(FltGetFileNameInformationUnsafe(imageInfoEx->FileObject, instance, FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP | FLT_FILE_NAME_NORMALIZED, &fileNameInfo));
	IFERR_LOG_RET(FltParseFileNameInformation(fileNameInfo));

	if (!imageInfo::isImportantSystemDll(fileNameInfo->FinalComponent))
		return;

	DynUnicodeString fullPath;
	if (isWow64Process(HandleToUlong(ProcessId)))
	{
		DECLARE_CONST_UNICODE_STRING(substring, L"\\SysWOW64\\");
		IFERR_LOG_RET(fullPath.concatenate(g_pCommonData->systemRootDirectory, &substring, &fileNameInfo->FinalComponent));
		if (!fullPath.isEqual(&fileNameInfo->Name, true))
			return;
	}
	else
	{
		DECLARE_CONST_UNICODE_STRING(substring, L"\\System32\\");
		IFERR_LOG_RET(fullPath.concatenate(g_pCommonData->systemRootDirectory, &substring, &fileNameInfo->FinalComponent));
		if (!fullPath.isEqual(&fileNameInfo->Name, true))
			return;
	}

	UniqueLock lock(m_processListLock);
	for (auto& process : m_processList)
	{
		if (HandleToUlong(ProcessId) != process.GetProcessId())
			continue;

		IFERR_LOG_RET(process.AddMappedImage(ImageInfo->ImageBase, ImageInfo->ImageSize, fileNameInfo->FinalComponent), "Failed to add mapped image %wZ\r\n", &fileNameInfo->Name);
	}

	imageInfo::ProcessDllInfo newItem(HandleToUlong(ProcessId));
	IFERR_LOG_RET(newItem.initialize(), "Failed to initialize synch\r\n");
	IFERR_LOG_RET(newItem.AddMappedImage(ImageInfo->ImageBase, ImageInfo->ImageSize, fileNameInfo->FinalComponent), "Failed to add mapped image %wZ\r\n", &fileNameInfo->Name);
	IFERR_LOG_RET(m_processList.pushBack(std::move(newItem)), "Failed to add mapped image %wZ\r\n", &fileNameInfo->Name);
	return;
}

bool Injector::isDllMapped(const ULONG ProcessId, const UNICODE_STRING& ImageName) const
{
	SharedLock lock(const_cast<cmd::EResource&>(m_processListLock));
	for (const auto& process : m_processList)
	{
		if (ProcessId != process.GetProcessId())
			continue;

		PVOID imageBase = process.GetImageBase(ImageName, isWow64Process(ProcessId));
		if (imageBase > MM_LOWEST_USER_ADDRESS && imageBase < MM_HIGHEST_USER_ADDRESS)
			return true;
	}
	return false;
}

[[nodiscard]] const PVOID Injector::getProcAddress(const ULONG ProcessId, const UNICODE_STRING& ImageName, const ANSI_STRING& ProcName) const
{
	SharedLock lock(const_cast<cmd::EResource&>(m_processListLock));
	for (const auto& process : m_processList)
	{
		if (ProcessId != process.GetProcessId())
			continue;

		PVOID imageBase = process.GetImageBase(ImageName, isWow64Process(ProcessId));
		if (imageBase < MM_LOWEST_USER_ADDRESS || imageBase >= MM_HIGHEST_USER_ADDRESS)
			return nullptr;

		return petools::getProcAddress(imageBase, ProcName);
	}
	return nullptr;
}

void Injector::cleanupDllList()
{
	UniqueLock lock(m_dllListLock);

	m_DllList32.clear();

#if defined (_WIN64)
	m_DllList64.clear();
#endif

	return;
}

void Injector::enableDllVerification(SE_SIGNING_LEVEL RequiredLevel)
{
	m_RequiredSignatureLevel = RequiredLevel;
	return;
}

//
// This method adds the DLL to the injected list 
// 
NTSTATUS Injector::addSystemDll(const UNICODE_STRING& DllPath)
{
	UNICODE_STRING parentComponent = { 0 }, finalComponent = { 0 };
	IFERR_RET(tools::getObjectNamePathParentComponent(&DllPath, &parentComponent, &finalComponent), "Invalid Dll path <%wZ>\r\n", (PCUNICODE_STRING)&DllPath);

	DynUnicodeString fullDllPath;
	IFERR_RET(fullDllPath.assign(&DllPath), "Insufficient resources for Dll <%wZ>\r\n", (PCUNICODE_STRING)&DllPath);
	IFERR_RET(m_DllPathList.pushBack(std::move(fullDllPath)), "Insufficient resources for Dll <%wZ>\r\n", (PCUNICODE_STRING)&DllPath);

	if (tools::isNameInExpression(RTL_CONSTANT_STRING(L"*\\WINDOWS\\SYSTEM32"), parentComponent))
	{
#if defined (_WIN64)
		return this->addDll64(finalComponent);
#else
		return this->addDll32(finalComponent);
#endif
	}
#if defined (_WIN64)
	else if (tools::isNameInExpression(RTL_CONSTANT_STRING(L"*\\WINDOWS\\SYSWOW64"), parentComponent))
	{
		return this->addDll32(finalComponent);
	}
#endif
	return STATUS_OBJECT_NAME_INVALID;
}

//
// This method check the DLL if it's signed properly for injection
// 
NTSTATUS Injector::isDllVerified(bool& isSignedProperly)
{
#if (!DBG)
	isSignedProperly = this->m_DllSignedProperly;
	if (this->m_DllVerified)
	{
		return STATUS_SUCCESS;
	}

	if (this->getRequiredSignatureLevel() <= SE_SIGNING_LEVEL_UNSIGNED)
	{
		this->m_DllVerified = true;
		this->m_DllSignedProperly = true;
		isSignedProperly = this->m_DllSignedProperly;
		return STATUS_SUCCESS;
	}
	
	SharedLock lock(m_dllListLock);
	for (const auto& dllPath : m_DllPathList)
	{
		if (dllPath.getLengthCb() < sizeof(L"X:\\"))
			return LOGERROR(STATUS_OBJECT_PATH_INVALID, "DLL <%wZ> path invalid\r\n", (PCUNICODE_STRING)&dllPath);

		DECLARE_CONST_UNICODE_STRING(prefix, L"\\??\\");

		DynUnicodeString dynBuffer;
		IFERR_RET(dynBuffer.alloc(prefix.Length + dllPath.getLengthCb() + sizeof(UNICODE_NULL)),
			"Insufficient resources for DLL <%wZ> verification\r\n", (PCUNICODE_STRING)&dllPath);

		UNICODE_STRING targetDll = dynBuffer;
		if (':' == dllPath.getBuffer()[1])
		{
			RtlCopyUnicodeString(&targetDll, &prefix);
		}
		IFERR_RET_NOLOG(RtlAppendUnicodeStringToString(&targetDll, (PCUNICODE_STRING)&dllPath));

		SE_SIGNING_LEVEL signatureLevel = 0;
		IFERR_RET(cmd::getModuleSignatureLevel(targetDll, signatureLevel), "Failed to verify DLL <%wZ> signing level\r\n", (PCUNICODE_STRING)&dllPath);
		if (signatureLevel < this->getRequiredSignatureLevel())
		{
			this->m_DllVerified = true;
			this->m_DllSignedProperly = false;
			isSignedProperly = this->m_DllSignedProperly;
			return STATUS_SUCCESS;
		}
	}

	this->m_DllVerified = true;
	this->m_DllSignedProperly = true;
	isSignedProperly = this->m_DllSignedProperly;
	return STATUS_SUCCESS;
#else
	isSignedProperly = true;
	return STATUS_SUCCESS;
#endif
}

Injector::~Injector()
{
	m_DllList32.clear();

#if defined (_WIN64)
	m_DllList64.clear();
#endif
}


bool Injector::isAllowDllInjection(PEPROCESS Process, ULONG ProcessId) const
{
	if (cmd::isProtectedProcess(Process) || cmd::isProtectedProcessLight(Process))
	{
		LOGINFO4("Skip injection into protected process PID= %d\r\n", ProcessId);
		return false;
	}

	if (SE_SIGNING_LEVEL_UNSIGNED == Injector::getRequiredSignatureLevel())
		return true;

	if (isSystemLuidProcess(Process))
	{
		SE_SIGNING_LEVEL sectionSignatureLevel = 0,
		processSignatureLevel = cmd::getProcessSignatureLevel(Process, &sectionSignatureLevel);

		if (processSignatureLevel >= Injector::getRequiredSignatureLevel())
		{
			LOGINFO4("Skip injection into system process PID= %d, signatureLevel= 0x%08X\r\n", ProcessId, processSignatureLevel);
			return false;
		}

		if (processSignatureLevel > Injector::getRequiredSignatureLevel())
		{
			LOGINFO4("Skip injection into system process PID= %d, signatureLevel= 0x%08X, requiredSignatureLevel= 0x%08X\r\n",
				ProcessId, processSignatureLevel, Injector::getRequiredSignatureLevel());

			return false;
		}

		FileObjectPtr fileObject;
		IFERR_RET(PsReferenceProcessFilePointer(Process, &fileObject), "PsReferenceProcessFilePointer failed\r\n");

		if (NT_SUCCESS(cmd::getProcessModuleSignatureLevel(Process, fileObject, processSignatureLevel)))
		{
			if (processSignatureLevel >= Injector::getRequiredSignatureLevel())
			{
				LOGINFO4("Skip injection into system process PID= %d, signatureLevel= 0x%08X\r\n", ProcessId, processSignatureLevel);
				return false;
			}
		}
	}
	return true;
}

IInjector* IInjector::CreateInstance(eInjectorType Type) noexcept
{
	switch (Type)
	{
	case eInjectorType::IatInjector:
	{
		auto ptr = new (NonPagedPool) IatInjector();
		return ptr;
	}
#if 0 // TODO: _KTHREAD should have valid _KTRAP_FRAME for KeUserModeCallback execution 
	case eInjectorType::UsermodeCallbackInjector:
	{
		auto ptr = new (NonPagedPool) UserCallbackInjector();
		return ptr;
	}
#endif 
	case eInjectorType::ApcInjector:
	{
		auto ptr = new (NonPagedPool) ApcInjector();
		return ptr;
	}
	default:
		break;
	}
	return nullptr;
}

IInjector::~IInjector()
{
}

} // namespace kernelInjectLib
} // namespace cmd 
