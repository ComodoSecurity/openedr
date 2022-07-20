//
// edrav2.libcloud.unittest
//
// Heartbeat service tests 
//
// Autor: Denis Bogdanov (16.03.2019)
// Reviewer: Yury Ermakov (02.07.2019)
//
#include "pch.h"

using namespace cmd;

// GCP Public environment
namespace stagging {
	static const char c_sGcpRootUrl[] = "http://api.staging.cwatchedr.com/endpoint";
	static const char c_sGcpClientId[] = "GcpClientIdPlaceholder";
	// FIXME: change "xxxxxxx" with real GcpToken value
	static const char c_sGcpToken[] = "xxxxxxx";
	static const char c_sGcpEndpointId[] = "GcpEndpointIdPlaceholder";
	static const char c_sGcpCustomerId[] = "GcpCustomerIdPlaceholder";
	static const char c_sGcpApiKey[] = "GcpApiKeyPlaceholder";
}

namespace production {
	//static const char c_sRootUrl2[] = "http://api2.cwatchedr.com/endpoint";
	static const char c_sGcpRootUrl[] = "https://api.cwatchedr.com/endpoint";
	// FIXME: change "xxxxxxx" with real  GcpToken value
	static const char c_sGcpToken[] = "xxxxxxxx";
	static const char c_sGcpClientId[] = "GcpClientIdPlaceholder";
	static const char c_sGcpEndpointId[] = "GcpEndpointIdPlaceholder";
	static const char c_sGcpCustomerId[] = "GcpCustomerIdPlaceholder";
	static const char c_sGcpApiKey[] = "GcpApiKeyPlaceholder";
}

using namespace production;

// AWS Production environment
static const char c_sAwsRootUrl[] = "https://wtfibam2s5.execute-api.us-west-2.amazonaws.com/production/";
// FIXME: put real AwsToken value
static const char c_sAwsToken[] = "xxxxxxx";
static const char c_sAwsEndpointSeed[] = "AwsEndpointSeedPlaceholder";
static const char c_sAwsEndpointId[] = "AwsEndpointIdPlaceholder";
static const char c_sAwsCustormerId[] = "AwsCustomerIdPlaceholder";

//
// FIXME: Move it somewhere
//
TEST_CASE("Common.parseVersion")
{
	auto fnScenario = [](std::string sVersion, Size nMajor, Size nMinor, Size nRevision, Size nBuild)
	{
		Variant v;
		REQUIRE_NOTHROW(v = parseVersion(sVersion));
		REQUIRE(v.get("major", c_nMaxSize) == nMajor);
		REQUIRE(v.get("minor", c_nMaxSize) == nMinor);
		REQUIRE(v.get("revision", c_nMaxSize) == nRevision);
		REQUIRE(v.get("build", c_nMaxSize) == nBuild);
	};

	SECTION("default")
	{
		fnScenario("1.2.3.4", 1, 2, 3, 4);
	}

	SECTION("zero")
	{
		fnScenario("0.0.0.00", 0, 0, 0, 0);
	}

	SECTION("default-two-digit")
	{
		fnScenario("01.20.03.40", 1, 20, 3, 40);
	}

	SECTION("with-suffix")
	{
		fnScenario("0.2.0-alpha1.200", 0, 2, 0, 200);
	}

	SECTION("short1")
	{
		fnScenario("10", 10, c_nMaxSize, c_nMaxSize, c_nMaxSize);
	}

	SECTION("short2")
	{
		fnScenario("10.2", 10, 2, c_nMaxSize, c_nMaxSize);
	}

	SECTION("short3")
	{
		fnScenario("10.2.3-alpha1", 10, 2, 3, c_nMaxSize);
	}

	SECTION("null")
	{
		fnScenario("", c_nMaxSize, c_nMaxSize, c_nMaxSize, c_nMaxSize);
	}

	SECTION("incomplete1")
	{
		fnScenario("10.10.", 10, 10, 0, c_nMaxSize);
	}

	SECTION("incomplete2")
	{
		fnScenario(".10.", 0, 10, 0, c_nMaxSize);
	}
}

//
//
//
TEST_CASE("CloudService.checkCredentials")
{
	//REQUIRE(variant::convert<variant::ValueType::String>(v["endpointID"]) == getCatalogData("app.config.extern.endpoint.license.endpointId"));
	//REQUIRE(variant::convert<variant::ValueType::String>(v["companyID"]) == getCatalogData("app.config.extern.endpoint.license.customerId"));
}

