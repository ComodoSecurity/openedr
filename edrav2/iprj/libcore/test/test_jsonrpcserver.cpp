//
// edrav2.ut_libcore project
// 
// Author: Yury Ermakov (31.01.2019)
// Reviewer:
//
// Unit tests for IPC subsystem.
//
#include "pch.h"

using namespace openEdr;

//
//
//
class SampleCommandProcessor : public ObjectBase<>, public ICommandProcessor
{
public:
	Variant execute(Variant vCommand, Variant vParams) override
	{
		if (vCommand == "echo")
			return vParams;
		
		if (vCommand == "sleep")
		{
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(std::chrono::seconds(vParams["duration"]));
			return {};
		}

		error::InvalidArgument(SL).throwException();
	}
};

//
//
//
TEST_CASE("JsonRpcServer.finalConstruct")
{
	auto fnScenario = [](const Variant& vConfig)
	{
		auto pServer = queryInterface<ICommandProcessor>(createObject(CLSID_JsonRpcServer, vConfig));
		(void)pServer->execute("start", {});

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);

		(void)pServer->execute("stop", {});
	};

	SECTION("missingProcessor")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({ {"port", 9999} })), error::InvalidArgument);
	}

	SECTION("missingPort")
	{
		Variant vProcessor = createObject<SampleCommandProcessor>();
		REQUIRE_THROWS_AS(fnScenario(Dictionary({ {"processor", vProcessor} })), error::InvalidArgument);
	}

	SECTION("invalidConfig")
	{
		REQUIRE_THROWS_AS(fnScenario(Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("JsonRpcServer.call")
{
	auto fnScenario = [](const Variant& vProcessor, const Variant& vCommand, const Variant& vParams = {})
	{
		auto pServer = queryInterface<ICommandProcessor>(createObject(
			CLSID_JsonRpcServer,
			Dictionary({
				{"port", 9999},
				{"processor", vProcessor}
				})
			));
		(void)pServer->execute("start", {});

		auto pCommand = createCommand(Dictionary({
			{"clsid", CLSID_JsonRpcClient},
			{"host", "127.0.0.1"},
			{"port", 9999}
			}), vCommand, vParams);

		Variant vResult = pCommand->execute();

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);

		(void)pServer->execute("stop", {});
	};
	
	Variant vProcessor = createObject<SampleCommandProcessor>();
	REQUIRE_NOTHROW(fnScenario(vProcessor, "echo"));
}
