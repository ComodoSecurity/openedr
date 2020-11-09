//
// edrav2.libprocmon project
//
#pragma once

#include "extheaders.h"
#include "objects.h"
// Enable objects creation
CMD_IMPORT_LIBRARY_OBJECTS(libprocmon)

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "libprocmon.lib")
#pragma comment(lib, "madCHook.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER



