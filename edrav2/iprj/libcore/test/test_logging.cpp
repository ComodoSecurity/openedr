//
// edrav2.ut_libcore project
//
// Author: Yury Ermakov (21.12.2018)
// Reviewer:
//
// Unit tests for Logging subsystem of libcore.
//
// TODO: Log level dynamic change (setRootLogLevel, setComponentLogLevel)
// TODO: Logfile rotation
// TODO: Different log files (sinks)
// TODO: Output cut-off according to configured level
// TODO: Invalid config (or we should use tests from log4cplus library?):
//   - invalid logfile path
//   - unknown log level
//   - unknown sink's clsid
//   - unknown log4cplus factory for appenders and layout
//
#include "pch.h"

using namespace cmd;

#undef CMD_COMPONENT
#define CMD_COMPONENT "default" 

void initDefaultLogging()
{
	std::string sJson = R"json({
			"logLevel": 0,
			"sink": ["logfile"],
			"components": {
				"comp1": { "logLevel": 1 },
				"comp2": { "logLevel": 2 }
			},
			"sinks": {
				"console": {
					"$$clsid": "0xE18D3126",
					"factory": "log4cplus::ConsoleAppender",
					"layout": {
						"factory": "log4cplus::PatternLayout",
						"ConversionPattern": "[%-5p][%d] %m%n"
					}
				},
				"logfile" : {
					"$$clsid": "0xE18D3126",
					"factory": "log4cplus::RollingFileAppender",
					"File": "logging.txt",
					"Locale": "en_US.UTF8",
					"MaxFileSize": "16MB",
					"MaxBackupIndex": "1",
					"layout": {
						"factory": "log4cplus::PatternLayout",
						"ConversionPattern": "%D{%Y%m%d-%H%M%S.%q} %t [%-8.-8c{1}] %m%n"
					}
				}
			}
		})json";

	Variant vConfig(cmd::variant::deserializeFromJson(sJson));
	logging::initLogging(vConfig);
}

//
// Test for logging in UTF8 encoding.
//
#define UTF8_TEST_STRING u8"ABCァD漢字фыва"
#define WCHAR_TEST_STRING L"ABCァD漢字фыва"
TEST_CASE("logging.uft8")
{
	REQUIRE_NOTHROW([]()
	{
		initDefaultLogging();
		LOGINF("utf-8 literal " << 123 << " >> " << UTF8_TEST_STRING << " << " << 456);
		LOGINF("utf-8 string " << 123 << " >> " << std::string(UTF8_TEST_STRING) << " << " << 456);

	}());
}

//
// Test for checking the case of double initialization of logging module.
//
TEST_CASE("logging.double_initializaion")
{
	REQUIRE_NOTHROW([]()
	{
		initDefaultLogging();
		initDefaultLogging();
	}());
}

//
// Test for checking for information logging on different levels.
//
TEST_CASE("logging.LOGLVL")
{
	initDefaultLogging();
	SECTION("Filtered")
	{
		REQUIRE_NOTHROW([]()
		{
			LOGLVL(Filtered, "LOGLVL(Filtered): Implicit component");
			LOGLVL(Filtered, "unittest", "LOGLVL(Filtered): Explicit component");
		}());
	}
	SECTION("Normal")
	{
		REQUIRE_NOTHROW([]()
		{
			LOGLVL(Normal, "LOGLVL(Normal): Implicit component");
			LOGLVL(Normal, "unittest", "LOGLVL(Normal): Explicit component");
		}());
	}
	SECTION("Detailed")
	{
		REQUIRE_NOTHROW([]()
		{
			LOGLVL(Detailed, "LOGLVL(Detailed): Implicit component");
			LOGLVL(Detailed, "unittest", "LOGLVL(Detailed): Explicit component");
		}());
	}
	SECTION("Debug")
	{
		REQUIRE_NOTHROW([]()
		{
			LOGLVL(Debug, "LOGLVL(Debug): Implicit component");
			LOGLVL(Debug, "unittest", "LOGLVL(Debug): Explicit component");
		}());
	}
	SECTION("Trace")
	{
		REQUIRE_NOTHROW([]()
		{
			LOGLVL(Trace, "LOGLVL(Trace): Implicit component");
			LOGLVL(Trace, "unittest", "LOGLVL(Trace): Explicit component");
		}());
	}
}

