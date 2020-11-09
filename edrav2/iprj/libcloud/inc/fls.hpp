//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (19.03.2019)
// Reviewer: Denis Bogdanov (25.03.2019)
//
///
/// @file File Lookup Service (FLS) interface declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once

namespace openEdr {
namespace cloud {
namespace fls {

///
/// FLS file verdict enum.
///
enum class FileVerdict : uint8_t
{
	Absent = 0, ///< File (?) doesn't exist
	Safe = 1, ///< File is safe
	Malicious = 2, ///< File is malicious
	Unknown = 3, ///< Verdict is unknown
	Fail = 4 ///< Unable to determine a verdict
};

using Hash = std::string;

//
//
//
inline std::ostream& operator<<(std::ostream& os, const FileVerdict& verdict)
{
	return (os << std::to_string(static_cast<uint8_t>(verdict)));
}

///
/// FLS client interface.
///
class IFlsClient : OBJECT_INTERFACE
{

public:

	///
	/// Queries a verdict for the file.
	///
	/// @param sHash - a string (40 chars) with SHA1 hash of the file's content.
	/// @return The function returns a verdict value from FileVerdict enum.
	///
	virtual FileVerdict getFileVerdict(const Hash& hash) = 0;

	///
	/// Checks if verdict for the file is ready.
	///
	/// @param sHash - a string (40 chars) with SHA1 hash of the file's content.
	/// @return The function returns `true` if a verdict is ready.
	///
	virtual bool isFileVerdictReady(const Hash& hash) = 0;
};

} // namespace fls
} // namespace cloud
} // namespace openEdr

/// @} 
