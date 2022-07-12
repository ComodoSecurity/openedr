//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (09.03.2019)
// Reviewer: Denis Bogdanov (13.03.2019)
//
///
/// @file User Data Provider implementation
///
/// @addtogroup syswin System library (Windows)
/// @{
#include "pch.h"
#include "userdataprovider.h"

namespace cmd {
namespace sys {
namespace win {

#undef CMD_COMPONENT
#define CMD_COMPONENT "usrdtprv"

//
//
//
bool impersonateThread(Variant vParams)
{
	// New format of security token
	if (vParams.isObject())
		return queryInterface<IToken>(vParams)->impersonate();

	// Old way
	auto pUserDP = queryInterfaceSafe<IUserInformation>(queryServiceSafe("userDataProvider"));
	if (!pUserDP || !vParams.isDictionaryLike())
		return false;
	auto vToken = pUserDP->getTokenInfo(vParams);
	if (vToken.isEmpty() || !vToken.has("tokenObj"))
		return false;
	return queryInterface<IToken>(vToken["tokenObj"])->impersonate();
}


// URL: https://stackoverflow.com/questions/42762595/mapping-network-drive-using-wnetaddconnection2-from-run-as-administrator-proce
// URL: https://stackoverflow.com/questions/42767614/how-to-correctly-detect-network-drive-when-running-with-elevated-privileges
// URL: https://stackoverflow.com/questions/45871151/unable-to-get-the-associated-network-unc-of-a-mapped-drive-c-c

//////////////////////////////////////////////////////////////////////////
//
// UserDataProvider
//

//
//
//
UserDataProvider::UserDataProvider()
{
}

//
//
//
UserDataProvider::~UserDataProvider()
{
}

//
//
//
void UserDataProvider::finalConstruct(Variant vConfig)
{
	m_sEndpointId = getCatalogData("app.config.license.endpointId", "");

	wchar_t sCompName[MAX_COMPUTERNAME_LENGTH + 1] = {};
	DWORD nCompNameSize = DWORD(std::size(sCompName));
	if (!::GetComputerNameW(sCompName, &nCompNameSize))
		error::win::WinApiError(SL, "Fail to get computer name").throwException();
	m_sLocalSid = getSidByUserName(sCompName);
	LOGLVL(Debug, "Local machine SID: <" << m_sLocalSid << ">");
}

//
//
//
UserId UserDataProvider::generateId(const std::string& sSid)
{
	TRACE_BEGIN
	crypt::xxh::Hasher ctx;
	crypt::updateHash(ctx, sSid);
	// Check local accounts
	// TODO: Need more investigation and tests!
	if ((!m_sLocalSid.empty() && 
		string::startsWith(sSid, m_sLocalSid)) ||	// Local user
		string::startsWith(sSid, "S-1-5-18") ||		// LocalSystem
		string::startsWith(sSid, "S-1-5-19") ||		// LocalService
		string::startsWith(sSid, "S-1-5-20"))		// NetworkService
		crypt::updateHash(ctx, m_sEndpointId);
	return ctx.finalize();
	TRACE_END(FMT("Fail to generate id for SID <" << sSid << ">"))
}

//
//
//
std::wstring UserDataProvider::getUserNameBySid(const std::string& sSid)
{
	ScopedLocalMem pSid;
	if (!::ConvertStringSidToSidA(sSid.c_str(), &pSid))
		error::win::WinApiError(SL, "Fail to get user SID").throwException();

	const DWORD c_nBufferSize = 0x100;
	wchar_t sName[c_nBufferSize]; *sName = 0;
	DWORD nNameSize = c_nBufferSize;
	wchar_t sDomain[c_nBufferSize]; *sDomain = 0;
	DWORD nDomainSize = c_nBufferSize;
	SID_NAME_USE nUse;
	if (::LookupAccountSidW(nullptr, pSid, sName, &nNameSize, sDomain, &nDomainSize, &nUse))
		return std::wstring(sName) + L"@" + std::wstring(sDomain);
	auto ec = GetLastError();
	if (ec != ERROR_INSUFFICIENT_BUFFER)
	{
		LOGWRN("Fail to get user by SID <" << sSid << ">");
		return std::wstring();
	}

	std::wstring pName;
	pName.resize(nNameSize);
	std::wstring pDomain;
	pName.resize(nDomainSize);
	if (!::LookupAccountSidW(NULL, pSid, pName.data(), &nNameSize, pDomain.data(), &nDomainSize, &nUse))
		error::win::WinApiError(SL, "Fail to get user name by SID").throwException();
	if (!pName.empty())
		pName.pop_back();
	if (!pDomain.empty())
		pDomain.pop_back();

	return pName + L"@" + pDomain;
}

//
//
//
std::string UserDataProvider::getSidByUserName(std::wstring sUserName)
{
	auto nAtPos = sUserName.find(L'@');
	if (nAtPos != sUserName.npos)
	{
		std::wstring sUser(sUserName.substr(0, nAtPos));
		std::wstring sDomain(sUserName.substr(nAtPos + 1));
		sUserName = sDomain + L"\\" + sUser;
	}

	Byte pSid[c_nMaxBinSidSize];
	DWORD nSidSize = c_nMaxBinSidSize;
	wchar_t pDomainBuff[0x100];
	DWORD nDomainSize = DWORD(std::size(pDomainBuff));
	std::vector<wchar_t> pDomainDyn;

	SID_NAME_USE nUse;
	wchar_t* pDomain = pDomainBuff;
	if (!::LookupAccountNameW(nullptr, sUserName.c_str(), pSid, &nSidSize, pDomain, &nDomainSize, &nUse))
	{
		auto ec = GetLastError();
		if (ec != ERROR_INSUFFICIENT_BUFFER)
		{
			LOGWRN("Fail to get SID for user <" << string::convertWCharToUtf8(sUserName) << ">");
			return {};
		}
		if (nSidSize > std::size(pDomainBuff))
			error::OutOfRange(SL, FMT("Buffer size for SID is small, need <" << nSidSize << ">")).throwException();
		pDomainDyn.resize(nDomainSize);
		pDomain = pDomainDyn.data();
		if (!::LookupAccountNameW(nullptr, sUserName.c_str(), pSid, &nSidSize, pDomain, &nDomainSize, &nUse))
		{
			error::win::WinApiError(SL, FMT("Fail to get SID for user <" <<
				string::convertWCharToUtf8(sUserName) << ">")).log();
			return {};
		}
	}

	ScopedLocalMem sTmp;
	if (!::ConvertSidToStringSidA(pSid, (LPSTR*)&sTmp))
	{
		error::win::WinApiError(SL, "Fail to convert SID to string").log();
		return {};
	}

	return (char*)sTmp.get();
}

//
// URL: https://stackoverflow.com/questions/3493607/how-to-find-mymusic-folder-from-other-users
//
std::wstring UserDataProvider::getProfileDirBySid(const std::string& sSid)
{
	if (!string::startsWith(sSid, "S-1-5-18") &&	// LocalSystem
		!string::startsWith(sSid, "S-1-5-19") &&	// LocalService
		!string::startsWith(sSid, "S-1-5-20") &&	// NetworkService
		!string::startsWith(sSid, "S-1-5-21"))	 	// Local or domain user
		return {};

	std::wstring sKeyPath(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
	sKeyPath += string::convertUtf8ToWChar(sSid);
	DWORD nValueSize = 0;
	if (::RegGetValueW(HKEY_LOCAL_MACHINE, sKeyPath.data(),
		L"ProfileImagePath", RRF_RT_REG_SZ, NULL, NULL, &nValueSize))
	{
		LOGWRN("Fail to read profile data from registry for user <" << sSid << ">");
		return {};
	}
	std::vector<Byte> pValue;
	pValue.resize(nValueSize);
	if (::RegGetValueW(HKEY_LOCAL_MACHINE, sKeyPath.data(),
		L"ProfileImagePath", RRF_RT_REG_SZ, NULL, pValue.data(), &nValueSize))
	{
		error::win::WinApiError(SL, FMT("Fail to read profile data from registry for user <" << sSid << ">")).log();
		return {};
	}
	return (wchar_t*)pValue.data();
}

//
// URL: https://support.microsoft.com/en-us/help/243330/well-known-security-identifiers-in-windows-operating-systems
//
UserClass UserDataProvider::getUserClass(const std::string& sSid, const HANDLE hToken)
{
	if (sSid.empty())
		return UserClass::Unknown;
	if (sSid == "S-1-5-18")		// System
		return UserClass::System;
	if (sSid == "S-1-5-19" ||		// LocalService
		sSid == "S-1-5-20")		// NetworkService
		return UserClass::Service;

	auto eClass = UserClass::Unknown;
	if (eClass < UserClass::LocalAdmin && string::endsWith(sSid, "-500"))
		eClass = UserClass::LocalAdmin; // Local administrator
	if (eClass < UserClass::Guest && string::endsWith(sSid, "-501"))
		eClass = UserClass::Guest; // Local guest
	if (eClass < UserClass::User && string::startsWith(sSid, m_sLocalSid))
		eClass = UserClass::User; // Local user

	while (hToken)
	{
		DWORD nSize = 0;
		::GetTokenInformation(hToken, TokenGroups, NULL, nSize, &nSize);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			error::win::WinApiError(SL, "Fail to get token information size").log();
			break;
		}

		std::vector<uint8_t> pBuffer(nSize);
		if (!::GetTokenInformation(hToken, TokenGroups, pBuffer.data(), DWORD(pBuffer.size()), &nSize))
		{
			error::win::WinApiError(SL, "Fail to get token information").log();
			break;
		}

		TOKEN_GROUPS* pGroupInfo = (TOKEN_GROUPS*)pBuffer.data();
		for (Size n = 0; n < pGroupInfo->GroupCount && eClass < UserClass::System; ++n)
		{
			const auto& sid = pGroupInfo->Groups[n].Sid;
			const auto& attr = pGroupInfo->Groups[n].Attributes;
			if (!(attr & SE_GROUP_ENABLED) || (attr & SE_GROUP_USE_FOR_DENY_ONLY))
				continue;

/***
			// Uncomment for debug
			ScopedLocalMem sTmp;
			::ConvertSidToStringSidA(sid, (LPSTR*)& sTmp);
			std::string sGroupSid((char*)sTmp.get());
			auto sName = getUserNameBySid(sGroupSid);
/***/
			
			static const SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
			PSID_IDENTIFIER_AUTHORITY psia = GetSidIdentifierAuthority(sid);
			if (memcmp(psia, &ntAuthority, sizeof(SID_IDENTIFIER_AUTHORITY)))
				continue;

			const auto nSunAutCount = *GetSidSubAuthorityCount(sid);
			if (!nSunAutCount)
				continue;

			const auto nSubAuth = *GetSidSubAuthority(sid, 0);
			const auto nSubAuthLast = *GetSidSubAuthority(sid, nSunAutCount - 1);
			if (nSubAuth == SECURITY_NT_NON_UNIQUE)
			{
				if (eClass < UserClass::DomainAdmin && nSubAuthLast == DOMAIN_GROUP_RID_ADMINS)
					eClass = UserClass::DomainAdmin; // Domain administrators
				else if (eClass < UserClass::User && nSubAuthLast == DOMAIN_GROUP_RID_USERS)
					eClass = UserClass::User; // Domain users
				else if (eClass < UserClass::Guest && nSubAuthLast == DOMAIN_GROUP_RID_GUESTS)
					eClass = UserClass::Guest; // Domain guests
			}
			else if (nSubAuth == SECURITY_BUILTIN_DOMAIN_RID)
			{
				if (eClass < UserClass::LocalAdmin && nSubAuthLast == DOMAIN_ALIAS_RID_ADMINS)
					eClass = UserClass::LocalAdmin; // Administrators group
				if (eClass < UserClass::User && nSubAuthLast == DOMAIN_ALIAS_RID_USERS)
					eClass = UserClass::User; // Users group
				if (eClass < UserClass::Guest && nSubAuthLast == DOMAIN_ALIAS_RID_GUESTS)
					eClass = UserClass::Guest; // Guests group
				if (eClass < UserClass::User && nSubAuthLast == DOMAIN_ALIAS_RID_POWER_USERS)
					eClass = UserClass::User; // Power users group
			}
/*
			else if (eClass < UserClass::System && nSubAuth == SECURITY_LOCAL_SYSTEM_RID)
				eClass = UserClass::System;
			else if (eClass < UserClass::Service &&
				(nSubAuth == SECURITY_LOCAL_SERVICE_RID || nSubAuth == SECURITY_NETWORK_SERVICE_RID))
				eClass = UserClass::Service;
*/
		}
		break;
	}

	return eClass;
}


//
//
//
Variant UserDataProvider::getUserInfoById(UserId sUserId)
{
	TRACE_BEGIN
	std::scoped_lock _lock(m_mtxUser);
	auto itUser = m_pIdMap.find(sUserId);
	if (itUser != m_pIdMap.end())
		return itUser->second;
	error::NotFound(SL, FMT("Can't find user <" << sUserId << ">")).throwException();
	TRACE_END(FMT("Fail to get user info"))
}

//
//
//
Variant UserDataProvider::getUserInfoBySid(const std::string& sSid)
{
	TRACE_BEGIN
	if (sSid.empty())
		return {};

	{
		std::scoped_lock _lock(m_mtxUser);
		auto itUser = m_pSidMap.find(sSid.data());
		if (itUser != m_pSidMap.end())
			return getUserInfoById(itUser->second);
	}

	Dictionary vUser;
	UserId sId = generateId(sSid);
	vUser.put("id", sId);
	vUser.put("sid", sSid);
	auto sUserName = getUserNameBySid(sSid);
	if (!sUserName.empty())
		vUser.put("name", sUserName);
	auto sProfileDir = getProfileDirBySid(sSid);
	if (!sProfileDir.empty())
	{
		vUser.put("%profileDir%", sProfileDir);
		vUser.put("%userRegKey%", std::string("HKEY_USERS\\") + sSid.data());
	}
	
	{
		std::scoped_lock _lock(m_mtxUser);
		m_pIdMap[sId] = vUser;
		m_pSidMap[sSid.data()] = sId;
		if (!sUserName.empty())
			m_pUserMap[sUserName] = sId;
	}

	LOGLVL(Debug, "New user: SID <" << sSid << "> to <" << sId << ">");
	return vUser;
	TRACE_END(FMT("Fail to get user info"))
}

//
//
//
Variant UserDataProvider::getUserInfoByName(const std::wstring& sUserName)
{
	TRACE_BEGIN
	{
		std::scoped_lock _lock(m_mtxUser);
		auto itUser = m_pUserMap.find(sUserName.data());
		if (itUser != m_pUserMap.end())
			return getUserInfoById(itUser->second);
	}

	return getUserInfoBySid(getSidByUserName(sUserName));
	TRACE_END(FMT("Fail to get user info"))
}

//
//
//
Variant UserDataProvider::getUserInfo(Variant vParams)
{
	TRACE_BEGIN
	if (vParams.has("id"))
		return getUserInfoById(vParams["id"]);
	if (vParams.has("sid"))
		return getUserInfoBySid(vParams["sid"]);
	else if (vParams.has("name"))
		return getUserInfoByName(vParams["name"]);
	else
		error::InvalidArgument(SL).throwException();
	TRACE_END(FMT("Fail to get token info"))

}


//
//
//
Variant UserDataProvider::getTokenInfo(HANDLE hProcess, const std::string& sSid, const bool fElevated)
{
	TRACE_BEGIN
	Dictionary vTokenInfo;

	ScopedHandle hProcess2;
	if (!hProcess)
	{
		hProcess2.reset(searchProcessBySid(sSid, fElevated));
		hProcess = hProcess2.get();
	}

	HANDLE hToken = NULL;
	if (::OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken) ||
		::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
	{
		// No need to call CloseHandle() for hToken
		auto pToken = createObject(CLSID_Token, Dictionary({ {"handle", ULONG_PTR(hToken)} }));
		vTokenInfo.put("tokenObj", pToken);
	}

	Variant vUserInfo = getUserInfoBySid(sSid);
	if (vUserInfo.isEmpty())
		return vTokenInfo;

	vUserInfo = vUserInfo.clone();
	vUserInfo.put("class", getUserClass(sSid, hToken));
	vTokenInfo.put("user", vUserInfo);
	crypt::xxh::Hasher ctx;
	crypt::updateHash(ctx, sSid);
	crypt::updateHash(ctx, fElevated);
	vTokenInfo.put("id", ctx.finalize());
	return vTokenInfo;
	TRACE_END(FMT("Fail to get token info"))
}

//
//
//
Variant UserDataProvider::getTokenInfo(Variant vParams)
{
	if (vParams.isEmpty())
		error::InvalidArgument(SL).throwException();

	sys::win::ScopedHandle hProcess;
	if (vParams.has("pid"))
	{
		Pid nPid = vParams["pid"];
		hProcess.reset(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nPid));
		if (!hProcess)
			hProcess.reset(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, nPid));
	}
	std::string sSid = vParams.get("sid", "");
	if (sSid.empty() && vParams.has("name"))
		sSid = getSidByUserName(vParams.get("name", ""));
	return getTokenInfo(hProcess, sSid, vParams.get("isElevated", false));
}

//
//
//
Variant UserDataProvider::getSessionsInfo()
{
	auto pSesData = cmd::sys::getSessionsInfo(true);
	Sequence vSessions;
	for (auto& sesData : *pSesData)
	{
		Dictionary vSession;
		vSession.put("id", sesData.nId);
		vSession.put("logonTime", sesData.logonTime);
		vSession.put("isLocalConsole", sesData.fLocalConsole);
		vSession.put("isActive", sesData.fActive);
		vSession.put("isRemote", sesData.fRemote);
		vSession.put("userName", sesData.sUser);
		vSession.put("domain", sesData.sDomain);

		vSessions.push_back(vSession);
	}

	return vSessions;
}

//
//
//
void UserDataProvider::loadState(Variant vState)
{
}

//
//
//
cmd::Variant UserDataProvider::saveState()
{
	return {};
}

//
//
//
void UserDataProvider::start()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "User Data Provider is being started");
	LOGLVL(Detailed, "User Data Provider is started");
	TRACE_END("Fail to start User Data Provider");
}

