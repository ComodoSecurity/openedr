//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (23.02.2019)
// Reviewer:
//

#include "pch.h"
#include "../src/dictionarymixer.h"

using namespace openEdr;

//
//
//
TEST_CASE("DictionaryMixer.create")
{
	auto fnScenario = [&](Variant vConfig, Variant vEthalon)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		auto dict = Dictionary(pObj);
		REQUIRE(dict == vEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vConfig, vEthalon
		{ "blank", { Dictionary({ { "layers", Sequence({
				Dictionary()
			}) }, }),
			Dictionary(),
			} },
		{ "2layers", { Dictionary({ { "layers", Sequence({
				Dictionary({
					{ "a", 0 },
					{ "b", Dictionary({ { "b1", 1 } }) },
					{ "c", Dictionary({ { "c1", 11 }, { "c2", 2 } }) },
					}),
				Dictionary({
					{ "b", 2 },
					{ "c", Dictionary({ { "c1", 1 }, { "c3", 3 }, }) },
					{ "d", Dictionary({ { "d1", 1 }, { "d2", 3 }, }) },
					{ "e", 42 }
					}),
			}) }, }),
			Dictionary({
				{ "a", 0 },
				{ "b", Dictionary({ { "b1", 1 } }) },
				{ "c", Dictionary({ { "c1", 11 }, { "c2", 2 }, { "c3", 3 }, }) },
				{ "d", Dictionary({ { "d1", 1 }, { "d2", 3 } }) },
				{ "e", 42 }
				}),
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vEthalon));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBad{
		// sName: sMsg, vConfig, vEthalon
		// TODO: if behaviour of has() on non-dictionary will be changed
		{ "config_not_dictionary", { "has() is not applicable for current type <Integer>. Key: <layers>", 42, nullptr } },
		{ "missing_layers", { "Missing field <layers>", Dictionary(), nullptr } },
		{ "invalid_layers", { "Layers must be a sequence", Dictionary({ { "layers", 42 } }), nullptr } },
		{ "no_layers", { "No layers found", Dictionary({ { "layers", Sequence() } }), nullptr } },
		{ "invalid_layer", { "Layer must be a dictionary", Dictionary({ { "layers", Sequence({ 42 }) } }), nullptr } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vConfig, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig, vEthalon), sMsg);
		}
	}
}

//
//
//
TEST_CASE("DictionaryMixer.basic_operations")
{
	auto fnScenario = [&](Variant vConfig, Variant vEthalon)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		auto dict = Dictionary(pObj);
		dict.put("x", Dictionary({ { "x1", 1 }, { "x2", 2 }, { "x3", 3 } }));
		auto dictX = dict.get("x");
		REQUIRE(dictX.getSize() == 3);
		REQUIRE(dictX.erase("x2") == 1);
		REQUIRE(dictX.erase("x2") == 0);
		REQUIRE(dictX.getSize() == 2);
		variant::deleteByPath(dict, "x.x3");
		REQUIRE(dictX.getSize() == 1);
		dictX.put("x4", Dictionary({ { "x41", 41 } }));
		REQUIRE(dict == vEthalon);
	};

	const std::map<const std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vConfig, vEthalon
		{ "blank", { Dictionary({ { "layers", Sequence({
				Dictionary()
			}) }, }),
			Dictionary({ { "x", Dictionary({ { "x1", 1 }, { "x4", Dictionary({ { "x41", 41 } }) } }) }
				}),
			} },
		{ "2layers", { Dictionary({ { "layers", Sequence({
				Dictionary({ { "a", 42 } }),
				Dictionary({ { "b", 2 }, { "x", 42 }, { "y", 0 } }),
			}) }, }),
			Dictionary({
				{ "a", 42 },
				{ "b", 2 },
				{ "x", Dictionary({ { "x1", 1 }, { "x4", Dictionary({ { "x41", 41 } }) } }) },
				{ "y", 0 }
				}),
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("DictionaryMixer.putByPath")
{
	auto fnScenario = [&](Variant vConfig, std::string_view sPath, Variant vValue, Variant vEthalon)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		auto dictResult = Dictionary(pDictProxy);
		variant::putByPath(dictResult, sPath, vValue, /* fCreatePaths */ true);
		REQUIRE(dictResult == vEthalon);
	};

	const auto c_dictConfig = Dictionary({ { "layers", Sequence({
		Dictionary({
			{ "a", 42 },
			{ "x", Dictionary({ { "x1", 1 }	}) }
			}),
		Dictionary({
			{ "b", 2 },
			{ "x", 42 },
			{ "y", Dictionary({	{ "y1", 1 }	}) }
			}),
		}) }, });

	const std::map<const std::string, std::tuple<Variant, std::string, Variant, Variant>> mapData{
		// sName: vConfig, sPath, vValue, vEthalon
		{ "new_simple", { c_dictConfig,
			"z",
			2,
			Dictionary({
				{ "a", 42 },
				{ "b", 2 },
				{ "x", Dictionary({	{ "x1", 1 }	}) },
				{ "y", Dictionary({ { "y1", 1 } }) },
				{ "z", 2 }
				}),
			} },
		{ "new_intermediate_paths", { c_dictConfig,
			"z.z1.z11",
			2,
			Dictionary({
				{ "a", 42 },
				{ "b", 2 },
				{ "x", Dictionary({	{ "x1", 1 }	}) },
				{ "y", Dictionary({ { "y1", 1 } }) },
				{ "z", Dictionary({ { "z1", Dictionary({ { "z11", 2 } }) } })  }
				}),
			} },
		{ "update_l0", { c_dictConfig,
			"a",
			Sequence(),
			Dictionary({
				{ "a", Sequence() },
				{ "b", 2 },
				{ "x", Dictionary({	{ "x1", 1 }	}) },
				{ "y", Dictionary({ { "y1", 1 } }) },
				}),
			} },
		{ "new_l1", { c_dictConfig,
			"y.y2",
			2,
			Dictionary({
				{ "a", 42 },
				{ "b", 2 },
				{ "x", Dictionary({	{ "x1", 1 }	}) },
				{ "y", Dictionary({ { "y1", 1 }, { "y2", 2 } }) },
				}),
			} },
		{ "append_l1", { c_dictConfig,
			"y.y2",
			2,
			Dictionary({
				{ "a", 42 },
				{ "b", 2 },
				{ "x", Dictionary({	{ "x1", 1 }	}) },
				{ "y", Dictionary({ { "y1", 1 }, { "y2", 2 } }) },
				}),
			} },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sPath, vValue, vEthalon] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sPath, vValue, vEthalon));
		}
	}
}

