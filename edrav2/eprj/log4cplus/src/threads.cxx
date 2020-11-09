// Module:  Log4CPLUS
// File:    threads.cxx
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

#include <log4cplus/config.hxx>

#include <exception>
#include <ostream>
#include <cerrno>
#include <iomanip>

#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef LOG4CPLUS_HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#ifdef LOG4CPLUS_HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef LOG4CPLUS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(LOG4CPLUS_USE_PTHREADS)
#  include <pthread.h>
#  include <sched.h>
#  include <signal.h>
#elif defined (LOG4CPLUS_USE_WIN32_THREADS)
#  include <process.h>
#endif
#include <log4cplus/config/windowsh-inc.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/tstring.h>
#include <log4cplus/internal/cygwin-win32.h>
#include <log4cplus/streams.h>

#ifndef LOG4CPLUS_SINGLE_THREADED

#include <log4cplus/thread/threads.h>
#include <log4cplus/thread/impl/threads-impl.h>
#include <log4cplus/thread/impl/tls.h>
#include <log4cplus/ndc.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/internal/internal.h>

#endif // LOG4CPLUS_SINGLE_THREADED


namespace log4cplus { namespace thread {

LOG4CPLUS_EXPORT
void
blockAllSignals()
{
#if defined (LOG4CPLUS_USE_PTHREADS)
    // Block all signals.
    sigset_t signal_set;
    sigfillset (&signal_set);
    pthread_sigmask (SIG_BLOCK, &signal_set, nullptr);
#endif
}


LOG4CPLUS_EXPORT
void
yield()
{
#if defined(LOG4CPLUS_USE_PTHREADS)
    sched_yield();
#elif defined(_WIN32)
    if (! SwitchToThread ())
        Sleep (0);
#endif
}

#if defined(LOG4CPLUS_SINGLE_THREADED)
static log4cplus::tstring thread_name
    LOG4CPLUS_INIT_PRIORITY (LOG4CPLUS_INIT_PRIORITY_BASE - 1)
    (LOG4CPLUS_TEXT("single"));
static log4cplus::tstring thread_name2
    LOG4CPLUS_INIT_PRIORITY (LOG4CPLUS_INIT_PRIORITY_BASE - 1)
    (thread_name);
#endif

LOG4CPLUS_EXPORT
log4cplus::tstring const &
getCurrentThreadName()
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    log4cplus::tstring & name = log4cplus::internal::get_thread_name_str ();
    if (LOG4CPLUS_UNLIKELY (name.empty ()))
    {
        log4cplus::tostringstream tmp;
        tmp << std::setfill(LOG4CPLUS_TEXT('0')) << std::setw(8) << std::hex << impl::getCurrentThreadId ();
        name = tmp.str ();
    }
#else
    log4cplus::tstring & name = thread_name;
    if (LOG4CPLUS_UNLIKELY(name.empty()))
    {
        name = LOG4CPLUS_TEXT("single");
    }
#endif

    return name;
}


namespace
{


static
bool
get_current_thread_name_alt (log4cplus::tostream * s)
{
    log4cplus::tostream & os = *s;

#if defined (LOG4CPLUS_USE_PTHREADS) && defined (__linux__) \
    && defined (LOG4CPLUS_HAVE_GETTID)
    pid_t tid = syscall (SYS_gettid);
    os << tid;

#elif defined (__CYGWIN__)
    unsigned long tid = cygwin::get_current_win32_thread_id ();
    os << tid;

#else
    os << getCurrentThreadName ();

#endif

    return true;
}


} // namespace


LOG4CPLUS_EXPORT
log4cplus::tstring const &
getCurrentThreadName2()
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    log4cplus::tstring & name = log4cplus::internal::get_thread_name2_str ();
    if (LOG4CPLUS_UNLIKELY (name.empty ()))
    {
        log4cplus::tostringstream tmp;
        get_current_thread_name_alt (&tmp);
        name = tmp.str ();
    }

#else
    log4cplus::tstring & name = thread_name2;
    if (LOG4CPLUS_UNLIKELY(name.empty()))
    {
        name = getCurrentThreadName();
    }

#endif

    return name;
}

LOG4CPLUS_EXPORT void setCurrentThreadName(const log4cplus::tstring & name)
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    log4cplus::internal::get_thread_name_str() = name;
#else
    thread_name = name;
#endif
}

LOG4CPLUS_EXPORT void setCurrentThreadName2(const log4cplus::tstring & name)
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    log4cplus::internal::get_thread_name2_str() = name;
#else
    thread_name2 = name;
#endif
}


#ifndef LOG4CPLUS_SINGLE_THREADED

//
//
//

AbstractThread::AbstractThread ()
    : flags (0)
{ }


bool
AbstractThread::isRunning() const
{
    return (flags & fRUNNING) != 0;
}


void
AbstractThread::start()
{
    try
    {
        flags |= fRUNNING;
        thread.reset (
            new std::thread ([this] (AbstractThreadPtr const & thread_ptr) {
                    (void) thread_ptr;
                    blockAllSignals ();
                    helpers::LogLog & loglog = helpers::getLogLog();
                    try
                    {
                        this->run ();
                    }
                    catch (std::exception const & e)
                    {
                        tstring err (LOG4CPLUS_TEXT ("threadStartFunc()")
                            LOG4CPLUS_TEXT ("- run() terminated with an exception: "));
                        err += LOG4CPLUS_C_STR_TO_TSTRING(e.what());
                        loglog.warn(err);
                    }
                    catch(...)
                    {
                        loglog.warn(LOG4CPLUS_TEXT("threadStartFunc()")
                            LOG4CPLUS_TEXT ("- run() terminated with an exception."));
                    }
                    this->flags &= ~fRUNNING;
                    threadCleanup ();
                }, AbstractThreadPtr (this)));
    }
    catch (...)
    {
        flags &= ~fRUNNING;
        throw;
    }
}


void
AbstractThread::join () const
{
    if (! thread
        || (flags & fJOINED) == fJOINED)
        throw std::logic_error ("this thread is not running");

    thread->join ();
    flags |= +fJOINED;
}


AbstractThread::~AbstractThread()
{
    if ((flags & fJOINED) == 0)
        thread->detach ();
}

#endif // LOG4CPLUS_SINGLE_THREADED


} } // namespace log4cplus { namespace thread {
