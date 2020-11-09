//
// edrav2.libprocmon
// 
// Author: Denis Kroshin (20.04.2019)
// Reviewer: Denis Bogdanov (13.05.2019)
//
///
/// @file Process Monitor Controller implementation
///
///
/// @addtogroup procmon Process Monitor library
/// @{
#include "pch.h"
#include "controller.h"

// madCodeHook internal function
void WINAPI EnableAllPrivileges(void);

#undef CMD_COMPONENT
#define CMD_COMPONENT "procmon"

namespace openEdr {
namespace win {

//
//
//
ProcessMonitorController::ProcessMonitorController()
{
	std::srand((unsigned int)std::time(0));
}

//
//
//
ProcessMonitorController::~ProcessMonitorController()
{
}

//
//
//
void ProcessMonitorController::finalConstruct(Variant vConfig)
{
	CHECK_IN_SOURCE_LOCATION();
	m_pReceiver = queryInterfaceSafe<IDataReceiver>(vConfig.get("receiver", {}));
	m_nTimeout = vConfig.get("timeout", c_nDefaultTimeout);

	InitializeMadCHook();

	if (m_pReceiver)
	{
		m_fInjectOnStart = vConfig.get("injectAllOnStart", m_fInjectOnStart);
		m_fUninjectOnExit = vConfig.get("uninjectAllOnExit", m_fUninjectOnExit);
		m_vInjectionConfig = vConfig.get("injectionConfig", Dictionary({}));
		// FIXME: Do we need this parameter now?
		m_sTargetProcessName = vConfig.get("targetProcessName", L"");

		TRACE_BEGIN
		m_vEventSchema = variant::deserializeFromJson(edrpm::c_sEventSchema);
		m_vConfigSchema = variant::deserializeFromJson(edrpm::c_sConfigSchema);
		TRACE_END("Can't deserialize schema");

		CHECK_IN_SOURCE_LOCATION();
		if (!CreateIpcQueue(edrpm::c_sIpcEventsPort, ipcEventsCallback, this, 1))
			error::RuntimeError("Can't create IPC channel").throwException();
		if (!CreateIpcQueue(edrpm::c_sIpcErrorsPort, ipcErrorsCallback, this, 1))
			error::RuntimeError("Can't create IPC channel").throwException();

		CHECK_IN_SOURCE_LOCATION();
		m_hGlobalEvent.reset(OpenGlobalEvent(edrpm::c_sGlobalCaptureEvent));
		if (m_hGlobalEvent)
			ResetEvent(m_hGlobalEvent);
		else
			m_hGlobalEvent.reset(CreateGlobalEvent(edrpm::c_sGlobalCaptureEvent, TRUE, FALSE));
		if (!m_hGlobalEvent)
			error::RuntimeError("Fail to create global handle").throwException();
	}

	// Setup directories
	m_sSystemDir = getCatalogData("os.systemDir");
#ifdef _WIN64
	m_sSyswowDir = getCatalogData("os.syswowDir");
	if (!std::filesystem::exists(m_sSystemDir / c_sInjDll64) ||
		!std::filesystem::exists(m_sSyswowDir / c_sInjDll32))
	{
		LOGWRN("Injection DLLs not found in system directory. Use default directory.");
		m_sSystemDir = getCatalogData("app.imagePath", ".");
		m_sSyswowDir = m_sSystemDir;
	}
#else
	if (!std::filesystem::exists(m_sSystemDir / c_sInjDll32))
	{
		LOGWRN("Injection DLLs not found in system directory. Use default directory.");
		m_sSystemDir = getCatalogData("app.imagePath", ".");
	}
#endif
}

//
// FIXME: Maybe it is better to create universal function looking specified dll?
//
bool ProcessMonitorController::isDllLoadedIntoProcess(const uint32_t dwPid, const std::wstring_view sDllName)
{
	sys::win::ScopedFileHandle hModuleSnap;
	do
	{
		hModuleSnap.reset(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwPid));
	} while (hModuleSnap == INVALID_HANDLE_VALUE && GetLastError() == ERROR_BAD_LENGTH);
	if (hModuleSnap == INVALID_HANDLE_VALUE)
		return false;

	MODULEENTRY32 me32;
	me32.dwSize = sizeof(me32);
	if (Module32First(hModuleSnap, &me32))
	{
		do
		{
			if (sDllName == me32.szModule)
				return true;
		} while (Module32Next(hModuleSnap, &me32));
	}
	return false;
}

//
//
//
bool ProcessMonitorController::jectProcess(bool fInject, const uint32_t nPid)
{
	sys::win::ScopedHandle hTarget(::OpenProcess(PROCESS_ALL_ACCESS, false, nPid));
	if (!hTarget)
	{
		LOGLVL(Debug, "Can't open target process (pid <" << nPid << ">)");
		return false;
	}

	if (sys::win::isCritical(hTarget))
	{
		LOGLVL(Debug, "Skip critical process (pid <" << nPid << ">)");
		return true;
	}

#ifdef _WIN64
	bool fIs64Bit = Is64bitProcess(hTarget);
	std::filesystem::path sLibName = fIs64Bit ? c_sInjDll64 : c_sInjDll32;
	std::filesystem::path sLibPath = fIs64Bit ? (m_sSystemDir / sLibName) : (m_sSyswowDir / sLibName);
#else
	std::filesystem::path sLibName = c_sInjDll32;
	std::filesystem::path sLibPath = m_sSystemDir / sLibName;
#endif

	DWORD nTimeout = sys::win::isSuspended(hTarget) ? 500 : 5000;

	if (fInject)
	{
		if (InjectLibrary(sLibPath.c_str(), hTarget, nTimeout))
		{
			if (isDllLoadedIntoProcess(nPid, sLibName.native()))
				LOGLVL(Debug, "Library <" << sLibName.u8string() << "> is injected to process (pid <" << nPid << ">)");
			else
				LOGLVL(Debug, "Library <" << sLibName.u8string() << "> is injected to process but not found (pid <" << nPid << ">)");
		}
		else
		{
			LOGWRN("Can't inject library <" << sLibName.u8string() << "> to process (pid <" << nPid << ">)");
			return false;
		}
	}
	else
	{
		if (isDllLoadedIntoProcess(nPid, sLibName.native()))
		{
			if (UninjectLibrary(sLibPath.c_str(), hTarget, nTimeout))
				LOGLVL(Debug, "Library <" << sLibName.u8string() << "> is uninjected from process (pid <" << nPid << ">)");
			else
			{
				LOGWRN("Can't uninject library <" << sLibName.u8string() << "> from process (pid <" << nPid << ">)");
				return false;
			}
		}
		else
			LOGLVL(Debug, "Library <" << sLibName.u8string() << "> is not injected into process (pid <" << nPid << ">)");
	}
	return true;
}

//
//
//
BOOL WINAPI ProcessMonitorController::injectionCallback(PVOID pContext, DWORD dwProcessId, DWORD dwParentId,
	DWORD dwSessionId, DWORD dwFlags, LPCWSTR pProcessImagePath, LPCWSTR pProcessCommandLine)
{
	UNREFERENCED_PARAMETER(pContext);
	UNREFERENCED_PARAMETER(dwParentId);
	UNREFERENCED_PARAMETER(dwSessionId);
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(pProcessImagePath);
	UNREFERENCED_PARAMETER(pProcessCommandLine);

	auto pThis = (ProcessMonitorController*)pContext;
	if (pThis != NULL && !pThis->m_fAllowInjection)
		return false;

	sys::win::ScopedHandle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId));
	if (!hProcess)
		hProcess.reset(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId));
	if (hProcess && sys::win::isCritical(hProcess))
		return false;

	return true;
}

//
//
//
void ProcessMonitorController::jectAll(const bool fInject, const std::wstring& sIncludeMask, const std::wstring& sExcludeMask)
{
	LOGLVL(Detailed, (fInject ?
		"Start to inject to all existing processes" :
		"Start to uninject from all existing processes"));

	static const uint32_t c_nFlags = INJECT_SYSTEM_PROCESSES | INJECT_METRO_APPS;

	const wchar_t* pIncludeMask = sIncludeMask.empty() ? NULL : sIncludeMask.c_str();
	const wchar_t* pExcludeDef = fInject ? L"smss.exe|csrss.exe|wininit.exe" : NULL;
	const wchar_t* pExcludeMask = sExcludeMask.empty() ? pExcludeDef : sExcludeMask.c_str();
	const ULONG pPids[] = { ::GetCurrentProcessId(), 0 };

	if (fInject)
	{
#ifdef _WIN64
		InjectLibrarySystemWideW(NULL, (m_sSystemDir / c_sInjDll64).c_str(), ALL_SESSIONS, c_nFlags,
			pIncludeMask, pExcludeMask, PULONG(pPids), &injectionCallback, this, DWORD(m_nTimeout));
		InjectLibrarySystemWideW(NULL, (m_sSyswowDir / c_sInjDll32).c_str(), ALL_SESSIONS, c_nFlags,
			pIncludeMask, pExcludeMask, PULONG(pPids), &injectionCallback, this, DWORD(m_nTimeout));
#else
		InjectLibrarySystemWideW(NULL, (m_sSystemDir / c_sInjDll32).c_str(), ALL_SESSIONS, c_nFlags,
			pIncludeMask, pExcludeMask, PULONG(pPids), &injectionCallback, this, DWORD(m_nTimeout));
#endif
	}
	else
	{
#ifdef _WIN64
		UninjectLibrarySystemWideW(NULL, (m_sSystemDir / c_sInjDll64).c_str(), ALL_SESSIONS, c_nFlags,
			pIncludeMask, pExcludeMask, NULL, DWORD(m_nTimeout));
		UninjectLibrarySystemWideW(NULL, (m_sSyswowDir / c_sInjDll32).c_str(), ALL_SESSIONS, c_nFlags,
			pIncludeMask, pExcludeMask, NULL, DWORD(m_nTimeout));
#else
		UninjectLibrarySystemWideW(NULL, (m_sSystemDir / c_sInjDll32).c_str(), ALL_SESSIONS, c_nFlags,
			pIncludeMask, pExcludeMask, NULL, DWORD(m_nTimeout));
#endif
	}

	LOGLVL(Detailed, (fInject ? 
		"Finish to inject to all existing processes" : 
		"Finish to uninject from all existing processes"));
}

Event g_pEventMap[] = {
	Event::LLE_PROCESS_MEMORY_READ,		// PROCMON_PROCESS_MEMORY_READ = 0x0000,
	Event::LLE_PROCESS_MEMORY_WRITE,	// PROCMON_PROCESS_MEMORY_WRITE = 0x0001,
	Event::LLE_NONE,					// PROCMON_API_SET_WINDOWS_HOOK = 0x0002,
	Event::LLE_NONE,					// PROCMON_STORAGE_RAW_ACCESS_WRITE = 0x0003,
	Event::LLE_NONE,					// PROCMON_STORAGE_RAW_LINK_CREATE = 0x0004,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_GET_KEYBOARD_STATE = 0x0005,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_GET_KEY_STATE = 0x0006,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_REGISTER_HOT_KEY = 0x0007,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_REGISTER_RAW_INPUT_DEVICES = 0x0008,
	Event::LLE_KEYBOARD_BLOCK,			// PROCMON_API_BLOCK_INPUT = 0x0009,
	Event::LLE_KEYBOARD_BLOCK,			// PROCMON_API_ENABLE_WINDOW = 0x000A,
	Event::LLE_CLIPBOARD_READ,			// PROCMON_API_GET_CLIPBOARD_DATA = 0x000B,
	Event::LLE_CLIPBOARD_READ,			// PROCMON_API_SET_CLIPBOARD_VIEWER = 0x000C,
	Event::LLE_KEYBOARD_GLOBAL_WRITE,	// PROCMON_API_SEND_INPUT = 0x000D,
	Event::LLE_KEYBOARD_GLOBAL_WRITE,	// PROCMON_API_KEYBD_EVENT = 0x000E,
	Event::LLE_MICROPHONE_ENUM,			// PROCMON_API_ENUM_AUDIO_ENDPOINTS = 0x000F,
	Event::LLE_MICROPHONE_READ,			// PROCMON_API_WAVE_IN_OPEN = 0x0010,
	Event::LLE_MOUSE_GLOBAL_WRITE,		// PROCMON_API_MOUSE_EVENT = 0x0011,
	Event::LLE_WINDOW_DATA_READ,		// PROCMON_COPY_WINDOW_BITMAP = 0x0012,
	Event::LLE_DESKTOP_WALLPAPER_SET,	// PROCMON_DESKTOP_WALLPAPER_SET = 0x0013,
	Event::LLE_MOUSE_BLOCK,				// PROCMON_API_CLIP_CURSOR = 0x0014,
	Event::LLE_USER_LOGON,				// PROCMON_INTERACTIVE_LOGON = 0x0015,
	Event::LLE_USER_IMPERSONATION,		// PROCMON_THREAD_IMPERSONATION = 0x0016,
	Event::LLE_USER_IMPERSONATION,		// PROCMON_PIPE_IMPERSONATION = 0x0017,
};

static_assert(std::size(g_pEventMap) == Size(edrpm::RawEvent::_Max), "Not enough items in the table <g_pEventMap>");

//
//
//
void ProcessMonitorController::parseEvent(edrpm::RawEvent eEvent, Variant vEvent)
{
	auto nEvent = uint32_t(eEvent);
	if (nEvent >= std::size(g_pEventMap))
	{
		LOGLVL(Trace, vEvent.print());
		error::InvalidArgument(SL, FMT("There is no mapping for event <" << nEvent << ">")).throwException();
	}

	vEvent.put("baseType", g_pEventMap[nEvent]);

	switch (eEvent)
	{
		case edrpm::RawEvent::PROCMON_API_SET_WINDOWS_HOOK:
		{
			std::string sHookType = vEvent["hookType"];
			if (sHookType == "WH_KEYBOARD" || sHookType == "WH_KEYBOARD_LL")
				vEvent.put("baseType", Event::LLE_KEYBOARD_GLOBAL_READ);
			else
				vEvent.put("baseType", Event::LLE_WINDOW_PROC_GLOBAL_HOOK);
			break;
		}
		case edrpm::RawEvent::PROCMON_STORAGE_RAW_ACCESS_WRITE:
		{
			uint32_t nObjectType = vEvent["objectType"];
			if (nObjectType == 0)
				vEvent.put("baseType", Event::LLE_DISK_RAW_WRITE_ACCESS);
			else if (nObjectType == 1)
				vEvent.put("baseType", Event::LLE_VOLUME_RAW_WRITE_ACCESS);
			else if (nObjectType == 2)
				vEvent.put("baseType", Event::LLE_DEVICE_RAW_WRITE_ACCESS);
			else
				error::InvalidFormat(SL, FMT("Fail to parse event <" << uint32_t(eEvent) << ">")).throwException();
			break;
		}
		case edrpm::RawEvent::PROCMON_STORAGE_RAW_LINK_CREATE:
		{
			uint32_t nObjectType = vEvent["objectType"];
			if (nObjectType == 0)
				vEvent.put("baseType", Event::LLE_DISK_LINK_CREATE);
			else if (nObjectType == 1)
				vEvent.put("baseType", Event::LLE_VOLUME_LINK_CREATE);
			else if (nObjectType == 2)
				vEvent.put("baseType", Event::LLE_DEVICE_LINK_CREATE);
			else
				error::InvalidFormat(SL, FMT("Fail to parse event <" << uint32_t(eEvent) << ">")).throwException();
			break;
		}
		case edrpm::RawEvent::PROCMON_THREAD_IMPERSONATION:
		{
			vEvent.put("impersonationType", sys::win::ImpersonationType::Local);
			break;
		}
		case edrpm::RawEvent::PROCMON_PIPE_IMPERSONATION:
		{
			vEvent.put("impersonationType", sys::win::ImpersonationType::Ipc);
			break;
		}
	}
}

//
//
//
void WINAPI ProcessMonitorController::ipcEventsCallback(const char*, const void* pMessageBuf, 
	unsigned long dwMessageLen, void* pAnswerBuf, unsigned long dwAnswerLen, void* pContext)
{

	edrpm::RawEvent nRawEventId = edrpm::RawEvent::_Max;
	CMD_TRY
	{
		ProcessMonitorController* pThis = (ProcessMonitorController*)pContext;
		if (pThis == nullptr)
			error::InvalidArgument(SL, "Invalid this pointer").throwException();
		if (pMessageBuf == nullptr)
			error::InvalidArgument(SL, "Invalid incoming buffer for IPC command").throwException();

		Variant vEvent = variant::deserializeFromLbvs(pMessageBuf, dwMessageLen, pThis->m_vEventSchema);
		nRawEventId = vEvent["rawEventId"];

		if (nRawEventId == edrpm::RawEvent::PROCMON_LOG_MESSAGE)
		{
#undef CMD_COMPONENT
#define CMD_COMPONENT "inject"
			uint32_t nPid = getByPath(vEvent, "process.pid", 0);
			std::string sModule = vEvent.get("path", "");
			std::string sMsg = std::string(vEvent["errMsg"]) + " (pid <" + 
				std::to_string(nPid) + ">, name (" + sModule + ">)";

			edrpm::ErrorType eType(vEvent["errType"]);
			switch (eType)
			{
			case edrpm::ErrorType::Error:
				error::RuntimeError(SL, sMsg).log();
				break;
			case edrpm::ErrorType::Warning:
				LOGWRN(sMsg);
				break;
			case edrpm::ErrorType::Info:
				LOGLVL(Debug, sMsg);
				break;
			default:
				LOGWRN("Unknown message type. <" << sMsg << ">");
			}
			return;
#undef CMD_COMPONENT
#define CMD_COMPONENT "procmon"
		}
		if (nRawEventId == edrpm::RawEvent::PROCMON_INJECTION_CONFIG_UPDATE)
		{
			if (pAnswerBuf == nullptr || dwAnswerLen < sizeof(variant::lbvs::LbvsHeader))
				error::InvalidArgument(SL, "Invalid answer buffer for IPC command").throwException();

			std::vector<uint8_t> pLbvs;
			if (!variant::serializeToLbvs(pThis->m_vInjectionConfig, pThis->m_vConfigSchema, pLbvs))
				error::InvalidFormat(SL, "Can't serialize injection config to LBVS").throwException();

			memcpy(pAnswerBuf, pLbvs.data(), std::min<size_t>(dwAnswerLen, pLbvs.size()));
			return;
		}

		// Drop events if service is stopped
		if (!pThis->m_fInitialized)
			return;

		LOGLVL(Trace, "Parse raw event <" << size_t(nRawEventId) <<
			"> from process <" << getByPath(vEvent, "process.pid", -1) << ">");

		vEvent.put("rawEventId", createRaw(c_nClassId, (uint32_t)nRawEventId));
		pThis->parseEvent(nRawEventId, vEvent);

		// Send message to receiver
		if (!pThis->m_pReceiver)
			error::InvalidArgument(SL, "Receiver interface is undefined").throwException();
		pThis->m_pReceiver->put(vEvent);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("System monitor fail to parse event <" << uint32_t(nRawEventId) << ">"));
		LOGLVL(Trace, string::convertToHex((Byte*)pMessageBuf, (Byte*)pMessageBuf + dwMessageLen));
	}
}

//
// TODO: Remove this callback in new releases
//
void WINAPI ProcessMonitorController::ipcErrorsCallback(const char*, const void* pMessageBuf, unsigned long dwMessageLen, void*, unsigned long, void* pContext)
{
	CMD_TRY
	{
		if (pMessageBuf == nullptr || dwMessageLen < 2)
			error::InvalidArgument(SL, "Invalid incoming buffer for IPC command").throwException();

		ProcessMonitorController* pThis = (ProcessMonitorController*)pContext;
		if (pThis == nullptr)
			error::InvalidArgument(SL, "Invalid this pointer").throwException();

		uint8_t nType = 'E';
		char* sMsg = (char*)pMessageBuf;
		if (sMsg[dwMessageLen - 2] == '#')
		{
			nType = sMsg[dwMessageLen - 1];
			dwMessageLen -= 2;
		}

#undef CMD_COMPONENT
#define CMD_COMPONENT "inject"
		if (nType == 'E')
			error::RuntimeError(SL, std::string(sMsg, sMsg + dwMessageLen)).log();
		else if (nType == 'W')
			LOGWRN(std::string(sMsg, sMsg + dwMessageLen));
		else if (nType == 'I')
			LOGLVL(Debug, std::string(sMsg, sMsg + dwMessageLen));
		else
			LOGWRN("Unknown message type. <" << std::string(sMsg, sMsg + dwMessageLen) << ">");
#undef CMD_COMPONENT
#define CMD_COMPONENT "procmon"
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, "System monitor fail to log error");
	}
	
}

