//
// edrav2.ut_libedr project
//
// Author: Yury Ermakov (20.08.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"
#include "common.h"

using namespace openEdr;
using namespace openEdr::edr::policy;

const std::string c_sRealSample = R"json({
"version": 2,
"name": "ptmLocal",
"group": "patternsMatching",

"constants": {
	"basket": {
		"fileReadWrite": "FILE_COPY_READ_WRITE",
		"fileWriteRead": "FILE_COPY_WRITE_READ"
	}
},

"variables": {
	"copyFileKey": [ "@event.process.id", "@event.file.rawHash" ]
},

"chains": {
	"createDetailedFileCopyEvent": [

		// Event MLE_FILE_COPY_TO_USB
		{
			"condition": {
				"$operation": "and",
				"args": [
					{
						"$operation": "equal",
						"args": [ "@event.source.volume.type", "FIXED" ] // FIXME: Volume might not exist
					},
					{
						"$operation": "equal",
						"args": [ "@event.destination.volume.type", "REMOVABLE" ]
					}
				]
			},
			"createEvent": {
				"eventType": "MLE_FILE_COPY_TO_USB",
				"destination": "OUT"
			}
		},

		// Event MLE_FILE_COPY_TO_SHARE
		{
			"condition": {
				"$operation": "and",
				"args": [
					{
						"$operation": "equal",
						"args": [ "@event.source.volume.type", "FIXED" ] // Volume might not exist
					},
					{
						"$operation": "equal",
						"args": [ "@event.destination.volume.type", "NETWORK" ]
					}
				]
			},
			"createEvent": {
				"eventType": "MLE_FILE_COPY_TO_SHARE",
				"destination": "OUT"
			}
		},

		// Event MLE_FILE_COPY_FROM_USB
		{
			"condition": {
				"$operation": "and",
				"args": [
					{
						"$operation": "equal",
						"args": [ "@event.source.volume.type", "REMOVABLE" ] // Volume might not exist
					},
					{
						"$operation": "equal",
						"args": [ "@event.destination.volume.type", "FIXED" ]
					}
				]
			},
			"createEvent": {
				"eventType": "MLE_FILE_COPY_FROM_USB",
				"destination": "OUT"
			}
		},

		// Event MLE_FILE_COPY_FROM_SHARE
		{
			"condition": {
				"$operation": "and",
				"args": [
					{
						"$operation": "equal",
						"args": [ "@event.source.volume.type", "NETWORK" ] // Volume might not exist
					},
					{
						"$operation": "equal",
						"args": [ "@event.destination.volume.type", "FIXED" ]
					}
				]
			},
			"createEvent": {
				"eventType": "MLE_FILE_COPY_FROM_SHARE",
				"destination": "OUT"
			}
		}
	]
},

"bindings": [
	{
		"eventType": "MLE_FILE_COPY",
		"rules": [ "createDetailedFileCopyEvent" ]
	}
],

