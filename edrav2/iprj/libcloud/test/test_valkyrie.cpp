//
// edrav2.ut_libcloud project
//
// Author: Yury Ermakov (01.04.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"

using namespace cmd;
using namespace cmd::cloud;


// Production
const char c_sApiKey[] = "75c810a9-1584-4a5b-bcbe-b4deeed521b5";
const char c_sUrl[] = "https://valkyrie.comodo.com";
const char c_sUrlHttp[] = "valkyrie.comodo.com";
const char c_sEndpointId[] = "AwsEndpointIdPlaceholder";

//
//
//
TEST_CASE("ValkyrieService.create")
{
	auto fnScenario = [](Variant vConfig, bool fCreateFileDataProvider)
	{
		putCatalogData("objects", Dictionary{});
		if (fCreateFileDataProvider)
		{
			putCatalogData("objects.userDataProvider", createObject(CLSID_UserDataProvider));
			putCatalogData("objects.signatureDataProvider", createObject(CLSID_SignDataProvider));
			putCatalogData("objects.fileDataProvider", createObject(CLSID_FileDataProvider));
		}
		auto pValkyrieService = queryInterface<IService>(createObject(CLSID_ValkyrieService, vConfig));
	};

	const std::map<const std::string, std::tuple<Variant, bool>> mapData{
		// sName: vConfig, fCreateFileDataProvider
		{ "good", {
			Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} }), true } }
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, fCreateFileDataProvider] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, fCreateFileDataProvider));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, bool>> mapBad{
		// sName: sMsg, vConfig, fCreateFileDataProvider
		{ "no_apiKey", { "Missing field <apiKey>",
			Dictionary({ {"url", c_sUrl}, {"endpointId", c_sEndpointId} }), true } },
		{ "no_url", { "Missing field <url>",
			Dictionary({ {"apiKey", c_sApiKey}, {"endpointId", c_sEndpointId} }), true } },
		{ "no_endpointId", { "Missing field <endpointId>",
			Dictionary({ {"apiKey", c_sApiKey}, {"url", c_sUrl} }), false } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, vConfig, fCreateFileDataProvider] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig, fCreateFileDataProvider), sMsg);
		}
	}
}

//
//
//
TEST_CASE("ValkyrieService.submit", "[!mayfail]")
{
	auto fnScenario = [](Variant vConfig, Variant vFile, bool fExpected)
	{
		try
		{
			auto pFile = io::createFileStream(std::filesystem::path(vFile["path"]), io::FileMode::Read);
			auto hash = crypt::sha1::getHash(pFile);
			vFile.put("hash", string::convertToHex(std::begin(hash.byte), std::end(hash.byte)));
		}
		catch (...) {}
		 
		// remove old services
		removeGlobalObject("userDataProvider");
		removeGlobalObject("signatureDataProvider");
		removeGlobalObject("fileDataProvider");
		removeGlobalObject("flsService");

		addGlobalObject("userDataProvider", createObject(CLSID_UserDataProvider));
		addGlobalObject("signatureDataProvider", createObject(CLSID_SignDataProvider));
		addGlobalObject("fileDataProvider", createObject(CLSID_FileDataProvider));
		addGlobalObject("flsService", createObject(CLSID_FlsService));
		auto pValkyrieService = queryInterface<IService>(createObject(CLSID_ValkyrieService, vConfig));
		auto pProcessor = queryInterface<ICommandProcessor>(pValkyrieService);
		pValkyrieService->start();
		auto fResult = execCommand(pProcessor, "submit", Dictionary({ {"file", vFile} }));
		REQUIRE(fResult == fExpected);
		pValkyrieService->stop();
	};

	const std::map<std::string, std::tuple<Variant, Variant, bool>> mapData{
		// sName: vConfig, vFile, fExpected
		// Use HTTP URL for testing if redirect to HTTPs is working
		{ "under_1mb", {
			Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrlHttp}, {"endpointId", c_sEndpointId} }),
			Dictionary({ {"path", "C:\\Windows\\System32\\notepad.exe"} }), true } },
		{ "over_1mb", {
			Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} }),
			Dictionary({ {"path", "C:\\Windows\\System32\\mmc.exe"} }), true } },
		//{ "over_10mb", { Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl} }),
		//	Dictionary({ {"path", "C:\\Windows\\System32\\wmp.dll"} }), true } },
		//{ "over_20mb", { Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl} }),
		//	Dictionary({ {"path", "C:\\Windows\\System32\\mshtml.dll"} }), true } }
		//{ "max_file_size_1", {
		//	Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} }),
		//	Dictionary({ {"path", "C:\\Windows\\System32\\MRT.exe" } }), true } },
		{ "max_file_size_2", {
			Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} }),
			Dictionary({ {"path", "C:\\Windows\\System32\\MRT.exe" }, {"size", 86 * 1024 * 1024} }), true } },
		{ "invalid_apikey", {
			Dictionary({ {"apiKey", "<invalid>" }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} }),
			Dictionary({ {"path", "C:\\Windows\\System32\\notepad.exe"} }), true } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vFile, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vFile, fExpected));
		}
	}
}

