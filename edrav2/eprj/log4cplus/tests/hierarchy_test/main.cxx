#include <iostream>
#include <string>
#include "log4cplus/hierarchy.h"
#include "log4cplus/helpers/loglog.h"
#include "log4cplus/loggingmacros.h"
#include "log4cplus/configurator.h"
#include "log4cplus/initializer.h"

using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;

int
main()
{
    log4cplus::Initializer initializer;
    {
        BasicConfigurator::doConfigure ();
        Logger root = Logger::getRoot ();
        Hierarchy & hier = root.getHierarchy ();

        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("test"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test2"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a.b.c"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a.b.c"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a.b.c.d"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a.b.c"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest.a"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        logger = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest"));
        log4cplus::tcout << "Logger name: " << logger.getName()
             << " Parent = " << logger.getParent().getName() << endl;

        // Test that loggers exist.

        LOG4CPLUS_ASSERT (root, hier.exists (LOG4CPLUS_TEXT ("test.subtest")));
        LOG4CPLUS_ASSERT (root, hier.exists (LOG4CPLUS_TEXT ("test")));

        // Test clearing hierarchy.

        hier.clear ();

        // Test that loggers have been removed from the hierarchy.

        LOG4CPLUS_ASSERT (root,
            ! hier.exists (LOG4CPLUS_TEXT ("test.subtest")));
        LOG4CPLUS_ASSERT (root, ! hier.exists (LOG4CPLUS_TEXT ("test")));

        // Test that instantiation of logger with empty name returns
        // the root logger.

        LOG4CPLUS_ASSERT (root,
            Logger::getInstance (LOG4CPLUS_TEXT ("")).getName ()
            == Logger::getRoot ().getName ());

        // Test for root logger existence.

        LOG4CPLUS_ASSERT (root, hier.exists (LOG4CPLUS_TEXT ("")));

        // Test disabling log level by name.

        LOG4CPLUS_ASSERT (root, ! hier.isDisabled (WARN_LOG_LEVEL));
        hier.disable (LOG4CPLUS_TEXT ("WARN"));
        LOG4CPLUS_ASSERT (root, hier.isDisabled (WARN_LOG_LEVEL));
        LOG4CPLUS_ERROR (root,
            LOG4CPLUS_TEXT ("WARN log level and below are disabled")
            LOG4CPLUS_TEXT (" and that is fine"));
        LOG4CPLUS_WARN (root,
            LOG4CPLUS_TEXT ("WARN log level and below are disabled")
            LOG4CPLUS_TEXT (" and this should not be printed"));

        // Test disabling log level by log level constant.

        hier.enableAll ();
        LOG4CPLUS_ASSERT (root, ! hier.isDisabled (WARN_LOG_LEVEL));
        hier.disable (ERROR_LOG_LEVEL);
        LOG4CPLUS_ASSERT (root, hier.isDisabled (WARN_LOG_LEVEL));
        LOG4CPLUS_ERROR (root,
            LOG4CPLUS_TEXT ("ERROR log level and below are disabled")
            LOG4CPLUS_TEXT (" and this should not be printed"));
        LOG4CPLUS_FATAL (root,
            LOG4CPLUS_TEXT ("ERROR log level and below are disabled")
            LOG4CPLUS_TEXT (" and this should still be printed"));

        // Test disabling debug log level by disableDebug().

        hier.disable (Hierarchy::DISABLE_OFF);
        LOG4CPLUS_ASSERT (root, ! hier.isDisabled (DEBUG_LOG_LEVEL));
        hier.disableDebug ();
        LOG4CPLUS_ASSERT (root, hier.isDisabled (DEBUG_LOG_LEVEL));

        // Test disabling info log level by disableInfo().

        hier.enableAll ();
        LOG4CPLUS_ASSERT (root, ! hier.isDisabled (INFO_LOG_LEVEL));
        hier.disableInfo ();
        LOG4CPLUS_ASSERT (root, hier.isDisabled (INFO_LOG_LEVEL));
        LOG4CPLUS_ASSERT (root, ! hier.isDisabled (WARN_LOG_LEVEL));

        // Test disabling of all log levels.

        hier.enableAll ();
        LOG4CPLUS_ASSERT (root, ! hier.isDisabled (FATAL_LOG_LEVEL));
        hier.disableAll ();
        LOG4CPLUS_ASSERT (root, hier.isDisabled (FATAL_LOG_LEVEL));
        LOG4CPLUS_FATAL (root,
            LOG4CPLUS_TEXT ("all log levels are disabled")
            LOG4CPLUS_TEXT (" and this should not be printed"));
    }
    log4cplus::tcout << "Exiting main()..." << endl;
    return 0;
}
