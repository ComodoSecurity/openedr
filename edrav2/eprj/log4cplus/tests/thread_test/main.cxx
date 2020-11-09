#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/logger.h>
#include <log4cplus/ndc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/streams.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/tracelogger.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/initializer.h>
#include <exception>
#include <iostream>
#include <string>


using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;
using namespace log4cplus::thread;


#define NUM_THREADS 4
#define NUM_LOOPS 10

class SlowObject
    : public log4cplus::helpers::SharedObject
{
public:
    SlowObject()
        : logger(Logger::getInstance(LOG4CPLUS_TEXT("SlowObject")))
    {
        logger.setLogLevel(TRACE_LOG_LEVEL);
    }

    void doSomething()
    {
        LOG4CPLUS_TRACE_METHOD(logger, LOG4CPLUS_TEXT("SlowObject::doSomething()"));
        {
            log4cplus::thread::MutexGuard guard (mutex);
            LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("Actually doing something..."));
            std::this_thread::sleep_for (std::chrono::milliseconds (75));
            LOG4CPLUS_INFO_FMT(logger,
                LOG4CPLUS_TEXT (
                    "Actually doing something...%d, %d, %d, %ls...DONE"),
                1, 2, 3, L"testing");
        }
        log4cplus::thread::yield();
    }

    ~SlowObject ()
    { }

private:
    log4cplus::thread::Mutex mutex;
    Logger logger;
};


using SlowObjectPtr = log4cplus::helpers::SharedObjectPtr<SlowObject>;


class TestThread : public AbstractThread {
public:
    TestThread (tstring const & n, SlowObjectPtr so)
        : name(n)
        , slow(std::move (so))
        , logger(Logger::getInstance(LOG4CPLUS_TEXT("test.TestThread")))
     { }

    virtual void run();

private:
    tstring name;
    SlowObjectPtr slow;
    Logger logger;
};


using TestThreadPtr = log4cplus::helpers::SharedObjectPtr<TestThread>;


int
main()
{
    log4cplus::Initializer initializer;

    try
    {
        SlowObjectPtr slowObject(new SlowObject());
        log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("main"));
        Logger::getRoot().setLogLevel(INFO_LOG_LEVEL);
        LogLevel ll = logger.getLogLevel();
        tcout << "main Priority: " << getLogLevelManager().toString(ll) << endl;

        log4cplus::helpers::Properties props;
        props.setProperty(LOG4CPLUS_TEXT ("AsyncAppend"),
            LOG4CPLUS_TEXT ("true"));
        helpers::SharedObjectPtr<Appender> append_1(new ConsoleAppender(props));
        append_1->setLayout(std::unique_ptr<Layout>(new log4cplus::TTCCLayout));
        Logger::getRoot().addAppender(append_1);
        append_1->setName(LOG4CPLUS_TEXT("cout"));
        append_1 = 0;

        TestThreadPtr threads[NUM_THREADS];
        int i = 0;
        for(i=0; i<NUM_THREADS; ++i) {
            tostringstream s;
            s << "Thread-" << i;
            threads[i] = new TestThread(s.str(), slowObject);
        }

        for(i=0; i<NUM_THREADS; ++i) {
            threads[i]->start();
            log4cplus::setThreadPoolSize (i % 10);
        }

        LOG4CPLUS_DEBUG(logger, "All Threads started...");

        for(i=0; i<NUM_THREADS; ++i) {
            threads[i]->join ();
            log4cplus::setThreadPoolSize (i % 10);
        }
        LOG4CPLUS_INFO(logger, "Exiting main()...");
    }
    catch(std::exception &e) {
        LOG4CPLUS_FATAL(Logger::getRoot(), "main()- Exception occured: " << e.what());
    }
    catch(...) {
        LOG4CPLUS_FATAL(Logger::getRoot(), "main()- Exception occured");
    }

    return 0;
}


void
TestThread::run()
{
    // Test is here just to test initializer from multiple threads.
    log4cplus::Initializer initializer;

    try {
        LOG4CPLUS_WARN(logger, name + LOG4CPLUS_TEXT(" TestThread.run()- Starting..."));
        NDC& ndc = getNDC();
        NDCContextCreator _first_ndc(name);
        LOG4CPLUS_DEBUG(logger, "Entering Run()...");
        for(int i=0; i<NUM_LOOPS; ++i) {
            NDCContextCreator _ndc(LOG4CPLUS_TEXT("loop"));
            log4cplus::setThreadPoolSize (16 - (i % 8));
            slow->doSomething();
        }
        LOG4CPLUS_DEBUG(logger, "Exiting run()...");

        ndc.remove();
    }
    catch(std::exception const & e) {
        LOG4CPLUS_FATAL(logger, "TestThread.run()- Exception occurred: " << e.what());
    }
    catch(...) {
        LOG4CPLUS_FATAL(logger, "TestThread.run()- Exception occurred!!");
    }
    LOG4CPLUS_WARN(logger, name << " TestThread.run()- Finished");
} // end "run"
