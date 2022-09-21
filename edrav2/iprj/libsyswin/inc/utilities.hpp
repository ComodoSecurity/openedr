//
// edrav2.libsyswin project
//
// Author: Denis Bogdanov (07.02.2019)
// Reviewer: Denis Bogdanov (22.02.2019)
//
///
/// @file Common auxiliary functions for interaction with OS Windows API
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once
#include <winternl.h>

namespace cmd {
namespace sys {
namespace win {

#define _CMD_MAX_PATH 296
#define EXCLUDE_NO_FILE  0x00000000
#define EXCLUDE_EXE_FILE 0x00000001
#define EXCLUDE_MSI_FILE 0x00000002

#define STATUS_INVALID_PARAMETER_2 ((NTSTATUS)0xC00000F0L)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

#define WOW64_POINTER(Type) ULONG
#define RTL_MAX_DRIVE_LETTERS 32

#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#ifndef _WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

	typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];
	typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
	typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];

	//
	// Used for get WOW64 PEB information
	//
	typedef struct _STRING32
	{
		USHORT Length;
		USHORT MaximumLength;
		ULONG Buffer;
	} STRING32, * PSTRING32;
	typedef STRING32 UNICODE_STRING32, * PUNICODE_STRING32;

	typedef struct _CURDIR32
	{
		UNICODE_STRING32 DosPath;
		WOW64_POINTER(HANDLE) Handle;
	} CURDIR32, * PCURDIR32;

	typedef struct _RTL_DRIVE_LETTER_CURDIR32
	{
		USHORT Flags;
		USHORT Length;
		ULONG TimeStamp;
		STRING32 DosPath;
	} RTL_DRIVE_LETTER_CURDIR32, * PRTL_DRIVE_LETTER_CURDIR32;

	typedef struct _EDR_RTL_USER_PROCESS_PARAMETERS32
	{
		ULONG MaximumLength;
		ULONG Length;

		ULONG Flags;
		ULONG DebugFlags;

		WOW64_POINTER(HANDLE) ConsoleHandle;
		ULONG ConsoleFlags;
		WOW64_POINTER(HANDLE) StandardInput;
		WOW64_POINTER(HANDLE) StandardOutput;
		WOW64_POINTER(HANDLE) StandardError;

		CURDIR32 CurrentDirectory;
		UNICODE_STRING32 DllPath;
		UNICODE_STRING32 ImagePathName;
		UNICODE_STRING32 CommandLine;
		WOW64_POINTER(PVOID) Environment;

		ULONG StartingX;
		ULONG StartingY;
		ULONG CountX;
		ULONG CountY;
		ULONG CountCharsX;
		ULONG CountCharsY;
		ULONG FillAttribute;

		ULONG WindowFlags;
		ULONG ShowWindowFlags;
		UNICODE_STRING32 WindowTitle;
		UNICODE_STRING32 DesktopInfo;
		UNICODE_STRING32 ShellInfo;
		UNICODE_STRING32 RuntimeData;
		RTL_DRIVE_LETTER_CURDIR32 CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

		WOW64_POINTER(ULONG_PTR) EnvironmentSize;
		WOW64_POINTER(ULONG_PTR) EnvironmentVersion;
		WOW64_POINTER(PVOID) PackageDependencyData;
		ULONG ProcessGroupId;
		ULONG LoaderThreads;

		UNICODE_STRING32 RedirectionDllName; // REDSTONE4
		UNICODE_STRING32 HeapPartitionName; // 19H1
		WOW64_POINTER(ULONG_PTR) DefaultThreadpoolCpuSetMasks;
		ULONG DefaultThreadpoolCpuSetMaskCount;
	} EDR_RTL_USER_PROCESS_PARAMETERS32, * PEDR_RTL_USER_PROCESS_PARAMETERS32;

