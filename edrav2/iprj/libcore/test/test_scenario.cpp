//
//  EDRAv2. Unit test
//    Command subsystem test
//
#include "pch.h"

using namespace openEdr;

CMD_DECLARE_LIBRARY_CLSID(CLSID_TestExtractContextCommand, 0xE52D3FA0);
CMD_DECLARE_LIBRARY_CLSID(CLSID_TestReturnCommand, 0xE52D3FA1);
CMD_DECLARE_LIBRARY_CLSID(CLSID_SimpleTestObject, 0xE52D3FA2);
CMD_DECLARE_LIBRARY_CLSID(CLSID_FinalConstructCounter, 0xE52D3FA3);
CMD_DECLARE_LIBRARY_CLSID(CLSID_TestErrorCommand, 0xE52D3FA4);

namespace test_scenario {

//
//
//
class SimpleTestObject : public ObjectBase <CLSID_SimpleTestObject>
{
public:
};

//
//
//
class FinalConstructCounter : public ObjectBase<CLSID_FinalConstructCounter>
{
public:
	void finalConstruct(Variant vCfg)
	{
		++g_nActionCount;
	}

	static size_t g_nActionCount;

	static Variant createDataProxy(bool fCached)
	{
		return createObjProxy(Dictionary({ {"clsid", CLSID_FinalConstructCounter} }), fCached);
	}
};
size_t FinalConstructCounter::g_nActionCount = 0;


//
// return getByPath from context. 
// Path is value passed into finalConstruct(in the "path" field.
//
class TestExtractContextCommand : public ObjectBase<CLSID_TestExtractContextCommand>,
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
};

//
// Return value passed into finalConstruct (in the "retvalue" field)
//
class TestReturnCommand : public ObjectBase<CLSID_TestReturnCommand>,
	public ICommand
{
private:
	Variant m_vReturnedValue;
public:
	void finalConstruct(Variant vConfig)
	{
		m_vReturnedValue = vConfig["retvalue"];
	}

	virtual Variant execute(Variant vParam) override
	{
		return m_vReturnedValue;
	}

	virtual std::string getDescriptionString() override
	{
		return "";
	}

};


//
// Return value passed into finalConstruct (in the "retvalue" field)
//
class TestErrorCommand : public ObjectBase<CLSID_TestErrorCommand>, 
	public ICommand
{
private:
	std::function<void()> m_pFn;

public:
	void finalConstruct(Variant vConfig)
	{
		std::string sErrorMessage = vConfig.get("msg", "No error message");
		m_pFn = [sErrorMessage]() { 
			error::InvalidFormat(SL, sErrorMessage).throwException();
		};
	}

	virtual Variant execute(Variant vParam) override
	{
		m_pFn();
		return {};
	}

	virtual std::string getDescriptionString() override { return ""; }

	static ObjPtr<TestErrorCommand> create(std::function<void()> fn)
	{
		auto pThis = createObject<TestErrorCommand>();
		pThis->m_pFn = fn;
		return pThis;
	}
};

} // namespace test_scenario

using namespace test_scenario;

CMD_DEFINE_LIBRARY_CLASS(TestReturnCommand)
CMD_DEFINE_LIBRARY_CLASS(TestExtractContextCommand)
CMD_DEFINE_LIBRARY_CLASS(SimpleTestObject)
CMD_DEFINE_LIBRARY_CLASS(FinalConstructCounter)
CMD_DEFINE_LIBRARY_CLASS(TestErrorCommand)


Variant execScenario(Variant vCode, Variant vContext = nullptr)
{
	auto pScenario = queryInterface<IContextCommand>(createObject(CLSID_Scenario, Dictionary({ {"code", vCode} })));
	return pScenario->execute(vContext);
}

//
//
//
TEST_CASE("Scenario.set_action")
{
	SECTION("src_is_scalar")
	{
		Variant vCode = Sequence ({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
		});

		Dictionary vContext;
		(void)execScenario(vCode, vContext);
		REQUIRE(vContext["a"] == 1);
	}

	SECTION("src_is_ContextCommand")
	{
		auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "a"} }));

		Variant vCode = Sequence({
			Dictionary({{"$set", pCommand}, {"$dst", "b"}})
		});


		Dictionary vContext({ {"a", 1} });
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 1);
	}


	SECTION("dst_is_absent")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}}),
			});

		Dictionary vContext;

		REQUIRE_THROWS([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
	}

	SECTION("dst_is_invalid")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", 1}}),
			});

		Dictionary vContext;

		REQUIRE_THROWS([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
	}

	SECTION("exception_process")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", 1}, {"$catch", nullptr}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
	}

	SECTION("disabled")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$if", false}, {"$dst", "a"}}),
			});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE_FALSE(vContext.has("a"));
	}
}