//
//
//
TEST_CASE("ValkyrieService.submit_to_stopped")
{
	auto fnScenario = [](Variant vConfig, Variant vFile)
	{
		try
		{
			auto pFile = io::createFileStream(std::filesystem::path(vFile["path"]), io::FileMode::Read);
			auto hash = crypt::sha1::getHash(pFile);
			vFile.put("hash", string::convertToHex(std::begin(hash.byte), std::end(hash.byte)));
		}
		catch (...) {}
		 
		// remove old services
		removeGlobalObject("userDataProvider");
		removeGlobalObject("signatureDataProvider");
		removeGlobalObject("fileDataProvider");
		removeGlobalObject("flsService");

		addGlobalObject("userDataProvider", createObject(CLSID_UserDataProvider));
		addGlobalObject("signatureDataProvider", createObject(CLSID_SignDataProvider));
		addGlobalObject("fileDataProvider", createObject(CLSID_FileDataProvider));
		addGlobalObject("flsService", createObject(CLSID_FlsService));
		auto pValkyrieService = queryInterface<IService>(createObject(CLSID_ValkyrieService, vConfig));
		auto pProcessor = queryInterface<ICommandProcessor>(pValkyrieService);
		execCommand(pProcessor, "submit", Dictionary({ {"file", vFile} }));
	};

	Variant vConfig = Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} });
	Variant vFile = Dictionary({ {"path", "C:\\Windows\\System32\\mmc.exe"} });
	REQUIRE_THROWS_AS(fnScenario(vConfig, vFile), error::OperationDeclined);
}

//
//
//
TEST_CASE("ValkyrieService.updateSettings")
{
	auto fnScenario = [](Variant vSettings, bool fExpected)
	{
		// remove old services
		removeGlobalObject("userDataProvider");
		removeGlobalObject("signatureDataProvider");
		removeGlobalObject("fileDataProvider");
		removeGlobalObject("flsService");

		addGlobalObject("userDataProvider", createObject(CLSID_UserDataProvider));
		addGlobalObject("signatureDataProvider", createObject(CLSID_SignDataProvider));
		addGlobalObject("fileDataProvider", createObject(CLSID_FileDataProvider));
		addGlobalObject("flsService", createObject(CLSID_FlsService));

		Variant vConfig = Dictionary({ {"apiKey", c_sApiKey }, {"url", c_sUrl}, {"endpointId", c_sEndpointId} });
		Variant vFile = Dictionary({ {"path", "C:\\Windows\\System32\\notepad.exe"} });
		auto pFile = io::createFileStream(std::filesystem::path(vFile["path"]), io::FileMode::Read);
		auto hash = crypt::sha1::getHash(pFile);
		vFile.put("hash", string::convertToHex(std::begin(hash.byte), std::end(hash.byte)));

		auto pValkyrieService = queryInterface<IService>(createObject(CLSID_ValkyrieService, vConfig));
		auto pProcessor = queryInterface<ICommandProcessor>(pValkyrieService);
		auto pClient = queryInterface<valkyrie::IValkyrieClient>(pValkyrieService);
		pValkyrieService->start();

		Variant vData = Dictionary({ {"file", vFile} });
		execCommand(pProcessor, "updateSettings", vSettings);
		auto fResult = execCommand(pProcessor, "submit", vData);
		CHECK(fResult == fExpected);
		execCommand(pProcessor, "updateSettings", vConfig);
		fResult = execCommand(pProcessor, "submit", vData);
		CHECK(fResult);

		pValkyrieService->stop();
	};

	const std::map<std::string, std::tuple<Variant, bool>> mapData{
		// sName: vSettings
		{ "url", { Dictionary({ {"url", "169.254.1.1"} }), false } },
		{ "apiKey", { Dictionary({ {"apiKey", "<invalid>"} }), true } }
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vSettings, fExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vSettings, fExpected));
		}
	}
}
