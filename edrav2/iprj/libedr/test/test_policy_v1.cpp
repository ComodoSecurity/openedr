//
// edrav2.ut_libedr project
//
// Author: Yury Ermakov (16.04.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"
#include "common.h"

using namespace openEdr;
using namespace openEdr::edr::policy;


const std::string c_sCompilerConfig = R"json({
"objects": {},
"eventClasses": {},
"eventGroups": {},
"events": {}
})json";

//
//
//
Variant getRealCompilerConfig()
{
	const std::string_view sCompilerCfg = R"json({
			"eventGroups": {
				"default": {
					"fields": {
						"child_prcs_elevation_type": "event.process.token.elevationType",
						"file_hash": "event.file.hash",
						"file_old_path": {
							"value": "event.source.path",
							"useFileReplacer": true
						},
						//"filesize": "",
						"ftype": "event.file.type",
						//"isFolder": "",
						//"isPE": "",
						//"isSigned": "",
						//"newpath": "",
						//"newsize": "",
						//"newtype": "",
						"parentVerdict": {
							"value": "event.process.imageFile.verdict",
							"default": 404,
							"valueType": 2 // Integer
						},
						"parentProcessPath": {
							"value": "event.process.imageFile.abstractPath",
							"useFileReplacer": true
						},
						"path": {
							"value": "event.process.imageFile.abstractPath",
							"useFileReplacer": true
						},
						"prcs_cmd_line": "event.process.cmdLine",
						"targetName": {
							"value": "event.target.imageFile.abstractPath",
							"useFileReplacer": true
						},
						//"time": "",
						//"type": "",
						"verdict": {
							"value": "event.process.imageFile.verdict",
							"default": 404,
							"valueType": 2 // Integer
						}
					}
				},
				"process": {
					"baseGroup": "default",
					"fields": {
						"parentProcessPath": {
							"value": "event.process.parent.imageFile.abstractPath",
							"useFileReplacer": true
						},
						"parentVerdict": {
							"value": "event.process.parent.imageFile.verdict",
							"default": 404,
							"valueType": 2 // Integer
						}
					}
				},
				"file": {
					"baseGroup": "default",
					"fields": {
						"path": {
							"value": "event.file.abstractPath",
							"useFileReplacer": true
						}
					}
				},
				"registry": {
					"baseGroup": "default",
					"fields": {
						"path": {
							"value": "event.registry.abstractPath",
							"useRegistryReplacer": true,
							"caseInsensitive": true
						},
						"reg_value_data": "event.registry.data",
						"reg_value_name": {
							"value": "event.registry.name",
							"caseInsensitive": true
						}
					}
				}
			},
			"events": {
				"RP1": {
					"baseGroup": "process",
					"eventType": "LLE_PROCESS_CREATE"
				},
				"RP2": {
					"baseGroup": "process",
					"eventType": "LLE_PROCESS_DELETE"
				},
				// "RP3":
				"RF1": {
					"baseGroup": "file",
					"eventType": "LLE_FILE_CREATE"
				},
				"RF2": {
					"baseGroup": "file",
					"eventType": "LLE_FILE_CLOSE"
				},
				// "RF3":
				"RF4": {
					"baseGroup": "file",
					"eventType": "LLE_FILE_DELETE"
				},
				// "RF5":
				// "RF6":
				// "RF7":
				// "RF8":
				// "RF9":
				"RF10": {
					"baseGroup": "file",
					"eventType": "LLE_FILE_DATA_CHANGE"
				},
				"RF11": {
					"baseGroup": "file",
					"eventType": "MLE_FILE_COPY_FROM_SHARE"
				},
				"RF12": {
					"baseGroup": "file",
					"eventType": "MLE_FILE_COPY_FROM_USB"
				},
				"RF13": {
					"baseGroup": "file",
					"eventType": "MLE_FILE_COPY_TO_SHARE"
				},
				"RF14": {
					"baseGroup": "file",
					"eventType": "MLE_FILE_COPY_TO_USB"
				},
				"RR1": {
					"baseGroup": "registry",
					"eventType": "LLE_REGISTRY_KEY_CREATE"
				},
				"RR2": {
					"baseGroup": "registry",
					"eventType": "LLE_REGISTRY_KEY_DELETE"
				},
				"RR3": {
					"baseGroup": "registry",
					"eventType": "LLE_REGISTRY_VALUE_DELETE"
				},
				"RR4": {
					"baseGroup": "registry",
					"eventType": "LLE_REGISTRY_KEY_NAME_CHANGE"
				},
				"RR5": {
					"baseGroup": "registry",
					"eventType": "LLE_REGISTRY_VALUE_SET"
				},
				// "RN1":
				// "RN2":
				// "RB1":
				// "RB2":
				// "RB3":
				// "RB4":
				"RD6": {
					"baseGroup": "default",
					"eventType": [
						"LLE_DISK_RAW_WRITE_ACCESS",
						"LLE_VOLUME_RAW_WRITE_ACCESS",
						"LLE_DEVICE_RAW_WRITE_ACCESS",
						"LLE_DISK_LINK_CREATE",
						"LLE_VOLUME_LINK_CREATE",
						"LLE_DEVICE_LINK_CREATE"
					]
				},
				"RD7": {
					"baseGroup": "default",
					"eventType": [
						"LLE_KEYBOARD_GLOBAL_READ",
						"LLE_KEYBOARD_BLOCK",
						"LLE_CLIPBOARD_READ",
						"LLE_KEYBOARD_GLOBAL_WRITE"
					]
				},
				"RD8": {
					"baseGroup": "default",
					"eventType": [
						"LLE_MOUSE_GLOBAL_WRITE",
						"LLE_WINDOW_DATA_READ",
						"LLE_DESKTOP_WALLPAPER_SET",
						"LLE_MOUSE_BLOCK"
					]
				},
				"RD11": {
					"baseGroup": "default",
					"eventType": "LLE_WINDOW_PROC_GLOBAL_HOOK"
				},
				"RD15": {
					"baseGroup": "default",
					"eventType": [
						"LLE_PROCESS_MEMORY_READ",
						"LLE_PROCESS_MEMORY_WRITE",
					]
				},
				"RD16": {
					"baseGroup": "default",
					"eventType": [
						"LLE_MICROPHONE_ENUM",
						"LLE_MICROPHONE_READ"
					]
				},
				// "RD17":
				"RD20": {
					"baseGroup": "default",
					"eventType": "LLE_PROCESS_OPEN"
				}
			},

			"fieldValueTransform": {
				"ftype": {
					"PORTABLE_EXECUTABLE": "EXECUTABLE"
				}
			},

			"boolOperations": {
				"And": "and",
				"Or": "or"
			},

			"operations": {
				"Equal": "equal",
				"Match": "imatch",
				"MatchInList": {
					"value": "imatch",
					"isListValue": true
				},
				"!Equal": "!equal",
				"!Match": "!imatch",
				"!MatchInList": {
					"value": "!imatch",
					"isListValue": true
				}
			}
		})json";
	return variant::deserializeFromJson(sCompilerCfg);
}

