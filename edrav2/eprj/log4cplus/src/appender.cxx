// Module:  Log4CPLUS
// File:    appender.cxx
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright 2003-2017 Tad E. Smith
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <log4cplus/appender.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/pointer.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/internal/internal.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <stdexcept>


namespace log4cplus
{


///////////////////////////////////////////////////////////////////////////////
// log4cplus::ErrorHandler dtor
///////////////////////////////////////////////////////////////////////////////

ErrorHandler::ErrorHandler ()
{ }


ErrorHandler::~ErrorHandler()
{ }



///////////////////////////////////////////////////////////////////////////////
// log4cplus::OnlyOnceErrorHandler
///////////////////////////////////////////////////////////////////////////////

OnlyOnceErrorHandler::OnlyOnceErrorHandler()
    : firstTime(true)
{ }


OnlyOnceErrorHandler::~OnlyOnceErrorHandler ()
{ }


void
OnlyOnceErrorHandler::error(const log4cplus::tstring& err)
{
    if(firstTime) {
        helpers::getLogLog().error(err);
        firstTime = false;
    }
}



void
OnlyOnceErrorHandler::reset()
{
    firstTime = true;
}



///////////////////////////////////////////////////////////////////////////////
// log4cplus::Appender ctors
///////////////////////////////////////////////////////////////////////////////

Appender::Appender()
 : layout(new SimpleLayout),
   name(internal::empty_str),
   threshold(NOT_SET_LOG_LEVEL),
   errorHandler(new OnlyOnceErrorHandler),
   useLockFile(false),
   async(false),
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
   in_flight(0),
#endif
   closed(false)
{
}



Appender::Appender(const log4cplus::helpers::Properties & properties)
    : layout(new SimpleLayout)
    , name()
    , threshold(NOT_SET_LOG_LEVEL)
    , errorHandler(new OnlyOnceErrorHandler)
    , useLockFile(false)
    , async(false)
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    , in_flight(0)
#endif
    , closed(false)
{
    if(properties.exists( LOG4CPLUS_TEXT("layout") ))
    {
        log4cplus::tstring const & factoryName
            = properties.getProperty( LOG4CPLUS_TEXT("layout") );
        spi::LayoutFactory* factory
            = spi::getLayoutFactoryRegistry().get(factoryName);
        if (factory == nullptr) {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Cannot find LayoutFactory: \"")
                + factoryName
                + LOG4CPLUS_TEXT("\""), true);
        }

        helpers::Properties layoutProperties =
                properties.getPropertySubset( LOG4CPLUS_TEXT("layout.") );
        try {
            std::unique_ptr<Layout> newLayout(factory->createObject(layoutProperties));
            if (newLayout == nullptr) {
                helpers::getLogLog().error(
                    LOG4CPLUS_TEXT("Failed to create Layout: ")
                    + factoryName, true);
            }
            else {
                layout = std::move(newLayout);
            }
        }
        catch(std::exception const & e) {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Error while creating Layout: ")
                + LOG4CPLUS_C_STR_TO_TSTRING(e.what()), true);
        }

    }

    // Support for appender.Threshold in properties configuration file
    if(properties.exists(LOG4CPLUS_TEXT("Threshold"))) {
        tstring tmp = properties.getProperty(LOG4CPLUS_TEXT("Threshold"));
        tmp = log4cplus::helpers::toUpper(tmp);
        threshold = log4cplus::getLogLevelManager().fromString(tmp);
    }

    // Configure the filters
    helpers::Properties filterProps
        = properties.getPropertySubset( LOG4CPLUS_TEXT("filters.") );
    unsigned filterCount = 0;
    tstring filterName;
    while (filterProps.exists(
        filterName = helpers::convertIntegerToString (++filterCount)))
    {
        tstring const & factoryName = filterProps.getProperty(filterName);
        spi::FilterFactory* factory
            = spi::getFilterFactoryRegistry().get(factoryName);

        if(! factory)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Appender::ctor()- Cannot find FilterFactory: ")
                + factoryName, true);
        }
        spi::FilterPtr tmpFilter = factory->createObject (
            filterProps.getPropertySubset(filterName + LOG4CPLUS_TEXT(".")));
        if (! tmpFilter)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Appender::ctor()- Failed to create filter: ")
                + filterName, true);
        }
        addFilter (std::move (tmpFilter));
    }

    // Deal with file locking settings.
    properties.getBool (useLockFile, LOG4CPLUS_TEXT("UseLockFile"));
    if (useLockFile)
    {
        tstring const & lockFileName
            = properties.getProperty (LOG4CPLUS_TEXT ("LockFile"));
        if (! lockFileName.empty ())
        {
            try
            {
                lockFile.reset (new helpers::LockFile (lockFileName));
            }
            catch (std::runtime_error const &)
            {
                return;
            }
        }
        else
        {
            helpers::getLogLog ().debug (
                LOG4CPLUS_TEXT (
                    "UseLockFile is true but LockFile is not specified"));
        }
    }

    // Deal with asynchronous append flag.
    properties.getBool (async, LOG4CPLUS_TEXT("AsyncAppend"));
}


Appender::~Appender()
{
    helpers::LogLog & loglog = helpers::getLogLog ();

    loglog.debug(LOG4CPLUS_TEXT("Destroying appender named [") + name
        + LOG4CPLUS_TEXT("]."));

    if (! closed)
        loglog.error (
            LOG4CPLUS_TEXT ("Derived Appender did not call destructorImpl()."));
}