"patterns": {

	//
	// Pattern for copy-file READ-WRITE scenario
	//
	"FILE_COPY_READ_WRITE": [

		// Detect full read and create signature context. No conditions
		{
			"eventType": "LLE_FILE_DATA_READ_FULL",
			"rule": {
				"saveContext": {
					"basket": "@const.basket.fileReadWrite",
					"key": [ "@event.process.id", "@event.file.rawHash" ], // "@var.key.copyFile"
					"group": "@event.process.id",
					"lifetime": 60000,
					"data": { "file": "@event.file" }
				}
			}
		},

		// Detect full write, check signature context and generate new event
		{
			"eventType": "LLE_FILE_DATA_WRITE_FULL",
			"rule": {
				"loadContext": {
					"basket": "@const.basket.fileReadWrite",
					"key": [ "@event.process.id", "@event.file.rawHash" ] // "@var.key.copyFile"
				},
				"createEvent": {
					"eventType": "MLE_FILE_COPY",
					"destination": "IN",
					"clone": false,
					"data": [
						{
							"name": "process",
							"value": "@event.process"
						},
						{
							"name": "destination",
							"value": "@event.file"
						},
						{
							"name": "source",
							"value": "@context.file"
						},
						{
							"name": "time",
							"value": "@event.time"
						},
						{
							"name": "tickTime",
							"value": "@event.tickTime"
						}
					]
				}
			}
		},

		// Detect process termination and free signature related contexts
		{
			"eventType": "LLE_PROCESS_DELETE",
			"rule": {
				"deleteContext": {
					"basket": "@const.basket.fileReadWrite",
					"group": "@event.process.id"
				}
			}
		}
	], // FILE_COPY_READ_WRITE

	//
	// Pattern for copy-file WRITE-READ scenario
	//
	"FILE_COPY_WRITE_READ": [

		// Detect full write and create signature context. No conditions
		{
			"eventType": "LLE_FILE_DATA_WRITE_FULL",
			"rule": {
				"saveContext": {
					"basket": "@const.basket.fileWriteRead",
					"key": [ "@event.process.id", "@event.file.rawHash" ], // "@var.key.copyFile"
					"group": "@event.process.id",
					"lifetime": 60000,
					"maxCopyCount": 100,
					"data": {
						"file": "@event.file",
						"time": "@event.time",
						"tickTime": "@event.tickTime"
					}
				}
			}
		},

		// Detect full write, check signature context and generate new event
		{
			"eventType": "LLE_FILE_DATA_READ_FULL",
			"rule": {
				"loadContext": {
					"basket": "@const.basket.fileWriteRead",
					"key": [ "@event.process.id", "@event.file.rawHash" ], // "@var.key.copyFile"
					"delete": true
				},
				"createEvent": {
					"eventType": "MLE_FILE_COPY",
					"destination": "IN",
					"clone": false,
					"data": [
						{
							"name": "process",
							"value": "@event.process"
						},
						{
							"name": "destination",
							"value": "@context.file"
						},
						{
							"name": "source",
							"value": "@event.file"
						},
						{
							"name": "time",
							"value": "@context.time"
						},
						{
							"name": "tickTime",
							"value": "@context.tickTime"
						}
					]
				}
			}
		},

		// Detect process termination and free signature related contexts
		{
			"eventType": "LLE_PROCESS_DELETE",
			"rule": {
				"deleteContext": {
					"basket": "@const.basket.fileWriteRead",
					"group": "@event.process.id"
				}
			}
		}
	] // FILE_COPY_WRITE_READ
}
})json";

const std::string c_sCompilerConfig = R"json({
"objects": {},
"eventClasses": {}
})json";

//
//
//
TEST_CASE("policy_v2.compile")
{
	auto fnScenario = [](std::string_view sPolicy)
	{
		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		putCatalogData("objects", Dictionary());
		putCatalogData("test.groups.patternsMatching.active", Dictionary());
		putCatalogData("test.groups.patternsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		(void)pCompiler->compile(PolicyGroup::PatternsMatching,
			Dictionary({ {"ptmTest", variant::deserializeFromJson(sPolicy)} }),
			true /* fActivate */);

		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.patternsMatching.scenario"));
		auto vResult = pScenario->execute(Dictionary({
			{"event", Dictionary({
				{"type", 0}
				})}
			}));
	};

	const std::map<std::string, std::tuple<std::string>> mapData{
		// sName: sPolicy
		{ "valid", { R"json({"version": 2, "name": "testPolicy"})json" }},
		{ "real_sample", { c_sRealSample }}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sPolicy] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sPolicy));
		}
	}

	const std::map<std::string, std::tuple<std::string, std::string>> mapBad{
		// sName: sMsg, sPolicy
		{ "version_invalid_1", { "Unknown policy version <999>", R"json({"version":999})json" }},
		{ "version_invalid_2", { "Can't get Integer value", R"json({"version":"999"})json" }},
		{ "name_absent", { "Missing or empty field <name>",
			R"json({"version": 2})json" }},
		{ "name_empty", { "Missing or empty field <name>",
			R"json({"version": 2, "name": ""})json" }},
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, sPolicy] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(sPolicy), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("policy_v2.constants")
{
	auto fnScenario = [](std::string_view sConstants, Variant vExpected)
	{
		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});

		const std::string_view sPolicy = R"json({
"version": 2,
"name": "testPolicy",
//"dependencies": [ "basePolicy ],
"group": "patternsMatching"
})json";

		auto vPolicy = variant::deserializeFromJson(sPolicy);
		vPolicy.put("constants", variant::deserializeFromJson(sConstants));

		putCatalogData("objects", Dictionary());
		putCatalogData("test.groups.patternsMatching.active", Dictionary());
		putCatalogData("test.groups.patternsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		(void)pCompiler->compile(PolicyGroup::PatternsMatching, Dictionary({ {"ptmTest", vPolicy} }),
			false /* fActivate */);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: sConstants, vExpected
		{ "matched", {
			R"json({
"scalars": [ "@const.xyz", "@const.true", "@const.42" ],
"xyz": "xyz",
"true": true,
"42": 42,
"link2xyz": "@const.xyz",
"sequence_1": [ "item_1", "@const.link2xyz" ],
"sequence_2": [ "@const.sequence_1", "item_3", "@const.sequence_3" ],
"sequence_3": [ "item_5", "item_6" ]
			})json",
			{}
			}},
