#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/helpers/fileinfo.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/initializer.h>


using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;

typedef helpers::chrono::high_resolution_clock hr_clock;
typedef helpers::chrono::duration<double, std::ratio<1>> sec_dur_type;

log4cplus::tostream& operator <<(log4cplus::tostream& s, const Time& t)
{
    return s << to_time_t (t) << "sec "
             << microseconds_part (t) << "usec";
}


#define LOOP_COUNT 100000


log4cplus::tstring
getPropertiesFileArgument (int argc, char * argv[])
{
    if (argc >= 2)
    {
        char const * arg = argv[1];
        log4cplus::tstring file = LOG4CPLUS_C_STR_TO_TSTRING (arg);
        log4cplus::helpers::FileInfo fi;
        if (getFileInfo (&fi, file) == 0)
            return file;
    }

    return LOG4CPLUS_TEXT ("log4cplus.properties");
}


int
main(int argc, char * argv[])
{
    tcout << LOG4CPLUS_TEXT("Entering main()...") << endl;
    log4cplus::Initializer initializer;

    PropertyConfigurator::doConfigure(getPropertiesFileArgument (argc, argv));
    Logger root = Logger::getRoot();
    try {
        Logger logger = Logger::getInstance(LOG4CPLUS_TEXT("testlogger"));

        LOG4CPLUS_WARN(Logger::getRoot (), "Starting test loop....");

        hr_clock::time_point start = hr_clock::now ();
        tstring msg(LOG4CPLUS_TEXT("This is a WARNING..."));
        int i = 0;
        for(i=0; i<LOOP_COUNT; ++i) {
            LOG4CPLUS_WARN(logger, msg);
        }
        hr_clock::time_point end = hr_clock::now ();
        hr_clock::duration diff = end - start;
        double diff_seconds = sec_dur_type (diff).count ();
        LOG4CPLUS_WARN(LOG4CPLUS_TEXT("root"), "Logging " << LOOP_COUNT
                       << " took: " << diff_seconds << endl);
        LOG4CPLUS_WARN(root, "Logging average: " << (diff_seconds/LOOP_COUNT)
                       << endl);

        start = hr_clock::now ();
        for(i=0; i<LOOP_COUNT; ++i) {
            tostringstream buffer;
            buffer /*<< "test"*/ << 123122;
            tstring tmp = buffer.str();
        }
        end = hr_clock::now ();
        diff = end - start;
        diff_seconds = sec_dur_type (diff).count ();
        LOG4CPLUS_WARN(root, "tostringstream average: "
                       << (diff_seconds/LOOP_COUNT) << endl);

        start = hr_clock::now ();
        for(i=0; i<LOOP_COUNT; ++i) {
            log4cplus::spi::InternalLoggingEvent e(logger.getName(),
                log4cplus::WARN_LOG_LEVEL, msg, __FILE__, __LINE__, "main");
        }
        end = hr_clock::now ();
        diff = end - start;
        diff_seconds = sec_dur_type (diff).count ();
        LOG4CPLUS_WARN(root, "Creating log " << LOOP_COUNT << " objects took: "
                       << diff_seconds);
        LOG4CPLUS_WARN(root, "Creating log object average: "
                       << (diff_seconds/LOOP_COUNT) << endl);

        start = hr_clock::now ();
        for(i=0; i<LOOP_COUNT; ++i) {
            log4cplus::spi::InternalLoggingEvent e(logger.getName(),
                log4cplus::WARN_LOG_LEVEL, msg, __FILE__, __LINE__, "main");
            e.getNDC();
            e.getThread();
        }
        end = hr_clock::now ();
        diff = end - start;
        diff_seconds = sec_dur_type (diff).count ();
        LOG4CPLUS_WARN(root, "Creating FULL log " << LOOP_COUNT
                       << " objects took: " << diff_seconds);
        LOG4CPLUS_WARN(root, "Creating FULL log object average: "
                       << (diff_seconds/LOOP_COUNT) << endl);

        start = hr_clock::now ();
        for(i=0; i<LOOP_COUNT; ++i) {
            log4cplus::spi::InternalLoggingEvent e(logger.getName(),
                log4cplus::WARN_LOG_LEVEL, msg, __FILE__, __LINE__, "main");
            e.getNDC();
        }
        end = hr_clock::now ();
        diff = end - start;
        diff_seconds = sec_dur_type (diff).count ();
        LOG4CPLUS_WARN(root, "getNDC() " << LOOP_COUNT << " calls took: "
                       << diff_seconds);
        LOG4CPLUS_WARN(root, "getNDC() average: " << (diff_seconds/LOOP_COUNT)
                       << endl);

        start = hr_clock::now ();
        for(i=0; i<LOOP_COUNT; ++i) {
            log4cplus::spi::InternalLoggingEvent e(logger.getName(),
                log4cplus::WARN_LOG_LEVEL, msg, __FILE__, __LINE__, "main");
            e.getThread();
        }
        end = hr_clock::now ();
        diff = end - start;
        diff_seconds = sec_dur_type (diff).count ();
        LOG4CPLUS_WARN(root, "getThread() " << LOOP_COUNT << " calls took: "
                       << diff_seconds);
        LOG4CPLUS_WARN(root, "getThread() average: "
                       << (diff_seconds/LOOP_COUNT) << endl);
    }
    catch(...) {
        tcout << LOG4CPLUS_TEXT("Exception...") << endl;
        LOG4CPLUS_FATAL(root, "Exception occured...");
    }

    tcout << LOG4CPLUS_TEXT("Exiting main()...") << endl;

    return 0;
}
