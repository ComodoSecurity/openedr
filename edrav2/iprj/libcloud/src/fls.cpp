//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (19.03.2019)
// Reviewer: Yury Podpruzhnikov (28.03.2019)
//
///
/// @file File Lookup Service (FLS) objects implementation
///
/// @addtogroup cloud Cloud communication library
/// @{
///
#include "pch.h"
#include <net.hpp>
#include "fls.h"
#include "flsproto4.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "fls"

constexpr char c_sZeroHash[] = "0000000000000000000000000000000000000000";
constexpr char c_sUdpPrefix[] = "udp://";
constexpr char c_sProtocolDelimiter[] = "://";

namespace openEdr {
namespace cloud {
namespace fls {

//
//
//
Client::Client(std::string_view sHost, Size nPort) :
	m_sHost(sHost), m_nPort(nPort)
{
}


//
//
//
Client::~Client()
{
}

//
//
//
VerdictProvider::~VerdictProvider()
{
	m_sendPool.stop(true);
}

//
//
//
void VerdictProvider::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;

	m_nMaxHashPerRequest = v4::UdpClient::c_nMaxHashPerRequest;
	m_nRequestTimeout = vConfig.get("timeout", m_nRequestTimeout);
	m_nUnavailableServerTimeout = vConfig.get("unavailableServerTimeout", m_nUnavailableServerTimeout);
	m_sServiceName = vConfig.get("serviceName", "FlsVerdictProvider");

	updateSettings(vConfig);

	// Init cache purge
	m_nPurgeMask = vConfig.get("purgeMask", m_nPurgeMask);
	for (const auto& vItem : vConfig.get("verdictCache", nullptr))
		m_purgeTimeouts.try_emplace(vItem["verdict"], (Size)vItem["purgeTimeout"] * 1000);

	// Init sender thread pool
	m_sendPool.addThreads(1);

	// Init server check timer
	m_pCheckServerTimer = m_sendPool.runWithDelay(m_nUnavailableServerTimeout, [this]()
	{
		return this->sendQuery(true);
	});
	m_pCheckServerTimer->cancel();