	typedef struct _PEB32
	{
		BOOLEAN InheritedAddressSpace;
		BOOLEAN ReadImageFileExecOptions;
		BOOLEAN BeingDebugged;
		union
		{
			BOOLEAN BitField;
			struct
			{
				BOOLEAN ImageUsesLargePages : 1;
				BOOLEAN IsProtectedProcess : 1;
				BOOLEAN IsImageDynamicallyRelocated : 1;
				BOOLEAN SkipPatchingUser32Forwarders : 1;
				BOOLEAN IsPackagedProcess : 1;
				BOOLEAN IsAppContainer : 1;
				BOOLEAN IsProtectedProcessLight : 1;
				BOOLEAN IsLongPathAwareProcess : 1;
			};
		};
		WOW64_POINTER(HANDLE) Mutant;

		WOW64_POINTER(PVOID) ImageBaseAddress;
		WOW64_POINTER(PPEB_LDR_DATA) Ldr;
		WOW64_POINTER(PEDR_RTL_USER_PROCESS_PARAMETERS32) ProcessParameters;
		WOW64_POINTER(PVOID) SubSystemData;
		WOW64_POINTER(PVOID) ProcessHeap;
		WOW64_POINTER(PRTL_CRITICAL_SECTION) FastPebLock;
		WOW64_POINTER(PVOID) AtlThunkSListPtr;
		WOW64_POINTER(PVOID) IFEOKey;
		union
		{
			ULONG CrossProcessFlags;
			struct
			{
				ULONG ProcessInJob : 1;
				ULONG ProcessInitializing : 1;
				ULONG ProcessUsingVEH : 1;
				ULONG ProcessUsingVCH : 1;
				ULONG ProcessUsingFTH : 1;
				ULONG ReservedBits0 : 27;
			};
		};
		union
		{
			WOW64_POINTER(PVOID) KernelCallbackTable;
			WOW64_POINTER(PVOID) UserSharedInfoPtr;
		};
		ULONG SystemReserved;
		ULONG AtlThunkSListPtr32;
		WOW64_POINTER(PVOID) ApiSetMap;
		ULONG TlsExpansionCounter;
		WOW64_POINTER(PVOID) TlsBitmap;
		ULONG TlsBitmapBits[2];
		WOW64_POINTER(PVOID) ReadOnlySharedMemoryBase;
		WOW64_POINTER(PVOID) HotpatchInformation;
		WOW64_POINTER(PVOID*) ReadOnlyStaticServerData;
		WOW64_POINTER(PVOID) AnsiCodePageData;
		WOW64_POINTER(PVOID) OemCodePageData;
		WOW64_POINTER(PVOID) UnicodeCaseTableData;

		ULONG NumberOfProcessors;
		ULONG NtGlobalFlag;

		LARGE_INTEGER CriticalSectionTimeout;
		WOW64_POINTER(SIZE_T) HeapSegmentReserve;
		WOW64_POINTER(SIZE_T) HeapSegmentCommit;
		WOW64_POINTER(SIZE_T) HeapDeCommitTotalFreeThreshold;
		WOW64_POINTER(SIZE_T) HeapDeCommitFreeBlockThreshold;

		ULONG NumberOfHeaps;
		ULONG MaximumNumberOfHeaps;
		WOW64_POINTER(PVOID*) ProcessHeaps;

		WOW64_POINTER(PVOID) GdiSharedHandleTable;
		WOW64_POINTER(PVOID) ProcessStarterHelper;
		ULONG GdiDCAttributeList;

		WOW64_POINTER(PRTL_CRITICAL_SECTION) LoaderLock;

		ULONG OSMajorVersion;
		ULONG OSMinorVersion;
		USHORT OSBuildNumber;
		USHORT OSCSDVersion;
		ULONG OSPlatformId;
		ULONG ImageSubsystem;
		ULONG ImageSubsystemMajorVersion;
		ULONG ImageSubsystemMinorVersion;
		WOW64_POINTER(ULONG_PTR) ActiveProcessAffinityMask;
		GDI_HANDLE_BUFFER32 GdiHandleBuffer;
		WOW64_POINTER(PVOID) PostProcessInitRoutine;

		WOW64_POINTER(PVOID) TlsExpansionBitmap;
		ULONG TlsExpansionBitmapBits[32];

		ULONG SessionId;

		ULARGE_INTEGER AppCompatFlags;
		ULARGE_INTEGER AppCompatFlagsUser;
		WOW64_POINTER(PVOID) pShimData;
		WOW64_POINTER(PVOID) AppCompatInfo;

		UNICODE_STRING32 CSDVersion;

		WOW64_POINTER(PVOID) ActivationContextData;
		WOW64_POINTER(PVOID) ProcessAssemblyStorageMap;
		WOW64_POINTER(PVOID) SystemDefaultActivationContextData;
		WOW64_POINTER(PVOID) SystemAssemblyStorageMap;

		WOW64_POINTER(SIZE_T) MinimumStackCommit;

		WOW64_POINTER(PVOID) SparePointers[4];
		ULONG SpareUlongs[5];

		WOW64_POINTER(PVOID) WerRegistrationData;
		WOW64_POINTER(PVOID) WerShipAssertPtr;
		WOW64_POINTER(PVOID) pContextData;
		WOW64_POINTER(PVOID) pImageHeaderHash;
		union
		{
			ULONG TracingFlags;
			struct
			{
				ULONG HeapTracingEnabled : 1;
				ULONG CritSecTracingEnabled : 1;
				ULONG LibLoaderTracingEnabled : 1;
				ULONG SpareTracingBits : 29;
			};
		};
		ULONGLONG CsrServerReadOnlySharedMemoryBase;
		WOW64_POINTER(PVOID) TppWorkerpListLock;
		LIST_ENTRY32 TppWorkerpList;
		WOW64_POINTER(PVOID) WaitOnAddressHashTable[128];
		WOW64_POINTER(PVOID) TelemetryCoverageHeader; // REDSTONE3
		ULONG CloudFileFlags;
		ULONG CloudFileDiagFlags; // REDSTONE4
		CHAR PlaceholderCompatibilityMode;
		CHAR PlaceholderCompatibilityModeReserved[7];
	} PEB32, * PPEB32;

