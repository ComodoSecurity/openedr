//
//  EDRAv2. Unit test
//    Command subsystem test
//
#include "pch.h"

using namespace cmd;


CMD_DECLARE_LIBRARY_CLSID(CLSID_TestCtxCmd, 0xE52D3FA0);
CMD_DECLARE_LIBRARY_CLSID(CLSID_TestCommand, 0xE52D3FA1);
CMD_DECLARE_LIBRARY_CLSID(CLSID_SimpleTestObject, 0xE52D3FA2);

namespace test_contextcommand {

//
//
//
class SimpleTestObject : public ObjectBase <CLSID_SimpleTestObject>
{
public:
};


//
// return getByPath from context. 
// Path is value passed into finalConstruct(in the "path" field.
//
class TestCtxCmd : public ObjectBase<CLSID_TestCtxCmd>,
	public IContextCommand
{
private:
	std::string m_Path;
public:
	void finalConstruct(Variant vConfig)
	{
		m_Path = vConfig["path"];
	}

	virtual Variant execute(Variant vContext, Variant vParam) override
	{
		return getByPath(vContext, m_Path);
	}

	static ObjPtr<TestCtxCmd> create(Variant vPath)
	{
		return createObject<TestCtxCmd>(Dictionary({ {"path", vPath} }));
	}
};

//
// Returns param and ctx as result. 
//
class PassParamCtxCmd : public ObjectBase<>, public IContextCommand
{
public:
	virtual Variant execute(Variant vContext, Variant vParam) override
	{
		return getEthalonRes(vContext, vParam);
	}

	static ObjPtr<PassParamCtxCmd> create()
	{
		return createObject<PassParamCtxCmd>();
	}

	static Variant getEthalonRes(Variant vContext, Variant vParam)
	{
		return Dictionary({ {"ctx", vContext}, {"params", vParam} });
	}
};

//
// Returns param as result. 
//
class PassParamCommand : public ObjectBase<>, public ICommand
{
public:

	virtual Variant execute(Variant vParam) override
	{
		return vParam;
	}

	virtual std::string getDescriptionString() override
	{
		return "";
	}

	static ObjPtr<PassParamCommand> create()
	{
		return createObject<PassParamCommand>();
	}

	static Variant getEthalonRes(Variant vParam)
	{
		return vParam;
	}
};

//
// Return value passed into finalConstruct (in the "retvalue" field)
//
class TestCommand : public ObjectBase<CLSID_TestCommand>, public ICommand
{
private:
	Variant m_vReturnedValue;
	std::string m_sErrorMessage;
public:
	void finalConstruct(Variant vConfig)
	{
		m_vReturnedValue = vConfig["retvalue"];
		m_sErrorMessage = vConfig.get("throw", "");
	}

	virtual Variant execute(Variant vParam) override
	{
		if (!m_sErrorMessage.empty())
			error::RuntimeError(SL, m_sErrorMessage).throwException();
		return m_vReturnedValue;
	}

	virtual std::string getDescriptionString() override
	{
		return "";
	}

	static ObjPtr<TestCommand> create(Variant vRetVal)
	{
		return createObject<TestCommand>(Dictionary({ {"retvalue", vRetVal} }));
	}

	static ObjPtr<TestCommand> createError(std::string_view sMessage = "Error from ICommand")
	{
		return createObject<TestCommand>(Dictionary({ {"throw", sMessage}, {"retvalue", nullptr} }));
	}

};

} // namespace test_contextcommand

using namespace test_contextcommand;

CMD_DEFINE_LIBRARY_CLASS(TestCommand)
CMD_DEFINE_LIBRARY_CLASS(TestCtxCmd)
CMD_DEFINE_LIBRARY_CLASS(SimpleTestObject)

//
//
//
bool isDataProxy(const Variant& v)
{
	return variant::detail::getRawType(v) == variant::detail::RawValueType::DataProxy;
}



