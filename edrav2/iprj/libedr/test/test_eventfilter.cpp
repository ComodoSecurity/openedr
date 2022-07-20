//
// edrav2.ut_libedr project
//
// Author: Yury Ermakov (16.04.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"
#include "common.h"

using namespace cmd;

namespace {

//
// ProcessDataProvider for this test
//
class TestProcessDataProvider : public ObjectBase <>, 
	public sys::win::IProcessInformation
{
public:
	typedef sys::win::ProcId ProcId;
	typedef sys::win::Pid Pid;

private:

	ProcId convertProcId(Pid pid)
	{
		return ~ProcId(pid);

		// Unique ProcId

		//static std::atomic_uint32_t g_nCounter = 0;
		//
		//uint32_t nCounter = ++g_nCounter;
		//
		//crypt::xxh::Hasher calc;
		//crypt::updateHash(calc, nCounter);
		//crypt::updateHash(calc, pid);
		//
		//return ProcId(calc.finalize());
	}

public:

	//
	//
	//
	Variant enrichProcessInfo(Variant vParams) override
	{
		return getProcessInfoByPid(vParams["pid"]);
	}

	//
	//
	//
	virtual Variant getProcessInfoById(ProcId id) override
	{
		return Dictionary({
			{"id", id},
		});
	}

	//
	//
	//
	virtual Variant getProcessInfoByPid(Pid pid) override
	{
		return Dictionary({
			{"id", convertProcId(pid)},
			{"pid", pid},
		});
	}

	static void createService()
	{
		auto pThis = createObject<TestProcessDataProvider>();
		putCatalogData("objects.processDataProvider", pThis);
	}

	static void deleteService()
	{
		putCatalogData("objects.processDataProvider", nullptr);
	}
};

//
//
//
static Variant createEvent(Event eType, Time nTickTime, uint64_t nPid, Variant vOthers = {})
{
	Variant vEvent = Dictionary({
		{"process", Dictionary({ {"pid", nPid} })},
		{"tickTime", nTickTime},
		{"baseType", eType},
		});

	if (!vOthers.isNull())
		vEvent.merge(vOthers, Variant::MergeMode::All);

	return vEvent;
}

struct FilterRule
{
	Event eType;
	Time nTimeout;
	std::vector<Variant> hashedFields;
};

//
//
//
static ObjPtr<IQueueFilter> createFilter(const std::vector<FilterRule>& rules, Variant vOthers = {})
{
	Sequence vRules;
	for (auto& rule : rules)
	{
		Sequence vHashedFields;
		for (auto& hashedField : rule.hashedFields)
			vHashedFields.push_back(hashedField);

		Dictionary vRule({
			{"type", rule.eType},
			{"timeout", rule.nTimeout},
			{"hashedFields", vHashedFields},
		});
		vRules.push_back(vRule);
	}


	Variant vConfig = Dictionary({
		{"events", vRules},
	});
	if (!vOthers.isNull())
		vConfig.merge(vOthers, Variant::MergeMode::All);

	return queryInterface<IQueueFilter> (createObject(CLSID_PerProcessEventFilter, vConfig));
}

struct EventInfo
{
	Variant vEvent;
	bool fSkipped;
};

template<size_t N>
static void testEventFiltering(ObjPtr<IQueueFilter> pFilter, EventInfo(&events)[N])
{
	for (size_t i = 0; i < std::size(events); ++i)
	{
		auto& event = events[i];
		CAPTURE(i, event.vEvent);
		bool fResult = pFilter->filterElem(event.vEvent);
		REQUIRE(fResult == event.fSkipped);
	}
}

}

//
//
//
TEST_CASE("PerProcessEventFilter.finalConstruct_events_type")
{
	TestProcessDataProvider::createService();

	SECTION("with_rule")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1), false},
			{createEvent(Event(1), 2, 1), true},
		};

		auto pFilter = createFilter({
			{Event(1), 10000, { "baseType" } }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("without_rule")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1), false},
			{createEvent(Event(1), 2, 1), true},
			{createEvent(Event(2), 3, 1), false},
			{createEvent(Event(2), 4, 1), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10000, { "baseType" } }
		});

		testEventFiltering(pFilter, events);
	}
}