	typedef struct _CURDIR
	{
		UNICODE_STRING DosPath;
		HANDLE Handle;
	} CURDIR, * PCURDIR;

	typedef struct _STRING
	{
		USHORT Length;
		USHORT MaximumLength;
		_Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
	} STRING, * PSTRING, ANSI_STRING, * PANSI_STRING, OEM_STRING, * POEM_STRING;

	typedef struct _RTL_DRIVE_LETTER_CURDIR
	{
		USHORT Flags;
		USHORT Length;
		ULONG TimeStamp;
		STRING DosPath;
	} RTL_DRIVE_LETTER_CURDIR, * PRTL_DRIVE_LETTER_CURDIR;

	typedef struct _EDR_RTL_USER_PROCESS_PARAMETERS
	{
		ULONG MaximumLength;
		ULONG Length;

		ULONG Flags;
		ULONG DebugFlags;

		HANDLE ConsoleHandle;
		ULONG ConsoleFlags;
		HANDLE StandardInput;
		HANDLE StandardOutput;
		HANDLE StandardError;

		CURDIR CurrentDirectory;
		UNICODE_STRING DllPath;
		UNICODE_STRING ImagePathName;
		UNICODE_STRING CommandLine;
		PVOID Environment;

		ULONG StartingX;
		ULONG StartingY;
		ULONG CountX;
		ULONG CountY;
		ULONG CountCharsX;
		ULONG CountCharsY;
		ULONG FillAttribute;

		ULONG WindowFlags;
		ULONG ShowWindowFlags;
		UNICODE_STRING WindowTitle;
		UNICODE_STRING DesktopInfo;
		UNICODE_STRING ShellInfo;
		UNICODE_STRING RuntimeData;
		RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

		ULONG_PTR EnvironmentSize;
		ULONG_PTR EnvironmentVersion;

		PVOID PackageDependencyData;
		ULONG ProcessGroupId;
		ULONG LoaderThreads;

		UNICODE_STRING RedirectionDllName;
		UNICODE_STRING HeapPartitionName;
		ULONG_PTR DefaultThreadpoolCpuSetMasks;
		ULONG DefaultThreadpoolCpuSetMaskCount;
	} EDR_RTL_USER_PROCESS_PARAMETERS, * PEDR_RTL_USER_PROCESS_PARAMETERS;

