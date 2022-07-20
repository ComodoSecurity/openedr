//
// edrav2.ut_libcloud project
//
// Author: Yury Ermakov (20.03.2019)
// Reviewer:
//

#include "pch.h"
#include "config.h"
#include "../src/flsproto4.h"

using namespace cmd;
using namespace cmd::cloud;

//
//
//
TEST_CASE("fls_v4_UdpClient.getFileVerdict", "[!mayfail]")
{
	auto fnScenario = [](std::string_view sHost, Size nPort, const fls::HashList& hashList,
		const fls::FileVerdictMap& expected)
	{
		fls::v4::UdpClient client(sHost, nPort);
		auto result = client.getFileVerdict(hashList);
		CHECK(result == expected);
	};

	const std::map<const std::string, std::tuple<std::string, Size, fls::HashList, fls::FileVerdictMap>> mapData{
		// sName: sHost, nPort, hashList, expected
		{ "single", { fls::v4::c_sDefaultHost, fls::v4::c_nDefaultUdpPort, { {
			"5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690"
			} }, {
			{ "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Malicious }
			}, } },
		{ "multiple", { fls::v4::c_sDefaultHost, fls::v4::c_nDefaultUdpPort, { {
			{ "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690" },
			{ "0000000000000000000000000000000000000000" },
			} }, {
			{ "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Malicious },
			{ "0000000000000000000000000000000000000000", fls::FileVerdict::Absent },
			}, } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sHost, nPort, hashList, expected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(sHost, nPort, hashList, expected));
		}
	}

	const std::map<std::string, std::tuple<std::string, std::string, Size, fls::HashList>> mapBad{
		// sName: sHost, nPort, hashList
		{ "host_without_fls", {
			"receive_from: An existing connection was forcibly closed by the remote host",
			"127.0.0.1", 54321, {{ "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690" }} } },
		{ "unavailable_host", {
			//"Failed to send data (A socket operation was attempted to an unreachable network)",
			"send_to: A socket operation was attempted to an unreachable network",
			"169.254.1.1", 54321, {{ "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690" }} } },
		{ "unknown_host", { 
			"System error 0x00002AF9 - No such host is known",
			"bla.bla.bla.com", 54321, {{ "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690" }} } },
	};

	for (const auto& [sName, params] : mapBad)
	{
		const auto& [sMsg, sHost, nPort, hashList] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(sHost, nPort, hashList, {}), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("FlsService.getFileVerdict")
{
	auto fnScenario = [](Variant vConfig, const fls::Hash& sHash,
		fls::FileVerdict expected)
	{
		auto pFlsService = queryInterface<IService>(createObject(CLSID_FlsService, vConfig));
		pFlsService->start();

		auto pClient = queryInterface<fls::IFlsClient>(pFlsService);
		auto result = pClient->getFileVerdict(sHash);
		CHECK(result == expected);
		result = pClient->getFileVerdict(sHash);
		CHECK(result == expected);

		pFlsService->stop();
		pFlsService->shutdown();
	};

	const std::map<const std::string, std::tuple<Variant, std::string, fls::FileVerdict>> mapData{
		// sName: vConfig, sHash, expected
		{ "nocache", { {}, "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Malicious } },
		{ "cached", {
			Dictionary({ {"verdictCache", Sequence({ Dictionary({ {"verdict", 2}, {"purgeTimeout", 3600} }) })} }),
			"5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Malicious } },
		{ "unavailable_host", {Dictionary({{"url", "169.254.1.1"}, {"port", 54321}}), 
			"5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Unknown } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sHash, expected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sHash, expected));
		}
	}
}

//
//
//
TEST_CASE("FlsService.multithread")
{
	std::atomic_size_t nSuccessCounter = 0;
	std::atomic_size_t nUnknownCounter = 0;
	auto pFlsService = queryInterface<IService>(createObject(CLSID_FlsService));
	pFlsService->start();
	auto pClient = queryInterface<fls::IFlsClient>(pFlsService);

	auto fnScenario = [&nSuccessCounter, &nUnknownCounter, pClient](size_t nThreadNumber, size_t nMaxIndex, size_t nRepeateCount)
	{
		for (size_t j = 0; j < nRepeateCount; ++j)
			for (size_t i = 0; i < nMaxIndex; ++i)
			{
				char sHash[100];
				snprintf(sHash, sizeof(sHash), "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D7%02X%02X", 
					(unsigned int)nThreadNumber, (unsigned int)i);

				auto result = pClient->getFileVerdict(sHash);
				if (result == fls::FileVerdict::Absent)
					nSuccessCounter++;
				if (result == fls::FileVerdict::Unknown)
					nUnknownCounter++;
			}
	};


	static constexpr Size c_nThreadCount = 10;
	static constexpr Size c_nMaxIndex = 0x20;
	static constexpr Size c_nRepeateCount = 5;

	std::thread testThreads[c_nThreadCount];
	for(size_t i=0; i<std::size(testThreads); ++i)
		testThreads[i] = std::thread(fnScenario, i/2, c_nMaxIndex, c_nRepeateCount);

	for (auto& testThread : testThreads)
		testThread.join();

	pFlsService->stop();
	pFlsService->shutdown();
	REQUIRE(nSuccessCounter + nUnknownCounter == c_nThreadCount * c_nMaxIndex * c_nRepeateCount);
	if (nUnknownCounter != 0)
		WARN("Unknown count: " << nUnknownCounter << " from " << nSuccessCounter + nUnknownCounter);
}

//
//
//
TEST_CASE("FlsService.purgeCache")
{
	auto fnScenario = [](Variant vConfig, const fls::Hash& sHash,
		fls::FileVerdict expected)
	{
		auto pFlsService = queryInterface<IService>(createObject(CLSID_FlsService, vConfig));
		pFlsService->start();

		auto pClient = queryInterface<fls::IFlsClient>(pFlsService);
		pClient->getFileVerdict(sHash);
		auto result = pClient->getFileVerdict(sHash);
		CHECK(result == expected);
		result = pClient->getFileVerdict(sHash);
		CHECK(result == expected);

		pFlsService->stop();
		pFlsService->shutdown();
	};

	const std::map<const std::string, std::tuple<Variant, std::string, fls::FileVerdict>> mapData{
		// sName: vConfig, sHash, expected
		{ "cached", { Dictionary({
				{"timeout", 10000}, // 10s
				{"purgeMask", 1},
				{"verdictCache", Sequence({ Dictionary({ {"verdict", 2}, {"purgeTimeout", 0} }) })}
				}),
			"5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Malicious } }
	};

	for (const auto&[sName, params] : mapData)
	{
		const auto&[vConfig, sHash, expected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sHash, expected));
		}
	}
}

//
//
//
TEST_CASE("FlsService.isFileVerdictReady")
{
	auto fnScenario = [](Variant vConfig, const fls::Hash& sHash, fls::FileVerdict expected)
	{
		auto pFlsService = queryInterface<IService>(createObject(CLSID_FlsService, vConfig));
		pFlsService->start();

		auto pClient = queryInterface<fls::IFlsClient>(pFlsService);
		auto fResult = pClient->isFileVerdictReady(sHash);
		CHECK(!fResult);
		auto result = pClient->getFileVerdict(sHash);
		CHECK(result == expected);
		fResult = pClient->isFileVerdictReady(sHash);
		CHECK(fResult);

		pFlsService->stop();
		pFlsService->shutdown();
	};

	const std::map<const std::string, std::tuple<Variant, std::string, fls::FileVerdict>> mapData{
		// sName: vConfig, sHash, expected
		{ "cached", {
			Dictionary({ {"verdictCache", Sequence({ Dictionary({ {"verdict", 2}, {"purgeTimeout", 3600} }) })} }),
			"5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690", fls::FileVerdict::Malicious } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, sHash, expected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, sHash, expected));
		}
	}
}

//
//
//
TEST_CASE("FlsService.updateSettings")
{
	auto fnScenario = [](Variant vSettings)
	{
		auto pFlsService = queryInterface<IService>(createObject(CLSID_FlsService,
			Dictionary({ {"unavailableServerTimeout", 0} })));
		auto pProcessor = queryInterface<ICommandProcessor>(pFlsService);
		auto pClient = queryInterface<fls::IFlsClient>(pFlsService);
		pFlsService->start();

		execCommand(pProcessor, "updateSettings", vSettings);
		auto result = pClient->getFileVerdict("5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690");
		CHECK(result == fls::FileVerdict::Unknown);
		execCommand(pProcessor, "updateSettings", Dictionary({ {"url", fls::v4::c_sDefaultHost} }));
		//std::this_thread::sleep_for(std::chrono::milliseconds(500));
		result = pClient->getFileVerdict("5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690");
		CHECK(result == fls::FileVerdict::Malicious);

		pFlsService->stop();
		pFlsService->shutdown();
	};

	const std::map<std::string, std::tuple<Variant>> mapData{
		// sName: vSettings
		{ "host", { Dictionary({ {"url", "169.254.1.1"} }) } },
		{ "full_url", { Dictionary({ {"url", "http://169.254.1.1"} }) } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vSettings] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vSettings));
		}
	}
}

//
//
//
TEST_CASE("FlsService.enrichFileVerdict")
{
	auto fnScenario = [](Variant vConfig, Variant vData, fls::FileVerdict expectedVerdict,
		bool fCheckExpireTime)
	{
		auto pFlsService = queryInterface<IService>(createObject(CLSID_FlsService, vConfig));
		auto pProcessor = queryInterface<ICommandProcessor>(pFlsService);
		pFlsService->start();
		auto vResult = vData.clone();
		//execCommand(pProcessor, "enrichFileVerdict", vResult);
		createCommand(pProcessor, "enrichFileVerdict")->execute(vResult);
		CHECK(variant::getByPath(vResult, "file.fls.verdict", -1) == expectedVerdict);
		if (fCheckExpireTime)
		{
			CHECK(variant::getByPath(vResult, "file.fls", "").getSize() == 2);
			CHECK(variant::getByPath(vResult, "file.fls.verdictExpireTimeout", 0) != 0);
		}
		else
			CHECK(variant::getByPath(vResult, "file.fls", "").getSize() == 1);

		pFlsService->stop();
		pFlsService->shutdown();
	};

	const std::map<const std::string, std::tuple<Variant, Variant, fls::FileVerdict, bool>> mapData{
		// sName: vConfig, vData, expectedVerdict, fCheckExpireTime
		{ "trusted_microsoft", {
			Dictionary({ {"verdictCache", Sequence({ Dictionary({
				{"verdict", 1},
				{"purgeTimeout", 7200} // 2 hours
				}) })} }),
			Dictionary({ {"file", Dictionary({
				{"signature", Dictionary({ {"vendor", "Microsoft"} })},
				{"hash", ""}
				})} }),
			fls::FileVerdict::Safe,
			true } },
		{ "trusted_intel", {
			Dictionary({ {"verdictCache", Sequence({ Dictionary({
				{"verdict", 1},
				{"purgeTimeout", 7200} // 2 hours
				}) })} }),
			Dictionary({ {"file", Dictionary({
				{"signature", Dictionary({ {"vendor", "Intel"} })},
				{"hash", ""}
				})} }),
			fls::FileVerdict::Safe,
			true } },
		{ "unknown_signer", { {},
			Dictionary({ {"file", Dictionary({
				{"signature", Dictionary({ {"producer", "<unknown>"} })},
				{"hash", ""}
				})} }),
			fls::FileVerdict::Unknown,
			false } },
		{ "malware", {
			Dictionary({ {"verdictCache", Sequence({ Dictionary({
				{"verdict", 2},
				{"purgeTimeout", 7200} // 2 hours
				}) })} }),
			Dictionary({ {"file", Dictionary({
				{"hash", "5AD3F7C2BB8A5F7F7DA037C24AA0BC9675D76690"}
				})} }),
			fls::FileVerdict::Malicious,
			true } }
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vConfig, vData, expectedVerdict, fCheckExpireTime] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vConfig, vData, expectedVerdict, fCheckExpireTime));
		}
	}
}
