//
// edrext
// 
// Author: Denis Bogdanov (17.09.2019)
//
/// @file Declarations of external EDR Agent API - system-related functions implementation
/// @addtogroup edrext
/// @{
///
#include "pch_win.h"

namespace cmd {
namespace sys {

#ifndef _WIN32
#error Please provide the code for non-Windows platform 
#endif // !_WIN32

//
//
//
std::wstring getHostName()
{
	DWORD nSize = 0;
	::GetComputerNameExW(ComputerNamePhysicalNetBIOS, NULL, &nSize);
	
	std::wstring sName(nSize, 0);
	if (!::GetComputerNameExW(ComputerNamePhysicalNetBIOS, &sName[0], &nSize))
		return {};

	// Remove trailing zero char
	if (!sName.empty())
		sName.resize(sName.size() - 1);

	return sName;
}

//
//
//
std::wstring getDomainName()
{
	DWORD nSize = 0;
	::GetComputerNameExW(ComputerNameDnsDomain, NULL, &nSize);

	std::wstring sName(nSize, 0);
	if (!::GetComputerNameExW(ComputerNameDnsDomain, &sName[0], &nSize))
		return {};

	// Remove trailing zero char
	if (!sName.empty())
		sName.resize(sName.size() - 1);

	return sName;
}

//
//
//
std::wstring getUserName()
{
	DWORD nSize = UNLEN;
	std::wstring sName(UNLEN, 0);
	if (::GetUserNameExW(NameUserPrincipal, &sName[0], &nSize))
	{
		// We don't need to delete terminated null
		sName.resize(size_t(nSize));
		return sName;
	}

	nSize = UNLEN;
	if (!::GetUserNameW(&sName[0], &nSize))
		return {};

	// We need to delete terminated null
	sName.resize(size_t(nSize - 1));
	
	auto sDomain = getHostName();
	if (!sDomain.empty())
	{
		std::transform(sDomain.begin(), sDomain.end(),
			sDomain.begin(), std::towlower);
		sName += L"@" + sDomain;
	}
	return sName;
}

//
//
//
std::string readStringFromRegistry(HKEY hRootKey, const char* sPath, const char* sValue)
{
	// Get data size
	DWORD dwDataSize = 0;
	LSTATUS eRes = RegGetValueA(hRootKey, sPath, sValue, RRF_RT_REG_SZ, nullptr,
		nullptr, &dwDataSize);
	if (eRes != ERROR_SUCCESS) return {};

	// Get data value
	std::string sData(size_t(dwDataSize / sizeof(char)), 0);
	eRes = RegGetValueA(hRootKey, sPath, sValue, RRF_RT_REG_SZ, nullptr,
		&sData[0], &dwDataSize);
	if (eRes != ERROR_SUCCESS) return {};

	// Remove trailing zero char
	// Attention: dwDataSize is can be changed after second call RegGetValueA
	// it can be decreased on 1 if sData contains null-terminator
	if (!sData.empty())
		sData.resize(dwDataSize / sizeof(char) - 1);

	return sData;
}

//
//
//
std::string getOsFriendlyName()
{
	auto sProductName = readStringFromRegistry(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName");
	if (sProductName.empty())
		sProductName = "Windows";

	auto sReleaseId = readStringFromRegistry(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ReleaseId");
	if (!sReleaseId.empty())
		sReleaseId += " ";

	auto osVersion = getOsVersion();

	return sProductName + " " + sReleaseId + "(" + std::to_string(osVersion.nMajor) +
		"." + std::to_string(osVersion.nMinor) + "." + std::to_string(osVersion.nBuild) + ")";
}

//
//
//
std::string getOsFamilyName()
{
	return "Windows";
}

//
//
//
OsVersion getOsVersion()
{
	OSVERSIONINFO osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable: 4996)
	::GetVersionEx(&osvi);
#pragma warning(pop)
	return { osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber, 0 };
}

//
//
//
uint64_t getOsUptime()
{
	return ::GetTickCount64();
}

//
// TODO: Move function to platform-independent sys.cpp
//
uint64_t getOsBootTime()
{
	uint64_t timeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000;
	return timeNow - getOsUptime();
}

//
//
//
SessionsInfoPtr getSessionsInfo(bool fOnlyActive)
{
	DWORD nSessionsCount = 0;
	WTS_SESSION_INFOW* pSession = nullptr;
	if (!::WTSEnumerateSessionsW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSession, &nSessionsCount))
	{
		auto dwRes = ::GetLastError();
		throw std::system_error(dwRes, std::system_category(), "Fail to enum current sessions");
	}

	auto nConsoleSession = WTSGetActiveConsoleSessionId();

	SessionsInfoPtr pSessionsInfo = std::make_unique<SessionsInfo>();
	for (DWORD i = 0; i < nSessionsCount; i++)
	{
		SessionInfo sesInfo;
		if (fOnlyActive && pSession[i].State != WTSActive)
			continue;

		sesInfo.nId = pSession[i].SessionId;
		sesInfo.sName = pSession[i].pWinStationName;
		sesInfo.fActive = pSession[i].State == WTSActive;
		sesInfo.fLocalConsole = pSession[i].SessionId == nConsoleSession;

		DWORD nDataSize = 0;
		PWTSINFOW pSessionInfo = NULL;
		if (::WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, pSession[i].SessionId,
			WTSSessionInfo, reinterpret_cast<LPWSTR*>(&pSessionInfo), &nDataSize))
		{			
			sesInfo.logonTime = pSessionInfo->LogonTime.QuadPart == 0? 0 :
				((pSessionInfo->LogonTime.QuadPart / 10000) - 11644473600000LL);
			sesInfo.sUser = pSessionInfo->UserName;
			sesInfo.sDomain = pSessionInfo->Domain;
			std::transform(sesInfo.sDomain.begin(), sesInfo.sDomain.end(),
				sesInfo.sDomain.begin(), std::towlower);
			::WTSFreeMemory(pSessionInfo);
		}

		DWORD* pIsRemote = NULL;
		if (::WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, pSession[i].SessionId,
			WTSIsRemoteSession, reinterpret_cast<LPWSTR*>(&pIsRemote), &nDataSize))
		{
			sesInfo.fRemote = *pIsRemote == TRUE;
			::WTSFreeMemory(pIsRemote);
		}

		pSessionsInfo->push_back(std::move(sesInfo));
	}

	::WTSFreeMemory(pSession);

	return pSessionsInfo;
}

//
//
//
std::wstring getLocalConsoleUser()
{
	auto pSesData = cmd::sys::getSessionsInfo(true);
	std::wstring sUserName;
	for (auto& sesData : *pSesData)
	{
		if (sesData.fLocalConsole)
		{
			sUserName = sesData.sUser;
			if (!sesData.sDomain.empty())
				sUserName += L"@" + sesData.sDomain;
			break;
		}
		else if (sUserName.empty())
		{
			sUserName = sesData.sUser;
			if (!sesData.sDomain.empty())
				sUserName += L"@" + sesData.sDomain;
		}
	}
	if (sUserName.empty())
		sUserName = L"unknown";
	return sUserName;
}

} // namespace sys
} // namespace cmd 
/// @}
