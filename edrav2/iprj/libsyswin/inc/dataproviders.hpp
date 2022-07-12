//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (22.02.2019)
// Reviewer: Denis Bogdanov (07.03.2019)
//
///
/// @file Declaration of providers interface
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once

namespace cmd {
namespace sys {
namespace win {

typedef crypt::xxh::hash64_t ProcId;
typedef crypt::xxh::hash64_t UserId;
typedef crypt::xxh::hash64_t FileId;
typedef crypt::xxh::hash64_t TokenId;

// FIXME: @kroshin Use these patterns to build an abstract Registry path
constexpr char c_sHKeyLocalMachinePattern[] = "%hklm%";
constexpr wchar_t c_sHKeyLocalMachinePatternW[] = L"%hklm%";
constexpr char c_sHKeyCurrentUserPattern[] = "%hkcu%";
constexpr wchar_t c_sHKeyCurrentUserPatternW[] = L"%hkcu%";

///
/// Basic process information interface.
///
class IProcessInformation : OBJECT_INTERFACE
{
public:

	///
	/// Returns a cached or create new process descriptor.
	///
	/// @param vParams - process descriptor base (event.process).
	/// @return The function returns a data packet with process information.
	///
	virtual Variant enrichProcessInfo(Variant vParams) = 0;

	///
	/// Query information of existing process by ID.
	///
	/// @param id - process identifier.
	/// @return The function returns a data packet with process information.
	///
	virtual Variant getProcessInfoById(ProcId id) = 0;

	///
	/// Query information of existing process by PID.
	///
	/// @param pid - process pid.
	/// @return The function returns a data packet with process information.
	///
	virtual Variant getProcessInfoByPid(Pid pid) = 0;
};

///
/// The enum class of user classes.
///
enum class UserClass
{
	Unknown = 0,
	Guest = 1,
	User = 2,
	LocalAdmin = 3,
	DomainAdmin = 4,
	Service = 5,
	System = 6
};

///
/// Basic user information interface.
///
class IUserInformation : OBJECT_INTERFACE
{
public:

	///
	/// Query information about a user.
	///
	/// @param vParams - the object's parameters including the following fields:
	///   **id** - user descriptor id.
	///   **sid** - user SID.
	///   **name** - user name.
	/// @return The function returns a data packet with user information.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	virtual Variant getUserInfo(Variant vParams) = 0;

	///
	/// Query token information for process.
	///
	/// @param hProcess - a handle of the process.
	/// @param sSid - user SID, optional.
	/// @param fElevated - is user SID elevated.
	/// @return The function returns a data packet with user information.
	///
	virtual Variant getTokenInfo(const HANDLE hProcess, const std::string& sSid, const bool fElevated) = 0;

	///
	/// Query token information.
	///
	/// @param vParams - the object's parameters including the following fields:
	///   **pid** - pid of process.
	///   **sid** - user SID.
	///   **name** - user name.
	///   **isElevated** - is user's SID or name elevated.
	/// @return The function returns a data packet with user information.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	virtual Variant getTokenInfo(Variant vParams) = 0;
};

///
/// The interface for requesting a session-related information.
///
class ISessionInformation : OBJECT_INTERFACE
{
public:

	///
	/// Get info about current sessions.
	///
	/// @return The function returns a sequence of session descriptors.
	///
	virtual Variant getSessionsInfo() = 0;
};

///
/// Basic security token interface.
///
class IToken : OBJECT_INTERFACE
{
public:

	///
	/// Get token handle.
	///
	/// @return The function returns a HANDLE value.
	///
	virtual const HANDLE get() = 0;

	///
	/// Get token identifier.
	///
	/// @return The function returns a HANDLE value.
	///
	virtual TokenId getId() = 0;

	///
	/// Impersonate current thread.
	///
	/// @return The function returns a boolean status of the operation.
	///
	virtual bool impersonate() = 0;
};

///
/// Impersonate calling thread.
///
/// @param vParams - token object or a dictionary with optional fields "pid", "sid" or "name".
	/// @return The function returns a boolean status of the operation.
///
bool impersonateThread(Variant vParams);

///
/// Basic user impersonation interface.
///
class IPathConverter : OBJECT_INTERFACE
{
public:

	///
	/// Converts a path to "abstract" (C:\Windows\dir -> %windir%\dir).
	///
	/// @param sPath - original path.
	/// @param vToken - security token
	/// @return The function returns a string with converted abstract path.
	///
	virtual std::wstring getAbstractPath(std::wstring sPath, Variant vToken) = 0;

};

///
/// The enum class of path types
///
enum class PathType
{
	Unique = 0,
	HumanReadable = 1,
	NtPath = 2,
	Retry = 3,
};

// Max file size for hash calculation
static const io::IoSize g_nMaxStreamSize = 256 * 1024 * 1024;

///
/// Basic file information interface.
///
class IFileInformation : OBJECT_INTERFACE
{
public:

	///
	/// Converts a path and name to full absolute value.
	/// 
	/// @param sPathName - path to normalize.
	/// @param vSecurity - security descriptor for impersonation.
	/// @param eType - path type.
	/// @return The function returns a string with converted path.
	///
	virtual std::wstring normalizePathName(const std::wstring& sPathName, Variant vSecurity, PathType eType) = 0;

	///
	/// Get file information.
	///
	/// @param vFile - a file descriptor.
	/// @return The function returns a data packet with file information.
	///
	virtual Variant getFileInfo(Variant vFile) = 0;

	///
	/// Get file size.
	///
	/// @param vFile - a file descriptor.
	/// @return The function returns a size of the file.
	///
	virtual io::IoSize getFileSize(Variant vFile) = 0;

	///
	/// Get file hash.
	///
	/// @param vFile - a file descriptor.
	/// @return The function returns a string with SHA1-hash of the file content.
	///
	virtual std::string getFileHash(Variant vFile) = 0;

	///
	/// Get file stream.
	///
	/// @param vFile - a file descriptor.
	/// @return The function returns a smart pointer to the file stream.
	///
	virtual ObjPtr<io::IReadableStream> getFileStream(Variant vFile) = 0;

	///
	/// Get volume information.
	///
	/// @param vVolume - a volume descriptor.
	/// @return The function returns a data packet with volume information.
	///
	virtual Variant getVolumeInfo(Variant vVolume) = 0;
};

///
/// The enum class of signature statuses.
///
enum class SignStatus
{
	Valid = 0,
	Invalid = 1,
	Unsigned = 2,
	Untrusted = 3,
	Undefined = 4
};

///
/// Basic signature information interface.
///
class ISignatureInformation : OBJECT_INTERFACE
{
public:
	///
	/// Get signature information.
	///
	/// @param vFileName - a name of the file.
	/// @param vSecurity - security descriptor for impersonation.
	/// @param vFileHash - a hash of the file content.
	/// @return The function returns a data packet with signature-related information.
	///
	virtual Variant getSignatureInfo(Variant vFileName, Variant vSecurity, Variant vFileHash) = 0;
};

///
/// The enum class of impersonation types.
///
enum class ImpersonationType
{
	Local = 0,
	Ipc = 1,
};

} // namespace win
} // namespace sys
} // namespace cmd

/// @}
