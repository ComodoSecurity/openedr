//
// edrext
// 
// Author: Denis Bogdanov (17.09.2019)
//
/// @file Declarations of external EDR Agent API - system-related functions declaration
/// @addtogroup edrext
/// @{
///
#pragma once

namespace openEdr {
namespace sys {

///
/// Function returns the host name for current PC
/// @return - PC name. In case of system error function returns empty string
/// @throw - std exceptions in case of memory allocation errors
///
std::wstring getHostName();

///
/// Function returns the domain name for current PC
/// @return - domain name. In case of system error function returns empty string
/// @throw - std exceptions in case of memory allocation errors
///
std::wstring getDomainName();

///
/// Function returns the current user name (without domain part)
/// @return - user name. In case of system error function returns empty string
/// @throw - std exceptions in case of memory allocation errors
///
std::wstring getUserName();

///
/// Function returns the user-friendly represenration of operating system name
/// @return - OS name
/// @throw - std exceptions in case of memory allocation errors
///
std::string getOsFriendlyName();

///
/// Function returns the represenration of operating system family
/// @return - OS family
///
std::string getOsFamilyName();

///
/// Unified representation of OS version
///
struct OsVersion
{
	size_t nMajor = 0;
	size_t nMinor = 0;
	size_t nBuild = 0;
	size_t nRevision = 0;
};

///
/// Function returns components of OS version
/// Please note that for Windows the proper application targetting using manifest 
/// should be provided for receiving the correct result
/// @return - OS version
///
OsVersion getOsVersion();

///
/// Function returns time from last boot
/// @return - the amount of milliseconds from boot
///
uint64_t getOsUptime();

///
/// Function returns time of last boot
/// @return - unix time in milliseconds
///
uint64_t getOsBootTime();

///
/// Logon session information
///
struct SessionInfo
{
	size_t nId = 0;
	std::wstring sName;
	std::wstring sUser;			///< The user who owns the session
	std::wstring sDomain;		///< Domain of the user who owns the session
	bool fActive = false;		///< The session is active
	bool fLocalConsole = false;	///< This is the session of current local user
	bool fRemote = false;		///< This is remote (not local) session
	uint64_t logonTime {};		///< Unix time in milliseconds when session has started
};

typedef std::vector<SessionInfo> SessionsInfo;
typedef std::unique_ptr<SessionsInfo> SessionsInfoPtr;

///
/// Return information about current sessions on PC
///	@param fOnlyActive - set filter only for active sessions
/// @return - Smart pointer to array of session information descriptors
/// @throw - std memory-related errors and system_error in case of system API failure
///
SessionsInfoPtr getSessionsInfo(bool fOnlyActive);

//
// Return the name of user attached to the local console 
// @return - the user name or "unknown" if it is absent
//
std::wstring getLocalConsoleUser();

} // namespace sys
} // namespace openEdr
/// @} 
