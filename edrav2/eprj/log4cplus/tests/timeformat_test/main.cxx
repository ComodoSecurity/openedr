#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/streams.h>
#include <log4cplus/logger.h>
#include <log4cplus/initializer.h>
#include <iostream>

using namespace log4cplus;
using namespace log4cplus::helpers;


log4cplus::tchar const fmtstr[] =
    LOG4CPLUS_TEXT("%s, %Q%%q%q %%Q %%q=%%%q%%;%%q, %%Q=%Q");


int
main()
{
    std::cout << "Entering main()..." << std::endl;
    log4cplus::Initializer initializer;
    try
    {
        Time time;
        log4cplus::tstring str;

        time = now ();
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << LOG4CPLUS_TEXT ("now: ") << str << std::endl;

        time = time_from_parts (0, 7);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;

        time = time_from_parts (0, 17);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;

        time = time_from_parts (0, 123);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;

        time = time_from_parts (0, 1234);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;

        time = time_from_parts (0, 12345);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;

        time = time_from_parts (0, 123456);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;

        time = time_from_parts (0, 0);
        str = getFormattedTime (fmtstr, time);
        log4cplus::tcout << str << std::endl;
    }
    catch(std::exception const & e)
    {
        std::cout << "Exception: " << e.what () << std::endl;
    }
    catch(...)
    {
        std::cout << "Exception..." << std::endl;
    }

    std::cout << "Exiting main()..." << std::endl;
    return 0;
}
