//
// edrav2.ut_libcloud project
//
// Author: Yury Ermakov (31.05.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"

using namespace openEdr;

const std::string_view c_sCustomerId = "CustomerIdPlaceholder"; 
const std::string_view c_sEndpointId = "EndpointIdPlaceholder"; // UT_LIBCLOUD

const std::string_view c_sPubSubTopic = "edr-prod-raw-evt-ps";
const std::string_view c_sSaCredentials = "SaCredentialsPlaceholder"; 


const std::string_view c_sSaCredentialsBad = "SaCredentialsBadPlaceholder";

//
//
//
TEST_CASE("GcpPubSubClient.submit")
{
	auto pathToData = initTest();
	auto pathCaCertificates = pathToData / "cacert.pem";
	std::ifstream f(pathCaCertificates);
	std::ostringstream oss;
	oss << f.rdbuf();
	auto sCaCertificates = oss.str();

	auto fnScenario = [&](Variant vConfig, Variant vParams)
	{
		putCatalogData("app.stage", "active");
		auto pObj = createObject(CLSID_GcpPubSubClient, vConfig);
		auto pProcessor = queryInterface<ICommandProcessor>(pObj);
		auto fResult = execCommand(pProcessor, "submit", vParams);
		REQUIRE(fResult);
	};

	const auto c_dictConfig = Dictionary({
		{"pubSubTopic", c_sPubSubTopic},
		{"saCredentials", c_sSaCredentials},
		{"caCertificates", sCaCertificates}
		});

	const auto dictEvent1 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\notepad.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b2d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d69f"},
			{"imagePath", "C:\\Windows\\System32\\notepad.exe"},
			{"pid", 1111}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", {}},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 2222},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 4611686018427387905},
		{"version", "1.1"}
		});

	const auto dictEvent2 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\calc.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b1d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d68f"},
			{"imagePath", "C:\\Windows\\System32\\calc.exe"},
			{"pid", 333}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", {}},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 2222},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 4611686018427387905},
		{"version", "1.1"}
		});

	const auto dictEvent3 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\calc.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b1d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d68f"},
			{"imagePath", "C:\\Windows\\System32\\calc.exe"},
			{"pid", 1320}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", {}},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 1080},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 4611686018427387905},
		{"version", "1.1"}
		});

	const std::map<const std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vConfig, vParams
		{ "dict", { c_dictConfig, Dictionary({ { "data", dictEvent3 } }) } },
		{ "seq", { c_dictConfig, Dictionary({ { "data", Sequence({ dictEvent1, dictEvent2 }) } }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vParams] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vParams));
		}
	}
}

//
//
//
TEST_CASE("GcpPubSubClient.updateSettings")
{
	auto pathToData = initTest();
	auto pathCaCertificates = pathToData / "cacert.pem";
	std::ifstream f(pathCaCertificates);
	std::ostringstream oss;
	oss << f.rdbuf();
	auto sCaCertificates = oss.str();

	Variant vConfig = Dictionary({
		{"pubSubTopic", c_sPubSubTopic},
		{"saCredentials", c_sSaCredentials},
		{"caCertificates", sCaCertificates}
		});

	auto fnScenario = [&](Variant vSettings, Variant vData)
	{
		putCatalogData("app.stage", "active");
		auto pObj = createObject(CLSID_GcpPubSubClient, vConfig);
		auto pProcessor = queryInterface<ICommandProcessor>(pObj);
		auto fResult = execCommand(pProcessor, "submit", vData);
		CHECK(fResult);
		execCommand(pProcessor, "updateSettings", vSettings);
		fResult = execCommand(pProcessor, "submit", vData);
		CHECK(!fResult);
	};

	const auto dictEvent1 = Dictionary({
	{"baseEventType", 1},
	{"childProcess", Dictionary({
		{"cmdLine", "C:\\Windows\\system32\\notepad.exe"},
		{"creationTime", getCurrentTime() - 100},
		{"elevationType", 3},
		{"flsVerdict", 1},
		{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b2d"},
		{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d69f"},
		{"imagePath", "C:\\Windows\\System32\\notepad.exe"},
		{"pid", 1111}
		})},
	{"customerId", c_sCustomerId},
	{"deviceName", "UT_LIBCLOUD"},
	{"endpointId", c_sEndpointId},
	{"eventType", {}},
	{"processes", Sequence({
		Dictionary({
			{"creationTime", getCurrentTime() - 1000},
			{"flsVerdict", 1},
			{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
			{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
			{"imagePath", "C:\\Windows\\explorer.exe"},
			{"pid", 2222},
			{"userName", "test@UT_LIBCLOUD"}
			})
		})},
	{"sessionUser", "test@UT_LIBCLOUD"},
	{"time", getCurrentTime()},
	{"type", 4611686018427387905},
	{"version", "1.1"}
		});

	const auto dictEvent2 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\calc.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b1d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d68f"},
			{"imagePath", "C:\\Windows\\System32\\calc.exe"},
			{"pid", 333}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", {}},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 2222},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 4611686018427387905},
		{"version", "1.1"}
		});

	const std::map<std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vSettings, vData
		{ "pubSubTopic", {
			Dictionary({ {"pubSubTopic", "<invalid>"} }),
			Dictionary({ {"data", dictEvent1} }) } },
		{ "saCredentials", {
			Dictionary({ {"saCredentials", c_sSaCredentialsBad} }),
			Dictionary({ {"data", dictEvent2} }) } }
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vSettings, vData] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vSettings, vData));
		}
	}
}//
//
//
TEST_CASE("GcpPubSubClient.updateSettings_CloudConfigurationIsChanged")
{
	auto pathToData = initTest();
	auto pathCaCertificates = pathToData / "cacert.pem";
	std::ifstream f(pathCaCertificates);
	std::ostringstream oss;
	oss << f.rdbuf();
	auto sCaCertificates = oss.str();

	Variant vConfig = Dictionary({
		{"pubSubTopic", c_sPubSubTopic},
		{"saCredentials", c_sSaCredentials},
		{"caCertificates", sCaCertificates}
		});

	const auto vEvent = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\calc.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b1d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d68f"},
			{"imagePath", "C:\\Windows\\System32\\calc.exe"},
			{"pid", 333}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", {}},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 2222},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 4611686018427387905},
		{"version", "1.1"}
		});

	auto fnScenario = [&]()
	{
		auto vData = Dictionary({ {"data", vEvent} });
		putCatalogData("app.stage", "active");
		putCatalogData("app.config.cloud.gcp", vConfig);
		auto pObj = createObject(CLSID_GcpPubSubClient, vConfig);
		auto pProcessor = queryInterface<ICommandProcessor>(pObj);
		auto fResult = execCommand(pProcessor, "submit", vData);
		CHECK(fResult);

		putCatalogData("app.config.cloud.gcp.saCredentials", c_sSaCredentialsBad);
		sendMessage(Message::CloudConfigurationIsChanged);
		fResult = execCommand(pProcessor, "submit", vData);
		CHECK(!fResult);
	};

	REQUIRE_NOTHROW(fnScenario());
}

