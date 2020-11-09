//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (09.03.2019)
// Reviewer: Denis Bogdanov (13.03.2019)
//
///
/// @file User Data Provider declaration
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once
#include <objects.h>

namespace openEdr {
namespace sys {
namespace win {

class Token : public ObjectBase <CLSID_Token>,
	public IToken,
	public ISerializable
{
private:
	ScopedHandle hToken;

	std::string getUserSid()
	{
		if (!hToken)
			return {};
		CMD_TRY
		{
			auto pInfo = getTokenInformation(hToken, TokenUser);
			ScopedLocalMem sTmp;
			if (!::ConvertSidToStringSidA(PTOKEN_USER(pInfo.data())->User.Sid, (LPSTR*)& sTmp))
				error::win::WinApiError(SL, "Fail to convert SID to string").throwException();
			return (char*)sTmp.get();
		}
		CMD_CATCH("Fail to get token SID");
		return {};
	}

	bool isElevated()
	{
		if (!hToken)
			return false;
		CMD_TRY
		{
			auto pInfo = getTokenInformation(hToken, TokenElevation);
			return PTOKEN_ELEVATION(pInfo.data())->TokenIsElevated != 0;
		}
		CMD_CATCH("Fail to get token elevation");
		return false;
	}

public:

	void finalConstruct(Variant vConfig)
	{
		if (vConfig.isEmpty())
			error::InvalidArgument(SL).throwException();
		if (vConfig.has("handle"))
			hToken.reset(HANDLE(ULONG_PTR(vConfig["handle"])));
		else if (vConfig.has("pid"))
		{
			Pid nPid = vConfig["pid"];
			ScopedHandle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nPid));
			if (!hProcess)
				hProcess.reset(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, nPid));
			if (hProcess)
			{
				if (!::OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken) &&
					!::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
					error::win::WinApiError(SL, FMT("Fail to query process token, pid <" << nPid << ">")).log();
			}
			else
				error::win::WinApiError(SL, FMT("Fail to open process, pid <" << nPid << ">")).log();
		}
		else if (vConfig.has("sid"))
		{
			std::string sSid(vConfig["sid"]);
			bool fElevated = vConfig.get("isElevated", false);
			ScopedHandle hProcess(searchProcessBySid(sSid, fElevated));
			if (hProcess &&
				!::OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken) &&
				!::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
				error::win::WinApiError(SL, "Fail to query process token").log();
		}
		else
			error::InvalidArgument(SL).throwException();
	}


	virtual const HANDLE get() override
	{
		return hToken.get();
	}

	virtual TokenId getId() override
	{
		crypt::xxh::Hasher ctx;
		crypt::updateHash(ctx, getUserSid());
		crypt::updateHash(ctx, isElevated());
		return ctx.finalize();
	}

	virtual bool impersonate() override
	{
		return (hToken && ::ImpersonateLoggedOnUser(hToken));
//		ScopedHandle hImpToken;
// 		return (hToken && 
// 			::DuplicateToken(hToken, SecurityImpersonation, &hImpToken) &&
// 			::SetThreadToken(0, hImpToken));
	}

	virtual Variant serialize() override
	{
		return Dictionary({ 
			{"$$clsid", CLSID_Token},
			{"sid", getUserSid()},
			{"isElevated", isElevated()}
		});
	}
};

///
/// User Data Provider class.
///
class UserDataProvider : public ObjectBase<CLSID_UserDataProvider>, 
	public ICommandProcessor, 
	public IUserInformation,
	public ISessionInformation,
	public IService
{
private:
	// URL: https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/f992ad60-0fe4-4b87-9fed-beb478836861
	static const size_t c_nMaxBinSidSize = 68;
	// URL: https://msdn.microsoft.com/en-us/library/ff632068.aspx
	static const size_t c_nMaxStrSidSize = 164;

	std::recursive_mutex m_mtxUser;
	std::map<UserId, Variant> m_pIdMap;
	std::map<std::string, UserId> m_pSidMap;
	std::map<std::wstring, UserId> m_pUserMap;

	std::string m_sEndpointId;
	std::string m_sLocalSid;

	bool isLocalSid(const std::string& sSid);
	UserId generateId(const std::string& sSid);
	std::wstring getUserNameBySid(const std::string& sSid);
	std::string getSidByUserName(std::wstring sUserName);
	std::wstring getProfileDirBySid(const std::string& sSid);
	UserClass getUserClass(const std::string& sSid, const HANDLE hToken);

	Variant getUserInfoById(UserId sUserId);
	Variant getUserInfoBySid(const std::string& sSid);
	Variant getUserInfoByName(const std::wstring& sUserName);

protected:
	/// Constructor.
	UserDataProvider();

	/// Destructor.
	virtual ~UserDataProvider() override;

public:

	///
	/// Implements the object's final construction.
	///
	void finalConstruct(Variant vConfig);

	// IUserInformation

	/// @copydoc IUserInformation::getUserInfo()
	Variant getUserInfo(Variant vParams) override;
	/// @copydoc IUserInformation::getTokenInfo()
	Variant getTokenInfo(HANDLE hProcess, const std::string& sSid, const bool fElevated) override;
	/// @copydoc IUserInformation::getTokenInfo()
	Variant getTokenInfo(Variant vParams) override;

	// ISessionInformation

	/// @copydoc ISessionInformation::getSessionsInfo()
	Variant getSessionsInfo() override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IService

	/// @copydoc IService::loadState()
	void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	Variant saveState() override;
	/// @copydoc IService::start()
	void start() override;
	/// @copydoc IService::stop()
	void stop() override;
	/// @copydoc IService::shutdown()
	void shutdown() override;
};

///
/// The class for path transformation.
///
class PathConverter : public ObjectBase<CLSID_PathConverter>,
	public IPathConverter,
	//	public IService,
	public ICommandProcessor
{
private:
	std::mutex m_mtxMatcherMap;
	std::map<TokenId, ObjPtr<IStringMatcher> > m_pMatcherMap;

public:
	///
	/// Implements the object's final construction.
	///
	void finalConstruct(Variant vConfig);

	// IPathConverter

	/// @copydoc IPathConverter::getAbstractPath()
	std::wstring getAbstractPath(std::wstring sPath, Variant vSecurity) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IService interface
/*
	virtual void loadState(Variant vState) override;
	virtual Variant saveState() override;
	virtual void start() override;
	virtual void stop() override;
	virtual void shutdown() override;
*/
};

} // namespace win
} // namespace sys
} // namespace openEdr 

/// @}