	typedef enum _EDR_PEB_OFFSET
	{
		PEBCurrentDirectory,
		PEBDllPath,
		PEBImagePathName,
		PEBCommandLine,
		PEBWindowTitle,
		PEBDesktopInfo,
		PEBShellInfo,
		PEBRuntimeData,
		PEBTypeMask = 0xffff,

		PEBWow64 = 0x10000
	} EDR_PEB_OFFSET;

	enum CMD_PARSER_FLAG
	{
		CMD_PARSER_FILE = 0x10,
		CMD_PARSER_SCRIPT = 0x20
	};

	typedef struct _REPARSE_DATA_BUFFER {
		ULONG  ReparseTag;
		USHORT ReparseDataLength;
		USHORT Reserved;
		union {
			struct {
				USHORT SubstituteNameOffset;
				USHORT SubstituteNameLength;
				USHORT PrintNameOffset;
				USHORT PrintNameLength;
				ULONG  Flags;
				WCHAR  PathBuffer[1];
			} SymbolicLinkReparseBuffer;
			struct {
				USHORT SubstituteNameOffset;
				USHORT SubstituteNameLength;
				USHORT PrintNameOffset;
				USHORT PrintNameLength;
				WCHAR  PathBuffer[1];
			} MountPointReparseBuffer;
			struct {
				UCHAR DataBuffer[1];
			} GenericReparseBuffer;
		} DUMMYUNIONNAME;
	} REPARSE_DATA_BUFFER, * PREPARSE_DATA_BUFFER;

	//
	//parse command line
	//
	typedef LPWSTR* (__stdcall* CommandLineToArgvWFunction)(_In_ LPCWSTR lpCmdLine, _Out_ int* pNumArgs);
	typedef BOOL(*pathPatternMatchFunction)(IN LPWSTR lpCmdLine, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);
	typedef BOOL(*parseCommandLineFunction)(LPCWSTR lpFullPath, LPCWSTR lpCommandLine, LPCWSTR lpCurrentDirectory, LPWSTR* lppScriptPath);
	typedef BOOL(*parseIsScriptAFileFunction)(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath);

	typedef struct _CMDLINE_ENTRY {
		LPCWSTR m_szName;
		DWORD m_excludeExe;
		parseCommandLineFunction m_pParseEmbeddedFunction;
		parseIsScriptAFileFunction m_pIsScriptAFile;
		parseCommandLineFunction m_pParseFunction;
	}CMDLINE_ENTRY, * PCMDLINE_ENTRY;