//		{ "link_scalar_l2", {
//			R"json({
//"link2xyz": "@const.xyz",
//"xyz": "xyz",
//"link2link2xyz": "@const.link2xyz"
//			})json",
//			{}
//			}},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConstants, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConstants, vExpected));
		}
	}

	//const std::map<std::string, std::tuple<std::string, Variant>> mapBad{
	//	// sName: vEvent
	//	{ "mandatory_field", {
	//		"Invalid dictionary index <fieldA>",
	//		Dictionary({ {"event", Dictionary({ {"type", 1} })} })
	//		}}
	//};

	//for (const auto&[sName, params] : mapBad)
	//{
	//	const auto&[sMsg, vEvent] = params;
	//	DYNAMIC_SECTION(sName)
	//	{
	//		REQUIRE_THROWS_WITH(fnScenario(vEvent, {}), Catch::Matchers::Contains(sMsg));
	//	}
	//}
}

constexpr char c_sFullReadEvent[] = R"json({
"rawEventId": 6512224311221157899,
"time": 1567377181696,
"tickTime": 11316671,
"process": {
    "pid": 2352,
    "parent": {},
    "imageFile": {},
    "creationTime": 1567365878388,
    "isElevated": true,
    "elevationType": 1,
    "user": "user",
    "cmdLine": "C:\\Tools\\Total Commander\\TOTALCMD64.EXE",
    "id": "0f8aee9490ea30f7715602628a145970581963a1",
    "abstractCmdLine": "C:\\Tools\\Total Commander\\TOTALCMD64.EXE",
    "security": {
        "pid": 2352,
        "sid": "S-1-5-21-1595028980-4146408302-209789686-1000"
    }
},
"file": {
    "rawPath": "\\Device\\Mup\\VBoxSvr\\bin\\win-Release-x64\\edrcon.exe",
    "volume": {
        "name": "",
        "type": "NETWORK",
        "device": "\\Device\\Mup"
    },
    "rawHash": "53678C9B33D8A89E"
},
"baseType": 7,
"type": "LLE_FILE_DATA_READ_FULL"
})json";

constexpr char c_sFullWriteEvent[] = R"json({
"rawEventId": 6512224311221157900,
"time": 1567377181696,
"tickTime": 11316671,
"process": {
    "pid": 2352,
    "parent": {},
    "imageFile": {},
    "creationTime": 1567365878388,
    "isElevated": true,
    "elevationType": 1,
    "user": "user",
    "cmdLine": "C:\\Tools\\Total Commander\\TOTALCMD64.EXE",
    "id": "0f8aee9490ea30f7715602628a145970581963a1",
    "abstractCmdLine": "C:\\Tools\\Total Commander\\TOTALCMD64.EXE",
    "security": {
        "pid": 2352,
        "sid": "S-1-5-21-1595028980-4146408302-209789686-1000"
    }
},
"file": {
    "rawPath": "\\Device\\HarddiskVolume2\\Users\\Tester\\Desktop\\ats-build\\edrcon.exe",
    "volume": {
        "name": "\\??\\Volume{57c6ce44-cc8a-11e8-b504-806e6f6e6963}",
        "type": "FIXED",
        "device": "\\Device\\HarddiskVolume2"
    },
    "rawHash": "53678C9B33D8A89E"
},
"baseType": 8,
"type": "LLE_FILE_DATA_WRITE_FULL"
})json";

