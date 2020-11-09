//
// edrav2.libnetmon project
//
// Reviewer: Denis Bogdanov (02.09.2019)
//
// Metaheader for whole libnetmon library
// Please put here the includes for other libnetmon components
//
#pragma once

#include "extheaders.h"
#include "netmon.hpp"

#ifndef PLATFORM_WIN
#error You should include this library only into windows-specific code
#endif // PLATFORM_WIN

CMD_IMPORT_LIBRARY_OBJECTS(libnetmon)
#include "objects.h"

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "libnetmon.lib")
#pragma comment(lib, "nfapi.lib")
#pragma comment(lib, "ProtocolFilters.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER
