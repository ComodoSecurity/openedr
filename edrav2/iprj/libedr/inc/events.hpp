//
// edrav2.libedr project
// 
// Author: Podpruzhnikov Yury (02.10.2019)
// Reviewer: 
//
///
/// @file Declaration of shared data for events
///
/// @addtogroup edr
/// @{
#pragma once

namespace cmd {

///
/// IP protocols numbers.
///
enum class IpProtocol : uint16_t
{
	Tcp = 6,
	Udp = 17,
	Unknown = 256, ///< Special constant for "unknown" protocol
};

///
/// Application protocols.
///
enum class AppProtocol : uint16_t
{
	Unknown = 0,
	HTTP = 1,
	FTP = 2,
};

} // namespace cmd

/// @} 
