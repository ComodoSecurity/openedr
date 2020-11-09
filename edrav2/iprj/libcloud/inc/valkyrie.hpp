//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (08.04.2019)
// Reviewer: Denis Bogdanov (18.04.2019)
//
///
/// @file Valkyrie interface declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once

namespace openEdr {
namespace cloud {
namespace valkyrie {

///
/// Valkyrie client interface.
///
class IValkyrieClient : OBJECT_INTERFACE
{

public:

	///
	/// Submits a file to the Valkyrie.
	///
	/// @param vFile - a file descriptor.
	/// @returns The function returns `true` if the file has been sent or
	/// was sent earlier or doesn't need to be sent.
	///
	virtual bool submit(Variant vFile) = 0;
};

} // namespace valkyrie
} // namespace cloud
} // namespace openEdr

/// @} 
