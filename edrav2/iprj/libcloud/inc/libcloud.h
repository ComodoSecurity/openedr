//
// edrav2.libcloud project
//
// Author: Yury Ermakov (11.03.2019)
// Reviewer: Denis Bogdanov (15.03.2019)
//
#pragma once

#include "extheaders.h"

#include "net.hpp"
#include "fls.hpp"
#include "valkyrie.hpp"

#include "objects.h"
CMD_IMPORT_LIBRARY_OBJECTS(libcloud)

// Declaration of linking dependences (statical libraries)
#ifdef _MSC_VER
#pragma comment(lib, "libcloud.lib")
#pragma comment(lib, "network-uri.lib")
#pragma comment(lib, "aws-cpp-sdk-core.lib")
#pragma comment(lib, "aws-cpp-sdk-firehose.lib")
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")

// Google Cloud
#pragma comment(lib, "gpr.lib")
#pragma comment(lib, "grpc.lib")
#pragma comment(lib, "grpc++.lib")
#pragma comment(lib, "address_sorting.lib")
#pragma comment(lib, "cares.lib")
#if defined(_DEBUG)
#pragma comment(lib, "libprotobufd.lib")
#pragma comment(lib, "zlibd.lib")
#else // _DEBUG
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "zlib.lib")
#endif // _DEBUG

#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER

#if defined(PLATFORM_WIN) && defined(_MSC_VER)
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "version.lib")
#endif // PLATFORM_WIN && _MSC_VER