//
//
//
TEST_CASE("ContextAwareValue._")
{
	SECTION("None")
	{
		ContextAwareValue cavVal;
		REQUIRE_THROWS_AS([&cavVal]() { (void)cavVal.get(nullptr); }(), error::InvalidUsage);
	}

	SECTION("ICommand")
	{
		auto pCommand = createObject<TestCommand>(Dictionary({ {"retvalue", 1} }));
		ContextAwareValue cavVal(pCommand);
		CHECK(cavVal.get(nullptr) == 1);
	}

	SECTION("IContextCommand")
	{
		auto pCommand = createObject<TestCtxCmd>(Dictionary({ {"path", "b"} }));
		ContextAwareValue cavVal(pCommand);
		Dictionary vContext({ {"b", 2} });
		CHECK(cavVal.get(vContext) == 2);
	}

	SECTION("raw_value")
	{
		Variant valuesArray[] = { Variant(nullptr), Variant(true), Variant(1), Variant("XXXX"), Dictionary({{"a",1}}), Sequence(1), Variant(createObject<SimpleTestObject>()) };

		for (auto& vValue : valuesArray)
		{
			ContextAwareValue cavVal(vValue);
			CHECK(cavVal.get(nullptr) == vValue);
		}
	}

	SECTION("dict_with_Value")
	{
		Variant valuesArray[] = { Variant(nullptr), Variant(true), Variant(1), Variant("XXXX"),
			Dictionary({{"a",1}}), Sequence(1), Variant(createObject<SimpleTestObject>()),
			createObject<TestCommand>(Dictionary({ {"retvalue", 1} })),
			createObject<TestCtxCmd>(Dictionary({ {"path", "b"} }))
		};

		for (auto& vValue : valuesArray)
		{
			ContextAwareValue cavVal(Dictionary({ {"$val", vValue} }));
			CHECK(cavVal.get(nullptr) == vValue);
		}
	}

	SECTION("dict_with_DataProxy")
	{
		Variant vDataProxy = variant::createCmdProxy(
			createObject<TestCommand>(Dictionary({ {"retvalue", 1} })), true);
		REQUIRE(isDataProxy(vDataProxy));
		ContextAwareValue cavVal(Dictionary({ {"$val", vDataProxy} }));
		Variant vResult = cavVal.get(nullptr);
		REQUIRE(isDataProxy(vResult));
		REQUIRE(vResult == 1);
	}

	SECTION("dict_with_path")
	{
		auto fnTest = [](auto vCavCfg)
		{
			ContextAwareValue cavVal(vCavCfg);
			Dictionary vContext({ {"a", 1} });
			return cavVal.get(vContext);
		};

		CHECK(fnTest(Dictionary({ {"$path", "a"} })) == 1);
		REQUIRE_THROWS_AS(fnTest(Dictionary({ {"$path", "b"} })), error::OutOfRange);
		CHECK(fnTest(Dictionary({ {"$path", "a"}, {"$default", 10} })) == 1);
		CHECK(fnTest(Dictionary({ {"$path", "b"}, {"$default", 10} })) == 10);
	}

	SECTION("dict_with_gpath")
	{
		putCatalogData("a", 1);
		putCatalogData("b", nullptr);
		auto fnTest = [](auto vCavCfg)
		{
			ContextAwareValue cavVal(vCavCfg);
			return cavVal.get(nullptr);
		};

		CHECK(fnTest(Dictionary({ {"$gpath", "a"} })) == 1);
		REQUIRE_THROWS_AS(fnTest(Dictionary({ {"$gpath", "b"} })), error::OutOfRange);
		CHECK(fnTest(Dictionary({ {"$gpath", "a"}, {"$default", 10} })) == 1);
		CHECK(fnTest(Dictionary({ {"$gpath", "b"}, {"$default", 10} })) == 10);
	}
}

static Variant callBasicContextCommand(Variant vOperation, Variant vArgs, Variant vContext, Variant vExecParams)
{
	auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_VariantCtxCmd, Dictionary({
		{"operation", vOperation}, {"args", vArgs} })));

	return pCommand->execute(vContext, vExecParams);
}

//
//
//
TEST_CASE("VariantCtxCmd.not")
{
	SECTION("arg_is_scalar")
	{
		Sequence vArgs({ true });
		REQUIRE(callBasicContextCommand("not", vArgs, nullptr, nullptr) == false);
	}

	SECTION("arg_from_context")
	{
		Dictionary vContext({ {"a", false} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}) });
		REQUIRE(callBasicContextCommand("not", vArgs, vContext, nullptr) == true);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", false} });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("not", nullptr, vContext, vParam), error::LogicError);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("not", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
