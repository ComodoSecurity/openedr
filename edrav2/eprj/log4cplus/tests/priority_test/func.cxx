
#include "log4cplus/logger.h"
#include "log4cplus/helpers/loglog.h"
#include "log4cplus/loggingmacros.h"

using namespace log4cplus;
using namespace log4cplus::helpers;

void
writeLogMessage() 
{
    {
        Logger subTest = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest"));
        subTest.log(FATAL_LOG_LEVEL, LOG4CPLUS_TEXT("Entering writeLogMessage()..."));
        LOG4CPLUS_FATAL(subTest, "writeLogMessage()- This is a message from a different file");
        subTest.log(FATAL_LOG_LEVEL, LOG4CPLUS_TEXT("Exiting writeLogMessage()..."));
    }
    LogLog::getLogLog()->warn(LOG4CPLUS_TEXT("REALLY exiting writeLogMessage()..."));
}