//
//
//
TEST_CASE("Scenario.ret_action")
{
	SECTION("with_scalar_value")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", 1}}),
			});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("with_command_value")
	{
		auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "a"} }));
		Variant vCode = Sequence({
			Dictionary({{"$ret", pCommand}}),
			});

		Dictionary vContext({ {"a", 1} });
		REQUIRE(execScenario(vCode, vContext) == 1);
	}

	SECTION("with_null")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", nullptr}}),
		});

		REQUIRE(execScenario(vCode) == nullptr);
	}

	SECTION("exception_process")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", createObject<TestErrorCommand>()}, {"$catch", nullptr}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
	}

	SECTION("disabled")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", 1}, {"$if", false}}),
			});

		REQUIRE(execScenario(vCode) == nullptr);
	}
}


//
//
//
TEST_CASE("Scenario.goto_action")
{
	SECTION("branchname_is_string")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", "second"}}),
				Dictionary({{"$ret", 2}}),
			})},
			{"second", Sequence({
				Dictionary({{"$ret", 1}}),
			})},
		});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("branchname_is_int")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", 100}}),
				Dictionary({{"$ret", 2}}),
			})},
			{"100", Sequence({
				Dictionary({{"$ret", 1}}),
			})},
			});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("branchname_is_command")
	{
		auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "gotoBranch"} }));
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", pCommand}}),
				Dictionary({{"$ret", 2}}),
			})},
			{"second", Sequence({
				Dictionary({{"$ret", 1}}),
			})},
		});

		Dictionary vContext({ {"gotoBranch", "second"} });
		REQUIRE(execScenario(vCode, vContext) == 1);
	}

	SECTION("invalid_value")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", 1}}),
			})},
		});

		REQUIRE_THROWS([vCode]() { (void)execScenario(vCode); }());
	}

	SECTION("absent_branch")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", "second"}}),
			})},
		});

		REQUIRE_THROWS([vCode]() { (void)execScenario(vCode); }());
	}

	SECTION("absent_branch_with_default")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", "third"}, {"$default", "second"} }),
			})},
			{"second", Sequence({
				Dictionary({{"$ret", 1}}),
			})},
			});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("absent_branch_with_absent_default")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", "third"}, {"$default", "second"} }),
			})},
			});

		REQUIRE_THROWS([vCode]() { (void)execScenario(vCode); }());
	}

	SECTION("exception_process")
	{
		Variant vCode = Sequence({
			Dictionary({{"$goto", createObject<TestErrorCommand>()}, {"$catch", nullptr}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
	}

	SECTION("disabled")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", "second"}, {"$if", false}}),
				Dictionary({{"$ret", 2}}),
			})},
			{"second", Sequence({
				Dictionary({{"$ret", 1}}),
			})},
		});

		REQUIRE(execScenario(vCode) == 2);
	}
}

//
//
//
TEST_CASE("Scenario.exec_action")
{
	SECTION("exec_contain_cmd")
	{
		auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "a"} }));
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({{"$exec", pCommand}, {"$dst", "b"}}),
			Dictionary({{"$ret", Dictionary({{"$path", "b"}}) }}),
		});


		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("exec_contain_cmd_without_dst")
	{
		auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "a"} }));
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({{"$exec", pCommand}}),
			Dictionary({{"$ret", Dictionary({{"$path", "b"}}) }}),
		});

		REQUIRE(execScenario(vCode) == 2);
	}

	SECTION("exec_absent")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({
				{"$dst", "b"},
				{"clsid", CLSID_TestExtractContextCommand},
				{"path", "a"},
			}),
			Dictionary({{"$ret", Dictionary({{"$path", "b"}}) }}),
		});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("exec_is_wrong")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({
				{"$exec", 1},
				{"$dst", "b"},
				{"clsid", CLSID_TestExtractContextCommand},
				{"path", "a"},
			}),
			Dictionary({{"$ret", Dictionary({{"$path", "b"}}) }}),
			});

		REQUIRE_THROWS([vCode]() { (void)execScenario(vCode); }());
	}

	SECTION("exec_absent_invalid_object")
	{
		Variant vCode = Sequence({
			Dictionary({
				{"clsid", CLSID_SimpleTestObject},
			}),
		});

		REQUIRE_THROWS([vCode]() { (void)execScenario(vCode); }());
	}


	SECTION("exception_process")
	{
		Variant vCode = Sequence({
			Dictionary({ {"clsid", CLSID_TestErrorCommand}, {"$catch", nullptr} }),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
	}

	SECTION("disabled")
	{
		auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "a"} }));
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({{"$exec", pCommand}, {"$dst", "b"}, {"$if", false} }),
			Dictionary({{"$ret", Dictionary({{"$path", "b"}}) }}),
		});

		REQUIRE(execScenario(vCode) == 2);
	}
}

