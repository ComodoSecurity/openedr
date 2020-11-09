// -*- C++ -*-
//  Copyright (C) 2010-2017, Vaclav Haisman. All rights reserved.
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

#ifndef LOG4CPLUS_THREAD_SYNCPRIMS_PUB_IMPL_H
#define LOG4CPLUS_THREAD_SYNCPRIMS_PUB_IMPL_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <algorithm>

#if (defined (LOG4CPLUS_INLINES_ARE_EXPORTED)           \
    && defined (LOG4CPLUS_BUILD_DLL))                   \
    || defined (LOG4CPLUS_ENABLE_SYNCPRIMS_PUB_IMPL)
#include <log4cplus/thread/syncprims.h>

#if ! defined (LOG4CPLUS_SINGLE_THREADED)
#  include <log4cplus/thread/impl/syncprims-impl.h>
#endif

#define LOG4CPLUS_THROW_RTE(msg) \
    do { log4cplus::thread::impl::syncprims_throw_exception (msg, __FILE__, \
            __LINE__); } while (0)

namespace log4cplus { namespace thread {

namespace impl
{

LOG4CPLUS_EXPORT void LOG4CPLUS_ATTRIBUTE_NORETURN
    syncprims_throw_exception(char const * const msg,
    char const * const file, int line);

}

//
//
//

LOG4CPLUS_INLINE_EXPORT
Mutex::Mutex ()
    LOG4CPLUS_THREADED (: mtx ())
{ }


LOG4CPLUS_INLINE_EXPORT
Mutex::~Mutex ()
{ }


LOG4CPLUS_INLINE_EXPORT
void
Mutex::lock () const
{
    LOG4CPLUS_THREADED (mtx.lock ());
}


LOG4CPLUS_INLINE_EXPORT
void
Mutex::unlock () const
{
    LOG4CPLUS_THREADED (mtx.unlock ());
}


//
//
//

LOG4CPLUS_INLINE_EXPORT
Semaphore::Semaphore (unsigned LOG4CPLUS_THREADED (max_),
    unsigned LOG4CPLUS_THREADED (initial))
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    : max (max_)
    , val ((std::min) (max, initial))
#endif
{ }


LOG4CPLUS_INLINE_EXPORT
Semaphore::~Semaphore ()
{ }


LOG4CPLUS_INLINE_EXPORT
void
Semaphore::unlock () const
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::lock_guard<std::mutex> guard (mtx);

    if (val >= max)
        LOG4CPLUS_THROW_RTE ("Semaphore::unlock(): val >= max");

    ++val;
    cv.notify_all ();
#endif
}


LOG4CPLUS_INLINE_EXPORT
void
Semaphore::lock () const
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::unique_lock<std::mutex> guard (mtx);

    if (LOG4CPLUS_UNLIKELY(val > max))
        LOG4CPLUS_THROW_RTE ("Semaphore::unlock(): val > max");

    while (val == 0)
        cv.wait (guard);

    --val;

    if (LOG4CPLUS_UNLIKELY(val >= max))
        LOG4CPLUS_THROW_RTE ("Semaphore::unlock(): val >= max");
#endif
}


//
//
//

LOG4CPLUS_INLINE_EXPORT
ManualResetEvent::ManualResetEvent (bool LOG4CPLUS_THREADED (sig))
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    : signaled (sig)
    , sigcount (0)
#endif
{ }


LOG4CPLUS_INLINE_EXPORT
ManualResetEvent::~ManualResetEvent ()
{ }


LOG4CPLUS_INLINE_EXPORT
void
ManualResetEvent::signal () const
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::unique_lock<std::mutex> guard (mtx);

    signaled = true;
    sigcount += 1;
    cv.notify_all ();
#endif
}


LOG4CPLUS_INLINE_EXPORT
void
ManualResetEvent::wait () const
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::unique_lock<std::mutex> guard (mtx);

    if (! signaled)
    {
        unsigned prev_count = sigcount;
        do
        {
            cv.wait (guard);
        }
        while (prev_count == sigcount);
    }
#endif
}


LOG4CPLUS_INLINE_EXPORT
bool
ManualResetEvent::timed_wait (unsigned long LOG4CPLUS_THREADED (msec)) const
{
#if defined (LOG4CPLUS_SINGLE_THREADED)
    return true;

#else
    std::unique_lock<std::mutex> guard (mtx);

    if (! signaled)
    {
        unsigned prev_count = sigcount;

        std::chrono::steady_clock::time_point const wait_until_time
            = std::chrono::steady_clock::now ()
            + std::chrono::milliseconds (msec);

        do
        {
            int ret = static_cast<int>(
                cv.wait_until (guard, wait_until_time));
            switch (ret)
            {
            case static_cast<int>(std::cv_status::no_timeout):
                break;

            case static_cast<int>(std::cv_status::timeout):
                return false;

            default:
                guard.unlock ();
                guard.release ();
                LOG4CPLUS_THROW_RTE ("ManualResetEvent::timed_wait");
            }
        }
        while (prev_count == sigcount);
    }

    return true;
#endif
}


LOG4CPLUS_INLINE_EXPORT
void
ManualResetEvent::reset () const
{
#if ! defined (LOG4CPLUS_SINGLE_THREADED)
    std::lock_guard<std::mutex> guard (mtx);

    signaled = false;
#endif
}


//
//
//

LOG4CPLUS_INLINE_EXPORT
SharedMutexImplBase::~SharedMutexImplBase ()
{ }


//
//
//

LOG4CPLUS_INLINE_EXPORT
SharedMutex::SharedMutex ()
    : sm (LOG4CPLUS_THREADED (new impl::SharedMutex))
{ }


LOG4CPLUS_INLINE_EXPORT
SharedMutex::~SharedMutex ()
{
    LOG4CPLUS_THREADED (delete static_cast<impl::SharedMutex *>(sm));
}


LOG4CPLUS_INLINE_EXPORT
void
SharedMutex::rdlock () const
{
    LOG4CPLUS_THREADED (static_cast<impl::SharedMutex *>(sm)->rdlock ());
}


LOG4CPLUS_INLINE_EXPORT
void
SharedMutex::wrlock () const
{
    LOG4CPLUS_THREADED (static_cast<impl::SharedMutex *>(sm)->wrlock ());
}


LOG4CPLUS_INLINE_EXPORT
void
SharedMutex::rdunlock () const
{
    LOG4CPLUS_THREADED (static_cast<impl::SharedMutex *>(sm)->rdunlock ());
}


LOG4CPLUS_INLINE_EXPORT
void
SharedMutex::wrunlock () const
{
    LOG4CPLUS_THREADED (static_cast<impl::SharedMutex *>(sm)->wrunlock ());
}


} } // namespace log4cplus { namespace thread {

#endif // LOG4CPLUS_ENABLE_SYNCPRIMS_PUB_IMPL

#endif // LOG4CPLUS_THREAD_SYNCPRIMS_PUB_IMPL_H
