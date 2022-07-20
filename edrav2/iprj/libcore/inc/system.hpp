//
// edrav2.libcore project
//
// Author: Denis Bogdanov (16.04.2019)
// Reviewer: Denis Bogdanov (16.04.2019)
//
///
/// @file Common auxillary system-related functions
/// (declaration)
///
#pragma once
#include "utilities.hpp"

namespace cmd {
namespace sys {

//
// Windows-specific functions
//
#ifdef PLATFORM_WIN
namespace win {

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


// Specialization for windows library HMODULE
struct LibraryTraits
{
	using ResourceType = HMODULE;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedLibrary = UniqueResource<LibraryTraits>;


// Specialization for SC_HANDLE
struct ServiceHandleTraits
{
	using ResourceType = SC_HANDLE;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedServiceHandle = UniqueResource<ServiceHandleTraits>;

// Specialization for local memory LPVOID
struct LocalMemoryTraits
{
	using ResourceType = LPVOID;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedLocalMem = UniqueResource<LocalMemoryTraits>;

// Specialization for windows registry HANDLE
struct RegKeyTraits
{
	using ResourceType = HKEY;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedRegKey = UniqueResource<RegKeyTraits>;

// Specialization for environment block LPVOID
struct EnvBlockTraits
{
	using ResourceType = LPVOID;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedEnvBlock = UniqueResource<EnvBlockTraits>;

// Specialization for impersonation
struct ImpersonationTraits
{
	typedef bool ResourceType;
	static inline const ResourceType c_defVal = false;
	static void cleanup(ResourceType&) noexcept;
};
using ScopedImpersonation = UniqueResource<ImpersonationTraits>;

// Specialization for find volume HANDLE
struct FileVolumeTraits
{
	using ResourceType = HANDLE;
	static inline const ResourceType c_defVal = INVALID_HANDLE_VALUE;
	static void cleanup(ResourceType& rsrc) noexcept;
};
using ScopedVolumeHandle = UniqueResource<FileVolumeTraits>;

Variant ReadRegistryValue(HKEY rootKey, const std::wstring& sKey, const std::wstring& sValue);
void WriteRegistryValue(HKEY rootKey, const std::wstring& sKey, const std::wstring& sValue, Variant vData);

//
//
//
inline Variant ReadRegistryValue(HKEY rootKey, const std::wstring& sKey, const std::wstring& sValue, Variant vDefault)
{
	try
	{
		return ReadRegistryValue(rootKey, sKey, sValue);
	}
	catch (...)
	{
		return vDefault;
	}
}

} // namespace win
#endif

///
/// Executes specified application with certain parameters
///
/// @param pathProgram - path to the applcation
/// @param sParams - application parameters
/// @param fElevatePrivileges - flag is application is run in elevated mode
/// @param nTimeout - Timeout in milliseconds how long function waits application's termination (0 - no wait)
/// @return Application exit code
///
uint32_t executeApplication(const std::filesystem::path& pathProgram, std::wstring_view sParams, 
	bool fElevatePrivileges = false, Size nTimeout = INFINITE);


///
/// Get unique hardware key
/// 
/// @return Unique hadrware key string
///
std::string getHardwareMachineId();

///
/// Get unique hardware key
/// 
/// @return Unique machine id string
///
std::string getUniqueMachineId();

///
/// Set name for current thread
/// 
/// @param - Thread name
///
void setThreadName(const std::string& sName);

} // namespace sys
} // namespace cmd
