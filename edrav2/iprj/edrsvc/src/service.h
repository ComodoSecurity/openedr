//
// EDRAv2.edrsvc project
//
// Autor: Denis Bogdanov (04.02.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file Windows service class declaration
///
/// @addtogroup application Application
/// @{
#pragma once

namespace cmd {
namespace win {

inline ObjPtr<ICommandProcessor> startElevatedInstanceWithParams(const std::wstring& params)
{
	const std::wstring wPath = getCatalogData("app.imageFile");
	const std::wstring completeParams = std::wstring(L" ") + params;
	auto ec = sys::executeApplication(wPath, completeParams, true, 0);
	if (ec != 0)
		error::Exception(SL, ErrorCode(ec),
			"Can't run elevated instance of application").throwException();
	std::this_thread::sleep_for(std::chrono::seconds(1));

	return queryInterface<ICommandProcessor>(queryService("appRpcClient"));
}

//
//
//
inline ObjPtr<ICommandProcessor> startElevatedInstance(bool fStdOut)
{
	bool fNeedElevation = getCatalogData("app.config.needElevation", false);

	if (!fNeedElevation || sys::win::isProcessElevated(::GetCurrentProcess()))
		return queryInterface<ICommandProcessor>(queryService("application"));
	if (fStdOut)
		std::cout << "Seems we have no rights. Performing UAC elevation..." << std::endl;

	std::wstring wParams(L" server /l=");
	wParams += std::to_wstring((Size)getCatalogData("app.config.log.logLevel"));
	wParams += L" --timeout=60000 --logpath=";
	wParams += getCatalogData("app.logPath");
	wParams += L"\\server";

	return startElevatedInstanceWithParams(wParams);
}

//
//
//
inline void stopElevatedInstance(ObjPtr<ICommandProcessor> pProcessor, bool fStdOut)
{
	// Check if we have a remote server
	if (pProcessor == nullptr || pProcessor->getClassId() != CLSID_JsonRpcClient)
		return;
	
	if (fStdOut)
		std::cout << "Stopping elevated instance..." << std::endl;
	(void)execCommand(pProcessor, "execute", Dictionary({
		{ "processor", "objects.application" },
		{ "command", "shutdown" },
	}));
}

//
// Scan adapter class
//
class WinService : public Application
{
private:
	static constexpr DWORD c_nServicePreviousState = -1;
	static constexpr wchar_t c_sServiceName[] = L"edrsvc";

	SERVICE_STATUS m_svcStatus {};
	SERVICE_STATUS_HANDLE m_ssHandle = nullptr;
	std::exception_ptr m_pCurrException = nullptr;
	std::mutex m_mtxServiceProc;

#ifdef _DEBUG
	static constexpr bool c_fDefaultAllowUnload = true;
#else // _DEBUG
	static constexpr bool c_fDefaultAllowUnload = false;
#endif // _DEBUG
	bool m_fAllowUnload = c_fDefaultAllowUnload;

	void setAllowUnload(bool fAllow)
	{
		m_fAllowUnload = fAllow;
		if (m_fAllowUnload)
			setFlag(m_svcStatus.dwControlsAccepted, DWORD(SERVICE_ACCEPT_STOP));
		else
			resetFlag(m_svcStatus.dwControlsAccepted, DWORD(SERVICE_ACCEPT_STOP));
	}
	void allowUnload(bool fAllow);

	ErrorCode runService(bool fAppMode);
	void reportStatus(DWORD dwState = c_nServicePreviousState,
		DWORD dwWaitHint = -1, ErrorCode errCode = ErrorCode::OK);
	Variant update(Variant vParams);

	// 
	// Stops runned service
	//
	Variant stopService();


protected:
	virtual void initEnvironment(bool fUseCommonData);

	static void WINAPI runService(DWORD dwArgc, PTSTR* pszArgv);
	static ULONG WINAPI controlService(ULONG dwControl, ULONG dwEventType,
		PVOID pvEventData, PVOID pvContext);
	
	std::mutex& getLock()
	{
		return m_mtxServiceProc;
	}

	virtual void doSelfcheck() override;
	virtual ErrorCode process() override;
	virtual ErrorCode doShutdown(ErrorCode errCode) override;

	Variant install(Variant vParams);
	Variant uninstall(Variant vParams);

	Variant loadServiceData(const std::string& sName, Variant vDefault) const;
	std::optional<Variant> loadServiceData(const std::string& sName) const;
	void saveServiceData(const std::string& sName, Variant vData) const;
	void runInstance(const std::wstring& args);

public:
	WinService();
	virtual ~WinService() override;

	// ICommandProcessor
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace win
} // namespace cmd
/// @}
