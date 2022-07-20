//
// edrav2.libprocmon
// 
// Author: Denis Kroshin (20.04.2019)
// Reviewer: Denis Bogdanov (13.05.2019)
//
///
/// @file Process Monitor Controller declaration
///
/// @addtogroup procmon Process Monitor library
/// @{
#pragma once
#include "objects.h"
#include "../../libsysmon/inc/FltPortReceiver.h"
#include "../../libsysmon/inc/edrdrvapi.hpp"

namespace cmd {
namespace win {

///
/// Service that receive raw events from DLL injected to processes and handle them.
///
class ProcessMonitorController : 
	public ObjectBase <CLSID_ProcessMonitorController>, 
	public ICommandProcessor,
	public IService
{
private:
	static const Time c_nDefaultTimeout = 2000; // 2 sec
	inline static const wchar_t c_sPortName[] = L"" CMD_EDRDRV_FLTPORT_PROCMON_OUT_NAME;
	/// Number of threads to process events from driver.
	static const size_t c_nTreadsCount = 1;

	ObjPtr<IDataReceiver> m_pReceiver;
	Time m_nTimeout = c_nDefaultTimeout;
	bool m_fInjectOnStart = false;
	bool m_fUninjectOnExit = false;

	sys::win::ScopedHandle m_hGlobalEvent;
	Variant m_vEventSchema;
	Variant m_vConfigSchema;
	Variant m_vInjectionConfig;
	std::wstring m_sTargetProcessName;
	std::filesystem::path m_sSystemDir;
#ifdef _WIN64
	std::filesystem::path m_sSyswowDir;
#endif

	std::mutex m_mtxStartStop;
	std::atomic_bool m_fInitialized = false;
	std::atomic_bool m_fAllowInjection = false;
	std::thread m_pInjectionThread;
	cmd::win::FltPortReceiver m_hFltPortReceiver;

	// Check if DLL loaded into process
	bool isDllLoadedIntoProcess(const uint32_t dwPid, const std::wstring_view sDllName);

	// Without Madchook we using injection from our driver, no need to inject from usermode
#if defined(FEATURE_ENABLE_MADCHOOK)
	bool injectProcess(bool fInject, const uint32_t nPid);
	static BOOL WINAPI injectionCallback(PVOID pContext, DWORD dwProcessId, DWORD dwParentId, 
		DWORD dwSessionId, DWORD dwFlags, LPCWSTR pProcessImagePath, LPCWSTR pProcessCommandLine);
	void injectAll(const bool fInject, const std::wstring& sIncludeMask = {}, const std::wstring& sExcludeMask = {});
#endif
	void copyInjectionDll(const std::wstring& sDllName, const GUID guidFolder);
	void removeInjectionDll(const std::wstring& sDllName, const GUID guidFolder);

	//void parseEvent(edrpm::RawEvent eEvent, Variant vEvent);

	void install(Variant vParams);
	void uninstall(Variant vParams);
	bool start(Variant vParams);
	bool stop(Variant vParams);

protected:
	ProcessMonitorController();
	virtual ~ProcessMonitorController() override;

public:

	static void WINAPI ipcEventsCallbackInt(const void* pMessageBuf,
		unsigned long dwMessageLen, void* pAnswerBuf, unsigned long dwAnswerLen,
		void* pContext, bool& fHasAnswer);

	///
	/// MadCHook IPC callback. IPC command port.
	///
	static void WINAPI ipcEventsCallback(const char* pIpc, const void* pMessageBuf,
		unsigned long dwMessageLen, void* pAnswerBuf, unsigned long dwAnswerLen,
		void* pContext);

	static void WINAPI ipcErrorsCallback(const char* pIpc, const void* pMessageBuf,
		unsigned long dwMessageLen, void* pAnswerBuf, unsigned long dwAnswerLen,
		void* pContext);

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - object's configuration including the following fields:
	///   **receiver** - object that receives a parsed data.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

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

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace win
} // namespace cmd

/// @}
