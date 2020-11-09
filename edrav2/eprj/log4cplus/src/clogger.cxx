// Module:  Log4CPLUS
// File:    clogger.cxx
// Created: 01/2011
// Author:  Jens Rehsack
//
//
// Copyright 2011-2017 Jens Rehsack & Tad E. Smith
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

#include <log4cplus/logger.h>
#include <log4cplus/clogger.h>
#include <log4cplus/appender.h>
#include <log4cplus/hierarchylocker.h>
#include <log4cplus/hierarchy.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/spi/loggerimpl.h>
#include <log4cplus/configurator.h>
#include <log4cplus/streams.h>
#include <log4cplus/helpers/snprintf.h>
#include <log4cplus/initializer.h>
#include <log4cplus/callbackappender.h>
#include <log4cplus/internal/internal.h>
#include <log4cplus/internal/customloglevelmanager.h>

#include <cerrno>
#include <cstdio>
#include <cstdarg>
#include <cstring>

#include <sstream>
#include <map>

using namespace log4cplus;
using namespace log4cplus::helpers;


LOG4CPLUS_EXPORT void *
log4cplus_initialize(void)
{
    try
    {
        return new Initializer();
    }
    catch (std::exception const &)
    {
        return 0;
    }
}


LOG4CPLUS_EXPORT int
log4cplus_deinitialize(void * initializer_)
{
    if (!initializer_)
        return EINVAL;

    try
    {
        Initializer * initializer = static_cast<Initializer *>(initializer_);
        delete initializer;
    }
    catch (std::exception const &)
    {
        return -1;
    }

    return 0;
}


LOG4CPLUS_EXPORT int
log4cplus_file_configure(const log4cplus_char_t *pathname)
{
    if( !pathname )
        return EINVAL;

    try
    {
        PropertyConfigurator::doConfigure( pathname );
    }
    catch(std::exception const &)
    {
        return -1;
    }

    return 0;
}

LOG4CPLUS_EXPORT int
log4cplus_file_reconfigure(const log4cplus_char_t *pathname)
{
    if( !pathname )
        return EINVAL;

    try
    {
        // lock the DefaultHierarchy
        HierarchyLocker theLock(Logger::getDefaultHierarchy());

        // reconfigure the DefaultHierarchy
        theLock.resetConfiguration();
        PropertyConfigurator::doConfigure( pathname );
    }
    catch(std::exception const &)
    {
        return -1;
    }

    return 0;
}

LOG4CPLUS_EXPORT int
log4cplus_str_configure(const log4cplus_char_t *config)
{
    if( !config )
        return EINVAL;

    try
    {
        tstring s(config);
        tistringstream iss(s);
        PropertyConfigurator pc(iss);
        pc.configure();
    }
    catch(std::exception const &)
    {
        return -1;
    }

    return 0;
}

LOG4CPLUS_EXPORT int
log4cplus_str_reconfigure(const log4cplus_char_t *config)
{
    if( !config )
        return EINVAL;

    try
    {
        tstring s(config);
        tistringstream iss(s);
        HierarchyLocker theLock(Logger::getDefaultHierarchy());

        // reconfigure the DefaultHierarchy
        theLock.resetConfiguration();
        PropertyConfigurator pc(iss);
        pc.configure();
    }
    catch(std::exception const &)
    {
        return -1;
    }

    return 0;
}

LOG4CPLUS_EXPORT int
log4cplus_basic_configure(void)
{
    try
    {
        BasicConfigurator::doConfigure();
    }
    catch(std::exception const &)
    {
        return -1;
    }

    return 0;
}

LOG4CPLUS_EXPORT int
log4cplus_basic_reconfigure(int logToStdErr)
{
    try
    {
        HierarchyLocker theLock(Logger::getDefaultHierarchy());

        // reconfigure the DefaultHierarchy
        theLock.resetConfiguration();
        BasicConfigurator::doConfigure(Logger::getDefaultHierarchy(), logToStdErr);
    }
    catch(std::exception const &)
    {
        return -1;
    }

    return 0;
}

LOG4CPLUS_EXPORT void
log4cplus_shutdown(void)
{
    Logger::shutdown();
}


LOG4CPLUS_EXPORT int
log4cplus_add_callback_appender(const log4cplus_char_t * logger_name,
    log4cplus_log_event_callback_t callback, void * cookie)
{
    try
    {
        Logger logger = logger_name
            ? Logger::getInstance(logger_name)
            : Logger::getRoot();
        SharedAppenderPtr appender(new CallbackAppender(callback, cookie));
        logger.addAppender(appender);
    }
    catch (std::exception const &)
    {
        return -1;
    }

    return 0;
}


