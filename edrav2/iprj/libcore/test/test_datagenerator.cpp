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

const Size c_nSampleSeqSize = 5;
Sequence createSampleSeq()
{
	return Sequence({
				Dictionary({
					{"f_str", "str0"},
					{"f_int", 0}
				}),
				Dictionary({
					{"f_str", "str1"},
					{"f_int", 1}
				}),
				Dictionary({
					{"f_str", "str2"},
					{"f_int", 2}
				}),
				Dictionary({
					{"f_str", "str3"},
					{"f_int", 3}
				}),
				Dictionary({
					{"f_str", "str4"},
					{"f_int", 4}
				}),
		});
}

const Size c_nSampleStgSize = 5;
ObjPtr<IDataProvider> createSampleProvider(std::filesystem::path pathToData)
{
	auto sStgPath = pathToData / L"default";
	auto pInfo = createTestObject<IInformationProvider>(CLSID_FsDataStorage,
		Dictionary({ {"path", sStgPath.c_str()} }));
	REQUIRE(pInfo->getInfo().get("size", 0) == c_nSampleStgSize);
	return queryInterface<IDataProvider>(pInfo);
}

//
//
//
TEST_CASE("DataGenerator.create")
{
	auto pathToData = initTest();

	auto fnScenario = [](Variant vData)
	{
		auto pDataGen = createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({ {"data", vData} }));
		auto pInfo = queryInterface<IInformationProvider>(pDataGen);
		REQUIRE(pInfo->getInfo().get("count", 0) == 0);
		REQUIRE(pInfo->getInfo().get("maxCount", 0) == 0);
		REQUIRE(pInfo->getInfo().get("period", 0) == 0);
		REQUIRE(pInfo->getInfo().get("stopped", false));
	};

	SECTION("sequence_based")
	{
		REQUIRE_NOTHROW(fnScenario(
			Dictionary({ {"default", createSampleSeq() } })));
	}

	SECTION("dictionary_based")
	{
		REQUIRE_NOTHROW(fnScenario(
			Dictionary({ {"default", Dictionary({ {"item", 1} })}})));
	}

	SECTION("provider_based")
	{
		REQUIRE_NOTHROW(fnScenario(
			Dictionary({ {"default", createSampleProvider(pathToData)} })));
	}

	SECTION("empty_sets")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary()));
	}

	SECTION("empty_set")
	{
		REQUIRE_NOTHROW(fnScenario(
			Dictionary({ {"default", Dictionary()} })));
	}

	SECTION("no_data")
	{
		REQUIRE_THROWS_AS(createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({ {"count", 10} })), error::InvalidArgument);
	}

	SECTION("invalid_config_format")
	{
		REQUIRE_THROWS_AS(createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("DataGenerator.autostart")
{
	auto pathToData = initTest();

	auto fnScenario = [](Variant vSource, Size nMaxCount, std::string_view sName = "default") {
		auto pDataGen = createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({
				{"data", Dictionary({ {"default", vSource }})},
				{"start", sName}
			}));
		auto pInfo = queryInterface<IInformationProvider>(pDataGen);
		REQUIRE(pInfo->getInfo().get("count", 0) == 0);
		REQUIRE(pInfo->getInfo().get("maxCount", 0) == nMaxCount);
		REQUIRE(!pInfo->getInfo().get("stopped", true));
	};

	SECTION("autostart_sequence_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleSeq(), c_nSampleSeqSize));
	}

	SECTION("autostart_dictionary_based")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"item", 1} }), 1));
	}

	SECTION("autostart_provider_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleProvider(pathToData), c_nSampleStgSize));
	}

	SECTION("bad_start_name")
	{
		REQUIRE_THROWS_AS(fnScenario(createSampleSeq(), c_nSampleSeqSize, "bad"),
			error::NotFound);
	}
}

//
//
//
TEST_CASE("DataGenerator.stop")
{
	auto pathToData = initTest();

	auto fnScenario = [](Variant vSource, Size nCount) {
		auto pDataGen = createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({
				{"data", Dictionary({ {"default", vSource }})},
				}));
		auto pInfo = queryInterface<IInformationProvider>(pDataGen);
		pDataGen->start("default", nCount);
		REQUIRE(pInfo->getInfo().get("count", 0) == 0);
		REQUIRE(pInfo->getInfo().get("maxCount", 0) == nCount);
		REQUIRE(!pInfo->getInfo().get("stopped", true));

		auto pProvider = queryInterface<IDataProvider>(pDataGen);
		auto v = pProvider->get();
		REQUIRE(v.has_value());
		REQUIRE(v.value().get("f_int", 0) == 0);

		pDataGen->stop();
		v = pProvider->get();
		REQUIRE(!v.has_value());
		REQUIRE(pInfo->getInfo().get("count", 0) == 0);
		REQUIRE(pInfo->getInfo().get("maxCount", 0) == 0);
		REQUIRE(pInfo->getInfo().get("stopped", false));
	};

	SECTION("get_sequence_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleSeq(), c_nSampleSeqSize));
	}

	SECTION("get_dictionary_based")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"item", 1} }), 5));
	}

	SECTION("get_provider_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleProvider(pathToData), c_nSampleStgSize));
	}
}

