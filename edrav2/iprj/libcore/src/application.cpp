//
// edrav2.libcore project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (18.01.2019)
//
///
/// @file Application class implementation
///
/// @addtogroup application Application
/// @{
#include "pch_win.h"
#ifndef PLATFORM_WIN
#error "Please include different platform-specific PCH"
// #include "pch.h"
#endif
#include <application.hpp>
#include <utilities.hpp>
#include <system.hpp>
#include <io.hpp>
#include <dump.hpp>
#include <objects.h>
#include <message.hpp>
#include <task.hpp>
#include <events.hpp>
//#include <memory.hpp>
#include <version.h>

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "app"

namespace openEdr {

namespace fs = std::filesystem;

//
//
//
void Application::addMode(std::string sName, std::shared_ptr<IApplicationMode> pMode)
{
	m_appModes[std::move(sName)] = pMode;
}

//
//
//
int Application::run(std::string_view sName, std::string_view sFullName, 
	int argc, const wchar_t* const argv[])
{
	std::vector<const char*> argvU8;
	std::vector<std::string> sData;
	try
	{
		for (int i = 0; i < argc; ++i)
			sData.push_back(string::convertWCharToUtf8(argv[i]));
		for (int i = 0; i < argc; ++i)
			argvU8.push_back(sData[i].c_str());
	}
	catch (...)
	{
		std::cerr << "Unhandled exception in application (bad CL parameters)";
		return (int)ErrorCode::SystemError;
	}
	return run(sName, sFullName, argc, &argvU8[0]);
}

//
//
//
int Application::run(std::string_view sName, std::string_view sFullName, 
	int argc, const char* const argv[])
{
	try
	{
		error::dump::win::initCrashHandlers();
		return (int)run2(sName, sFullName, argc, argv);
	}
	catch (error::Exception& ex)
	{
		// Output unhandled exceptions to stderr
		if (m_fOutExceptionLog)
			ex.log(SL, "Error occurred during application execution");
		std::cerr << ex;
		return (int)ex.getErrorCode();
	}
	catch (...)
	{
		// Output unhandled exceptions to stderr
		error::SystemError(SL, "Unknown unhandled exception in application").log();
		std::cerr << "Unknown unhandled exception in application";
		return (int)ErrorCode::SystemError;
	}
}

//
//
//
ErrorCode Application::run2(std::string_view sName, std::string_view sFullName, 
	int argc, const char* const argv[])
{
	ErrorCode ec = ErrorCode::OK;
	std::exception_ptr pCurrException;
	CMD_TRY
	{	
		if (!doBasicStartup(sName, sFullName, argc, argv))
			return ec;

		ec = process();
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.log(SL, "Error is occurred in application");
		m_fOutExceptionLog = false;
		pCurrException = std::current_exception();
		ec = ex.getErrorCode();
	}

	ec = doBasicShutdown(ec);
	if (pCurrException)
		std::rethrow_exception(pCurrException);
	return ec;
}

//
//
//
ErrorCode Application::process()
{
	// Perform advanced startup operations
	if (!doStartup())
		return ErrorCode::NoAction;

	// Runs the mode handler
	return doShutdown(runMode());
}

//
//
//
ErrorCode Application::runMode()
{
	{
		std::unique_lock<std::mutex> sync(m_mtxTerminate);
		if (m_fTerminate)
			return ErrorCode::OK;
	}
	if (m_pAppMode)
		return m_pAppMode->main(this);
	LOGWRN("The handler for default working mode is not specified - application is exiting");
	return ErrorCode::NoAction;
}

//
//
//
bool Application::doBasicStartup(std::string_view sAppName, 
	std::string_view sFullName, int argc, const char* const argv[])
{
	TRACE_BEGIN;

#ifdef PLATFORM_WIN
	const Size c_nMaxNtPathSize = 0x8000;
	std::wstring sImageFile(c_nMaxNtPathSize, 0);
	if (::GetModuleFileNameW(NULL, &sImageFile[0], c_nMaxNtPathSize) == 0)
	{
		error::win::WinApiError(SL, "Can't get image path").log();
		sImageFile.clear();
	}
	const wchar_t* pImagePath = sImageFile.c_str();
#else
	const char* pImagePath = argv[0];
#endif
	
	putCatalogData("app", Dictionary({
		{ "name", sAppName },
		{ "fullName", sFullName },
		{ "stage", "starting" },
		{ "imageFile", pImagePath }
	}));

	initEnvironment(false);
	if (!parseParameters(argc, argv)) return false;
	loadConfig();
	initLog();

	// Init global objects
	putCatalogData("objects", createObject(CLSID_ServiceManager));
	putCatalogData("objects.application", getPtrFromThis());

	processConfig();
	doSelfcheck();

	// Output config-depended data
	LOGINF("Current log level: " << (Size)(getCatalogData("app.config.log.logLevel", 0)));
	LOGINF("Customer Id: " << std::string(getCatalogData("app.config.license.customerId", "<undefined>")));
	LOGINF("Endpoint Id: " << std::string(getCatalogData("app.config.license.endpointId", "<undefined>")));
	LOGINF("Active machine Id: " << std::string(getCatalogData("app.config.license.machineId", "<undefined>")));

	return true;
	TRACE_END("Basic startup error");
}

//
//
//
bool Application::doStartup()
{
	TRACE_BEGIN;
	LOGINF("Processing startup modules");

	// Send startup event
	sendMessage(Message::AppStarted);

	// Set state to active
	putCatalogData("app.stage", "active");

	LOGINF("Startup modules are processed");
	return true;
	TRACE_END("Startup error");
}

//
//
//
ErrorCode Application::doShutdown(ErrorCode errCode)
{
	TRACE_BEGIN;
	{
		std::unique_lock<std::mutex> sync(m_mtxTerminate);
		m_fTerminate = true;
		m_cvTerminate.notify_all();
	}
	LOGINF("Application is shutting down - <AppFinishing> stage");
	sendMessage(Message::AppFinishing);
	putCatalogData("app.stage", "finishing");
	if (getCatalogData("objects", nullptr) != nullptr)
		(void)execCommand("objects", "stop");

	// Shutdown core thread pools
	LOGINF("Global pools are being finished...");
	shutdownGlobalPools();

	LOGINF("Application is shutting down - <AppFinished> stage");
	putCatalogData("app.stage", "finished");
	sendMessage(Message::AppFinished);
	LOGINF("Application is shutting down - <shutdown> stage");
	if (getCatalogData("objects", nullptr) != nullptr)
		(void)execCommand("objects", "shutdown");
	return errCode;
	TRACE_END("Shutdown error");
}

//
//
//
ErrorCode Application::doBasicShutdown(ErrorCode errCode)
{
	TRACE_BEGIN;
	// Try to shutdown if it wasn't performed before
	std::string sStage = getCatalogData("app.stage", "finished");
	if (sStage != "finished")
	{
		if (sStage == "finishing")
			LOGWRN("Shutdown operation is incompleted");
		else
			doShutdown(errCode);
	}

	putCatalogData("objects", nullptr);
	clearCatalog();
	unsubscribeAll();

	LOGINF("Application shutdowned (" << errCode << ")");
	return errCode;
	TRACE_END("Basic shutdown error");
}

//
//
//
bool Application::needShutdown()
{
	std::unique_lock<std::mutex> sync(m_mtxTerminate);
	return m_fTerminate;
}

//
//
//
void Application::shutdown()
{
	std::unique_lock<std::mutex> sync(m_mtxTerminate);
	m_fTerminate = true;
	m_cvTerminate.notify_all();
}

//
//
//
void Application::waitShutdown()
{
	LOGLVL(Detailed, "Application waits shutdown()");
	std::unique_lock<std::mutex> sync(m_mtxTerminate);
	m_cvTerminate.wait(sync, [this] {return m_fTerminate; });
}

//
//
//
std::pair<std::wstring_view, std::wstring_view> splitEnvVar(std::wstring_view sLine)
{
	std::wstring_view sKey;
	std::wstring_view sValue;

	auto nEndOfKey = sLine.find_first_of(L"=");
	if (nEndOfKey == std::wstring_view::npos)
		sKey = sLine;
	else
	{
		sKey = sLine.substr(0, nEndOfKey);
		sValue = sLine.substr(nEndOfKey + 1);
	}
	return std::make_pair(sKey, sValue);
}

#ifdef PLATFORM_WIN
#pragma warning(push)
#pragma warning(disable:4996)
fs::path getEnvPath(const wchar_t* sEnvVarName)
{
	const wchar_t* sPath = _wgetenv(sEnvVarName);
	if (sPath == nullptr)
		return {};
	return sPath;
}

#pragma warning(pop)
#endif

//
//
//
void Application::initEnvironment(bool fUseCommonData)
{
	TRACE_BEGIN;

	fs::path pathImageFile = std::string(getCatalogData("app.imageFile"));
	putCatalogData("app.imagePath", pathImageFile.parent_path().c_str());

	fs::path pathWorkDir = std::filesystem::current_path();
	putCatalogData("app.workPath", pathWorkDir.c_str());

	// Fill Application data
#ifdef PLATFORM_WIN
	fs::path pathProfile = getEnvPath(L"USERPROFILE");
	if(pathProfile.empty())
		error::SystemError(SL, "Can't found %USERPROFILE%.").throwException();
	fs::path pathTemp = getEnvPath(L"TEMP");
	if (pathTemp.empty())
		error::SystemError(SL, "Can't found %TEMP%.").throwException();

	fs::path pathLocalAppData = getEnvPath(L"LOCALAPPDATA");
	if (pathLocalAppData.empty())
	{
		error::SystemError(SL, "Can't found %LOCALAPPDATA%.").log();
		pathLocalAppData = pathTemp;
	}

	fs::path pathData = pathLocalAppData;

	if (fUseCommonData)
	{
		wchar_t pathCommonAppData[MAX_PATH]; *pathCommonAppData = 0;
		auto hRes = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, pathCommonAppData);
		if (FAILED(hRes))
			error::win::WinApiError(SL, error::win::getFromHResult(hRes), "Can't get common data folder").log();
		else
			pathData = pathCommonAppData;
	}

#else
#error Add code for different paltrorm 
// we may use getenv("HOME");
#endif

