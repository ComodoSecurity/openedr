//
// edrav2.libedr project
//
// Author: Yury Ermakov (16.04.2019)
// Reviewer:
//
#pragma once

#include "extheaders.h"

#include "policy.hpp"
#include "events.hpp"
#include "lane.hpp"

#include "objects.h"
CMD_IMPORT_LIBRARY_OBJECTS(libedr)

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "libedr.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER
