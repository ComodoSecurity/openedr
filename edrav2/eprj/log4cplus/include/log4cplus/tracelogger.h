// -*- C++ -*-
// Module:  Log4CPLUS
// File:    tracelogger.h
// Created: 1/2009
// Author:  Vaclav Haisman
//
//
// Copyright 2009-2017 Tad E. Smith
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

/** @file */

#ifndef LOG4CPLUS_TRACELOGGER_H
#define LOG4CPLUS_TRACELOGGER_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/logger.h>


namespace log4cplus
{


/**
 * This class is used to produce "Trace" logging.  When an instance of
 * this class is created, it will log a <code>"ENTER: " + msg</code>
 * log message if TRACE_LOG_LEVEL is enabled for <code>logger</code>.
 * When an instance of this class is destroyed, it will log a
 * <code>"ENTER: " + msg</code> log message if TRACE_LOG_LEVEL is enabled
 * for <code>logger</code>.
 * <p>
 * @see LOG4CPLUS_TRACE
 */
class TraceLogger
{
public:
    TraceLogger(Logger l, log4cplus::tstring _msg,
        const char* _file = LOG4CPLUS_CALLER_FILE (),
        int _line = LOG4CPLUS_CALLER_LINE (),
        char const * _function = LOG4CPLUS_CALLER_FUNCTION ())
        : logger(std::move (l)), msg(std::move (_msg)), file(_file),
          function(_function), line(_line)
    {
        if(logger.isEnabledFor(TRACE_LOG_LEVEL))
            logger.forcedLog(TRACE_LOG_LEVEL, LOG4CPLUS_TEXT("ENTER: ") + msg,
                file, line, function);
    }

    ~TraceLogger()
    {
        if(logger.isEnabledFor(TRACE_LOG_LEVEL))
            logger.forcedLog(TRACE_LOG_LEVEL, LOG4CPLUS_TEXT("EXIT:  ") + msg,
                file, line, function);
    }

private:
    TraceLogger (TraceLogger const &);
    TraceLogger & operator = (TraceLogger const &);

    Logger logger;
    log4cplus::tstring msg;
    const char* file;
    const char* function;
    int line;
};


} // log4cplus


#endif // LOG4CPLUS_TRACELOGGER_H
