//
//  EDRAv2. Unit test
//    Command subsystem test
//
#include "pch.h"

using namespace openEdr;
/*
class ExampleCommandProcessorError : public error::RuntimeError
{
private:
	std::exception_ptr getRealException(bool fAddTrace) override
	{
		if (fAddTrace)
			addTrace(m_sourceLocation.sFileName, m_sourceLocation.nLine, what());
		return std::make_exception_ptr(*this);
	}

public:
	ExampleCommandProcessorError(SourceLocation sourceLocation) :
		error::RuntimeError(sourceLocation) {}
	virtual ~ExampleCommandProcessorError() {}
};*/

typedef error::detail::ExceptionBaseTmpl<error::RuntimeError, ErrorCode::Undefined> ExampleCommandProcessorError;

class ExampleCommandProcessor : public ObjectBase<>, public ICommandProcessor
{
public:
	virtual Variant execute(Variant vCommand, Variant vParams) override
	{
		if (vCommand == "passParams")
		{
			return vParams;
		}

		if (vCommand == "throwException")
		{
			ExampleCommandProcessorError(SL).throwException();
		}

		// Command is not string
		if(vCommand.getType() != Variant::ValueType::String)
		{
			return vCommand;
		}
		error::InvalidArgument(SL).throwException();
		// Here is an unreachable code to suppress "error C4716:: must return a value"
		return {};
	}
};


static constexpr Size c_nExecWayCount = 8;
//
// Testing call command with different ways [0, c_nExecWayCount)
//
Variant testCommandExecution(Variant vProcessor, Variant vCommand, Variant vParams, Size nWay)
{
	switch (nWay)
	{
		case 0:
		{
			auto pCmd = queryInterface<ICommand>(createObject(CLSID_Command, Dictionary({
				{"processor", vProcessor},
				{"command", vCommand},
				{"params", vParams},
				})));
			return pCmd->execute();
		}
		case 1:
		{
			auto pCmd = queryInterface<ICommand>(createObject(CLSID_Command, Dictionary({
				{"processor", vProcessor},
				{"command", vCommand},
				})));
			return pCmd->execute(vParams);
		}
		case 2:
		{
			auto pCmd = createCommand(Dictionary({
				{"processor", vProcessor},
				{"command", vCommand},
				{"params", vParams},
				}));
			return pCmd->execute();
		}
		case 3:
		{
			auto pCmd = createCommand(Dictionary({
				{"processor", vProcessor},
				{"command", vCommand},
				}));
			return pCmd->execute(vParams);
		}
		case 4:
		{
			auto pCmd = createCommand(vProcessor, vCommand);
			return pCmd->execute(vParams);

		}
		case 5:
		{
			auto pCmd = createCommand(vProcessor, vCommand, vParams);
			return pCmd->execute();
		}
		case 6:
		{
			return execCommand(vProcessor, vCommand, vParams);
		}
		case 7:
		{
			return execCommand(Dictionary({
				{"processor", vProcessor},
				{"command", vCommand},
				{"params", vParams},
				}));
		}
	}

	FAIL("Invalid nWay");
	return nullptr;
}