//
// Test for checking for information logging on Normal level.
//
TEST_CASE("logging.LOGINF")
{
	REQUIRE_NOTHROW([]()
	{
		initDefaultLogging();
		LOGINF("LOGINF: Implicit component");
		LOGINF("unittest", "LOGINF: Explicit component");
	}());
}

//
// Test for checking for errors logging.
//
TEST_CASE("logging.LOGERR_errorCode")
{
	REQUIRE_NOTHROW([]()
	{
		initDefaultLogging();
		LOGERR(ErrorCode::AccessDenied);
		LOGERR(ErrorCode::AccessDenied, "LOGERR: Implicit component");
		LOGERR("unittest", ErrorCode::AccessDenied, "LOGERR: Explicit component");
	}());
}

//
// Test for checking for exceptions logging.
//
TEST_CASE("logging.LOGERR_Exception")
{
	REQUIRE_NOTHROW([]()
	{
		initDefaultLogging();
		LOGERR(error::Exception(ErrorCode::Undefined, "Unittest Exception"));
		LOGERR(error::Exception(ErrorCode::Undefined, "Unittest Exception"),
			"LOGERR: Implicit component");
		LOGERR("unittest", error::Exception(ErrorCode::Undefined, "Unittest Exception"),
			"LOGERR: Explicit component");

		// TODO: multi-level exceptions
	}());
}

//
// Test for checking for warnings logging.
//
TEST_CASE("logging.LOGWRN")
{
	REQUIRE_NOTHROW([]()
	{
		initDefaultLogging();
		LOGWRN("LOGWRN: Implicit component");
		LOGWRN("unittest", "LOGWRN: Explicit component");
	}());
}

//
//
//
template<typename fnGetLogEvent>
void logExpanded(const char* logger, log4cplus::LogLevel logLevel, fnGetLogEvent logEvent)
{
	__pragma (warning(push))
	__pragma (warning(disable:4127))

	do {
		log4cplus::Logger const& _l = log4cplus::detail::macros_get_logger(logger);
		if (!_l.isEnabledFor(logLevel))
			break;
		log4cplus::detail::macro_oss_guard ___log4cplus_buf__;
		log4cplus::tostringstream& _log4cplus_buf = ___log4cplus_buf__.get();
		//log4cplus::tostringstream& _log4cplus_buf = log4cplus::detail::get_macro_body_oss();
		//log4cplus::tostringstream _log4cplus_buf;
		_log4cplus_buf << logEvent();
		log4cplus::detail::macro_forced_log(_l, logLevel, _log4cplus_buf.str(),
			LOG4CPLUS_MACRO_FILE(), __LINE__, LOG4CPLUS_MACRO_FUNCTION());
	} while (0)

	__pragma (warning(pop));
}

//
//
//
std::string getSomeStringWithInnerLogging()
{
	logExpanded("", 0, [](){ return "[INNER LOGGING TEXT]"; });
	//LOGINF("[INNER LOGGING TEXT]");
	return "Some string";
}

//
//
//
TEST_CASE("logging.nested")
{
	initDefaultLogging();
	auto fnScenario = []()
	{
		//LOGINF("[OUTER LOGGING TEXT #1]");
		//LOGINF("[OUTER LOGGING TEXT #2]" << getSomeStringWithInnerLogging());
		logExpanded("", 0, [](){ return getSomeStringWithInnerLogging(); });
	};
	REQUIRE_NOTHROW(fnScenario());
}
