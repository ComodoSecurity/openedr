//
// edrext
// 
// Author: Denis Bogdanov (17.09.2019)
//
/// @file Declarations of external EDR Agent API - network-related functions declaration
/// @addtogroup edrext
/// @{
///
#pragma once

namespace cmd {
namespace sys {
namespace net {

///
/// Structure contains data about discovered network interface
///
struct InterfaceInfo
{
	std::string sName;					//< Adapter/interface name
	std::wstring sDnsSuffix;			//< DNS suffix for this interface
	std::wstring sDescription;			//< Adapter description
	std::wstring sFriendlyName;			//< Friendly adapter name
	uint64_t nUpSpeed = 0;				//< Upload speed in Mb
	uint64_t nDownSpeed = 0;			//< Download speed in Mb
	size_t nMtu = 0;					//< MTU size
	bool fEnabled = false;				//< TRUE if adapter is switched on
	std::string sMacAddress;			//< MAC address
	std::vector<std::string> vIpV4Addr;	//< IPv4 addresses
	std::vector<std::string> vIpV6Addr;	//< IPv6 addresses
	std::vector<std::string> vIpV4Dns; 	//< IPv4 DNS addresses
	std::vector<std::string> vIpV6Dns;	//< IPv6 DNS addresses
};

typedef std::vector<InterfaceInfo> InterfacesInfo;
typedef std::unique_ptr<InterfacesInfo> InterfacesInfoPtr;

///
/// Get information about all installed network adaptors/interfaces (real or virtual)
///
/// @param fOnlyEnabled - Return only enabled (connected) adapters
/// @param fOnlyWithAssignedIp - Return only adapters with assigned IP address
/// @return - Smart pointer to array of adapters descriptors
///
InterfacesInfoPtr getInterfaces(bool fOnlyEnabled, bool fOnlyWithAssignedIp);

} // namespace net
} // namespace sys
} // namespace cmd
/// @} 
