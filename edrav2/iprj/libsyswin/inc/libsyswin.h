//
// EDRAv2.libsyswin project
//
// Metaheader for whole libsyswin library
// Please put here the includes for other libsyswin components
//
#pragma once

#include "extheaders.h"

#include "utilities.hpp"
#include "dataproviders.hpp"

#ifndef PLATFORM_WIN
#error You should include this library only into windows-specific code
#endif // PLATFORM_WIN

#include "objects.h"
CMD_IMPORT_LIBRARY_OBJECTS(libsyswin)

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "libsyswin.lib")
#pragma comment(lib, "NtDll.lib")
#pragma comment(lib, "Mpr.lib")
#pragma comment(lib, "Wtsapi32.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER



