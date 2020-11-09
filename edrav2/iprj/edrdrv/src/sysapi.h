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

namespace openEdr {

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
#define SystemProcessInformation 5

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

} // namespace openEdr

/// @}
