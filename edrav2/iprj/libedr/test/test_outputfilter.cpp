//
// edrav2.ut_libedr project
//
// Author: Yury Ermakov (15.11.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"
#include "common.h"

using namespace cmd;

//
//
//
TEST_CASE("OutputFilter.create")
{
	auto fnScenario = [](Variant vConfig, Variant vParams)
	{
		auto pFilter = queryInterface<ICommandProcessor>(createObject(CLSID_OutputFilter, vConfig));
		(void)pFilter->execute("filter", vParams);
	};

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBad{
		// sName: sMsg, vConfig, vParams
		{ "no_events", { "Missing field <events>", Dictionary(), {} }},
		{ "wrong_events", { "Field <events> must be a dictionary",
			Dictionary({ {"events", 42} }), {} }},
		{ "wrong_group_value_type", { "Value must be sequence or string",
			Dictionary({ {"events", Dictionary({ {"xyz", 42 }})} }), {} }},
		{ "unknown_group", { "Unknown event group <z>",
			Dictionary({ {"events", Dictionary({ {"xy", "z" }})} }), {} }},
		{ "self_loop", { "Looping group <xyz>",
			Dictionary({ {"events", Dictionary({ {"xyz", "xyz"} })} }), {} } },
		{ "loop", { "Looping group <y>",
			Dictionary({ {"events", Dictionary({ {"x", "y"}, {"y", "z"}, {"z", "x"} })} }), {} } }
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
TEST_CASE("OutputFilter.group")
{
	auto fnScenario = [](Variant vConfig, Variant vData, Variant vExpected)
	{
		Variant vParams = Dictionary({
			{"type", vData["type"]},
			{"data", vData}
			});
		auto pFilter = queryInterface<ICommandProcessor>(createObject(CLSID_OutputFilter, vConfig));
		auto vResult = pFilter->execute("filter", vParams);
		REQUIRE(vResult == vExpected);
	};

	const auto c_vConfig = Dictionary({
		{"events", Dictionary({
			{"default", Sequence({
				"^type$",
				"^A1\\.A11$"
				})},
			{"EVENT2", Sequence({
				"^type$",
				"^A1\\.A12$"
				})},
			{"some_group", Sequence({
				"^type$",
				"^A2$"
				})},
			{"EVENT3", "some_group"}
			})}
		});

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vConfig, vData, vExpected
		{ "default", {
			c_vConfig,
			Dictionary({
				{"type", "EVENT1"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary({
				{"type", "EVENT1"},
				{"A1", Dictionary({ {"A11", 11} })},
				})
			}},
		{ "explicit", {
			c_vConfig,
			Dictionary({
				{"type", "EVENT2"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary({
				{"type", "EVENT2"},
				{"A1", Dictionary({ {"A12", 12} })},
				})
			}},
		{ "through", {
			c_vConfig,
			Dictionary({
				{"type", "EVENT3"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary({
				{"type", "EVENT3"},
				{"A2", "xyz"}
				})
			}},
		{ "empty", {
			Dictionary({ {"events", Dictionary()} }),
			Dictionary({
				{"type", "EVENT4"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary()
			}}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vData, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vData, vExpected));
		}
	}
}

//
//
//
TEST_CASE("OutputFilter.filter")
{
	auto fnScenario = [](Variant vConfig, Variant vParams, Variant vExpected = {})
	{
		auto pFilter = queryInterface<ICommandProcessor>(createObject(CLSID_OutputFilter, vConfig));
		auto vResult = pFilter->execute("filter", vParams);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vDefault, vData, vExpected
		{ "one_field", {
			Sequence({ "^A1\\.A11$" }),
			Dictionary({
				{"type", "EVENT1"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary({ {"A1", Dictionary({ {"A11", 11} })} })
			}},
		{ "many_fields", {
			Sequence({ "^A1\\.A.+" }),
			Dictionary({
				{"type", "EVENT1"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary({ {"A1", Dictionary({ {"A11", 11}, {"A12", 12} })} })
			}},
		{ "nothing", {
			Sequence({ "^XYZ$" }),
			Dictionary({
				{"type", "EVENT1"},
				{"A1", Dictionary({ {"A11", 11}, {"A12", 12} })},
				{"A2", "xyz"}
				}),
			Dictionary()
			}},
		{ "check_bounds", {
			Sequence({ "^type$" }),
			Dictionary({
				{"type", "EVENT1"},
				{"A2typeA2", "xyz"},
				{"A2type", "xyz"},
				{"typeA2", "xyz"}
				}),
			Dictionary({ {"type", "EVENT1"} })
			}},
		{ "multi_level", {
			Sequence({
				"^process\\.id$",
				"^process\\.(parent\\.){1,2}id$"
				}),
			Dictionary({
				{"type", "EVENT1"},
				{"process", Dictionary({
					{"id", 1},
					{"parent", Dictionary({
						{"id", 2},
						{"parent", Dictionary({
							{"id", 3},
							{"parent", Dictionary({
								{"id", 4},
								{"parent", {}}
								})}
							})}
						})}
					})}
				}),
			Dictionary({
				{"process", Dictionary({
					{"id", 1},
					{"parent", Dictionary({
						{"id", 2},
						{"parent", Dictionary({
							{"id", 3}
							})}
						})}
					})}
				})
			}}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vDefault, vData, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(
				Dictionary({ {"events", Dictionary({ {"default", vDefault} })} }),
				Dictionary({ {"type", vData["type"]}, {"data", vData} }),
				vExpected));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBad{
		// sName: sMsg, vConfig, vParams
		{ "no_type", { "Missing field <type>", Dictionary({ {"events", Dictionary()} }), Dictionary() }},
		{ "no_data", { "Missing field <data>",
			Dictionary({ {"events", Dictionary()} }), Dictionary({ {"type", "xyz"} }) }}
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
