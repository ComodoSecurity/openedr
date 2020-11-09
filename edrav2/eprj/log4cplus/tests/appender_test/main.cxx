#include <exception>
#include <iostream>
#include <string>
#include <sstream>
#include "log4cplus/logger.h"
#include "log4cplus/consoleappender.h"
#include "log4cplus/helpers/appenderattachableimpl.h"
#include "log4cplus/helpers/loglog.h"
#include "log4cplus/helpers/pointer.h"
#include "log4cplus/helpers/property.h"
#include "log4cplus/spi/loggingevent.h"
#include "log4cplus/initializer.h"


static void
printAppenderList(const log4cplus::SharedAppenderPtrList& list)
{
    log4cplus::tcout << "List size: " << list.size() << std::endl;
    for (auto & appender : list)
        log4cplus::tcout << "Loop Body: Appender name = " << appender->getName()
                         << std::endl;
}


// This appender does not call destructorImpl(), which is wrong.
class BadDerivedAppender
    : public log4cplus::Appender
{
public:
    virtual void close ()
    { }

    virtual void append (const log4cplus::spi::InternalLoggingEvent&)
    { }
};


int
main()
{
    log4cplus::Initializer initializer;

    log4cplus::helpers::LogLog::getLogLog()->setInternalDebugging(true);
    {
        try {
            log4cplus::SharedAppenderPtr append_1(
                new log4cplus::ConsoleAppender(false, true));
            append_1->setName(LOG4CPLUS_TEXT("First"));

            log4cplus::SharedAppenderPtr append_2(
                new log4cplus::ConsoleAppender);
            append_2->setName(LOG4CPLUS_TEXT("Second"));

            // Test that we get back the same layout as we set.

            log4cplus::Layout * layout_2;
            append_2->setLayout(
                std::unique_ptr<log4cplus::Layout>(
                    layout_2 = new log4cplus::SimpleLayout));
            if (append_2->getLayout () != layout_2)
                return 1;

            // Default error handler should be set.

            if (! append_2->getErrorHandler ())
                return 2;

            // Test warning on NULL handler.

            append_2->setErrorHandler (
                std::unique_ptr<log4cplus::ErrorHandler>());

            // Set working error handler.

            std::unique_ptr<log4cplus::ErrorHandler> errorHandler (
                new log4cplus::OnlyOnceErrorHandler);
            append_2->setErrorHandler (std::move(errorHandler));

            // Test logging through instantiated appenders.

            log4cplus::spi::InternalLoggingEvent event(
                log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("test")).getName(),
                log4cplus::DEBUG_LOG_LEVEL, LOG4CPLUS_TEXT("This is a test..."),
                __FILE__, __LINE__, "main");

            log4cplus::helpers::AppenderAttachableImpl aai;
            aai.addAppender(append_1);
            aai.addAppender(append_2);
            aai.addAppender(append_1);
            aai.addAppender(append_2);
            aai.addAppender(log4cplus::SharedAppenderPtr());
            printAppenderList(aai.getAllAppenders());

            aai.removeAppender(append_2);
            printAppenderList(aai.getAllAppenders());

            aai.removeAppender(LOG4CPLUS_TEXT("First"));
            aai.removeAppender(log4cplus::SharedAppenderPtr());
            printAppenderList(aai.getAllAppenders());

            aai.addAppender(append_1);
            aai.addAppender(append_2);
            aai.addAppender(append_1);
            aai.addAppender(append_2);
            log4cplus::tcout << "Should be First: "
                             << aai.getAppender(LOG4CPLUS_TEXT("First"))->getName()
                             << std::endl;
            log4cplus::tcout << "Should be Second: "
                             << aai.getAppender(LOG4CPLUS_TEXT("Second"))->getName()
                             << std::endl
                             << std::endl;
            append_1->doAppend(event);
            append_2->doAppend(event);

            // Test appending to closed appender error handling.

            append_2->close ();
            append_2->doAppend (event);

            // Test appender's error handling for wrong layout.

            try
            {
                log4cplus::tistringstream propsStream (
                    LOG4CPLUS_TEXT ("layout=log4cplus::WrongLayout"));
                log4cplus::helpers::Properties props (propsStream);
                log4cplus::SharedAppenderPtr append (
                    new log4cplus::ConsoleAppender (props));
                append->setName (LOG4CPLUS_TEXT ("Third"));
            }
            catch (std::exception const & ex)
            {
                log4cplus::tcout << "Got expected exception: "
                                 << LOG4CPLUS_C_STR_TO_TSTRING (ex.what ())
                                 << std::endl;
            }

            // Test threshold parsing.

            {
                log4cplus::tistringstream propsStream (
                    LOG4CPLUS_TEXT ("Threshold=ERROR"));
                log4cplus::helpers::Properties props (propsStream);
                log4cplus::SharedAppenderPtr append (
                    new log4cplus::ConsoleAppender (props));
                append->setName (LOG4CPLUS_TEXT ("Fourth"));
            }

            // Test threshold parsing of wrong log level.

            {
                log4cplus::tistringstream propsStream (
                    LOG4CPLUS_TEXT ("Threshold=WRONG"));
                log4cplus::helpers::Properties props (propsStream);
                log4cplus::SharedAppenderPtr append (
                    new log4cplus::ConsoleAppender (props));
                append->setName (LOG4CPLUS_TEXT ("Fifth"));
            }

            // Test wrong filter parsing.

            try
            {
                log4cplus::tistringstream propsStream (
                    LOG4CPLUS_TEXT ("filters.1=log4cplus::spi::WrongFilter"));
                log4cplus::helpers::Properties props (propsStream);
                log4cplus::SharedAppenderPtr append (
                    new log4cplus::ConsoleAppender (props));
                append->setName (LOG4CPLUS_TEXT ("Sixth"));
            }
            catch (std::exception const & ex)
            {
                log4cplus::tcout << "Got expected exception: "
                                 << LOG4CPLUS_C_STR_TO_TSTRING (ex.what ())
                                 << std::endl;
            }

            // Test error reporting of badly coded appender.

            {
                BadDerivedAppender appender;
                appender.setName (LOG4CPLUS_TEXT ("Seventh"));
            }
        }
        catch(std::exception const & e) {
            log4cplus::tcout << "**** Exception occured: " << e.what()
                             << std::endl;
        }
    }

    log4cplus::tcout << "Exiting main()..." << std::endl;

    return 0;
}
