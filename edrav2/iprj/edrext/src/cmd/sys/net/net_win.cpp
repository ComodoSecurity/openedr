//
// edrext
// 
// Author: Denis Bogdanov (17.09.2019)
//
/// @file Declarations of external EDR Agent API - network-related functions implementation (Windows)
/// @addtogroup edrext
/// @{
///
#include "pch_win.h"
#include <cmd/sys/net.hpp>

namespace cmd {
namespace sys {
namespace net {

#ifndef _WIN32
#error Please provide the code for non-wondows platform 
#endif // !_WIN32

//
//
//
InterfacesInfoPtr getInterfaces(bool fOnlyEnabled, bool fOnlyWithAssignedIp)
{
	ULONG nBufferSize = 0;	
	std::unique_ptr<char[]> pData(new char[15000]); // Allocate a 15 KB buffer to start with as recommended.

	for (size_t nTriesCount = 3;;--nTriesCount)
	{
		auto dwRes = ::GetAdaptersAddresses(AF_UNSPEC,
			GAA_FLAG_INCLUDE_ALL_INTERFACES | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_ANYCAST,
			NULL, (PIP_ADAPTER_ADDRESSES)pData.get(), &nBufferSize);
		// No adaptors are found
		if (dwRes == ERROR_NO_DATA)
			return std::make_unique<InterfacesInfo>();
		if (dwRes == ERROR_SUCCESS)
			break;
		if (dwRes != ERROR_BUFFER_OVERFLOW || nTriesCount == 0)
			throw std::system_error(dwRes, std::system_category(), "Fail to enum network adaptors");
	
		pData.reset(new char[nBufferSize]);
	}

	// If successful, output some information from the data we received
	InterfacesInfoPtr pResInfo = std::make_unique<InterfacesInfo>();
	for (auto pCurrAddresses = (PIP_ADAPTER_ADDRESSES)pData.get();
		pCurrAddresses != nullptr; pCurrAddresses = pCurrAddresses->Next)
	{
		bool fEnabled = pCurrAddresses->OperStatus == IfOperStatusUp;
		if (fOnlyEnabled && !fEnabled)
			continue;

		InterfaceInfo data;
		data.sName = pCurrAddresses->AdapterName;
		data.sDnsSuffix = pCurrAddresses->DnsSuffix;
		data.sDescription = pCurrAddresses->Description;
		data.sFriendlyName = pCurrAddresses->FriendlyName;
		data.nUpSpeed = (uint64_t)pCurrAddresses->TransmitLinkSpeed;
		data.nDownSpeed = (uint64_t)pCurrAddresses->ReceiveLinkSpeed;
		data.nMtu = pCurrAddresses->Mtu;
		data.fEnabled = fEnabled;

		// Fetch IP addresses
		for (auto pUnicast = pCurrAddresses->FirstUnicastAddress;
			pUnicast != nullptr; pUnicast = pUnicast->Next)
		{
			auto eType = pUnicast->Address.lpSockaddr->sa_family;
			if (eType == AF_INET)
			{
				std::string sAddress(size_t(INET_ADDRSTRLEN), 0);
				if (inet_ntop(AF_INET, &((sockaddr_in*)pUnicast->Address.lpSockaddr)->sin_addr,
					&sAddress[0], INET_ADDRSTRLEN) != nullptr)
				{
					sAddress.resize(strnlen_s(sAddress.data(), sAddress.size()));
					data.vIpV4Addr.push_back(sAddress);
				}
			}
			else if (eType == AF_INET6)
			{
				std::string sAddress(size_t(INET6_ADDRSTRLEN), 0);
				if (inet_ntop(AF_INET6, &((sockaddr_in6*)pUnicast->Address.lpSockaddr)->sin6_addr,
					&sAddress[0], INET6_ADDRSTRLEN) != nullptr)
				{
					sAddress.resize(strnlen_s(sAddress.data(), sAddress.size()));
					data.vIpV6Addr.push_back(sAddress);
				}
			}
		}

		// Fetch DNS addresses
		for (auto pDnsServer = pCurrAddresses->FirstDnsServerAddress;
			pDnsServer != nullptr; pDnsServer = pDnsServer->Next)
		{
			auto eType = pDnsServer->Address.lpSockaddr->sa_family;
			if (eType == AF_INET)
			{
				std::string sAddress(size_t(INET_ADDRSTRLEN), 0);
				if (inet_ntop(AF_INET, &((sockaddr_in*)pDnsServer->Address.lpSockaddr)->sin_addr,
					&sAddress[0], INET_ADDRSTRLEN) != nullptr)
				{
					sAddress.resize(strnlen_s(sAddress.data(), sAddress.size()));
					data.vIpV4Dns.push_back(sAddress);
				}
			}
			else if (eType == AF_INET6)
			{
				std::string sAddress(size_t(INET6_ADDRSTRLEN), 0);
				if (inet_ntop(AF_INET6, &((sockaddr_in6*)pDnsServer->Address.lpSockaddr)->sin6_addr,
					&sAddress[0], INET6_ADDRSTRLEN) != nullptr)
				{
					sAddress.resize(strnlen_s(sAddress.data(), sAddress.size()));
					data.vIpV6Dns.push_back(sAddress);
				}
			}
		}

		// Fetch MAC addresses
		std::stringstream sMac;
		for (ULONG i = 0; i < pCurrAddresses->PhysicalAddressLength; i++)
		{
			sMac << std::setw(2) << std::setfill('0') << std::hex << std::uppercase <<
				(int)pCurrAddresses->PhysicalAddress[i];

			if (i != pCurrAddresses->PhysicalAddressLength - 1)
				sMac << ":";
		}
		data.sMacAddress = sMac.str();

		if (!fOnlyWithAssignedIp || !data.vIpV4Addr.empty() || !data.vIpV6Addr.empty())
			pResInfo->push_back(std::move(data));
	}

	return pResInfo;
}

} // namespace net
} // namespace sys
} // namespace cmd 
/// @}
