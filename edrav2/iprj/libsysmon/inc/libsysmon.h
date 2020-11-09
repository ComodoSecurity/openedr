//
// edrav2.libsysmon project
//
// Metaheader for whole libsysmon library
// Please put here the includes for other libsysmon components
//
#pragma once

#include "extheaders.h"

#ifndef PLATFORM_WIN
#error You should include this library only into windows-specific code
#endif // PLATFORM_WIN

CMD_IMPORT_LIBRARY_OBJECTS(libsysmon)
#include "objects.h"

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "libsysmon.lib")
#pragma comment(lib, "FltLib.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER
