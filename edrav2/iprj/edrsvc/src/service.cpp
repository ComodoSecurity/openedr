//
// edrav2.edrsvc project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file Windows service class implementation
///
/// @addtogroup application Application
/// @{

#include "pch.h"
#include "service.h"

namespace openEdr {
namespace win {

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "winsvc"

//
// Constructor
//
WinService::WinService()
{
	m_svcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_svcStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN;
	setAllowUnload(c_fDefaultAllowUnload);
}

//
// Destructor
//
WinService::~WinService()
{
}

//
//
//
void WinService::reportStatus(DWORD dwState, DWORD dwWaitHint, ErrorCode errCode)
{
	if (m_ssHandle == NULL) return;
	if (dwState == c_nServicePreviousState ||
		dwState == m_svcStatus.dwCurrentState)
	{
		// We just update status
		if (dwState == SERVICE_START_PENDING ||
			dwState == SERVICE_STOP_PENDING ||
			dwState == SERVICE_PAUSE_PENDING ||
			dwState == SERVICE_CONTINUE_PENDING)
			m_svcStatus.dwCheckPoint++;
	}
	else
	{
		// We set a new status
		m_svcStatus.dwCurrentState = dwState;
		m_svcStatus.dwCheckPoint = 0;
		m_svcStatus.dwWin32ExitCode = NO_ERROR;
		m_svcStatus.dwServiceSpecificExitCode = 0;
		m_svcStatus.dwWaitHint = 0;
	}

	if (isError(errCode))
	{
		m_svcStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
		m_svcStatus.dwServiceSpecificExitCode = DWORD(errCode);
	}

	if (dwWaitHint != -1)
		m_svcStatus.dwWaitHint = dwWaitHint;
	
	if (!::SetServiceStatus(m_ssHandle, &m_svcStatus))
		error::win::WinApiError(SL, "Can't set service state").log();

	if (dwState == SERVICE_STOPPED)
		m_ssHandle = nullptr;
}

//
//
//
void WinService::allowUnload(bool fAllow)
{
	if (fAllow == m_fAllowUnload)
		return;
	setAllowUnload(fAllow);

	if (m_ssHandle == NULL)
		error::RuntimeError(SL, "Invalid service state").throwException();

	if (!::SetServiceStatus(m_ssHandle, &m_svcStatus))
		error::win::WinApiError(SL, "Can't set service state").throwException();
}

//
// Service main function is called by Windows Service Manager
//
void WINAPI WinService::runService(DWORD dwArgc, PTSTR* pszArgv)
{
	auto vApp = getCatalogData("objects.application", nullptr);
	auto pSvcApp = queryInterface<WinService>(vApp);
	if (!pSvcApp)
		error::RuntimeError(SL, "Can't access to the application instance").throwException();
	std::scoped_lock lock(pSvcApp->getLock());

	try
	{
		pSvcApp->runService(false);
	}
	catch (error::Exception& ex)
	{
		if (pSvcApp)
			pSvcApp->m_pCurrException = std::current_exception();
		ex.log(SL, "Unhandled exception in the service thread");
	}
	catch (...)
	{
		auto sExStr = "Unhandled exception in the service thread";
		if (pSvcApp)
			pSvcApp->m_pCurrException = 
				std::make_exception_ptr(error::RuntimeError(SL, sExStr));
		error::RuntimeError(SL, sExStr).log();
	}
}

//
// Service control handler. It is called by Windows Service Manager
//
ULONG WINAPI WinService::controlService(ULONG dwControl, ULONG dwEventType,
	PVOID pvEventData, PVOID pvContext)
{
	LOGLVL(Detailed, "Service received request (" <<
		"dwControl=" << std::hex << dwControl <<
		", dwEventType=" << std::hex << dwEventType << ")");

	WinService* pThis = (WinService*)pvContext;
	if (!pThis) return NO_ERROR;

	switch (dwControl)
	{
		case SERVICE_CONTROL_SHUTDOWN:
		case SERVICE_CONTROL_STOP:
		{	
			pThis->reportStatus(SERVICE_STOP_PENDING);
			pThis->shutdown();
			return NO_ERROR;
		}
		case SERVICE_CONTROL_INTERROGATE:
		{
			pThis->reportStatus();
			return NO_ERROR;
		}
	}

	return ERROR_CALL_NOT_IMPLEMENTED;
}

//
//
//
void WinService::initEnvironment(bool fUseCommonData)
{
	std::string sInstallationId = FMT("45CC556C-A03B-42FF-A2FE-" << std::setw(12) <<
		std::setfill('0') << CMD_VERSION_BUILD);
	putCatalogData("app.installationId", sInstallationId);

	return Application::initEnvironment(true);
}

//
//
//
void WinService::doSelfcheck()
{
	Application::doSelfcheck();
	
	//
	// Restore config credential info
	//

	// Restore specified machine id
	std::string sMachineId = getCatalogData("app.config.extern.endpoint.license.machineId", "");
	if (sMachineId.empty())
	{
		std::string sSavedMachineId = loadServiceData("machineId", "");
		if (!sSavedMachineId.empty())
			putCatalogData("app.config.extern.endpoint.license.machineId", sSavedMachineId);
	}

	// Restore directly specified customerId/endpointId
	std::string sCurrCustomerId = getCatalogData("app.config.extern.endpoint.license.customerId", "");
	std::string sCurrEndpointId = getCatalogData("app.config.extern.endpoint.license.endpointId", "");
	if (!sCurrCustomerId.empty() && !sCurrEndpointId.empty())
		return;

	putCatalogData("app.config.extern.endpoint.license.customerId", "");
	putCatalogData("app.config.extern.endpoint.license.endpointId", "");

	std::string sSavedCustomerId = loadServiceData("customerId", "");
	std::string sSavedEndpointId = loadServiceData("endpointId", ""); 
	std::string sSavedClientId = loadServiceData("clientId", "");
	if (!sSavedEndpointId.empty())
	{
		if (!sSavedCustomerId.empty())
		{
			putCatalogData("app.config.extern.endpoint.license.customerId", sSavedCustomerId);
			putCatalogData("app.config.extern.endpoint.license.endpointId", sSavedEndpointId);
			return;
		}
		else if (!sSavedClientId.empty())
		{
			putCatalogData("app.config.extern.endpoint.license.clientId", sSavedClientId);
			putCatalogData("app.config.extern.endpoint.license.endpointId", sSavedEndpointId);
			return;
		}	
	}

	// Restore using token
	std::string sCurrToken = getCatalogData("app.config.extern.endpoint.license.token", "");
	if (sCurrToken.empty())
	{
		std::string sSavedToken = loadServiceData("token", "");
		if (!sSavedToken.empty())
		{
			putCatalogData("app.config.extern.endpoint.license.token", sSavedToken);
			return;
		}
	}

	// Restore using legacyCustomerId
	std::string sCurrLegacyCustomerId = getCatalogData("app.config.extern.endpoint.license.legacyCustomerId", "");
	if (sCurrLegacyCustomerId.empty())
	{
		std::string sSavedLegacyCustomerId = loadServiceData("legacyCustomerId", "");
		if (!sSavedLegacyCustomerId.empty())
			putCatalogData("app.config.extern.endpoint.license.legacyCustomerId", sSavedLegacyCustomerId);
	}
}

//
//
//
ErrorCode WinService::process()
{
	if (!getCatalogData("app.config.isService", false))
	{
		LOGINF("Service is being started as an Application");
		return runService(true);
	}

	// Set unloading state
	// If value in config is null or absent, use default
	Variant vAllowUnload = getCatalogData("app.config.state.service.allowUnload", Variant());
	if (!vAllowUnload.isNull())
		setAllowUnload(vAllowUnload);

	LOGINF("Service is being started as a Service");
	wchar_t sName[std::size(c_sServiceName)];
	wcscpy_s(sName, c_sServiceName);
	SERVICE_TABLE_ENTRY ServiceTable[] =
	{
		{sName, runService},
		{NULL, NULL}   // End of list
	};

	if (!::StartServiceCtrlDispatcher(ServiceTable))
		error::win::WinApiError(SL, "Can't start service dispatcher").throwException();

	LOGINF("Service thread is finished");

	std::scoped_lock lock(m_mtxServiceProc);
	if (m_pCurrException)
		std::rethrow_exception(m_pCurrException);

	return ErrorCode::OK;
}

//
// Start service 
//
ErrorCode WinService::runService(bool fAppMode)
{
	LOGINF("Service is started");

	// Register service handler proc
	if (!fAppMode)
	{
		m_ssHandle = RegisterServiceCtrlHandlerEx(c_sServiceName, controlService, this);
		if (m_ssHandle == NULL)
			error::win::WinApiError(SL, "Can't register service handler").throwException();
	}

	// Perform advanced startup operations
	reportStatus(SERVICE_START_PENDING, 2000);
	ErrorCode ec = ErrorCode::NoAction;
	if (doStartup())
	{
		ec = ErrorCode::OK;
		// Run working mode handler
		if (!needShutdown())
		{
			reportStatus(SERVICE_RUNNING);
			ec = runMode();
		}
	}

	// Runs the mode handler
	LOGINF("Service is being stopped");
	return doShutdown(ec);
}

//
//
//
ErrorCode WinService::doShutdown(ErrorCode errCode)
{
	reportStatus(SERVICE_STOP_PENDING, 10000);
	ErrorCode ec = Application::doShutdown(errCode);
	reportStatus(SERVICE_STOPPED, 0, errCode);
	LOGINF("Service is stopped");
	m_ssHandle = NULL;
	return ec;
}

//
//
//
Variant WinService::install(Variant vParams)
{
	bool fReinstall = execCommand(Dictionary({
		{"processor", Dictionary({{"clsid", CLSID_WinServiceController}}) },
		{"command", "isExist"},
		{"params", Dictionary({
			{"name", c_sServiceName},
		})},
	}));

	if (fReinstall)
	{
		(void)execCommand(createObject(CLSID_WinServiceController), "stop",
			Dictionary({ { "name", c_sServiceName } }));
	}

	std::string sCmdLine(getCatalogData("app.imageFile"));
	if (vParams.has("params"))
		sCmdLine += " " + std::string(vParams.get("params", ""));
	
	std::string sStartMode = vParams.get("startMode", "off");
	Size nStartMode = (sStartMode == "auto") ? SERVICE_AUTO_START :
		((sStartMode == "manual") ? SERVICE_DEMAND_START : SERVICE_DISABLED);

	return execCommand(createObject(CLSID_WinServiceController), "create",
		Dictionary({
			{ "name", c_sServiceName },
			{ "path", sCmdLine },
			{ "type", SERVICE_WIN32_OWN_PROCESS },
			{ "startMode", nStartMode},
			{ "errorControl", vParams.get("errorControl", SERVICE_ERROR_NORMAL) },  
			{ "description", "Protects system against malicious activity and data leaks" },
			{ "displayName", "OpenEDR Service" },
			{ "restartIfCrash", Dictionary() },
			{ "reinstall", fReinstall}
		})
	);
}

//
//
//
Variant WinService::uninstall(Variant vParams)
{
	auto vResult = execCommand(createObject(CLSID_WinServiceController), "isExist",
		Dictionary({ { "name", c_sServiceName } }));

	if (!vResult) return false;

	try
	{
		(void)execCommand(createObject(CLSID_WinServiceController), "stop",
			Dictionary({ { "name", c_sServiceName } }));
	}
	catch (error::Exception& ex)
	{
		ex.log(SL, "Can't stop service <edrsvc> before uninstall, trying uninstall without stopping");
	}
	catch (...)
	{
		error::RuntimeError(SL, "Unhandled exception in stopping service <edrsvc>, trying uninstall without stopping").log();
	}

	return execCommand(createObject(CLSID_WinServiceController), "delete",
		Dictionary({ { "name", c_sServiceName } }));
}

//
//
//
Variant WinService::update(Variant vParams)
{
	std::string sVersion = vParams.get("version", getCatalogData("app.update.version", "0.0.0.0"));
	std::string sUrl = vParams.get("url", getCatalogData("app.update.url", ""));
	if (sVersion.empty() || sUrl.empty())
	{
		LOGLVL(Detailed, "No updates were found");
		return false;
	}

	std::string sFileName = "edragent-" + sVersion + ".msi";
	std::filesystem::path updatePath = getCatalogData("app.dataPath").convertToPath() / "updates";
	std::filesystem::path filePath = updatePath / sFileName;

	//net::HttpClient hc("", Dictionary({ {"maxResponseSize", 50 * 1024 * 1024} }));
	//LOGLVL(Detailed, "Checking metadata of update file <" << sUrl << ">");
	//auto vFileData = hc.head(sUrl);
	//io::IoSize nFileSize = vFileData.get("contentLength", 0);

	std::string sChecksum = vParams.get("checksum", getCatalogData("app.update.checksum", ""));
	bool fSkipChecksumCheck = sChecksum.empty();

	LOGLVL(Detailed, "Looking for update file for version <" << sVersion << ">");

	bool fUpdateFileReady = false;
	if (std::filesystem::exists(filePath))
	{
		if (fSkipChecksumCheck)
			fUpdateFileReady = true;
		else
		{
			// Check the checksum of existing file
			auto pStream = queryInterface<io::IRawReadableStream>(io::createFileStream(filePath, io::FileMode::Read));
			auto hash = crypt::md5::getHash(pStream);
			auto sNewChecksum = string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
			fUpdateFileReady = (sNewChecksum == string::convertToLower((std::string_view)sChecksum));
		}
	}

	if (!fUpdateFileReady)
	{
		// Clean cache
		if (std::filesystem::exists(updatePath))
		{
			LOGLVL(Detailed, "Cleaning update cache <" << updatePath << ">");
			std::error_code ec;
			auto nDeleted = std::filesystem::remove_all(updatePath, ec);
			LOGLVL(Detailed, "Deleted <" << nDeleted - 1 << "> cached files");
		}

		// Download data
		{
			LOGLVL(Detailed, "Downloading update file <" << sUrl << ">");
			auto pStream = queryInterface<io::IWritableStream>(io::createFileStream(filePath,
				io::FileMode::Read | io::FileMode::Write | io::FileMode::Truncate | io::FileMode::CreatePath));
			net::HttpClient hc("", Dictionary({ {"maxResponseSize", 50 * 1024 * 1024} }));
			(void)hc.get(sUrl, pStream);
		}

		//io::IoSize nNewSize = std::filesystem::file_size(filePath);
		//if (nNewSize != nFileSize)
		//	error::InvalidFormat(SL, FMT("Invalid size of update file <" <<
		//		nNewSize << " != " << nFileSize << ">")).throwException();

		if (!fSkipChecksumCheck)
		{
			// Check the checksum of downloaded file
			auto pStream = queryInterface<io::IRawReadableStream>(io::createFileStream(filePath, io::FileMode::Read));
			auto hash = crypt::md5::getHash(pStream);
			auto sNewChecksum = string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
			if (sNewChecksum != string::convertToLower((std::string_view)sChecksum))
				error::InvalidFormat(SL, FMT("Wrong file checksum <" <<
					sNewChecksum << " != " << sChecksum << ">")).throwException();
		}
	}

	// Run installation in update mode
	// msiexec /q /i update.msi UPDATE=1
	std::wstring sParams = L"/q /i " + filePath.wstring();
	sParams += L" UPDATE=1";
	LOGLVL(Detailed, "Executing installer <" << string::convertWCharToUtf8(sParams) << ">");
	auto ec = sys::executeApplication("msiexec", sParams, false, 0);

	return true;
}

//
//
//
Variant WinService::stopService()
{
	

	// Check that service is run and try simply to send "stop"
	CMD_TRY
	{
		return execCommand(createObject(CLSID_WinServiceController), "stop",
			Dictionary({ { "name", c_sServiceName } }));
	}
	CMD_PREPARE_CATCH
	catch (error::win::WinApiError& ex)
	{
		// Skip exception caused by self protection
		if (ex.getWinErrorCode() != ERROR_INVALID_SERVICE_CONTROL)
			throw;
	}

	// Send shutdown command
	auto pServiceRpcClient = queryInterface<ICommandProcessor>(getCatalogData("objects.svcControlRpcClient"));
	(void)execCommand(pServiceRpcClient, "shutdown");
	// Wait Stopped state
	(void)execCommand(createObject(CLSID_WinServiceController), "waitState",
		Dictionary({ { "name", c_sServiceName }, { "state", SERVICE_STOPPED} }));
	return true;
}

//
//
//
void WinService::saveServiceData(const std::string& sName, Variant vData) const
{
	sys::win::WriteRegistryValue(HKEY_LOCAL_MACHINE, L"Software\\OpenEdrAgent",
		std::wstring(sName.begin(), sName.end()), vData);

}

//
//
//
std::optional<Variant> WinService::loadServiceData(const std::string& sName) const
{
	try
	{
		return sys::win::ReadRegistryValue(HKEY_LOCAL_MACHINE,
			std::wstring(L"Software\\OpenEdrAgent"), std::wstring(sName.begin(), sName.end()));
	}
	catch (...)
	{
		return std::nullopt;
	}
}

//
//
//
Variant WinService::loadServiceData(const std::string& sName, Variant vDefault) const
{
	try
	{
		return sys::win::ReadRegistryValue(HKEY_LOCAL_MACHINE,
			std::wstring(L"Software\\OpenEdrAgent"), std::wstring(sName.begin(), sName.end()));
	}
	catch (...)
	{
		return vDefault;
	}
}

///
/// @copydoc Application::execute() 
///
/// #### Supported commands
///
Variant WinService::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### install()
	/// Installs service to the system
	///   * errorControl [in, opt] - error control mode (default: SERVICE_ERROR_NORMAL)
	///   * startMode [string,opt] - the following string modes are supported:
	///	    * "manual" - manual start
	///     * "auto" - automatic start (at OS start)
	///     * "off" - disable start
	///	  * params  [in, opt] - command line parameters for service
	///
	if (vCommand == "install")
		return install(vParams);

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### isInstalled()
	/// Checks if the service is installed. Returns true if yes.
	///
	else if (vCommand == "isInstalled")
	{
		return execCommand(createObject(CLSID_WinServiceController), "isExist",
			Dictionary({ { "name", c_sServiceName } }));
	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### waitState()
	/// Waits specified Status. Wrapper over WinServiceController.waitState().
	///  * name [str] - service name
	///  * state [int] - target service state. One of SERVICE_STOPPED, SERVICE_RUNNING etc.
	///  * timeout [int, opt] - wait in millisecond (while stopping). Default 60000.
	///  If timeout is occurred, throws LimitExceeded.
	///
	else if (vCommand == "waitState")
	{
		return execCommand(createObject(CLSID_WinServiceController), "waitState",
			Dictionary({
				{ "name", c_sServiceName },
				{ "state", vParams["state"]},
				{ "timeout", vParams.get("timeout", 60000)}
				}));
	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### uninstall()
	/// Uninstalls service from the system
	///
	else if (vCommand == "uninstall")
		return uninstall(vParams);

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### start()
	/// Starts service
	///
	else if (vCommand == "start")
	{
		std::string sStartMode = getCatalogData("app.config.serviceStartMode", "auto");
		Size nStartMode = (sStartMode == "auto") ? SERVICE_AUTO_START :
			((sStartMode == "manual") ? SERVICE_DEMAND_START : SERVICE_DISABLED);

		return execCommand(createObject(CLSID_WinServiceController), "start", Dictionary({ 
			{ "name", c_sServiceName }, 
			{ "startMode",  nStartMode},
		}));
	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### stop()
	/// Stops service
	///
	else if (vCommand == "stop")
	{
		return stopService();
	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### allowUnload()
	/// Disable/enable services stopping
	///  * value [bool] - value to set;
	///
	else if (vCommand == "allowUnload")
	{
		allowUnload( vParams["value"] );
		return {};
	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### update()
	/// Update service components
	///  * url [str,opt] - download ur;
	///  * version [str,opt] - new version
	///
	else if (vCommand == "update")
		return update(vParams);

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### getServiceInfo()
	/// Get information about service (for format result see queryStatus() of WinServiceController)
	///
	else if (vCommand == "getServiceInfo")
	{
		return execCommand(createObject(CLSID_WinServiceController), "queryStatus",
			Dictionary({ { "name", c_sServiceName } }));

	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### setServiceData()
	/// Set service-specific data
	///   * name - name of data 
	///   * data - data (string, integer)
	///
	else if (vCommand == "setServiceData")
	{
		saveServiceData(vParams["name"], vParams["data"]);
		return true;
	}

	return Application::execute(vCommand, vParams);

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace win
} // namespace openEdr
/// @}
