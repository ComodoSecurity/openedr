#pragma once

#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <SDKDDKVer.h>

// Win API
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

// std lib
#include <malloc.h>
#include <stdint.h>
#include <wchar.h>
#include <cstdlib>
#include <cstddef>
#include <type_traits>
#include <iterator>
#include <mutex>
#include <cwchar>
#include <atomic>


#include <libcore/inc/edrmmapi.hpp>

