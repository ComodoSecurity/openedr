#pragma once

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <SDKDDKVer.h>

// Win API
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <VersionHelpers.h>
#include <winternl.h>
#include <winioctl.h>
#include <tlhelp32.h>
#include <intsafe.h>
#include <combaseapi.h>
#include <mmeapi.h>
#include <sddl.h>
#define _NTDEF_
typedef NTSTATUS* PNTSTATUS;
#include <ntsecapi.h>
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "secur32.lib")

// std lib
#include <cctype>
#include <string>
#include <array>
#include <vector>
#include <deque>
#include <map>
#include <atomic>
#include <mutex>

#define CMD_SKIP_FRAMEWORK
#include <libcore\inc\basic.hpp>
#include <libcore\inc\variant\lbvs.hpp>
#include <libprocmon\inc\procmon.hpp>
#include <libcore\inc\utilities.hpp>

#define XXH_USE_STDLIB
#include <xxhash.hpp>

// MadCHook
#ifdef PLATFORM_WIN
#pragma warning(push)
#pragma warning(disable:4505)
#endif
//#include <madCHook.h>
#define _PROCESSTYPES_H
#include <SystemIncludes.h>
#include <Systems.h>
// We use internal functional
#include <StringConstants.h>
#include <FunctionTypes.h>
#ifdef PLATFORM_WIN
#pragma warning(pop)
#endif
#pragma comment(lib, "madCHook.lib")

// Local files
#include "common.h"
#include "trampoline.h"
#include "injection.h"
//#include "winapi.h"