//
// Minimal data receiver
//
class TestDataReceiver : public ObjectBase<>,
	public IDataReceiver
{
	Size m_nCount = 0;
public:

	Size getCount()
	{
		return m_nCount;
	}

	virtual void put(const Variant& vData) override
	{
		++m_nCount;
	}
};

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
TEST_CASE("policy_v2.createEvent")
{
	auto fnScenario = [](std::string_view sPolicy, std::vector<std::string> events)
	{
		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		addGlobalObject("contextService", createObject(CLSID_ContextService));
		putCatalogData("test.groups.patternsMatching.active", Dictionary());
		putCatalogData("test.groups.patternsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		auto vScenario = pCompiler->compile(PolicyGroup::PatternsMatching,
			Dictionary({ {"ptmTest", variant::deserializeFromJson(sPolicy)} }),
			true /* fActivate */);
		auto sJson = variant::serializeToJson(vScenario);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher, true /* fCreatePaths */);
		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.patternsMatching.scenario"));
		
		for (const auto& sEvent : events)
		{
			vContext.put("event", variant::deserializeFromJson(sEvent));
			(void)pScenario->execute(vContext);
		}

		auto pEventProvider = queryInterface<IDataProvider>(pEventCatcher);
		std::optional<Variant> optEvent;
		while ((optEvent = pEventProvider->get()) != std::nullopt)
		{
			vContext.put("event", optEvent.value());
			(void)pScenario->execute(vContext);
		}
	};

	const std::map<std::string, std::tuple<std::string, std::vector<std::string>>> mapData{
		// sName: sPolicy, events
		{ "copy_file", { c_sRealSample, { c_sFullReadEvent, c_sFullWriteEvent }}}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sPolicy, events] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sPolicy, events));
		}
	}
}

