//
// edrav2.libsyswin project
//
// Author: Denis Bogdanov (07.02.2019)
// Reviewer: Denis Bogdanov (22.02.2019)
//
///
/// @file Common auxiliary functions for interaction with OS Windows API
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once

namespace cmd {
namespace sys {
namespace win {

//
//
//
typedef uint32_t Pid;
static const Pid g_nSystemPid = 4;

//
//
//
std::vector<uint8_t> getTokenInformation(HANDLE hToken, TOKEN_INFORMATION_CLASS eTokenType);

//
//
//
std::vector<uint8_t> getProcessInformation(HANDLE hProcess, TOKEN_INFORMATION_CLASS eTokenType);

///
/// Checks it the current process has an elevated rights.
///
/// @param hProcess - a handle of the process.
/// @param defValue [opt] - this value is returned if error occurs (default: `false`).
/// @returns The function returns a boolean elevation status of the process or `defValue`
/// if error occurs.
///
bool isProcessElevated(HANDLE hProcess, bool defValue = false);

///
/// Get elevation type of process.
///
/// @param hProcess - a handle of the process.
/// @param defValue [opt] - this value is returned if error occurs (default: `TokenElevationTypeDefault`).
/// @returns The function returns an elevation type of the process or `defValue`
/// if error occurs.
Enum getProcessElevationType(HANDLE hProcess, Enum defValue = TokenElevationTypeDefault);

///
/// Get user SID by process handle.
///
/// @param hToken - a HANDLE value.
/// @param defValue [opt] - this value is returned if error occurs (default: blank).
/// @returns The function returns a SID of the user or `defValue` if error occurs.
///
std::string getProcessSid(HANDLE hToken, const std::string& defValue = {});

///
/// The enum class of integrity levels.
///
enum class IntegrityLevel
{
	Untrusted,
	Low,
	Medium,
	High,
	System,
	Protected
};

///
/// Get process integrity level.
///
IntegrityLevel getProcessIntegrityLevel(HANDLE hProcess, IntegrityLevel defValue = IntegrityLevel::Medium);

///
/// Get file handle.
///
HANDLE getFileHandle(const std::wstring& sPathName, uint32_t nAccess);

//
// Check if process is UWP application.
//
bool isUwpApp(const HANDLE hProcess);

//
// Check if process in suspended state.
//
bool isSuspended(const HANDLE hProcess);

//
// Check if process is critical.
//
bool isCritical(const HANDLE hProcess);

//
// Set privilege to the token.
//
bool setPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);

//
// Enable all privileges in the token except SE_BACKUP_NAME / SE_RESTORE_NAME and SE_TAKE_OWNERSHIP_NAME.
//
bool enableAllPrivileges();

//
// Wrapper of SHGetKnownFolderPath
// @return path or "" if error
//
std::wstring getKnownPath(const GUID& ePathId, const HANDLE hToken = nullptr);

//
// Search process running from the "same" user
//
HANDLE searchProcessBySid(const std::string& sSid, const bool fElevated);

//
//
//
std::wstring getProcessImagePath(const HANDLE hProcess);

} // namespace win
} // namespace sys
} // namespace cmd

/// @}
