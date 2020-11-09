//
//  EDRAv2. Unit test
//    Command subsystem test
//
#include "pch.h"

using namespace openEdr;

CMD_DECLARE_LIBRARY_CLSID(CLSID_CommandProcessorExecuteCounter, 0xF38770F2);

namespace test_scenariomgr {

//
//
//
class CommandProcessorExecuteCounter : public ObjectBase<CLSID_CommandProcessorExecuteCounter>, public ICommandProcessor
{
public:
	virtual Variant execute(Variant vCommand, Variant vParams) override
	{
		if (vCommand == "addCount")
		{
			g_nActionCount += (int)vParams;
			std::this_thread::sleep_for(std::chrono::seconds(0));
			return {};
		}

		error::InvalidArgument(SL).throwException();
	}

	static std::atomic_uint32_t g_nActionCount;

	static Variant createCmdDescr(uint32_t nCount)
	{
		return Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_CommandProcessorExecuteCounter}}) },
			{"command", "addCount"},
			{"params", nCount},
		});
	}

	static Variant createInvalidCmdDescr()
	{
		return Dictionary({
			{"processor", Dictionary({{"clsid", CLSID_CommandProcessorExecuteCounter}}) },
			{"command", "InvalidCommand"},
		});
	}
};
std::atomic_uint32_t CommandProcessorExecuteCounter::g_nActionCount = 0;

} // namespace test_scenariomgr

using namespace test_scenariomgr;

CMD_DEFINE_LIBRARY_CLASS(CommandProcessorExecuteCounter)


//
// Register tested ScenarioManager
// @param vScenariosDescr - scenarios descriptors (dict)
// @return service manager object
//
ObjPtr<IObject> registerScenarioManager(Variant vScenariosDescr)
{
	// ScenarioManager config
	Variant vScnarioManagerDescr = Dictionary({ 
		{"clsid", CLSID_ScenarioManager}, 
		{"collectStatistic", true},
		{"threadPools", Dictionary({
			{"high", Dictionary({ {"size", 3}, {"priority", "high"} })},
			{"low", Dictionary({ {"size", 3}, {"priority", "low"} })},
		})}
	});

	// global config
	Variant vConfig = Dictionary({
		{"objects", Dictionary({
			{"scenarioManager",  vScnarioManagerDescr},
		})},
		{"extern",  Dictionary({
			{"scenarios", vScenariosDescr},
		})}
	});

	// Set data info CatalogData
	putCatalogData("app", Dictionary({ {"config", vConfig} }));

	putCatalogData("objects", nullptr);
	putCatalogData("objects", createObject(CLSID_ServiceManager));

	return queryService("scenarioManager");
}

//
//
//
TEST_CASE("ScenarioManager.simple_run")
{
	LOGINF("");

	using CPEC = CommandProcessorExecuteCounter;

	Variant vScenarioCode = Sequence({ CPEC::createCmdDescr(1) });

	Variant vScenarios = Dictionary({
		{"scenario1",  Dictionary({ {"code", vScenarioCode}, {"threadPoolTag", "normal"} })},
	});


	auto pQueueNotify = queryInterface<IQueueNotificationAcceptor>(registerScenarioManager(vScenarios));
	auto pService = queryInterface<IService>(pQueueNotify);
	pService->start();

	CommandProcessorExecuteCounter::g_nActionCount = 0;
	pQueueNotify->notifyAddQueueData("scenario1");
	std::this_thread::sleep_for(std::chrono::seconds(1));
	REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1);
}


//
//
//
TEST_CASE("ScenarioManager.multiple_run")
{
	LOGINF("");

	using CPEC = CommandProcessorExecuteCounter;

	Variant vScenarios = Dictionary({
		{"scenario1",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(1) })}, {"threadPoolTag", "normal"} })},
		{"scenario2",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(10) })}, {"threadPoolTag", "high"} })},
		{"scenario3",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(100) })}, {"threadPoolTag", "low"} })},
		{"scenario4",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(1000) })} })},
	});


	auto pQueueNotify = queryInterface<IQueueNotificationAcceptor>(registerScenarioManager(vScenarios));
	auto pService = queryInterface<IService>(pQueueNotify);
	pService->start();

	CommandProcessorExecuteCounter::g_nActionCount = 0;

	static constexpr Size c_nRepeateCount = 5;
	std::string sScenarious[] = { "scenario1" , "scenario2" ,"scenario3" ,"scenario4" };
	for (Size i = 0; i < c_nRepeateCount; ++i)
		for (auto& sName : sScenarious)
			pQueueNotify->notifyAddQueueData(sName);

	std::this_thread::sleep_for(std::chrono::seconds(5));
	REQUIRE(CommandProcessorExecuteCounter::g_nActionCount == 1111 * c_nRepeateCount);
}

//
// Returns ScenarioMgr
//
ObjPtr<IObject> prepareStatisticTest()
{
	using CPEC = CommandProcessorExecuteCounter;

	Variant vScenarios = Dictionary({
		{"scenario1",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(1) })}, {"threadPoolTag", "normal"} })},
		{"scenario2",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(10) })}, {"threadPoolTag", "high"} })},
		{"scenario3",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(100) })}, {"threadPoolTag", "low"} })},
		{"scenario4",  Dictionary({ {"code", Sequence({ CPEC::createCmdDescr(1000) })} })},
		});

	auto pScenarioMgr = queryInterface<IService>(registerScenarioManager(vScenarios));
	pScenarioMgr->start();
	return pScenarioMgr;
}

//
//
//
void runStatisticTest(ObjPtr<IObject> pScenarioMgr, bool fEnableStatistic)
{
	(void)execCommand(pScenarioMgr, "enableStatistic", Dictionary({ {"value", fEnableStatistic} }));

	auto pQueueNotify = queryInterface<IQueueNotificationAcceptor>(pScenarioMgr);

	std::vector<std::tuple<const std::string, Size>> g_vecData = {
		// name, count
		{ "scenario1", 1 },
		{ "scenario2", 5 },
		{ "scenario3", 10 },
		{ "scenario4", 20 },
	};

	for (auto[sName, nCount] : g_vecData)
	{
		for (Size i = 0; i < nCount; ++i)
				pQueueNotify->notifyAddQueueData(sName);
	}

	std::this_thread::sleep_for(std::chrono::seconds(3));

	Variant vStat = execCommand(pQueueNotify, "getStatistic");

	if (!fEnableStatistic)
	{
		REQUIRE(vStat.isEmpty());
		return;
	}

	REQUIRE(!vStat.isEmpty());
	Variant vScenariosStat = vStat.get("total");
	REQUIRE(vScenariosStat.getSize() == 4);

	for (auto[sName, nCount] : g_vecData)
	{
		Size nStatCount = vScenariosStat[sName]["count"];
		CHECK(nStatCount == nCount);
	}

}

//
//
//
TEST_CASE("ScenarioManager.getStatistic")
{
	LOGINF("");
	auto pScenarioMgr = prepareStatisticTest();
	runStatisticTest(pScenarioMgr, true);
}

//
//
//
TEST_CASE("ScenarioManager.enableStatistic")
{
	LOGINF("");
	auto pScenarioMgr = prepareStatisticTest();
	runStatisticTest(pScenarioMgr, true);
	runStatisticTest(pScenarioMgr, false);
	runStatisticTest(pScenarioMgr, true);
	runStatisticTest(pScenarioMgr, false);
}