//
//
//
enum class CatchTestResult
{
	ExceptionIsNotCatched,
	CompileError,
	GotoNextLine,
	GotoBranch,
	GotoUnknownBranch,
};

//
//
//
ObjPtr<IContextCommand> createAndCompileScenario(Variant vScenarioCode, bool fExpectException)
{
	// Create and compile
	ObjPtr<IContextCommand> pScenario;
	if (fExpectException)
	{
		REQUIRE_THROWS([&vScenarioCode, &pScenario]() {
			pScenario = queryInterface<IContextCommand>(createObject(CLSID_Scenario, Dictionary({
				{"code", vScenarioCode},
				{"compileOnCall", true},
				})));
			}());
		REQUIRE(pScenario == nullptr);
	}
	else
	{
		REQUIRE_NOTHROW([&vScenarioCode, &pScenario]() {
			pScenario = queryInterface<IContextCommand>(createObject(CLSID_Scenario, Dictionary({
				{"code", vScenarioCode},
				{"compileOnCall", true},
				})));
		}());
		REQUIRE(pScenario != nullptr);
	}
	return pScenario;
}

//
//
//
void testCatchCodeField(std::optional<Variant> vCode, CatchTestResult eExpectedResult)
{
	Variant vScenarioCode = Sequence({
		Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch",
				(vCode.has_value() ? Dictionary({{"code", vCode.value()}}) : Dictionary())
			}}),
		Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

	auto pScenario = createAndCompileScenario(vScenarioCode, eExpectedResult == CatchTestResult::CompileError);
	if (pScenario == nullptr) 
		return;

	if (eExpectedResult == CatchTestResult::GotoNextLine)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext);}());
		REQUIRE(vContext["b"] == 2);
	}
	else if (eExpectedResult == CatchTestResult::ExceptionIsNotCatched)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_THROWS([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["b"] == 1);
	}
	else
	{
		FAIL("Invalid eExpectedResult");
	}
}

//
//
//
void testCatchLogField(std::optional<Variant> vLog, CatchTestResult eExpectedResult)
{
	Variant vScenarioCode = Sequence({
		Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch",
				(vLog.has_value() ? Dictionary({{"log", vLog.value()}}) : Dictionary())
			}}),
		Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

	auto pScenario = createAndCompileScenario(vScenarioCode, eExpectedResult == CatchTestResult::CompileError);
	if (pScenario == nullptr)
		return;

	if (eExpectedResult == CatchTestResult::GotoNextLine)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext);}());
		REQUIRE(vContext["b"] == 2);
	}
	else
	{
		FAIL("Invalid eExpectedResult");
	}
}

//
//
//
void testCatchGotoField(std::optional<Variant> vGoto, CatchTestResult eExpectedResult)
{
	Variant vScenarioCode = Dictionary({
		{"main", Sequence({
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch",
					(vGoto.has_value() ? Dictionary({{"goto", vGoto.value()}}) : Dictionary())
			}}),
			Dictionary({{"$set", 3}, {"$dst", "b"}}),
		})},
		{"branch", Sequence({
			Dictionary({{"$set", 4}, {"$dst", "b"}}),
		})},
	});

	auto pScenario = createAndCompileScenario(vScenarioCode, eExpectedResult == CatchTestResult::CompileError);
	if (pScenario == nullptr)
		return;

	if (eExpectedResult == CatchTestResult::ExceptionIsNotCatched)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_THROWS([&vContext, &pScenario]() { pScenario->execute(vContext);}());
		REQUIRE(vContext["b"] == 2);
	}
	else if (eExpectedResult == CatchTestResult::GotoNextLine)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["b"] == 3);
	}
	else if (eExpectedResult == CatchTestResult::GotoBranch)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["b"] == 4);
	}
	else if (eExpectedResult == CatchTestResult::GotoUnknownBranch)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["b"] == 2);
	}
	else
	{
		FAIL("Invalid eExpectedResult");
	}
}


