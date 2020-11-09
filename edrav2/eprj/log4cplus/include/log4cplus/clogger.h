// -*- C -*-
/**
 * Module:  Log4CPLUS
 * File:    clogger.h
 * Created: 01/2011
 * Author:  Jens Rehsack
 *
 *
 * Copyright 2011-2017 Jens Rehsack & Tad E. Smith
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file
 * This header defines the C API for log4cplus and the logging macros. */

#ifndef LOG4CPLUS_CLOGGERHEADER_
#define LOG4CPLUS_CLOGGERHEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif


#ifdef __cplusplus
extern "C"
{
#endif

// TODO UNICDE capable

typedef void * log4cplus_logger_t;
typedef log4cplus_logger_t logger_t;

typedef int log4cplus_loglevel_t;
typedef log4cplus_loglevel_t loglevel_t;

#define L4CP_OFF_LOG_LEVEL 60000
#define L4CP_FATAL_LOG_LEVEL 50000
#define L4CP_ERROR_LOG_LEVEL 40000
#define L4CP_WARN_LOG_LEVEL 30000
#define L4CP_INFO_LOG_LEVEL 20000
#define L4CP_DEBUG_LOG_LEVEL 10000
#define L4CP_TRACE_LOG_LEVEL 0
#define L4CP_ALL_LOG_LEVEL TRACE_LOG_LEVEL
#define L4CP_NOT_SET_LOG_LEVEL -1

#ifdef UNICODE
typedef wchar_t log4cplus_char_t;
#else
typedef char log4cplus_char_t;
#endif // UNICODE

#if ! defined (LOG4CPLUS_TEXT)
#ifdef UNICODE
#  define LOG4CPLUS_TEXT2(STRING) L##STRING
#else
#  define LOG4CPLUS_TEXT2(STRING) STRING
#endif // UNICODE
#define LOG4CPLUS_TEXT(STRING) LOG4CPLUS_TEXT2(STRING)
#endif // LOG4CPLUS_TEXT

LOG4CPLUS_EXPORT void * log4cplus_initialize(void);
LOG4CPLUS_EXPORT int log4cplus_deinitialize(void * initializer);

LOG4CPLUS_EXPORT int log4cplus_file_configure(const log4cplus_char_t *pathname);
LOG4CPLUS_EXPORT int log4cplus_file_reconfigure(const log4cplus_char_t *pathname);
LOG4CPLUS_EXPORT int log4cplus_str_configure(const log4cplus_char_t *config);
LOG4CPLUS_EXPORT int log4cplus_str_reconfigure(const log4cplus_char_t *config);
LOG4CPLUS_EXPORT int log4cplus_basic_configure(void);
LOG4CPLUS_EXPORT int log4cplus_basic_reconfigure(int logToStdErr);
LOG4CPLUS_EXPORT void log4cplus_shutdown(void);

LOG4CPLUS_EXPORT int log4cplus_logger_exists(const log4cplus_char_t *name);
LOG4CPLUS_EXPORT int log4cplus_logger_is_enabled_for(
    const log4cplus_char_t *name, log4cplus_loglevel_t ll);

LOG4CPLUS_EXPORT int log4cplus_logger_log(const log4cplus_char_t *name,
    log4cplus_loglevel_t ll, const log4cplus_char_t *msgfmt, ...)
    LOG4CPLUS_FORMAT_ATTRIBUTE (__printf__, 3, 4);

LOG4CPLUS_EXPORT int log4cplus_logger_log_str(const log4cplus_char_t *name,
    log4cplus_loglevel_t ll, const log4cplus_char_t *msg);

LOG4CPLUS_EXPORT int log4cplus_logger_force_log(const log4cplus_char_t *name,
    log4cplus_loglevel_t ll, const log4cplus_char_t *msgfmt, ...)
    LOG4CPLUS_FORMAT_ATTRIBUTE (__printf__, 3, 4);

LOG4CPLUS_EXPORT int log4cplus_logger_force_log_str(const log4cplus_char_t *name,
    log4cplus_loglevel_t ll, const log4cplus_char_t *msg);

//! CallbackAppender callback type.
typedef void (* log4cplus_log_event_callback_t)(void * cookie,
    log4cplus_char_t const * message, log4cplus_char_t const * loggerName,
    log4cplus_loglevel_t ll, log4cplus_char_t const * thread,
    log4cplus_char_t const * thread2,
    unsigned long long timestamp_secs, unsigned long timestamp_usecs,
    log4cplus_char_t const * file, log4cplus_char_t const * function, int line);

LOG4CPLUS_EXPORT int log4cplus_add_callback_appender(
    const log4cplus_char_t * logger, log4cplus_log_event_callback_t callback,
    void * cookie);

// Custom LogLevel
LOG4CPLUS_EXPORT int log4cplus_add_log_level(unsigned int ll,
    const log4cplus_char_t *ll_name);
LOG4CPLUS_EXPORT int log4cplus_remove_log_level(unsigned int ll,
    const log4cplus_char_t *ll_name);

#ifdef __cplusplus
}
#endif

#endif /*?LOG4CPLUS_CLOGGERHEADER_*/
