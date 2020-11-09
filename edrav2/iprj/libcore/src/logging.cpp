// 
// edrav2.libcore project
//
// Author: Yury Ermakov (22.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
///
/// @file Logging implementation
///
#include "pch.h"
#include <io.hpp>
#include "logsink.h"

namespace openEdr {
namespace logging {
namespace detail {

static const log4cplus::tstring c_sCritical(LOG4CPLUS_TEXT("CRITICAL"));
static const log4cplus::tstring c_sFiltered(LOG4CPLUS_TEXT("FILTERED"));
static const log4cplus::tstring c_sNormal(LOG4CPLUS_TEXT("NORMAL"));
static const log4cplus::tstring c_sDetailed(LOG4CPLUS_TEXT("DETAILED"));
static const log4cplus::tstring c_sDebug(LOG4CPLUS_TEXT("DEBUG"));
static const log4cplus::tstring c_sTrace(LOG4CPLUS_TEXT("TRACE"));
static const log4cplus::tstring c_sAll(LOG4CPLUS_TEXT("ALL"));
static const log4cplus::tstring c_sNotSet(LOG4CPLUS_TEXT("NOTSET"));
static const log4cplus::tstring c_sOff(LOG4CPLUS_TEXT("OFF"));
static const log4cplus::tstring c_sUnknown(LOG4CPLUS_TEXT("UNKNOWN"));

//
//
//
static const log4cplus::tstring& getLogLevelString(LogLevel loglevel)
{
	switch (loglevel)
	{
		case LogLevel::Critical:	return c_sCritical;
		case LogLevel::Filtered:	return c_sFiltered;
		case LogLevel::Normal:		return c_sNormal;
		case LogLevel::Detailed:	return c_sDetailed;
		case LogLevel::Debug:		return c_sDebug;
		case LogLevel::Trace:		return c_sTrace;
		case LogLevel::NotSet:		return c_sNotSet;
		// All and Off levels are not used here
	};

	return c_sUnknown;
}

//
//
//
static LogLevel getLogLevelFromString(const log4cplus::tstring& s)
{
	// Since C++11, accessing str[0] is always safe as it returns '\0' for
	// empty string.

	switch (s[0])
	{
#define DEF_LLMATCH(_chr, _logLevel)        \
        case LOG4CPLUS_TEXT (_chr):         \
            if (s == c_s ## _logLevel)      \
                return LogLevel::_logLevel;	\
            else                            \
                break;

		//DEF_LLMATCH('O', OFF);
		DEF_LLMATCH('C', Critical);
		DEF_LLMATCH('F', Filtered);
		DEF_LLMATCH('N', Normal);
		DEF_LLMATCH('T', Trace);
		DEF_LLMATCH('A', All);
#undef DEF_LLMATCH

		// First letter collision. Not using the macro
		case LOG4CPLUS_TEXT('D'):
		{
			if (s == c_sDetailed)
				return LogLevel::Detailed;
			else if (s == c_sDebug)
				return LogLevel::Debug;
			else
				break;
		}
	}

	return LogLevel::NotSet;
}

//////////////////////////////////////////////////////////////////////////////
//
// LogController methods
//
//////////////////////////////////////////////////////////////////////////////

//
//
//
LogController::LogController()
{
}

//
//
//
LogController::~LogController()
{
}

//
//
//
void LogController::init(const Variant& vConfig)
{
	TRACE_BEGIN;

	if (vConfig.isNull())
	{
		initDefault();
		return;
	}

	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "Dictionary expected").throwException();

	std::scoped_lock lockInit(m_mtxInit);
	if (m_fInitialized)
	{
		LOGLVL(Debug, "Logging module is being reinitialized");
		m_sinkMap.clear();
		std::scoped_lock lock(m_mtxLoggerMap);
		m_loggerMap.clear();
		m_fInitialized = false;
	}
	else
	{
		// Only for first initialization
		// Register custom log levels
		log4cplus::LogLevelManager& manager = log4cplus::getLogLevelManager();
		manager.pushToStringMethod((log4cplus::LogLevelToStringMethod)getLogLevelString);
		manager.pushFromStringMethod((log4cplus::StringToLogLevelMethod)getLogLevelFromString);
	}

	log4cplus::helpers::Properties properties;

	// Configure root logger
	std::optional<std::string> sAppenders = getAppendersList(Sequence(vConfig.get("sink", Sequence())));
	properties.setProperty("log4cplus.rootLogger",
		getLogLevelString(static_cast<LogLevel>(int(vConfig.get("logLevel", 3)))) + ", " +
		(sAppenders.has_value() ? sAppenders.value() : ""));

	// Configure non-root loggers
	Dictionary components = vConfig.get("components", Dictionary());
	for (auto comp : components)
	{
		sAppenders = getAppendersList(Sequence(comp.second.get("sink", Sequence())));
		properties.setProperty("log4cplus.logger." + static_cast<std::string>(comp.first),
			getLogLevelString(comp.second["logLevel"]) +
			(sAppenders.has_value() ? sAppenders.value() : ""));
	}

	// Load sinks
	Dictionary sinks = vConfig.get("sinks", Dictionary());
	for (auto sink : sinks)
	{ 
		auto pSinkObject = getSinkObject(sink.second);
		ObjPtr<ILog4CPlusAppender> pAppender = queryInterfaceSafe<ILog4CPlusAppender>(pSinkObject);
		if (pAppender != nullptr)
			pAppender->fillProperties(static_cast<std::string>(sink.first), properties);

		m_sinkMap[static_cast<std::string>(sink.first)] = pSinkObject;
	}

	log4cplus::PropertyConfigurator configurator(properties);
	configurator.configure();

	m_rootLogger = log4cplus::Logger::getRoot();

	m_fInitialized = true;
	TRACE_END("Error during LogController initialization");
}

//
//
//
void LogController::initDefault()
{
	TRACE_BEGIN;

	std::scoped_lock lockInit(m_mtxInit);
	if (m_fInitialized)
		return;

	// Register custom log levels
	log4cplus::LogLevelManager& manager = log4cplus::getLogLevelManager();
	manager.pushToStringMethod((log4cplus::LogLevelToStringMethod)getLogLevelString);
	manager.pushFromStringMethod((log4cplus::StringToLogLevelMethod)getLogLevelFromString);

	log4cplus::helpers::Properties properties;
	properties.setProperty("log4cplus.rootLogger",
		getLogLevelString(LogLevel::Normal) + ", consoleAppender");
	properties.setProperty("log4cplus.appender.consoleAppender", "log4cplus::ConsoleAppender");
	properties.setProperty("log4cplus.appender.consoleAppender.layout", "log4cplus::PatternLayout");
	properties.setProperty("log4cplus.appender.consoleAppender.layout.ConversionPattern",
		"%t [%-8.-8c{1}] %m%n");

	log4cplus::PropertyConfigurator configurator(properties);
	configurator.configure();

	m_rootLogger = log4cplus::Logger::getRoot();

	m_fInitialized = true;
	TRACE_END("Error during LogController default initialization");
}

//
//
//
ObjPtr<IObject> LogController::getSinkObject(Variant& v)
{
	if (v.getType() == Variant::ValueType::Object)
		return v;
	return createObject(v);
}

//
//
//
std::optional<std::string> LogController::getAppendersList(Sequence seqAppenders)
{
	std::string sResult;
	for (auto sink : seqAppenders)
		sResult += std::string(sink) + ',';
	if (sResult.empty()) return {};
	sResult.resize(sResult.size() - 1);
	return sResult;
}

//
//
//
LogController& LogController::getInstance()
{
	static LogController g_logController;
	return g_logController;
}

//
//
//
log4cplus::Logger LogController::getLogger(const std::string& sName)
{
	if (!m_fInitialized)
	{
		CMD_TRY
		{
			initDefault();
		}
		CMD_PREPARE_CATCH
		catch (const error::Exception& ex)
		{
			ex.print(std::cerr);
			throw;
		}
	}

	if (sName.empty())
		return m_rootLogger;

	std::scoped_lock lock(m_mtxLoggerMap);

	LoggerMap::const_iterator iter = m_loggerMap.find(sName);
	if (iter != m_loggerMap.end())
		return iter->second;
	
	m_loggerMap[sName] = log4cplus::Logger::getInstance(sName);
	return m_loggerMap[sName];
}

} // namespace detail

//
//
//
void initLogging(const Variant& vConfig)
{
	TRACE_BEGIN;
	CMD_TRY
	{
		detail::LogController::getInstance().init(vConfig);
	}
	CMD_PREPARE_CATCH
	catch (const error::Exception& ex)
	{
		ex.print(std::cerr);
		throw;
	}
	TRACE_END("Error during logging initialization");
}

//
//
//
void initLogging()
{
	initLogging(getCatalogData("app.config.log", {}));
}

//
//
//
void setRootLogLevel(LogLevel logLevel)
{
	detail::LogController::getInstance().getLogger("").setLogLevel(
		static_cast<log4cplus::LogLevel>(logLevel));
}

//
//
//
void setComponentLogLevel(const std::string& sName, LogLevel logLevel)
{
	detail::LogController::getInstance().getLogger(sName).setLogLevel(
		static_cast<log4cplus::LogLevel>(logLevel));
}

} // namespace logging
} // namespace openEdr


#undef CMD_COMPONENT
#define CMD_COMPONENT "madchook"

void logMadCHookError(const wchar_t* sMsg)
{
	auto s = openEdr::string::convertWCharToUtf8(sMsg);
	openEdr::error::RuntimeError(SL, s).log();
}

void logMadCHookWarning(const wchar_t* sMsg)
{
	auto s = openEdr::string::convertWCharToUtf8(sMsg);
	LOGWRN(s);
}

void logMadCHookInfo(const wchar_t* sMsg)
{
	auto s = openEdr::string::convertWCharToUtf8(sMsg);
	LOGLVL(Debug, s);
}
