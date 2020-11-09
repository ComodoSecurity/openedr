//  Copyright (C) 2009-2017, Vaclav Haisman. All rights reserved.
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

#include <log4cplus/config.hxx>
#ifndef LOG4CPLUS_SINGLE_THREADED

#include <log4cplus/asyncappender.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/thread/syncprims-pub-impl.h>


namespace log4cplus
{


namespace
{


class QueueThread
    : public thread::AbstractThread
{
public:
    QueueThread (AsyncAppenderPtr, thread::QueuePtr);

    void run() override;

private:
    AsyncAppenderPtr appenders;
    thread::QueuePtr queue;
};


QueueThread::QueueThread (AsyncAppenderPtr aai, thread::QueuePtr q)
    : appenders (std::move (aai))
    , queue (std::move (q))
{ }


void
QueueThread::run()
{
    typedef log4cplus::thread::Queue::queue_storage_type ev_buf_type;
    ev_buf_type ev_buf;

    while (true)
    {
        unsigned qflags = queue->get_events (&ev_buf);
        if (qflags & thread::Queue::EVENT)
        {
            ev_buf_type::const_iterator const ev_buf_end = ev_buf.end ();
            for (ev_buf_type::const_iterator it = ev_buf.begin ();
                it != ev_buf_end; ++it)
                appenders->appendLoopOnAppenders (*it);
        }

        if (((thread::Queue::EXIT | thread::Queue::DRAIN
                | thread::Queue::EVENT) & qflags)
            == (thread::Queue::EXIT | thread::Queue::DRAIN
                | thread::Queue::EVENT))
            continue;
        else if (thread::Queue::EXIT & qflags)
            break;
    }
}


} // namespace


AsyncAppender::AsyncAppender (SharedAppenderPtr const & app,
    unsigned queue_len)
{
    addAppender (app);
    init_queue_thread (queue_len);
}


AsyncAppender::AsyncAppender (helpers::Properties const & props)
    : Appender (props)
{
    tstring const & appender_name (
        props.getProperty (LOG4CPLUS_TEXT ("Appender")));
    if (appender_name.empty ())
    {
        getErrorHandler ()->error (
            LOG4CPLUS_TEXT ("Unspecified appender for AsyncAppender."));
        return;
    }

    spi::AppenderFactoryRegistry & appender_registry
        = spi::getAppenderFactoryRegistry ();
    spi::AppenderFactory * factory = appender_registry.get (appender_name);
    if (! factory)
    {
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("AsyncAppender::AsyncAppender()")
            LOG4CPLUS_TEXT (" - Cannot find AppenderFactory: ")
            + appender_name, true);
    }

    helpers::Properties appender_props = props.getPropertySubset (
        LOG4CPLUS_TEXT ("Appender."));
    addAppender (factory->createObject (appender_props));

    unsigned queue_len = 100;
    props.getUInt (queue_len, LOG4CPLUS_TEXT ("QueueLimit"));

    init_queue_thread (queue_len);
}


AsyncAppender::~AsyncAppender ()
{
    destructorImpl ();
}


void
AsyncAppender::init_queue_thread (unsigned queue_len)
{
    queue = new thread::Queue (queue_len);
    queue_thread = new QueueThread (AsyncAppenderPtr (this), queue);
    queue_thread->start ();
    helpers::getLogLog ().debug (LOG4CPLUS_TEXT("Queue thread started."));
}


void
AsyncAppender::close ()
{
    if (queue)
    {
        unsigned ret = queue->signal_exit ();
        if (ret & (thread::Queue::ERROR_BIT | thread::Queue::ERROR_AFTER))
            getErrorHandler ()->error (
                LOG4CPLUS_TEXT ("Error in AsyncAppender::close"));
    }

    if (queue_thread && queue_thread->isRunning ())
        queue_thread->join ();

    removeAllAppenders();

    queue_thread = nullptr;
    queue = nullptr;
}


void
AsyncAppender::append (spi::InternalLoggingEvent const & ev)
{
    if (queue_thread && queue_thread->isRunning ())
    {
        unsigned ret = queue->put_event (ev);
        if (ret & (thread::Queue::ERROR_BIT | thread::Queue::ERROR_AFTER))
        {
            getErrorHandler ()->error (
                LOG4CPLUS_TEXT ("Error in AsyncAppender::append,")
                LOG4CPLUS_TEXT (" event queue has been lost."));
            // Exit the queue consumer thread without draining
            // the events queue.
            queue->signal_exit (false);
            queue_thread->join ();
            queue_thread = nullptr;
            queue = nullptr;
            appendLoopOnAppenders (ev);
        }
    }
    else
    {
        // If the thread has died for any reason, fall back to synchronous
        // operation.
        appendLoopOnAppenders (ev);
    }
}


} // namespace log4cplus


#endif // #ifndef LOG4CPLUS_SINGLE_THREADED
