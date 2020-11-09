//
// Precompiled header for windows-specific c/c++ files
//

#ifndef PCH_WIN_H
#define PCH_WIN_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Lmcons.h>
#include <winsock2.h>

#define SECURITY_WIN32
#include <security.h>
#pragma comment(lib, "Secur32.lib")

#include <wtsapi32.h>
#pragma comment(lib, "Wtsapi32.lib")

#include <iphlpapi.h>
#pragma comment(lib, "IPHLPAPI.lib")

#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <pch.h>

#endif //PCH_WIN_H