//
//
//
TEST_CASE("CloudService.enroll")
{
	auto fnScenario = [](Variant vConfig, Variant vParams = Dictionary())
	{
		auto pObj = queryInterface<ICommandProcessor>(createObject(CLSID_CloudService, vConfig));
		return execCommand(pObj, "enroll", vParams);
	};
	
	SECTION("edr_registration")
	{
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({ 
				{"customerId", c_sGcpCustomerId},
				{"endpointId", c_sGcpEndpointId},
				{"triesCount", 3},
				{"tryTimeout", 5000} 
			})));
		REQUIRE(v.has("endpointId"));
		REQUIRE(v.has("customerId"));

		REQUIRE(v["endpointId"] == c_sGcpEndpointId);
		REQUIRE(v["customerId"] == c_sGcpCustomerId);
	}
	
	SECTION("c1_registration")
	{
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({
				{"clientId", c_sGcpClientId},
				{"endpointId", c_sGcpEndpointId},
				{"triesCount", 3},
				{"tryTimeout", 5000}
			})));
		REQUIRE(v.has("endpointId"));
		REQUIRE(v.has("customerId"));

		REQUIRE(v["endpointId"] == c_sGcpEndpointId);
		REQUIRE(v["customerId"] == c_sGcpCustomerId);
	}
	
	SECTION("invalid_client")
	{
		Variant v;
		REQUIRE_THROWS_AS(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({
				{"clientId", "INVALID_CLIENT_ID"},
				{"endpointId", c_sGcpEndpointId},
				{"triesCount", 3},
				{"tryTimeout", 5000}
			})), error::ConnectionError);
	}
	
	SECTION("empty_endpoint")
	{
		Variant v;
		REQUIRE_THROWS_AS(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({
				{"clientId", c_sGcpCustomerId},
				{"customerId", c_sGcpCustomerId},
				{"triesCount", 3},
				{"tryTimeout", 5000}
			})), error::InvalidArgument);
	}

	SECTION("empty_client")
	{
		Variant v;
		REQUIRE_THROWS_AS(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({
				{"endpointId", c_sGcpEndpointId},
				{"triesCount", 3},
				{"tryTimeout", 5000}
			})), error::InvalidArgument);
	}

}

//
//
//
TEST_CASE("CloudService.getIdentity")
{
	auto sMachineId = sys::getUniqueMachineId(); // +"_new1";

	auto fnScenario = [](Variant vConfig, Variant vParams = Dictionary())
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_CloudService, vConfig));
		return execCommand(pObj, "getIdentity", vParams);
	};

	SECTION("default")
	{
		putCatalogData("app.config.license.token", c_sGcpToken);
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({ 
				{"token", c_sGcpToken},
				{"machineId", sMachineId},
				{"triesCount", 3},
				{"tryTimeout", 5000} 
			})));
		REQUIRE(v.has("endpointId"));
		REQUIRE(v.has("customerId"));
		//REQUIRE(v["endpointId"] == c_sGcpEndpointId);
		REQUIRE(v["customerId"] == c_sGcpCustomerId);
	}
	
	SECTION("invalid_token")
	{
		putCatalogData("app.config.license.token", c_sGcpToken);
		Variant v;
		REQUIRE_THROWS_AS(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({
				{"token", "BAD_TOKEN"},
				{"machineId", sMachineId},
				{"triesCount", 3},
				{"tryTimeout", 5000}
				})), error::ConnectionError);
	}
}

//
//
//
TEST_CASE("CloudService.getConfig")
{
	putCatalogData("app.config.license.endpointId", c_sGcpEndpointId);
	putCatalogData("app.config.license.customerId", c_sGcpCustomerId);

	auto fnScenario = [](Variant vConfig)
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_CloudService, vConfig));
		return execCommand(pObj, "getConfig", Dictionary({ {"triesCount", 3}, {"tryTimeout", 5000} }));
	};

	SECTION("default")
	{
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} })));
		REQUIRE(v.has("gcpPubSubTopic"));
		REQUIRE(v.has("gcpSACredentials"));
		REQUIRE(v.has("valkyrieURL"));
		REQUIRE(v.has("flsURL"));
		REQUIRE(v["gcpPubSubTopic"] == getCatalogData("app.config.extern.endpoint.cloud.gcp.pubSubTopic"));
		REQUIRE(v["gcpSACredentials"] == getCatalogData("app.config.extern.endpoint.cloud.gcp.saCredentials"));
		REQUIRE(v["valkyrieURL"] == getCatalogData("app.config.extern.endpoint.cloud.valkyrie.url"));
		REQUIRE(v["flsURL"] == getCatalogData("app.config.extern.endpoint.cloud.fls.url"));
	}

}

