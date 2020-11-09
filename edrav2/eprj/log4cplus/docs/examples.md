# Code Examples

## Hello World

Here is a minimal [log4cplus] example for [log4cplus] version 2.0 and later:

~~~~{.cpp}
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>

int
main()
{
    // Initialization and deinitialization.
    log4cplus::Initializer initializer;

    log4cplus::BasicConfigurator config;
    config.configure();

    log4cplus::Logger logger = log4cplus::Logger::getInstance(
        LOG4CPLUS_TEXT("main"));
    LOG4CPLUS_WARN(logger, LOG4CPLUS_TEXT("Hello, World!"));
    return 0;
}
~~~~

The above code prints `WARN - Hello, World!` on console. Let's dissect it:

~~~~{.cpp}
#include <log4cplus/logger.h>
~~~~

We need this header to get `Logger` class which represents a handle to named
logger.

~~~~{.cpp}
#include <log4cplus/loggingmacros.h>
~~~~

This header declares `LOG4CPLUS_WARN()` logging macro. Beside this one, it also
declares one for each standard logging level: FATAL, ERROR, WARN, INFO, DEBUG,
TRACE.

~~~~{.cpp}
#include <log4cplus/configurator.h>
~~~~

This header declares `BasicConfigurator` class.

~~~~{.cpp}
#include <log4cplus/initializer.h>
~~~~

This header declares `Initializer` class.

~~~~{.cpp}
log4cplus::Initializer initializer;
~~~~

Instantiating the `Initializer` class internally initializes [log4cplus].

The `Initializer` class also maintains a reference count. The class can be
instantiated multiple times. When this reference count reaches zero, after the
last instance of `Initializer` is destroyed, it shuts down [log4cplus]
internals. Currently, after [log4cplus] is deinitialized, it cannot be
re-initialized.

[log4cplus] tries to use some other methods of shutting down its
internals. However, that means that it **_cannot be used after `main()`
exits_**.

~~~~{.cpp}
log4cplus::BasicConfigurator config;
config.configure();
~~~~

These two lines configure _root logger_ with `ConsoleAppender` and simple
layout.

~~~~{.cpp}
log4cplus::Logger logger = log4cplus::Logger::getInstance(
    LOG4CPLUS_TEXT("main"));
~~~~

Here we obtain logger handle to logger named _main_.

The `LOG4CPLUS_TEXT()` macro used above has the same function as the `TEXT()`
or `_T()` macros do on Windows: In case `UNICODE` preprocessor symbol is
defined, it prefixes the string literal that is passed as its parameter with
the `L` to make it wide character string literal.

~~~~{.cpp}
LOG4CPLUS_WARN(logger, LOG4CPLUS_TEXT("Hello, World!"));
~~~~

Here we invoke the `LOG4CPLUS_WARN()` macro to log the _Hello, World!_ message
into the _main_ logger. The logged message will be propagated from the _main_
logger towards the _root logger_ which has a `ConsoleAppender` attached to it
to print it on console.

Internally, this macro uses C++ string stream to format the _Hello, World!_
message. The consequence of this is that you can use all of the standard C++
streams manipulators.


## (De-)Initialization

### Initialization

In most cases, [log4cplus] is initialized before `main()` is executed. However,
depending on compiler, platform, run time libraries and how log4cplus is linked
to, it is possible it will not be initialized automatically. This is why
initializing [log4cplus] on top of `main()` is a good rule of thumb.

As the previous code example shows, initialization of [log4cplus] is done by
instantiation of `log4cplus::Initializer` class. This is true for [log4cplus]
versions 2.0 and later. In previous versions, instead of instantiating this
class (the header `log4cplus/initializer.h` and the class do not exist there),
call to function `log4cplus::initialize()` is used.

### Deinitialization

[log4cplus] tries to deinitialize itself and free all of its allocated
resources after `main()` exits. However, again, depending on compiler, platform
and run time libraries, it might not be possible. This is why proper
deinitialization is _necessary_.

In version 2.0 and later, it is done by the last instance of
`log4cplus::Initializer` class and its destructor. In previous versions,
calling `Logger::shutdown()` was the proper shutdown method.

## Logging macros

As we have mentioned earlier, `LOG4CPLUS_WARN()`, `LOG4CPLUS_ERROR()`, etc.,
macros use C++ string stream under the hood. The following example demonstrates
how is it possible to use it with the macros.

Beside these macros, there are two more groups of logging macros.
`LOG4CPLUS_*_STR()` can be used for logging messages that are just plain
strings that do not need any kind of formatting. There is also group of
`LOG4CPLUS_*_FMT()` macros which format logged message using `printf()`
formatting string.

~~~~{.cpp}
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>
#include <iomanip>