//
//
//
void testCatchDstField(std::optional<Variant> vDst, CatchTestResult eExpectedResult, bool fSerializeException)
{
	Variant vScenarioCode = Sequence({
		Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch",
				(vDst.has_value() ? Dictionary({{"dst", vDst.value()}}) : Dictionary())
			}}),
		Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

	auto pScenario = createAndCompileScenario(vScenarioCode, eExpectedResult == CatchTestResult::CompileError);
	if (pScenario == nullptr)
		return;

	if (eExpectedResult == CatchTestResult::ExceptionIsNotCatched)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_THROWS([&vContext, &pScenario]() { pScenario->execute(vContext);}());
		REQUIRE(vContext["b"] == 1);
	}
	else if (eExpectedResult == CatchTestResult::GotoNextLine)
	{
		Dictionary vContext({ {"b", 1} });
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["b"] == 2);
		
		if (!fSerializeException)
		{
			REQUIRE(vContext.getSize() == 1);
		}
		else
		{
			REQUIRE(vContext.getSize() == 2);
			std::string sDst = vDst.value();

			Variant vExceptionCode;
			REQUIRE_NOTHROW([&vContext, &vExceptionCode, &sDst]() {
				vExceptionCode = variant::getByPath(vContext, sDst).get("errCode");
				}());
			REQUIRE(vExceptionCode == error::ErrorCode::InvalidFormat);
		}
	}
	else
	{
		FAIL("Invalid eExpectedResult");
	}
}

//
//
//
ObjPtr<IContextCommand> createScenarioWithSequenceCatch(error::ErrorCode eThrownException, bool fAddDefaultRule)
{
	Variant vCatch = Sequence({
		Dictionary({
			{"code", error::ErrorCode::OutOfRange},
			{"goto", "OutOfRange"},
			{"dst", "OutOfRange"}
		}),
		Dictionary({
			{"code", error::ErrorCode::InvalidArgument},
			{"goto", "InvalidArgument"},
			{"dst", "InvalidArgument"}
		}),
	});

	if (fAddDefaultRule)
	{
		vCatch.push_back(Dictionary({
			{"goto", "All"},
			{"dst", "All"}
		}));
	}

	Variant vExceptionCmd = TestErrorCommand::create([eThrownException]() {
		if (eThrownException == error::ErrorCode::OutOfRange)
			error::OutOfRange(SL, "msg").throwException();
		if (eThrownException == error::ErrorCode::InvalidArgument)
			error::InvalidArgument(SL, "msg").throwException();
		if (eThrownException == error::ErrorCode::NotFound)
			error::NotFound(SL, "msg").throwException();
	});


	Variant vScenarioCode = Dictionary({
		{"main", Sequence({
			Dictionary({{"$set", 1}, {"$dst", "d"}}),
			Dictionary({{"$set", vExceptionCmd}, {"$dst", "a"}, {"$catch", vCatch}}),
			Dictionary({{"$set", 2}, {"$dst", "d"}}),
		})},
		{"OutOfRange", Sequence({
			Dictionary({{"$set", 3}, {"$dst", "d"}}),
		})},
		{"InvalidArgument", Sequence({
			Dictionary({{"$set", 4}, {"$dst", "d"}}),
		})},
		{"All", Sequence({
			Dictionary({{"$set", 5}, {"$dst", "d"}}),
		})},
	});

	auto pScenario = createAndCompileScenario(vScenarioCode, false);
	REQUIRE(pScenario != nullptr);
	return pScenario;
}