//
//
//
void UserDataProvider::stop()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "User Data Provider is being stopped");
	LOGLVL(Detailed, "User Data Provider is stopped");
	TRACE_END("Fail to stop User Data Provider");
}

//
//
//
void UserDataProvider::shutdown()
{
	LOGLVL(Detailed, "User Data Provider is being shutdowned");
	std::scoped_lock _lock(m_mtxUser);
	m_pIdMap.clear();
	m_pSidMap.clear();
	m_pUserMap.clear();
	LOGLVL(Detailed, "User Data Provider is shutdowned");
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * 'objects.userDataProvider'
///
/// #### Supported commands
///
Variant UserDataProvider::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### getUserInfoById()
	/// Get user information.
	///   * **id** [str] - user identifier.
	///   * **sid** [str] - user SID.
	///   * **name** [str] - user name ("domain\user" or "user@domain").
	/// Returns a user descriptor.
	///
	if (vCommand == "getUserInfo")
		return getUserInfo(vParams);

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### getSessionsInfo()
	/// Get information about current sessions.
	/// Returns a sequence of session descriptors.
	///
	if (vCommand == "getSessionsInfo")
		return getSessionsInfo();

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	if (vCommand == "start")
	{
		start();
		return {};
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	if (vCommand == "stop")
	{
		stop();
		return {};
	}

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("ProcessDataProvider doesn't support a command <"
		<< vCommand << ">")).throwException();
}