int
main()
{
    log4cplus::Initializer initializer;

    log4cplus::BasicConfigurator config;
    config.configure();

    log4cplus::Logger logger = log4cplus::Logger::getInstance(
        LOG4CPLUS_TEXT("main"));

    LOG4CPLUS_INFO(logger,
        LOG4CPLUS_TEXT("This is")
        << LOG4CPLUS_TEXT(" a reall")
        << LOG4CPLUS_TEXT("y long message.") << std::endl
        << LOG4CPLUS_TEXT("Just testing it out") << std::endl
        << LOG4CPLUS_TEXT("What do you think?"));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a bool: ") << true);
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a char: ")
        << LOG4CPLUS_TEXT('x'));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a short: ")
        << static_cast<short>(-100));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a unsigned short: ")
        << static_cast<unsigned short>(100));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a int: ") << 1000);
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a unsigned int: ") << 1000U);
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a long(hex): ")
        << std::hex << 100000000L);
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a unsigned long: ")
        << static_cast<unsigned long>(100000000U));
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a float: ") << 1.2345f);
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a double: ")
        << std::setprecision(15)
        << 1.2345234234);
    LOG4CPLUS_INFO(logger, LOG4CPLUS_TEXT("This is a long double: ")
        << std::setprecision(15)
        << 123452342342.342L);

    return 0;
}
~~~~

The code above should just print the expected:

~~~~
INFO - This is a really long message.
Just testing it out
What do you think?
INFO - This is a bool: 1
INFO - This is a char: x
INFO - This is a short: -100
INFO - This is a unsigned short: 100
INFO - This is a int: 1000
INFO - This is a unsigned int: 1000
INFO - This is a long(hex): 5f5e100
INFO - This is a unsigned long: 100000000
INFO - This is a float: 1.2345
INFO - This is a double: 1.2345234234
INFO - This is a long double: 123452342342.342
~~~~

Beside these macros, there are two more groups of logging macros.
`LOG4CPLUS_*_STR()` can be used for logging messages that are just plain
strings that do not need any kind of formatting. There is also group of
`LOG4CPLUS_*_FMT()` macros which format logged message using `printf()`
formatting string.


## Log level

This example shows how log messages can be filtered at run time by adjusting
the _log level threshold_ on `Logger` instance.

~~~~{.cpp}
#include <log4cplus/logger.h>
#include <log4cplus/loglevel.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>
#include <iomanip>

void
printMessages(log4cplus::Logger const & logger)
{
    // Print messages using all common log levels.
    LOG4CPLUS_TRACE (logger, "printMessages()");
    LOG4CPLUS_DEBUG (logger, "This is a DEBUG message");
    LOG4CPLUS_INFO (logger, "This is a INFO message");
    LOG4CPLUS_WARN (logger, "This is a WARN message");
    LOG4CPLUS_ERROR (logger, "This is a ERROR message");
    LOG4CPLUS_FATAL (logger, "This is a FATAL message");
}

void
thresholdTest(log4cplus::LogLevel ll)
{
    log4cplus::Logger logger
        = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

    // Set log level threshold on logger.
    logger.setLogLevel(ll);

    // Print messages.
    log4cplus::tcout
        << LOG4CPLUS_TEXT("*** calling printMessages() with ")
        << log4cplus::getLogLevelManager().toString(ll)
        << LOG4CPLUS_TEXT(" set: ***")
        << std::endl;
    printMessages(logger);
    log4cplus::tcout << std::endl;
}

int
main()
{
    log4cplus::Initializer initializer;

    log4cplus::BasicConfigurator config;
    config.configure();

    thresholdTest(log4cplus::TRACE_LOG_LEVEL);
    thresholdTest(log4cplus::DEBUG_LOG_LEVEL);
    thresholdTest(log4cplus::INFO_LOG_LEVEL);
    thresholdTest(log4cplus::WARN_LOG_LEVEL);
    thresholdTest(log4cplus::ERROR_LOG_LEVEL);
    thresholdTest(log4cplus::FATAL_LOG_LEVEL);

    return 0;
}
~~~~

The code prints fewer and fewer messages as the log level threshold is being
risen.

~~~~
*** calling printMessages() with TRACE set: ***
TRACE - printMessages()
DEBUG - This is a DEBUG message
INFO - This is a INFO message
WARN - This is a WARN message
ERROR - This is a ERROR message
FATAL - This is a FATAL message

*** calling printMessages() with DEBUG set: ***
DEBUG - This is a DEBUG message
INFO - This is a INFO message
WARN - This is a WARN message
ERROR - This is a ERROR message
FATAL - This is a FATAL message

*** calling printMessages() with INFO set: ***
INFO - This is a INFO message
WARN - This is a WARN message
ERROR - This is a ERROR message
FATAL - This is a FATAL message

*** calling printMessages() with WARN set: ***
WARN - This is a WARN message
ERROR - This is a ERROR message
FATAL - This is a FATAL message

*** calling printMessages() with ERROR set: ***
ERROR - This is a ERROR message
FATAL - This is a FATAL message

*** calling printMessages() with FATAL set: ***
FATAL - This is a FATAL message
~~~~

## More examples

See sources in `tests/` directory in [log4cplus] source distribution for more
examples of [log4cplus] usage.

[log4cplus]: https://sourceforge.net/projects/log4cplus/