//
//
//
void ProcessMonitorController::copyInjectionDll(const std::wstring& sDllName, const GUID guidFolder)
{
	namespace fs = std::filesystem;
	auto sSrcFile = fs::path(getCatalogData("app.imagePath")) / sDllName;
	if (fs::exists(sSrcFile))
	{
		PWSTR pFolder;
		if (::SHGetKnownFolderPath(guidFolder, 0, NULL, &pFolder) != S_OK)
			error::win::WinApiError(SL, "Fail to get path of system directory").throwException();
		auto sDestFile = fs::path(pFolder) / sDllName;
		::CoTaskMemFree(pFolder);

		std::error_code ec;
		if (fs::copy_file(sSrcFile, sDestFile, fs::copy_options::overwrite_existing, ec))
			return;

		// Try to uninject dll from all processes
		jectAll(false);

		auto sDelFile = sDestFile;
		sDelFile.replace_extension(std::to_wstring(std::rand()) + L".bak");
		fs::rename(sDestFile, sDelFile); // Throw exception if fail
		fs::copy(sSrcFile, sDestFile);
		fs::remove(sDelFile, ec);
		if (ec)
		{
			LOGWRN("Fail remove file <" << sDestFile.u8string() << ">, rename it to <" << sDelFile.u8string() << ">");
			// Register delete-on-reboot
			if (!MoveFileExW(sDelFile.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
				error::win::WinApiError(SL, FMT("Fail to delayed move file <" << sDelFile.u8string() << ">")).log();
		}
	}
	else
		error::InvalidArgument(SL, FMT("Injection DLL <" << sSrcFile.u8string() << "> is not found")).throwException();
}

//
//
//
void ProcessMonitorController::removeInjectionDll(const std::wstring& sDllName, const GUID guidFolder)
{
	namespace fs = std::filesystem;

	PWSTR pFolder;
	if (::SHGetKnownFolderPath(guidFolder, 0, NULL, &pFolder) != S_OK)
		error::win::WinApiError(SL, "Fail to get path of system directory").throwException();
	auto sDestFile = fs::path(pFolder) / sDllName;
	::CoTaskMemFree(pFolder);

	std::error_code ec;
	if (!fs::exists(sDestFile) || fs::remove(sDestFile, ec))
		return;
	
	// Try to uninject dll from all processes
	jectAll(false);

	auto sDelFile = sDestFile;
	sDelFile.replace_extension(std::to_wstring(std::rand()) + L".bak");
	fs::rename(sDestFile, sDelFile); // Throw exception if fail
	fs::remove(sDelFile, ec);
	if (ec)
	{
		LOGWRN("Fail remove file <" << sDestFile.u8string() << ">, rename it to <" << sDelFile.u8string() << ">");
		// Register delete-on-reboot
		if (!MoveFileExW(sDelFile.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
			error::win::WinApiError(SL, FMT("Fail to delayed move file <" << sDelFile.u8string() << ">")).log();
	}
}

//
//
//
void ProcessMonitorController::install(Variant vParams)
{
	if (m_hGlobalEvent && !::ResetEvent(m_hGlobalEvent))
		error::win::WinApiError(SL, "Fail to reset global event").log();
	m_fAllowInjection = true;

	if (vParams.get("useSystemDir", true))
	{
#ifdef _WIN64
		copyInjectionDll(c_sInjDll64, FOLDERID_System);
		copyInjectionDll(c_sInjDll32, FOLDERID_SystemX86);
#else
		copyInjectionDll(c_sInjDll32, FOLDERID_System);
#endif
	}
	else
	{
		std::filesystem::path sBinaryDir(getCatalogData("app.imagePath"));
		if (!std::filesystem::exists(sBinaryDir / c_sInjDll32))
			error::InvalidArgument(SL, FMT("Injection DLL <" << 
			(sBinaryDir / c_sInjDll32).u8string() << "> is not found")).throwException();
#ifdef _WIN64
		if (!std::filesystem::exists(sBinaryDir / c_sInjDll64))
			error::InvalidArgument(SL, FMT("Injection DLL <" <<
			(sBinaryDir / c_sInjDll64).u8string() << "> is not found")).throwException();
#endif
	}
}

//
//
//
void ProcessMonitorController::uninstall(Variant vParams)
{
	if (m_hGlobalEvent && !::ResetEvent(m_hGlobalEvent))
		error::win::WinApiError(SL, "Fail to reset global event").log();
	m_fAllowInjection = true;

#ifdef _WIN64
	removeInjectionDll(c_sInjDll64, FOLDERID_System);
	removeInjectionDll(c_sInjDll32, FOLDERID_SystemX86);
#else
	removeInjectionDll(c_sInjDll32, FOLDERID_System);
#endif

}

//
//
//
void ProcessMonitorController::loadState(Variant vState)
{
}

//
//
//
openEdr::Variant ProcessMonitorController::saveState()
{
	return {};
}

//
//
//
void ProcessMonitorController::start()
{
	start({});
}

//
//
//
bool ProcessMonitorController::start(Variant vParams)
{
	LOGLVL(Detailed, "ProcMon controller is being started");

	std::scoped_lock _lock(m_mtxStartStop);
	if (m_fInitialized)
	{
		LOGLVL(Detailed, "ProcMon controller already started");
		return false;
	}

	if (!::SetEvent(m_hGlobalEvent))
		error::win::WinApiError(SL, "Fail to set global event").throwException();
	m_fInitialized = true;
	m_fAllowInjection = true;

	if (m_fInjectOnStart)
		m_pInjectionThread = std::thread([this]() 
		{ 
			this->jectAll(true);
		});
	LOGLVL(Detailed, "ProcMon controller is started");
	return true;
}

//
//
//
void ProcessMonitorController::stop()
{
	stop({});
}

//
//
//
bool ProcessMonitorController::stop(Variant vParams)
{
	LOGLVL(Detailed, "ProcMon controller is being stopped");

	std::scoped_lock _lock(m_mtxStartStop);
	if (!m_fInitialized)
	{
		LOGLVL(Detailed, "ProcMon controller already stopped");
		return false;
	}

	if (!::ResetEvent(m_hGlobalEvent))
		error::win::WinApiError(SL, "Fail to reset global event").log();

	m_fInitialized = false;
	m_fAllowInjection = false;
	if (m_pInjectionThread.joinable())
		m_pInjectionThread.join();

	LOGLVL(Detailed, "ProcMon controller is stopped");
	return true;
}

//
//
//
void ProcessMonitorController::shutdown()
{
	LOGLVL(Detailed, "ProcMon controller is being shutdowned");
	if (m_hGlobalEvent && !::ResetEvent(m_hGlobalEvent))
		error::win::WinApiError(SL, "Fail to reset global event").log();
	if (m_fUninjectOnExit)
		jectAll(false);

	LOGLVL(Detailed, "Finalize madCodeHook");
	// FinalizeMadCHook doesn't close IPC ports
	DestroyIpcQueue(edrpm::c_sIpcEventsPort);
	DestroyIpcQueue(edrpm::c_sIpcErrorsPort);
	FinalizeMadCHook();

	LOGLVL(Detailed, "ProcMon controller is shutdowned");
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * 'objects.ProcessMonitorController'
///
/// #### Supported commands
///
Variant ProcessMonitorController::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant ProcessMonitorController::execute()
	///
	/// ##### install()
	/// Installs the injection DLLs into the operating system.
	///   * useSystemDir [bool] - copy/delete files to/from system directory.
	///
	if (vCommand == "install")
	{
		install(vParams);
		return true;
	}

	///
	/// @fn Variant ProcessMonitorController::execute()
	///
	/// ##### uninstall()
	/// Uninstalls the injection DLLs.
	///
	else if (vCommand == "uninstall")
	{
		uninstall(vParams);
		return true;
	}

	///
	/// @fn Variant ProcessMonitorController::execute()
	///
	/// ##### start()
	/// Starts the service.
	/// Returns a status of the operation.
	///
	if (vCommand == "start")
		return start(vParams);

	///
	/// @fn Variant ProcessMonitorController::execute()
	///
	/// ##### stop()
	/// Stops the service.
	/// Returns a status of the operation.
	///
	if (vCommand == "stop")
		return stop(vParams);

	///
	/// @fn Variant ProcessMonitorController::execute()
	///
	/// ##### inject() / uninject()
	/// Inject/uninject DLL into/from process.
	///   * pid [int] - pid of process.
	///   * procName [str] - name of processes.
	/// Returns a status of the operation.
	///
	if (vCommand == "inject" || vCommand == "uninject")
	{
		m_fAllowInjection = true;
		bool fInject = (vCommand == "inject");
		uint32_t dwPid = vParams.get("pid", UINT_MAX);
		if (dwPid == UINT_MAX)
			jectAll(fInject, vParams.get("procName", ""));
		else
			jectProcess(fInject, dwPid);
		return true;
	}

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("ProcessMonitorController doesn't support a command <"
		<< vCommand << ">")).throwException();
}

} // namespace win
} // namespace openEdr

/// @}
