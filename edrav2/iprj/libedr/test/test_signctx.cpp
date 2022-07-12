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
using namespace cmd::edr;

//
//
//
TEST_CASE("ContextService.createObject")
{
	auto obj = createObject(CLSID_ContextService);
	auto pService = queryInterface<IService>(obj);
	auto pCommandProcessor = queryInterface<ICommandProcessor>(obj);
	obj.reset();
}

static const char g_sBasketName1[] = "BasketName1";
static const char g_sBasketName2[] = "BasketName2";
static const char g_sBasketName3[] = "BasketName3";

const Size g_Key1 = 12345;
const char g_Key2[] = "12345";
const Variant g_Key3 = Dictionary({ {"a", 1}, {"id", 67890}, {"b", 2} });

const Variant g_Group1 = Sequence({ {g_Key1, g_Key2} });
const Variant g_Group2 = Sequence({ {g_Key2, g_Key3} });

const Size g_nDefaultTimeout = 60 * 1000; // 1 min

auto vData1 = Dictionary({
	{"a", 123},
	{"b", "123"},
	{"c", true}
});

auto vData2 = Dictionary({
	{"a", 456},
	{"b", "456"},
	{"c", false}
});

auto vData3 = Dictionary({
	{"a", 789},
	{"b", "789"},
	{"c", Dictionary({})}
});

//
void saveKeys(ObjPtr<IObject> obj)
{
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key1},
		{"data", vData1},
		{"group", g_Group1},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key2},
		{"data", vData1},
		{"group", g_Group2},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key3},
		{"data", vData1},
		{"group", g_Group1},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", Sequence({ {g_Key1, g_Key2} })},
		{"data", vData1},
		{"timeout", g_nDefaultTimeout},
	}));
}

//
void saveMultikey(ObjPtr<IObject> obj)
{
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key1},
		{"data", vData1},
		{"limit", 10},
		{"group", g_Group1},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key1},
		{"data", vData2},
		{"limit", 10},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key1},
		{"data", vData3},
		{"limit", 10},
		{"group", g_Group1},
		{"timeout", g_nDefaultTimeout},
	}));
}

//
void saveBackets(ObjPtr<IObject> obj)
{
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName1},
		{"key", g_Key1},
		{"data", vData1},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName2},
		{"key", g_Key1},
		{"data", vData1},
		{"timeout", g_nDefaultTimeout},
	}));
	(void)execCommand(obj, "save", Dictionary({
		{"name", g_sBasketName3},
		{"key", g_Key1},
		{"data", vData1},
		{"timeout", g_nDefaultTimeout},
	}));
}

//
//
//
TEST_CASE("ContextService.save")
{
	auto obj = createObject(CLSID_ContextService);

	SECTION("keys")
	{
		REQUIRE_NOTHROW(saveKeys(obj));
	
		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 4);
		REQUIRE(vResult["baskets"][0]["keys"] == 4);
	}

	SECTION("multikey")
	{
		REQUIRE_NOTHROW(saveMultikey(obj));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 3);
		REQUIRE(vResult["baskets"][0]["keys"] == 1);
	}

	SECTION("baskets")
	{
		REQUIRE_NOTHROW(saveBackets(obj));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 3);
		REQUIRE(vResult["baskets"][0]["size"] == 1);
		REQUIRE(vResult["baskets"][0]["keys"] == 1);
		REQUIRE(vResult["baskets"][1]["size"] == 1);
		REQUIRE(vResult["baskets"][1]["keys"] == 1);
		REQUIRE(vResult["baskets"][2]["size"] == 1);
		REQUIRE(vResult["baskets"][2]["keys"] == 1);
	}

	obj.reset();
}

//
//
//
TEST_CASE("ContextService.has")
{
	auto obj = createObject(CLSID_ContextService);

	SECTION("keys")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		REQUIRE(execCommand(obj, "has", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
		})) == true);

		REQUIRE(execCommand(obj, "has", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key2},
		})) == true);

		REQUIRE(execCommand(obj, "has", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key3},
		})) == true);

		REQUIRE(execCommand(obj, "has", Dictionary({
			{"name", g_sBasketName1},
			{"key", Sequence({ {g_Key1, g_Key2} })},
		})) == true);

		REQUIRE(execCommand(obj, "has", Dictionary({
			{"name", g_sBasketName1},
			{"key", "aabbcc"},
		})) == false);
	}

	obj.reset();
}

//
//
//
TEST_CASE("ContextService.load")
{
	auto obj = createObject(CLSID_ContextService);

	SECTION("keys")
	{
		REQUIRE_NOTHROW(saveKeys(obj));
		
		auto vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
			}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key2},
			}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key3},
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", Sequence({ {g_Key1, g_Key2} })},
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 4);
		REQUIRE(vResult["baskets"][0]["keys"] == 4);
	}

	SECTION("keysWithDelete")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		auto vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
			{"delete", true}
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key2},
			{"delete", true}
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 2);
		REQUIRE(vResult["baskets"][0]["keys"] == 2);
	}

	SECTION("multikey")
	{
		REQUIRE_NOTHROW(saveMultikey(obj));

		auto vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 3);
		REQUIRE(vData[0] == vData1);
		REQUIRE(vData[1] == vData2);
		REQUIRE(vData[2] == vData3);

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 3);
		REQUIRE(vResult["baskets"][0]["keys"] == 1);
	}

	SECTION("multikeyWithDelete")
	{
		REQUIRE_NOTHROW(saveMultikey(obj));

		auto vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
			{"delete", true}
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 3);
		REQUIRE(vData[0] == vData1);
		REQUIRE(vData[1] == vData2);
		REQUIRE(vData[2] == vData3);

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 0);
		REQUIRE(vResult["baskets"][0]["keys"] == 0);
	}

	SECTION("baskets")
	{
		REQUIRE_NOTHROW(saveBackets(obj));

		auto vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName2},
			{"key", g_Key1},
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);

		vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName3},
			{"key", g_Key1},
		}));
		REQUIRE(vData.isSequenceLike());
		REQUIRE(vData.getSize() == 1);
		REQUIRE(vData[0] == vData1);
	}

	SECTION("invalid")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		auto vData = execCommand(obj, "load", Dictionary({
			{"name", g_sBasketName1},
			{"key", "aabbcc"},
		}));
		REQUIRE(vData.isNull());
	}

	obj.reset();
}