	TRACE_END("Error during configuration");
}

//
//
//
void VerdictProvider::updateSettings(Variant vConfig)
{
	if ((!vConfig.isDictionaryLike() || !vConfig.has("url")) && m_pClient)
		return;

	// Parse server address
	std::string sUrl = vConfig.get("url", fls::v4::c_sDefaultHost);
	if (sUrl.find(c_sProtocolDelimiter) == std::string::npos)
		sUrl = c_sUdpPrefix + sUrl;
	Variant vAddress = net::parseUrl(sUrl);
	if (variant::convert<variant::ValueType::Integer>(vAddress["port"]) == 80)
		vAddress.put("port", fls::v4::c_nDefaultUdpPort);

	// Init the internal client
	std::scoped_lock lock(m_mtxClient);
	m_pClient = std::make_unique<v4::UdpClient>(vAddress["host"], vAddress["port"]);

	// We're switched to a different host, so clear server-is-unavailable state
	m_fServerUnavailable = false;
	if (m_pCheckServerTimer)
		m_pCheckServerTimer->cancel();
}

//
//
//
Variant VerdictProvider::saveCache()
{
	std::scoped_lock lock(m_mtxData);
	if (m_cache.empty())
		return {};

	Variant vResult = Dictionary();
	for (const auto& item : m_cache)
	{
		Dictionary dictItem({
			{ "time", item.second.time },
			{ "verdict", item.second.verdict }
			});
		vResult.put(item.first, dictItem);
	}
	return vResult;
}

//
//
//
void VerdictProvider::loadCache(Variant vData)
{
	using namespace std::chrono;

	if (vData.isEmpty())
		return;

	Time baseTime = getCurrentTime();
	auto baseTimePoint = steady_clock::now();

	std::scoped_lock lock(m_mtxData);
	for (const auto& item : Dictionary(vData))
	{
		Time itemTime = item.second.get("time");
		if (itemTime > baseTime)
			continue;

		auto expireTimeout = milliseconds(baseTime - itemTime);
		m_cache.try_emplace(std::string(item.first),
			itemTime,
			baseTimePoint + expireTimeout,
			item.second.get("verdict"));
	}
}

//
//
//
void VerdictProvider::start()
{
	m_fStarted = true;
	LOGLVL(Detailed, m_sServiceName << " is started");
}

//
//
//
void VerdictProvider::stop()
{
	m_fStarted = false;
	LOGLVL(Detailed, m_sServiceName << " is stopped");
}

//
//
//
void VerdictProvider::shutdown()
{
	if(m_pCheckServerTimer)
		m_pCheckServerTimer->cancel();
	m_sendPool.stop(true);
}


//
// This function should be called with locking of m_mtxData
//
std::optional<FileVerdict> VerdictProvider::getCachedVerdictInt(const Hash& hash)
{
	auto it = m_cache.find(hash);
	if (it == m_cache.end())
		return std::nullopt;
	return it->second.verdict;
}

//
// This function should be called with locking of m_mtxData
//
std::optional<VerdictProvider::CacheItem> VerdictProvider::getCacheItemInt(const Hash& hash)
{
	auto it = m_cache.find(hash);
	if (it == m_cache.end())
		return std::nullopt;
	return it->second;
}

//
//
//
std::optional<FileVerdict> VerdictProvider::getCachedVerdict(const Hash& hash)
{
	std::scoped_lock lock(m_mtxData);
	return getCachedVerdictInt(hash);
}

//
//
//
std::optional<VerdictProvider::CacheItem> VerdictProvider::getCacheItem(const Hash& hash)
{
	std::scoped_lock lock(m_mtxData);
	return getCacheItemInt(hash);
}

//
//
//
FileVerdict VerdictProvider::getVerdict(const Hash& hash)
{
	// If caller can't calculate hash, it can pass "" or "000...000"
	if (hash.empty() || (hash == c_sZeroHash))
		return FileVerdict::Unknown;

	// Validate size
	if (hash.size() != c_nHashSize * 2)
		error::InvalidArgument(SL, FMT("Invalid hash size <" << hash << ">")).throwException();

	if (!m_fStarted)
	{
		LOGLVL(Trace, "Object <" << hash << "> has verdict <" << FileVerdict::Unknown <<
			"> (" << m_sServiceName << " is not running)");
		return FileVerdict::Unknown;
	}

	std::unique_lock lock(m_mtxData);

	// Add item to the waiting queue
	size_t nRequestCount = (m_waitQueue[hash].nRequestCount += 1);

	// If request is first for this hash, run sender
	if (nRequestCount == 1)
		m_sendPool.run(&VerdictProvider::sendQuery, this, false /*fIgnoreUnavailableServer*/);

	auto finishTime = boost::chrono::steady_clock::now() + boost::chrono::milliseconds(m_nRequestTimeout);

	// Wait sender or timeout
	while (true)
	{
		auto eWaitResult = m_cvRequester.wait_until(lock, finishTime);

		// Check stop conditions
		auto verdict = getCachedVerdict(hash);
		if ((eWaitResult != boost::cv_status::timeout) && !verdict.has_value())
			continue;

		// Decrement nRequestCount
		auto it = m_waitQueue.find(hash);
		if ((it != m_waitQueue.end()) && (it->second.nRequestCount != 0))
			it->second.nRequestCount -= 1;

		// Return verdict or Unknown
		if (verdict.has_value())
		{
			LOGLVL(Trace, "Object <" << hash << "> has verdict <" << verdict.value() << "> (from FLS server)");
			return verdict.value();
		}
		LOGLVL(Trace, "Object <" << hash << "> has verdict <" << FileVerdict::Unknown << "> (FLS server timeout)");
		return FileVerdict::Unknown;
	}
}

//
//
//
bool VerdictProvider::sendQuery(bool fIgnoreUnavailableServer)
{
	if (!m_fStarted)
		return false;

	CMD_TRY
	{
		sendQueryInt(fIgnoreUnavailableServer);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL);
	}