TEST_CASE("VariantCtxCmd.or")
{
	SECTION("arg_from_finalConstruct")
	{
		Dictionary vContext({ {"a", false} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), false, false, false });
		REQUIRE(callBasicContextCommand("or", vArgs, vContext, nullptr) == false);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", true} });
		Sequence vArgs({ false });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("or", vArgs, vContext, vParam), error::LogicError);
	}

	SECTION("many_arg")
	{
		REQUIRE(callBasicContextCommand("or", Sequence({ false, false, false, false }), nullptr, nullptr) == false);
		REQUIRE(callBasicContextCommand("or", Sequence({ false, false, true, false }), nullptr, nullptr) == true);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("or", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
TEST_CASE("VariantCtxCmd.and")
{
	SECTION("arg_from_finalConstruct")
	{
		Dictionary vContext({ {"a", true} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), true });
		REQUIRE(callBasicContextCommand("and", vArgs, vContext, nullptr) == true);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", false} });
		Sequence vArgs({ true });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("and", vArgs, vContext, vParam), error::LogicError);
	}

	SECTION("many_arg")
	{
		REQUIRE(callBasicContextCommand("and", Sequence({ true, true, true, true }), nullptr, nullptr) == true);
		REQUIRE(callBasicContextCommand("and", Sequence({ true, true, false, true }), nullptr, nullptr) == false);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("and", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
TEST_CASE("VariantCtxCmd.equal")
{
	SECTION("arg_from_finalConstruct")
	{
		Dictionary vContext({ {"a", 1} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), 1 });
		REQUIRE(callBasicContextCommand("equal", vArgs, vContext, nullptr) == true);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ 1 });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("equal", vArgs, vContext, vParam), error::LogicError);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("equal", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
TEST_CASE("VariantCtxCmd.greater")
{
	SECTION("arg_from_finalConstruct")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), 1 });
		REQUIRE(callBasicContextCommand("greater", vArgs, vContext, nullptr) == true);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ 1 });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("greater", vArgs, vContext, vParam), error::LogicError);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("greater", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
TEST_CASE("VariantCtxCmd.less")
{
	SECTION("arg_from_finalConstruct")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), 1 });
		REQUIRE(callBasicContextCommand("less", vArgs, vContext, nullptr) == false);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ 1 });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("less", vArgs, vContext, vParam), error::LogicError);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("less", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
TEST_CASE("VariantCtxCmd.add")
{
	SECTION("arg_from_finalConstruct")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), 1 });
		REQUIRE(callBasicContextCommand("add", vArgs, vContext, nullptr) == 3);
	}

	SECTION("arg_from_finalConstruct_3")
	{
		Dictionary vContext({ {"a", 2} });
		Sequence vArgs({ Dictionary({{"$path", "a"}}), 1, 3 });
		REQUIRE(callBasicContextCommand("add", vArgs, vContext, nullptr) == 6);
	}

	SECTION("arg_from_param")
	{
		Dictionary vContext({ {"a", 20} });
		Sequence vArgs({ 10 });
		Sequence vParam({ Dictionary({{"$path", "a"}}) });
		REQUIRE_THROWS_AS(callBasicContextCommand("add", vArgs, vContext, vParam), error::LogicError);
	}

	SECTION("error")
	{
		REQUIRE_THROWS_AS(callBasicContextCommand("add", nullptr, nullptr, nullptr), error::LogicError);
	}
}

//
//
//
Variant callConditionalCtxCmd(std::optional<Variant> vIf, std::optional<Variant> vThen,
	std::optional<Variant> vElse, Variant vContext)
{
	Dictionary vParam;
	if (vIf)
		vParam.put("if", vIf.value());
	if (vThen)
		vParam.put("then", vThen.value());
	if (vElse)
		vParam.put("else", vElse.value());

	auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_ConditionalCtxCmd, vParam));
	return pCommand->execute(vContext, nullptr);
}

//
//
//
TEST_CASE("ConditionalCtxCmd._")
{
	SECTION("if_scalar_true")
	{
		REQUIRE(callConditionalCtxCmd(true, 1, 2, nullptr) == 1);
	}

	SECTION("if_scalar_false")
	{
		REQUIRE(callConditionalCtxCmd(false, 1, 2, nullptr) == 2);
	}

	SECTION("if_IContextCommand")
	{
		auto pCmd = TestCtxCmd::create("a");
		Dictionary vCtx({ {"a", true} });
		REQUIRE(callConditionalCtxCmd(pCmd, 1, 2, vCtx) == 1);
	}

	SECTION("if_absent")
	{
		REQUIRE_THROWS_AS(callConditionalCtxCmd({}, 1, 2, nullptr), error::LogicError);
	}

	SECTION("if_error")
	{
		auto pErrorCmd = TestCommand::createError();
		REQUIRE_THROWS_AS(callConditionalCtxCmd(pErrorCmd, 1, 2, nullptr), error::RuntimeError);
	}

	SECTION("then_scalar")
	{
		REQUIRE(callConditionalCtxCmd(true, 1, 2, nullptr) == 1);
	}

	SECTION("then_cmd")
	{
		auto pCmd = TestCtxCmd::create("a");
		Dictionary vCtx({ {"a", 1} });
		REQUIRE(callConditionalCtxCmd(true, pCmd, 2, vCtx) == 1);
	}

	SECTION("then_absent")
	{
		REQUIRE(callConditionalCtxCmd(true, {}, 2, nullptr) == nullptr);
	}

	SECTION("then_error")
	{
		auto pErrorCmd = TestCommand::createError();
		REQUIRE_THROWS_AS(callConditionalCtxCmd(true, pErrorCmd, 2, nullptr), error::RuntimeError);
		REQUIRE(callConditionalCtxCmd(false, pErrorCmd, 2, nullptr) == 2);
	}

	SECTION("else_scalar")
	{
		REQUIRE(callConditionalCtxCmd(false, 1, 2, nullptr) == 2);
	}

	SECTION("else_cmd")
	{
		auto pCmd = TestCtxCmd::create("a");
		Dictionary vCtx({ {"a", 2} });
		REQUIRE(callConditionalCtxCmd(false, 1, pCmd, vCtx) == 2);
	}

	SECTION("else_absent")
	{
		REQUIRE(callConditionalCtxCmd(false, 1, {}, nullptr) == nullptr);
	}

	SECTION("else_error")
	{
		auto pErrorCmd = TestCommand::createError();
		REQUIRE_THROWS_AS(callConditionalCtxCmd(false, 1, pErrorCmd, nullptr), error::RuntimeError);
		REQUIRE(callConditionalCtxCmd(true, 1, pErrorCmd, nullptr) == 1);
	}
}

//
// 
//
Variant callCallerCtxCmdFull(Variant vCallContext, std::optional<Variant> vCmd, \
	std::optional<Variant> vParam, std::optional<Variant> vCtx,
	std::optional<Variant> vParamVars, std::optional<Variant> vCtxVars,
	std::optional<bool> fCloneParam = {}, std::optional<bool> fCloneCtx = {})
{
	Dictionary vConfig;
	if (vCmd)
		vConfig.put("command", vCmd.value());
	if (vParam)
		vConfig.put("params", vParam.value());
	if (vCtx)
		vConfig.put("ctx", vCtx.value());
	if (vParamVars)
		vConfig.put("ctxParams", vParamVars.value());
	if (vCtxVars)
		vConfig.put("ctxVars", vCtxVars.value());
	if (fCloneParam)
		vConfig.put("cloneParam", fCloneParam.value());
	if (fCloneCtx)
		vConfig.put("cloneCtx", fCloneCtx.value());

	auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_CallCtxCmd, vConfig));
	return pCommand->execute(vCallContext, nullptr);
}


//
// Call CallerCtx without vars
//
Variant callCallerCtxCmd(Variant vCallContext, std::optional<Variant> vCmd, \
	std::optional<Variant> vParam, std::optional<Variant> vCtx, 
	std::optional<bool> fCloneParam = {}, std::optional<bool> fCloneCtx = {})
{
	return callCallerCtxCmdFull(vCallContext, vCmd, vParam, vCtx, {}, {}, fCloneParam, fCloneCtx);
}

//
//
//
TEST_CASE("CallerCtxCmd.param_cmd")
{
	SECTION("ICommand")
	{
		auto pCmd = TestCommand::create(1);
		REQUIRE(callCallerCtxCmd(nullptr, pCmd, {}, {}) == 1);
	}
	
	SECTION("IContextCommand")
	{
		auto pCmd = TestCtxCmd::create("a");
		Dictionary vCallContext({ {"a", 1} });
		REQUIRE(callCallerCtxCmd(vCallContext, pCmd, {}, {}) == 1);
	}

	SECTION("dataProxy_with_IContextCommand")
	{
		auto vCmd = createObjProxy(Dictionary({
			{"clsid", CLSID_TestCtxCmd},
			{"path", "a"},
			}), true);

		Dictionary vCallContext({ {"a", 1} });
		REQUIRE(callCallerCtxCmd(vCallContext, vCmd, {}, {}) == 1);
	}

	SECTION("dataProxy_with_ICommand")
	{
		auto vCmd = createObjProxy(Dictionary({
			{"clsid", CLSID_TestCommand},
			{"retvalue", 1},
			}), true);

		REQUIRE(callCallerCtxCmd(nullptr, vCmd, {}, {}) == 1);
	}

	SECTION("not_object")
	{
		Variant vCmd = 1;
		REQUIRE_THROWS_AS(callCallerCtxCmd(nullptr, vCmd, {}, {}), error::LogicError);
	}

	SECTION("not_supported_object")
	{
		auto vCmd = createObject<SimpleTestObject>();

		REQUIRE_THROWS_AS(callCallerCtxCmd(nullptr, vCmd, {}, {}), error::LogicError);
	}

	SECTION("absent")
	{
		REQUIRE_THROWS_AS(callCallerCtxCmd(nullptr, {}, {}, {}), error::LogicError);
	}
}



struct PathThrowTest 
{
	Variant vCmd;
	Variant vEthalonRes;
};

std::vector<PathThrowTest> getPathThrowTestParam(Variant vContext, Variant vParam)
{
	std::vector<PathThrowTest> res;
	res.push_back(PathThrowTest{ Variant(PassParamCommand::create()), PassParamCommand::getEthalonRes(vParam) });
	res.push_back(PathThrowTest{ Variant(PassParamCtxCmd::create()), PassParamCtxCmd::getEthalonRes(vContext, vParam) });

	return res;
}


