// -*- C++ -*-
// Module:  Log4CPLUS
// File:    timehelper.h
// Created: 6/2003
// Author:  Tad E. Smith
//
//
// Copyright 2003-2017 Tad E. Smith
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

#ifndef LOG4CPLUS_HELPERS_TIME_HELPER_HEADER_
#define LOG4CPLUS_HELPERS_TIME_HELPER_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/tstring.h>

#if defined (LOG4CPLUS_HAVE_TIME_H)
#include <time.h>
#endif

#include <ctime>
#include <chrono>


namespace log4cplus {

namespace helpers {


using std::time_t;
using std::tm;
namespace chrono = std::chrono;

typedef chrono::system_clock Clock;
typedef chrono::duration<long long, std::micro> Duration;
typedef chrono::time_point<Clock, Duration> Time;


template <typename FromDuration>
inline
Time
time_cast (chrono::time_point<Clock, FromDuration> const & tp)
{
    return chrono::time_point_cast<Duration, Clock> (tp);
}


inline
Time
now ()
{
    return time_cast (Clock::now ());
}


inline
Time
from_time_t (time_t t_time)
{
    return time_cast (Clock::from_time_t (t_time));
}


inline
time_t
to_time_t (Time const & the_time)
{
    // This is based on <http://stackoverflow.com/a/17395137/341065>. It is
    // possible that to_time_t() returns rounded time and we want truncation.

    time_t time = Clock::to_time_t (the_time);
    auto const rounded_time = from_time_t (time);
    if (rounded_time > the_time)
        --time;

    return time;
}


LOG4CPLUS_EXPORT Time from_struct_tm (tm * t);


inline
Time
truncate_fractions (Time const & the_time)
{
    return from_time_t (to_time_t (the_time));
}


inline
long
microseconds_part (Time const & the_time)
{
    static_assert ((std::ratio_equal<Duration::period, std::micro>::value),
        "microseconds");

    // This is based on <http://stackoverflow.com/a/17395137/341065>
    return static_cast<long>(
        (the_time - from_time_t (to_time_t (the_time))).count ());
}


inline
Time
time_from_parts (time_t tv_sec, long tv_usec)
{
    return from_time_t (tv_sec) + chrono::microseconds (tv_usec);
}


/**
 * Populates <code>tm</code> using the <code>gmtime()</code>
 * function.
 */

LOG4CPLUS_EXPORT
void gmTime (tm* t, Time const &);

/**
 * Populates <code>tm</code> using the <code>localtime()</code>
 * function.
 */

LOG4CPLUS_EXPORT
void localTime (tm* t, Time const &);

/**
 * Returns a string with a "formatted time" specified by
 * <code>fmt</code>.  It used the <code>strftime()</code>
 * function to do this.
 *
 * Look at your platform's <code>strftime()</code> documentation
 * for the formatting options available.
 *
 * The following additional options are provided:<br>
 * <code>%q</code> - 3 character field that provides milliseconds
 * <code>%Q</code> - 7 character field that provides fractional
 * milliseconds.
 */
LOG4CPLUS_EXPORT
log4cplus::tstring getFormattedTime (log4cplus::tstring const & fmt,
    Time const & the_time, bool use_gmtime = false);


} // namespace helpers

} // namespace log4cplus


#endif // LOG4CPLUS_HELPERS_TIME_HELPER_HEADER_