	return true;
}

//
//
//
void VerdictProvider::sendQueryInt(bool fIgnoreUnavailableServer)
{
	using namespace std::chrono;

	// Skip if FLS server is unavailable
	if (m_fServerUnavailable && !fIgnoreUnavailableServer)
		return;

	// Prepare HashList to send
	HashList hashList;
	{
		std::scoped_lock lock(m_mtxData);

		// Process the waiting queue
		for (auto iter = m_waitQueue.begin(); iter != m_waitQueue.end(); )
		{
			// Is sentHashList full?
			if (hashList.size() >= m_nMaxHashPerRequest)
				break;

			// if item does not have waiting requesters, moves it into PostponedQueue
			if (iter->second.nRequestCount == 0)
			{
				m_postponedQueue.insert(iter->first);
				iter = m_waitQueue.erase(iter);
				continue;
			}

			hashList.push_back(iter->first);
			++iter;
		}

		// Process the postponed queue
		for (auto& elem : m_postponedQueue)
		{
			// Is the list full?
			if (hashList.size() >= m_nMaxHashPerRequest)
				break;

			hashList.push_back(elem);
		}
	}

	// Skip if there is no hashes to send
	if (hashList.empty())
		return;

	FileVerdictMap resultVerdicts;
	Time verdictTime = getCurrentTime();
	auto verdictTimePoint = steady_clock::now();
	CMD_TRY
	{
		resultVerdicts = getVerdictFromServer(hashList);
	}
	CMD_PREPARE_CATCH
	catch (const error::ConnectionError& ex)
	{
		// Detect no internet connection.
		// if (no internet), schedules delayed sendQuery
		// else logs error and just returns

		// For the present 
		// any error is recognized as no internet connection
		// and any error is logged

		// Schedules delayed sendQuery
		if (!m_fServerUnavailable)
		{
			LOGINF("FLS server is unavailable (" << ex.what() << ")");
			m_fServerUnavailable = true;
			m_pCheckServerTimer->reschedule(m_nUnavailableServerTimeout);
		}
		return;
	}
	catch (error::Exception& ex)
	{
		ex.log(SL, "Can't process FLS query");
		return;
	}

	// Success FLS query is occurred

	// Reset server-is-unavailable state
	if (m_fServerUnavailable)
	{
		LOGINF("FLS server is available again");
		m_fServerUnavailable = false;
		m_pCheckServerTimer->cancel();
	}

	// Update the cache and queues
	{
		std::scoped_lock lock(m_mtxData);
		for (const auto& verdict : resultVerdicts)
		{
			m_cache.try_emplace(verdict.first, verdictTime,
				verdictTimePoint + milliseconds(getPurgeTimeout(verdict.second)), verdict.second);
			m_waitQueue.erase(verdict.first);
			m_postponedQueue.erase(verdict.first);
		}
	}

	{
		// Purge the cache (in async thread)
		std::scoped_lock lock(m_mtxQueryCounter);
		if (((++m_nQueryCounter) & m_nPurgeMask) == 0)
			run(&VerdictProvider::purgeCache, getPtrFromThis(this));
	}

	m_cvRequester.notify_all();
}

//
//
//
void VerdictProvider::purgeCache()
{
	std::scoped_lock lock(m_mtxData);
	LOGLVL(Trace, "Cache purge is started <cacheSize=" << m_cache.size() << ">");
	Time now = getCurrentTime();
	for (auto iter = m_cache.begin(); iter != m_cache.end();)
	{
		Time verdictTime = iter->second.time;
		Size nTimeout = getPurgeTimeout(iter->second.verdict);
		if ((now > verdictTime) && (now - verdictTime >= nTimeout))
			iter = m_cache.erase(iter);
		else
			++iter;
	}
	LOGLVL(Trace, "Cache purge is finished <cacheSize=" << m_cache.size() << ">");
}

//
//
//
Size VerdictProvider::getPurgeTimeout(FileVerdict verdict) const
{
	auto iter = m_purgeTimeouts.find(verdict);
	return (iter != m_purgeTimeouts.end() ? iter->second : 0);
}

//
//
//
FileVerdictMap FileVerdictProvider::getVerdictFromServer(const HashList& hashList)
{
	std::scoped_lock lock(m_mtxClient);
	return m_pClient->getFileVerdict(hashList);
}

//
//
//
FileVerdictMap VendorVerdictProvider::getVerdictFromServer(const HashList& hashList)
{
	std::scoped_lock lock(m_mtxClient);
	return m_pClient->getVendorVerdict(hashList);
}

//
//
//
void FlsService::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	Variant vLocalConfig = vConfig.clone();
	m_pFileVerdictProvider = createObject<FileVerdictProvider>(
		vLocalConfig.merge(Dictionary({ {"serviceName", "FlsFileVerdictProvider" } }),
			Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict));
	m_pVendorVerdictProvider = createObject<VendorVerdictProvider>(
		vLocalConfig.merge(Dictionary({ {"serviceName", "FlsVendorVerdictProvider" } }),
			Variant::MergeMode::All | Variant::MergeMode::MergeNestedDict));
	updateSettings(vConfig);
	TRACE_END("Error during configuration");
}

