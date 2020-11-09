//
// edrav2.libcore project
//
// Scenario manager object
//
// Author: Yury Podpruzhnikov (03.02.2019)
// Reviewer: Denis Bogdanov (07.03.2019)
//
#include "pch.h"
#include "scenario.h"
#include "scenariomgr.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "scnromgr"

namespace openEdr {

//
//
//
void ScenarioManager::createThreadPools(Dictionary dictThreadPools)
{
	ThreadPoolMap mapThreadPools;

	for (auto[sKey, vValue] : dictThreadPools)
	{
		Size nThreadCount = vValue.get("size", c_nDefPoolSize);

		ThreadPool::Priority ePriority = ThreadPool::Priority::Normal;
		Variant vPriority = vValue.get("priority", "normal");
		if (vPriority == "high")
			ePriority = ThreadPool::Priority::High;
		if (vPriority == "low")
			ePriority = ThreadPool::Priority::Low;

		std::string sName = "ScenarioManager::";
		sName += sKey;
		mapThreadPools.emplace(sKey, std::move(ThreadPool(std::string(sName), nThreadCount, ePriority)));
	}

	// Add default pool, if need
	if (mapThreadPools.find(c_sDefPoolName) == mapThreadPools.end())
		mapThreadPools.emplace(c_sDefPoolName, std::move(ThreadPool(c_sDefPoolName, c_nDefPoolSize, ThreadPool::Priority::Normal)));

	std::scoped_lock lock(m_mtxThreadPools);
	
	// FIXME: We loose control under old thread pools if stop() isn't called!
	swap(m_mapThreadPools, mapThreadPools);
}

//
//
//
void ScenarioManager::updateScenarios()
{
	std::string sScenarioName = "unknown";

	CMD_TRY
	{
		std::scoped_lock lock(m_mtxUpdateScenarios);

		Variant vScenarios = getCatalogData(c_sScenariosPath, nullptr);

		// If scenarios are not found, clear current
		if (vScenarios.isNull())
		{
			std::scoped_lock lock2(m_mtxScenarios);
			m_mapScenarios.clear();
			return;
		}

		ScenarioMap mapScenarios;

		// Clone
		for (auto[sKey, vScenario] : Dictionary(vScenarios))
		{
			sScenarioName = std::string(sKey);

			Variant vVersion = vScenario.get("version", nullptr);

			// Try find the same scenario in the old map
			if (!vVersion.isNull())
			{
				std::scoped_lock lock2(m_mtxScenarios);
				auto iter = m_mapScenarios.find(sScenarioName);
				if (iter != m_mapScenarios.end() && iter->second.vVersion == vVersion)
				{
					mapScenarios[sScenarioName] = iter->second;
					continue;
				}
			}

			// Build new scenario
			Variant vScenarioDesc = vScenario.clone(); // unlink scenario from config

			// Add name
			if (!vScenarioDesc.has("name"))
				vScenarioDesc.put("name", sKey);

			// Add collectStatistic
			if (m_fEnableStatisticCollection && !vScenarioDesc.has("collectStatistic"))
				vScenarioDesc.put("collectStatistic", true);
			
			// Add clsid
			if (!vScenarioDesc.has("clsid"))
				vScenarioDesc.put("clsid", CLSID_Scenario);

			ScenarioInfo scenarioInfo;
			scenarioInfo.sThreadPoolTag = vScenarioDesc.get("threadPoolTag", c_sDefPoolName);
			scenarioInfo.vVersion = vVersion.clone();
			scenarioInfo.pCmd = queryInterface<ICommand>(createObject(vScenarioDesc));

			mapScenarios[sScenarioName] = scenarioInfo;
		}

		// replace old scenarios
		{
			std::scoped_lock lock2(m_mtxScenarios);
			swap(m_mapScenarios, mapScenarios);
		}
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("ScenarioManager can't load scenarios - wrong scenario <" << sScenarioName << ">"));
#ifdef _DEBUG
		e.throwException();
#endif // _DEBUG

	}
}

//
//
//
struct StatInfo
{
	std::string sName;
	uint64_t nValue = 0;

	bool operator < (const StatInfo& other) const
	{
		if (nValue != other.nValue)
			return nValue > other.nValue;
		return sName > other.sName;
	}

	StatInfo() = default;
	StatInfo(std::string_view sName_, uint64_t nValue_) : sName(sName_), nValue(nValue_) {}
};

//
//
//
Variant doReport(std::vector<StatInfo>& statInfo, Size nMaxCount)
{
	std::sort(statInfo.begin(), statInfo.end());
	Sequence vReport;

	Size nLimit = std::min<Size>(nMaxCount, statInfo.size());
	for (Size i = 0; i < nLimit; ++i)
		vReport.push_back(Dictionary({ {"name", statInfo[i].sName}, {"time", statInfo[i].nValue }}));
	return vReport;
}