//
//
//
TEST_CASE("CloudService.getToken")
{
	auto fnScenario = [](Variant vConfig, Variant vParams = Dictionary())
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_CloudService, vConfig));
		return execCommand(pObj, "getToken", vParams);
	};

	SECTION("default")
	{
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} }),
			Dictionary({ 
				{"legacyCustomerId", c_sAwsCustormerId}, 
				{"apiKey", c_sGcpApiKey},
				{"triesCount", 3},
				{"tryTimeout", 5000} 
			})));
		REQUIRE(v.has("token"));
	}
}

//
//
//
TEST_CASE("CloudService.getPolicy")
{
	putCatalogData("app.config.license.endpointId", c_sGcpEndpointId);
	putCatalogData("app.config.license.customerId", c_sGcpCustomerId);
	putCatalogData("app.config.policy.groups.eventsMatching.source.evmCloud", Dictionary());

	auto fnScenario = [](Variant vConfig)
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_CloudService, vConfig));
		return execCommand(pObj, "getPolicy", Dictionary({ {"triesCount", 3}, {"tryTimeout", 5000} }));
	};

	SECTION("default")
	{
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} })));
		REQUIRE(v.has("policy"));
	}
}

//
//
//
TEST_CASE("CloudService.doHeartbeat")
{
	putCatalogData("app.config.license.endpointId", c_sGcpEndpointId);
	putCatalogData("app.config.license.customerId", c_sGcpCustomerId);

	auto fnScenario = [](Variant vConfig)
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_CloudService, vConfig));
		return execCommand(pObj, "doHeartbeat");
	};

	SECTION("default")
	{
		Variant v;
		REQUIRE_NOTHROW(v = fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} })));
		if (!v.isNull())
		{
			REQUIRE(v.has("command"));
			REQUIRE((v.has("commandID") || v.has("commandId")));
		}
	}

}

//
//
//
TEST_CASE("CloudService.reportEndpointInfo")
{
	putCatalogData("app.config.license.endpointId", c_sGcpEndpointId);
	putCatalogData("app.config.license.customerId", c_sGcpCustomerId);
	putCatalogData("os.hostName", "test-host");
	putCatalogData("os.osName", "Windows 10");
	putCatalogData("os.bootTime", getCurrentTime());
	putCatalogData("objects.userDataProvider", nullptr); // Remove old userDataProvider
	putCatalogData("objects.userDataProvider", createObject(CLSID_UserDataProvider));

	auto fnScenario = [](Variant vConfig)
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_CloudService, vConfig));
		auto v = execCommand(pObj, "reportEndpointInfo", Dictionary({ {"full", true} }));
		REQUIRE(v.has("computerName"));
		REQUIRE(v.has("operatingSystem"));
		REQUIRE(v.has("lastReboot"));
		REQUIRE(v.has("agentVersion"));
		REQUIRE(v.has("localIPAddress"));
		REQUIRE(v.has("loggedOnUser"));

		auto v2 = execCommand(pObj, "reportEndpointInfo", Dictionary({ {"full", false} }));
		REQUIRE(v2.isEmpty());

		(void) execCommand(pObj, "resetDataCache");

		auto v3 = execCommand(pObj, "reportEndpointInfo", Dictionary({ {"full", false} }));
		REQUIRE(!v3.isEmpty());

	};

	SECTION("default")
	{
		REQUIRE_NOTHROW(fnScenario(Dictionary({ {"rootUrl", c_sGcpRootUrl} })));
	}

}

