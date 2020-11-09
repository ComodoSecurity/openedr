
#include <log4cplus/logger.h>
#include <log4cplus/helpers/loglog.h>

using namespace log4cplus;
using namespace log4cplus::helpers;

const LogLevel CRITICAL_LOG_LEVEL = 45000;

#define LOG4CPLUS_CRITICAL(logger, logEvent) \
    if(logger.isEnabledFor(CRITICAL_LOG_LEVEL)) { \
        log4cplus::tostringstream _log4cplus_buf; \
        _log4cplus_buf << logEvent; \
	logger.forcedLog(CRITICAL_LOG_LEVEL, _log4cplus_buf.str(), __FILE__, __LINE__); \
    }