TEST_CASE("ContextService.remove")
{
	auto obj = createObject(CLSID_ContextService);

	SECTION("keys")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
		})));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key2},
		})));
		
		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 2);
		REQUIRE(vResult["baskets"][0]["keys"] == 2);
	}

	SECTION("multikey")
	{
		REQUIRE_NOTHROW(saveMultikey(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"key", g_Key1},
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 0);
		REQUIRE(vResult["baskets"][0]["keys"] == 0);
	}

	SECTION("groupKeys")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"group", g_Group1},
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 4);
		REQUIRE(vResult["baskets"][0]["keys"] == 4);

		REQUIRE_NOTHROW(execCommand(obj, "purge", Dictionary({
			{"name", g_sBasketName1},
		})));

		vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 2);
		REQUIRE(vResult["baskets"][0]["keys"] == 2);
	}

	SECTION("groupMultikey")
	{
		REQUIRE_NOTHROW(saveMultikey(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"group", g_Group1},
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 3);
		REQUIRE(vResult["baskets"][0]["keys"] == 1);

		REQUIRE_NOTHROW(execCommand(obj, "purge", Dictionary({
			{"name", g_sBasketName1},
		})));

		vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 1);
		REQUIRE(vResult["baskets"][0]["keys"] == 1);
	}

	SECTION("groupKeysForce")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"group", g_Group1},
			{"force", true},
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 2);
		REQUIRE(vResult["baskets"][0]["keys"] == 2);
	}

	SECTION("groupMultikeyForce")
	{
		REQUIRE_NOTHROW(saveMultikey(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"group", g_Group1},
			{"force", true},
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 1);
		REQUIRE(vResult["baskets"][0]["keys"] == 1);
	}

	SECTION("baskets")
	{
		REQUIRE_NOTHROW(saveBackets(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1}
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 3);
		REQUIRE(vResult["baskets"][0]["size"] == 0);
		REQUIRE(vResult["baskets"][0]["keys"] == 0);
		REQUIRE(vResult["baskets"][1]["size"] == 1);
		REQUIRE(vResult["baskets"][1]["keys"] == 1);
		REQUIRE(vResult["baskets"][2]["size"] == 1);
		REQUIRE(vResult["baskets"][2]["keys"] == 1);
	}

	SECTION("all")
	{
		REQUIRE_NOTHROW(saveBackets(obj));

		REQUIRE_THROWS_AS(execCommand(obj, "remove", Dictionary({})), error::InvalidArgument);

/*
		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 3);
		REQUIRE(vResult["baskets"][0]["size"] == 0);
		REQUIRE(vResult["baskets"][0]["keys"] == 0);
		REQUIRE(vResult["baskets"][1]["size"] == 0);
		REQUIRE(vResult["baskets"][1]["keys"] == 0);
		REQUIRE(vResult["baskets"][2]["size"] == 0);
		REQUIRE(vResult["baskets"][2]["keys"] == 0);
*/
	}

	SECTION("invalid")
	{
		REQUIRE_NOTHROW(saveKeys(obj));

		REQUIRE_NOTHROW(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"key", "aabbcc"},
		})));

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == 4);
		REQUIRE(vResult["baskets"][0]["keys"] == 4);
	}

	obj.reset();
}

//
//
//
TEST_CASE("ContextService.purge")
{
	auto obj = createObject(CLSID_ContextService);

	SECTION("moreThanLimit")
	{
		const Size nCount = 15000; // Must be more than basket limit
		for (Size i = 0; i < nCount; ++i)
		{
			REQUIRE_NOTHROW(execCommand(obj, "save", Dictionary({
				{"name", g_sBasketName1},
				{"key", i},
				{"data", (i % 2) ? vData1.clone() : vData2.clone()},
				{"group", g_Group1},
				{"timeout", g_nDefaultTimeout},
			})));
		}

		Variant vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == nCount);
		REQUIRE(vResult["baskets"][0]["keys"] == nCount);

/*
		REQUIRE(execCommand(obj, "remove", Dictionary({
			{"name", g_sBasketName1},
			{"group", g_Group1},
		})) == true);

		vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		REQUIRE(vResult["baskets"][0]["size"] == nCount);
		REQUIRE(vResult["baskets"][0]["keys"] == nCount);
*/

		REQUIRE_NOTHROW(execCommand(obj, "purge", Dictionary({
			{"name", g_sBasketName1},
		})));

		vResult = execCommand(obj, "getStatistic");
		REQUIRE(vResult["baskets"].getSize() == 1);
		// Result size must be around (nCount / 2) 
		Size nResultCount = vResult["baskets"][0]["size"];
		REQUIRE(nResultCount > nCount / 2 - 500);
		REQUIRE(nResultCount < nCount / 2 + 500);
	}

	obj.reset();
}