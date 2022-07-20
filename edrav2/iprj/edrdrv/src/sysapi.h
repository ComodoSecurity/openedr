//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (24.03.2019)
// Reviewer: 
//
///
/// @file Native API declaration
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace cmd {

//
// ZwQueryInformationProcess needs dynamic linking
//
typedef NTSTATUS (NTAPI *PZwQueryInformationProcess)(
	_In_      HANDLE           ProcessHandle,
	_In_      PROCESSINFOCLASS ProcessInformationClass,
	_Out_     PVOID            ProcessInformation,
	_In_      ULONG            ProcessInformationLength,
	_Out_opt_ PULONG           ReturnLength
	);

//
// ZwQuerySystemInformation needs dynamic linking
//
typedef NTSTATUS (NTAPI *PZwQuerySystemInformation)(
	ULONG  SystemInformationClass,
	PVOID  SystemInformation,
	ULONG  SystemInformationLength,
	PULONG ReturnLength
	);

#define ObjectNameInformation  1
#define _SystemProcessInformation 5

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation,             // 0x002C
	SystemProcessorInformation,         // 0x000C
	SystemPerformanceInformation,       // 0x0138
	SystemTimeInformation,              // 0x0020
	SystemPathInformation,              // not implemented
	SystemProcessInformation,           // 0x00F8+ per process
	SystemCallInformation,              // 0x0018 + (n * 0x0004)
	SystemConfigurationInformation,     // 0x0018
	SystemProcessorCounters,            // 0x0030 per cpu
	SystemGlobalFlag,                   // 0x0004
	SystemInfo10,                       // not implemented
	SystemModuleInformation,            // 0x0004 + (n * 0x011C)
	SystemLockInformation,              // 0x0004 + (n * 0x0024)
	SystemInfo13,                       // not implemented
	SystemPagedPoolInformation,         // checked build only
	SystemNonPagedPoolInformation,      // checked build only
	SystemHandleInformation,            // 0x0004  + (n * 0x0010)
	SystemObjectInformation,            // 0x0038+ + (n * 0x0030+)
	SystemPagefileInformation,          // 0x0018+ per page file
	SystemInstemulInformation,          // 0x0088
	SystemInfo20,                       // invalid info class
	SystemCacheInformation,             // 0x0024
	SystemPoolTagInformation,           // 0x0004 + (n * 0x001C)
	SystemProcessorStatistics,          // 0x0000, or 0x0018 per cpu
	SystemDpcInformation,               // 0x0014
	SystemMemoryUsageInformation1,      // checked build only
	SystemLoadImage,                    // 0x0018, set mode only
	SystemUnloadImage,                  // 0x0004, set mode only
	SystemTimeAdjustmentInformation,    // 0x000C, 0x0008 writeable
	SystemMemoryUsageInformation2,      // checked build only
	SystemInfo30,                       // checked build only
	SystemInfo31,                       // checked build only
	SystemCrashDumpInformation,         // 0x0004
	SystemExceptionInformation,         // 0x0010
	SystemCrashDumpStateInformation,    // 0x0008
	SystemDebuggerInformation,          // 0x0002
	SystemThreadSwitchInformation,      // 0x0030
	SystemRegistryQuotaInformation,     // 0x000C
	SystemLoadDriver,                   // 0x0008, set mode only
	SystemPrioritySeparationInformation,// 0x0004, set mode only
	SystemInfo40,                       // not implemented
	SystemInfo41,                       // not implemented
	SystemInfo42,                       // invalid info class
	SystemInfo43,                       // invalid info class
	SystemTimeZoneInformation,          // 0x00AC
	SystemLookasideInformation,         // n * 0x0020

// info classes specific to Windows 2000
// WTS = Windows Terminal Server

	SystemSetTimeSlipEvent,             // set mode only
	SystemCreateSession,                // WTS, set mode only
	SystemDeleteSession,                // WTS, set mode only
	SystemInfo49,                       // invalid info class
	SystemRangeStartInformation,        // 0x0004
	SystemVerifierInformation,          // 0x0068
	SystemAddVerifier,                  // set mode only
	SystemSessionProcessesInformation,  // WTS
}  SYSTEM_INFORMATION_CLASS, * PSYSTEM_INFORMATION_CLASS, ** PPSYSTEM_INFORMATION_CLASS;

static_assert(SystemProcessInformation == _SystemProcessInformation);

