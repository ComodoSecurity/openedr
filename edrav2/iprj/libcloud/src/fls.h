//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (19.03.2019)
// Reviewer: Yury Podpruzhnikov (28.03.2019)
//
///
/// @file File Lookup Service (FLS) objects declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once
#include <objects.h>
#include <fls.hpp>
#include "flscommon.h"

namespace cmd {
namespace cloud {
namespace fls {

//
//
//
class VerdictProvider : public ObjectBase<>
{
private:

	using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
	struct CacheItem
	{
		Time time;
		TimePoint timePoint;
		FileVerdict verdict;
		CacheItem(Time _time, TimePoint _timePoint, FileVerdict _verdict) :
			time(_time), timePoint(_timePoint), verdict(_verdict) {}
	};

	struct WaitQueueItem
	{
		size_t nRequestCount = 0; //< number of requesting (waiting) threads
	};
	
	std::recursive_mutex m_mtxData;
	std::unordered_map<Hash, WaitQueueItem> m_waitQueue;
	std::unordered_set<Hash> m_postponedQueue;
	std::unordered_map<Hash, CacheItem> m_cache;

	Size m_nMaxHashPerRequest = 1;

	static constexpr Size c_nDefaultRequestTimeout = 2000; // ms
	Size m_nRequestTimeout = c_nDefaultRequestTimeout;
	boost::condition_variable_any m_cvRequester; //< conditional variable to sleep and wake up a sender

	ThreadPool m_sendPool{ "FlsService::VerdictProvider" };
	// This timer is used for calling if FLS server is unavailable
	ThreadPool::TimerContextPtr m_pCheckServerTimer;
	Size m_nUnavailableServerTimeout = c_nDefaultUnavailableServerTimeout;

	std::atomic_bool m_fServerUnavailable = false;
	static constexpr Size c_nDefaultUnavailableServerTimeout = 10000; // ms 

	static constexpr Size c_nPurgeMask = 0x3FF;
	Size m_nPurgeMask = c_nPurgeMask;
	std::unordered_map<FileVerdict, Size> m_purgeTimeouts;
	std::atomic<Size> m_nQueryCounter = 0;
	std::mutex m_mtxQueryCounter;
	std::atomic_bool m_fStarted = false; //< The provider is running
	std::string m_sServiceName; // The privider's name (for logging)

	bool sendQuery(bool fIgnoreUnavailableServer);
	void sendQueryInt(bool fIgnoreUnavailableServer);
	std::optional<FileVerdict> getCachedVerdictInt(const Hash& hash);
	std::optional<CacheItem> getCacheItemInt(const Hash& hash);
	void purgeCache();

protected:
	std::unique_ptr<Client> m_pClient;
	std::mutex m_mtxClient;

public:
	virtual ~VerdictProvider();
	
	void finalConstruct(Variant vConfig);
	void updateSettings(Variant vConfig);

	Variant saveCache();
	void loadCache(Variant vData);
	void start();
	void stop();
	void shutdown();
	std::optional<FileVerdict> getCachedVerdict(const Hash& hash);
	std::optional<CacheItem> getCacheItem(const Hash& hash);
	FileVerdict getVerdict(const Hash& hash);
	Size getPurgeTimeout(FileVerdict verdict) const;

	virtual FileVerdictMap getVerdictFromServer(const HashList& hashList) = 0;
};

//
//
//
class FileVerdictProvider : public VerdictProvider
{
public:
	FileVerdictMap getVerdictFromServer(const HashList& hashList) override;
};

//
//
//
class VendorVerdictProvider : public VerdictProvider
{
public:
	FileVerdictMap getVerdictFromServer(const HashList& hashList) override;
//	FileVerdict getVerdict(const std::string& sVendor);
};

///
/// File Lookup Service (FLS) client service.
///
class FlsService : public ObjectBase<CLSID_FlsService>,
	public IService,
	public IFlsClient,
	public ICommandProcessor
{
private:

	std::atomic_bool m_fStarted = false; //< The service is started
	ObjPtr<FileVerdictProvider> m_pFileVerdictProvider;
	ObjPtr<VendorVerdictProvider> m_pVendorVerdictProvider;

	void updateSettings(Variant vConfig);
	void enrichFileVerdict(Variant vFile);
	FileVerdict getVendorVerdict(const Hash& hash);

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **url** [string,opt] - a hostname or IP address of the FLS server.
	///   **timeout** [int,opt] - a timeout of the query (in milliseconds).
	///     (default: 2s).
	///   **unavailableServerTimeout** [int,opt] - a timeout for the next FLS query 
	///     if FLS server is unavailable (in milliseconds) (default: 10s).
	///   **purgeMask** [int,opt] - a mask for the query counter to launch
	///     a verdict cache purge process (default: 0x3ff).
	///   **verdictCache** [seq,opt] - a list of verdicts to be cached. If verdict
	///     is not specified here it will not be cached, so it will be requested
	///     from the server each time.
	///     Each verdict is represented by a dictionary-like descriptor containing:
	///       **verdict** [int] - a verdict's numeric code.
	///       **purgeTimeout** [int] - a verdict's storing duration (in seconds).
	///
	void finalConstruct(Variant vConfig);

	// IService

	/// @copydoc IService::loadState(Variant)
	void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	Variant saveState() override;
	/// @copydoc IService::start()
	void start() override;
	/// @copydoc IService::stop()
	void stop() override;
	/// @copydoc IService::shutdown()
	void shutdown() override;

	// IFlsClient

	/// @copydoc IFlsClient::getFileVerdict(std::string_view)
	FileVerdict getFileVerdict(const Hash& hash) override;

	/// @copydoc IFlsClient::isFileVerdictReady(std::string_view)
	bool isFileVerdictReady(const Hash& hash) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute(Variant,Variant)
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace fls
} // namespace cloud
} // namespace cmd

/// @} 