//
//
//
void FlsService::updateSettings(Variant vConfig)
{
	TRACE_BEGIN;
	if ((!vConfig.isDictionaryLike() || !vConfig.has("url")) &&
		m_pFileVerdictProvider && m_pVendorVerdictProvider)
		return;

	m_pFileVerdictProvider->updateSettings(vConfig);
	m_pVendorVerdictProvider->updateSettings(vConfig);
	TRACE_END("Error during settings update");
}

//
//
//
void FlsService::loadState(Variant vState)
{
	TRACE_BEGIN;
	if (vState.isEmpty())
		return;

	if (vState.has("fileCache"))
		m_pFileVerdictProvider->loadCache(vState.get("fileCache"));
	if (vState.has("vendorCache"))
		m_pVendorVerdictProvider->loadCache(vState.get("vendorCache"));
	TRACE_END("Error during loading state");
}

//
//
//
Variant FlsService::saveState()
{
	TRACE_BEGIN;
	Dictionary dictState;

	auto vFileCache = m_pFileVerdictProvider->saveCache();
	if (!vFileCache.isEmpty())
		dictState.put("fileCache", vFileCache);

	auto vVendorCache = m_pVendorVerdictProvider->saveCache();
	if (!vVendorCache.isEmpty())
		dictState.put("vendorCache", vVendorCache);

	return dictState;
	TRACE_END("Error during saving state");
}

//
//
//
void FlsService::start()
{
	m_pFileVerdictProvider->start();
	m_pVendorVerdictProvider->start();
	m_fStarted = true;
	LOGLVL(Detailed, "FlsService is started");
}

//
//
//
void FlsService::stop()
{
	if (!m_fStarted)
		return;
	m_fStarted = false;
	m_pFileVerdictProvider->stop();
	m_pVendorVerdictProvider->stop();
	LOGLVL(Detailed, "FlsService is stopped");
}

//
//
//
void FlsService::shutdown()
{
	m_fStarted = false;
	m_pFileVerdictProvider->shutdown();
	m_pVendorVerdictProvider->shutdown();
	m_pFileVerdictProvider.reset();
	m_pVendorVerdictProvider.reset();
}

//
//
//
FileVerdict FlsService::getFileVerdict(const Hash& hash)
{
	// Check cache
	auto readyVerdict = m_pFileVerdictProvider->getCachedVerdict(hash);
	if (readyVerdict.has_value())
	{
		LOGLVL(Trace, "File <" << hash << "> has verdict <" << readyVerdict.value() << "> (cached)");
		return readyVerdict.value();
	}

	// If the service is not started
	if (!m_fStarted)
	{
		LOGLVL(Trace, "File <" << hash << "> has verdict <" << FileVerdict::Unknown <<
			"> (FlsService is not running)");
		return FileVerdict::Unknown;
	}

	return m_pFileVerdictProvider->getVerdict(hash);
}

//
//
//
FileVerdict FlsService::getVendorVerdict(const Hash& hash)
{
	// Check cache
	auto readyVerdict = m_pVendorVerdictProvider->getCachedVerdict(hash);
	if (readyVerdict.has_value())
	{
		LOGLVL(Trace, "Vendor <" << hash << "> has verdict <" << readyVerdict.value() << "> (cached)");
		return readyVerdict.value();
	}

	// If the service is not started
	if (!m_fStarted)
	{
		LOGLVL(Trace, "Vendor <" << hash << "> has verdict <" << FileVerdict::Unknown <<
			"> (FlsService is not running)");
		return FileVerdict::Unknown;
	}

	return m_pVendorVerdictProvider->getVerdict(hash);
}

//
//
//
bool FlsService::isFileVerdictReady(const Hash& hash)
{
	return m_pFileVerdictProvider->getCachedVerdict(hash).has_value();
}