//
//
//
TEST_CASE("Scenario.catch_field")
{
	SECTION("no_exception")
	{
		Variant vCode = Sequence ({
			Dictionary({{"$set", 1}, {"$dst", "a"}, {"$catch", nullptr}}),
		});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["a"] == 1);
	}

	SECTION("no_catch")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}}),
		});

		Dictionary vContext;
		REQUIRE_THROWS([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
	}

	SECTION("catch_is_null")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch", nullptr}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
	}

	SECTION("catch_is_valid_string")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$set", 2}, {"$dst", "b"}}),
				Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch", "onexception"}}),
				Dictionary({{"$set", 3}, {"$dst", "b"}}),
			})},
			{"onexception", Sequence({
				Dictionary({{"$set", 4}, {"$dst", "d"}}),
			})},
		});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
		REQUIRE(vContext["d"] == 4);
	}

	SECTION("catch_is_invalid_string")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$set", 2}, {"$dst", "b"}}),
				Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch", "invalid_branch"}}),
				Dictionary({{"$set", 3}, {"$dst", "b"}}),
			})},
			{"onexception", Sequence({
				Dictionary({{"$set", 4}, {"$dst", "d"}}),
			})},
		});

		Dictionary vContext;
		REQUIRE_THROWS([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
	}

	SECTION("catch_has_invalid_type")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$set", 2}, {"$dst", "b"}}),
				Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch", true}}),
				Dictionary({{"$set", 3}, {"$dst", "b"}}),
			})},
			{"onexception", Sequence({
				Dictionary({{"$set", 4}, {"$dst", "d"}}),
			})},
			});

		Dictionary vContext;
		REQUIRE_THROWS([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
	}

	SECTION("exception_in_if")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$if", createObject<TestErrorCommand>()}, {"$set", 2}, {"$dst", "a"}, 
				{"$catch", nullptr}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
		});

		Dictionary vContext;
		REQUIRE_NOTHROW([vCode, vContext]() { (void)execScenario(vCode, vContext); }());
		REQUIRE(vContext["b"] == 2);
		REQUIRE(vContext["a"] == 1);
	}

	SECTION("code_is_absent")
	{
		testCatchCodeField(std::nullopt, CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_zero_int")
	{
		testCatchCodeField(Variant(0), CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_empty_string")
	{
		testCatchCodeField(Variant(""), CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_valid_int_catched")
	{
		testCatchCodeField(Variant(error::ErrorCode::InvalidFormat), CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_valid_int_catched_based")
	{
		testCatchCodeField(Variant(error::ErrorCode::RuntimeError), CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_valid_int_uncatched")
	{
		testCatchCodeField(Variant(error::ErrorCode::AccessDenied), CatchTestResult::ExceptionIsNotCatched);
	}
	SECTION("code_is_valid_string_catched")
	{
		testCatchCodeField(Variant("0xE0020005"), CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_valid_string_catched_based")
	{
		testCatchCodeField(Variant("0xE0020000"), CatchTestResult::GotoNextLine);
	}
	SECTION("code_is_valid_string_uncatched")
	{
		testCatchCodeField(Variant("0xE0020001"), CatchTestResult::ExceptionIsNotCatched);
	}
	SECTION("code_is_invalid_string")
	{
		testCatchCodeField(Variant("QWERTY"), CatchTestResult::CompileError);
	}
	SECTION("code_is_invalid_type")
	{
		testCatchCodeField(Variant(false), CatchTestResult::CompileError);
	}

	SECTION("log_is_absent")
	{
		testCatchLogField(std::nullopt, CatchTestResult::GotoNextLine);
	}
	SECTION("log_is_true")
	{
		testCatchLogField(Variant(true), CatchTestResult::GotoNextLine);
	}
	SECTION("log_is_false")
	{
		testCatchLogField(Variant(false), CatchTestResult::GotoNextLine);
	}
	SECTION("log_is_invalid")
	{
		testCatchLogField(Variant(""), CatchTestResult::CompileError);
	}

	SECTION("goto_is_absent")
	{
		testCatchGotoField(std::nullopt, CatchTestResult::GotoNextLine);
	}
	SECTION("goto_is_empty_string")
	{
		testCatchGotoField(Variant(""), CatchTestResult::GotoNextLine);
	}
	SECTION("goto_is_valid_string")
	{
		testCatchGotoField(Variant("branch"), CatchTestResult::GotoBranch);
	}
	SECTION("goto_is_invalid_string")
	{
		testCatchGotoField(Variant("xxxx"), CatchTestResult::ExceptionIsNotCatched);
	}
	SECTION("goto_is_invalid_type")
	{
		testCatchGotoField(Variant(false), CatchTestResult::CompileError);
	}

	SECTION("dst_is_invalid_type")
	{
		testCatchDstField(Variant(false), CatchTestResult::CompileError, false);
	}
	SECTION("dst_is_absent")
	{
		testCatchDstField(std::nullopt, CatchTestResult::GotoNextLine, false);
	}
	SECTION("dst_is_valid_simple_path")
	{
		testCatchDstField(Variant("z"), CatchTestResult::GotoNextLine, true);
	}
	SECTION("dst_is_valid_complex_path")
	{
		testCatchDstField(Variant("z.x.y"), CatchTestResult::GotoNextLine, true);
	}
	SECTION("dst_is_invalid_path")
	{
		testCatchDstField(Variant(".."), CatchTestResult::ExceptionIsNotCatched, false);
	}

	SECTION("catch_is_invalid_sequence")
	{
		Variant vScenarioCode = Sequence({
			Dictionary({{"$set", createObject<TestErrorCommand>()}, {"$dst", "a"}, {"$catch", Sequence({false}) }}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			});

		auto pScenario = createAndCompileScenario(vScenarioCode, true);
		REQUIRE(pScenario == nullptr);
	}

	SECTION("catch_is_sequence_without_default_and_first_catch")
	{
		auto pScenario = createScenarioWithSequenceCatch(error::ErrorCode::OutOfRange, false);
		Dictionary vContext;
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["d"] == 3);
		REQUIRE(vContext["OutOfRange"]["errCode"] == error::ErrorCode::OutOfRange);
		REQUIRE_FALSE(vContext.has("InvalidArgument"));
		REQUIRE_FALSE(vContext.has("All"));
	}

	SECTION("catch_is_sequence_without_default_and_second_catch")
	{
		auto pScenario = createScenarioWithSequenceCatch(error::ErrorCode::InvalidArgument, false);
		Dictionary vContext;
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["d"] == 4);
		REQUIRE_FALSE(vContext.has("OutOfRange"));
		REQUIRE(vContext["InvalidArgument"]["errCode"] == error::ErrorCode::InvalidArgument);
		REQUIRE_FALSE(vContext.has("All"));
	}

	SECTION("catch_is_sequence_without_default_and_no_catch")
	{
		auto pScenario = createScenarioWithSequenceCatch(error::ErrorCode::NotFound, false);
		Dictionary vContext;
		REQUIRE_THROWS([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["d"] == 1);
		REQUIRE_FALSE(vContext.has("OutOfRange"));
		REQUIRE_FALSE(vContext.has("InvalidArgument"));
		REQUIRE_FALSE(vContext.has("All"));
	}

	SECTION("catch_is_sequence_with_default_and_first_catch")
	{
		auto pScenario = createScenarioWithSequenceCatch(error::ErrorCode::OutOfRange, true);
		Dictionary vContext;
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["d"] == 3);
		REQUIRE(vContext["OutOfRange"]["errCode"] == error::ErrorCode::OutOfRange);
		REQUIRE_FALSE(vContext.has("InvalidArgument"));
		REQUIRE_FALSE(vContext.has("All"));
	}

	SECTION("catch_is_sequence_with_default_and_default_catch")
	{
		auto pScenario = createScenarioWithSequenceCatch(error::ErrorCode::NotFound, true);
		Dictionary vContext;
		REQUIRE_NOTHROW([&vContext, &pScenario]() { pScenario->execute(vContext); }());
		REQUIRE(vContext["d"] == 5);
		REQUIRE_FALSE(vContext.has("OutOfRange"));
		REQUIRE_FALSE(vContext.has("InvalidArgument"));
		REQUIRE(vContext["All"]["errCode"] == error::ErrorCode::NotFound);
	}
}

//
//
//
TEST_CASE("Scenario.finalConstruct")
{
	SECTION("name_is_exist")
	{
		char sScenarioName[] = "ExampleScenarioName";

		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
		});

		Variant vCfg = Dictionary({ 
			{"code", vCode}, 
			{"name", sScenarioName},
		});

		auto pScenario = queryInterface<ICommand>(createObject(CLSID_Scenario, vCfg));

		REQUIRE(pScenario->getDescriptionString() == sScenarioName);
	}

	SECTION("name_is_absent")
	{
		Variant vCode = Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			});

		Variant vCfg = Dictionary({
			{"code", vCode},
		});

		auto pScenario = queryInterface<ICommand>(createObject(CLSID_Scenario, vCfg));

		std::string sScenarioName = FMT("sid-" << hex(pScenario->getId()));

		REQUIRE(pScenario->getDescriptionString() == sScenarioName);
	}

	SECTION("code_is_Sequence")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", 1}}),
			});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("code_is_Dictionary")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$goto", "a"}})
			})},
			{"a", Sequence({
				Dictionary({{"$goto", "b"}})
			})},
			{"b", Sequence({
				Dictionary({{"$ret", 1}})
			})},
		});

		REQUIRE(execScenario(vCode) == 1);
	}

	SECTION("code_is_Dictionary_without_main")
	{
		Variant vCode = Dictionary({
			{"b", Sequence({
				Dictionary({{"$ret", 1}})
			})},
		});

		REQUIRE_THROWS_AS([vCode]() { (void)execScenario(vCode); }(), error::LogicError);
	}

	SECTION("code_is_invalid_type")
	{
		Variant vCode = 1;

		REQUIRE_THROWS_AS([vCode]() { (void)execScenario(vCode); }(), error::LogicError);
	}

	SECTION("addParams_is_true")
	{
		Variant vCode = Sequence({
			Dictionary({{ "$ret", Dictionary({{"$path", "params"}, {"$default", 0}}) }}),
		});

		Variant vCfg = Dictionary({
			{"code", vCode},
			{"addParams", true},
			});

		auto pCmd = queryInterface<ICommand>(createObject(CLSID_Scenario, vCfg));
		REQUIRE(pCmd->execute(1) == 1);
		auto pCtxCmd = queryInterface<IContextCommand>(pCmd);
		REQUIRE(pCtxCmd->execute(Dictionary(), 1) == 1);
	}

	SECTION("addParams_is_false")
	{
		Variant vCode = Sequence({
			Dictionary({{ "$ret", Dictionary({{"$path", "params"}, {"$default", 0}}) }}),
			});

		Variant vCfg = Dictionary({
			{"code", vCode},
			{"addParams", false},
			});

		auto pCmd = queryInterface<ICommand>(createObject(CLSID_Scenario, vCfg));
		REQUIRE(pCmd->execute(1) == 0);
		auto pCtxCmd = queryInterface<IContextCommand>(pCmd);
		REQUIRE(pCtxCmd->execute(Dictionary(), 1) == 0);
	}

	SECTION("addParams_is_absent")
	{
		Variant vCode = Sequence({
			Dictionary({{ "$ret", Dictionary({{"$path", "params"}, {"$default", 0}}) }}),
			});

		Variant vCfg = Dictionary({
			{"code", vCode},
			});

		auto pCmd = queryInterface<ICommand>(createObject(CLSID_Scenario, vCfg));
		REQUIRE(pCmd->execute(1) == 0);
		auto pCtxCmd = queryInterface<IContextCommand>(pCmd);
		REQUIRE(pCtxCmd->execute(Dictionary(), 1) == 0);
	}

}

