// 
// EDRAv2.libcore project
//
// Author: Yury Ermakov (20.12.2018)
// Reviewer: Denis Bogdanov (14.01.2019)
//
///
/// @file Logging declarations
///
/// @addtogroup logging Logging subsystem
/// Logging subsystem is intended to provide an ability to receive and 
/// permanently store (within files on disk) an arbitrary text information
/// from objects about its internal status, current processing stage, etc.
/// @{
#pragma once

#include "variant.hpp"
#include "error.hpp"

namespace cmd {
namespace logging {

///
/// Enumeration class for log levels.
///
enum class LogLevel : int
{
	Off = 100,		///< No log output.
	Critical = 5,	///< Is used only for errors and warnings.
	Filtered = 4,	///< Is used only for messages which are important from the point of view of business logic.
	Normal = 3,		///< Default work mode. The primary diagnostic messages are also acceptable.
	Detailed = 2,	///< The detailed diagnostic messages use it. This level is default for internal builds.
	Debug = 1,		///< The debug information is outputed.
	Trace = 0,		///< Is used for receiving step traces (only for debug purposes).
	All = Trace,	///< All messages to be shown.
	NotSet = -1,	///< Log level is not set.
};

namespace detail {

using LoggerMap = std::map<std::string, log4cplus::Logger>;
using LogSinkMap = std::map<std::string, ObjPtr<IObject>>;

///
/// The main controller class for logging subsystem.
///
class LogController
{
private:
	std::atomic<bool> m_fInitialized = false;
	std::mutex m_mtxInit;
	log4cplus::Initializer m_intInitializer;	// initialization and shutdown of log4cplus library objects
	LogSinkMap m_sinkMap;
	log4cplus::Logger m_rootLogger;
	LoggerMap m_loggerMap;
	mutable std::mutex m_mtxLoggerMap;

	//
	// Object default initialization
	//
	void initDefault();

	//
	// Returns sink object from config (in Variant)
	//
	ObjPtr<IObject> getSinkObject(Variant& vConfig);

	//
	// Returns comma-separated list of appenders
	//
	std::optional<std::string> getAppendersList(Sequence s);

public:
	///
	/// Constructor.
	///
	LogController();

	///
	/// Destructor.
	///
	~LogController();

	///
	/// Object initialization.
	///
	/// @param vConfig - configuration.
	///
	void init(const Variant& vConfig);

	///
	/// Gets the single global instance of `LogController` class.
	///
	/// @returns The function returns the single global instance of `LogController` class.
	///
	static LogController& getInstance();

	///
	/// Gets a logger by name.
	///
	/// @param sName - the logger's name. Empty ("") name for the root logger.
	/// @returns The function returns a requested logger object.
	///
	log4cplus::Logger getLogger(const std::string& sName);
};

static const char* c_sLogIndent = "                                              ";

} // namespace detail 

///
/// Global logging initialization using parameters.
///
/// This function shall be called once at application start.
///
/// @param vConfig - log subsystem configuration.
///
void initLogging(const Variant& vConfig);

///
/// Global logging initialization using global catalog.
///
/// This function shall be called once at application start. 
/// It gets the configuration data from global canalogue.
///
void initLogging();

///
/// Configure the root log level.
///
/// This function can be used for dynamically changing of the log level
/// of the root logger.
/// @param logLevel - a logging level.
///
void setRootLogLevel(LogLevel logLevel);

///
/// Configure the log level of specified component's logger.
///
/// This function can be used for dynamically changing of the log level
/// of the specified component's logger.
/// @param sName - a name of the component.
/// @param logLevel - a logging level.
///
void setComponentLogLevel(const std::string& sName, LogLevel logLevel);

} // namespace logging

// Bring log levels to cmd namespace
using logging::LogLevel;

} // namespace cmd

//////////////////////////////////////////////////////////////////////////////
//
// Logging macros
//
//////////////////////////////////////////////////////////////////////////////

// Helpers for using optional parameters in macros.
#define _MSVC_VA_ARGS_WORKAROUND(define, args) define args
#define _GET_MACRO(_1, _2, _3, _4, name, ...) name

#define _LOG4CPLUS_MACRO_BODY(logger, logLevel, logEvent) \
	LOG4CPLUS_SUPPRESS_DOWHILE_WARNING() \
	do { \
		log4cplus::Logger const& _l = log4cplus::detail::macros_get_logger(logger);	\
		if (!_l.isEnabledFor(logLevel)) break; \
		LOG4CPLUS_MACRO_INSTANTIATE_OSTRINGSTREAM(_log4cplus_buf); \
		_log4cplus_buf << logEvent; \
		log4cplus::detail::macro_forced_log(_l,	logLevel, _log4cplus_buf.str(),	\
			LOG4CPLUS_MACRO_FILE(), __LINE__, LOG4CPLUS_MACRO_FUNCTION()); \
	} while (0)	\
	LOG4CPLUS_RESTORE_DOWHILE_WARNING()