//
//
//
TEST_CASE("CallerCtxCmd.param_param")
{
	SECTION("absent")
	{
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		auto CmdInfos = getPathThrowTestParam(vCallContext, nullptr);

		for (auto& info : CmdInfos)
			REQUIRE(callCallerCtxCmd(vCallContext, info.vCmd, {}, {}) == info.vEthalonRes);
	}

	SECTION("scalar")
	{
		Variant vParam = 1;
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		auto CmdInfos = getPathThrowTestParam(vCallContext, vParam);

		for (auto& info : CmdInfos)
			REQUIRE(callCallerCtxCmd(vCallContext, info.vCmd, vParam, {}) == info.vEthalonRes);
	}

	SECTION("IContextCommand")
	{
		Variant vParam = TestCtxCmd::create("a");
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		auto CmdInfos = getPathThrowTestParam(vCallContext, 1);

		for (auto& info : CmdInfos)
			REQUIRE(callCallerCtxCmd(vCallContext, info.vCmd, vParam, {}) == info.vEthalonRes);
	}
}

//
//
//
TEST_CASE("CallerCtxCmd.param_ctx")
{
	SECTION("absent")
	{
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		auto CmdInfos = getPathThrowTestParam(vCallContext, nullptr);

		for (auto& info : CmdInfos)
			REQUIRE(callCallerCtxCmd(vCallContext, info.vCmd, {}, {}) == info.vEthalonRes);
	}

	SECTION("scalar")
	{
		Variant vCtx = 1;
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		auto CmdInfos = getPathThrowTestParam(vCtx, nullptr);

		for (auto& info : CmdInfos)
			REQUIRE(callCallerCtxCmd(vCallContext, info.vCmd, {}, vCtx) == info.vEthalonRes);
	}

	SECTION("IContextCommand")
	{
		Variant vCtx = TestCtxCmd::create("a");
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		auto CmdInfos = getPathThrowTestParam(1, nullptr);

		for (auto& info : CmdInfos)
			REQUIRE(callCallerCtxCmd(vCallContext, info.vCmd, {}, vCtx) == info.vEthalonRes);
	}
}

//
//
//
TEST_CASE("CallerCtxCmd.param_cloneCtx")
{
	SECTION("absent_and_ctx_is_absent")
	{
		Variant vCallContext = Dictionary({ {"a", 1}, {"b", 2} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmd(vCallContext, vCmd, {}, {}, {}, {});
		REQUIRE(vCallContext == vRes["ctx"]);
		vCallContext.put("a", 2);
		REQUIRE(vCallContext == vRes["ctx"]);
	}

	SECTION("absent_and_ctx_is_exist")
	{
		Variant vCallContext = Dictionary({ {"c", 3} });
		Variant vCtx = Dictionary({ {"a", 1}, {"b", 2} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmd(vCallContext, vCmd, {}, vCtx, {}, {});
		REQUIRE(vCtx == vRes["ctx"]);
		vCtx.put("a", 2);
		REQUIRE(vCtx != vRes["ctx"]);
	}

	SECTION("false")
	{
		Variant vCallContext = Dictionary({ {"c", 3} });
		Variant vCtx = Dictionary({ {"a", 1}, {"b", 2} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmd(vCallContext, vCmd, {}, vCtx, {}, false);
		REQUIRE(vCtx == vRes["ctx"]);
		vCtx.put("a", 2);
		REQUIRE(vCtx == vRes["ctx"]);
	}

	SECTION("true")
	{
		Variant vCallContext = Dictionary({ {"c", 3} });
		Variant vCtx = Dictionary({ {"a", 1}, {"b", 2} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmd(vCallContext, vCmd, {}, vCtx, {}, true);
		REQUIRE(vCtx == vRes["ctx"]);
		vCtx.put("a", 2);
		REQUIRE(vCtx != vRes["ctx"]);
	}
}

//
//
//
TEST_CASE("CallerCtxCmd.param_cloneParam")
{
	SECTION("absent")
	{
		Variant vParam = Dictionary({ {"a", 1} });
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmd(nullptr, vCmd, vParam, {}, {}, {});
		REQUIRE(vParam == vRes);
		vRes.put("a", 2);
		REQUIRE(vParam != vRes);
	}

	SECTION("false")
	{
		Variant vParam = Dictionary({ {"a", 1} });
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmd(nullptr, vCmd, vParam, {}, false, {});
		REQUIRE(vParam == vRes);
		vRes.put("a", 2);
		REQUIRE(vParam == vRes);
	}

	SECTION("true")
	{
		Variant vParam = Dictionary({ {"a", 1} });
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmd(nullptr, vCmd, vParam, {}, true, {});
		REQUIRE(vParam == vRes);
		vRes.put("a", 2);
		REQUIRE(vParam != vRes);
	}
}

//
//
//
TEST_CASE("CallerCtxCmd.param_paramVar")
{
	SECTION("absent_param_is_absent")
	{
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmdFull(nullptr, vCmd, {}, {}, {}, {});
		REQUIRE(vRes == nullptr);
	}

	SECTION("exist_param_is_absent")
	{
		Variant vVars = Dictionary({ {"a", 1}, {"b", Dictionary({{"$path", "b"}}) } });
		Variant vCallContext = Dictionary({ {"b", 2} });
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmdFull(vCallContext, vCmd, {}, {}, vVars, {});
		REQUIRE(vRes == Dictionary({ {"a", 1}, {"b", 2 } }));
	}

	SECTION("absent_param_is_exist")
	{
		Variant vParam = Dictionary({ {"a", 10}, {"c", 3} });
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmdFull(nullptr, vCmd, vParam, {}, {}, {});
		REQUIRE(vRes == vParam);
	}

	SECTION("exist_param_is_exist")
	{
		Variant vParam = Dictionary({ {"a", 10}, {"c", 3} });
		Variant vVars = Dictionary({ {"a", 1}, {"b", Dictionary({{"$path", "b"}}) } });
		Variant vCallContext = Dictionary({ {"b", 2} });
		Variant vCmd = PassParamCommand::create();
		Variant vRes = callCallerCtxCmdFull(vCallContext, vCmd, vParam, {}, vVars, {});
		REQUIRE(vRes == Dictionary({ {"a", 1}, {"b", 2 }, {"c", 3} }));
	}

	SECTION("invalid")
	{
		Variant vVars = 1;
		Variant vCmd = PassParamCommand::create();
		REQUIRE_THROWS_AS(callCallerCtxCmdFull(nullptr, vCmd, {}, {}, vVars, {}), error::LogicError);
	}

	SECTION("error_in_calculation")
	{
		Variant vVars = Dictionary({ {"a", TestCommand::createError()} });
		Variant vCmd = PassParamCommand::create();
		REQUIRE_THROWS_AS(callCallerCtxCmdFull(nullptr, vCmd, {}, {}, vVars, {}), error::RuntimeError);
	}
}

//
//
//
TEST_CASE("CallerCtxCmd.param_paramCtx")
{
	SECTION("absent_ctx_is_absent")
	{
		Variant vCmd = PassParamCtxCmd::create();
		Variant vCallContext = Dictionary({ {"b", 2} });
		Variant vRes = callCallerCtxCmdFull(vCallContext, vCmd, {}, {}, {}, {});
		REQUIRE(vRes["ctx"] == vCallContext);
	}

	SECTION("exist_ctx_is_absent")
	{
		Variant vVars = Dictionary({ {"a", 1}, {"b", Dictionary({{"$path", "b"}}) } });
		Variant vCallContext = Dictionary({ {"b", 2} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmdFull(vCallContext, vCmd, {}, {}, {}, vVars);
		REQUIRE(vRes["ctx"] == Dictionary({ {"a", 1}, {"b", 2 } }));
	}

	SECTION("absent_ctx_is_exist")
	{
		Variant vCtx = Dictionary({ {"a", 10}, {"c", 3} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmdFull(nullptr, vCmd, {}, vCtx, {}, {});
		REQUIRE(vRes["ctx"] == vCtx);
	}

	SECTION("exist_ctx_is_exist")
	{
		Variant vCtx = Dictionary({ {"a", 10}, {"c", 3} });
		Variant vVars = Dictionary({ {"a", 1}, {"b", Dictionary({{"$path", "b"}}) } });
		Variant vCallContext = Dictionary({ {"b", 2} });
		Variant vCmd = PassParamCtxCmd::create();
		Variant vRes = callCallerCtxCmdFull(vCallContext, vCmd, {}, vCtx, {}, vVars);
		REQUIRE(vRes["ctx"] == Dictionary({ {"a", 1}, {"b", 2 }, {"c", 3} }));
	}

	SECTION("invalid")
	{
		Variant vVars = 1;
		Variant vCmd = PassParamCtxCmd::create();
		REQUIRE_THROWS_AS(callCallerCtxCmdFull(nullptr, vCmd, {}, {}, {}, vVars), error::LogicError);
	}

	SECTION("error_in_calculation")
	{
		Variant vVars = Dictionary({ {"a", TestCommand::createError()} });
		Variant vCmd = PassParamCtxCmd::create();
		REQUIRE_THROWS_AS(callCallerCtxCmdFull(nullptr, vCmd, {}, {}, {}, vVars), error::RuntimeError);
	}
}

//
//
//
TEST_CASE("ForEachCtxCmd")
{
	auto fnScenario = [](Variant vCommand, Variant vData, std::string_view sPath, Variant vContext,
		Variant vExpected)
	{
		Variant vConfig = Dictionary({ {"command", vCommand}, {"data", vData} });
		if (!sPath.empty() && !vContext.isNull())
		{
			// Patch expected
			vConfig.put("path", sPath);
			auto vValue = (vData.isSequenceLike() ? vData.get(vData.getSize() - 1) : vData);
			for (auto vItem : vExpected)
			{
				if (!vItem.isDictionaryLike())
					continue;
				vItem.put("params", {});
				vItem.get("ctx").put(sPath, vValue);
			}
		}
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_ForEachCtxCmd, vConfig));
		auto vResult = pCommand->execute(vContext, nullptr);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant, Variant>> mapData{
		// sName: vCommand, vData, vContext, vExpected
		{ "ICommand_sequence", {
			PassParamCommand::create(),
			Sequence({ "x", "y", "z" }),
			{},
			Sequence({
				PassParamCommand::getEthalonRes("x"),
				PassParamCommand::getEthalonRes("y"),
				PassParamCommand::getEthalonRes("z")
				})
			}},
		{ "ICommand_scalar", {
			PassParamCommand::create(),
			"xyz",
			{},
			Sequence({ PassParamCommand::getEthalonRes("xyz") })
			}},
		{ "ICommand_dictionary", {
			PassParamCommand::create(),
			Dictionary({ {"xyz", 42} }),
			{},
			Sequence({ PassParamCommand::getEthalonRes(Dictionary({ {"xyz", 42} })) })
			}},
		{ "IContextCommand_sequence", {
			PassParamCtxCmd::create(),
			Sequence({ "x", "y", "z" }),
			Dictionary({ {"a", 42} }),
			Sequence({
				PassParamCtxCmd::getEthalonRes(Dictionary({ {"a", 42} }), "x"),
				PassParamCtxCmd::getEthalonRes(Dictionary({ {"a", 42} }), "y"),
				PassParamCtxCmd::getEthalonRes(Dictionary({ {"a", 42} }), "z")
				})
			}},
		{ "IContextCommand_scalar", {
			PassParamCtxCmd::create(),
			"xyz",
			Dictionary({ {"a", 42} }),
			Sequence({ PassParamCtxCmd::getEthalonRes(Dictionary({ {"a", 42} }), "xyz") })
			}},
		{ "IContextCommand_dictionary", {
			PassParamCtxCmd::create(),
			Dictionary({ {"xyz", 42} }),
			Dictionary({ {"a", 42} }),
			Sequence({ PassParamCtxCmd::getEthalonRes(
				Dictionary({ {"a", 42} }),
				Dictionary({ {"xyz", 42} }))
				}),
			}},
		{ "dyn_ICommand_sequence", {
			createObjProxy(Dictionary({	{"clsid", CLSID_TestCommand}, {"retvalue", 42} }), true),
			Sequence({ "x", "y", "z" }),
			{},
			Sequence({ 42, 42, 42 })
			}},
		{ "dyn_ICommand_scalar", {
			createObjProxy(Dictionary({	{"clsid", CLSID_TestCommand}, {"retvalue", 42} }), true),
			"xyz",
			{},
			Sequence({ 42 })
			}},
		{ "dyn_ICommand_dictionary", {
			createObjProxy(Dictionary({	{"clsid", CLSID_TestCommand}, {"retvalue", 42} }), true),
			Dictionary({ {"xyz", 42} }),
			{},
			Sequence({ 42 })
			}},
		{ "dyn_IContextCommand_sequence", {
			createObjProxy(Dictionary({ {"clsid", CLSID_TestCtxCmd}, {"path", "a"}	}), true),
			Sequence({ "x", "y", "z" }),
			Dictionary({ {"a", 42} }),
			Sequence({ 42, 42, 42 })
			}},
		{ "dyn_IContextCommand_scalar", {
			createObjProxy(Dictionary({ {"clsid", CLSID_TestCtxCmd}, {"path", "a"}	}), true),
			"xyz",
			Dictionary({ {"a", 42} }),
			Sequence({ 42 })
			}},
		{ "dyn_IContextCommand_dictionary", {
			createObjProxy(Dictionary({ {"clsid", CLSID_TestCtxCmd}, {"path", "a"}	}), true),
			Dictionary({ {"xyz", 42} }),
			Dictionary({ {"a", 42} }),
			Sequence({ 42 }),
			}},
	};

	// Do all cases without "path" parameter
	for (const auto& [sName, params] : mapData)
	{
		const auto& [vCommand, vData, vContext, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vCommand, vData, "", vContext, vExpected));
		}
	}

	// Repeat all cases with "path" parameter
	for (const auto& [sName, params] : mapData)
	{
		const auto& [vCommand, vData, vContext, vExpected] = params;
		std::string sSectionName(sName);
		sSectionName += "_path";
		DYNAMIC_SECTION(sSectionName)
		{
			REQUIRE_NOTHROW(fnScenario(vCommand, vData, "xxx", vContext, vExpected));
		}
	}

	const std::map<std::string, std::tuple<std::string, Variant, Variant>> mapBadData{
		// sName: sMsg, vCommand, vData
		{ "not_object", { "Invalid <command> parameter", 42, { 42 } }},
	};

	for (const auto& [sName, params] : mapBadData)
	{
		const auto& [sMsg, vCommand, vData] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vCommand, vData, "", {}, {}), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("MakeDictionaryCtxCmd.good")
{
	auto fnScenario = [](Variant vData, Variant vContext, Variant vExpected)
	{
		Variant vConfig = Dictionary({ {"data", vData} });
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_MakeDictionaryCtxCmd, vConfig));
		auto vResult = pCommand->execute(vContext);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vData, vContext, vExpected
		{ "dictionary", {
			Dictionary({ {"dstA", 42}, {"dstB", Dictionary({ {"$path", "ctxB"} })} }),
			Dictionary({ {"ctxB", "xyz"} }),
			Dictionary({ {"dstA", 42}, {"dstB", "xyz"} }),
			}},
		{ "sequence", {
			Sequence({
				Dictionary({ {"name", "dstA"}, {"value", 42} }),
				Dictionary({ {"name", "dstB"}, {"value", Dictionary({ {"$path", "ctxB"} })} })
				}),
			Dictionary({ {"ctxB", "xyz"} }),
			Dictionary({ {"dstA", 42}, {"dstB", "xyz"} }),
			}},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, vContext, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, vContext, vExpected));
		}
	}
}

//
//
//
TEST_CASE("MakeDictionaryCtxCmd.bad")
{
	auto fnScenario = [](Variant vConfig)
	{
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_MakeDictionaryCtxCmd, vConfig));
		(void)pCommand->execute(Dictionary());
	};

	const std::map<std::string, std::tuple<std::string, Variant>> mapData{
		// sName: sMsg, vConfig
		{ "no_data", { "Missing field <data>", Dictionary() }},
		{ "invalid_data", { "Data must be dictionary or sequence", Dictionary({ {"data", 42} }) }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sMsg, vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("MakeSequenceCtxCmd.good")
{
	auto fnScenario = [](Variant vData, Variant vContext, Variant vExpected)
	{
		Variant vConfig = Dictionary({ {"data", vData} });
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_MakeSequenceCtxCmd, vConfig));
		auto vResult = pCommand->execute(vContext);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vData, vContext, vExpected
		{ "sequence", {
			Sequence({ 42, Dictionary({ {"$path", "ctxB"} }), "xxx" }),
			Dictionary({ {"ctxB", "xyz"} }),
			Sequence({ 42, "xyz", "xxx" }),
			}},
		{ "context_aware_value", {
			Dictionary({ {"$path", "ctxB"} }),
			Dictionary({ {"ctxB", "xyz"} }),
			Sequence({ "xyz" }),
			}},
		{ "other", { 42, Dictionary(), Sequence({ 42 }) }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vData, vContext, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vData, vContext, vExpected));
		}
	}
}

//
//
//
TEST_CASE("MakeSequenceCtxCmd.bad")
{
	auto fnScenario = [](Variant vConfig)
	{
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_MakeSequenceCtxCmd, vConfig));
		(void)pCommand->execute(Dictionary());
	};

	const std::map<std::string, std::tuple<std::string, Variant>> mapData{
		// sName: sMsg, vConfig
		{ "no_data", { "Missing field <data>", Dictionary() }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sMsg, vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig), Catch::Matchers::Contains(sMsg));
		}
	}
}

