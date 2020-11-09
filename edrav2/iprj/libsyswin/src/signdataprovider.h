//
// edrav2.libsyswin project
//
// Signature Data Provider declaration
//
// Author: Denis Kroshin (14.03.2019)
// Reviewer: Denis Bogdanov (01.04.2019) 
//
///
/// @file Signature Data Provider declaration
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once
#include <objects.h>

namespace openEdr {
namespace sys {
namespace win {

static const char g_sServiceName[] = "CryptSvc";

///
/// Signature Data Provider class.
///
class SignDataProvider : public ObjectBase<CLSID_SignDataProvider>,
	public ISignatureInformation,
	public ICommandProcessor,
	public IService
{
private:
	static const Time c_nPurgeTimeout = 24 * 60 * 60 * 1000; // 1 day
	static const Time c_nPurgeCallTimeout = 60 * 60 * 1000; // 1 hour

	std::mutex m_mtxLock;
	std::condition_variable m_cvLock;
	std::unordered_set<std::string> m_pLockQueue;
	std::unordered_map<std::string, Variant> m_pSignInfoMap;

	std::mutex m_mtxStartStop;
	std::atomic_bool m_fInitialized = false;
	io::IoSize m_nMaxStreamSize = g_nMaxStreamSize;
	ThreadPool::TimerContextPtr m_timerPurge;
	Time m_nPurgeTimeout = c_nPurgeTimeout;
	Time m_nPurgeCallTimeout = c_nPurgeCallTimeout;

	std::mutex m_mtxThread;
	std::thread m_pStartSvcThread;
	std::atomic_bool m_fCryptSvcIsRun = true;

	bool m_fIsWin8Plus = false;
	bool m_fDisableNetworAccess = false;

	std::wstring m_sWindowsApps;
	std::wstring m_sWindowsAppsNt;
	std::wstring m_sWindowsAppsVl;

	SignStatus convertSignInfo(LONG winStatus);
	std::wstring getCertificateProvider(HANDLE hWVTStateData);
	SignStatus winVerifyTrust(GUID wt_guid, PWINTRUST_DATA_W8 pWtData, std::wstring& sProvider);
	Variant getEmbeddedSignInfo(const std::wstring& sFileName, HANDLE hFile);
	Variant getCatalogSignInfo(const std::wstring& sFileName, HANDLE hFile, std::wstring sCatName = {});

	bool isUwpApplication(const std::wstring& sFileName);
	bool isUwpAppValid(const std::wstring& sFileName, const std::wstring& sAppPath);
	Variant getUwpSignInfo(const std::wstring& sFileName, const HANDLE hFile);

	Variant getSignatureInfo(Variant vFileName, Variant vSecurity);

	bool purgeItems();
	void startCryptSvc();

protected:
	
	// Constructor
	SignDataProvider();

	// Destructor
	virtual ~SignDataProvider() override;

public:

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **purgeMask** - purge process start every (purgeMask + 1) unique file query.
	///   **purgeTimeout** - guaranteed time file stay in cache after last access.
	///   **minPurgeCheckTimeout** - minimal time between purge actions.
	///   **receiver** - object for pushing data.
	///   **maxFileSize** - max file size to calculate hash
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// ISignatureInformation

	/// @copydoc ISignatureInformation::getSignatureInfo()
	Variant getSignatureInfo(Variant vFileName, Variant vSecurity, Variant vFileHash) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IService interface
	
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
} // namespace sys
} // namespace openEdr 

/// @}
