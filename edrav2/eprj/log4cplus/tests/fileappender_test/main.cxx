#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/ndc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/initializer.h>


using namespace log4cplus;

const int LOOP_COUNT = 20000;


int
main()
{
    log4cplus::Initializer initializer;
    helpers::LogLog::getLogLog()->setInternalDebugging(true);
    SharedFileAppenderPtr append_1(
        new RollingFileAppender(LOG4CPLUS_TEXT("a////b/c/d/Test.log"), 5*1024, 5,
            false, true));
    append_1->setName(LOG4CPLUS_TEXT("First"));
    append_1->setLayout( std::unique_ptr<Layout>(new TTCCLayout()) );
    append_1->getloc();
    Logger::getRoot().addAppender(SharedAppenderPtr(append_1.get ()));

    Logger root = Logger::getRoot();
    Logger test = Logger::getInstance(LOG4CPLUS_TEXT("test"));
    Logger subTest = Logger::getInstance(LOG4CPLUS_TEXT("test.subtest"));

    for(int i=0; i<LOOP_COUNT; ++i) {
        NDCContextCreator _context(LOG4CPLUS_TEXT("loop"));
        LOG4CPLUS_DEBUG(subTest, "Entering loop #" << i);
    }

    {
        tistringstream propsStream (LOG4CPLUS_TEXT ("File="));
        helpers::Properties props (propsStream);
        FileAppender appender (props);
        appender.setName (LOG4CPLUS_TEXT ("Second"));
    }

    {
        tistringstream propsStream (
            LOG4CPLUS_TEXT("File=nonexistent/Test.log"));
        helpers::Properties props (propsStream);
        FileAppender appender (props);
        appender.setName (LOG4CPLUS_TEXT ("Third"));
    }

    {
        // This is checking that CreateDirs is respected when UseLockFile is
        // provided and that directories for the lock file are created.
        tistringstream propsStream (
            LOG4CPLUS_TEXT("CreateDirs=true\n")
            LOG4CPLUS_TEXT("File=./logs/some_name.log\n")
            LOG4CPLUS_TEXT("UseLockFile=true\n")
            LOG4CPLUS_TEXT("MaxFileSize=100MB\n")
            LOG4CPLUS_TEXT("MaxBackupIndex=10\n"));
        helpers::Properties props (propsStream);
        FileAppender appender (props);
        appender.setName (LOG4CPLUS_TEXT ("Fourth"));
    }

    return 0;
}
