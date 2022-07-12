//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (14.08.2019)
// Reviewer: Denis Bogdanov
//
///
/// @file Network Monitor Controller declaration
///
#pragma once

namespace cmd {
namespace netmon {

inline constexpr uint64_t c_nDefaultEventsFlags = 0xffffffffffffffffui64;

enum class ConnectionStatus : uint32_t
{
	Success = 0,
	GenericFail = 1,
};

///
/// NetMon raw event id.
///
enum class RawEvent : uint32_t
{
	NETMON_CONNECT_IN = 0x0000,
	NETMON_CONNECT_OUT = 0x0001,
	NETMON_LISTEN = 0x0002,
	NETMON_REQUEST_DNS = 0x0003,
	NETMON_REQUEST_DATA_HTTP = 0x0004,
	NETMON_REQUEST_DATA_FTP = 0x0004,

	_Max, //< "last" event. for internal usage
};

} // namespace netmon
} // namespace cmd