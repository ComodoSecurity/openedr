//
// edrav2.libcore project
//
// Scenario manager object
//
// Author: Yury Podpruzhnikov (03.02.2019)
// Reviewer: Denis Bogdanov (07.03.2019)
//
#pragma once
#include <command.hpp>
#include <service.hpp>
#include <queue.hpp>
#include <threadpool.hpp>
#include <objects.h>
#include <common.hpp>

namespace cmd {
	
//
//
//
class ScenarioManager : public ObjectBase<CLSID_ScenarioManager>, 
	public ICommandProcessor, 
	public IService, 
	public IQueueNotificationAcceptor
{
private:

	//
	// Scenario descriptor 
	//
	struct ScenarioInfo 
	{
		ObjPtr<ICommand> pCmd; //< Scenario object
		std::string sThreadPoolTag; //< Tag of thread pool
		Variant vVersion; //< Scenario version (to detect changing)
	};


	typedef std::unordered_map<std::string, ScenarioInfo> ScenarioMap; //< Scenarios storage type
	ScenarioMap m_mapScenarios; //< Scenarios storage
	std::mutex m_mtxScenarios; //< Scenarios storage mutex
	std::mutex m_mtxUpdateScenarios; //< Update scenarios mutex

	typedef std::unordered_map<std::string, ThreadPool> ThreadPoolMap; //< ThreadPools storage type
	ThreadPoolMap m_mapThreadPools; //< ThreadPools storage
	std::mutex m_mtxThreadPools; //< ThreadPools storage mutex
	Variant m_vThreadPoolsInfo;

	/// Name of default pool. 
	/// It is used for a scenario  which does no have a thread pool tag.
	static inline char c_sDefPoolName[] = "DefaultPool";
	static inline constexpr Size c_nDefPoolSize = 1;

	// Create thread pools
	void createThreadPools(Dictionary dictThreadPools);

	/// Path to descriptors of scenarios in global catalog
	static inline constexpr char c_sScenariosPath[] = "app.config.extern.scenarios";

	// Update scenarios storage
	void updateScenarios();

	//
	// List of Scenarios for statistic collection
	//
	std::list<ObjWeakPtr<IObject>> m_StatScenarios;
	std::mutex m_mtxStatScenarios; //< StatScenarios mutex

	// Garbage collection of m_StatScenarios
	// Check health of weak_ptr each c_nGCRateCounter-th adding
	static constexpr Size c_nGCRateCounter = 1000; 
	Size m_nGCCounter = 0;


	bool m_fEnableStatisticCollection = false; // Synced with m_StatScenarios
	Variant enableCollectStatInfo(bool fEnable);
	Variant getStatInfo(bool fShowLineInfo);


public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "threadPools" [dict, opt] - Dictionary with thread pools descriptors. 
	///     key is thread pool tag, value is ThreadPool descriptor. Dictionaries with fields:
	///     * "size" [int, opt] - start thread count. default = 2.
	///     * "priority" [str, opt] - threads priority. "high", "normal", "low". default = "normal"
	///     * "collectStatistic" [bool, opt] - enable statistic collection by default
	///       If it is true, collection is enabled for all scenarios, which does not have parameter "collectStatistic".
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	virtual Variant execute(Variant vCommand, Variant vParams = Variant()) override;


	// IService

	/// @copydoc IService::loadState()
	virtual void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	virtual Variant saveState() override;
	/// @copydoc IService::start()
	virtual void start() override;
	/// @copydoc IService::start()
	virtual void stop() override;
	/// @copydoc IService::shutdown()
	virtual void shutdown() override;

	// IQueueNotificationAcceptor

	/// @copydoc ICommandProcessor::notifyAddQueueData()
	virtual void notifyAddQueueData(Variant vTag) override;
	/// @copydoc ICommandProcessor::notifyQueueOverflowWarning()
	virtual void notifyQueueOverflowWarning(Variant vTag) override;

	// self public methods

	//
	// All scenarios registration
	// For statistic collection  for the present
	//
	void registerScenario(ObjPtr<IObject> pScenario);
};

} // namespace cmd 
