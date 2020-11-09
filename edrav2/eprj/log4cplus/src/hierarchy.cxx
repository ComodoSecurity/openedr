// Module:  Log4CPLUS
// File:    hierarchy.cxx
// Created: 6/2001
// Author:  Tad E. Smith
//
//
// Copyright 2001-2017 Tad E. Smith
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

#include <log4cplus/hierarchy.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/spi/loggerimpl.h>
#include <log4cplus/spi/rootlogger.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <utility>
#include <limits>


namespace log4cplus
{


//////////////////////////////////////////////////////////////////////////////
// File "Local" methods
//////////////////////////////////////////////////////////////////////////////

namespace
{

static
bool startsWith(tstring const & teststr, tstring const & substr)
{
    bool val = false;
    tstring::size_type const len = substr.length();
    if (teststr.length() > len)
        val = teststr.compare (0, len, substr) == 0;

    return val;
}

} // namespace


//////////////////////////////////////////////////////////////////////////////
// Hierarchy static declarations
//////////////////////////////////////////////////////////////////////////////

const LogLevel Hierarchy::DISABLE_OFF = -1;
const LogLevel Hierarchy::DISABLE_OVERRIDE = -2;



//////////////////////////////////////////////////////////////////////////////
// Hierarchy ctor and dtor
//////////////////////////////////////////////////////////////////////////////

Hierarchy::Hierarchy()
  : defaultFactory(new DefaultLoggerFactory())
  , root(nullptr)
  // Don't disable any LogLevel level by default.
  , disableValue(DISABLE_OFF)
  , emittedNoAppenderWarning(false)
{
    root = Logger( new spi::RootLogger(*this, DEBUG_LOG_LEVEL) );
}


Hierarchy::~Hierarchy()
{
    shutdown();
}



//////////////////////////////////////////////////////////////////////////////
// Hierarchy public methods
//////////////////////////////////////////////////////////////////////////////

void
Hierarchy::clear()
{
    thread::MutexGuard guard (hashtable_mutex);

    provisionNodes.erase(provisionNodes.begin(), provisionNodes.end());
    loggerPtrs.erase(loggerPtrs.begin(), loggerPtrs.end());
}


bool
Hierarchy::exists(const tstring& name)
{
    // Root logger always does exist.
    if (name.empty ())
        return true;

    thread::MutexGuard guard (hashtable_mutex);

    auto it = loggerPtrs.find(name);
    return it != loggerPtrs.end();
}


void
Hierarchy::disable(const tstring& loglevelStr)
{
    if(disableValue != DISABLE_OVERRIDE) {
        disableValue = getLogLevelManager().fromString(loglevelStr);
    }
}


void
Hierarchy::disable(LogLevel ll)
{
    if(disableValue != DISABLE_OVERRIDE) {
        disableValue = ll;
    }
}


void
Hierarchy::disableAll()
{
    disable((std::numeric_limits<LogLevel>::max) ());
}


void
Hierarchy::disableDebug()
{
    disable(DEBUG_LOG_LEVEL);
}


void
Hierarchy::disableInfo()
{
    disable(INFO_LOG_LEVEL);
}


void
Hierarchy::enableAll()
{
    disableValue = DISABLE_OFF;
}


Logger
Hierarchy::getInstance(const tstring& name)
{
    return getInstance(name, *defaultFactory);
}


Logger
Hierarchy::getInstance(const tstring& name, spi::LoggerFactory& factory)
{
    thread::MutexGuard guard (hashtable_mutex);

    return getInstanceImpl(name, factory);
}


LoggerList
Hierarchy::getCurrentLoggers()
{
    LoggerList ret;

    {
        thread::MutexGuard guard (hashtable_mutex);
        initializeLoggerList(ret);
    }

    return ret;
}


bool
Hierarchy::isDisabled(LogLevel level)
{
    return disableValue >= level;
}


Logger
Hierarchy::getRoot() const
{
    return root;
}


void
Hierarchy::resetConfiguration()
{
    getRoot().setLogLevel(DEBUG_LOG_LEVEL);
    disableValue = DISABLE_OFF;

    shutdown();

    LoggerList loggers = getCurrentLoggers();
    for (auto & logger : loggers)
    {
        logger.setLogLevel(NOT_SET_LOG_LEVEL);
        logger.setAdditivity(true);
    }

}


void
Hierarchy::setLoggerFactory(std::unique_ptr<spi::LoggerFactory> factory)
{
    defaultFactory = std::move(factory);
}


spi::LoggerFactory *
Hierarchy::getLoggerFactory()
{
    return defaultFactory.get();
}


// from global-init.cxx
void waitUntilEmptyThreadPoolQueue ();

void
Hierarchy::shutdown()
{
    waitUntilEmptyThreadPoolQueue ();

    LoggerList loggers;
    initializeLoggerList (loggers);

    // begin by closing nested appenders
    // then, remove all appenders

    for (auto & appenderPtr : root.getAllAppenders())
    {
        Appender & appender = *appenderPtr;
        appender.waitToFinishAsyncLogging ();
    }
    root.closeNestedAppenders();
    root.removeAllAppenders();

    // repeat
    for (auto & logger : loggers)
    {
        for (auto & appenderPtr : logger.getAllAppenders())
        {
            Appender & appender = *appenderPtr;
            appender.waitToFinishAsyncLogging ();
        }
        logger.closeNestedAppenders();
        logger.removeAllAppenders();
    }
}



//////////////////////////////////////////////////////////////////////////////
// Hierarchy private methods
//////////////////////////////////////////////////////////////////////////////

Logger
Hierarchy::getInstanceImpl(const tstring& name, spi::LoggerFactory& factory)
{
    Logger logger;
    LoggerMap::iterator lm_it;

    if (name.empty ())
        logger = root;
    else if ((lm_it = loggerPtrs.find(name)) != loggerPtrs.end())
        logger = lm_it->second;
    else
    {
        // Need to create a new logger
        logger = factory.makeNewLoggerInstance(name, *this);
        bool inserted = loggerPtrs.emplace (name, logger).second;
        if (! inserted)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("Hierarchy::getInstanceImpl()- Insert failed"),
                true);
        }

