// -*- C++ -*-
//  Copyright (C) 2015-2017, Vaclav Haisman. All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modifica-
//  tion, are permitted provided that the following conditions are met:
//
//  1. Redistributions of  source code must  retain the above copyright  notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS  FOR A PARTICULAR  PURPOSE ARE  DISCLAIMED.  IN NO  EVENT SHALL  THE
//  APACHE SOFTWARE  FOUNDATION  OR ITS CONTRIBUTORS  BE LIABLE FOR  ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLU-
//  DING, BUT NOT LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS
//  OF USE, DATA, OR  PROFITS; OR BUSINESS  INTERRUPTION)  HOWEVER CAUSED AND ON
//  ANY  THEORY OF LIABILITY,  WHETHER  IN CONTRACT,  STRICT LIABILITY,  OR TORT
//  (INCLUDING  NEGLIGENCE OR  OTHERWISE) ARISING IN  ANY WAY OUT OF THE  USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


/** @file */

#ifndef LOG4CPLUS_CALLBACK_APPENDER_HEADER_
#define LOG4CPLUS_CALLBACK_APPENDER_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/appender.h>
#include <log4cplus/clogger.h>


namespace log4cplus {

/**
* Send log events to a C function callback.
*/
class LOG4CPLUS_EXPORT CallbackAppender
    : public Appender {
public:
    CallbackAppender();
    CallbackAppender(log4cplus_log_event_callback_t callback, void * cookie);
    CallbackAppender(const log4cplus::helpers::Properties&);

    virtual ~CallbackAppender();
    virtual void close();

    void setCookie(void *);
    void setCallback(log4cplus_log_event_callback_t);

protected:
    virtual void append(const log4cplus::spi::InternalLoggingEvent& event);

private:
    log4cplus_log_event_callback_t callback;
    void * cookie;

    // Disallow copying of instances of this class
    CallbackAppender(const CallbackAppender&) = delete;
    CallbackAppender& operator=(const CallbackAppender&) = delete;
};

} // end namespace log4cplus

#endif // LOG4CPLUS_CALLBACK_APPENDER_HEADER_
