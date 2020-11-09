#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/ndc.h>
#include <log4cplus/helpers/loglog.h>
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
        append_1->setLayout(
            std::unique_ptr<Layout>(
                new log4cplus::PatternLayout(
                    LOG4CPLUS_TEXT ("%-5p %c <%x> - %m%n"))));
        Logger::getRoot().addAppender(append_1);

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("test"));
        log4cplus::tcout << "Logger: " << logger.getName() << endl;
        getNDC().push(LOG4CPLUS_TEXT("tsmith"));
        LOG4CPLUS_DEBUG(logger, LOG4CPLUS_TEXT("This is a short test..."));

        getNDC().push(LOG4CPLUS_TEXT("password"));
        LOG4CPLUS_DEBUG(logger, LOG4CPLUS_TEXT("This should have my password now"));

        getNDC().pop();
        LOG4CPLUS_DEBUG(logger, LOG4CPLUS_TEXT("This should NOT have my password now"));

        getNDC().pop_void ();
        cout << "Just returned from pop..." << endl;
        LOG4CPLUS_DEBUG(logger, LOG4CPLUS_TEXT("There should be no NDC..."));

        getNDC().push(LOG4CPLUS_TEXT("tsmith"));
        getNDC().push(LOG4CPLUS_TEXT("password"));
        LOG4CPLUS_DEBUG(logger, LOG4CPLUS_TEXT("This should have my password now"));
        getNDC().remove();
        LOG4CPLUS_DEBUG(logger, LOG4CPLUS_TEXT("There should be no NDC..."));
    }
    catch(...) {
        cout << "Exception..." << endl;
        Logger::getRoot().log(FATAL_LOG_LEVEL, LOG4CPLUS_TEXT("Exception occured..."));
    }

    cout << "Exiting main()..." << endl;
    return 0;
}