	putCatalogData("app.homePath", pathProfile.c_str());
	putCatalogData("app.tempPath", pathTemp.c_str());
	putCatalogData("app.silent", m_cmdLineParams.m_fSilentMode);

	std::string sAppName = getCatalogData("app.name");
	fs::path pathAppData = pathData / sAppName;
	putCatalogData("app.defaultDataPath", pathAppData.c_str());

	fs::path pathLogData = pathAppData / "log";
	putCatalogData("app.logPath", pathLogData.c_str());
	
	time_t timeAppStart = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000;
	putCatalogData("app.startTime", timeAppStart);
	
	// Fill OS data
#ifdef PLATFORM_WIN
#pragma warning(push)
#pragma warning(disable:4996)

	// TODO: Move code to the planned libsys library
	SYSTEM_INFO sinfo = {0};
	::GetSystemInfo(&sinfo);
	Size nCpuCount = sinfo.dwNumberOfProcessors;
	Size nPageSize = sinfo.dwPageSize;
	std::string sCpuType("unknown");
	switch (sinfo.wProcessorArchitecture)
	{
		case PROCESSOR_ARCHITECTURE_AMD64: sCpuType = c_sAmd64; break;
		case PROCESSOR_ARCHITECTURE_ARM64: sCpuType = c_sArm64; break;
		case PROCESSOR_ARCHITECTURE_IA64: sCpuType = c_sIa64; break;
		case PROCESSOR_ARCHITECTURE_ARM: sCpuType = c_sArm; break;
		case PROCESSOR_ARCHITECTURE_INTEL: sCpuType = c_sX86; break;
	}

	MEMORYSTATUSEX memstatus = {0};
	memstatus.dwLength = sizeof(memstatus);
	if (!::GlobalMemoryStatusEx(&memstatus))
		error::win::WinApiError(SL, "Can't get OS memory state").log();
	Size nMemSizeMb = static_cast<Size>(memstatus.ullTotalPhys >> 20);
	