//
//
//
TEST_CASE("PerProcessEventFilter.finalConstruct_events_timeout")
{
	TestProcessDataProvider::createService();

	SECTION("unexpired")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1), false},
			{createEvent(Event(1), 2, 1), true},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { "baseType" } }
			});

		testEventFiltering(pFilter, events);
	}

	SECTION("expired")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1), false},
			{createEvent(Event(1), 9, 1), true},
			{createEvent(Event(1), 11, 1), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { "baseType" } }
			});

		testEventFiltering(pFilter, events);
	}

	SECTION("_0")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1), false},
			{createEvent(Event(1), 2, 1), true},
			{createEvent(Event(1), 20, 1), true},
			{createEvent(Event(1), 200, 1), true},
			{createEvent(Event(1), 2000, 1), true},
			{createEvent(Event(1), 20000, 1), true},
			{createEvent(Event(1), 200000, 1), true},
		};

		auto pFilter = createFilter({
			{Event(1), 0, { "baseType" } }
			});

		testEventFiltering(pFilter, events);
	}

	SECTION("other_process")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1), false},
			{createEvent(Event(1), 2, 2), false},
			{createEvent(Event(1), 3, 1), true},
			{createEvent(Event(1), 4, 2), true},
			{createEvent(Event(1), 20, 1), false},
			{createEvent(Event(1), 21, 2), false},
			{createEvent(Event(1), 22, 1), true},
			{createEvent(Event(1), 41, 2), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { "baseType" } }
			});

		testEventFiltering(pFilter, events);
	}
}


//
//
//
TEST_CASE("PerProcessEventFilter.finalConstruct_events_hashedFields")
{
	TestProcessDataProvider::createService();

	SECTION("string_and_path_is_present")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"a", 1}})), true},
			{createEvent(Event(1), 3, 1, Dictionary({{"a", 2}})), false},
			{createEvent(Event(1), 4, 1, Dictionary({{"a", 3}})), false},
			{createEvent(Event(1), 5, 1, Dictionary({{"a", 2}})), true},
			{createEvent(Event(1), 22, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 23, 1, Dictionary({{"a", 2}})), false},
			{createEvent(Event(1), 24, 1, Dictionary({{"a", 3}})), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { "a" } }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("string_and_path_is_absent")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"b", 2}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"b", 2}})), false},
			{createEvent(Event(1), 3, 1, Dictionary({{"b", 2}})), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { "a" } }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("dict_and_path_is_present")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"a", 1}})), true},
			{createEvent(Event(1), 3, 1, Dictionary({{"a", 2}})), false},
			{createEvent(Event(1), 4, 1, Dictionary({{"a", 3}})), false},
			{createEvent(Event(1), 5, 1, Dictionary({{"a", 2}})), true},
			{createEvent(Event(1), 22, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 23, 1, Dictionary({{"a", 2}})), false},
			{createEvent(Event(1), 24, 1, Dictionary({{"a", 3}})), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { Dictionary({ {"path", "a"} }) } }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("dict_and_path_is_absent")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"b", 2}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"b", 2}})), false},
			{createEvent(Event(1), 3, 1, Dictionary({{"b", 2}})), false},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { Dictionary({ {"path", "a"} }) } }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("dict_with_default")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"a", 5}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"a", 5}})), true},
			{createEvent(Event(1), 3, 1, Dictionary({{"a", 6}})), false},
			{createEvent(Event(1), 4, 1, Dictionary({{"b", 2}})), false},
			{createEvent(Event(1), 5, 1, Dictionary({{"a", 1}})), true},
			{createEvent(Event(1), 21, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 22, 1, Dictionary({{"b", 2}})), true},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { Dictionary({ {"path", "a"}, {"default", 1} }) } }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("empty")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"a", 1}})), true},
			{createEvent(Event(1), 3, 1, Dictionary({{"a", 2}})), true},
			{createEvent(Event(1), 4, 1, Dictionary({{"b", 2}})), true},
			{createEvent(Event(1), 21, 1, Dictionary({{"a", 1}})), false},
			{createEvent(Event(1), 22, 1, Dictionary({{"a", 2}})), true},
		};

		auto pFilter = createFilter({
			{Event(1), 10, {} }
		});

		testEventFiltering(pFilter, events);
	}

	SECTION("many_fields")
	{
		EventInfo events[] = {
			{createEvent(Event(1), 1, 1, Dictionary({{"a", 1}, {"b", 1}})), false},
			{createEvent(Event(1), 2, 1, Dictionary({{"a", 1}, {"b", 2}})), false},
			{createEvent(Event(1), 3, 1, Dictionary({{"a", 2}, {"b", 1}})), false},
			{createEvent(Event(1), 4, 1, Dictionary({{"a", 1}, {"b", 1}})), true},
			{createEvent(Event(1), 5, 1, Dictionary({{"a", 1}, {"b", 2}})), true},
			{createEvent(Event(1), 6, 1, Dictionary({{"a", 2}, {"b", 1}})), true},
			{createEvent(Event(1), 20, 1, Dictionary({{"a", 1}, {"b", 1}})), false},
			{createEvent(Event(1), 21, 1, Dictionary({{"a", 1}, {"b", 1}})), true},
		};

		auto pFilter = createFilter({
			{Event(1), 10, { "a", "b" } }
		});

		testEventFiltering(pFilter, events);
	}

}