//////////////////////////////////////////////////////////////////////////
//
// Create user path replacer
//

// Replaced path
struct ReplacedPath
{
	std::wstring sPath;
	std::wstring sTemplate;

	ReplacedPath() = default;
	ReplacedPath(std::wstring_view _sPath, std::wstring_view _sTemplate) :
		sPath(_sPath), sTemplate(_sTemplate) {}
};

//
// Extract replaced path from the system
//
std::vector<ReplacedPath> createRawReplacedPaths(Variant vToken)
{
	std::vector<ReplacedPath> replacedRawPaths;

	struct KnownPath
	{
		KNOWNFOLDERID ePathId;
		std::wstring sTemplate;
	};


	// Add common paths from SHGetKnownFolderPath
	KnownPath commonKnownPaths[] = {
		{FOLDERID_ProgramData, L"%programdata%"},
		{FOLDERID_Windows, L"%systemroot%"},
		{FOLDERID_ProgramFilesX86, L"%programfiles%"},

		// FOLDERID_ProgramFilesX64 is supported in x64 application only.
		#ifdef CPUTYPE_AMD64
			{FOLDERID_ProgramFilesX64, L"%programfiles%"},
		#endif // CPUTYPE_AMD64
	};

	auto pToken = queryInterfaceSafe<IToken>(vToken);
	auto hToken = pToken ? pToken->get() : nullptr;

	for (auto knownPath : commonKnownPaths)
	{
		auto sPath = getKnownPath(knownPath.ePathId, hToken);
		if (!sPath.empty())
			replacedRawPaths.emplace_back(sPath, knownPath.sTemplate);
	}

	// Add user paths from SHGetKnownFolderPath
	if (!vToken.isEmpty())
	{
		KnownPath userKnownPaths[] = {
			{FOLDERID_RoamingAppData, L"%appdata%"},
			{FOLDERID_LocalAppData, L"%localappdata%"},
			{FOLDERID_Profile, L"%userprofile%"},
		};

		for (auto knownPath : userKnownPaths)
		{
			auto sPath = getKnownPath(knownPath.ePathId, hToken);
			if (!sPath.empty())
				replacedRawPaths.emplace_back(sPath, knownPath.sTemplate);
		}
	}

	// Add %systemdrive%
	auto pathSystemDrive = std::filesystem::path(getKnownPath(FOLDERID_Windows, hToken)).root_name();
	if (!pathSystemDrive.empty())
		replacedRawPaths.emplace_back(pathSystemDrive.wstring() + L"\\", L"%systemdrive%\\");

	return replacedRawPaths;
}