	size_t nWinDirLen = GetWindowsDirectoryW(NULL, 0);
	MemPtr<wchar_t> sWinDir = allocMem<wchar_t>(nWinDirLen);
	if (::GetWindowsDirectoryW((LPWSTR)sWinDir.get(), UINT(nWinDirLen)) != nWinDirLen - 1)
		error::win::WinApiError(SL, HRESULT_FROM_WIN32(GetLastError()),
			"Can't get Windows directory").log();
	std::wstring sWindowsDir(sWinDir.get());

	PWSTR pSystemDir;
	if (::SHGetKnownFolderPath(FOLDERID_System, 0, NULL, &pSystemDir) != S_OK)
		error::win::WinApiError(SL, "Fail to get path of system directory").log();
	std::wstring sSystemDir(pSystemDir);
	::CoTaskMemFree(pSystemDir);

#ifdef _WIN64
	PWSTR pSyswowDir;
	if (::SHGetKnownFolderPath(FOLDERID_SystemX86, 0, NULL, &pSyswowDir) != S_OK)
		error::win::WinApiError(SL, "Fail to get path of system directory").log();
	std::wstring sSyswowDir(pSyswowDir);
	::CoTaskMemFree(pSyswowDir);
#endif

#pragma warning(pop)
#else
#error Add code for different platforms here
#endif

	auto osVersion = sys::getOsVersion();

	putCatalogData("os", Dictionary(
	{ 
		{ "osName", sys::getOsFriendlyName()},
		{ "osFamily", sys::getOsFamilyName()},
		{ "osMajorVersion", osVersion.nMajor},
		{ "osMinorVersion", osVersion.nMinor},
		{ "osBuildNumber", osVersion.nBuild},
		{ "osRevision", osVersion.nRevision},
		{ "machineId", sys::getUniqueMachineId()},
		{ "bootTime", sys::getOsBootTime()},
		{ "cpuType", sCpuType},
		{ "cpuCount", nCpuCount},
		{ "memPageSize", nPageSize},
		{ "memSizeMb", nMemSizeMb},
		{ "hostName", sys::getHostName()},
		{ "domainName", sys::getDomainName()},
		{ "userName", sys::getUserName()},
		{ "windowsDir", sWindowsDir},
		{ "systemDir", sSystemDir},
#ifdef _WIN64
		// FIXME: Remove platform-dependent data from catalog
		{ "syswowDir", sSyswowDir},
#endif
	}));

	// Fill environment variables
#ifdef PLATFORM_WIN
	sys::win::ScopedHandle hToken;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken))
		error::win::WinApiError(SL, GetLastError(),
			"Fail to get process token").throwException();

	sys::win::ScopedEnvBlock pEnv;
	if (!::CreateEnvironmentBlock(&pEnv, hToken, TRUE))
		error::win::WinApiError(SL, GetLastError(),
			"Can't create an environment block").throwException();
	
	Variant vEnv = Dictionary();
	for (auto pCurEnv = (wchar_t*)pEnv.get(); *pCurEnv; pCurEnv += wcslen(pCurEnv) + 1)
	{
		auto kv = splitEnvVar(pCurEnv);
		if (kv.first.empty())
			continue;
		std::string sKey = string::convertWCharToUtf8(kv.first);
		std::transform(sKey.begin(), sKey.end(), sKey.begin(), 
			[](char c) { return static_cast<char>(std::tolower(c)); });
		vEnv.put("%env_" + sKey + "%", kv.second);
	}
	putCatalogData("app.variables", vEnv);
#else
#error Add code for different platforms here
#endif

	TRACE_END("Environment processing");
}

//
//
//
inline uint32_t getCurrentProcessId()
{
#ifdef PLATFORM_WIN
	return GetCurrentProcessId();
#else
#error Add code for different paltrorm 
#endif
}

//
//
//
void Application::initLog()
{
	TRACE_BEGIN;

	// Set log path
	if (!m_cmdLineParams.m_pathLogData.empty())
		putCatalogData("app.logPath", m_cmdLineParams.m_pathLogData.c_str());

	std::error_code ec;
	fs::path pathLogData(getCatalogData("app.logPath"));
	fs::create_directories(pathLogData, ec);
	if (ec)
	{
		error::StdSystemError(SL, ec, FMT("Can't access to logs data directory <"
			<< pathLogData << ">. Temp directory is used")).log();
		pathLogData = getCatalogData("app.tempPath");
		putCatalogData("app.logPath", pathLogData.c_str());
	}

	fs::path pathDumpData = pathLogData / "dump";
	ec.clear();
	fs::create_directories(pathDumpData, ec);
	if (ec)
	{
		error::StdSystemError(SL, ec, FMT("Can't access to dump data directory <"
			<< pathDumpData << ">. Temp directory is used")).log();
		pathDumpData = pathLogData;
	}

	if (m_cmdLineParams.m_fSilentMode)
		setRootLogLevel(LogLevel::Off);
	else
		logging::initLogging();

	// @ermakov TODO: Add postCrash handler when it is ready
	error::dump::win::initCrashHandlers(Dictionary{ {"dumpDirectory", pathDumpData.c_str()} });

	// Init memoryleakpath
	{
		auto sFilename = FMT("memoryleaks-" <<
			std::hex << std::noshowbase << std::setfill('0') << std::setw(16) << getCurrentTime()
			<< "-" << std::dec << getCurrentProcessId() << ".log");
		std::filesystem::path pathMemoryLeakFilename = pathLogData / sFilename;
		setMemLeaksInfoFilename(pathMemoryLeakFilename);
	}

	outputLogHeader();
	TRACE_END("Error during log initialization");
}