//
//
//
TEST_CASE("Scenario.compilation")
{
	FinalConstructCounter::g_nActionCount = 0;
	Variant vLazyFinalConstructCounter = FinalConstructCounter::createDataProxy(false);

	Variant vCode = Sequence({
		Dictionary({{"$ret", vLazyFinalConstructCounter}, {"$dst", "a"}}),
	});

	REQUIRE(FinalConstructCounter::g_nActionCount == 0);

	SECTION("compile_on_first_exec")
	{
		auto pScenario = queryInterface<ICommand>(createObject(CLSID_Scenario, Dictionary({ {"code", vCode}, {"compileOnCall", true} })));
		REQUIRE(FinalConstructCounter::g_nActionCount == 1);

		// First call check
		{
			ObjPtr<IObject> pObj = pScenario->execute(nullptr);
			REQUIRE(pObj->getClassId() == CLSID_FinalConstructCounter);
			REQUIRE(FinalConstructCounter::g_nActionCount == 1);
		}

		// Second call check
		{
			ObjPtr<IObject> pObj = pScenario->execute(nullptr);
			REQUIRE(pObj->getClassId() == CLSID_FinalConstructCounter);
			REQUIRE(FinalConstructCounter::g_nActionCount == 1);
		}
	}

	SECTION("compile_on_finalConstruct")
	{
		auto pScenario = queryInterface<ICommand>(createObject(CLSID_Scenario, Dictionary({ {"code", vCode} })));
		REQUIRE(FinalConstructCounter::g_nActionCount == 0);

		// First call check
		{
			ObjPtr<IObject> pObj = pScenario->execute(nullptr);
			REQUIRE(pObj->getClassId() == CLSID_FinalConstructCounter);
			REQUIRE(FinalConstructCounter::g_nActionCount == 1);
		}

		// Second call check
		{
			ObjPtr<IObject> pObj = pScenario->execute(nullptr);
			REQUIRE(pObj->getClassId() == CLSID_FinalConstructCounter);
			REQUIRE(FinalConstructCounter::g_nActionCount == 1);
		}
	}
}

