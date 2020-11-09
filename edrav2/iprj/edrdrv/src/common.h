//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Common header
///
/// @addtogroup edrdrv
/// @{
#pragma once

// Standard header has 4-th level warnings in C++
#pragma warning(push, 3)
#include <suppress.h>
#include <fltKernel.h>
#pragma warning(pop)

#include "kernelcmnlib.hpp"
#include "log.h"
#include "sysapi.h"
#include "stringutils.h"
#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//////////////////////////////////////////////////////////////////////////
//
// User mode communication protocols
//

//
// Includes
//

#define CMD_KERNEL_MODE

typedef uint64_t Enum;

// External libraries
#include <xxhash.hpp>
#include <libsysmon\inc\edrdrvapi.hpp>
#include "lbvsext.h"


namespace openEdr {

using EvFld = edrdrv::EventField;

//////////////////////////////////////////////////////////////////////////
//
// Internal STATUSes
//
//////////////////////////////////////////////////////////////////////////

/// ftlport can't send message because limits
inline constexpr NTSTATUS STATUS_QUEUE_LIMIT_EXEEDED = 0x4F000001;

using namespace openEdr::edrdrv;

//
// Process rules
//

struct ProcessOptionRule
{
	// Conditions
	DynUnicodeString usPathMask;

	// Data
	BOOLEAN m_fValue = FALSE; // Option value
	BOOLEAN fInherit = FALSE; // enable einheritance
	DynUnicodeString usTag; // Tag for deletion
};

typedef List<ProcessOptionRule> ProcessOptionRuleList;

struct ProcessOptionRules
{
	ProcessOptionRuleList tempRules;
	ProcessOptionRuleList persistRules;
};

struct ProcessRules
{
	ProcessOptionRules enableInjectRules;
	ProcessOptionRules sendEventRules;
	ProcessOptionRules trustedRules;
	ProcessOptionRules protectedRules;
};


//
// File rules
//

//
//
//
struct FileRule
{
	// Conditions
	DynUnicodeString usNtPath; //< Prefix
	BOOLEAN fIsDir = FALSE;

	// Data
	edrdrv::AccessType eAccessType = edrdrv::AccessType::NoAccess; //< applied access type
	DynUnicodeString usTag; //< Tag for deletion
};

//
//
//
typedef List<FileRule> FileRuleList;


//
// Registry rules
//

//
//
//
struct RegRule
{
	// Conditions
	DynUnicodeString usPath; //< Prefix
	BOOLEAN fRecursive = FALSE;

	// Data
	edrdrv::AccessType eAccessType = edrdrv::AccessType::NoAccess; //< applied access type
	DynUnicodeString usTag; //< Tag for deletion
};

//
//
//
typedef List<RegRule> RegRuleList;

///
/// Global data for all.
///
struct CommonGlobalData
{
	PDRIVER_OBJECT pDriverObject = nullptr; ///< Driver object
	PFLT_FILTER pFilter = nullptr; ///< FS Filter object

	DynUnicodeString usRegistryPath; ///< Driver registry path

	DynUnicodeString usLogFilepath; ///< Path to log file
		
	// IOCTL Device
	PDEVICE_OBJECT  pIoctlDeviceObject = nullptr; ///< Device for IOCTL
	PUNICODE_STRING pusIoctlDeviceSymlink = nullptr;  ///< Symlink of Device for IOCTL 

	BOOLEAN fIsWin8orHigher = FALSE; ///< Current OS is Win8 or higher

	//
	// Config common information
	//
	KMutex mtxConfig; ///< mutex of processContextList
	uint64_t eChangedConfigFields = 0; ///< Flags for changed (saved) data (bit mask from edrdrv::ConfigField)


	/// Level of logged info (using DbgPrint)
	/// 0 - no log
	/// 1 - Public info (Enduser information)
	/// 2 - Extended info
	/// 3 - Debug info
	/// 3 - Verbose debug info
	AtomicInt<uint32_t> nLogLevel = c_nDefaultLogLevel;

	PDRIVER_UNLOAD pfnSaveDriverUnload = nullptr; ///< Backup of unloadDriver function (while unloading is disabled)

	//
	// Common events info
	//
	AtomicInt<uint64_t> nSentEventsFlags = c_nDefaultEventsFlags; ///< Enable/Disable event sending
	AtomicBool fEnableMonitoring = false; ///< Monitoring is enabled
	AtomicBool fDisableSelfProtection = c_fDefaultDisableSelfProtection; ///< Pause all self protection

	//
	// filemon
	//
	AtomicBool fUseFilemonMask = false; ///< usFilemonMask is set
	FastMutex mtxFilemonMask; ///< mutex of processContextList
	DynUnicodeString usFilemonMask; ///< Filemon mask (postfix)
	AtomicInt<uint64_t> nMinFullActFileSize = c_nDefaultMinFullActFileSize; ///< Min file size to send full read/write
	AtomicInt<uint64_t> nMaxFullActFileSize = c_nDefaultMaxFullActFileSize; ///< Max file size to send full read/write
	FileRuleList fileRules; ///< File self protection rules
	EResource mtxFileRules; ///< RW sync for fileRules