//
//
//
TEST_CASE("policy_v2.operations")
{
	auto fnScenario = [](std::string_view sCondition, bool fMatchExpected)
	{
		const std::string c_sPolicyTemplate = R"json({
"version": 2,
"name": "evmTest",
"group": "eventsMatching",
"constants": {
	"matchList": [24, 42, "xyz"]
	},
"chains": {
	"testOperation": [{
		"createEvent": {
			"eventType": "MATCHED",
			"destination": "OUT",
			"clone": false,
			"data": {}
			}
		}]
	},
"bindings": [ { "eventType": "0", "rules": [ "testOperation" ] } ]
})json";

		auto vPolicy = variant::deserializeFromJson(c_sPolicyTemplate);
		variant::putByPath(vPolicy, "chains.testOperation[0].condition",
			variant::deserializeFromJson(sCondition), true /* fCreatePaths */);

		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		putCatalogData("objects", Dictionary());
		putCatalogData("test.groups.eventsMatching.active", Dictionary());
		putCatalogData("test.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		(void)pCompiler->compile(PolicyGroup::EventsMatching, Dictionary({ {"evmTest", vPolicy} }),
			true /* fActivate */);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);

		vContext.put("event", Dictionary({
			{"type", "0"},
			{"true_value", true},
			{"false_value", false}
			}));

		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.eventsMatching.scenario"));
		(void)pScenario->execute(vContext);
		
		if (fMatchExpected)
		{
			auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
			REQUIRE(vResult == Dictionary({ {"type", "MATCHED"} }));
		}
		else
			REQUIRE(!queryInterface<IDataProvider>(pEventCatcher)->get().has_value());
	};

	const std::map<std::string, std::tuple<std::string, bool>> mapData{
		// sName: sCondition, fMatchExpected
		{ "not_matched", { R"json({"$operation": "not", "args": false})json", true }},
		{ "not_unmatched", { R"json({"$operation": "not", "args": true})json", false }},
		{ "and_matched", { R"json({"$operation": "and", "args": [true, true] })json", true }},
		{ "and_matched_many", { R"json({"$operation": "and", "args": [true, true, 42, true] })json", true }},
		{ "and_unmatched", { R"json({"$operation": "and", "args": [true, false] })json", false }},
		{ "and_unmatched_many", { R"json({"$operation": "and", "args": [true, true, false, true] })json", false }},
		{ "or_matched", { R"json({"$operation": "or", "args": [false, true] })json", true }},
		{ "or_unmatched", { R"json({"$operation": "or", "args": [false, false] })json", false }},
		{ "equal_matched", { R"json({"$operation": "equal", "args": [42, "42"] })json", true }},
		{ "equal_unmatched", { R"json({"$operation": "equal", "args": [false, true] })json", false }},
		{ "notequal_matched", { R"json({"$operation": "!equal", "args": [42, 24] })json", true}},
		{ "notequal_unmatched", { R"json({"$operation": "!equal", "args": [false, false] })json", false }},
		{ "greater_matched", { R"json({"$operation": "greater", "args": [42, 24] })json", true}},
		{ "greater_unmatched", { R"json({"$operation": "greater", "args": [24, 42] })json", false }},
		{ "less_matched", { R"json({"$operation": "less", "args": [24, 42] })json", true}},
		{ "less_unmatched", { R"json({"$operation": "less", "args": [42, 24] })json", false }},
		{ "match_matched", { R"json({"$operation": "match", "pattern": "xyz*", "args": [ "xyz42" ] })json", true}},
		{ "match_unmatched",
			{ R"json({"$operation": "match", "pattern": "xyz", "args": [ "xyz42" ] })json", false }},
		{ "notmatch_matched",
			{ R"json({"$operation": "!match", "pattern": "xyz", "args": [ "xyz42" ] })json", true}},
		{ "notmatch_unmatched",
			{ R"json({"$operation": "!match", "pattern": "xyz*", "args": [ "xyz42" ] })json", false }},
		{ "imatch_matched",
			{ R"json({"$operation": "imatch", "pattern": "xyz*", "args": [ "XYZ42" ] })json", true}},
		{ "imatch_unmatched",
			{ R"json({"$operation": "imatch", "pattern": "xyz", "args": [ "XYZ42" ] })json", false }},
		{ "notimatch_matched",
			{ R"json({"$operation": "!imatch", "pattern": "xyz", "args": [ "XYZ42" ] })json", true}},
		{ "notimatch_unmatched",
			{ R"json({"$operation": "!imatch", "pattern": "xyz*", "args": [ "XYZ42" ] })json", false }},
		{ "contain_matched",
			{ R"json({"$operation": "contain", "item": 42, "args": [ ["xyz", 42, 24] ] })json", true}},
		{ "contain_matched_link",
			{ R"json({"$operation": "contain", "item": 42, "args": [ "@const.matchList" ] })json", true}},
		{ "contain_unmatched",
			{ R"json({"$operation": "contain", "item": 42, "args": [ ["xyz", 24, 0] ] })json", false }},
		{ "contain_unmatched_link",
			{ R"json({"$operation": "contain", "item": 33, "args": [ "@const.matchList" ] })json", false}},
		{ "notcontain_matched",
			{ R"json({"$operation": "!contain", "item": 33, "args": [ ["xyz", 42, 24] ] })json", true}},
		{ "notcontain_unmatched",
			{ R"json({"$operation": "!contain", "item": 42, "args": [ "@const.matchList" ] })json", false}},
		{ "add_matched", { R"json({"$operation": "add", "args": [ 1, 0 ] })json", true}},
		{ "add_unmatched", { R"json({"$operation": "add", "args": [ 0, 0 ] })json", false}},
		{ "extract_matched",
			{ R"json({"$operation": "extract", "path": "true_value", "args": [ "@event" ] })json", true}},
		{ "extract_unmatched",
			{ R"json({"$operation": "extract", "path": "false_value", "args": [ "@event" ] })json", false}},
		{ "has_matched", { R"json({"$operation": "has", "path": "true_value", "args": [ "@event" ] })json", true}},
		{ "has_unmatched", { R"json({"$operation": "has", "path": "unknown", "args": [ "@event" ] })json", false}},
		{ "nothas_matched",
			{ R"json({"$operation": "!has", "path": "unknown", "args": [ "@event" ] })json", true}},
		{ "nothas_unmatched",
			{ R"json({"$operation": "!has", "path": "false_value", "args": [ "@event" ] })json", false}},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sCondition, fMatchExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sCondition, fMatchExpected));
		}
	}
}

