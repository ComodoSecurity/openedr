//
// edrav2.ut_libcloud project
//
// Author: Yury Ermakov (11.03.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"

using namespace openEdr;

const std::string_view c_sCustomerId = "CustomerIdPlaceholder";
const std::string_view c_sEndpointId = "EndpointIdPlaceholder"; // UT_LIBCLOUD
const std::string_view c_sDeliveryStream = "edr-evt-stream-development-central-v060";
const std::string_view c_sAccessKey = "AccessKeyPlaceholder";
const std::string_view c_sSecretKey = "SecretKeyPlaceholder";

//
//
//
TEST_CASE("AwsFirehoseClient.create")
{
	auto fnScenario = [&](Variant vCatalogConfig, Variant vConfig)
	{
		if (vCatalogConfig.isDictionaryLike())
			for (const auto& [sPath, vValue] : Dictionary(vCatalogConfig))
				putCatalogData(FMT("app.config.aws." << sPath), vValue);
		else
			putCatalogData("app.config.aws", vCatalogConfig);

		auto pObj = createObject(CLSID_AwsFirehoseClient, vConfig);
	};

	const std::map<const std::string, std::tuple<Variant, Variant>> mapData{
		// sName: vCatalogConfig, vConfig
		{ "catalog_conf", {
			Dictionary({
				{ "deliveryStream", c_sDeliveryStream },
				{ "accessKey", c_sAccessKey },
				{ "secretKey", c_sSecretKey }
				}),
			{}
			} },
		{ "custom_conf", {
			{},
			Dictionary({
				{ "deliveryStream", c_sDeliveryStream },
				{ "accessKey", c_sAccessKey },
				{ "secretKey", c_sSecretKey }
				})
			} },
		{ "mixed_conf", {
			Dictionary({
				{ "accessKey", c_sAccessKey },
				{ "secretKey", c_sSecretKey }
				}),
			Dictionary({
				{ "deliveryStream", c_sDeliveryStream },
				})
			} },
		};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vCatalogConfig, vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vCatalogConfig, vConfig));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBad{
		// sName: sMsg, vCatalogConfig, vConfig 
		{ "catalog_conf_not_dict", { "String keys are not applicable for type <Integer>. Key: <deliveryStream>", 42, {} } },
		{ "custom_conf_not_dict", { "String keys are not applicable for type <Integer>. Key: <deliveryStream>", {}, 42 } },
		{ "deliveryStream_missing", { "Invalid dictionary index <deliveryStream>", {}, Dictionary() } },
		{ "accessKey_missing", { "Invalid dictionary index <accessKey>", {}, 
			Dictionary({ { "deliveryStream", "" }, { "secretKey", "" } }) } },
		{ "secretKey_missing", { "Invalid dictionary index <secretKey>", {},
			Dictionary({ { "deliveryStream", "" }, { "accessKey", "" } }) } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vCatalogConfig, vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vCatalogConfig, vConfig), sMsg);
		}
	}
}

