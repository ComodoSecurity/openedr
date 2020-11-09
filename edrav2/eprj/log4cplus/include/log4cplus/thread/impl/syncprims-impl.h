// -*- C++ -*-
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

#ifndef LOG4CPLUS_THREAD_SYNCPRIMS_IMPL_H
#define LOG4CPLUS_THREAD_SYNCPRIMS_IMPL_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#if ! defined (INSIDE_LOG4CPLUS)
#  error "This header must not be be used outside log4cplus' implementation files."
#endif

#include <stdexcept>
#include <log4cplus/thread/syncprims.h>
#include <mutex>
#include <thread>
#include <condition_variable>


namespace log4cplus { namespace thread { namespace impl {


LOG4CPLUS_EXPORT void LOG4CPLUS_ATTRIBUTE_NORETURN
    syncprims_throw_exception (char const * const msg,
    char const * const file, int line);


class SharedMutex
    : public SharedMutexImplBase
{
public:
    SharedMutex ();
    ~SharedMutex ();

    void rdlock () const;
    void wrlock () const;
    void rdunlock () const;
    void wrunlock () const;

private:
    Mutex m1;
    Mutex m2;
    Mutex m3;
    Semaphore w;
    mutable unsigned writer_count;
    Semaphore r;
    mutable unsigned reader_count;

    SharedMutex (SharedMutex const &);
    SharedMutex & operator = (SharedMutex const &);
};


} } } // namespace log4cplus { namespace thread { namespace impl {


// Include the appropriate implementations of the classes declared
// above.

#include <log4cplus/thread/impl/syncprims-cxx11.h>

#undef LOG4CPLUS_THROW_RTE


#endif // LOG4CPLUS_THREAD_SYNCPRIMS_IMPL_H
