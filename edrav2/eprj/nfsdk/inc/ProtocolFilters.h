//
//  EDRAv2.nfsdk
//  ProtocolFilters.lib header file
//
#pragma once

// Enable C++ API
#undef _C_API
// Enable static link
#define _NFAPI_STATIC_LIB
#define _PF_STATIC_LIB
// Disable SSL
#define PF_NO_SSL_FILTER

#include "../src/ProtocolFilters/include/ProtocolFilters.h"

#pragma warning(push)
#pragma warning(disable:4100)
#include "../src/ProtocolFilters/include/PFEventsDefault.h"
#pragma warning(pop)