#define _LOG2(logger, logLevel, logEvent) \
	_LOG4CPLUS_MACRO_BODY(cmd::logging::detail::LogController::getInstance().getLogger(logger), \
		static_cast<log4cplus::LogLevel>(logLevel), logEvent)

#define _LOG(logger, logLevel, logEvent) \
	_LOG2(logger, cmd::LogLevel::logLevel, logEvent)

///
/// @def LOGERR([component,] exception [, logEvent])
/// @def LOGERR([component,] errorCode [, logEvent])
///
/// Error message output to log.
///
/// This macro is used for reporting about errors. Besides the general
/// information about the incident, additional information will be added
/// to log, such as: error code's full description and the call stack.
///
/// Examples:
/// @code{.cpp}
///   LOGERR("injector", exceptionObject, "Can't load driver inject.sys"); 
///   LOGERR(exceptionObject, "Can't load driver inject.sys");		// implicit component
///   LOGERR(errorCode, "Can't load driver inject.sys");			// implicit component
/// @endcode
///
#define LOGERR(...) \
	_MSVC_VA_ARGS_WORKAROUND(_GET_MACRO, (__VA_ARGS__, _4, _LOG_ERR3, _LOG_ERR2, _LOG_ERR1))(__VA_ARGS__)

//20181024-145614.254 0b48 [INJECTOR] [ERR] 0000A428 - Access denied
//										    Can't load driver inject.sys
//										    Call trace:
//										    -> driver.cpp:255 (WinAPI exception: 0x8004822 - Privileged operation)
//											   inject.cpp:295
//											   main.cpp:28

#define _LOG_ERR(logger, errCodeOrException, logEvent) \
	_LOG(logger, Critical, "[ERR] " << cmd::error::ErrorCodeFmt(errCodeOrException) << std::endl \
		logEvent \
		cmd::error::StackTraceFmt(SL, errCodeOrException, cmd::logging::detail::c_sLogIndent))

#define _LOG_ERR1(errCodeOrException) \
	_LOG_ERR(CMD_COMPONENT, errCodeOrException, <<)

#define _LOG_ERR2(errCodeOrException, logEvent) \
	_LOG_ERR(CMD_COMPONENT, errCodeOrException, \
		<< cmd::logging::detail::c_sLogIndent \
		<< cmd::error::SourceLocationFmt(SL) << " (" << logEvent << ")" << std::endl <<)

#define _LOG_ERR3(logger, errCodeOrException, logEvent) \
	_LOG_ERR(logger, errCodeOrException, \
		<< cmd::logging::detail::c_sLogIndent \
		<< cmd::error::SourceLocationFmt(SL) << " (" << logEvent << ")" << std::endl <<)

///
/// Output error without logging of source location.
///
/// @param "..." - variable arguments providing additional information.
///
#define LOGERRNOSL(...) \
	_MSVC_VA_ARGS_WORKAROUND(_GET_MACRO, (__VA_ARGS__, _4, _LOG_ERRNOSL3, _LOG_ERRNOSL2, _LOG_ERRNOSL1))(__VA_ARGS__)

#define _LOG_ERRNOSL(logger, errCodeOrException, logEvent) \
	_LOG(logger, Critical, "[ERR] " << cmd::error::ErrorCodeFmt(errCodeOrException) << std::endl \
		logEvent \
		cmd::error::StackTraceFmt(SL, errCodeOrException, cmd::logging::detail::c_sLogIndent))

#define _LOG_ERRNOSL1(errCodeOrException) \
	_LOG_ERRNOSL(CMD_COMPONENT, errCodeOrException, << )

#define _LOG_ERRNOSL2(logger, errCodeOrException) \
	_LOG_ERRNOSL(logger, errCodeOrException, <<)

#define _LOG_ERRNOSL3(logger, errCodeOrException, logEvent) \
	_LOG_ERRNOSL(logger, errCodeOrException, \
		<< cmd::logging::detail::c_sLogIndent << logEvent << std::endl <<)


