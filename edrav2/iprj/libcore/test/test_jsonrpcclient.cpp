//
// edrav2.ut_libcore project
// 
// Author: Yury Ermakov (17.01.2019)
// Reviewer:
//
// Unit tests for IPC subsystem.
//
#include "pch.h"

using namespace cmd;

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

		if (vCommand == "throwException")
		{
			error::Exception(SL, static_cast<ErrorCode>(vParams["errCode"])).throwException();
			return {};
		}

		if (vCommand == "getStream")
		{
			static const uint8_t g_sData[] = "test";
			Variant vResult = Dictionary({
				{ "stream", io::createMemoryStream(g_sData, std::size(g_sData) - 1) }
				});
			return vResult;
		}

		error::InvalidArgument(SL).throwException();
	}
};

//
// @ermakov FIXME: Fix this test!
//
TEST_CASE("JsonRpcClient.ICommandProcessor", "[!mayfail]")
{
	auto fn = []()
	{
		auto pObj = createObject(CLSID_JsonRpcClient,
			Dictionary({ {"host", "127.0.0.1"}, {"port", 9999} }));
		auto pCommandProcessor = queryInterface<ICommandProcessor>(pObj);
		Variant vParams = Dictionary({
			{"data", Dictionary({
				{"event_id", 999},
				{"b", true},
				{"i", 999},
				{"s", "string"},
				{"d", Dictionary({ {"a", 1} })},
				{"q", Sequence({ 1, 2, 3})}
				})}
			});
		//std::cerr << "JSON-RPC Call: " << "put " << vParams << std::endl;
		Variant vResult = pCommandProcessor->execute("put", vParams);
		//std::cerr << "JSON-RPC Result: " << vResult << std::endl;
	};
	REQUIRE_NOTHROW(fn());
}

//
//
//
TEST_CASE("JsonRpcClient.call_to_unavailable_host")
{
	auto fn = []()
	{
		auto pObj = createObject(CLSID_JsonRpcClient,
			Dictionary({ {"host", "169.254.1.1"}, {"port", 9999} }));
		auto pCommandProcessor = queryInterface<ICommandProcessor>(pObj);
		Variant vParams = Dictionary({
			{"data", Dictionary({
				{"event_id", 999},
				{"b", true},
				{"i", 999},
				{"s", "string"},
				{"d", Dictionary({ {"a", 1} })},
				{"q", Sequence({ 1, 2, 3})}
				})}
			});
		//std::cerr << "JSON-RPC Call: " << "put " << vParams << std::endl;
		Variant vResult = pCommandProcessor->execute("put", vParams);
		//std::cerr << "JSON-RPC Result: " << vResult << std::endl;
	};
	REQUIRE_THROWS_AS(fn(), error::RuntimeError);
}

TEST_CASE("JsonRpcClient.create")
{
	auto fnScenario = [](const Variant& vConfig)
	{
		auto pObj = createObject(CLSID_JsonRpcClient, vConfig);
		auto pCommandProcessor = queryInterface<ICommandProcessor>(pObj);
		Variant vParams = Dictionary({
			{"data", Dictionary({
				{"event_id", 999},
				{"b", true},
				{"i", 999},
				{"s", "string"},
				{"d", Dictionary({ {"a", 1} })},
				{"q", Sequence({ 1, 2, 3})}
				})}
			});
		//std::cerr << "JSON-RPC Call: " << "put " << vParams << std::endl;
		Variant vResult = pCommandProcessor->execute("put", vParams);
		//std::cerr << "JSON-RPC Result: " << vResult << std::endl;
	};

	SECTION("missing_host")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({ {"port", 9999} })), error::InvalidArgument);
	}

	SECTION("missing_port")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({ {"host", "127.0.0.1"} })), error::InvalidArgument);
	}

	SECTION("unknown_protocol")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({
			{"host", "127.0.0.1"},
			{"port", 9999},
			{"protocol", "bla-bla-bla"}
			})), error::InvalidArgument);
	}

	SECTION("invalid_timeout")
	{
		REQUIRE_THROWS_AS(fnScenario(Dictionary({
			{"host", "127.0.0.1"},
			{"port", 9999},
			{"timeout", "bla-bla-bla"}
			})), error::TypeError);
	}

	SECTION("invalid_config")
	{
		REQUIRE_THROWS_AS(fnScenario(Variant(42)), error::InvalidArgument);
	}
}