//
//
//
TEST_CASE("Scenario.execute")
{
	SECTION("pass_context")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", Dictionary({{"$path", "a"}}) }}),
		});

		Dictionary vContext({ {"a", 1} });
		REQUIRE(execScenario(vCode, vContext) == 1);
	}

	SECTION("pass_null_context")
	{
		Variant vCode = Sequence({
			Dictionary({{"$ret", Dictionary({{"$path", ""}}) }}),
		});

		REQUIRE(execScenario(vCode, nullptr) == Dictionary());
	}

	SECTION("execution_without_return")
	{
		Variant vCode = Dictionary({
			{"main", Sequence({
				Dictionary({{"$set", 1}, {"$dst", "a"}}),
				Dictionary({{"$set", 2}, {"$dst", "b"}}),
				Dictionary({{"$set", 3}, {"$dst", "c"}}),
			})},
			{"second", Sequence({
				Dictionary({{"$set", 4}, {"$dst", "d"}}),
			})},
		});

		Dictionary vContext;
		REQUIRE(execScenario(vCode, vContext) == nullptr);

		REQUIRE(vContext == Dictionary({ {"a",1}, {"b",2}, {"c",3} }));
	}
}

//
//
//
enum class MultithreadCounters
{
	Run = 0,
	InvalidResult,
	InvalidContext,
	Exception,

