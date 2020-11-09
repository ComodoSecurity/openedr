// 
// Example of usage EDR Agent Raw API
//
#include "pch_win.h"
#include "cmd/sys.hpp"
#include "cmd/sys/net.hpp"

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> cvt;

	// getchar();

    std::cout << "Diagnostic information:\n";
	std::cout << "OS family: " << openEdr::sys::getOsFamilyName() << std::endl;
	std::cout << "OS name: " << openEdr::sys::getOsFriendlyName() << std::endl;
	std::cout << "Host name: " << cvt.to_bytes(openEdr::sys::getHostName()) << std::endl;
	std::cout << "Domain name: " << cvt.to_bytes(openEdr::sys::getDomainName()) << std::endl;
	std::cout << "User name: " << cvt.to_bytes(openEdr::sys::getUserName()) << std::endl;
	std::cout << "OS uptime: " << openEdr::sys::getOsUptime() << "ms" << std::endl;
	std::cout << "Local console user: " << cvt.to_bytes(openEdr::sys::getLocalConsoleUser()) << std::endl;

	time_t t = openEdr::sys::getOsBootTime() / 1000;
	std::cout << "OS last boot time: " << t << "s";
	std::tm tm = { 0 };
	if (localtime_s(&tm, &t) == 0)
		std::cout << " (" << std::put_time(&tm, "%F %T %z") << ")";
	std::cout << std::endl;
		
	auto pSesData = openEdr::sys::getSessionsInfo(false);
	for (auto& sesData : *pSesData)
	{
		std::cout << "Session [" << sesData.nId << "]:" << std::endl;
		std::cout << "    Name: " << cvt.to_bytes(sesData.sName) << std::endl;
		std::cout << "    User name: " << cvt.to_bytes(sesData.sUser) << std::endl;
		std::cout << "    User domain: " << cvt.to_bytes(sesData.sDomain) << std::endl;
		std::cout << "    Is active: " << sesData.fActive << std::endl;
		std::cout << "    Is local console: " << sesData.fLocalConsole << std::endl;
		std::cout << "    Is remote: " << sesData.fRemote << std::endl;

		time_t t = sesData.logonTime / 1000;
		std::cout << "    Logon time: " << t << "s";
		std::tm tm = {0};
		if (localtime_s(&tm, &t) == 0)
			std::cout << " (" << std::put_time(&tm, "%F %T %z") << ")";
		std::cout << std::endl;
	}
	
	auto pAdData = openEdr::sys::net::getInterfaces(false, false);
	for (auto& adData : *pAdData)
	{
		std::cout << "Network adaptor " << adData.sName << ":" << std::endl;
		std::cout << "    Friendly name: " << cvt.to_bytes(adData.sFriendlyName) << std::endl;
		std::cout << "    Description: " << cvt.to_bytes(adData.sDescription) << std::endl;
		std::cout << "    DNS suffix: " << cvt.to_bytes(adData.sDnsSuffix) << std::endl;
		std::cout << "    Active: " << adData.fEnabled << std::endl;
		std::cout << "    Up/Down speed (mb): " << adData.nUpSpeed/1000/1000
			<< "/" << adData.nDownSpeed / 1000 / 1000  << std::endl;
		std::cout << "    MAC address: " << adData.sMacAddress << std::endl;
		std::cout << "    MTU: " << adData.nMtu << std::endl;
		
		std::cout << "    IPv4 addresses: ";
		for (auto &sAddr : adData.vIpV4Addr)
			std::cout << "[" << sAddr << "] ";
		std::cout << std::endl;

		std::cout << "    IPv6 addresses: ";
		for (auto& sAddr : adData.vIpV6Addr)
			std::cout << "[" << sAddr << "] ";
		std::cout << std::endl;

		std::cout << "    DNSv4 addresses: ";
		for (auto& sAddr : adData.vIpV4Dns)
			std::cout << "[" << sAddr << "] ";
		std::cout << std::endl;

		std::cout << "    DNSv6 addresses: ";
		for (auto& sAddr : adData.vIpV6Dns)
			std::cout << "[" << sAddr << "] ";
		std::cout << std::endl;

	}
}