//
//
//
TEST_CASE("JsonRpcClient.IDataReceiver", "[!mayfail]")
{
	auto fnScenario = []()
	{
		auto pObj = createObject(CLSID_JsonRpcClient,
			Dictionary({ {"host", "127.0.0.1"}, {"port", 9999} }));
		auto pDataReceiver = queryInterface<IDataReceiver>(pObj);
		Variant vData = Dictionary({
			{"event_id", 999},
			{"b", true},
			{"i", 999},
			{"s", "string"},
			{"d", Dictionary({ {"a", 1} })},
			{"q", Sequence({ 1, 2, 3})}
			});
		//std::cerr << "JSON-RPC Call: " << "put " << vParams << std::endl;
		pDataReceiver->put(vData);
		//std::cerr << "JSON-RPC Result: " << vResult << std::endl;
	};
	REQUIRE_NOTHROW(fnScenario());
}

//
//
//
TEST_CASE("JsonRpcClient.exception_transmission")
{
	auto fnScenario = [](const Variant& vProcessor, const Variant& vCommand, const Variant& vParams = {})
	{
		auto pServer = queryInterface<ICommandProcessor>(createObject(
			CLSID_JsonRpcServer,
			Dictionary({
				{ "port", 9999 },
				{ "processor", vProcessor }
				})
		));
		(void)pServer->execute("start", {});

		auto pCommand = createCommand(Dictionary({
			{ "clsid", CLSID_JsonRpcClient },
			{ "host", "127.0.0.1" },
			{ "port", 9999 },
			{ "timeout", 120 }
			}), vCommand, vParams);

		Variant vResult = pCommand->execute();

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);

		(void)pServer->execute("stop", {});
	};

	Variant vProcessor = createObject<SampleCommandProcessor>();

	SECTION("good")
	{
		REQUIRE_THROWS_AS(fnScenario(vProcessor, "throwException", Dictionary({
			{ "errCode", ErrorCode::SystemError }
			})), error::SystemError);
	}

	SECTION("unknown_error_code")
	{
		REQUIRE_THROWS_AS(fnScenario(vProcessor, "throwException", Dictionary({
			{ "errCode", 42 }
			})), error::Exception);
	}
}

//
//
//
TEST_CASE("JsonRpcClient.stream")
{
	auto fnScenario = [](const Variant& vProcessor, const Variant& vCommand, const Variant& vParams = {})
	{
		auto pServer = queryInterface<ICommandProcessor>(createObject(
			CLSID_JsonRpcServer,
			Dictionary({
				{ "port", 9999 },
				{ "processor", vProcessor }
				})
		));
		(void)pServer->execute("start", {});

		auto pCommand = createCommand(Dictionary({
			{ "clsid", CLSID_JsonRpcClient },
			{ "host", "127.0.0.1" },
			{ "port", 9999 },
			{ "timeout", 120 }
			}), vCommand, vParams);

		Variant vResult = pCommand->execute();

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);
	
		auto pReadStream = queryInterface<io::IReadableStream>(vResult["stream"]);
		REQUIRE(pReadStream->getSize() == 4);

		pReadStream->setPosition(0);
		std::vector<uint8_t> vecResult(4, 0);
		REQUIRE(pReadStream->read(&vecResult[0], vecResult.size()) == vecResult.size());
		REQUIRE(std::string_view((char*)vecResult.data(), vecResult.size()) == "test");

		(void)pServer->execute("stop", {});
	};

	Variant vProcessor = createObject<SampleCommandProcessor>();

	SECTION("good")
	{
		REQUIRE_NOTHROW(fnScenario(vProcessor, "getStream"));
	}
}

//
//
//
TEST_CASE("JsonRpcClient.encrypted")
{
	auto fnScenario = [](const Variant& vProcessor, const Variant& vCommand, const Variant& vParams = {})
	{
		auto pServer = queryInterface<ICommandProcessor>(createObject(
			CLSID_JsonRpcServer,
			Dictionary({
				{ "port", 9999 },
				{ "processor", vProcessor },
				{ "channelMode", "both" },
				{ "encryption", "aes" }
				})
		));
		(void)pServer->execute("start", {});

		auto pCommand = createCommand(Dictionary({
			{ "clsid", CLSID_JsonRpcClient },
			{ "host", "127.0.0.1" },
			{ "port", 9999 },
			{ "timeout", 120 },
			{ "channelMode", "encrypted" },
			{ "encryption", "aes" }
			}), vCommand, vParams);

		Variant vResult = pCommand->execute();

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(2s);
	
		auto pReadStream = queryInterface<io::IReadableStream>(vResult["stream"]);
		REQUIRE(pReadStream->getSize() == 4);

		pReadStream->setPosition(0);
		std::vector<uint8_t> vecResult(4, 0);
		REQUIRE(pReadStream->read(&vecResult[0], vecResult.size()) == vecResult.size());
		REQUIRE(std::string_view((char*)vecResult.data(), vecResult.size()) == "test");

		(void)pServer->execute("stop", {});
	};

	Variant vProcessor = createObject<SampleCommandProcessor>();

	SECTION("good")
	{
		REQUIRE_NOTHROW(fnScenario(vProcessor, "getStream"));
	}
}