//
//
//
TEST_CASE("PerProcessEventFilter.filterEvent_multythread")
{
	TestProcessDataProvider::createService();

	Variant events[] = {
		createEvent(Event(1), 1, 1, Dictionary({{"a", 1}, {"b", 1}})),
		createEvent(Event(2), 1, 1, Dictionary({{"a", 1}, {"b", 1}})),
		createEvent(Event(1), 1, 1, Dictionary({{"a", 1}, {"b", 2}})),
		createEvent(Event(2), 1, 1, Dictionary({{"a", 1}, {"b", 2}})),
		createEvent(Event(1), 1, 1, Dictionary({{"a", 2}, {"b", 1}})),
		createEvent(Event(2), 1, 1, Dictionary({{"a", 2}, {"b", 1}})),
		createEvent(Event(1), 1, 1, Dictionary({{"a", 3}, {"b", 1}})),
		createEvent(Event(2), 1, 1, Dictionary({{"a", 3}, {"b", 1}})),
		createEvent(Event(1), 1, 1, Dictionary({{"a", 3}, {"b", 2}})),
		createEvent(Event(2), 1, 1, Dictionary({{"a", 3}, {"b", 2}})),
		createEvent(Event(1), 1, 1, Dictionary({{"a", 2}, {"b", 3}})),
		createEvent(Event(2), 1, 1, Dictionary({{"a", 2}, {"b", 3}})),

		createEvent(Event(1), 1, 2, Dictionary({{"a", 1}, {"b", 1}})),
		createEvent(Event(2), 1, 2, Dictionary({{"a", 1}, {"b", 1}})),
		createEvent(Event(1), 1, 2, Dictionary({{"a", 1}, {"b", 2}})),
		createEvent(Event(2), 1, 2, Dictionary({{"a", 1}, {"b", 2}})),
		createEvent(Event(1), 1, 2, Dictionary({{"a", 2}, {"b", 1}})),
		createEvent(Event(2), 1, 2, Dictionary({{"a", 2}, {"b", 1}})),
		createEvent(Event(1), 1, 2, Dictionary({{"a", 3}, {"b", 1}})),
		createEvent(Event(2), 1, 2, Dictionary({{"a", 3}, {"b", 1}})),
		createEvent(Event(1), 1, 2, Dictionary({{"a", 3}, {"b", 2}})),
		createEvent(Event(2), 1, 2, Dictionary({{"a", 3}, {"b", 2}})),
		createEvent(Event(1), 1, 2, Dictionary({{"a", 2}, {"b", 3}})),
		createEvent(Event(2), 1, 2, Dictionary({{"a", 2}, {"b", 3}})),

		createEvent(Event(1), 1, 3, Dictionary({{"a", 1}, {"b", 1}})),
		createEvent(Event(2), 1, 3, Dictionary({{"a", 1}, {"b", 1}})),
		createEvent(Event(1), 1, 3, Dictionary({{"a", 1}, {"b", 2}})),
		createEvent(Event(2), 1, 3, Dictionary({{"a", 1}, {"b", 2}})),
		createEvent(Event(1), 1, 3, Dictionary({{"a", 2}, {"b", 1}})),
		createEvent(Event(2), 1, 3, Dictionary({{"a", 2}, {"b", 1}})),
		createEvent(Event(1), 1, 3, Dictionary({{"a", 3}, {"b", 1}})),
		createEvent(Event(2), 1, 3, Dictionary({{"a", 3}, {"b", 1}})),
		createEvent(Event(1), 1, 3, Dictionary({{"a", 3}, {"b", 2}})),
		createEvent(Event(2), 1, 3, Dictionary({{"a", 3}, {"b", 2}})),
		createEvent(Event(1), 1, 3, Dictionary({{"a", 2}, {"b", 3}})),
		createEvent(Event(2), 1, 3, Dictionary({{"a", 2}, {"b", 3}})),
	};

	auto pFilter = createFilter({
		{Event(1), 100, { "a", "b" } },
		{Event(2), 100, { "a", "b" } }
	});

	std::atomic_uint64_t nSkipCount = 0;
	std::atomic_uint64_t nPassCount = 0;

	auto fnScenario = [&]()
	{
		for (size_t i = 0; i < std::size(events); ++i)
		{
			auto& vEvent = events[i];
			bool fResult = pFilter->filterElem(vEvent);
			if (fResult)
				++nSkipCount;
			else
				++nPassCount;
		}
	};

	static constexpr size_t c_nThreadCount = 10;
	std::thread testThreads[c_nThreadCount];
	for (auto& testThread : testThreads)
		testThread = std::thread(fnScenario);

	for (auto& testThread : testThreads)
		testThread.join();


	CHECK(nPassCount == std::size(events));
	CHECK(nSkipCount == std::size(events)*(c_nThreadCount-1));
}