//
//
//
Variant ScenarioManager::enableCollectStatInfo(bool fEnable)
{
	std::scoped_lock lock(m_mtxStatScenarios);
	if (m_fEnableStatisticCollection == fEnable)
		return Variant();

	m_fEnableStatisticCollection = fEnable;

	for (auto& pWeak : m_StatScenarios)
	{
		auto pObj = pWeak.lock();
		if (pObj == nullptr)
			continue;

		auto pScenario = queryInterfaceSafe<Scenario>(pObj);
		if (pScenario == nullptr)
			continue;

		pScenario->enableStatInfo(fEnable);
	}

	return Variant();
}

//
//
//
Variant ScenarioManager::getStatInfo(bool fShowLineInfo)
{
	// Collect stat
	Dictionary vScenariosInfo;
	{
		std::scoped_lock lock(m_mtxStatScenarios);

		auto endIter = m_StatScenarios.end();
		for (auto iter = m_StatScenarios.begin(); iter != endIter;)
		{
			auto pObj = iter->lock();
			
			// remove deleted object
			if (pObj == nullptr)
			{
				iter = m_StatScenarios.erase(iter);
				continue;
			}
			// iter go to next (for "continue" usage)
			++iter;

			auto pScenario = queryInterfaceSafe<Scenario>(pObj);
			if (pScenario == nullptr)
				continue;

			Variant vScenarioStatInfo = pScenario->getStatInfo();
			if (vScenarioStatInfo.isEmpty())
				continue;

			

			vScenariosInfo.put(pScenario->getName(), vScenarioStatInfo);
		}
	}

	if (vScenariosInfo.isEmpty())
		return Dictionary();

	Dictionary vResult;

	// Add scenarios full info
	if (fShowLineInfo)
	{
		vResult.put("full", vScenariosInfo);
	}

	// Add scenarios common info
	{
		Dictionary vRawScenariosInfo;
		for (const auto&[sScenarioName, vInfo] : vScenariosInfo)
		{
			auto vCommonInfo = vInfo.getSafe("total");
			if (vCommonInfo.has_value())
				vRawScenariosInfo.put(sScenarioName, *vCommonInfo);
		}
		vResult.put("total", vRawScenariosInfo);
	}

	// Create reports
	Dictionary vReports;
	vResult.put("reports", vReports);

	// threadTime. top 10 scenario
	{
		std::vector<StatInfo> statInfo;
		for (const auto&[sScenarioName, vInfo] : vScenariosInfo)
			 statInfo.emplace_back(sScenarioName, vInfo.get("total", {}).get("threadTime", 0));
		vReports.put("scenariosByThreadTime", doReport(statInfo, 10));
	}

	// globalTime. top 10 scenario
	{
		std::vector<StatInfo> statInfo;
		for (const auto&[sScenarioName, vInfo] : vScenariosInfo)
			statInfo.emplace_back(sScenarioName, vInfo.get("total", {}).get("globalTime", 0));
		vReports.put("scenariosByGlobalTime", doReport(statInfo, 10));
	}

	// threadTime. top 20 lines
	{
		std::vector<StatInfo> statInfo;

		for (const auto&[sScenarioName, vScenarioInfo] : vScenariosInfo)
		{
			Dictionary vLines(vScenarioInfo.get("lines", Dictionary()));
			for (const auto&[sLineName, vLineInfo] : vLines)
				statInfo.emplace_back(std::string(sScenarioName) + "@" + std::string(sLineName), vLineInfo.get("threadTime", 0));
		}
		vReports.put("linesByThreadTime", doReport(statInfo, 20));
	}

	// globalTime. top 20 lines
	{
		std::vector<StatInfo> statInfo;

		for (const auto&[sScenarioName, vScenarioInfo] : vScenariosInfo)
		{
			Dictionary vLines(vScenarioInfo.get("lines", Dictionary()));
			for (const auto&[sLineName, vLineInfo] : vLines)
				statInfo.emplace_back(std::string(sScenarioName) + "@" + std::string(sLineName), vLineInfo.get("globalTime", 0));
		}
		vReports.put("linesByGlobalTime", doReport(statInfo, 20));
	}

	return vResult;
}

//
//
//
void ScenarioManager::finalConstruct(Variant vConfig)
{
	m_vThreadPoolsInfo = Dictionary(vConfig.get("threadPools", Dictionary()));
	m_fEnableStatisticCollection = vConfig.get("collectStatistic", m_fEnableStatisticCollection);
}

//
//
//
void ScenarioManager::loadState(Variant vState)
{
}