//
//
//
TEST_CASE("DataGenerator.stop_timer")
{
	auto pathToData = initTest();

	auto fnScenario = [](Variant vSource, Size nCount) {
		auto pDataReceiver = createObject<TestDataReceiver>();
		auto pReceiver = queryInterface<IDataReceiver>(pDataReceiver);
		auto pDataGen = createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({
				{"data", Dictionary({ {"default", vSource }})},
				{"receiver", pReceiver},
				{"period", 100}
			}));
		auto pInfo = queryInterface<IInformationProvider>(pDataGen);
		pDataGen->start("default", nCount);
		REQUIRE(!pInfo->getInfo().get("stopped", true));
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		pDataGen->stop();
		REQUIRE(pInfo->getInfo().get("stopped", false));
		auto nOldCount = pDataReceiver->getCount();
		REQUIRE(nOldCount > 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		REQUIRE(pDataReceiver->getCount() == nOldCount);
	};

	SECTION("get_sequence_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleSeq(), c_nSampleSeqSize));
	}

	SECTION("get_dictionary_based")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"item", 1} }), 5));
	}

	SECTION("get_provider_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleProvider(pathToData), c_nSampleStgSize));
	}
}

class ErrorNoData : public std::exception {};

//
//
//
TEST_CASE("DataGenerator.get")
{
	auto pathToData = initTest();

	auto fnScenario = [](Variant vSource, Size nSize, Size nCount) {
		auto pDataGen = createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({
				{"data", Dictionary({ {"default", vSource }})},
			}));
		auto pInfo = queryInterface<IInformationProvider>(pDataGen);
		pDataGen->start("default", nCount);
		REQUIRE(pInfo->getInfo().get("count", 0) == 0);
		REQUIRE(pInfo->getInfo().get("maxCount", 0) == nCount);
		REQUIRE(!pInfo->getInfo().get("stopped", true));

		auto pProvider = queryInterface<IDataProvider>(pDataGen);
		for (Size n = 0; n < nCount; ++n)
		{
			auto v = pProvider->get();
			if (!v.has_value()) throw ErrorNoData();
			REQUIRE(v.value().get("f_int", 0) == n % nSize);
			REQUIRE(pInfo->getInfo().get("count", 0) == n + 1);
		}
		auto v = pProvider->get();
		REQUIRE(!v.has_value());
	};

	SECTION("get_sequence_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleSeq(), c_nSampleSeqSize, 
			c_nSampleSeqSize * 2));
	}

	SECTION("get_dictionary_based")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"item", 1} }), 1, 3));
	}

	SECTION("get_provider_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleProvider(pathToData), c_nSampleStgSize,
			c_nSampleStgSize));
	}

	SECTION("get_provider_based_overflow")
	{
		REQUIRE_THROWS_AS(fnScenario(createSampleProvider(pathToData), c_nSampleStgSize,
			c_nSampleStgSize * 2), ErrorNoData);
	}
}

//
//
//
TEST_CASE("DataGenerator.push")
{
	auto pathToData = initTest();

	auto fnScenario = [](Variant vSource, Size nCount, Size nRealCount, Size nPeriod) {
		auto pDataReceiver = createObject<TestDataReceiver>();
		auto pReceiver = queryInterface<IDataReceiver>(pDataReceiver);
		auto pDataGen = createTestObject<IDataGenerator>(CLSID_DataGenerator,
			Dictionary({
				{"data", Dictionary({ {"default", vSource }})},
				{"receiver", pReceiver},
				{"period", nPeriod}
			}));
		auto pInfo = queryInterface<IInformationProvider>(pDataGen);
		pDataGen->start("default", nCount);

		std::this_thread::sleep_for(std::chrono::milliseconds(200 + nPeriod));
		REQUIRE(pDataReceiver->getCount() > 0);
		std::this_thread::sleep_for(std::chrono::milliseconds(200 + nPeriod * nCount));
		REQUIRE(pDataReceiver->getCount() == nRealCount);
		std::this_thread::sleep_for(std::chrono::milliseconds(200 + nPeriod));
		REQUIRE(pDataReceiver->getCount() == nRealCount);
	};

	SECTION("push_sequence_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleSeq(), c_nSampleSeqSize * 2, 
			c_nSampleSeqSize * 2, 100));
	}

	SECTION("push_dictionary_based")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"item", 1} }), 3, 3, 100));
	}

	SECTION("push_provider_based")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleProvider(pathToData), 
			c_nSampleStgSize, c_nSampleStgSize, 100));
	}

	SECTION("push_provider_based_overflow")
	{
		REQUIRE_NOTHROW(fnScenario(createSampleProvider(pathToData), 
			c_nSampleStgSize + 2, c_nSampleStgSize, 100));
	}
}