//
// Function outputs initial data to log file
//
void Application::outputLogHeader()
{
	LOGINF("Application: " << getCatalogData("app.fullName", "<undefined>") 
		<< " [" << CPUTYPE_STRING << "] " << CMD_LOG_VERSION);

	//time_t appStartTime = getCatalogData("app.startTime");
	//std::tm tm = {0};
	//if (localtime_s(&tm, &(appStartTime / 1000)))
	//	error::CrtError(SL, "Can't calculate application start time").log();
	//LOGINF("Application start time: " << std::put_time(&tm, "%c %z"));
	
	LOGINF("Application start time: " << getIsoTime(getCatalogData("app.startTime", 0)));
	LOGINF("OS boot time: " << getIsoTime(getCatalogData("os.bootTime", 0)));
	LOGINF("OS version: " << getCatalogData("os.osName", 0));
	LOGINF("CPU architecure: " << getCatalogData("os.cpuType", 0) << ", " << getCatalogData("os.cpuCount", 1) << " core(s)");
	LOGINF("Host: " << getCatalogData("os.hostName", ""));
	LOGINF("Domain: " << getCatalogData("os.domainName", ""));
	LOGINF("Process Id: " << getCurrentProcessId());
	LOGINF("Config file: " << getCatalogData("app.config.file", ""));
	LOGINF("Working directory: " << getCatalogData("app.workPath", ""));
	LOGINF("Logging directory: " << getCatalogData("app.logPath", ""));
	LOGINF("Machine Id: " << getCatalogData("os.machineId", "<undefined>"));
	LOGINF("Application working mode: " << m_sAppMode);
	LOGINF("Command line: " << m_sCmdLine);
}

//
//
//
void Application::doSelfcheck()
{
	TRACE_BEGIN;
	if (!getCatalogData("app.config.selfcheck.enable", false))
		return;

	// Checking configuration files
	Sequence seqConfigs(getCatalogData("app.config.selfcheck.configs", Sequence()));
	for (auto& vCfgInfo : seqConfigs)
	{
		bool fNeedRepair = false;
		LOGLVL(Detailed, "Checking <" << vCfgInfo.get("name", "undefined") << "> component...");
		std::filesystem::path pathToFile = vCfgInfo["path"].convertToPath();
		try
		{
			if (std::filesystem::exists(pathToFile))
			{
				auto vCfg = variant::deserializeFromJson(io::createFileStream(pathToFile,
					io::FileMode::Read | io::FileMode::ShareRead | io::FileMode::ShareWrite));
				for (auto& vFieldName : vCfgInfo.get("fields", Sequence()))
				{
					if (variant::getByPath(vCfg, vFieldName, "testStub") == "testStub")
						error::InvalidFormat(SL, FMT("Required field <" << vFieldName << "> is absent")).throwException();
				}
			}
			else if (vCfgInfo.get("required", true) == false)
				continue;
			else
			{
				LOGWRN("Configuration file is missing. It is being repaired using the specified action.");
				fNeedRepair = true;
			}
		}
		catch (error::InvalidFormat& ex)
		{
			ex.log();
			LOGWRN("Configuration file is damaged. It is being repaired using the specified action.");
			fNeedRepair = true;
		}
		catch (error::NoData& ex)
		{
			if (!vCfgInfo.get("allowEmpty", false))
			{
				ex.log();
				LOGWRN("Configuration file is damaged. It is being repaired using the specified action.");
			}
			fNeedRepair = true;
		}

		if (fNeedRepair)
		{
			std::string sAction = vCfgInfo.get("action", "exit");
			if (sAction == "exit")
				throw;
			else if (sAction == "delete")
			{
				LOGLVL(Detailed, "Removing file <" << std::string(vCfgInfo["path"]) << ">...");
				std::error_code ec;
				std::filesystem::remove(pathToFile, ec);
			}
			else if (sAction == "create")
			{
				LOGLVL(Detailed, "Creating config file <" << std::string(vCfgInfo["path"]) << "> with defaults...");
				auto pStream = queryInterface<io::IWritableStream>(io::createFileStream(pathToFile,
					io::FileMode::Write | io::FileMode::CreatePath | io::FileMode::Truncate));
				variant::serialize(pStream, vCfgInfo.get("default", ""));
				pStream->flush();
			}
			else
				error::InvalidArgument(SL, FMT("Unknown action <" << sAction << "> is specified")).throwException();
		}
	}

	TRACE_END("Error during selfchecking");
}


//
//
//
Variant loadConfig(Variant vFullCfg, std::string_view sMode)
{
	TRACE_BEGIN;
	if (vFullCfg[sMode].has("baseMode"))
	{
		auto vBaseCfg = loadConfig(vFullCfg, vFullCfg[sMode]["baseMode"]);
		return vBaseCfg.merge(vFullCfg[sMode], variant::MergeMode::All |
			variant::MergeMode::CheckType | variant::MergeMode::MergeNestedDict | 
			variant::MergeMode::MergeNestedSeq);
	}
	return vFullCfg[sMode];
	TRACE_END(FMT("Can't load config for mode <" << sMode << ">"));
}

//
//
//
void Application::loadConfig()
{
	TRACE_BEGIN;
	std::string sAppName = getCatalogData("app.name");

	// Default initialization
	if (m_cmdLineParams.m_pathConfigFile.empty())
	{
		m_cmdLineParams.m_pathConfigFile = getCatalogData("app.defaultDataPath");
		m_cmdLineParams.m_pathConfigFile /= sAppName + ".cfg";
	}

	// Attempt to load from image directory
	if (!fs::exists(m_cmdLineParams.m_pathConfigFile))
	{
		m_cmdLineParams.m_pathConfigFile = getCatalogData("app.imagePath");
		m_cmdLineParams.m_pathConfigFile /= sAppName + ".cfg";
	}
	m_cmdLineParams.m_pathConfigFile = fs::absolute(m_cmdLineParams.m_pathConfigFile);

	Variant vFullCfg{};
	auto fFileExists = fs::exists(m_cmdLineParams.m_pathConfigFile);
	if (fFileExists)
	{
		TRACE_BEGIN;
		putCatalogData("app.configPath", m_cmdLineParams.m_pathConfigFile.parent_path().c_str());
		auto pCfgStream = io::createFileStream(m_cmdLineParams.m_pathConfigFile,
			io::FileMode::Read | io::FileMode::ShareRead);
		vFullCfg = variant::deserializeFromJson(pCfgStream);
		TRACE_END(FMT("Can't load config <" << m_cmdLineParams.m_pathConfigFile << ">"));
	}
	else
	{
		LOGWRN("Configuration file is not found");
		fFileExists = false;
	}

	auto vCfg = vFullCfg;
	if (!vFullCfg.isEmpty())
	{
		if (vFullCfg.has(m_sAppMode))
			vCfg = openEdr::loadConfig(vFullCfg, m_sAppMode);
		else if (vFullCfg.has(c_sDefModeName))
			vCfg = openEdr::loadConfig(vFullCfg, c_sDefModeName);
	}

	putCatalogData("app.config", vCfg);
	if (fFileExists)
		putCatalogData("app.config.file", m_cmdLineParams.m_pathConfigFile.c_str());
	
	// Overrride config values according to parameters
	if (m_cmdLineParams.m_nLogLevel != c_nMaxSize)
	{
		if (getCatalogData("app.config.log", Dictionary()).isEmpty())
			putCatalogData("app.config.log", Dictionary());
		putCatalogData("app.config.log.logLevel", m_cmdLineParams.m_nLogLevel);
		putCatalogData("app.config.log.cmdLineLogLevel", m_cmdLineParams.m_nLogLevel);
	}

	TRACE_END("Error during loading configuration");
}