//
//
//
openEdr::Variant ScenarioManager::saveState()
{
	return {};
}

//
//
//
void ScenarioManager::start()
{
	updateScenarios();
	createThreadPools(m_vThreadPoolsInfo);
}

//
//
//
void ScenarioManager::stop()
{
	ThreadPoolMap mapThreadPools;
	{
		std::scoped_lock lock(m_mtxThreadPools);
		swap(m_mapThreadPools, mapThreadPools);
	}

	for (auto&[sKey, threadPool] : mapThreadPools)
		threadPool.stop(true);
}

//
//
//
void ScenarioManager::shutdown()
{
	stop();

	// Clear scenarios to remove circular pointers
	std::list<ObjWeakPtr<IObject>> statScenarios;
	{
		std::scoped_lock lock(m_mtxStatScenarios);
		swap(m_StatScenarios, statScenarios);
	}
	statScenarios.clear();

	ScenarioMap mapScenarios;
	{
		std::scoped_lock lock(m_mtxScenarios);
		swap(m_mapScenarios, mapScenarios);
	}
	mapScenarios.clear();
}

//
//
//
void ScenarioManager::notifyAddQueueData(Variant vTag)
{
	std::string sScenarioTag = vTag;
	std::scoped_lock lock(m_mtxScenarios);

	// Find scenario
	auto scenarioIter = m_mapScenarios.find(sScenarioTag);
	if (scenarioIter == m_mapScenarios.end())
		return;
	ScenarioInfo* pScenarioInfo = &scenarioIter->second;

	std::scoped_lock lock2(m_mtxThreadPools);

	if (m_mapThreadPools.empty())
	{
		// error::InvalidUsage(SL, "Notification received but threads are stopped").log();
		return;
	}
	 
	// Find pool
	auto itPool = m_mapThreadPools.find(pScenarioInfo->sThreadPoolTag);
	if (itPool == m_mapThreadPools.end())
	{
		itPool = m_mapThreadPools.find(c_sDefPoolName);
		if (itPool == m_mapThreadPools.end())
		{
			error::InvalidUsage(SL, "Notification received but no threads found").log();
			return;
		}
	}
	ThreadPool* pThreadPool = &itPool->second;

	// schedule task
	ObjPtr<ICommand> pCmd = pScenarioInfo->pCmd;
	pThreadPool->run([pCmd]() {
		(void)pCmd->execute();
	});
}

//
//
//
void ScenarioManager::notifyQueueOverflowWarning(Variant vTag)
{
}


//
//
//
void ScenarioManager::registerScenario(ObjPtr<IObject> pObj)
{
	auto pScenario = queryInterface<Scenario>(pObj);

	ObjWeakPtr<IObject> pWeak = pObj;

	std::scoped_lock lock2(m_mtxStatScenarios);
	if (m_fEnableStatisticCollection)
		pScenario->enableStatInfo(true);
	m_StatScenarios.push_back(pWeak);
	
	// Garbage collection
	++m_nGCCounter;
	if (m_nGCCounter < c_nGCRateCounter)
		return;
	m_nGCCounter = 0;

	m_StatScenarios.remove_if([](auto& pWeak) {
		return pWeak.lock() == nullptr;
	});
}


//
//
//
Variant ScenarioManager::execute(Variant vCommand, Variant vParams /*= Variant()*/)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	// TODO: call command updateScenarios on updating

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### updateScenarios()
	/// Update scenarios from config.
	///
	if (vCommand == "updateScenarios")
	{
		updateScenarios();
		return {};
	}

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### start()
	/// start scenarios processing
	///
	else if (vCommand == "start")
	{
		start();
		return {};
	}

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### stop()
	/// Stop scenarios processing
	///
	else if (vCommand == "stop")
	{
		stop();
		return {};
	}

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### getStatistic()
	/// Returns statistic
	///   * showLineInfo [bool, opt]
	///
	else if (vCommand == "getStatistic")
	{
		return getStatInfo(vParams.get("showLineInfo", false));
	}

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### getStatistic()
	/// Returns statistic
	///   * showLineInfo [bool, opt]
	///
	else if (vCommand == "dumpStatistic")
	{
		Variant vInfo = getStatInfo(vParams.get("showLineInfo", false));
		if(!vInfo.isEmpty())
			LOGLVL(Critical, "\n" << vInfo << "\n");
		return {};
	}

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### enableStatistic()
	/// Enables or disables statistic collection
	///   * value [bool]
	///
	else if (vCommand == "enableStatistic")
	{
		return enableCollectStatInfo(vParams.get("value"));
	}


	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));

	error::OperationNotSupported(SL,
		FMT("ScenarioManager doesn't support command <" << vCommand << ">")).throwException();
}

} // namespace openEdr 
