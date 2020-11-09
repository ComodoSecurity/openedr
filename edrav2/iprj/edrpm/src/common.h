//
// edrav2.edrpm project
//
// Author: Denis Kroshin (30.04.2019)
// Reviewer: Denis Bogdanov (05.05.2019)
//
///
/// @file Injection library
///
/// @addtogroup edrpm
/// 
/// Injection library for process monitoring.
/// 
/// @{
#pragma once

namespace openEdr {
namespace edrpm {

typedef uint64_t Time;
typedef xxh::hash64_t EventHash;
typedef std::vector<uint8_t> Buffer;

// Specialization for windows HANDLE
struct HandleTraits
{
	using ResourceType = HANDLE;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedHandle = UniqueResource<HandleTraits>;

// Specialization for windows file HANDLE
struct FileHandleTraits
{
	using ResourceType = HANDLE;
	static inline const ResourceType c_defVal = INVALID_HANDLE_VALUE;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedFileHandle = UniqueResource<FileHandleTraits>;

// Specialization for local memory LPVOID
struct LocalMemoryTraits
{
	using ResourceType = LPVOID;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedLocalMem = UniqueResource<LocalMemoryTraits>;

//////////////////////////////////////////////////////////////////////////
// Logging

// Log errors to "error" IPC port
void logError(const std::string sStr, ErrorType = ErrorType::Error);

// Print message to DebugView
void dbgPrint(const std::string sStr);

// 
void UserBreak(const char* sFn = NULL);

#define TRY try
#define CATCH catch(const std::runtime_error& ex) \
{ logError(std::string("Runtime error: ") + ex.what()); } \
catch (const std::exception& ex) \
{ logError(std::string("Error occurred: ") + ex.what()); } \
catch (...) \
{ logError("Unknown failure. Possible memory corruption"); }


//////////////////////////////////////////////////////////////////////////
//  String functions

namespace string
{

inline std::wstring makeString(PUNICODE_STRING pUStr)
{
	return std::wstring(pUStr->Buffer, pUStr->Length / sizeof(wchar_t));
}

template <typename T>
std::string convertToHex(T num, size_t hexLen = sizeof(T) << 1) {
	static const char* digits = "0123456789ABCDEF";
	std::string hex(hexLen, '0');
	for (size_t i = 0, j = (hexLen - 1) * 4; i < hexLen; ++i, j -= 4)
		hex[i] = digits[(num >> j) & 0x0F];
	return hex;
}

//
// FIXME: Make generic function and add unit tests
//
template<typename T>
bool matchWildcard(const std::basic_string_view<T> sString, const std::basic_string_view<T> sPattern)
{
	auto itStr = sString.begin();
	auto itPat = sPattern.begin();
	while (itStr != sString.end() && itPat != sPattern.end())
	{
		switch (*itPat)
		{
			case '?':
				// FIXME: strange condition
				if (*itStr == '.')
					return false;
				break;
			case '*':
				do 
				{
					++itPat;
				} while (itPat != sPattern.end() && *itPat == '*');
				if (itPat == sPattern.end())
					return true;
				while (itStr != sString.end())
				{
					if (matchWildcard<T>(sString.substr(itStr - sString.begin()), 
						sPattern.substr(itPat - sPattern.begin())))
						return true;
					itStr++;
				}
				return false;
			default:
				if (std::toupper(*itStr) != std::toupper(*itPat))
					return false;
				break;
		}
		++itPat;
		++itStr;
	}
	while (itPat != sPattern.end() && *itPat == '*')
		++itPat;
	return itPat == sPattern.end() && itStr == sString.end();
}

inline bool matchWildcard(const std::string_view sString, const std::string_view sPattern)
{
	return matchWildcard<char>(sString, sPattern);
}

inline bool matchWildcard(const std::wstring_view sString, const std::wstring_view sPattern)
{
	return matchWildcard<wchar_t>(sString, sPattern);
}

} // namespace string

} // namespace edrpm
} // namespace openEdr