//
//
//
TEST_CASE("LegacyCloudService.getConfig", "[!hide]")
{
	putCatalogData("app.config.license.machineId", sys::getUniqueMachineId());
	putCatalogData("app.config.license.token", c_sAwsToken);
	putCatalogData("objects.userDataProvider", nullptr); // Remove old userDataProvider
	putCatalogData("objects.userDataProvider", createObject(CLSID_UserDataProvider));

	auto fnScenario = [](Variant vConfig)
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_LegacyCloudService, vConfig));
		Variant v;
		REQUIRE_NOTHROW(v = execCommand(pObj, "getConfig"));
		return v;
	};
	
	SECTION("clean")
	{
		auto v = fnScenario(Dictionary({ {"rootUrl", c_sAwsRootUrl} }));
		REQUIRE(v.has("endpointID"));
		REQUIRE(getCatalogData("app.config.extern.endpoint.license.endpointId", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.license.customerId", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.license.token", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.license.machineId", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.aws.accessKey", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.aws.secretKey", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.aws.deliveryStream", "") != "");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.valkyrie.apiKey", "") != "");
	}

	SECTION("existing")
	{
		putCatalogData("app.config.extern.endpoint.cloud.endpointId", "000000000000000000000000");
		putCatalogData("app.config.extern.endpoint.cloud.customerId", "00000000-0000-0000-0000-000000000000");
		putCatalogData("app.config.extern.endpoint.cloud.aws.accessKey", "00000000");
		putCatalogData("app.config.extern.endpoint.cloud.aws.secretKey", "00000000");
		putCatalogData("app.config.extern.endpoint.cloud.aws.deliveryStream", "00000000");
		putCatalogData("app.config.extern.endpoint.cloud.valkyrie.apiKay", "00000000");
		auto v = fnScenario(Dictionary({ {"rootUrl", c_sAwsRootUrl} }));
		REQUIRE(v.has("endpointID"));
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.endpointId", "") == "000000000000000000000000");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.customerId", "") == "00000000-0000-0000-0000-000000000000");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.aws.accessKey", "") != "00000000");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.aws.secretKey", "") != "00000000");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.aws.deliveryStream", "") != "00000000");
		REQUIRE(getCatalogData("app.config.extern.endpoint.cloud.valkyrie.apiKey", "") != "00000000");
	}

}

//
//
//
TEST_CASE("LegacyCloudService.doHeartbeat", "[!hide]")
{
	putCatalogData("app.config.license.endpointId", c_sAwsEndpointId);
	putCatalogData("app.config.license.customerId", c_sAwsCustormerId);
	putCatalogData("app.config.policy.source", Dictionary());
	putCatalogData("app.config.policy.active", Dictionary());
	putCatalogData("os.hostName", "test-host");
	putCatalogData("os.domainName", "test-domain");
	putCatalogData("os.osName", "Windows 10");
	putCatalogData("os.bootTime", getCurrentTime());
	putCatalogData("objects.userDataProvider", nullptr); // Remove old userDataProvider
	putCatalogData("objects.userDataProvider", createObject(CLSID_UserDataProvider));

	auto fnScenario = [](Variant vConfig)
	{
		auto pObj = queryInterface<ICommandProcessor>(
			createObject(CLSID_LegacyCloudService, vConfig));
		Variant v;
		REQUIRE_NOTHROW(v = execCommand(pObj, "doHeartbeat"));
		return v;
	};

	SECTION("default")
	{
		auto v = fnScenario(Dictionary({ {"rootUrl", c_sAwsRootUrl} }));
		REQUIRE(v.has("status"));
	}

	SECTION("high_version")
	{
		auto v = fnScenario(Dictionary({ {"rootUrl", c_sAwsRootUrl},
			{"currVersion", "100.1.1.2"} }));
		REQUIRE(v.has("status"));
		REQUIRE(!getCatalogData("app").has("update"));
	}

	SECTION("low_version")
	{
		auto v = fnScenario(Dictionary({ {"rootUrl", c_sAwsRootUrl},
			{"currVersion", "0.1.1.2"} }));
		REQUIRE(v.has("status"));
		REQUIRE(getCatalogData("app.update", Dictionary()).has("url"));
		REQUIRE(getCatalogData("app.update", Dictionary()).has("version"));
	}

	SECTION("new_policy")
	{
		auto v = fnScenario(Dictionary({ {"rootUrl", c_sAwsRootUrl} }));
		REQUIRE(v.has("status"));
		REQUIRE(getCatalogData("app.config.policy.source", Dictionary()).has("hash"));
	}

}