//
//
//
TEST_CASE("Command.processor_and_command")
{
	SECTION("processor_is_ICommandProcessor")
	{
		for (Size nWay = 0; nWay < c_nExecWayCount; ++nWay)
		{
			CAPTURE(nWay);
			CHECK(testCommandExecution(createObject<ExampleCommandProcessor>(), "passParams", Variant(1), nWay) == 1);
		}
	}

	SECTION("processor_is_string")
	{
		putCatalogData("testExampleCommandProcessor", createObject<ExampleCommandProcessor>());
		for (Size nWay = 0; nWay < c_nExecWayCount; ++nWay)
		{
			CAPTURE(nWay);
			CHECK(testCommandExecution("testExampleCommandProcessor", "passParams", Variant(2), nWay) == 2);
		}
	}

	SECTION("processor_is_invalid")
	{
		for (Size nWay = 0; nWay < c_nExecWayCount; ++nWay)
		{
			CAPTURE(nWay);
			REQUIRE_THROWS_AS([&]() {
				(void)testCommandExecution(100, "passParams", Variant(), nWay);
			}(), error::InvalidArgument);
		}
	}

	SECTION("command_is_string")
	{
		for (Size nWay = 0; nWay < c_nExecWayCount; ++nWay)
		{
			CAPTURE(nWay);
			CHECK(testCommandExecution(createObject<ExampleCommandProcessor>(), "passParams", Variant(1), nWay) == 1);
		}
	}

	SECTION("command_is_not_string")
	{
		for (Size nWay = 0; nWay < c_nExecWayCount; ++nWay)
		{
			CAPTURE(nWay);
			CHECK(testCommandExecution(createObject<ExampleCommandProcessor>(), Dictionary({ {"a",1},  {"b",1} }), Variant(), nWay) == Dictionary({ {"a",1},  {"b",1} }));
		}
	}
}

//
// Testing parameter passing
//
Variant testCommandPassParameter(bool fAddDefault, Variant vDefaultParams, Variant vParams)
{
	Variant cmdDescr = Dictionary({
		{"processor", createObject<ExampleCommandProcessor>()},
		{"command", "passParams"},
		});
	if (fAddDefault)
		cmdDescr.put("params", vDefaultParams);

	auto pCmd = createCommand(cmdDescr);
	return pCmd->execute(vParams);
}

//
//
//
TEST_CASE("Command.params")
{
	SECTION("params_null__default_not_null")
	{
		REQUIRE(testCommandPassParameter(true, Dictionary({ {"a",1},  {"b",1} }), nullptr) == 
			Dictionary({ {"a",1},  {"b",1} }));
	}

	SECTION("params_null__default_null")
	{
		REQUIRE(testCommandPassParameter(true, nullptr, nullptr).isNull());
	}

	SECTION("params_null__default_none")
	{
		REQUIRE(testCommandPassParameter(false, nullptr, nullptr).isNull());
	}

	SECTION("params_not_null__default_not_null")
	{
		REQUIRE(testCommandPassParameter(true, Dictionary({ {"a",1},  {"b",1} }), Dictionary({ {"b",2},  {"c",2} })) ==
			Dictionary({ {"a", 1},  {"b", 2},  {"c", 2} }));
	}

	SECTION("params_not_null__default_null")
	{
		REQUIRE(testCommandPassParameter(true, nullptr, Dictionary({ {"b",2},  {"c",2} })) == Dictionary({ {"b",2},  {"c",2} }));
	}

	SECTION("params_not_null__default_none")
	{
		REQUIRE(testCommandPassParameter(false, nullptr, Dictionary({ {"b",2},  {"c",2} })) == Dictionary({ {"b",2},  {"c",2} }));
	}

	SECTION("multiple_call")
	{
		auto pCmd = createCommand(createObject<ExampleCommandProcessor>(), "passParams", Dictionary({ {"a",1},  {"b",1} }));

		REQUIRE(pCmd->execute() == Dictionary({ {"a",1},  {"b",1} }));
		REQUIRE(pCmd->execute(Dictionary({ {"b",2},  {"c",2} })) == Dictionary({ {"a",1},  {"b",2},  {"c",2} }));
		REQUIRE(pCmd->execute(Dictionary({ {"a",2},  {"d",2} })) == Dictionary({ {"a",2},  {"b",1},  {"d",2} }));
		REQUIRE(pCmd->execute() == Dictionary({ {"a",1},  {"b",1} }));
	}

}

//
//
//
TEST_CASE("Command.returning_error")
{
	for (Size nWay = 0; nWay < c_nExecWayCount; ++nWay)
	{
		CAPTURE(nWay);
		REQUIRE_THROWS_AS([&]() {
			(void)testCommandExecution(createObject<ExampleCommandProcessor>(), "throwException", Variant(), nWay);
		}(), ExampleCommandProcessorError);
	}
}