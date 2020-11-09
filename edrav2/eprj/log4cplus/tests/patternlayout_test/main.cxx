#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/ndc.h>
#include <log4cplus/mdc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/initializer.h>
#include <iostream>
#include <string>

using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;

int
main()
{
    cout << "Entering main()..." << endl;
    log4cplus::Initializer initializer;
    LogLog::getLogLog()->setInternalDebugging(true);
    try {
        SharedObjectPtr<Appender> append_1(new ConsoleAppender());
        append_1->setName(LOG4CPLUS_TEXT("First"));

        log4cplus::getMDC ().put (LOG4CPLUS_TEXT ("key"),
            LOG4CPLUS_TEXT ("MDC value"));
        log4cplus::tstring pattern = LOG4CPLUS_TEXT("%d{%m/%d/%y %H:%M:%S,%Q} [%t] %-5p %c{2} %%%x%% - %X{key} - %m [%l]%n");
        //	std::tstring pattern = LOG4CPLUS_TEXT("%d{%c} [%t] %-5p [%.15c{3}] %%%x%% - %m [%l]%n");
        append_1->setLayout( std::unique_ptr<Layout>(new PatternLayout(pattern)) );
        Logger::getRoot().addAppender(append_1);

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("test.a.long_logger_name.c.logger"));
        LOG4CPLUS_DEBUG(logger, "This is the FIRST log message...");

        std::this_thread::sleep_for (std::chrono::seconds (1));
        {
            NDCContextCreator ndc(LOG4CPLUS_TEXT("second"));
            LOG4CPLUS_INFO(logger, "This is the SECOND log message...");
        }

        std::this_thread::sleep_for (std::chrono::seconds (1));
        LOG4CPLUS_WARN(logger, "This is the THIRD log message...");

        std::this_thread::sleep_for (std::chrono::seconds (1));
        LOG4CPLUS_ERROR(logger, "This is the FOURTH log message...");

        std::this_thread::sleep_for (std::chrono::seconds (1));
        LOG4CPLUS_FATAL(logger, "This is the FIFTH log message...");
    }
    catch(...) {
        cout << "Exception..." << endl;
        Logger::getRoot().log(FATAL_LOG_LEVEL, LOG4CPLUS_TEXT("Exception occured..."));
    }

    cout << "Exiting main()..." << endl;
    return 0;
}