//
//
//
TEST_CASE("policy_v2.operation_MakeDescriptor")
{
	auto fnScenario = [](std::string_view sOpParams)
	{
		const std::string c_sPolicyTemplate = R"json({
"version": 2,
"name": "evmTest",
"group": "eventsMatching",
"constants": {
	"testPath": "C:\\Windows\\explorer.exe"
	},
"chains": {
	"testOperation": [{
		"createEvent": {
			"eventType": "MATCHED",
			"destination": "OUT",
			"clone": false,
			"data": {
				"file": {
					"$operation": "makeDescriptor"
					}
				}
			}
		}]
	},
"bindings": [ { "eventType": "0", "rules": [ "testOperation" ] } ]
})json";

		auto vPolicy = variant::deserializeFromJson(c_sPolicyTemplate);
		auto vFile = variant::getByPath(vPolicy, "chains.testOperation[0].createEvent.data.file");
		vFile.merge(variant::deserializeFromJson(sOpParams), variant::MergeMode::All |
			variant::MergeMode::CheckType | variant::MergeMode::MergeNestedDict |
			variant::MergeMode::MergeNestedSeq);

		// remove old services
		removeGlobalObject("userDataProvider");
		removeGlobalObject("signatureDataProvider");
		removeGlobalObject("fileDataProvider");

		addGlobalObject("userDataProvider", createObject(CLSID_UserDataProvider));
		addGlobalObject("signatureDataProvider", createObject(CLSID_SignDataProvider));
		addGlobalObject("fileDataProvider", createObject(CLSID_FileDataProvider));

		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		putCatalogData("test.groups.eventsMatching.active", Dictionary());
		putCatalogData("test.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		auto vScenario = pCompiler->compile(PolicyGroup::EventsMatching, Dictionary({ {"evmTest", vPolicy} }),
			true /* fActivate */);
		auto sJson = variant::serializeToJson(vScenario);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);

		vContext.put("event", Dictionary({
			{"type", "0"},
			{"path", "C:\\Windows\\explorer.exe"}
			}));

		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.eventsMatching.scenario"));
		(void)pScenario->execute(vContext);
		
		auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
		CHECK(variant::getByPathSafe(vResult, "file.id").has_value());
	};

	const std::map<std::string, std::tuple<std::string>> mapData{
		// sName: sOpParams
		{ "file_explicit_path",
			{ R"json({ "type": "FILE", "path": "C:\\Windows\\explorer.exe" })json"}},
		{ "file_const_path",
			{ R"json({ "type": "FILE", "path": "@const.path" })json"}},
		{ "file_event_path",
			{ R"json({ "type": "FILE", "path": "@event.path" })json"}}
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sOpParams] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sOpParams));
		}
	}

	const std::map<std::string, std::tuple<std::string, std::string>> mapBad{
		// sName: sMsg, sOpParams
		{ "invalid_type", { "Unknown descriptor type <xyz>", R"json({"type": "xyz"})json" }},
		{ "missing_type", { "Invalid dictionary index <type>", R"json({"xyz": 42})json" }},
		{ "file_path_absent", { "Invalid dictionary index <path>", R"json({"type":"FILE"})json" }},
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, sOpParams] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(sOpParams), Catch::Matchers::Contains(sMsg));
		}
	}

}

//
//
//
TEST_CASE("policy_v2.priority")
{
	auto fnScenario = [](std::string_view sEventType, std::string_view sExpected)
	{
		const std::string c_sPolicyTemplate = R"json({
"version": 2,
"name": "evmTest",
"group": "eventsMatching",
"constants": {
	"matchList": [24, 42, "xyz"]
	},
"chains": {
	"chainDefault1000": [ {"createEvent": {
		"eventType": "chainDefault1000-00",
		"destination": "OUT",
		"clone": false,
		"data": {}
		}} ],
	"chain500": [
		{ "priority": 500, "createEvent": {
			"eventType": "chain500-00",
			"destination": "OUT",
			"clone": false,
			"data": {}
			}},
		{ "priority": 600, "createEvent": {
			"eventType": "chain500-01",
			"destination": "OUT",
			"clone": false,
			"data": {}
			}},
		{ "priority": 400, "createEvent": {
			"eventType": "chain500-02",
			"destination": "OUT",
			"clone": false,
			"data": {}
			}}
		],
	"chain1500": [
		{ "priority": 1500, "createEvent": {
			"eventType": "chain1500-00",
			"destination": "OUT",
			"clone": false,
			"data": {}
			}}
		]
	},
"bindings": [
	{ "eventType": "0", "rules": [ "chain500", "chainDefault1000" ] }, 
	{ "eventType": "1", "rules": [ "chainDefault1000", "chain500", "chain1500" ] },
	{ "eventType": "2", "rules": [ "chain500" ] } 
	]
})json";

		auto vPolicy = variant::deserializeFromJson(c_sPolicyTemplate);

		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		putCatalogData("objects", Dictionary());
		putCatalogData("test.groups.eventsMatching.active", Dictionary());
		putCatalogData("test.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		(void)pCompiler->compile(PolicyGroup::EventsMatching, Dictionary({ {"evmTest", vPolicy} }),
			true /* fActivate */);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);

		vContext.put("event", Dictionary({ {"type", sEventType} }));

		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.eventsMatching.scenario"));
		(void)pScenario->execute(vContext);

		auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
		REQUIRE(vResult == Dictionary({ {"type", sExpected} }));
	};

	const std::map<std::string, std::tuple<std::string, std::string>> mapData{
		// sName: sEventType, sExpected
		{ "default", { "0", "chainDefault1000-00" }},
		{ "higher", { "1", "chain1500-00" }},
		{ "one_chain", { "2", "chain500-01" }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sEventType, sExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sEventType, sExpected));
		}
	}
}

