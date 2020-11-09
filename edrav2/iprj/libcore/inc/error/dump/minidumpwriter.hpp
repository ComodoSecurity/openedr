//
// edrav2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer: Denis Bogdanov (21.12.2018)
//
///
/// @file MinidumpWriter class declaration (for Windows only)
///
// File contains definition and declaration of `MinidumpWriter` class
// which supports creation of minidumps on Windows platform.
///
/// @addtogroup errors Errors processing
/// @{
#pragma once
#include <utilities.hpp>
#include <system.hpp>

#ifndef _WIN32
#error You should include this file only into windows-specific code
#endif // _WIN32

namespace openEdr {
namespace error {
namespace dump {
namespace win {

//
// Extracts exception code from exception info
//
NTSTATUS getStatusFromExceptionInfo(PEXCEPTION_POINTERS pExInfo);
	
namespace detail {

// based on dbghelp.h
typedef BOOL(WINAPI* MiniDumpWriteDumpFunc)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
	CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

///
/// Minidumps writer class.
///
class MinidumpWriter
{
private:
	sys::win::ScopedLibrary m_dbhelpLib;
	MINIDUMP_TYPE m_minidumpType;
	std::wstring m_sPrefix;
	std::wstring m_sDumpsPath;
	MiniDumpWriteDumpFunc callMiniDumpWriteDump;

	MinidumpWriter();
	virtual ~MinidumpWriter();

	//
	// Makes dump filename string.
	//
	std::wstring makeDumpFilename(DWORD dwProcessId, DWORD dwThreadId, NTSTATUS status = 0) const;

	//
	// Saves dump with given filename.
	//
	void save(const std::wstring& sDumpFile, PEXCEPTION_POINTERS pExInfo = nullptr) const;

public:
	///
	/// Gets the single global instance of `MinidumpWriter` class.
	///
	/// @returns The function returns the single global instance of `MinidumpWriter` class.
	///
	static MinidumpWriter& getInstance();

	///
	/// Initialization.
	/// 
	/// @param vConfig [opt] - object's configuration (default: blank) including the 
	/// following fields:
	///   **prefix** [opt] - the prefix of the dumpfile's name. If parameter
	///     is omitted current module's name is used.
	///   **dumpDirectory** [opt] - the path of the directory to save dumps. If 
	///     parameter is omitted directory 'dump' is used inside the current 
	///     module's directory.
	///
	void init(const Variant& vConfig = {});

	///
	/// Saves minidump to the file on disk.
	///
	/// The function creates a minidump of the current process and saves it into
	/// the default directory for dumps.
	///
	/// @param pExceptionInfo - pointer to an exception info.
	///
	void save(PEXCEPTION_POINTERS pExInfo = nullptr) const;
};

} // namespace detail
} // namespace win
} // namespace dump
} // namespace error
} // namespace openEdr
/// @}
