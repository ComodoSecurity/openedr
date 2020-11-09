
#include "customloglevel.h"
#include <log4cplus/loggingmacros.h>

using namespace log4cplus;
using namespace log4cplus::helpers;

void
writeLogMessage() 
{
    {
        Logger subTest = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest"));
        subTest.log(FATAL_LOG_LEVEL, LOG4CPLUS_TEXT("Entering writeLogMessage()..."));
        LOG4CPLUS_CRITICAL(subTest, 
            LOG4CPLUS_TEXT("writeLogMessage()- This is a message from a different file"));
        subTest.log(FATAL_LOG_LEVEL, LOG4CPLUS_TEXT("Exiting writeLogMessage()..."));
    }
    LogLog::getLogLog()->warn(LOG4CPLUS_TEXT("REALLY exiting writeLogMessage()..."));
}