///
/// @def LOGWRN([component,] logEvent)
///
/// Warngnig message output to log.
///
/// @param component - a name of the component which the message belongs to.
/// If `component` is omitted the value of the `CMD_COMPONENT` define will be
/// used instead.
/// @param logEvent - a message text in ostream-like format.
///
/// Examples:
/// @code{.cpp}
///   LOGWRN("config", "Section 'services' is not found"); 
///   LOGWRN("Section 'services' is not found");	// implicit component
/// @endcode
///
#define LOGWRN(...) \
	_MSVC_VA_ARGS_WORKAROUND(_GET_MACRO, (__VA_ARGS__, _4, _3, _LOG_WRN2, _LOG_WRN1))(Critical, __VA_ARGS__)

#define _LOG_WRN1(logLevel, logEvent) _LOG_WRN2(logLevel, CMD_COMPONENT, logEvent)

#define _LOG_WRN2(logLevel, logger, logEvent) _LOG(logger, logLevel, "[WRN] " << logEvent)

///
/// @def LOGLVL(logLevel, [component,] logEvent)
///
/// Adds to log the information message on given level.
///
/// @param logLevel - a log level name (from logging::LogLevel).
/// @param component [opt] - a name of the component which the message belongs to.
/// If `component` is omitted the value of the `CMD_COMPONENT` define will be
/// used instead.
/// @param logEvent - a message text in ostream-like format.
///
/// Examples:
/// @code{.cpp}
///   LOGLVL(Filtered, "application", "Some important message from the POV of business logic"); 
///   LOGLVL(Normal, "Section 'services' is not found");			// implicit component
/// @endcode
///
#define LOGLVL(...) \
	_MSVC_VA_ARGS_WORKAROUND(_GET_MACRO, (__VA_ARGS__, _4, _LOG_INF3, _LOG_INF2, _1))(__VA_ARGS__)

#define _LOG_INF2(logLevel, logEvent) _LOG_INF3(logLevel, CMD_COMPONENT, logEvent)

#define _LOG_INF3(logLevel, logger, logEvent) _LOG(logger, logLevel, "[INF] " << logEvent)

///
/// @def LOGLVL2(logLevel, [component,] logEvent)
///
/// Adds to log the information message on given level.
///
/// @param logLevel - a logging level number.
/// @param component [opt] - a name of the component which the message belongs to.
/// If `component` is omitted the value of the `CMD_COMPONENT` define will be
/// used instead.
/// @param logEvent - a message text in ostream-like format.
///
/// Examples:
/// @code{.cpp}
///   LOGLVL2(LogLevel::Filtered, "application", "Some important message from the POV of business logic"); 
///   LOGLVL2(LogLevel::Normal, "Omitted component name leads to using of implicit one");
///   LOGLVL2(0, "Use 0 as number for trace log level");
/// @endcode
///
#define LOGLVL2(...) \
	_MSVC_VA_ARGS_WORKAROUND(_GET_MACRO, (__VA_ARGS__, _4, _LOG2_INF3, _LOG2_INF2, _1))(__VA_ARGS__)

#define _LOG2_INF2(logLevel, logEvent) _LOG2_INF3(logLevel, CMD_COMPONENT, logEvent)

#define _LOG2_INF3(logLevel, logger, logEvent) _LOG2(logger, logLevel, "[INF] " << logEvent)

///
/// @def LOGINF([component,] logEvent)
///
/// Adds to log the information message on `Normal` level.
///
/// @param component - a name of the component which the message belongs to.
/// If `component` is omitted the value of the `CMD_COMPONENT` define will
/// be used instead.
/// @param logEvent - a message text in ostream-like format.
///
/// This macro is used as a default in any common cases. 
/// The primary diagnostic messages are also acceptable.
///
/// Examples:
/// @code{.cpp}
///   LOGINF("application", "Application started"); 
///   LOGINF("Application finished");			// implicit component
/// @endcode
///
#define LOGINF(...) LOGLVL(Normal, __VA_ARGS__)

// This macro is for preventing IntelliSence errors
#ifdef __INTELLISENSE__
#undef LOGERR
#undef LOGERRNOSL
#undef LOGINF
#undef LOGWRN
#undef LOGLVL
#define LOGERR(...)
#define LOGERRNOSL(...)
#define LOGINF(...)
#define LOGWRN(...)
#define LOGLVL(...)
#endif

// Logging functions from madCHook
#ifdef __cplusplus
extern "C" {
#endif
	void logMadCHookError(const wchar_t* sMsg);
	void logMadCHookWarning(const wchar_t* sMsg);
	void logMadCHookInfo(const wchar_t* sMsg);
#ifdef __cplusplus
} // extern "C" 
#endif
/// @}