//
//
//
TEST_CASE("CachedValueCtxCmd.good")
{
	auto fnScenario = [](Variant vValue, Variant vContext, Variant vExpected)
	{
		std::string sPath("path.in.context");
		Variant vConfig = Dictionary({ {"path", sPath}, {"value", vValue} });
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_CachedValueCtxCmd, vConfig));
		auto vResult = pCommand->execute(vContext);
		REQUIRE(vResult == vExpected);
		vResult = pCommand->execute(vContext);
		REQUIRE(vResult == vExpected);
	};

	const std::map<std::string, std::tuple<Variant, Variant, Variant>> mapData{
		// sName: vValue, vContext, vExpected
		{ "context_aware_value", {
			Dictionary({ {"$path", "ctxB"} }),
			Dictionary({ {"ctxB", "xyz"} }),
			"xyz",
			}},
		{ "other", { 42, Dictionary(), 42 }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [vValue, vContext, vExpected] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_NOTHROW(fnScenario(vValue, vContext, vExpected));
		}
	}
}

//
//
//
TEST_CASE("CachedValueCtxCmd.bad")
{
	auto fnScenario = [](Variant vConfig)
	{
		auto pCommand = queryInterface<IContextCommand>(createObject(CLSID_CachedValueCtxCmd, vConfig));
		(void)pCommand->execute(Dictionary());
	};

	const std::map<std::string, std::tuple<std::string, Variant>> mapData{
		// sName: sMsg, vConfig
		{ "no_path", { "Missing field <path>", Dictionary() }},
		{ "no_value", { "Missing field <value>", Dictionary({ {"path", ""} }) }},
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [sMsg, vConfig] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_WITH(fnScenario(vConfig), Catch::Matchers::Contains(sMsg));
		}
	}
}