	typedef struct _CMD_PATH_CONVERSION
	{
		WCHAR m_From[64];
		WCHAR m_To[64];
	} CMD_PATH_CONVERSION, * PCMD_PATH_CONVERSION;

//
//
//
typedef uint32_t Pid;
static const Pid g_nSystemPid = 4;

//
//
//
std::vector<uint8_t> getTokenInformation(HANDLE hToken, TOKEN_INFORMATION_CLASS eTokenType);

//
//
//
std::vector<uint8_t> getProcessInformation(HANDLE hProcess, TOKEN_INFORMATION_CLASS eTokenType);

///
/// Checks it the current process has an elevated rights.
///
/// @param hProcess - a handle of the process.
/// @param defValue [opt] - this value is returned if error occurs (default: `false`).
/// @returns The function returns a boolean elevation status of the process or `defValue`
/// if error occurs.
///
bool isProcessElevated(HANDLE hProcess, bool defValue = false);

///
/// Get elevation type of process.
///
/// @param hProcess - a handle of the process.
/// @param defValue [opt] - this value is returned if error occurs (default: `TokenElevationTypeDefault`).
/// @returns The function returns an elevation type of the process or `defValue`
/// if error occurs.
Enum getProcessElevationType(HANDLE hProcess, Enum defValue = TokenElevationTypeDefault);

///
/// Get user SID by process handle.
///
/// @param hToken - a HANDLE value.
/// @param defValue [opt] - this value is returned if error occurs (default: blank).
/// @returns The function returns a SID of the user or `defValue` if error occurs.
///
std::string getProcessSid(HANDLE hToken, const std::string& defValue = {});

///
/// The enum class of integrity levels.
///
enum class IntegrityLevel
{
	Untrusted,
	Low,
	Medium,
	High,
	System,
	Protected
};

///
/// Get process integrity level.
///
IntegrityLevel getProcessIntegrityLevel(HANDLE hProcess, IntegrityLevel defValue = IntegrityLevel::Medium);

///
/// Get file handle.
///
HANDLE getFileHandle(const std::wstring& sPathName, uint32_t nAccess);

//
// Check if process is UWP application.
//
bool isUwpApp(const HANDLE hProcess);

//
// Check if process in suspended state.
//
bool isSuspended(const HANDLE hProcess);

//
// Check if process is critical.
//
bool isCritical(const HANDLE hProcess);

//
// Set privilege to the token.
//
bool setPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);

//
// Enable all privileges in the token except SE_BACKUP_NAME / SE_RESTORE_NAME and SE_TAKE_OWNERSHIP_NAME.
//
bool enableAllPrivileges();

//
// Wrapper of SHGetKnownFolderPath
// @return path or "" if error
//
std::wstring getKnownPath(const GUID& ePathId, const HANDLE hToken = nullptr);

//
// Search process running from the "same" user
//
HANDLE searchProcessBySid(const std::string& sSid, const bool fElevated);

//
// 
//
std::wstring getProcessImagePath(const HANDLE hProcess);

//
// Get strings from PEB
//
NTSTATUS getProcessPebString(HANDLE hProcess, int Offset, std::wstring& String);

//
// Get process current directory from PEB
//
NTSTATUS getProcessCurrentDirectory(HANDLE hProcess, std::wstring& currentDirectory);

//
// Get process full path from PEB (Dos path, not device path)
//
NTSTATUS getProcessImageFullPath(HANDLE hProcess, std::wstring& imageFullpath);

//
// String pattern match
//
BOOL strPatternMatch(LPCTSTR pat, LPCTSTR str);

//
// Parse command line
//
BOOL genericParseCommandline(LPCWSTR lpFullPath, LPCWSTR lpCmdLineWithoutPath, LPCWSTR lpCurrentDirectory, LPWSTR* lppScriptPath);

//
// Get file original name
//
BOOL getOriginalFileName(LPCWSTR lpFullPath, LPWSTR lpszValue, DWORD dwLength);

//
// Parse embedded script for python
//
BOOL pythonParseEmbeddedScript(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);

//
// judge script is a file
//
BOOL genericIsScriptAFile(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath);

//
// Parse script for python
//
BOOL pythonParseCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);

//
// Parse embedded script for cmd
//
BOOL cmdParseEmbeddedScript(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);

//
// Parse script for cmd
//
BOOL cmdParseCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);

//
// judge script is a file for cmd
//
BOOL cmdIsScriptAFile(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath);

//
// Parse embedded script for Powershell
//
BOOL powerShellParseEmbeddedScript(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);

//
// Parse script for Powershell
//
BOOL powershellParseCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath);

//
// judge script is a file for powershell
//
BOOL powerShellIsScriptAFile(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath);

//
// Pare command line and get full path for script file
//
INT_PTR getFullScriptPath(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPCWSTR lpCurrentDirectory, std::wstring& lpFullPath);

} // namespace win
} // namespace sys
} // namespace cmd

/// @}