	Max
};

static std::atomic_uint32_t g_eMultithreadCounters[(size_t)MultithreadCounters::Max];

void runScenarioSeveralTimes(ObjPtr<IContextCommand> pScenario, Size nRunCount)
{
	Variant vEthalonContext = Dictionary({ {"a", 11}, {"b", 22}, {"c", 33}, {"aa", 1} });

	for (Size i = 0; i < nRunCount; ++i)
	{
		try
		{
			Dictionary vContext;
			if (pScenario->execute(vContext, nullptr) != 1)
				++g_eMultithreadCounters[(size_t)MultithreadCounters::InvalidResult];

			if (vContext != vEthalonContext)
				++g_eMultithreadCounters[(size_t)MultithreadCounters::InvalidContext];

		}
		catch (error::Exception& e)
		{
			e.log();
			++g_eMultithreadCounters[(size_t)MultithreadCounters::Exception];
		}
		catch (...)
		{
			++g_eMultithreadCounters[(size_t)MultithreadCounters::Exception];
		}

		++g_eMultithreadCounters[(size_t)MultithreadCounters::Run];
	}

}

//
//
//
TEST_CASE("Scenario.multithread_execute")
{
	auto pCommand = createObject<TestExtractContextCommand>(Dictionary({ {"path", "a"} }));

	Variant vCode = Dictionary({
		{"main", Sequence({
			Dictionary({{"$set", 1}, {"$dst", "a"}}),
			Dictionary({{"$set", 2}, {"$dst", "b"}}),
			Dictionary({{"$set", 3}, {"$dst", "c"}}),
			Dictionary({{"$exec", pCommand}, {"$dst", "aa"}}),
			Dictionary({{"$goto", "second"}}),
		})},
		{"second", Sequence({
			Dictionary({{"$set", 11}, {"$dst", "a"}}),
			Dictionary({{"$set", 22}, {"$dst", "b"}}),
			Dictionary({{"$set", 33}, {"$dst", "c"}}),
			Dictionary({{"$ret", Dictionary({{"$path", "aa"}}) }}),
		})},
	});

	auto pScenario = queryInterface<IContextCommand>(createObject(CLSID_Scenario, Dictionary({ {"code", vCode} })));

	static constexpr Size c_nRunCount = 2000;
	static constexpr Size c_nThreadCount = 10;

	std::thread testThreads[c_nThreadCount];
	for (auto& testThread : testThreads)
		testThread = std::thread(runScenarioSeveralTimes, pScenario, c_nRunCount);

	for (auto& testThread : testThreads)
		testThread.join();

	CHECK(g_eMultithreadCounters[(size_t)MultithreadCounters::Run] == c_nRunCount * c_nThreadCount);
	CHECK(g_eMultithreadCounters[(size_t)MultithreadCounters::InvalidContext] == 0);
	CHECK(g_eMultithreadCounters[(size_t)MultithreadCounters::InvalidResult] == 0);
	CHECK(g_eMultithreadCounters[(size_t)MultithreadCounters::Exception] == 0);
}