//
//
//
TEST_CASE("GcpPubSubClient.statistic")
{
	auto pathToData = initTest();
	auto pathCaCertificates = pathToData / "cacert.pem";
	std::ifstream f(pathCaCertificates);
	std::ostringstream oss;
	oss << f.rdbuf();
	auto sCaCertificates = oss.str();

	auto fnScenario = [](Variant vConfig, Variant vParams)
	{
		unsubscribeAll();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		putCatalogData("app.stage", "active");
		putCatalogData("app.startTime",
			std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000);
		auto pObj = createObject(CLSID_GcpPubSubClient, vConfig);
		auto pProcessor = queryInterface<ICommandProcessor>(pObj);
		auto fResult = execCommand(pProcessor, "submit", vParams);
		auto vStatistic = execCommand(pProcessor, "getStatistic");
		
		// Wait for statistic to be flushed
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		REQUIRE(fResult);
	};

	const auto c_dictConfig = Dictionary({
		{"pubSubTopic", c_sPubSubTopic},
		{"saCredentials", c_sSaCredentials},
		{"caCertificates", sCaCertificates},
		{"enableStatistic", true},
		{"statFilePath", "gcpstat.dat"},
		{"statFlushTimeout", 10}
		});

	const auto dictEvent1 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\notepad.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b2d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d69f"},
			{"imagePath", "C:\\Windows\\System32\\notepad.exe"},
			{"pid", 1111}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", "{GUID}"},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 2222},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 1},
		{"version", "1.1"}
		});

	const auto dictEvent2 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\calc.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b1d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d68f"},
			{"imagePath", "C:\\Windows\\System32\\calc.exe"},
			{"pid", 333}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", "{GUID}"},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 2222},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 2},
		{"version", "1.1"}
		});

	const auto dictEvent3 = Dictionary({
		{"baseEventType", 1},
		{"childProcess", Dictionary({
			{"cmdLine", "C:\\Windows\\system32\\calc.exe"},
			{"creationTime", getCurrentTime() - 100},
			{"elevationType", 3},
			{"flsVerdict", 1},
			{"id", "e51e73c1f903a927a7de9d7de81a9bdbae335b1d"},
			{"imageHash", "42d36eeb2140441b48287b7cd30b38105986d68f"},
			{"imagePath", "C:\\Windows\\System32\\calc.exe"},
			{"pid", 1320}
			})},
		{"customerId", c_sCustomerId},
		{"deviceName", "UT_LIBCLOUD"},
		{"endpointId", c_sEndpointId},
		{"eventType", {}},
		{"processes", Sequence({
			Dictionary({
				{"creationTime", getCurrentTime() - 1000},
				{"flsVerdict", 1},
				{"id", "7306776e05664ed2b6aba60d717cfc1393ae31b4"},
				{"imageHash", "84123a3decdaa217e3588a1de59fe6cee1998004"},
				{"imagePath", "C:\\Windows\\explorer.exe"},
				{"pid", 1080},
				{"userName", "test@UT_LIBCLOUD"}
				})
			})},
		{"sessionUser", "test@UT_LIBCLOUD"},
		{"time", getCurrentTime()},
		{"type", 3},
		{"version", "1.1"}
		});

	const std::map<const std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vConfig, vParams
		{ "dict", { c_dictConfig, Dictionary({ { "data", dictEvent1 } }) } },
		{ "seq", { c_dictConfig, Dictionary({ { "data", Sequence({ dictEvent2, dictEvent3 }) } }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vParams] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vParams));
		}
	}
}
