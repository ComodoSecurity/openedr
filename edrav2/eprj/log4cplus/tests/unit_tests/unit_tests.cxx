#include <log4cplus/log4cplus.h>

int
main (int argc, char* argv[])
{
    log4cplus::Initializer initializer;
    int result = 0;

#if defined (LOG4CPLUS_WITH_UNIT_TESTS)
    result = log4cplus::unit_tests_main (argc, argv);
#endif

    return result;
}
