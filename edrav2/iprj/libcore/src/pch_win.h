#pragma once

#include "pch.h"

#ifndef _WIN32
#error You should include this file only into windows-specific code
#endif // _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#include <shlobj.h>
#include <dbghelp.h>
#include <Lmcons.h>
#include <userenv.h>
#include <wtsapi32.h>
#define SECURITY_WIN32
#include <security.h>

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "Secur32.lib")
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "userenv.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER

// #include "libcore_int.h"
