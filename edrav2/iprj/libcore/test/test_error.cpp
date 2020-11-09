//
//  EDRAv2 project
// Unit test for errors processing subsystem
//
#include "pch.h"

using namespace openEdr;

TEST_CASE("ErrorCode.Enum")
{
	REQUIRE(static_cast<int>(ErrorCode::OK) == 0x00000000);
	REQUIRE(static_cast<int>(ErrorCode::Undefined) == INT32_MIN);
}

TEST_CASE("ErrorCode.Description")
{
	SECTION("enumOverload")
	{
		REQUIRE(error::getErrorString(ErrorCode::OK) == std::string("OK"));
		REQUIRE(error::getErrorString(ErrorCode::Undefined) == std::string("Undefined"));
		REQUIRE(error::getErrorString(ErrorCode::RuntimeError) == std::string("Runtime error"));
	}

	// TODO:
	//SECTION("unknown description")
	//{
	//}
}

TEST_CASE("ErrorCode.outputToStream")
{
	std::ostringstream os;
	os << ErrorCode::OK;
	REQUIRE(os.str() == "0x00000000");
}

TEST_CASE("Exception.Generic")
{
	SECTION("defaultParameters")
	{
		error::Exception e(ErrorCode::Undefined);
		REQUIRE(e.what() == std::string("Undefined"));
		REQUIRE(e.getErrorCode() == ErrorCode::Undefined);
	}

	SECTION("selfThrowAndCatch")
	{
		auto fn = [&]()
		{
			error::Exception(SL, ErrorCode::Undefined, "test").throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("arbitraryExceptionToException")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			throw "Arbitrary exception";
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}
}

TEST_CASE("Exception.Hierarchy")
{
	SECTION("genericLogicErrorToException")
	{
		auto fn = [&]()
		{
			error::LogicError(SL, "test").throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("genericRuntimeErrorToException")
	{
		auto fn = [&]()
		{
			error::RuntimeError(SL, "test").throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("genericSystemErrorToException")
	{
		auto fn = [&]()
		{
			error::SystemError(SL, "test").throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("genericSystemErrorToRuntimeError")
	{
		auto fn = [&]()
		{
			error::SystemError(SL, "test").throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("specificExceptionToException")
	{
		auto fn = [&]()
		{
			error::TypeError(SL, "test").throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("specificExceptionToLogicError")
	{
		auto fn = [&]()
		{
			try
			{
				error::OutOfRange(SL, "test").throwException();
			}
			catch (const error::LogicError&)
			{
				throw;
			}
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("specificExceptionToRuntimeError")
	{
		auto fn = [&]()
		{
			try
			{
				error::AccessDenied(SL, "test").throwException();
			}
			catch (const error::RuntimeError&)
			{
				throw;
			}
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	// TODO: use any specific SystemError
	//SECTION("specific exception to SystemError") {
	//  auto fn = [&]() {
	//    CMD_THROW(..., "test");
	//  };
	//  REQUIRE_THROWS_AS(fn(), SystemError);
	//}
}

TEST_CASE("Exception.StdExceptions")
{
	SECTION("out_of_range")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			auto s = std::string().at(1);
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::OutOfRange);
	}

	SECTION("out_of_rangeToLogicError")
	{
		auto fn = [&]() {
			CMD_TRACE_BEGIN;
			std::vector<int> v;
			v.resize(v.max_size() + 1);
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("logic_error")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			throw std::logic_error("manual std::logic_error");
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("runtime_error")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			throw std::runtime_error("manual std::runtime_error");
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("complexExceptionDescription")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			auto s = std::string().at(1);
			CMD_TRACE_END("High-level exception");
		};
		REQUIRE_THROWS_WITH(fn(), "High-level exception: invalid string position");
	}
}

TEST_CASE("Exception.RuntimeError")
{
	SECTION("defaultParameters")
	{
		error::RuntimeError e;
		REQUIRE(e.what() == std::string("Runtime error"));
		REQUIRE(e.getErrorCode() == ErrorCode::RuntimeError);
	}

	SECTION("selfThrowAndCatch")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			error::RuntimeError(SL, "test").throwException();
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}
}

TEST_CASE("Exception.LogicError")
{
	SECTION("defaultParameters")
	{
		error::LogicError e;
		REQUIRE(e.what() == std::string("Logic error"));
		REQUIRE(e.getErrorCode() == ErrorCode::LogicError);
	}

	SECTION("selfThrowAndCatch")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			error::LogicError(SL, "test").throwException();
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}
}

TEST_CASE("Exception.Stacktrace")
{
	SECTION("simple")
	{
		auto fn3 = [&]()
		{
			std::vector<int> v;
			v.resize(v.max_size() + 1);
		};
		auto fn2 = [&]()
		{
			CMD_TRACE_BEGIN;
			fn3();
			CMD_TRACE_END("Catch on L2");
		};
		auto fn1 = [&]()
		{
			CMD_TRACE_BEGIN;
			fn2();
			CMD_TRACE_END("Catch on L1");
		};
		try
		{
			fn1();
		}
		catch (const error::Exception& e)
		{
			// TODO: Check the stacktrace's content here
			e.print();
		}
		catch (...)
		{
			FAIL("Unhandled exception!");
		}
	}
}

TEST_CASE("Exception.rethrow")
{
	auto fn = [&]()
	{
		std::exception_ptr exc;
		try
		{
			error::LogicError(SL, "test").throwException();
		}
		catch (error::LogicError&)
		{
			exc = std::current_exception();
		}
		std::rethrow_exception(exc);
	};
	REQUIRE_THROWS_AS(fn(), error::LogicError);
}

TEST_CASE("Exception.factory")
{
	auto fn = [&]()
	{
		error::Exception(SL, ErrorCode::SystemError, "test").throwException();
	};
	REQUIRE_THROWS_AS(fn(), error::SystemError);
}

//
//
//
TEST_CASE("Exception.serialization")
{
	Variant vSerialized = Dictionary({
		{ "sMsg", "Test exception message" },
		{ "errCode", ErrorCode::LogicError },
		{ "stackTrace", Sequence({
			Dictionary({ { "sFile", "test_error.cpp" }, { "nLine", 42 }, {"sDescription", "line #1" } }),
			Dictionary({ { "sFile", "test_error.cpp" }, { "nLine", 43 }, {"sDescription", "line #2" } }),
			}) }
		});

	auto fnScenario = [&](const Variant& vOrigin)
	{
		try
		{
			error::Exception(vOrigin).throwException();
		}
		catch (const error::OutOfRange&)
		{
			throw;
		}
		catch (const error::Exception& ex)
		{
			Variant vResult = ex.serialize();
			if (vResult["sMsg"] != vOrigin["sMsg"]) FAIL("sMsg fields are not equal");
			if (vResult["errCode"] != vOrigin["errCode"]) FAIL("errCode fields are not equal");
			if (!vResult.has("stackTrace")) FAIL("stackTrace is not found");
			if (vResult["stackTrace"].getSize() != 2) FAIL("stackTrace has an unexpected size");
			if (!vResult["stackTrace"][0].has("sFile")) FAIL("stackTrace.sFile is not found");
			if (vResult["stackTrace"][0]["sFile"] != vOrigin["stackTrace"][0]["sFile"]) FAIL("stackTrace.sFile is not equal to original one");
			if (!vResult["stackTrace"][0].has("nLine")) FAIL("stackTrace.nLine is not found");
			if (vResult["stackTrace"][0]["nLine"] != vOrigin["stackTrace"][0]["nLine"]) FAIL("stackTrace.nLine is not equal to original one");
			if (!vResult["stackTrace"][0].has("sDescription")) FAIL("stackTrace.nLine is not found");
			if (vResult["stackTrace"][0]["sDescription"] != vOrigin["stackTrace"][0]["sDescription"]) FAIL("stackTrace.sDescription is not equal to original one");
			if (!vResult["stackTrace"][1].has("sFile")) FAIL("stackTrace.sFile is not found");
			if (vResult["stackTrace"][1]["sFile"] != vOrigin["stackTrace"][1]["sFile"]) FAIL("stackTrace.sFile is not equal to original one");
			if (!vResult["stackTrace"][1].has("nLine")) FAIL("stackTrace.nLine is not found");
			if (vResult["stackTrace"][1]["nLine"] != vOrigin["stackTrace"][1]["nLine"]) FAIL("stackTrace.nLine is not equal to original one");
			if (!vResult["stackTrace"][1].has("sDescription")) FAIL("stackTrace.nLine is not found");
			if (vResult["stackTrace"][1]["sDescription"] != vOrigin["stackTrace"][1]["sDescription"]) FAIL("stackTrace.sDescription is not equal to original one");
		}
	};

	SECTION("good")
	{
		REQUIRE_NOTHROW(fnScenario(vSerialized));
	}

	SECTION("unknown_errCode")
	{
		vSerialized.put("errCode", 42);
		REQUIRE_NOTHROW(fnScenario(vSerialized));
	}

	SECTION("missing_sMsg")
	{
		vSerialized.erase("sMsg");
		REQUIRE_THROWS_AS(fnScenario(vSerialized), error::OutOfRange);
	}

	SECTION("missing_errCode")
	{
		vSerialized.erase("errCode");
		REQUIRE_THROWS_AS(fnScenario(vSerialized), error::OutOfRange);
	}
}

#ifdef _WIN32

//
//
//
TEST_CASE("WinAPIError.throwException")
{
	auto fn = [&]()
	{
		if (!::UnlockFile(INVALID_HANDLE_VALUE, 0, 0, 0, 0))
			error::win::WinApiError(SL, "unittest").throwException();
	};
	REQUIRE_THROWS_AS(fn(), error::win::WinApiError);
}

//
//
//
TEST_CASE("WinAPIError.log")
{
	auto fn = [&]()
	{
		if (!::UnlockFile(INVALID_HANDLE_VALUE, 0, 0, 0, 0))
			error::win::WinApiError(SL, "unittest").log();
	};
	REQUIRE_NOTHROW(fn());
}

//
//
//
TEST_CASE("getWinApiErrorString._")
{
	using openEdr::error::win::getWinApiErrorString;
	CHECK(getWinApiErrorString(ERROR_INVALID_FUNCTION).find("0x00000001") == 0);
	CHECK(getWinApiErrorString(ERROR_FILE_NOT_FOUND).find("0x00000002") == 0);
	CHECK(getWinApiErrorString(ERROR_INVALID_HANDLE).find("0x00000006") == 0);
}

//
//
//
TEST_CASE("Exception.WinAPIError")
{
	using openEdr::error::win::getWinApiErrorString;
	SECTION("knownError")
	{
		error::win::WinApiError e(ERROR_FILE_NOT_FOUND);
		REQUIRE(e.what() == std::string("WinAPI error ") + getWinApiErrorString(ERROR_FILE_NOT_FOUND));
		REQUIRE(e.getErrorCode() == ErrorCode::SystemError);
	}

	SECTION("unknownError")
	{
		error::win::WinApiError e(100000);
		REQUIRE(e.what() == std::string("WinAPI error 0x000186A0"));
		REQUIRE(e.getErrorCode() == ErrorCode::SystemError);
	}

	SECTION("stdExceptionWhat")
	{
		auto fn = [&]()
		{
			try
			{
				error::win::WinApiError(ERROR_FILE_NOT_FOUND).throwException();
			}
			catch (const std::exception&)
			{
				throw;
			}
		};
		REQUIRE_THROWS_WITH(fn(), std::string("WinAPI error ") + getWinApiErrorString(ERROR_FILE_NOT_FOUND));
	}

	SECTION("booleanAsResult")
	{
		auto fn = [&]()
		{
			TRACE_BEGIN;
			if (!::UnlockFile(INVALID_HANDLE_VALUE, 0, 0, 0, 0))
				error::win::WinApiError(SL).throwException();
			TRACE_END();
		};
		
		REQUIRE_THROWS_WITH(fn(), std::string("WinAPI error ") + getWinApiErrorString(ERROR_INVALID_HANDLE));
	}

	SECTION("handleAsResult")
	{
		auto fn = [&]()
		{
			CMD_TRACE_BEGIN;
			HANDLE hFile = ::CreateFile(TEXT("unexisting_file.txt"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
				error::win::WinApiError(SL).throwException();
			CMD_TRACE_END();
		};
		REQUIRE_THROWS_WITH(fn(), std::string("WinAPI error ") + getWinApiErrorString(ERROR_FILE_NOT_FOUND));
	}

	SECTION("stacktrace")
	{
		auto fn3 = [&]()
		{
			CMD_TRACE_BEGIN;
			HANDLE hFile = ::CreateFile(TEXT("unexisting_file.txt"), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
				error::win::WinApiError(SL).throwException();
			CMD_TRACE_END("Catch on L1");
		};
		auto fn2 = [&]()
		{
			CMD_TRACE_BEGIN;
			fn3();
			CMD_TRACE_END("Catch on L2");
		};
		auto fn1 = [&]()
		{
			CMD_TRACE_BEGIN;
			fn2();
			CMD_TRACE_END("Catch on L1");
		};
		try
		{
			fn1();
		}
		catch (const error::Exception& e)
		{
			// TODO: Check the stacktrace's content here
			e.print();
		}
		catch (...)
		{
			FAIL("Unhandled exception!");
		}
	}
}

#endif // #ifdef _WIN32

//
//
//
TEST_CASE("CrtError.basic")
{
	SECTION("knownError")
	{
		error::CrtError e(ECHILD);
		REQUIRE(e.what() == std::string("CRT error 0x0000000A - No child processes"));
		REQUIRE(e.getErrorCode() == ErrorCode::SystemError);
	}

	SECTION("unknownError")
	{
		error::CrtError e(100000);
		REQUIRE(e.what() == std::string("CRT error 0x000186A0 - Unknown error"));
		REQUIRE(e.getErrorCode() == ErrorCode::SystemError);
	}
}

//
//
//
TEST_CASE("CrtError.translate")
{
	SECTION("EINVAL")
	{
		auto fn = [&]()
		{
			error::CrtError(EINVAL).throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidArgument);
	}

	SECTION("ENOENT")
	{
		auto fn = [&]()
		{
			error::CrtError(ENOENT).throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::NotFound);
	}

	SECTION("ECHILD")
	{
		auto fn = [&]()
		{
			error::CrtError(ECHILD).throwException();
		};
		REQUIRE_THROWS_AS(fn(), error::CrtError);
	}
}

//
// Throws cmd exception based on given std exception.
//
#define STD_TO_CMD_EXCEPTION(stdException, cmdException) \
	CMD_TRY \
	{ \
		throw stdException;\
	} \
	CMD_PREPARE_CATCH \
	catch (cmdException&) \
	{ \
		throw; \
	}

//
// Checks the special try-prepare-catch statement
//
TEST_CASE("Exception.CMD_PREPARE_CATCH")
{
//	throw std::logic_error("xx");
	SECTION("logic_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::logic_error("unittest"), error::LogicError);
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("invalid_argument")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::invalid_argument("unittest"), error::InvalidArgument);
		};
		REQUIRE_THROWS_AS(fn(), error::InvalidArgument);
	}

	SECTION("domain_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::domain_error("unittest"), error::LogicError);
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("length_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::length_error("unittest"), error::LogicError);
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("out_of_range")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::out_of_range("unittest"), error::OutOfRange);
		};
		REQUIRE_THROWS_AS(fn(), error::OutOfRange);
	}

	SECTION("future_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::future_error(std::future_errc::broken_promise), error::LogicError);
		};
		REQUIRE_THROWS_AS(fn(), error::LogicError);
	}

	SECTION("bad_optional_access")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_optional_access(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("runtime_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::runtime_error("unittest"), error::RuntimeError);
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("range_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::range_error("unittest"), error::RuntimeError);
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("overflow_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::overflow_error("unittest"), error::RuntimeError);
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("underflow_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::underflow_error("unittest"), error::RuntimeError);
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("regex_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::regex_error(std::regex_constants::error_type::error_collate),
				error::RuntimeError);
		};
		REQUIRE_THROWS_AS(fn(), error::RuntimeError);
	}

	SECTION("system_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::system_error(EDOM, std::generic_category(), "unittest"),
				error::SystemError);
		};
		REQUIRE_THROWS_AS(fn(), error::SystemError);
	}

	SECTION("ios_base_failure")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::ios_base::failure("unittest"), error::SystemError);
		};
		REQUIRE_THROWS_AS(fn(), error::SystemError);
	}

	SECTION("filesystem_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::filesystem::filesystem_error("unittest", std::error_code()),
				error::SystemError);
		};
		REQUIRE_THROWS_AS(fn(), error::SystemError);
	}

	SECTION("bad_typeid")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_typeid(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("bad_cast")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_cast(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("bad_any_cast")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_any_cast(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("bad_weak_ptr")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_weak_ptr(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("bad_function_call")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_function_call(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("bad_alloc")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_alloc(), error::BadAlloc);
		};
		REQUIRE_THROWS_AS(fn(), error::BadAlloc);
	}

	SECTION("bad_array_new_length")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_array_new_length(), error::BadAlloc);
		};
		REQUIRE_THROWS_AS(fn(), error::BadAlloc);
	}

	SECTION("bad_exception")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_exception(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("bad_variant_access")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::bad_variant_access(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("exception")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(std::exception(), error::Exception);
		};
		REQUIRE_THROWS_AS(fn(), error::Exception);
	}

	SECTION("boost_system_error")
	{
		auto fn = [&]()
		{
			STD_TO_CMD_EXCEPTION(boost::system::system_error(42, boost::system::generic_category(), "unittest"),
				error::BoostSystemError);
		};
		REQUIRE_THROWS_AS(fn(), error::BoostSystemError);
	}

}

//
//
//
TEST_CASE("BoostSystemError.translate")
{
	auto fnScenario = [&](int nCode)
	{
		error::BoostSystemError({ nCode, boost::system::generic_category() }).throwException();
	};

	const std::map<const std::string, std::tuple<int>> mapData{
		// sName: nCode
		{ "WSAEINVAL", { 10022 } },
		{ "WSAHOST_NOT_FOUND", { 11001 } },
		{ "WSAENETDOWN", { 10050 } },
		{ "WSAENETUNREACH", { 10051 } },
		{ "WSAETIMEDOUT", { 10060 } },
	};

	for (const auto& [sName, params] : mapData)
	{
		const auto& [nCode] = params;
		DYNAMIC_SECTION(sName)
		{
			REQUIRE_THROWS_AS(fnScenario(nCode), error::ConnectionError);
		}
	}
}
