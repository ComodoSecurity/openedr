// -*- C++ -*-
// Module:  Log4CPLUS
// File:    customloglevelmanager.h
// Created: 12/2018
// Author:  Jens Rehsack
// Author:  Václav Haisman
//
//
//  Copyright (C) 2018, Jens Rehsack. All rights reserved.
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

/** @file
 * This header contains declaration internal to log4cplus. They must never be
 * visible from user accesible headers or exported in DLL/shared library.
 */


#ifndef LOG4CPLUS_INTERNAL_CUSTOMLOGLEVELMANAGER_HEADER_
#define LOG4CPLUS_INTERNAL_CUSTOMLOGLEVELMANAGER_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#if ! defined (INSIDE_LOG4CPLUS)
#  error "This header must not be be used outside log4cplus' implementation files."
#endif

#include <map>
#include <log4cplus/thread/syncprims.h>
#include <log4cplus/internal/internal.h>


namespace log4cplus {

namespace internal {


/**
 * Custom log level manager used by C API.
 */
class CustomLogLevelManager {
protected:
    log4cplus::thread::Mutex mtx;
    bool pushed_methods;
    std::map<LogLevel,tstring> ll2nm;
    std::map<tstring,LogLevel> nm2ll;

public:
    CustomLogLevelManager()
        : pushed_methods (false)
    { }

    bool add(LogLevel ll, tstring const &nm)
    {
        log4cplus::thread::MutexGuard guard (mtx);

        if (! pushed_methods)
        {
            pushed_methods = true;
            getLogLevelManager().pushToStringMethod(customToStringMethod);
            getLogLevelManager().pushFromStringMethod(customFromStringMethod);
        }

        auto i = ll2nm.lower_bound(ll);
        if( ( i != ll2nm.end() ) && ( i->first == ll ) && ( i->second != nm ) )
            return false;

        auto j = nm2ll.lower_bound(nm);
        if( ( j != nm2ll.end() ) && ( j->first == nm ) && ( j->second != ll ) )
            return false;

        // there is no else after return
        ll2nm.insert( i, std::make_pair(ll, nm) );
        nm2ll.insert( j, std::make_pair(nm, ll) );
        return true;
    }

    bool remove(LogLevel ll, tstring const &nm)
    {
        log4cplus::thread::MutexGuard guard (mtx);

        auto i = ll2nm.find(ll);
        auto j = nm2ll.find(nm);
        if( ( i != ll2nm.end() ) && ( j != nm2ll.end() ) &&
            ( i->first == j->second ) && ( i->second == j->first ) ) {
            ll2nm.erase(i);
            nm2ll.erase(j);

            return true;
        }

        // there is no else after return
        return false;
    }

protected:
    tstring const & customToStringMethodWorker(LogLevel ll)
    {
        log4cplus::thread::MutexGuard guard (mtx);
        auto i = ll2nm.find(ll);
        if( i != ll2nm.end() )
            return i->second;

        return internal::empty_str;
    }

    LogLevel customFromStringMethodWorker(const tstring& nm)
    {
        log4cplus::thread::MutexGuard guard (mtx);
        auto i = nm2ll.find(nm);
        if( i != nm2ll.end() )
            return i->second;

        return NOT_SET_LOG_LEVEL;
    }

    LOG4CPLUS_PRIVATE static tstring const & customToStringMethod(LogLevel ll);
    LOG4CPLUS_PRIVATE static LogLevel customFromStringMethod(const tstring& nm);
};

LOG4CPLUS_PRIVATE CustomLogLevelManager & getCustomLogLevelManager ();

} // namespace internal

} // namespace log4cplus


#endif // LOG4CPLUS_INTERNAL_CUSTOMLOGLEVELMANAGER_HEADER