//
//
//
TEST_CASE("policy_v2.bindings")
{
	auto fnScenario = [](std::string_view sEventType, std::string_view sExpected)
	{
		const std::string c_sPolicyTemplate = R"json({
"version": 2,
"name": "evmTest",
"group": "eventsMatching",
"constants": {
	"eventType1": "eventType1",
	"eventType4": "eventType4",
	"eventTypeGroup": [ "eventType2", "eventType3" ]
},
"chains": {
	"chainA": [{ "createEvent": {
		"eventType": "chainA",
		"destination": "OUT",
		"clone": false
		}}],
	"chainB": [{ "createEvent": {
		"eventType": "chainB",
		"destination": "OUT",
		"clone": false
		}}],
	"chainC": [{ "createEvent": {
		"eventType": "chainC",
		"destination": "OUT",
		"clone": false
		}}],
	"chainD": [{ "createEvent": {
		"eventType": "chainD",
		"destination": "OUT",
		"clone": false
		}}]
},
"bindings": [
	{
		"eventType": "eventType0",
		"rules": [ "chainA" ]
	},
	{
		"eventType": "@const.eventType1",
		"rules": [ "chainB" ]
	},
	{
		"eventType": "@const.eventTypeGroup",
		"rules": [ "chainC" ]
	},
	{
		"eventType": [ "@const.eventType4", "eventType5" ],
		"rules": [ "chainD" ]
	}
]
})json";

		auto vPolicy = variant::deserializeFromJson(c_sPolicyTemplate);

		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		putCatalogData("objects", Dictionary());
		putCatalogData("test.groups.eventsMatching.active", Dictionary());
		putCatalogData("test.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		(void)pCompiler->compile(PolicyGroup::EventsMatching, Dictionary({ {"evmTest", vPolicy} }),
			true /* fActivate */);

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);

		vContext.put("event", Dictionary({ {"type", sEventType} }));

		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.eventsMatching.scenario"));
		(void)pScenario->execute(vContext);

		auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
		REQUIRE(vResult == Dictionary({ {"type", sExpected} }));
	};

	const std::map<std::string, std::tuple<std::string, std::string>> mapData{
		// sName: sEventType, sExpected
		{ "explicit_single", { "eventType0", "chainA" }},
		{ "const_single", { "eventType1", "chainB" }},
		{ "const_group", { "eventType2", "chainC" }},
		{ "explicit_group_1", { "eventType4", "chainD" }},
		{ "explicit_group_2", { "eventType5", "chainD" }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [nEventType, sExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(nEventType, sExpected));
		}
	}
}