#pragma pack(push, 1)
typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG NextEntryOffset;
	UCHAR Reserved1[52];
	PVOID Reserved2[3];
	HANDLE UniqueProcessId;
	PVOID Reserved3;
	ULONG HandleCount;
	UCHAR Reserved4[4];
	PVOID Reserved5[11];
	SIZE_T PeakPagefileUsage;
	SIZE_T PrivatePageCount;
	LARGE_INTEGER Reserved6[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;
#pragma pack(pop)

//
// CmCallbackGetKeyObjectIDEx is Win8+ routine
//
typedef NTSTATUS(NTAPI *PCmCallbackGetKeyObjectIDEx)(
	_In_ PLARGE_INTEGER Cookie,
	_In_ PVOID Object,
	_Out_opt_ PULONG_PTR ObjectID,
	_Outptr_opt_ PCUNICODE_STRING *ObjectName,
	_In_ ULONG Flags
	);

//
// CmCallbackReleaseKeyObjectIDEx is Win8+ routine
//
typedef VOID(NTAPI *PCmCallbackReleaseKeyObjectIDEx)(
	_In_ PCUNICODE_STRING ObjectName
	);

//
// Info for ZwQueryInformationProcess(ProcessProtectionInformation)
//
#pragma warning(push)
// warning disabling was copied from Windows SDK headers
#pragma warning(disable:4201) // nameless struct/union.

#pragma pack (push, 1)

typedef struct _PS_PROTECTION {
	union {
		UCHAR Level;
		struct {
			UCHAR Type : 3; // PS_PROTECTED_TYPE
			UCHAR Audit : 1; // Reserved
			UCHAR Signer : 4; // PS_PROTECTED_SIGNER
		};
	};
} PS_PROTECTION, *PPS_PROTECTION;

#pragma pack(pop)

#pragma warning(pop)

//
//
//
typedef enum _PS_PROTECTED_TYPE {
	PsProtectedTypeNone = 0,
	PsProtectedTypeProtectedLight = 1,
	PsProtectedTypeProtected = 2
} PS_PROTECTED_TYPE, *PPS_PROTECTED_TYPE;

//
//
//
typedef enum _PS_PROTECTED_SIGNER {
	PsProtectedSignerNone = 0,
	PsProtectedSignerAuthenticode,
	PsProtectedSignerCodeGen,
	PsProtectedSignerAntimalware,
	PsProtectedSignerLsa,
	PsProtectedSignerWindows,
	PsProtectedSignerWinTcb,
	PsProtectedSignerWinSystem,
	PsProtectedSignerApp,
	PsProtectedSignerMax
} PS_PROTECTED_SIGNER, *PPS_PROTECTED_SIGNER;

//exported since Windows 8.0
typedef 
__checkReturn
LOGICAL
(NTAPI* PPsIsProtectedProcess)(
	__in PEPROCESS Process
);

typedef
__checkReturn
PVOID
(NTAPI* PPsGetProcessWow64Process)(
	__in PEPROCESS Process
);

typedef
__checkReturn
NTSTATUS 
(NTAPI* PPsWrapApcWow64Thread)(
	__inout PVOID* ApcContext,
	__inout PVOID* ApcRoutine
);

typedef
__checkReturn
LOGICAL
(NTAPI* PPsIsProtectedProcessLight)(
	__in PEPROCESS Process
);

typedef 
__drv_maxIRQL(APC_LEVEL)
SE_SIGNING_LEVEL 
(NTAPI* PPsGetProcessSignatureLevel)(
	__in PEPROCESS Process, 
	__out PSE_SIGNING_LEVEL SectionSignatureLevel
);

typedef
__drv_maxIRQL(APC_LEVEL)
__checkReturn
NTSTATUS 
(NTAPI* PSeGetCachedSigningLevel)(
	__in PFILE_OBJECT FileObject,
	__out PULONG Flags,
	__out PSE_SIGNING_LEVEL SigningLevel,
	__reserved __out_ecount_full_opt(*ThumbprintSize) PUCHAR Thumbprint,
	__reserved __out_opt PULONG ThumbprintSize,
	__reserved __out_opt PULONG ThumbprintAlgorithm
);

#define UNPROTECTED_FLAG (1 << 2)

//exported since Windows 8.0
typedef 
__drv_maxIRQL(APC_LEVEL)
__checkReturn
NTSTATUS (NTAPI* PNtSetCachedSigningLevel)(
	__in ULONG Flags,
	__in SE_SIGNING_LEVEL InputSigningLevel,
	__in_ecount(SourceFileCount) PHANDLE SourceFiles,
	__in ULONG SourceFileCount,
	__in HANDLE TargetFile
);

extern "C"
NTSTATUS
NTAPI
PsReferenceProcessFilePointer(
	__in PEPROCESS Process,
	__out PFILE_OBJECT *OutFileObject
);

} // namespace cmd

/// @}