///////////////////////////////////////////////////////////////////////////////
// log4cplus::Appender public methods
///////////////////////////////////////////////////////////////////////////////

void
Appender::waitToFinishAsyncLogging()
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    if (async)
    {
        // When async flag is true we might have some logging still in flight
        // on thread pool threads. Wait for them to finish.

        std::unique_lock<std::mutex> lock (in_flight_mutex);
        in_flight_condition.wait (lock,
            [&] { return this->in_flight == 0; });
    }
#endif
}

void
Appender::destructorImpl()
{
    // An appender might be closed then destroyed. There is no point
    // in closing twice. It can actually be a wrong thing to do, e.g.,
    // files get rolled more than once.
    if (closed)
        return;

    waitToFinishAsyncLogging ();

    close();
    closed = true;
}


bool Appender::isClosed() const
{
    return closed;
}


#if ! defined (LOG4CPLUS_SINGLE_THREADED)
void
Appender::subtract_in_flight ()
{
    std::size_t const prev = std::atomic_fetch_sub_explicit (&in_flight,
        std::size_t (1), std::memory_order_acq_rel);
    if (prev == 1)
    {
        std::unique_lock<std::mutex> lock (in_flight_mutex);
        in_flight_condition.notify_all ();
    }
}

#endif


// from global-init.cxx
void enqueueAsyncDoAppend (SharedAppenderPtr const & appender,
    spi::InternalLoggingEvent const & event);


void
Appender::doAppend(const log4cplus::spi::InternalLoggingEvent& event)
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    if (async)
    {
        event.gatherThreadSpecificData ();

        std::atomic_fetch_add_explicit (&in_flight, std::size_t (1),
            std::memory_order_relaxed);

        try
        {
            enqueueAsyncDoAppend (SharedAppenderPtr (this), event);
        }
        catch (...)
        {
            subtract_in_flight ();
            throw;
        }
    }
    else
#endif
        syncDoAppend (event);
}


void
Appender::asyncDoAppend(const log4cplus::spi::InternalLoggingEvent& event)
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    struct handle_in_flight
    {
        Appender * const app;

        explicit
        handle_in_flight (Appender * app_)
            : app (app_)
        { }

        ~handle_in_flight ()
        {
            app->subtract_in_flight ();
        }
    };

    handle_in_flight guard (this);
#endif

    syncDoAppend (event);
}


void
Appender::syncDoAppend(const log4cplus::spi::InternalLoggingEvent& event)
{
    thread::MutexGuard guard (access_mutex);

    if(closed) {
        helpers::getLogLog().error(
            LOG4CPLUS_TEXT("Attempted to append to closed appender named [")
            + name
            + LOG4CPLUS_TEXT("]."));
        return;
    }

    // Check appender's threshold logging level.

    if (! isAsSevereAsThreshold(event.getLogLevel()))
        return;

    // Evaluate filters attached to this appender.

    if (checkFilter(filter.get(), event) == spi::DENY)
        return;

    // Lock system wide lock.

    helpers::LockFileGuard lfguard;
    if (useLockFile && lockFile.get ())
    {
        try
        {
            lfguard.attach_and_lock (*lockFile);
        }
        catch (std::runtime_error const &)
        {
            return;
        }
    }

    // Finally append given event.

    append(event);
}


tstring &
Appender::formatEvent (const spi::InternalLoggingEvent& event) const
{
    internal::appender_sratch_pad & appender_sp = internal::get_appender_sp ();
    detail::clear_tostringstream (appender_sp.oss);
    layout->formatAndAppend(appender_sp.oss, event);
    appender_sp.str = appender_sp.oss.str();
    return appender_sp.str;
}


log4cplus::tstring
Appender::getName()
{
    return name;
}



void
Appender::setName(const log4cplus::tstring& n)
{
    this->name = n;
}


ErrorHandler*
Appender::getErrorHandler()
{
    return errorHandler.get();
}



void
Appender::setErrorHandler(std::unique_ptr<ErrorHandler> eh)
{
    if (! eh.get())
    {
        // We do not throw exception here since the cause is probably a
        // bad config file.
        helpers::getLogLog().warn(
            LOG4CPLUS_TEXT("You have tried to set a null error-handler."));
        return;
    }

    thread::MutexGuard guard (access_mutex);

    this->errorHandler = std::move(eh);
}



void
Appender::setLayout(std::unique_ptr<Layout> lo)
{
    thread::MutexGuard guard (access_mutex);

    this->layout = std::move(lo);
}



Layout*
Appender::getLayout()
{
    thread::MutexGuard guard (access_mutex);

    return layout.get();
}


void
Appender::setFilter(log4cplus::spi::FilterPtr f)
{
    thread::MutexGuard guard (access_mutex);

    filter = std::move (f);
}


log4cplus::spi::FilterPtr
Appender::getFilter() const
{
    thread::MutexGuard guard (access_mutex);

    return filter;
}


void
Appender::addFilter (log4cplus::spi::FilterPtr f)
{
    thread::MutexGuard guard (access_mutex);

    log4cplus::spi::FilterPtr filterChain = getFilter ();
    if (filterChain)
        filterChain->appendFilter (std::move (f));
    else
        filterChain = std::move (f);

    setFilter (filterChain);
}


void
Appender::addFilter (std::function<
    spi::FilterResult (const spi::InternalLoggingEvent &)> filterFunction)
{
    addFilter (
        spi::FilterPtr (new spi::FunctionFilter (std::move (filterFunction))));
}


} // namespace log4cplus