//
//
//
TEST_CASE("policy_v2.variables")
{
	auto fnScenario = [](Variant vEvent, Variant vExpected)
	{
		const std::string c_sPolicyTemplate = R"json({
"version": 2,
"name": "evmTest",
"group": "eventsMatching",
"constants": {
	"eventType1": "eventType1",
	"eventType4": "eventType4",
	"eventTypeGroup": [ "eventType2", "eventType3" ]
},
"variables": {
	"varNumber": 0,
	"varString": "<string>",
	"varLink": "@event.dataA",
	"varSequence": [ "@event.dataB1", "@event.dataB2", "B3" ],
	"varOperationLink": {
		"$operation": "equal",
		"args": [
			"@event.dataC1",
			"C1"
		]
	},
	"varOperationVar": {
		"$operation": "not",
		"args": [{
			"$operation": "equal",
			"args": [
				"@var.varNumber",
				1
			]
		}]
	}
},
"chains": {
	"chainNumber": [{ "createEvent": {
		"eventType": "chainNumber",
		"destination": "OUT",
		"clone": false,
		"data": {
			"result": "@var.varNumber"
		}
	} }],
	"chainSequence": [{ "createEvent": {
		"eventType": "chainSequence",
		"destination": "OUT",
		"clone": false,
		"data": {
			"result": "@var.varSequence"
		}
	} }],
	"chainOperationLink": [{ "createEvent": {
		"eventType": "chainOperationLink",
		"destination": "OUT",
		"clone": false,
		"data": {
			"result": "@var.varOperationLink"
		}
	} }],
	"chainOperationVar": [{ "createEvent": {
		"eventType": "chainOperationVar",
		"destination": "OUT",
		"clone": false,
		"data": {
			"result": "@var.varOperationVar"
		}
	} }]
},
"bindings": [
	{
		"eventType": "0",
		"rules": [ "chainNumber" ]
	},
	{
		"eventType": "1",
		"rules": [ "chainSequence" ]
	},
	{
		"eventType": "2",
		"rules": [ "chainOperationLink" ]
	},
	{
		"eventType": "3",
		"rules": [ "chainOperationVar" ]
	}
]
})json";

		auto vPolicy = variant::deserializeFromJson(c_sPolicyTemplate);

		auto vConfig = Dictionary({
			{"policyCatalogPath", "test"},
			{"config", variant::deserializeFromJson(c_sCompilerConfig)}
			});
		putCatalogData("objects", Dictionary());
		putCatalogData("test.groups.eventsMatching.active", Dictionary());
		putCatalogData("test.groups.eventsMatching.scenario", Dictionary());
		auto pCompiler = queryInterface<IPolicyCompiler>(createObject(CLSID_PolicyCompiler, vConfig));
		auto vScenario = pCompiler->compile(PolicyGroup::EventsMatching,
			Dictionary({ {"evmTest", vPolicy} }), true /* fActivate */);
		auto sJson = variant::serializeToJson(vScenario);

		auto pScenario = queryInterface<IContextCommand>(
			getCatalogData("test.groups.eventsMatching.scenario"));

		Variant vContext = Dictionary();
		auto pEventCatcher = CreatingEventCatcher::create();
		variant::putByPath(vContext, "eventHandler.call.createEvent", pEventCatcher,
			true /* fCreatePaths */);
		vContext.put("event", vEvent);

		(void)pScenario->execute(vContext);

		auto vResult = queryInterface<IDataProvider>(pEventCatcher)->get().value();
		REQUIRE(vResult.get("result") == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vEvent, vExpected
		{ "number", {
			Dictionary({ {"type", "0"} }),
			0
			}},
		{ "sequence", {
			Dictionary({ {"type", "1"}, {"dataB1", "B1"}, {"dataB2", "B2"} }),
			Sequence({ "B1", "B2", "B3" })
			}},
		{ "operation_link_true", {
			Dictionary({ {"type", "2"}, {"dataC1", "C1"} }),
			true
			}},
		{ "operation_link_false", {
			Dictionary({ {"type", "2"}, {"dataC1", "xyz"} }),
			false
			}},
		{ "operation_var", {
			Dictionary({ {"type", "3"} }),
			true
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

//
//
//
TEST_CASE("policy.CompileError")
{
	auto fnScenario = [](const PolicySourceLocation& location, std::string_view sMsg)
	{
		error::CompileError(SL, location, sMsg).throwException();
	};

	const std::map<std::string, std::tuple<PolicySourceLocation, std::string, std::string>> mapData{
		// sName: location, sMsg, sExpected
		{ "msg", { {"", "", "", 0, 0, ""}, "ut", "ut"}},
		{ "msg_default", { {"", "", "", 0, 0, ""}, "", "Compile error"}},
		{ "policy", { {"xyzPolicy", "", "", 0, 0, ""}, "ut", "ut <xyzPolicy>"}},
		{ "pattern", { {"xyzPolicy", "xyz", "", 0, 0, ""}, "ut", "ut <xyzPolicy:patterns:xyz>"}},
		{ "pattern_with_chain", { {"xyzPolicy", "ppp", "ccc", 0, 0, ""}, "ut", "ut <xyzPolicy:patterns:ppp>"}},
		{ "pattern_with_rule", { {"xyzPolicy", "ppp", "", 42, 0, ""}, "ut", "ut <xyzPolicy:patterns:ppp:42>"}},
		{ "chain", { {"xyzPolicy", "", "ccc", 0, 0, ""}, "ut", "ut <xyzPolicy:chains:ccc>"}},
		{ "chain_with_rule", { {"xyzPolicy", "", "ccc", 42, 0, ""}, "ut", "ut <xyzPolicy:chains:ccc:42>"}},
		{ "binding", { {"xyzPolicy", "", "", 0, 42, ""}, "ut", "ut <xyzPolicy:bindings:42>"}},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [location, sMsg, sExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(location, sMsg), Catch::Matchers::Contains(sExpected));
		}
	}
}