//
//
//
void FlsService::enrichFileVerdict(Variant vFile)
{
	using namespace std::chrono;

	std::string sSignerHash;
	auto optSigner = getByPathSafe(vFile, "signature.vendor");
	if (optSigner.has_value())
	{
		std::string sSigner = optSigner.value();
		if (!sSigner.empty())
		{
			auto hash = crypt::sha1::getHash(sSigner);
			sSignerHash = string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
		}
	}

	if (!sSignerHash.empty())
	{
		// Check the signer in Trusted Vendors List
		auto verdict = getVendorVerdict(sSignerHash);
		if (verdict == FileVerdict::Safe)
		{
			putByPath(vFile, "fls.verdict", verdict, true /* fCreatePaths */);

			auto optItem = m_pVendorVerdictProvider->getCacheItem(sSignerHash);
			if (optItem.has_value())
			{ 
				auto now = steady_clock::now();
				if (optItem.value().timePoint > now)
					putByPath(vFile, "fls.verdictExpireTimeout", duration_cast<milliseconds>(
						optItem.value().timePoint - steady_clock::now()).count(), true /* fCreatePaths */);
			}
			return;
		}
	}

	// If caller can't calculate hash, it can pass "" or "000...000"
	// If <hash> field is absent -> throw an exception
	std::string sFileHash = vFile["hash"];
	if (sFileHash.empty() || (sFileHash == c_sZeroHash))
	{
		// EDR-2694: Treat huge files as trusted
		io::IoSize nFileSize = vFile.get("size", 0);
		auto verdict = (nFileSize > sys::win::g_nMaxStreamSize) ?
			FileVerdict::Safe : FileVerdict::Unknown;
		putByPath(vFile, "fls.verdict", verdict, true /* fCreatePaths */);
		return;
	}

	auto verdict = getFileVerdict(sFileHash);
	putByPath(vFile, "fls.verdict", verdict, true /* fCreatePaths */);

	auto optItem = m_pFileVerdictProvider->getCacheItem(sFileHash);
	if (optItem.has_value())
	{
		auto now = steady_clock::now();
		if (optItem.value().timePoint > now)
			putByPath(vFile, "fls.verdictExpireTimeout", duration_cast<milliseconds>(
				optItem.value().timePoint -	steady_clock::now()).count(), true /* fCreatePaths */);
	}
}

///
/// #### Supported commands
///
Variant FlsService::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command's parameters:\n" << vParams);

	///
	/// @fn Variant FlsService::execute()
	///
	/// ##### enrichFileVerdict()
	/// Enriches the file descriptor with verdict (fls.verdict, fls.verdictExpireTime)
	///   * file [Variant] - a file descriptor.
	///
	if (vCommand == "enrichFileVerdict")
	{
		if (!vParams.has("file"))
			error::InvalidArgument(SL, "Missing field <file>").throwException();
		enrichFileVerdict(vParams.get("file"));
		return {};
	}
	///
	/// @fn Variant FlsService::execute()
	///
	/// ##### getFileVerdict()
	/// Sends the data to the server.
	///   * hash [Variant] - string (40 chars) with SHA1 hash of the file's content.
	///
	else if (vCommand == "getFileVerdict")
	{
		if (!vParams.has("hash"))
			error::InvalidArgument(SL, "Missing field <hash>").throwException();
		return getFileVerdict(vParams["hash"]);
	}
	///
	/// @fn Variant FlsService::execute()
	///
	/// ##### isFileVerdictReady()
	/// Checks if file verdict is ready.
	///   * hash [Variant] - string (40 chars) with SHA1 hash of the file's content.
	///
	else if (vCommand == "isFileVerdictReady")
	{
		if (!vParams.has("hash"))
			error::InvalidArgument(SL, "Missing field <hash>").throwException();
		return isFileVerdictReady(vParams["hash"]);
	}
	///
	/// @fn Variant ValkyrieService::execute()
	///
	/// ##### updateSettings()
	/// Update the settings.
	///
	else if (vCommand == "updateSettings")
	{
		updateSettings(vParams);
		return true;
	}
	///
	/// @fn Variant FlsService::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	else if (vCommand == "start")
	{
		start();
		return {};
	}
	///
	/// @fn Variant FlsService::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	else if (vCommand == "stop")
	{
		stop();
		return {};
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
}

} // namespace fls
} // namespace cloud
} // namespace openEdr

/// @}
