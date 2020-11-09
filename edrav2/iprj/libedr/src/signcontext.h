//
// edrav2.libcore project
//
// Context service declaration
//
// Author: Denis Kroshin (25.08.2019)
// Reviewer: Denis Bogdanov (02.09.2019)
//
///
/// @file Context service declaration
///
/// @addtogroup edr
/// @{
#pragma once
#include <objects.h>

namespace openEdr {

///
/// Events Pattern Searcher.
///
class ContextService : public ObjectBase<CLSID_ContextService>,
	public ICommandProcessor,
	public IService
{
private:
	typedef crypt::xxh::Hasher Hasher;
	typedef Hasher::Hash Hash;

	static const Time c_nPurgeTimeout = 60 * 1000; // 1 min
	static const Time c_nMinPurgeDelta = 5 * 1000; // 5 sec
	static const Size c_nBasketMaxSize = 10000;
	static const auto c_nEmptyHash = Hash();

	struct Context
	{
		Variant data;
		Time expireTime = 0;
		Hash group = c_nEmptyHash;
	};

	typedef std::list<Context> ContextList;

	struct Basket: public std::enable_shared_from_this<Basket>
	{
	private:
		std::mutex mtxData;
		std::unordered_map<Hash, ContextList> mapKeys;
		Size size = 0;
		Time expireTimeSum = 0;
		std::unordered_set<Hash> purgeGroup;
		Time lastPurgeTime = 0;
		bool fFastPurge = false; // Purge cycle with short intervals
		bool fForcePurge = false; // Purge as fast as possible

		Size maxSize = c_nBasketMaxSize;
		Size wrnSize = c_nBasketMaxSize / 5 * 4; // 80%
		Time purgeTimeout = c_nPurgeTimeout;

		ThreadPool::TimerContextPtr timerPurge;

	public:
		// For debug purposes
		std::string name;

		void init(Variant vParams);
		void reset();
		bool has(const Hash nKeyId);
		Variant load(const Hash nKeyId, const bool fRemove = false);
		void save(const Hash nKeyId, const Context& ctx, const Size nLimit);
		void removeKey(const Hash nKeyId);
		void removeGroup(const Hash nGroupId, const bool fForce = false);

		Time purgeRound(const Time expireTime);
		bool purge();
		void schedule(ThreadPool* pThreadPool);
		void cancel();

		Variant getInfo();
	};

	typedef std::shared_ptr<Basket> BasketPtr;
	typedef std::vector<BasketPtr> Storage;

	std::mutex m_mtxData;
	Storage m_pStorage;
	std::map<std::string, Size> m_pNames;

	std::mutex m_mtxStartStop;
	std::atomic_bool m_fInitialized = false;
	ThreadPool m_threadPool{ "ContextServicePool" };
	Variant m_vConfig;

	inline Hash getKeyId(Variant vParams);

	Size createBasket(const std::string sName);
	Size getBasketId(const std::string sName);
	BasketPtr getBasket(Variant vParams, bool fCreate = false);

	bool has(Variant vParams);
	Variant load(Variant vParams);
	void save(Variant vParams);
	void remove(Variant vParam);
	void purge(Variant vParams);

	Variant getStatistic();

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **receiver** - object that receive MLE events
	///   **purgeMask** - purge process start every (purgeMask+1) event
	///   **purgeTimeout** - time to keep events in before purge
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IService

	/// @copydoc IService::loadState()
	virtual void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	virtual Variant saveState() override;
	/// @copydoc IService::start()
	virtual void start() override;
	/// @copydoc IService::stop()
	virtual void stop() override;
	/// @copydoc IService::shutdown()
	virtual void shutdown() override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	virtual Variant execute(Variant vCommand, Variant vParams) override;
};
} // namespace openEdr 

/// @} 