	//
	// fltport
	//
	AtomicBool fFltPortIsInitialized = false; ///< fltport is successfully initialized
	/// Timeout for calling FltSendMessage (milliseconds).
	/// Interval before next FltSendMessage after failed FltSendMessage.
	AtomicInt<uint64_t> nFltPortSendMessageTimeout = c_nSendMsgTimeout;
	AtomicInt<uint64_t> nFltPortConnectionTimeot = c_nConnectionTimeot;  ///< Time interval before stopping monitoring after disconnection (milliseconds).
	AtomicInt<uint32_t> nFltPortMaxQueueSize = c_nMaxQueueSize;
	ULONG_PTR nConnetedProcess = 0; ///< Connected process (white process)

	//
	// injector
	//
	AtomicBool fDllInjectorIsInitialized = false; ///< dllInj is successfully initialized
	AtomicBool fEnableDllInjection = false; ///< Enable DLL injection
	List<DynUnicodeString> injectedDllList;
	AtomicBool fVerifyInjectedDll = true; ///< Enable Dll verification

	//
	// regmon
	//
	AtomicBool fRegFltStarted = FALSE; ///< Registry filter is initialized
	LARGE_INTEGER nRegFltCookie = {}; ///< Records the cookie returned by CmRegisterCallbackEx
	/// Enable/Disable appropriate REG_*
	/// E.g. to enable sent type REG_DWORD set bit with same index (1 << REG_DWORD)
	AtomicInt<uint32_t> nSentRegTypesFlags = c_nDefaultSentRegTypesFlags; 
	AtomicInt<uint32_t> nMaxRegValueSize = c_nDefaultMaxRegValueSize; ///< Max value size to send
	RegRuleList regRules; ///< Registry self protection rules
	EResource mtxRegRules; ///< RW sync for regRules


	//
	// procmon
	//
	AtomicBool fProcCtxNotificationIsStarted = FALSE; ///< procmon is started
	LIST_ENTRY processContextList = {}; ///< List of all active process contexts
	RWSpinLock mtxProcessContextList; ///< mutex of processContextList
	KMutex mtxFillProcessContext; ///< mutex for filling process context
	ProcessRules processRules; ///< Process Rules set
	EResource mtxProcessRules; ///< RW sync for processRules

	//
	// objmon
	//
	AtomicBool fObjMonStarted = FALSE; ///< objmon is initialized
	HANDLE hProcFltCallbackRegistration = nullptr; ///< objmon internal callback
	FastMutex mtxOpenProcessFilter; ///< mutex for procmon::Context::pOpenProcessFilter
	/// Timeout before send same openProcess
	AtomicInt<uint64_t> nOpenProcessRepeatTimeout = c_nOpenProcessRepeatTimeout;
	/// Timeout before send same registry event
	AtomicInt<uint64_t> nRegEventRepeatTimeout = c_nRegEventRepeatTimeout;

	//
	// netmon
	//
	AtomicBool fNetMonStarted = FALSE; ///< netmon is initialized

	//
	// API functions
	//
	PZwQueryInformationProcess fnZwQueryInformationProcess = nullptr;
	PZwQuerySystemInformation fnZwQuerySystemInformation = nullptr;
	PCmCallbackGetKeyObjectIDEx fnCmCallbackGetKeyObjectIDEx = nullptr;
	PCmCallbackReleaseKeyObjectIDEx fnCmCallbackReleaseKeyObjectIDEx = nullptr;

	//
	// Contains resources initialization which can NOT return errors.
	//
	CommonGlobalData() noexcept
	{
		InitializeListHead(&processContextList);
	}

	//
	// Contains resources initialization which can return errors.
	//
	NTSTATUS Initialize()
	{
		IFERR_RET_NOLOG(mtxProcessRules.initialize());
		IFERR_RET_NOLOG(mtxFileRules.initialize());
		IFERR_RET_NOLOG(mtxRegRules.initialize());
		return STATUS_SUCCESS;
	}
};

extern CommonGlobalData *g_pCommonData;

///
/// Start monitoring.
///
NTSTATUS startMonitoring();

///
/// Stop monitoring.
///
void stopMonitoring(bool fResetQueue);

typedef uint64_t SysmonEventMask;


///
/// Create event mask.
///
constexpr SysmonEventMask createEventMask(SysmonEvent eventType)
{
	return 1ui64 << static_cast<SysmonEventMask>(eventType);
}

///
/// Checking that event is enabled to send.
///
inline bool isEventMaskEnabled(SysmonEventMask eventMask)
{
	return (g_pCommonData->nSentEventsFlags & eventMask) != 0;
}

///
/// Checking that event is enabled to send.
///
inline bool isEventEnabled(SysmonEvent eventType)
{
	return isEventMaskEnabled(createEventMask(eventType));
}

} // namespace openEdr

  /// @}

