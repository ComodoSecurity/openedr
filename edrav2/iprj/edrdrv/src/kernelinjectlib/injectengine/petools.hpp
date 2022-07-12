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

extern "C"
NTKERNELAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
	__in PVOID Base,
	__in BOOLEAN MappedAsImage,
	__in USHORT DirectoryEntry,
	__out PULONG Size
);

extern "C"
NTKERNELAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
	__in PVOID Base
);

typedef PVOID64 FARPROC, HMODULE, * PHMODULE;
typedef void* POINTER_32 PVOID32, FARPROC32, HMODULE32, *PHMODULE32;

typedef NTSTATUS(NTAPI* LDRLOADDLL)(
	__in_opt ULONG Flags,
	__in_opt PULONG Reserved,
	__in PCUNICODE_STRING ModuleFileName,
	__out HMODULE* ModuleHandle
	);

#define DONT_RESOLVE_DLL_REFERENCES         0x00000001
#define LOAD_LIBRARY_AS_DATAFILE            0x00000002
// reserved for internal LOAD_PACKAGED_LIBRARY: 0x00000004
#define LOAD_WITH_ALTERED_SEARCH_PATH       0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL        0x00000010
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE      0x00000020
#define LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE  0x00000040
#define LOAD_LIBRARY_REQUIRE_SIGNED_TARGET  0x00000080
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)

#define LOAD_LIBRARY_SAFE_CURRENT_DIRS      0x00002000

#define LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER   0x00004000

#else

//
// For anything building for downlevel, set the flag to be the same as LOAD_LIBRARY_SEARCH_SYSTEM32
// such that they're treated the same when running on older version of OS.
//

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)

#define LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY   0x00008000

#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS2)
#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS1)

namespace cmd {
namespace petools {

[[nodiscard]] PVOID getSystemModuleHandle(const ANSI_STRING& DllName);
[[nodiscard]] PVOID getSystemModuleHandle(const UNICODE_STRING& DllName);
[[nodiscard]] PVOID getProcAddress(const PVOID ImageBase, const ANSI_STRING& ProcName);

} // namespace petools
} // namespace cmd

/// @}