//
// edrav2.libcore.unittest
//
// Data generator objects test 
//
// Autor: Denis Bogdanov (08.03.2019)
// Reviewer: ???
//
#include "pch.h"
#include "common.h"

using namespace openEdr;

//
//
//
TEST_CASE("RandomDataGenerator.create")
{
	(void)initTest();

	auto fnScenario = [](const Variant& vConfig)
	{
		ObjPtr<ICommandProcessor> pCurrGen;
		auto pObj = createObject(CLSID_RandomDataGenerator, vConfig);
		pCurrGen = queryInterface<ICommandProcessor>(pObj);
	};

	SECTION("default")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({})));
	}

	SECTION("with_params")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({{"startValue", 100}})));
	}

	SECTION("invalid_config_format")
	{
		REQUIRE_THROWS_AS(fnScenario(Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("RandomDataGenerator.getNext")
{
	(void)initTest();

	auto fnScenario = [](const Variant& vConfig, Size nData)
	{
		auto pCurrGen = createTestObject<ICommandProcessor>
			(CLSID_RandomDataGenerator, vConfig);
		Size nVal = c_nMaxSize;
		REQUIRE_NOTHROW(nVal = pCurrGen->execute("getNext"));
		REQUIRE(nVal == nData);
		REQUIRE_NOTHROW(nVal = pCurrGen->execute("getNext"));
		REQUIRE(nVal == nData + 1);
	};

	SECTION("default")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary(), 0));
	}

	SECTION("with_params")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"start", 100} }), 100));
	}

}


//
//
//
TEST_CASE("RandomDataGenerator.getIsoDateTime")
{
	(void)initTest();

	SECTION("default")
	{
		auto pCurrGen = createTestObject<ICommandProcessor>
			(CLSID_RandomDataGenerator, Dictionary());
		for (int i = 0; i < 50; ++i)
		{
			std::string sVal = pCurrGen->execute("getIsoDateTime");
			REQUIRE(sVal.size() == 24);
			REQUIRE(sVal[0] == '2');
			REQUIRE(sVal[23] == 'Z');
		}
	}
}

//
//
//
TEST_CASE("RandomDataGenerator.getDateTime")
{
	(void)initTest();

	SECTION("default")
	{
		auto pCurrGen = createTestObject<ICommandProcessor>
			(CLSID_RandomDataGenerator, Dictionary());
		uint64_t tNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		for (int i = 0; i < 50; ++i)
		{
			uint64_t tVal = pCurrGen->execute("getDateTime");
			REQUIRE(tVal - (tNow*1000) < 10000);
		}
	}

	SECTION("with_positive_shift")
	{
		auto pCurrGen = createTestObject<ICommandProcessor>
			(CLSID_RandomDataGenerator, Dictionary());
		uint64_t tNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		for (int i = 0; i < 50; ++i)
		{
			uint64_t tVal = pCurrGen->execute("getDateTime", Dictionary({
					{"shift", 3000}
				}));
			REQUIRE(tVal - (tNow * 1000) < 10000);
			REQUIRE(tVal - (tNow * 1000) >= 3000);
		}
	}

	SECTION("with_negative_shift")
	{
		auto pCurrGen = createTestObject<ICommandProcessor>
			(CLSID_RandomDataGenerator, Dictionary());
		uint64_t tNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		for (int i = 0; i < 50; ++i)
		{
			uint64_t tVal = pCurrGen->execute("getDateTime", Dictionary({
					{"shift", -3000}
				}));
			REQUIRE((tNow * 1000) - tVal <= 3000);
		}
	}
}

//
//
//
TEST_CASE("RandomDataGenerator.select")
{
	(void)initTest();

	ObjPtr<ICommandProcessor> pCurrGen = queryInterface<ICommandProcessor>
		(createObject(CLSID_RandomDataGenerator, Dictionary()));

	SECTION("integer")
	{
		for (int i = 0; i < 50; ++i)
		{
			Size nVal = 1;
			REQUIRE_NOTHROW(nVal = pCurrGen->execute("select", Dictionary({
				{"data", Sequence({100, 200, 300}) },
				})));
			REQUIRE((nVal == 100 || nVal == 200 || nVal == 300));
		}
	}

	SECTION("string")
	{
		for (int i = 0; i < 50; ++i)
		{
			std::string sVal;
			REQUIRE_NOTHROW(sVal = pCurrGen->execute("select", Dictionary({
				{"data", Sequence({"one", "two", "three"}) },
				})));
			REQUIRE((sVal == "one" || sVal == "two" || sVal == "three"));
		}
	}

	SECTION("dictionary")
	{
		for (int i = 0; i < 50; ++i)
		{
			Dictionary dctVal;
			REQUIRE_NOTHROW(dctVal = pCurrGen->execute("select", Dictionary({
				{"data", Sequence({
					Dictionary({ {"f1", "d1"}, {"f2", 1} }),
					Dictionary({ {"f1", "d2"}, {"f2", 2} }),
					Dictionary({ {"f1", "d3"}, {"f2", 3} }),
				})},
				})));
			REQUIRE((dctVal.get("f2") == 1 || dctVal.get("f2") == 2 || dctVal.get("f2") == 3));
		}
	}
}

//
//
//
TEST_CASE("RandomDataGenerator.generate")
{
	(void)initTest();

	ObjPtr<ICommandProcessor> pCurrGen = queryInterface<ICommandProcessor>
		(createObject(CLSID_RandomDataGenerator, Dictionary()));

	SECTION("integer")
	{
		for (int i = 0; i < 50; ++i)
		{
			Size nVal = 1;
			REQUIRE_NOTHROW(nVal = pCurrGen->execute("generate", Dictionary({
				{"begin", 100},
				{"end", 200},
				})));
			REQUIRE(nVal >= 100);
			REQUIRE(nVal < 199);
		}
	}

	SECTION("string")
	{
		for (int i = 0; i < 50; ++i)
		{
			std::string sVal;
			REQUIRE_NOTHROW(sVal = pCurrGen->execute("generate", Dictionary({
				{"size", 8},
				})));
			REQUIRE(sVal.size() == 8);
		}
	}
}