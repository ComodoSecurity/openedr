//
// edrav2.libcore project
//
// Metaheader for libcore library
//
#pragma once

#include "extheaders.h"

#include "basic.hpp"
#include "error.hpp"
#include "constant.hpp"
#include "object.hpp"
#include "variant.hpp"
#include "logging.hpp"
#include "string.hpp"
#include "core.hpp"
#include "common.hpp"
#include "command.hpp"
#include "service.hpp"
#include "message.hpp"
#include "events.hpp"
#include "application.hpp"
#include "io.hpp"
#include "crypt.hpp"
#include "utilities.hpp"
#include "system.hpp"
#include "dump.hpp"
#include "queue.hpp"
#include "threadpool.hpp"
#include "task.hpp"
#include "version.h"

#include "objects.h"
// Enable objects creation
CMD_IMPORT_LIBRARY_OBJECTS(libcore)

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER

#pragma comment(lib, "libcore.lib")
#pragma comment(lib, "jsoncpp_lib.lib")

// jsonrpc and dependences
#pragma comment(lib, "jsonrpccpp-common.lib")
#pragma comment(lib, "jsonrpccpp-client.lib")
#pragma comment(lib, "jsonrpccpp-server.lib")
#if defined(_DEBUG)
#pragma comment(lib, "log4cplusSD.lib")
#pragma comment(lib, "libmicrohttpd_d.lib")
#pragma comment(lib, "libcurl-d.lib")
#else // _DEBUG
#pragma comment(lib, "log4cplusS.lib")
#pragma comment(lib, "libmicrohttpd.lib")
#pragma comment(lib, "libcurl.lib")
#endif // _DEBUG

#pragma comment(lib, "tiny-aes.lib")

#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER

//#if defined(PLATFORM_WIN) && defined(OS_DEPENDENT_PCH)
// #include <system.hpp>
//#endif // PLATFORM_WIN
