//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (15.04.2019)
// Reviewer:
//
///
/// @file Common network functions declaration
///
/// @addtogroup cloudNetwork Network generic functions
/// @{
///
#include "pch.h"
#include <net.hpp>

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "net"

namespace openEdr {
namespace net {

//
//
//
std::vector<std::string> getHostByName(const std::string& sHostname)
{
	using boost::asio::ip::tcp;

	std::vector<std::string> addresses;

	// We're dealing with network/connectivity 'stuff' + you never know
	// when DNS queries will fail, so we need to handle exceptional cases
	CMD_TRY
	{
		// Setup the resolver query
		boost::asio::io_service ioService;
		tcp::resolver resolver(ioService);
		tcp::resolver::query query(sHostname, "");
		// Prepare response iterator
		tcp::resolver::iterator destination = resolver.resolve(query);
		tcp::resolver::iterator end;
		// Iterate through possible multiple responses
		while (destination != end)
		{
			tcp::endpoint endpoint = *destination++;
			addresses.push_back(endpoint.address().to_string());
		}
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.addTrace(SL, "Hostname is not found");
		throw;
	}
	return addresses;
}

//
//
//
std::vector<std::string> getHostByAddress(const std::string& sAddress)
{
	using boost::asio::ip::tcp;
	using boost::asio::ip::address_v4;

	std::vector<std::string> hostnames;

	// We're dealing with network/connectivity 'stuff' + you never know
	// when DNS queries will fail, so we need to handle exceptional cases
	try
	{
		// Setup the resolver query (for PTR record)
		address_v4 addr = address_v4::from_string(sAddress);
		boost::asio::ip::tcp::endpoint endpoint;
		endpoint.address(addr);
		boost::asio::io_service ioService;
		tcp::resolver resolver(ioService);
		// Prepare response iterator
		tcp::resolver::iterator destination = resolver.resolve(endpoint);
		tcp::resolver::iterator end;
		// Iterate through possible multiple responses
		for (int i = 1; destination != end; ++destination, ++i)
			hostnames.push_back(destination->host_name());
	}
	catch (const boost::system::system_error& ex)
	{
		error::RuntimeError(SL, FMT("Address is not found (" << ex.what() << ")")).throwException();
	}

	return hostnames;
}

//
// fSafe - if specified then function doesn't throw exception if system error
// is occurred (ONLY system error!)
//
Sequence getNetworkAdaptorsInfo(bool fOnlyOnline, bool fOnlyWithAssignedIp, bool fSafe)
{
	Sequence seqResult;
	try
	{
		auto pAdData = openEdr::sys::net::getInterfaces(fOnlyOnline, fOnlyWithAssignedIp);
		for (auto& adData : *pAdData)
		{
			Dictionary vData;
			vData.put("name", adData.sName);
			vData.put("dnsSuffix", adData.sDnsSuffix);
			vData.put("description", adData.sDescription);
			vData.put("friendlyName", adData.sFriendlyName);
			vData.put("upSpeed", adData.nUpSpeed);
			vData.put("downSpeed", adData.nDownSpeed);
			vData.put("mtu", adData.nMtu);
			vData.put("status", adData.fEnabled);
			vData.put("mac", adData.sMacAddress);

			Sequence seqAddresses4;
			for (auto& sAddr : adData.vIpV4Addr)
				seqAddresses4.push_back(sAddr);
			vData.put("ip4", seqAddresses4);

			Sequence seqAddresses6;
			for (auto& sAddr : adData.vIpV6Addr)
				seqAddresses6.push_back(sAddr);
			vData.put("ip6", seqAddresses6);

			Sequence seqDns4;
			for (auto& sAddr : adData.vIpV4Dns)
				seqDns4.push_back(sAddr);
			vData.put("dns4", seqDns4);

			Sequence seqDns6;
			for (auto& sAddr : adData.vIpV6Dns)
				seqDns6.push_back(sAddr);
			vData.put("dns6", seqDns6);

			seqResult.push_back(vData);
		}
	}
	catch (std::exception& )
	{
		if (!fSafe)
			throw;
	}
	return seqResult;
}

namespace detail {

//
//
//
void setSoketOption(boost::asio::detail::socket_type socket,
	int nOptionName, const void* pOptionValue, int nOptionLen)
{
	if (0 == setsockopt(socket, SOL_SOCKET, nOptionName, (const char*)pOptionValue, nOptionLen))
		return;
	error::win::WinApiError(SL, WSAGetLastError(),
		FMT("Can't setsockopt(), optname: " << nOptionName)).throwException();
}

} // namespace detail

} // namespace net
} // namespace openEdr

/// @}