        auto pnm_it = provisionNodes.find(name);
        if (pnm_it != provisionNodes.end())
        {
            updateChildren(pnm_it->second, logger);
            bool deleted = (provisionNodes.erase(name) > 0);
            if (! deleted)
            {
                helpers::getLogLog().error(
                    LOG4CPLUS_TEXT("Hierarchy::getInstanceImpl()- Delete failed"),
                    true);
            }
        }
        updateParents(logger);
    }

    return logger;
}


void
Hierarchy::initializeLoggerList(LoggerList& list) const
{
    list.reserve (list.size () + loggerPtrs.size ());
    for (auto & kv : loggerPtrs)
        list.push_back(kv.second);
}


void
Hierarchy::updateParents(Logger const & logger)
{
    tstring const & name = logger.getName();
    auto const length = name.length();
    bool parentFound = false;
    tstring substr;

    // if name = "w.x.y.z", loop thourgh "w.x.y", "w.x" and "w", but not "w.x.y.z"
    for(std::size_t i=name.find_last_of(LOG4CPLUS_TEXT('.'), length-1);
        i != tstring::npos && i > 0;
        i = name.find_last_of(LOG4CPLUS_TEXT('.'), i-1))
    {
        substr.assign (name, 0, i);

        auto it = loggerPtrs.find(substr);
        if(it != loggerPtrs.end()) {
            parentFound = true;
            logger.value->parent = it->second.value;
            break;  // no need to update the ancestors of the closest ancestor
        }
        else {
            auto it2 = provisionNodes.find(substr);
            if(it2 != provisionNodes.end()) {
                it2->second.push_back(logger);
            }
            else {
                ProvisionNode node;
                node.push_back(logger);
                std::pair<ProvisionNodeMap::iterator, bool> tmp =
                    provisionNodes.emplace (substr, node);
                if(!tmp.second) {
                    helpers::getLogLog().error(
                        LOG4CPLUS_TEXT("Hierarchy::updateParents()- Insert failed"),
                        true);
                }
            }
        } // end if Logger found
    } // end for loop

    if(!parentFound) {
        logger.value->parent = root.value;
    }
}


void
Hierarchy::updateChildren(ProvisionNode& pn, Logger const & logger)
{
    for (Logger & c : pn)
    {
        // Unless this child already points to a correct (lower) parent,
        // make logger.parent point to c.parent and c.parent to logger.
        if( !startsWith(c.value->parent->getName(), logger.getName()) ) {
            logger.value->parent = c.value->parent;
            c.value->parent = logger.value;
        }
    }
}


} // namespace log4cplus