LOG4CPLUS_EXPORT int
log4cplus_logger_exists(const log4cplus_char_t *name)
{
    int retval = false;

    try
    {
        retval = Logger::exists(name);
    }
    catch(std::exception const &)
    {
        // Fall through.
    }

    return retval;
}

LOG4CPLUS_EXPORT int
log4cplus_logger_is_enabled_for(const log4cplus_char_t *name, loglevel_t ll)
{
    int retval = false;

    try
    {
        Logger logger = name ? Logger::getInstance(name) : Logger::getRoot();
        retval = logger.isEnabledFor(ll);
    }
    catch(std::exception const &)
    {
        // Fall through.
    }

    return retval;
}

LOG4CPLUS_EXPORT int
log4cplus_logger_log(const log4cplus_char_t *name, loglevel_t ll,
    const log4cplus_char_t *msgfmt, ...)
{
    int retval = -1;

    try
    {
        Logger logger = name ? Logger::getInstance(name) : Logger::getRoot();

        if( logger.isEnabledFor(ll) )
        {
            const tchar * msg = nullptr;
            snprintf_buf buf;
            std::va_list ap;

            do
            {
                va_start(ap, msgfmt);
                retval = buf.print_va_list(msg, msgfmt, ap);
                va_end(ap);
            }
            while (retval == -1);

            logger.forcedLog(ll, msg, 0, -1);
        }

        retval = 0;
    }
    catch(std::exception const &)
    {
        // Fall through.
    }

    return retval;
}


LOG4CPLUS_EXPORT int
log4cplus_logger_log_str(const log4cplus_char_t *name,
    log4cplus_loglevel_t ll, const log4cplus_char_t *msg)
{
    int retval = -1;

    try
    {
        Logger logger = name ? Logger::getInstance(name) : Logger::getRoot();

        if (logger.isEnabledFor(ll))
        {
            logger.forcedLog(ll, msg, 0, -1);
        }

        retval = 0;
    }
    catch (std::exception const &)
    {
        // Fall through.
    }

    return retval;
}


LOG4CPLUS_EXPORT int
log4cplus_logger_force_log(const log4cplus_char_t *name, loglevel_t ll,
    const log4cplus_char_t *msgfmt, ...)
{
    int retval = -1;

    try
    {
        Logger logger = name ? Logger::getInstance(name) : Logger::getRoot();
        const tchar * msg = nullptr;
        snprintf_buf buf;
        std::va_list ap;

        do
        {
            va_start(ap, msgfmt);
            retval = buf.print_va_list(msg, msgfmt, ap);
            va_end(ap);
        }
        while (retval == -1);

        logger.forcedLog(ll, msg, 0, -1);

        retval = 0;
    }
    catch(std::exception const &)
    {
        // Fall through.
    }

    return retval;
}


LOG4CPLUS_EXPORT int
log4cplus_logger_force_log_str(const log4cplus_char_t *name, loglevel_t ll,
    const log4cplus_char_t *msg)
{
    int retval = -1;

    try
    {
        Logger logger = name ? Logger::getInstance(name) : Logger::getRoot();
        logger.forcedLog(ll, msg, 0, -1);
        retval = 0;
    }
    catch (std::exception const &)
    {
        // Fall through.
    }

    return retval;
}


namespace log4cplus {

namespace internal {

tstring const &
CustomLogLevelManager::customToStringMethod(LogLevel ll)
{
    CustomLogLevelManager & customLogLevelManager = getCustomLogLevelManager ();
    return customLogLevelManager.customToStringMethodWorker(ll);
}

LogLevel
CustomLogLevelManager::customFromStringMethod(const tstring& nm)
{
    CustomLogLevelManager & customLogLevelManager = getCustomLogLevelManager ();
    return customLogLevelManager.customFromStringMethodWorker(nm);
}

} // namespace internal

} // namespace log4cplus


LOG4CPLUS_EXPORT int
log4cplus_add_log_level(unsigned int ll, const log4cplus_char_t *ll_name)
{
    if(ll == 0 || !ll_name)
        return EINVAL;

    tstring nm(ll_name);
    if( log4cplus::internal::getCustomLogLevelManager ().add(ll, nm) == true )
        return 0;

    return -1;
}

LOG4CPLUS_EXPORT int
log4cplus_remove_log_level(unsigned int ll, const log4cplus_char_t *ll_name)
{
    if(ll == 0 || !ll_name)
        return EINVAL;

    tstring nm(ll_name);
    if( log4cplus::internal::getCustomLogLevelManager ().remove(ll, nm) == true )
        return 0;

    return -1;
}