//
//
//
void Application::processConfig()
{
	TRACE_BEGIN;

	// Update data path
	if (!m_cmdLineParams.m_pathData.empty())
		putCatalogData("app.dataPath", m_cmdLineParams.m_pathData.c_str());
	else
	{
		std::string sNewDataPath = getCatalogData("app.config.dataPath", "");
		if (!sNewDataPath.empty())
			putCatalogData("app.dataPath", sNewDataPath);
		else
			putCatalogData("app.dataPath", getCatalogData("defaultDataPath"));
	}

	std::error_code ec;
	std::filesystem::path pathData = getCatalogData("app.dataPath").convertToPath();
	std::filesystem::create_directories(pathData, ec);
	if (ec)
	{
		error::StdSystemError(SL, ec, FMT("Can't access to application data directory <"
			<< pathData << ">. Temp directory is used")).log();
		pathData = getCatalogData("app.tempPath");
		putCatalogData("app.dataPath", pathData.c_str());
	}

	// Set log level from config
	// It is need todo after log initialization and finish doBasicStartup()
	if (!m_cmdLineParams.m_fSilentMode)
	{
		CMD_TRY
		{
			auto wll = getCatalogDataSafe("app.config.log.workLogLevel");
			if (wll.has_value())
			{
				putCatalogData("app.config.log.logLevel", wll.value());
				logging::setRootLogLevel(wll.value());
			}
			for (auto[sName, vLogLevel] : Dictionary(getCatalogData("app.config.log.workComponents", Dictionary())))
				logging::setComponentLogLevel(std::string(sName), vLogLevel);
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& ex)
		{
			ex.log(SL, "Error is occurred in setting log levels.");
		}
	}

	uint32_t nMemLeaksTimeout = getCatalogData("app.config.hangOnMemoryLeaks", 0);
	if (nMemLeaksTimeout > 0)
		setMemLeaksTimeout(nMemLeaksTimeout);

	TRACE_END("Error during processing configuration");
}

//
//
//
bool Application::parseParameters(int argc, const char* const argv[])
{
	using namespace clara;

	for (int i = 0; i < argc; ++i)
	{
		m_sCmdLine += argv[i];
		if (i != argc - 1) m_sCmdLine += " ";
	}

	// Preselect the mode from parameters
	if (argc> 1 && argv[1][0] != '-' && argv[1][0] != '/')
		m_sAppMode = argv[1]; 
	auto itAppMode = m_appModes.find(m_sAppMode);
	
	// Try to set default app mode
	if (m_sAppMode.empty())
	{
		itAppMode = m_appModes.find(c_sDefModeName);
		if (itAppMode != m_appModes.end())
			m_sAppMode = c_sDefModeName;
	}
	
	bool fShowHelp = false;
	bool fPrintVersion = false;	
	std::string sFakeMode;
	auto cli
		= Help(fShowHelp) | 
		Opt(m_cmdLineParams.m_pathConfigFile, "path-to-file")["-c"]["--config"]
			("set the main config file") |
		Opt(m_cmdLineParams.m_pathData, "path-to-file")["--data"]
			("set the data files") |
		Opt(m_cmdLineParams.m_nLogLevel, "0..5")["-l"]["--loglevel"]
			("set the level of a global logging") |
		Opt(m_cmdLineParams.m_fSilentMode) ["-s"]["--silent"]
			("enable silent mode (no stdout and log)") |
		Opt(m_cmdLineParams.m_pathLogData, "path-to-dir")["--logpath"]
			("set the path for log files") |
		Opt(fPrintVersion)["-v"]["--version"]
			("print a program version");	

	// Prepare a list of modes according to the registered AppModes 
	if (m_sAppMode.empty() || m_sAppMode == c_sDefModeName || 
		itAppMode == m_appModes.end())
	{
		std::string sModeHelp;
		for (auto itMode : m_appModes)
			sModeHelp += itMode.first + "|";
		if (!sModeHelp.empty())
		{
			sModeHelp.resize(sModeHelp.size() - 1);
			cli += Arg(sFakeMode, sModeHelp) ("Working mode");
		}
	}
	else
	{
		cli += Arg(sFakeMode, m_sAppMode) ("Working mode");
	}

	// Runs the appmode parameters initializer
	if (itAppMode != m_appModes.end())
		itAppMode->second->initParams(cli);

	// Parse CL parameters
	auto result = cli.parse(Args(argc, argv));
	if (!result)
		error::InvalidArgument(SL, FMT("Error in command line (" << result.errorMessage() << ")")).throwException();

	// Show help and exit
	if (fShowHelp)
	{
		std::cout << getCatalogData("app.fullName", "<unnamed>") 
			<< " v." << CMD_STD_VERSION << std::endl << cli << std::endl;
		return false;
	}

	// Print version and exit
	if (fPrintVersion)
	{
		std::cout << CMD_LOG_VERSION;
		return false;
	}

	putCatalogData("app.mode", m_sAppMode);
	if (itAppMode == m_appModes.end() && !m_sAppMode.empty())
	{
		std::cerr << "ERROR: Invalid working mode <" << m_sAppMode << ">" << std::endl;
		std::cout << "The following modes and parameters are suppoted:" 
			<< std::endl << std::endl << cli << std::endl;
		return false;
	}

	// Disable stdout
	if (m_cmdLineParams.m_fSilentMode)
	{
		std::cout.setstate(std::ios_base::failbit);
		// std::cerr.setstate(std::ios_base::failbit);
	}

	if (itAppMode != m_appModes.end())
		m_pAppMode = itAppMode->second;

	return true;
}

