//
// edrav2.libcore
// 
// Author: Denis Kroshin (06.02.2019)
// Reviewer: Podpruzhnikov Yury (18.02.2019)
//
///
/// @file System Monitor Controller declaration
///
/// @addtogroup sysmon System Monitor library
/// @{
#pragma once
#include "objects.h"
#include "FltPortReceiver.h"

namespace cmd {
namespace win {

#define SYSMON_USE_OVERLAPPED

///
/// Service that receive raw events from mini-filter driver and handle them.
///
class SystemMonitorController : public ObjectBase <CLSID_SystemMonitorController>, 
	public ICommandProcessor, 
	public IService
{
private:
	/// Number of threads to process events from driver.
	static const size_t c_nTreadsCount = 2;	

	/// Name of communication port to receive messages.
	inline static const wchar_t c_sPortName[] = L"" CMD_EDRDRV_FLTPORT_NAME;
	
	/// Name of Windows service.
	inline static const wchar_t c_sDrvSrvName[] = L"" CMD_EDRDRV_SERVICE_NAME;

	inline static const wchar_t c_sPCAService[] = L"" CMD_PCA_SERVICE_NAME;
	
	/// Name of mini-filter driver file.
	inline static const wchar_t c_sDriverName[] = L"" CMD_EDRDRV_FILE_NAME;

	/// Names of registry keys and values needed for service registration.
	inline static const wchar_t c_sServiceRegKey[] = L"SYSTEM\\CurrentControlSet\\services";
	inline static const wchar_t c_sSupportedFeatures[] = L"SupportedFeatures";
	inline static const DWORD c_dwSupportedFeaturesValue = 0x03;
	inline static const wchar_t c_sInstancesRegKey[] = L"Instances";
	inline static const wchar_t c_sDefaultInstance[] = L"DefaultInstance";
	inline static const wchar_t c_sDefaultInstanceValue[] = L"edrdrv instance";
	inline static const wchar_t c_sFlags[] = L"Flags";
	inline static const DWORD c_dwFlags = 0;
	inline static const wchar_t c_sAltitude[] = L"Altitude";

	enum class InjectionMode
	{
		None = 0,
		Driver = 1,
		Controller = 2
	};

	bool m_fStopDriverOnShutdown = false;

	//
	ObjPtr<IDataReceiver> m_pReceiver;

	sys::win::ScopedFileHandle m_pIoctl;
	std::locale m_Locale = std::locale("");
	cmd::win::FltPortReceiver m_hFltPortReceiver;

	Variant m_vEventSchema;
	Variant m_vConfigSchema;
	Variant m_vUpdateRulesSchema;
	Variant m_vSetProcessInfoSchema;
	Variant m_vDefaultDriverConfig;
	Variant m_vSelfProtectConfig;
	size_t m_nThreadsCount = 0;
	std::mutex m_mtxStartStop;
	bool m_fInitialized = false;
	bool m_fWasStarted = false;
	std::string m_sStartMode { "auto" };
	uint32_t m_nSelfPid = 0;
	InjectionMode m_eInjection = InjectionMode::None;

	// Driver messages parsing thread functions
	void sendIoctl(uint32_t nCode, void* pInput, Size nInput, void* pOutput, Size nOutput);

	// Deserialize buffer and send data into receiver
#ifdef ENABLE_EVENT_TIMINGS
	bool parseEvent(const Byte* pBuffer, const Size nBufferSize, std::pair<Size, Size>& nTimes);
#else
	bool parseEvent(const Byte* pBuffer, const Size nBufferSize);
#endif

	void install(Variant vParams);
	void uninstall(Variant vParams);
	bool startInt();
	bool stopInt();
	void startThreads();
	void stopThreads();
	void sendStartMonitoring();
	void sendStopMonitoring();

	void sendConfig(Variant vDrvConfig);
	void updateProcessRules(Variant vParams);
	void setProcessInfo(Variant vParams);
	void updateFileRules(Variant vParams);
	void updateRegRules(Variant vParams);

protected:
	/// Constructor.
	SystemMonitorController();
	
	/// Destructor.
	virtual ~SystemMonitorController() override;

public:

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - object's configuration including the following fields:
	///   **receiver** - object that receive parsed data.
	///   **threadsCount** - number of threads.
	///   **stopDriverOnShutdown** [bool, opt] - stop driver on shutdown (default: `false`).
	///   **injectNewProcesses** - inject to new processes (0 = None, 1 = Driver, 2 = Controller)
	///   **driverConfig** - default driver config.
	///   **selfprotectConfig** - default selfprotection config. Dict with fields:
	///     * processRules - sequence of dict. 
	///       Each element contains parameters for the "updateProcessRules" command.
	///     * fileRules - dict with parameters for the "updateFileRules" command.
	///     * regRules - dict with parameters for the "updateRegRules" command.
	///     * curProcessInfo - dict with parameters for the "setProcessInfo" command excepts "pid".
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

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

} // namespace win
} // namespace cmd

/// @}