//
// Create StringMatcher for User path replacing
//
ObjPtr<IStringMatcher> createUserPathReplacer(Variant vSecurity)
{
	// Step 1. Get raw paths
	auto rawPaths = createRawReplacedPaths(vSecurity);

	// Step 2. Apply normalization
	std::vector<ReplacedPath> normalizedPaths;
	ObjPtr<IFileInformation> pFileInformation = queryInterface<IFileInformation>(queryService("fileDataProvider"));
	for (auto& rawPath : rawPaths)
	{
		// Add both DOS and volume paths
		for (PathType eType : { PathType::Unique, PathType::HumanReadable })
		{
			CMD_TRY
			{
				auto sNormPath = pFileInformation->normalizePathName(rawPath.sPath, vSecurity, eType);
				normalizedPaths.emplace_back(sNormPath, rawPath.sTemplate);
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL, FMT("Can't normalize path <" << string::convertWCharToUtf8(rawPath.sPath) << ">"));
			}
		}
	}

	// Step 3. Create string matcher

	Sequence vSchema;
	for (auto& replacedPath : normalizedPaths)
	{
		Dictionary vRule = {
			{"target", replacedPath.sPath},
			{"value", replacedPath.sTemplate},
			{"mode", variant::operation::MatchMode::Begin},
		};

		vSchema.push_back(vRule);
	}

	return queryInterface<IStringMatcher>(createObject(CLSID_StringMatcher, Dictionary({ {"schema", vSchema } })));
}


