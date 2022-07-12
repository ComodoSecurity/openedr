#pragma once

#ifdef _WIN32
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#include <SDKDDKVer.h>
#endif
// see: https://github.com/chriskohlhoff/asio/issues/290#issuecomment-371867040
// #define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4702)
#endif
#define BOOST_ASIO_NO_WIN32_LEAN_AND_MEAN
#include <boost/asio.hpp>
#ifdef _WIN32
#pragma warning(pop)
#endif

#include <stdint.h>

#include <memory>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cinttypes>
#include <string>
#include <sstream>
#include <string_view>
#include <iostream>
#include <iomanip>
#include <optional>
#include <type_traits>
#include <filesystem>
#include <exception>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <array>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <utility>

#include <boost/system/error_code.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/variant.hpp>
#include <boost/mpl/contains.hpp>

// For timed wait use boost::condition_variable instead std::condition_variable
// Because boost-version uses steady_clock, but std-version uses system_clock
#include <boost/thread/condition_variable.hpp>

// Clara library for Application class
#include <clara.hpp>

// log4cplue library
#undef UNICODE
#define LOG4CPLUS_DISABLE_DLL_RUNTIME_WARNING
#define LOG4CPLUS_MACRO_USE_GUARD
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>
#define UNICODE

// xxhash library
namespace cmd {
namespace crypt { //namespace xxh {
#define XXH_USE_STDLIB
#define XXH_ACCEPT_NULL_INPUT_POINTER 1
#include <xxhash.hpp>
}
}//}
#include <aes.hpp>