//
//
//
void Application::Sleep(Size nDurationMs)
{
	std::unique_lock<std::mutex> lock(m_mtxTerminate);

	auto finishTime = boost::chrono::steady_clock::now() + boost::chrono::milliseconds(nDurationMs);

	// Wait sender or timeout
	while (true)
	{
		auto eWaitResult = m_cvTerminate.wait_until(lock, finishTime);
		if (eWaitResult == boost::cv_status::timeout)
			return;

		// if shutdown is started
		if (m_fTerminate)
			error::ShutdownIsStarted(SL, "Sleep is break because application is shutting down").throwException();
	}
}


///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * 'objects.application'
///
/// #### Supported commands
///
Variant Application::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant Application::execute()
	///
	/// ##### shutdown()
	/// Performs the current application's shutdown.
	///
	if (vCommand == "shutdown")
	{
		shutdown();
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### sleep()
	/// Sleep specified time.
	/// If application is shutting down, throw exception ShutdownIsStarted.
	///   * msec [int] - time of sleep in milliseconds.
	///
	if (vCommand == "sleep")
	{
		Sleep(vParams.get("msec"));
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### delay()
	/// Makes a pause in execution. It can be used inside configuration scripts.
	///   * msec [int] - time of delay in milliseconds.
	/// 
	else if (vCommand == "delay")
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(
			vParams.get("msec", 1000)));
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### echo()
	/// Returns all passed parameters back as a result. It can be used for debug purposes.
	///
	else if (vCommand == "echo")
	{
		return vParams;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### log()
	/// Writes message to log.
	///   * message [str] - message for writting.
	///   * logLevel [str] - log level (mormal, detailed, debug, trace, filtered, critical).
	///
	else if (vCommand == "log")
	{
		std::string sLevel = vParams.get("logLevel", "normal");
		if (sLevel == "normal")
			LOGLVL(Normal, vParams["message"]);
		else if (sLevel == "detailed")
			LOGLVL(Detailed, vParams["message"]);
		else if (sLevel == "debug")
			LOGLVL(Debug, vParams["message"]);
		else if (sLevel == "trace")
			LOGLVL(Trace, vParams["message"]);
		else if (sLevel == "filtered")
			LOGLVL(Filtered, vParams["message"]);
		else if (sLevel == "critical")
			LOGLVL(Critical, vParams["message"]);
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### setLogLevel()
	/// Sets the log level of an application. It can be used for debug purposes.
	///   * root [int] - Log level for a root component.
	///
	else if (vCommand == "setLogLevel")
	{
		logging::setRootLogLevel(vParams.get("root", LogLevel::Normal));
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### error()
	/// Generates a generic runtime error. It can be used for debug purposes.
	///
	else if (vCommand == "error")
	{
		error::RuntimeError(SL, "Test runtime error").throwException();
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### crash()
	/// Generates a crash in an application. It can be used for debug purposes.
	///
	else if (vCommand == "crash")
	{
		int* p = nullptr;
		*p = 100;
		return {};
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### doMemLeak()
	/// Generates memory leak in an application. It can be used for debug purposes.
	///	  * size [int, opt] - size of one leaked block in bytes (default: 1024).
	///	  * count [int, opt] - count of leaked blocks (default: 1).
	///	  * line [int, opt] - line in memory leak debug info (default: 1). 
	/// Each memory allocation has debug info: file and line. File is always "doMemLeak". Line can be changed.
	///
	else if (vCommand == "doMemLeak")
	{
		Size nSize = vParams.get("size", 1024);
		Size nCount = vParams.get("count", 1);
		Size nLine = vParams.get("line", 1);

		{
			CHECK_IN_SOURCE_LOCATION("doMemLeak", (int)nLine);
			for (Size i = 0; i < nCount; ++i)
				allocMem(nSize).release();
		}

		return {};
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### execute()
	/// Executes the specified command.
	///	  * processor [dict] - processor object definition.
	///   * command [dict] - command for execution.
	///	  * params [dict] - command's parameters.
	///
	else if (vCommand == "execute")
	{
		return execCommand(vParams);
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### run()
	/// Runs the specified external application.
	///	  * path [str] - path to application.
	///   * params [dict] - application parameters.
	///	  * isElevated [bool] - flag for applications which required elevation.
	///	  * timeout [int] - startup timeout.
	///
	else if (vCommand == "run")
	{
		auto ec = sys::executeApplication(vParams["path"].convertToPath(), vParams.get("params", ""), 
			vParams.get("isElevated", false), vParams.get("timeout", c_nMaxSize));
		return ec;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### getCatalogData()
	/// Returns data from a global catalog.
	///	  * path [str] - path in the global catalog.
	///
	// TODO: Unite "getCatalogData" and "getConfigValue"
	else if (vCommand == "getCatalogData")
	{
		if (vParams.has("default"))
			return getCatalogData(vParams.get("path", ""), vParams["default"]);
		if (vParams.get("resolve", false))
		{
			Variant v = getCatalogData(vParams.get("path", ""));
			v.getType();
			return v;
		}

		return getCatalogData(vParams.get("path", ""));
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### putCatalogData()
	/// Set data value in the global catalog.
	///	  * path [str] - path in the global catalog.
	///	  * data [var] - data.
	///
	else if (vCommand == "putCatalogData")
	{
		putCatalogData(vParams.get("path", "INVALID_PATH"), vParams.get("data", {} ));
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### clearCatalogData()
	/// Deletes specified data branch.
	///	  * path [str] - path in the global catalog.
	///
	else if (vCommand == "clearCatalogData")
	{
		putCatalogData(vParams.get("path", "INVALID_PATH"), nullptr);
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### deleteFile()
	/// Deletes the specified file.
	///	  * path [str] - path in the global catalog.
	///	  * failSafe [bool] - don't throw exception in case of error.
	///	  * silent [bool] - don't log error.
	///
	// TODO: Move command to the separate object
	else if (vCommand == "deleteFile")
	{
		bool fSafe = vParams.get("failSafe", false);
		CMD_TRY
		{
			std::filesystem::path pathObj(vParams["path"].convertToPath());
			std::filesystem::remove(pathObj);
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& ex)
		{
			if (!fSafe) throw;
			if (!vParams.get("silent", false))
				ex.log(SL, FMT("Can't delete file object <" << vParams["path"] << ">"));
			return false;
		}
		catch (...)
		{
			if (!fSafe) throw;
			if (!vParams.get("silent", false))
				error::RuntimeError(SL, FMT("General error duing deletion a file object <"
					<< vParams["path"] << ">")).log();
			return false;
		}
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### deleteAll()
	/// Deletes the specified file or directory.
	///	  * path [str] - path in the global catalog.
	///	  * failSafe [bool] - don't throw exception in case of error.
	///	  * silent [bool] - don't log error.
	///
	else if (vCommand == "deleteAll")
	{
		bool fSafe = vParams.get("failSafe", false);
		CMD_TRY
		{
			std::filesystem::path pathObj(vParams["path"].convertToPath());
			std::filesystem::remove_all(pathObj);
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& ex)
		{
			if (!fSafe) throw;
			if (!vParams.get("silent", false))
				ex.log(SL, FMT("Can't delete file object <" << vParams["path"] << ">"));
			return false;
		}
		catch (...)
		{
			if (!fSafe) throw;
			if (!vParams.get("silent", false))
				error::RuntimeError(SL, FMT("General error duing deletion a file object <"
					<< vParams["path"] << ">")).log();
			return false;
		}
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### getConfigValue()
	/// Combines value from several paths of globalCatalog.
	///	  * paths [cl|str] - collection of string or one string. It contains paths in the global catalog.
	///	  * default [opt] - default value. It is returned if no one of "paths" is exist.
	///	  * mergeMode [int, opt] - values merging mode (combination of MergeMode).
	///       0 - disable merging, just replacement. (default: 0).
	///	  * skipNull [bool, opt] - skip paths which contain null (as if they are absent). (default: false).
	///       Attention: If skipNull is true and path contains proxy, it will be resolved.
	///
	/// First elements of 'paths' has more priority than later elements.
	///
	/// If mergeMode is not 0, it gets default value and consistently updates it with a value 
	/// located at each path (from last element to first). If path is absent, it is not used for the update.
	///
	/// If mergeMode is 0, optimization is applied. 
	/// It finds first exist path and returns its value. If all paths are absent, default value is returned.
	///
	/// If all paths and the 'default' parameter are absent, exception is thrown. 
	///
	else if (vCommand == "getConfigValue")
	{
		auto vDefault = vParams.getSafe("default");
		variant::MergeMode eMergeMode = vParams.get("mergeMode", 0);
		bool fSkipNull = vParams.get("skipNull", false);

		std::vector<std::string> paths;
		{
			TRACE_BEGIN;
			Sequence vPaths(vParams.get("paths"));
			for (auto vPath : vPaths)
				paths.push_back(vPath);
			TRACE_END(FMT("Invalid or absent 'paths' parameter"));
		}

		// Optimization for replacement
		if (eMergeMode == variant::MergeMode(0))
		{
			for (auto& path : paths)
			{
				auto vPathValue = getCatalogDataSafe(path);
				if (!vPathValue.has_value())
					continue;
				if(fSkipNull && vPathValue.value().isNull())
					continue;

				return vPathValue.value();
			}

			if (vDefault.has_value())
				return vDefault.value();
		}

		// merge
		else
		{
			Variant vValue;
			bool fHasAnyPath = false;

			if (vDefault.has_value())
			{
				vValue = vDefault->clone();
				fHasAnyPath = true;
			}

			for (auto iter = paths.rbegin(); iter != paths.rend(); ++iter)
			{
				auto vPathValue = getCatalogDataSafe(*iter);
				if (!vPathValue.has_value())
					continue;

				if (fSkipNull && vPathValue.value().isNull())
					continue;

				fHasAnyPath = true;
				vValue.merge(vPathValue.value(), eMergeMode);
			}

			if (fHasAnyPath)
				return vValue;
		}

		// Error in case absence of any values
		error::NotFound(SL, "Value is not found and default parameter is not set.").throwException();
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### startTask()
	/// Returns data from a global catalog.
	///	  * name [str] - task name.
	///	  * command [obj] - command for start. If not specified then the existing command is started.
	///	  * period [int] - period for repeating in ms. (default: 100 ms)
	///
	else if (vCommand == "startTask")
	{
		if (vParams.has("command"))
		{
			auto pCommand = queryInterfaceSafe<ICommand>(vParams["command"]);
			if (pCommand == nullptr)
				pCommand = createCommand(vParams["command"]);
			return startTask(vParams["name"], vParams.get("period", 100), pCommand);
		}
		return startTask(vParams["name"]);
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### startTask()
	/// Returns data from a global catalog.
	///	  * name [str] - task name.
	///	  * command [obj] - command for start.
	///	  * period [int] - period for repeating.
	///
	else if (vCommand == "stopTask")
	{
		return stopTask(vParams["name"]);
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### subscribeToMessage()
	/// Binds the command handler to the event.
	///	  * id [str] - message identifier.
	///	  * command [obj] - command for subscription.
	///	  * subscriptionId [str] - subscription identifier.
	///
	else if (vCommand == "subscribeToMessage")
	{
		subscribeToMessage(vParams["id"], createCommand(vParams["command"]),
			vParams.get("subscriptionId", ""));
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### unsubscribeFromMessage()
	/// Unninds the command handler from the event.
	///	  * subscriptionId [str] - subscription identifier.
	///
	else if (vCommand == "unsubscribeFromMessage")
	{
		unsubscribeFromMessage(vParams["subscriptionId"]);
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### sendMessage()
	/// Sends the message.
	///	  * id [str] - message identifier.
	///	  * data [dict] - message parameters.
	///
	else if (vCommand == "sendMessage")
	{
		sendMessage(vParams["id"], vParams.get("data", Dictionary()));
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### getMemoryInfo()
	/// Returns information about current memory allocations.
	///
	else if (vCommand == "getMemoryInfo")
	{
		auto memInfo = getMemoryInfo();
		Dictionary vData;
		vData.put("size", memInfo.nCurrSize);
		vData.put("totalSize", memInfo.nTotalSize);
		vData.put("count", memInfo.nCurrCount);
		vData.put("totalCount", memInfo.nTotalCount);
		return vData;
	}

	///
	/// @fn Variant WinService::execute()
	///
	/// ##### saveCatalogData()
	/// Saves certain catalog branch to specified stream.
	///   * stream - target writable stream.
	///   * path - target path to file (if stream is not specified).
	///   * dataPath - path in the global catalog.
	///
	else if (vCommand == "saveCatalogData")
	{
		ObjPtr<io::IRawWritableStream> pStream;
		if (vParams.has("stream"))
			pStream = queryInterface<io::IRawWritableStream>(vParams["stream"]);
		else
			pStream = queryInterface<io::IRawWritableStream>(io::createFileStream(vParams["path"].convertToPath(),
				io::FileMode::Write | io::FileMode::Truncate | io::FileMode::CreatePath));
		variant::serialize(pStream, getCatalogData(vParams["dataPath"]));
		pStream->flush();
		return true;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### merge()
	/// Merges the Variants. ICommand adapter of Variant::merge().
	///	  * args [seq] - merged data. args[0] is a destination. args[1] and next are sources.
	///	  * mode [int, opt] - MergeMode flags. Default value: MergeMode::All.
	///	  * cloneDst [bool, opt] - enable/disable cloning of a destination (default: true).
	///
	/// It consistently merges vDst(args[0]) with args[1], args[2], etc. And returns the result.
	///
	else if (vCommand == "merge")
	{
		auto vArgs = vParams.get("args", nullptr);
		if (!vArgs.isSequenceLike() || vArgs.isEmpty())
			error::InvalidArgument(SL, "The <args> parameter is absent or invalid").throwException();
		
		Variant::MergeMode eMode = vParams.get("mode", Variant::MergeMode::All);
		bool fCloneDst = vParams.get("mode", true);

		Variant vDst = vArgs[0];
		if (fCloneDst)
			vDst = vDst.clone();

		Size nArgsSize = vArgs.getSize();
		for (Size i = 1; i < nArgsSize; ++i)
		{
			Variant vSrc = vArgs[i];
			vDst.merge(vSrc, eMode);
		}

		return vDst;
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### loadObject()
	/// Loads a Variant from the specified file.
	///	  * path [str] - name of target file.
	///	  * default - returned value if file is absent or invalid.
	///
	/// If 'default' is not specified and error is occurred the exception is thrown.
	///
	else if (vCommand == "loadObject")
	{
		auto optvFileName = vParams.getSafe("path");
		if (!optvFileName.has_value())
			error::InvalidArgument(SL, "The <path> parameter is absent or invalid").throwException();

		Sequence clPaths;
		if (optvFileName->isSequenceLike())
			clPaths = Sequence(optvFileName.value());
		else if (optvFileName->isString())
			clPaths.push_back(optvFileName.value());
		else
			error::InvalidArgument(SL, "The <path> parameter has invalid type").throwException();

		auto optvDefault = vParams.getSafe("default");
		bool fFileFound = false;

		for (auto vPath : clPaths)
		{
			std::filesystem::path pathFile(vPath);

			if (!std::filesystem::exists(pathFile) || !std::filesystem::is_regular_file(pathFile))
				continue;
			fFileFound = true;

			// Load file
			try
			{
				auto pStream = io::createFileStream(pathFile, 
					io::FileMode::Read | io::FileMode::ShareRead | io::FileMode::ShareWrite);
				return variant::deserializeFromJson(pStream);
			}
			catch (error::Exception& ex)
			{
				ex.log(SL, FMT("Can't load specified file <" << pathFile << ">"));
			}
		}
	
		if (optvDefault.has_value())
		return *optvDefault;
	
		if (fFileFound)
			error::InvalidFormat(SL, FMT("All specified files have invalid format <" << clPaths[0] << ">")).throwException();

		error::NotFound(SL, FMT("All specified files are not found <" << clPaths[0] << ">")).throwException();
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### loadStreamAsString()
	/// Reads all data from a stream as a string value.
	///	  * stream [obj] - a stream object.
	///	  * default [str, opt] - a value will be returned if the stream is invalid or
	///		read error is occured.
	///
	/// If 'default' parameter is omitted, the exception is thrown.
	///
	else if (vCommand == "loadStreamAsString")
	{
		auto optStream = vParams.getSafe("stream");
		if (!optStream.has_value())
			error::InvalidArgument(SL, "The <stream> parameter is missing or invalid").throwException();
		
		CMD_TRY
		{
			auto pReadStream = queryInterface<io::IReadableStream>(optStream.value());
			auto nIoSize = pReadStream->getSize();
			
			// IReadableStream uses IoSize, but std::string uses size_t
			// In x86 IoSize is large than size_t
			if(nIoSize > SIZE_MAX)
				error::LimitExceeded(SL, FMT("Stream size is too much. size = " << nIoSize)).throwException();
			size_t nSize = (size_t)nIoSize;

			std::string sData;
			sData.resize(nSize, 0);
			pReadStream->read(sData.data(), nSize);
			return sData;
		}
		CMD_PREPARE_CATCH
		catch (const error::Exception& ex)
		{
			LOGLVL(Trace, "Failed to load a stream as a string (" << ex.what() << ")");
			auto optDefault = vParams.getSafe("default");
			if (optDefault.has_value())
				return optDefault.value();
			throw;
		}	
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### getMle()
	/// Returns middle level event.
	///	  * lle [int] - LLE identifier.
	///	  * mleId [obj] - unique MLE external identifier.
	///
	else if (vCommand == "getMle")
	{
		if (vParams.get("mleId", nullptr) == nullptr)
			return createMle(vParams.get("lle", Event::LLE_NONE));
		return createMle(vParams.get("lle", Event::LLE_NONE), vParams["mleId"]);
	}

	///
	/// @fn Variant Application::execute()
	///
	/// ##### getTickTime()
	/// Returns steady clock time in milliseconds.
	/// This value is compatible with event["tickTime"].
	///
	else if (vCommand == "getTickTime")
	{
		return getTickCount();
	}


	return m_pAppMode->execute(this, vCommand, vParams);
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // cmd
// @}