//
//
//
void PathConverter::finalConstruct(Variant vConfig)
{

}

static const wchar_t c_sSysWOW64[] = L"\\syswow64";
static const wchar_t c_sSysArm64[] = L"\\sysarm64";

//
//
//
std::wstring PathConverter::getAbstractPath(std::wstring sPath, Variant vSecurity)
{
	TokenId nTokenId = vSecurity.isEmpty() ? TokenId() : queryInterface<IToken>(vSecurity)->getId();

	// get StringMatcher
	ObjPtr<IStringMatcher> pMatcher;
	{
		std::scoped_lock lock(m_mtxMatcherMap);
		auto itMatcher = m_pMatcherMap.find(nTokenId);
		if (itMatcher != m_pMatcherMap.end())
			pMatcher = itMatcher->second;
		else
		{
			pMatcher = createUserPathReplacer(vSecurity);
			m_pMatcherMap[nTokenId] = pMatcher;
		}
	}
	if (pMatcher == nullptr)
	{
		error::RuntimeError(SL, "Can't get string matcher").log();
		return sPath;
	}

	// EDR-2259: Cannot match file rule for 32-bit process on 64-bit OS
	auto itNode = sPath.find(c_sSysWOW64);
	if (itNode == sPath.npos)
		itNode = sPath.find(c_sSysArm64);
	if (itNode != sPath.npos)
		sPath.replace(itNode, std::wcslen(c_sSysWOW64), L"\\system32");

	return pMatcher->replace(sPath);
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * 'objects.pathConverter'
///
/// #### Supported commands
///
Variant PathConverter::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant PathConverter::execute()
	///
	/// ##### getAbstractPath()
	/// Get abstract path with user context.
	///   * **path** [str] - file pathname.
	///   * **security** [variant] - security token.
	/// Returns a string with file's abstract path.
	///
	if (vCommand == "getAbstractPath")
		return getAbstractPath(vParams["path"], vParams["security"]);


	///
	/// @fn Variant PathConverter::execute()
	///
	/// ##### start()
	/// Start driver and controller
	///
/*
	if (vCommand == "start")
	{
		start();
		return {};
	}
*/

	///
	/// @fn Variant PathConverter::execute()
	///
	/// ##### stop()
	/// Stop controller
	///
/*
	if (vCommand == "stop")
	{
		stop();
		return {};
	}
*/

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("PathConverter doesn't support a command <"
		<< vCommand << ">")).throwException();
}

} // namespace win
} // namespace sys
} // namespace cmd 

/// @}
