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

#include <log4cplus/callbackappender.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/thread/syncprims-pub-impl.h>


namespace log4cplus
{


CallbackAppender::CallbackAppender()
{ }


CallbackAppender::CallbackAppender(log4cplus_log_event_callback_t callback_,
    void * cookie_)
    : callback (callback_)
    , cookie (cookie_)
{ }


CallbackAppender::CallbackAppender(const helpers::Properties& properties)
    : Appender(properties)
{ }


CallbackAppender::~CallbackAppender()
{
    destructorImpl();
}


void
CallbackAppender::close()
{
}


void
CallbackAppender::append(const spi::InternalLoggingEvent& ev)
{
    if (callback)
    {
        helpers::Time const & t = ev.getTimestamp();
        callback(cookie, ev.getMessage().c_str(),
            ev.getLoggerName().c_str(), ev.getLogLevel(),
            ev.getThread().c_str(), ev.getThread2().c_str(),
            static_cast<unsigned long long>(helpers::to_time_t(t)),
            static_cast<unsigned long>(helpers::microseconds_part(t)),
            ev.getFile().c_str(),
            ev.getFunction().c_str(), ev.getLine());
    }
}


} // namespace log4cplus
