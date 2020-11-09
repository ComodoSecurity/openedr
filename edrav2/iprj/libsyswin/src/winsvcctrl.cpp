//
// edrav2.libsyswin project
//
// Author: Yury Podpruzhnikov (04.01.2019)
// Reviewer: Denis Bogdanov (06.01.2019)
//
///
/// @file Windows service controller implemention
///
/// @addtogroup syswin System library (Windows)
/// @{
#include "pch.h"
#include "winsvcctrl.h"

namespace openEdr {
namespace sys {
namespace win {

#undef CMD_COMPONENT
#define CMD_COMPONENT "winsvcmgr"

//
// Get current status of service
//
DWORD getServiceStatus(ScopedServiceHandle& hService, const Variant& vServiceName)
{
	SERVICE_STATUS ServiceStatus;
	if (!::QueryServiceStatus(hService, &ServiceStatus))
		error::win::WinApiError(SL, FMT("Can't QueryServiceStatus " << vServiceName)).throwException();
	return ServiceStatus.dwCurrentState;
}

//
// Check service status until FnChecker return true or timeout is occurred
// @return True if state is occurred, False if timeout is occurred.
// @throw WinApiError if any error occurred
//
template<typename TFnChecker>
bool waitServiceState(ScopedServiceHandle& hService, const Variant& vServiceName,
	DWORD nTimeout_ms, TFnChecker FnChecker)
{
	DWORD nSleepTime = std::min<DWORD>(500, nTimeout_ms);

	DWORD dwStartTickCount = ::GetTickCount();
	while (true)
	{
		DWORD nCurState = getServiceStatus(hService, vServiceName);
		if (FnChecker(nCurState)) return true;

		if (::GetTickCount() - dwStartTickCount >= nTimeout_ms)
			return false;

		Sleep(nSleepTime);
	}
}

//
// Enable privilege to load driver for effective token
//
bool enableLoadDriverPrivelege()
{
// 	ATL::CAccessToken Token;
// 	if (!Token.GetEffectiveToken(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY))
// 		return false;
// 	if (!Token.EnablePrivilege(SE_LOAD_DRIVER_NAME))
// 		return false;

	ScopedHandle hToken;
	if (!::OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
	{
		if (GetLastError() != ERROR_NO_TOKEN ||
			!::ImpersonateSelf(SecurityImpersonation) ||
			!::OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
			return false;
	}
	if (!setPrivilege(hToken, SE_LOAD_DRIVER_NAME, true))
		return false;
	return true;
}

//
//
//
template<typename T>
std::optional<T> getOptionalParam(const Variant& vVal)
{
	if (vVal.isNull())
		return {};
	else
		return T(vVal);
}

//
//
//
ScopedServiceHandle openService(const Variant& vServiceName, 
	DWORD nDesiredAccess = SERVICE_ALL_ACCESS)
{
	// get SCManager
	ScopedServiceHandle hSCManager(::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
	if (hSCManager == nullptr)
		error::win::WinApiError(SL, "Can't open SCM").throwException();

	ScopedServiceHandle hService(::OpenService(hSCManager, std::wstring(vServiceName).c_str(), nDesiredAccess));
	if (hService == nullptr)
		error::win::WinApiError(SL, FMT("Can't open service " << vServiceName)).throwException();
	
	return hService;
}

//
// Remove key of service from registry
//
static void deleteServiceFromRegistry(const Variant& vServiceName)
{
	std::wstring sFullRegPath = L"SYSTEM\\CurrentControlSet\\services\\";
	sFullRegPath += std::wstring(vServiceName);
	LSTATUS eStatus = ::RegDeleteTreeW(HKEY_LOCAL_MACHINE, sFullRegPath.c_str());
	if (eStatus != ERROR_SUCCESS && eStatus != ERROR_FILE_NOT_FOUND)
		error::win::WinApiError(SL, FMT("Can't delete registry key of service " << vServiceName)).log();
}

//
// Convert sequence of strings to MULTI_SZ string
//
std::wstring convertSequenceToMultiSz(const Variant& vStrings)
{
	// Fill MULTI_SZ string
	std::wstring sResult;
	for (auto vItem : vStrings)
	{
		sResult += std::wstring(vItem);
		sResult += std::wstring{ L'\0' };
	}

	// Add finish zero. In case of sResult is empty, it needs to add 2 chars.
	sResult += std::wstring{ L'\0', L'\0' };
	return sResult;
}

//
// Wrapper of ChangeServiceConfig2W
//
void configureService(ScopedServiceHandle& hService, const Variant& vServiceName, Variant vParam)
{
	if (vParam.has("description"))
	{
		std::wstring sDescription = vParam["description"];
		SERVICE_DESCRIPTION sd {(LPWSTR) sDescription.c_str() };
		if (!::ChangeServiceConfig2W(hService, SERVICE_CONFIG_DESCRIPTION, &sd))
			error::win::WinApiError(SL, FMT("Can't config description of service <" 
				<< vServiceName << ">")).throwException();
	}

	if (vParam.has("restartIfCrash"))
	{
		// FIXME: Add bool-like restartIfCrash behavior
		Dictionary dictActionOnCrash(vParam["restartIfCrash"]);

		DWORD dwResetPeriod = dictActionOnCrash.get("resetPeriod", 30 * 60);
		Size nRestartCount = dictActionOnCrash.get("count", 3);
		DWORD nDelayBeforeRestart = dictActionOnCrash.get("delay", 5000);

		SC_ACTION action = { SC_ACTION_RESTART , nDelayBeforeRestart};
		std::vector<SC_ACTION> actions(nRestartCount, action);
		actions.push_back(SC_ACTION{ SC_ACTION_NONE , 0 });

		SERVICE_FAILURE_ACTIONS sfa = {};
		sfa.dwResetPeriod = dwResetPeriod;
		sfa.cActions = (DWORD)actions.size();
		sfa.lpsaActions = actions.data();

		if (!::ChangeServiceConfig2W(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa))
			error::win::WinApiError(SL, FMT("Can't config restartIfCrash of service <"
				<< vServiceName << ">")).throwException();
	}

	if (vParam.has("privileges"))
	{
		auto sPrivileges = convertSequenceToMultiSz(vParam["privileges"]);

		SERVICE_REQUIRED_PRIVILEGES_INFO ServPriv;
		ServPriv.pmszRequiredPrivileges = sPrivileges.data();
		if (!::ChangeServiceConfig2W(hService, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &ServPriv))
			error::win::WinApiError(SL, FMT("Can't set privileges of service <"
				<< vServiceName << ">")).throwException();
	}

	if (vParam.has("startMode"))
	{
		DWORD eStartMode = vParam["startMode"];

		auto fResult = ::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, eStartMode,
			SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		if (!fResult)
			error::win::WinApiError(SL, FMT("Can't set startMode of service <" 
				<< vServiceName << ">")).throwException();
	}
}

//
// 
//
void configureService(Variant vParam)
{
	Variant vServiceName = vParam["name"];
	ScopedServiceHandle hService = openService(vServiceName);
	configureService(hService, vServiceName, vParam);
}

//
//
//
void installService(Variant vParam)
{
	// Parse params
	Variant vServiceName = vParam["name"];
	std::wstring sServiceName = vServiceName;
	std::wstring sPath = vParam["path"];
	DWORD nType = vParam["type"];
	DWORD nStart = vParam["startMode"];
	DWORD nErrorControl = vParam["errorControl"];

	auto sDisplayName = getOptionalParam<std::wstring>(vParam.get("displayName", {}));
	auto sLoadOrderGroup = getOptionalParam<std::wstring>(vParam.get("group", {}));
	auto sServiceStartName = getOptionalParam<std::wstring>(vParam.get("account", {}));
	auto sPassword = getOptionalParam<std::wstring>(vParam.get("password", {}));
	auto nTagId = getOptionalParam<DWORD>(vParam.get("tag", {}));

	std::optional<std::wstring> sDependencies;
	if (vParam.has("dependencies")) 
		sDependencies = convertSequenceToMultiSz(vParam["dependencies"]);

	bool fReinstall = vParam.get("reinstall", false);

	// get SCManager
	ScopedServiceHandle hSCManager(::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
	if (hSCManager == nullptr)
		error::win::WinApiError(SL, "Can't open SCM").throwException();

	ScopedServiceHandle hService(::OpenService(hSCManager, sServiceName.c_str(), SERVICE_ALL_ACCESS));

	// Updating an exist service
	if (hService && !fReinstall)
	{
		LOGLVL(Detailed, "Update existing service <" << vServiceName << ">");
		BOOL fSuccess = ::ChangeServiceConfig(hService, nType, nStart, nErrorControl, sPath.c_str(),
			sLoadOrderGroup.has_value() ? sLoadOrderGroup->c_str() : nullptr,
			nTagId.has_value() ? &(nTagId.value()) : nullptr,
			sDependencies.has_value() ? sDependencies->data() : nullptr,
			sServiceStartName.has_value() ? sServiceStartName->c_str() : nullptr,
			sPassword.has_value() ? sPassword->c_str() : nullptr,
			sDisplayName.has_value() ? sDisplayName->c_str() : nullptr);
		if (!fSuccess)
		{
			// FIXME: Create error string using Error Helper Object !!!
			LOGWRN("Can't update service " << vServiceName << "(Error: "
				<< hex(::GetLastError()) << "). Try reinstall it.");
			fReinstall = true;
		}
	}

	// try install or reinstall
	if (!hService || fReinstall)
	{
		size_t nRetryCnt = fReinstall ? 2 : 0;

		if (hService)
		{
			LOGLVL(Detailed, "Reinstall service <" << vServiceName << ">");
			if (!::DeleteService(hService))
				error::win::WinApiError(SL, "Can't delete old instance of service").throwException();
			deleteServiceFromRegistry(vServiceName);
			hService.reset();
		}
		else
		{
			LOGLVL(Detailed, "Install service <" << vServiceName << ">");
		}

		do 
		{
			hService.reset(::CreateServiceW(hSCManager, sServiceName.c_str(),
				sDisplayName.has_value() ? sDisplayName->c_str() : nullptr,
				SERVICE_ALL_ACCESS, nType, nStart, nErrorControl, sPath.c_str(),
				sLoadOrderGroup.has_value() ? sLoadOrderGroup->c_str() : nullptr,
				nTagId.has_value() ? &(nTagId.value()) : nullptr,
				sDependencies.has_value() ? sDependencies->data() : nullptr,
				sServiceStartName.has_value() ? sServiceStartName->c_str() : nullptr,
				sPassword.has_value() ? sPassword->c_str() : nullptr));

			if (!hService && nRetryCnt)
			{
				LOGWRN("Can't create service <" << vServiceName << ">. Retry after 1s");
				Sleep(1000);
			}
		} while (!hService && nRetryCnt--);

		if (!hService)
			error::win::WinApiError(SL, FMT("Can't create service <" << vServiceName << ">")).throwException();
	}

	// Set additional fields
	configureService(hService, vServiceName, vParam);
}

//
// 
//
Variant startService(Variant vParam)
{
	Variant vServiceName = vParam["name"];
	DWORD nTimeout = vParam.get("timeout", 60000);

	auto nStartTime = ::GetTickCount();
	ScopedServiceHandle hService = openService(vServiceName);

	// Checking that service already started
	DWORD nCurState = getServiceStatus(hService, vServiceName);
	if (nCurState == SERVICE_RUNNING)
		return false;

	// Wait stop
	if (nCurState == SERVICE_STOP_PENDING)
	{
		auto nTime = ::GetTickCount() - nStartTime;
		(void)waitServiceState(hService, vServiceName, nTimeout > nTime ? nTimeout - nTime : 0,
			[](DWORD CurState) {return CurState != SERVICE_STOP_PENDING; });
	}

	(void) enableLoadDriverPrivelege();
	if (!::StartService(hService, 0, NULL))
	{
		DWORD eError = ::GetLastError();
		if (eError == ERROR_SERVICE_ALREADY_RUNNING)
			return false;
		if (eError == ERROR_SERVICE_DISABLED && vParam.has("startMode"))
		{
			LOGLVL(Detailed, "Starting disabled service <" << vServiceName << ">");
			BOOL fSuccess = ::ChangeServiceConfig(hService, SERVICE_NO_CHANGE, vParam["startMode"],
				SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
			if (!fSuccess)
				error::win::WinApiError(SL, ::GetLastError(), "Can't change service startup mode").throwException();
			if (!::StartService(hService, 0, NULL))
			{
				if (eError == ERROR_SERVICE_ALREADY_RUNNING)
					return false;
				error::win::WinApiError(SL, eError, FMT("Can't start service <" << vServiceName << ">")).throwException();
			}
		}
		else
			error::win::WinApiError(SL, eError, FMT("Can't start service <" << vServiceName << ">")).throwException();
	}

	auto nTime = ::GetTickCount() - nStartTime;
	if (waitServiceState(hService, vServiceName, nTimeout > nTime ? nTimeout - nTime : 0,
		[](DWORD CurState) {return CurState == SERVICE_RUNNING; }))
		return true;

	error::LimitExceeded(SL, FMT("Can't start service <" << vServiceName << ">. Timeout is occured.")).throwException();
}


//
// 
//
Variant stopService(Variant vParam)
{
	Variant vServiceName = vParam["name"];
	DWORD nTimeout = vParam.get("timeout", 60000);
	ScopedServiceHandle hService = openService(vServiceName);

	// Checking that service already stopped
	DWORD eServiceStatus = getServiceStatus(hService, vServiceName);
	if (eServiceStatus == SERVICE_STOPPED)
		return false;

	// Wait until stopped
	if (eServiceStatus == SERVICE_STOP_PENDING)
	{
		(void)waitServiceState(hService, vServiceName, nTimeout,
			[](DWORD CurState) {return CurState != SERVICE_STOP_PENDING; });

		if (SERVICE_STOPPED == getServiceStatus(hService, vServiceName))
			return true;
	}

	(void)enableLoadDriverPrivelege();
	SERVICE_STATUS ServiceStatus;
	if (!::ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus)) 
		error::win::WinApiError(SL, FMT("Can't stop service <" 
			<< vServiceName << ">")).throwException();

	if (waitServiceState(hService, vServiceName, nTimeout, [](DWORD CurState) {return CurState == SERVICE_STOPPED; }))
		return true;

	error::LimitExceeded(SL, FMT("Can't stop service <" << vServiceName << ">. Timeout is occured.")).throwException();
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * TBD
///
/// #### Supported commands
///
Variant WinServiceController::execute(Variant vCommand, Variant vParams)
{
	Variant vNameForTrace = vParams.get("name", "#undefined");

	CMD_TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << "> for service <" << vNameForTrace << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### create()
	/// Installs a service.
	///
	/// Wrapper of WinAPI function CreateService.  
	/// Field description and supported values are described in MSDN.
	///
	///   * **name** [str] - lpServiceName.
	///   * **path** [str] - lpBinaryPathName.
	///   * **type** [int] - dwServiceType.
	///   * **startMode** [int] - dwStartType.
	///   * **errorControl** [int] - dwErrorControl.
	///   * **displayName** [str, opt] - lpDisplayName.
	///   * **group** [str, opt] - lpLoadOrderGroup.
	///   * **dependencies** [seq, opt] - sequence of strings. lpDependencies.
	///   * **account** [str, opt] - lpServiceStartName.
	///   * **password** [str, opt] - lpPassword.
	///   * **tag** [int, opt] - lpdwTagId.
	///   * **reinstall** [bool, opt] - reinstall if exist. If it is false or absent action,
	///	    update the exist service. 
	///
	/// Also support parameters of the `change` command.
	///
	if (vCommand == "create")
	{
		installService(vParams);
		return {};
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### delete()
	/// Uninstalls a service.
	///
	/// Field description and supported values are described in MSDN.
	///
	///   * **name** [str] - lpServiceName.
	///
	if (vCommand == "delete")
	{
		Variant vServiceName = vParams["name"];
		ScopedServiceHandle hService = openService(vServiceName);

		if (!::DeleteService(hService))
			error::win::WinApiError(SL, FMT("Can't delete service <" 
				<< vServiceName << ">")).throwException();
		deleteServiceFromRegistry(vServiceName);
		return {};
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### change()
	/// Changes the service's configuration.
	///
	/// Wrapper of WinAPI function ChangeServiceConfig.  
	/// Field description and supported values are described in MSDN.
	/// If field is absent, it is not changed.
	///
	///   * **name** [str] - lpServiceName.
	///   * **description** [str, opt] - a human-readable description. 
	///     See https://docs.microsoft.com/en-us/windows/desktop/api/winsvc/ns-winsvc-_service_descriptiona
	///   * **startMode** [int, opt] - one of SERVICE_*_START constant. See 
	///     the dwStartType parameter of CreateService in MSDN.
	///   * **privileges** [seq, opt] - a sequence of strings.
	///     See https://docs.microsoft.com/en-us/windows/desktop/api/winsvc/ns-winsvc-_service_required_privileges_infoa
	///   * **restartIfCrash** [dict, opt] - tuning of crash restart.
	///     See https://docs.microsoft.com/en-us/windows/desktop/api/winsvc/ns-winsvc-_service_failure_actionsa
	///   * **resetPeriod** [int, opt] - reset period in seconds (default: 1800).
	///     See SERVICE_FAILURE_ACTIONS::dwResetPeriod. 
	///   * **count** [int, opt] - count of restart attempts (default: 3).
	///   * **delay** [int, opt] - delay before a restart in milliseconds (default: 5000).
	///     See SC_ACTION::Delay. 
	//
	if (vCommand == "change")
	{
		configureService(vParams);
		return {};
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	/// Parameters:
	///   * **name** [str] - lpServiceName.
	///   * **timeout** [int, opt] - wait in milliseconds (while stopping) (default: 60000).
	///   * **startMode** [int, opt] - if service is disabled and this parameter 
	///     is present then the start mode is changed according to this 
	///     parameter - one of SERVICE_*_START constant. 
	///     See the dwStartType parameter of CreateService in MSDN.
	///
	if (vCommand == "start")
	{
		return startService(vParams);
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	/// Parameters:
	///   * **name** [str] - lpServiceName.
	///   * **timeout** [int, opt] - wait in milliseconds (while stopping) (default: 60000).
	///
	/// If action can't be done, throws appropriate exception.
	/// Waits until the service stopping and returns true. If timeout is occurred, returns false.
	/// 
	if (vCommand == "stop")
	{
		return stopService(vParams);
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### waitState()
	/// Waits until the service's state equals to the specified.
	///
	/// Parameters:
	///   * **name** [str] - lpServiceName.
	///   * **state** [int] - service state. One of SERVICE_*: SERVICE_STOPPED, SERVICE_RUNNING etc.
	///   * **timeout** [int, opt] - wait in milliseconds (while stopping) (default: 60000).
	///
	/// If timeout is occurred, throws LimitExceeded.
	///
	if (vCommand == "waitState")
	{
		Variant vServiceName = vParams["name"];
		ScopedServiceHandle hService = openService(vServiceName);
		DWORD eState = vParams["state"];
		DWORD nTimeout = vParams.get("timeout", 60000);

		if(!waitServiceState(hService, vServiceName, nTimeout,
			[&eState](DWORD CurState) {return CurState == eState; }))
			error::LimitExceeded(SL, FMT("Timeout is occurred during wait status of service <"
				<< vServiceName << ">")).throwException();
		return {};
	}

	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### isExist()
	/// Checks a service's existence.
	///
	/// Parameters:
	///   * **name** [str] - lpServiceName.
	///
	/// Returns: true or false. Throws exception if any errors is occurred.
	///
	if (vCommand == "isExist")
	{
		Variant vServiceName = vParams["name"];

		// get SCManager
		ScopedServiceHandle hSCManager(::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS));
		if (hSCManager == nullptr)
			error::win::WinApiError(SL, "Can't open SCM").throwException();

		ScopedServiceHandle hService(::OpenService(hSCManager, std::wstring(vServiceName).c_str(), SERVICE_QUERY_STATUS));
		if (hService)
			return true;
		if (::GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
			return false;

		error::win::WinApiError(SL, FMT("Can't check the existence of service <"
			<< vServiceName << ">")).throwException();
	}


	///
	/// @fn Variant UserDataProvider::execute()
	///
	/// ##### queryStatus()
	/// Checks a service's existence.
	///
	/// Wrapper of WinAPI `QueryServiceStatusEx` function.  
	///
	/// Parameters:
	///   * **name** [str] - a name of the service.
	///
	/// Returns a dictionary with content of the SERVICE_STATUS_PROCESS structure.
	/// Name of fields in the dict the same as name of fields of the structure.
	/// Return dictionary with fields (and an appropriate field of the structure SERVICE_STATUS_PROCESS):
	///   * **type** [int] - dwServiceType.
	///   * **state** [int] - dwCurrentState.
	///   * **controlsAccepted** [int] - dwControlsAccepted.
	///   * **win32ExitCode** [int] - dwWin32ExitCode.
	///   * **specificExitCode** [int] - dwServiceSpecificExitCode.
	///   * **checkPoint** [int] - dwCheckPoint.
	///   * **waitHint** [int] - dwWaitHint.
	///   * **processId** [int] - dwProcessId.
	///   * **flags** [int] - dwServiceFlags.
	///  
	if (vCommand == "queryStatus")
	{
		Variant vServiceName = vParams["name"];
		ScopedServiceHandle hService = openService(vServiceName, SERVICE_QUERY_STATUS);

		SERVICE_STATUS_PROCESS servStatus = {};
		DWORD cbBytesNeeded = 0;
		if (!::QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO,
			(LPBYTE)&servStatus, sizeof(servStatus), &cbBytesNeeded))
				error::win::WinApiError(SL, FMT("Can't query status for service <"
					<< vServiceName << ">")).throwException();

		return Dictionary({
			{"type", servStatus.dwServiceType},
			{"state", servStatus.dwCurrentState},
			{"controlsAccepted", servStatus.dwControlsAccepted},
			{"win32ExitCode", servStatus.dwWin32ExitCode},
			{"specificExitCode", servStatus.dwServiceSpecificExitCode},
			{"checkPoint", servStatus.dwCheckPoint},
			{"waitHint", servStatus.dwWaitHint},
			{"processId", servStatus.dwProcessId},
			{"flags", servStatus.dwServiceFlags},
		});
	}

	CMD_TRACE_END( FMT("Error during execution of a command <" << vCommand
		<< "> for service <" << vNameForTrace << ">"));

	error::InvalidArgument(SL, FMT("WinServiceController doesn't support a command <" 
		<< vCommand << ">")).throwException();
}

} // namespace win
} // namespace sys
} // namespace openEdr 

  /// @}
