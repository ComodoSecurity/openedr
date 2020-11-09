// -*- C++ -*-
// Module:  Log4CPLUS
// File:    threads.h
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

/** @file */

#ifndef LOG4CPLUS_IMPL_THREADS_IMPL_HEADER_
#define LOG4CPLUS_IMPL_THREADS_IMPL_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#if defined (_WIN32)
#include <log4cplus/config/windowsh-inc.h>
#endif
#include <log4cplus/tstring.h>
#include <log4cplus/helpers/pointer.h>
#include <log4cplus/thread/threads.h>

#if ! defined (INSIDE_LOG4CPLUS)
#  error "This header must not be be used outside log4cplus' implementation files."
#endif


namespace log4cplus { namespace thread { namespace impl {


#if defined (LOG4CPLUS_USE_PTHREADS)

typedef pthread_t os_handle_type;
typedef pthread_t os_id_type;


inline
pthread_t
getCurrentThreadId ()
{
    return pthread_self ();
}


#elif defined (LOG4CPLUS_USE_WIN32_THREADS)

typedef HANDLE os_handle_type;
typedef DWORD os_id_type;


inline
DWORD
getCurrentThreadId ()
{
    return GetCurrentThreadId ();
}


#elif defined (LOG4CPLUS_SINGLE_THREADED)

typedef void * os_handle_type;
typedef int os_id_type;


inline
int
getCurrentThreadId ()
{
    return 1;
}


#endif


} } } // namespace log4cplus { namespace thread { namespace impl {


#endif // LOG4CPLUS_IMPL_THREADS_IMPL_HEADER_