//
//
//
TEST_CASE("DictionaryMixer.isEmpty")
{
	auto fnScenario = [&](Variant vConfig, std::string_view sPath, bool fExpected)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictMixer = queryInterface<variant::IRootDictionaryProxy>(pObj);
		pDictMixer->erase("x", "x1");
		pDictMixer->erase("y", "y1");
		auto fResult = pDictMixer->isEmpty(sPath);
		REQUIRE(fResult == fExpected);
	};

	const auto c_dictConfig = Dictionary({ { "layers", Sequence({
		Dictionary({
			{ "a", 42 },
			{ "x", Dictionary({ { "x1", 1 }	}) }
			}),
		Dictionary({
			{ "b", 2 },
			{ "x", 42 },
			{ "y", Dictionary({	{ "y1", 1 }	}) }
			}),
		}) }, });

	const std::map<const std::string, std::tuple<Variant, std::string, bool>> mapData{
		// sName: vConfig, sPath, fExpected
		{ "l0", { c_dictConfig, "x", true } },
		{ "l1", { c_dictConfig, "y", true } }
		};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sPath, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sPath, fExpected));
		}
	}
}

//
//
//
TEST_CASE("DictionaryMixer.put_with_full_unerase")
{
	auto fnScenario = [&](Variant vConfig)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		pDictProxy->erase("x");
		pDictProxy->put("x", Dictionary({ { "x1", 1 } }));
		auto dictResult = pDictProxy->get("x");
		REQUIRE(dictResult.getSize() == 1);
	};

	const auto c_dictConfig = Dictionary({ { "layers", Sequence({
		Dictionary({
			{ "a", 42 },
			{ "x", Dictionary({ { "x1", 1 }	}) }
			}),
		Dictionary({
			{ "b", 2 },
			{ "x", 42 },
			{ "y", Dictionary({	{ "y1", 1 }	}) }
			}),
		}) }, });

	REQUIRE_NOTHROW(fnScenario(c_dictConfig));
}

//
//
//
TEST_CASE("DictionaryMixer.clear")
{
	auto fnScenario = [&](Variant vConfig)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictProxy = queryInterface<variant::IDictionaryProxy>(pObj);
		pDictProxy->clear();
		REQUIRE(pDictProxy->getSize() == 0);
	};

	const auto c_dictConfig = Dictionary({ { "layers", Sequence({
		Dictionary({
			{ "a", 42 },
			{ "x", Dictionary({ { "x1", 1 }	}) }
			}),
		Dictionary({
			{ "b", 2 },
			{ "x", 42 },
			{ "y", Dictionary({	{ "y1", 1 }	}) }
			}),
		}) }, });

	REQUIRE_NOTHROW(fnScenario(c_dictConfig));
}

//
//
//
TEST_CASE("DictionaryMixer.erase")
{
	auto fnScenario = [&](Variant vConfig, std::string_view sPath, bool fExpected)
	{
		auto pObj = createObject(CLSID_DictionaryMixer, vConfig);
		auto pDictMixer = queryInterface<variant::IRootDictionaryProxy>(pObj);
		pDictMixer->erase("", "x");
		pDictMixer->erase("", "y");
		pDictMixer->erase("", "a");
		pDictMixer->erase("", "b");
		auto fResult = pDictMixer->isEmpty(sPath);
		REQUIRE(fResult == fExpected);
	};

	const auto c_dictConfig = Dictionary({ { "layers", Sequence({
		Dictionary({
			{ "a", 42 },
			{ "x", Dictionary({ { "x1", 1 }	}) }
			}),
		Dictionary({
			{ "b", 2 },
			{ "x", 42 },
			{ "y", Dictionary({	{ "y1", 1 }	}) }
			}),
		}) }, });

	const std::map<const std::string, std::tuple<Variant, std::string, bool>> mapData{
		// sName: vConfig, sPath, fExpected
		{ "l0", { c_dictConfig, "x", true } },
		{ "l0_only", { c_dictConfig, "a", true } },
		{ "l1", { c_dictConfig, "y", true } },
		{ "l1_only", { c_dictConfig, "b", true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sPath, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sPath, fExpected));
		}
	}
}