//
// 
//
class CreatingEventCatcher : public ObjectBase<>,
	public IContextCommand,
	public IDataProvider
{
private:
	std::vector<Variant> m_events;
	Size m_nIndex = 0;

public:
	Variant execute(Variant vContext, Variant vParam) override
	{
		m_events.push_back(vParam["data"]);
		return {};
	}

	static ObjPtr<CreatingEventCatcher> create()
	{
		return createObject<CreatingEventCatcher>();
	}

	std::optional<Variant> get() override
	{
		if (m_events.empty() || (m_nIndex >= m_events.size()))
			return std::nullopt;
		return m_events[m_nIndex++];
	}
};

//
//
//
TEST_CASE("policy_v1.source")
{
	auto fnScenario = [](Variant vEvent, Variant vExpected)
	{
		auto vCompilerCfg = variant::deserializeFromJson(c_sCompilerConfig);
		vCompilerCfg.put("objects", variant::deserializeFromJson(R"json({
"event": {
	"optionalField": {
		"default": "<default>"
	}
}
})json"));
		vCompilerCfg.put("eventGroups", variant::deserializeFromJson(R"json({
"default": {
	"fields": {
		"policyFieldA": "event.fieldA",
		"policyFieldB": {
			"value": "event.optionalField",
			"valueType": 2 // Integer
		},
		"policyFieldC": {
			"value": "event.fieldC",
			"useFileReplacer": true
		}
	}
}
})json"));
		vCompilerCfg.put("events", variant::deserializeFromJson(R"json({
"EVENT1": {
	"baseGroup": "default",
	"eventType": "1"
}
})json"));

		auto vConfig = Dictionary({
			{"config", vCompilerCfg},
			{"policyCatalogPath", "app.config.policy"},
			{"verbose", true}
			});

		auto vSourcePolicy = Dictionary({
			//{"Lists"}
			{"Events", Dictionary({
				{"EVENT1", Sequence({
					Dictionary({
						{"BaseEventType", 1},
						{"EventType", {}},
						{"Condition", Dictionary({
							{"Field", "policyFieldA"},		// Mandatory field
							{"Operator", "Equal"},
							{"Value", "xyz"}
							})}
						}),
					Dictionary({
						{"BaseEventType", 2},
						{"EventType", {}},
						{"Condition", Dictionary({
							{"Field", "policyFieldB"},		// Optional field
							{"Operator", "Equal"},
							{"Value", "42"}
							})}
						}),
					Dictionary({
						{"BaseEventType", 3},
						{"EventType", {}},
						{"Condition", Dictionary({
							{"Field", "policyFieldXXX"},	// Unknown field
							{"Operator", "Equal"},
							{"Value", "42"}
							})}
						}),
					Dictionary({
						{"BaseEventType", 1},
						{"EventType", {}},
						{"Condition", Dictionary({
							{"Field", "policyFieldC"},		// File replacer
							{"Operator", "Match"},
							{"Value", "%windir%\\target_dir\\*"}
							})}
						})
					})}
				})}
			});

		putCatalogData("objects", Dictionary());
		putCatalogData("app.config.policy.groups.eventsMatching.active", Dictionary());
		putCatalogData("app.config.policy.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		auto vScenario = pCompiler->compile(PolicyGroup::EventsMatching,
			Dictionary({ {"evmCloud", vSourcePolicy} }), true /* fActivate */);
		auto sJson = variant::serializeToJson(vScenario);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);

		vContext.put("event", vEvent);
		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("app.config.policy.groups.eventsMatching.scenario"));
		(void)pScenario->execute(vContext);

		if (!vExpected.isNull())
		{
			auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
			REQUIRE(vResult == vExpected);
		}
		else
			REQUIRE(!queryInterface<IDataProvider>(pEventCatcher)->get().has_value());
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vEvent, vExpected
		{ "matched", {
			Dictionary({
				{"type", "1"},
				{"fieldA", "xyz"},
				{"fieldC", ""}
				}),
			Dictionary({
				{"type", "EVENT1.1"},
				{"fieldA", "xyz"},
				{"fieldC", ""},
				{"baseEventType", 1},
				{"eventType", {}}
				})
			}},
		{ "not_matched", {
			Dictionary({ {"type", "1"}, {"fieldA", "42"}, {"fieldC", ""} }),
			{}
			}},
		{ "type_mismatch", {
			Dictionary({ {"type", "1"}, {"fieldA", 42}, {"fieldC", ""} }),
			{}
			}},
		{ "filereplacer", {
			Dictionary({
				{"type", "1"},
				{"fieldA", "xyz"},
				{"fieldC", "%systemroot%\\target_dir\\some_file.exe"} 
				}),
			Dictionary({
				{"type", "EVENT1.1"},
				{"fieldA", "xyz"},
				{"fieldC", "%systemroot%\\target_dir\\some_file.exe"},
				{"baseEventType", 1},
				{"eventType", {}}
				})
			}}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vEvent, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vEvent, vExpected));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant>> mapBad{
		// sName: vEvent
		{ "mandatory_field", {
			"Invalid dictionary index <fieldA>",
			Dictionary({ {"type", "1"} })
			}}
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vEvent] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vEvent, {}), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("policy_v1.lists")
{
	auto fnScenario = [](Variant vEvent, Variant vExpected)
	{
		auto vCompilerCfg = variant::deserializeFromJson(c_sCompilerConfig);
		vCompilerCfg.put("eventGroups", Dictionary({
			{"default", Dictionary({
				{"fields", Dictionary({
					{"fieldA", "event.fieldA"}
					})}
				})}
			}));
		vCompilerCfg.put("events", Dictionary({
			{"EVENT1", Dictionary({
				{"baseGroup", "default"},
				{"eventType", "1"}
				})}
			}));

		auto vConfig = Dictionary({
			{"config", vCompilerCfg},
			{"policyCatalogPath", "app.config.policy"},
			{"verbose", true}
			});

		auto vSourcePolicy = Dictionary({
			{"Lists", Dictionary({
				{"sourceList1", Sequence({ "*SL11" })},
				{"sourceList2", Sequence({ "*SL21" })}
				})},
			{"Events", Dictionary({
				{"EVENT1", Sequence({ Dictionary({
					{"BaseEventType", 1},
					{"EventType", {}},
					{"Condition", Dictionary({
						{"Field", "fieldA"},
						{"Operator", "MatchInList"},
						{"Value", "sourceList1"}
						})}
					}) })}
				})}
			});

		putCatalogData("objects", Dictionary());
		putCatalogData("app.config.policy.groups.eventsMatching.active", Dictionary());
		putCatalogData("app.config.policy.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(
			createObject(CLSID_PolicyCompiler, vConfig));
		auto vScenario = pCompiler->compile(PolicyGroup::EventsMatching,
			Dictionary({ {"evmCloud", vSourcePolicy} }), true /* fActivate */);
		auto sJson = variant::serializeToJson(vScenario);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);

		vContext.put("event", vEvent);
		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("app.config.policy.groups.eventsMatching.scenario"));
		(void)pScenario->execute(vContext);

		auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vEvent, vExpected
		{ "source", {
			Dictionary({
				{"type", "1"},
				{"fieldA", "xyzSL11"} }),
			Dictionary({
				{"type", "EVENT1.1"},
				{"fieldA", "xyzSL11"},
				{"baseEventType", 1},
				{"eventType", {}} })
			}}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vEvent, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vEvent, vExpected));
		}
	}
}
