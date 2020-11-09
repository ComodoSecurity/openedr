//
// edrav2.libcore.unittest
//
// Cache objects test 
//
// Autor: Yury Ermakov (05.06.2019)
// Reviewer: 
//
#include "pch.h"
#include "common.h"

using namespace openEdr;

//
//
//
TEST_CASE("StringCache.put")
{
	auto fnScenario = [&](const Variant& vConfig, const Variant& vParams)
	{
		auto pObj = queryInterface<ICommandProcessor>(createObject(CLSID_StringCache, vConfig));
		(void)pObj->execute("put", vParams);
	};

	const std::map<const std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vConfig, vParams
		{ "good", { {}, Dictionary({ { "data", "<some-string-value>" } }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vParams] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vParams));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBad{
		// sName: sConfig, vParams
		{ "null", {	"String keys are not applicable for type <Null>", {}, {} }	}
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vConfig, vParams] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig, vParams), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("StringCache.self_clearing")
{
	auto fnScenario = [&](const Variant& vConfig, const std::string& sValue)
	{
		auto pObj = queryInterface<ICommandProcessor>(createObject(CLSID_StringCache, vConfig));
		REQUIRE(!pObj->execute("contains", Dictionary({ { "data", sValue } })));
		(void)pObj->execute("put", Dictionary({ { "data", sValue } }));
		REQUIRE(pObj->execute("contains", Dictionary({ { "data", sValue } })));
		std::string sOtherValue = "<other-value>";
		(void)pObj->execute("put", Dictionary({ { "data", sOtherValue } }));
		REQUIRE(!pObj->execute("contains", Dictionary({ { "data", sValue } })));
		REQUIRE(pObj->execute("contains", Dictionary({ { "data", sOtherValue } })));
	};

	const std::map<const std::string, std::tuple<Variant, std::string>> mapData{
		// sName: vConfig, sValue
		{ "good", { Dictionary({ { "maxSize", 1 } }), "<some-string-value>" } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sValue] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sValue));
		}
	}
}