//
//
//
TEST_CASE("AwsFirehoseClient.submit", "[!hide]")
{
	auto fnScenario = [&](Variant vConfig, Variant vParams, bool fExpected)
	{
		putCatalogData("app.config.aws", {});
		auto pObj = createObject(CLSID_AwsFirehoseClient, vConfig);
		auto pProcessor = queryInterface<ICommandProcessor>(pObj);
		bool fResult = pProcessor->execute("submit", vParams);
		REQUIRE(fResult == fExpected);
	};

	const auto c_dictConfig = Dictionary({
		{ "deliveryStream", c_sDeliveryStream },
		{ "accessKey", c_sAccessKey },
		{ "secretKey", c_sSecretKey }
		});

	const auto dictEvent1 = Dictionary({
		{ "SequenceId", 1 },
		{ "baseEventType", 7 },
		{ "customerId", c_sCustomerId },
		{ "deviceName", "UT_LIBCLOUD" },
		{ "endpointId", c_sEndpointId },
		{ "eventGroup", "FILE" },
		{ "eventTime", getCurrentISO8601TimeUTC() },
		{ "eventType", "8b85e917-3619-45e4-9416-0a21ea691a33" },
		{ "internalIp", Sequence({ "172.16.101.10", "10.100.136.119" }) },
		{ "loggedOnUser", "user1" },
		{ "processes", Sequence({
			Dictionary({
				{ "imageHash", "6fdb86aa78c3a54147a32696664e1baa2cb2297c" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-3000) },
				{ "processId", 968 },
				{ "processPath", "C:\\Windows\\System32\\winlogon.exe" },
				{ "userDomain", "NT AUTHORITY" },
				{ "userName", "SYSTEM" },
				{ "verdict", 1 }
				}),
			Dictionary({
				{ "imageHash", "17508b76fbc5dbcd451d236f48a6e0c0c6db02cf" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-2000) },
				{ "processId", 7840 },
				{ "processPath", "C:\\Windows\\system32\\userinit.exe" },
				{ "userDomain", "COMODO" },
				{ "userName", "user1" },
				{ "verdict", 1 }
				}),
			Dictionary({
				{ "imageHash", "408fe28868b5ac008b5df98b9928a2fa6543d0d7" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-1000) },
				{ "processId", 7228 },
				{ "processPath", "C:\\WINDOWS\\Explorer.EXE" },
				{ "userDomain", "COMODO" },
				{ "userName", "user1" },
				{ "verdict", 1 }
				})
			}) },
		{ "status", "SUCCESS" },
		{ "file", Dictionary({
			{ "attributeUpdates", Sequence() },
			{ "fileHash", "409be5b11b00f3e68d0b7eeb404d8ffb898f51a2" },
			{ "ftype", "OTHER" },
			{ "name", "sample.PDF" },
			{ "oldPath", "" },
			{ "parentProcessPath", "C:\\WINDOWS\\Explorer.EXE" },
			{ "path", "C:\\Users\\abc\\sample.PDF" },
			{ "productId", "" },
			{ "size", 0 },
			{ "timeUpdates", Sequence() },
			{ "vendorId", "" }
			}) },
		});

	const auto dictEvent2 = Dictionary({
		{ "SequenceId", 2 },
		{ "baseEventType", 1 },
		{ "childProcess", Dictionary({
			{ "cmdLine", "C:\\Windows\\system32\\SearchFilterHost.exe" },
			{ "creationTime", getCurrentISO8601TimeUTC(-100) },
			{ "elevationType", "TYPE1" },
			{ "id", 1652 },
			{ "imageHash", "fadfc40e929a22cceeb464d16ad823e4e71c14e4" },
			{ "isPacked", false },
			{ "isSigned", true },
			{ "isSys", false },
			{ "loadAddress", "C:\\Windows\\system32\\SearchFilterHost.exe" },
			{ "parentProcessPath", "C:\\Windows\\System32\\SearchIndexer.exe" },
			{ "path", "C:\\Windows\\system32\\SearchFilterHost.exe" },
			{ "product", "Windows Search" },
			{ "terminationTime", "" },
			{ "vendor", "Microsoft Windows" },
			{ "verdict", 1 },
			{ "version", "7.00.7601.23930" },
			}) },
		{ "customerId", c_sCustomerId },
		{ "deviceName", "UT_LIBCLOUD" },
		{ "endpointId", c_sEndpointId },
		{ "eventGroup", "PROCESS" },
		{ "eventTime", getCurrentISO8601TimeUTC() },
		{ "eventType", {} },
		{ "internalIp", Sequence({ "10.0.2.15" }) },
		{ "loggedOnUser", "user1" },
		{ "processes", Sequence({
			Dictionary({
				{ "imageHash", "c7bba9840c44e7739fb314b7a3efe30e6b25cc48" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-3000) },
				{ "processId", 400 },
				{ "processPath", "C:\\Windows\\System32\\wininit.exe" },
				{ "userDomain", "NT AUTHORITY" },
				{ "userName", "System" },
				{ "verdict", 1 },
				}),
			Dictionary({
				{ "imageHash", "deb12390e5089919586ae37f1597a09c60b126ad" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-2000) },
				{ "processId", 448 },
				{ "processPath", "C:\\Windows\\System32\\services.exe" },
				{ "userDomain", "NT AUTHORITY" },
				{ "userName", "System" },
				{ "verdict", 1 },
				}),
			 Dictionary({
				 { "imageHash", "7fe6b10868d4817c38d9e47c803c2a6171457bb2" },
				 { "processCreationTime", getCurrentISO8601TimeUTC(-1000) },
				 { "processId", 2096 },
				 { "processPath", "C:\\Windows\\System32\\SearchIndexer.exe" },
				 { "userDomain", "NT AUTHORITY" },
				 { "userName", "System" },
				 { "verdict", 1 },
				})
			}) },
		{ "status", "SUCCESS" }
		});
	
	const auto dictEvent3 = Dictionary({
		{ "SequenceId", 2 },
		{ "baseEventType", 1 },
		{ "childProcess", Dictionary({
			{ "cmdLine", "C:\\Windows\\system32\\SearchFilterHost.exe" },
			{ "creationTime", getCurrentISO8601TimeUTC(-100) },
			{ "elevationType", "TYPE1" },
			{ "id", 1652 },
			{ "imageHash", "fadfc40e929a22cceeb464d16ad823e4e71c14e4" },
			{ "isPacked", false },
			{ "isSigned", true },
			{ "isSys", false },
			{ "loadAddress", "C:\\Windows\\system32\\SearchFilterHost.exe" },
			{ "parentProcessPath", "C:\\Windows\\System32\\SearchIndexer.exe" },
			{ "path", "C:\\Windows\\system32\\SearchFilterHost.exe" },
			{ "product", "Windows Search" },
			{ "terminationTime", "" },
			{ "vendor", "Microsoft Windows" },
			{ "verdict", 1 },
			{ "version", "7.00.7601.23930" },
			}) },
		{ "customerId", c_sCustomerId },
		{ "deviceName", "UT_LIBCLOUD" },
		{ "endpointId", c_sEndpointId },
		{ "eventGroup", "PROCESS" },
		{ "eventTime", getCurrentISO8601TimeUTC() },
		{ "eventType", {} },
		{ "internalIp", Sequence({ "10.0.2.15" }) },
		{ "loggedOnUser", "user1" },
		{ "processes", Sequence({
			Dictionary({
				{ "imageHash", "c7bba9840c44e7739fb314b7a3efe30e6b25cc48" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-3000) },
				{ "processId", 400 },
				{ "processPath", "C:\\Windows\\System32\\wininit.exe" },
				{ "userDomain", "NT AUTHORITY" },
				{ "userName", "System" },
				{ "verdict", 1 },
				}),
			Dictionary({
				{ "imageHash", "deb12390e5089919586ae37f1597a09c60b126ad" },
				{ "processCreationTime", getCurrentISO8601TimeUTC(-2000) },
				{ "processId", 448 },
				{ "processPath", "C:\\Windows\\System32\\services.exe" },
				{ "userDomain", "NT AUTHORITY" },
				{ "userName", "System" },
				{ "verdict", 1 },
				}),
			 Dictionary({
				 { "imageHash", "7fe6b10868d4817c38d9e47c803c2a6171457bb2" },
				 { "processCreationTime", getCurrentISO8601TimeUTC(-1000) },
				 { "processId", 2096 },
				 { "processPath", "C:\\Windows\\System32\\SearchIndexer.exe" },
				 { "userDomain", "NT AUTHORITY" },
				 { "userName", "System" },
				 { "verdict", 1 },
				})
			}) },
		{ "status", "SUCCESS" }
		});

	const std::map<const std::string, std::tuple<Variant, Variant, bool>> mapData{
		// sName: vConfig, vParams, fExpected
		{ "dict", { c_dictConfig, Dictionary({ { "data", dictEvent3 } }), true } },
		{ "seq", { c_dictConfig, Dictionary({ { "data", Sequence({ dictEvent1, dictEvent2 }) } }), true } },
		{ "error_send", {
			Dictionary({
				{ "deliveryStream", c_sDeliveryStream },
				{ "accessKey", c_sAccessKey },
				{ "secretKey", "bad-key" }
				}),
			Dictionary({ { "data", Dictionary() } }), false } }
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vParams, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vParams, fExpected));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBad{
		// sName: sMsg, vConfig, vParams
		{ "no_data", { "Missing field <data> in parameters",
			c_dictConfig, Dictionary() } },
		{ "data_invalid_type", { "Field <data> must be a sequence or a dictionary",
			c_dictConfig, Dictionary({ { "data", 42 } }) } }
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vConfig, vData] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig, vData, false), sMsg);
		}
	}
}
