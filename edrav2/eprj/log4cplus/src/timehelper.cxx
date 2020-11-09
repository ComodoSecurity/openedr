// Module:  Log4CPLUS
// File:    timehelper.cxx
// Created: 4/2003
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

#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/streams.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/internal/internal.h>

#include <algorithm>
#include <vector>
#include <iomanip>
#include <cassert>
#include <cerrno>
#if defined (UNICODE)
#include <cwchar>
#endif

#if defined (LOG4CPLUS_HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(LOG4CPLUS_HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined (LOG4CPLUS_HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(LOG4CPLUS_HAVE_GMTIME_R) && !defined(LOG4CPLUS_SINGLE_THREADED)
#define LOG4CPLUS_NEED_GMTIME_R
#endif

#if defined(LOG4CPLUS_HAVE_LOCALTIME_R) && !defined(LOG4CPLUS_SINGLE_THREADED)
#define LOG4CPLUS_NEED_LOCALTIME_R
#endif

#include <log4cplus/config/windowsh-inc.h>


namespace log4cplus { namespace helpers {

const int ONE_SEC_IN_USEC = 1000000;

using std::mktime;
using std::gmtime;
using std::localtime;
#if defined (UNICODE)
using std::wcsftime;
#else
using std::strftime;
#endif


Time
from_struct_tm (tm * t)
{
    time_t time = helpers::mktime(t);
    if (LOG4CPLUS_LIKELY (time != -1))
        return from_time_t (time);
    else
    {
        int eno = errno;
        throw std::system_error (eno, std::system_category (),
            "from_struct_tm(): mktime() failed");
    }
}

void
gmTime (tm* t, Time const & the_time)
{
    time_t clock = to_time_t (the_time);
#if defined (LOG4CPLUS_HAVE_GMTIME_S) && defined (_MSC_VER)
    errno_t eno;
    if (LOG4CPLUS_UNLIKELY ((eno = gmtime_s (t, &clock)) != 0))
        throw std::system_error (eno, std::system_category (),
            "gmTime(): gmtime_s() failed");
#elif defined (LOG4CPLUS_HAVE_GMTIME_S) && defined (__BORLANDC__)
    gmtime_s (&clock, t);
#elif defined (LOG4CPLUS_NEED_GMTIME_R)
    gmtime_r (&clock, t);
#else
    tm* tmp = helpers::gmtime(&clock);
    *t = *tmp;
#endif
}


void
localTime (tm* t, Time const & the_time)
{
    time_t clock = to_time_t (the_time);
#ifdef LOG4CPLUS_NEED_LOCALTIME_R
    ::localtime_r(&clock, t);
#elif defined (LOG4CPLUS_HAVE_LOCALTIME_S)
    errno_t eno;
    if (LOG4CPLUS_UNLIKELY ((eno = localtime_s (t, &clock)) != 0))
        throw std::system_error (eno, std::system_category (),
            "localTime(): localtime_s() failed");
#else
    tm* tmp = helpers::localtime(&clock);
    *t = *tmp;
#endif
}


namespace
{


static log4cplus::tstring const padding_zeros[4] =
{
    { LOG4CPLUS_TEXT("000") },
    { LOG4CPLUS_TEXT("00") },
    { LOG4CPLUS_TEXT("0") },
    { }
};


static log4cplus::tstring const uc_q_padding_zeros[4] =
{
    { LOG4CPLUS_TEXT(".000") },
    { LOG4CPLUS_TEXT(".00") },
    { LOG4CPLUS_TEXT(".0") },
    { LOG4CPLUS_TEXT(".") }
};


static
void
build_q_value (log4cplus::tstring & q_str, long tv_usec)
{
    convertIntegerToString(q_str, tv_usec / 1000);
    std::size_t const len = q_str.length();
    if (len <= 2)
        q_str.insert (0, padding_zeros[q_str.length()]);
}


static
void
build_uc_q_value (log4cplus::tstring & uc_q_str, long tv_usec,
    log4cplus::tstring & tmp)
{
    build_q_value (uc_q_str, tv_usec);

    convertIntegerToString(tmp, tv_usec % 1000);
    std::size_t const usecs_len = tmp.length();
    tmp.insert (0, usecs_len <= 3
        ? uc_q_padding_zeros[usecs_len] : uc_q_padding_zeros[3]);
    uc_q_str.append (tmp);
}


} // namespace


log4cplus::tstring
getFormattedTime(const log4cplus::tstring& fmt_orig,
    Time const & the_time, bool use_gmtime)
{
    if (fmt_orig.empty () || fmt_orig[0] == 0)
        return log4cplus::tstring ();

    tm time;

    if (use_gmtime)
        gmTime (&time, the_time);
    else
        localTime (&time, the_time);

    enum State
    {
        TEXT,
        PERCENT_SIGN
    };

    internal::gft_scratch_pad & gft_sp = internal::get_gft_scratch_pad ();
    gft_sp.reset ();

    std::size_t const fmt_orig_size = gft_sp.fmt.size ();
    gft_sp.ret.reserve (fmt_orig_size + fmt_orig_size / 3);
    State state = TEXT;

    // Walk the format string and process all occurences of %q, %Q and %s.

    long const tv_usec = microseconds_part (the_time);
    time_t const tv_sec = to_time_t (the_time);
    for (auto fmt_ch : fmt_orig)
    {
        switch (state)
        {
        case TEXT:
        {
            if (fmt_ch == LOG4CPLUS_TEXT ('%'))
                state = PERCENT_SIGN;
            else
                gft_sp.ret.push_back (fmt_ch);
        }
        break;

        case PERCENT_SIGN:
        {
            switch (fmt_ch)
            {
            case LOG4CPLUS_TEXT ('q'):
            {
                if (! gft_sp.q_str_valid)
                {
                    build_q_value (gft_sp.q_str, tv_usec);
                    gft_sp.q_str_valid = true;
                }
                gft_sp.ret.append (gft_sp.q_str);
                state = TEXT;
            }
            break;

            case LOG4CPLUS_TEXT ('Q'):
            {
                if (! gft_sp.uc_q_str_valid)
                {
                    build_uc_q_value (gft_sp.uc_q_str, tv_usec, gft_sp.tmp);
                    gft_sp.uc_q_str_valid = true;
                }
                gft_sp.ret.append (gft_sp.uc_q_str);
                state = TEXT;
            }
            break;

            // Windows do not support %s format specifier
            // (seconds since epoch).
            case LOG4CPLUS_TEXT ('s'):
            {
                if (! gft_sp.s_str_valid)
                {
                    convertIntegerToString (gft_sp.s_str, tv_sec);
                    gft_sp.s_str_valid = true;
                }
                gft_sp.ret.append (gft_sp.s_str);
                state = TEXT;
            }
            break;

            default:
            {
                gft_sp.ret.push_back (LOG4CPLUS_TEXT ('%'));
                gft_sp.ret.push_back (fmt_ch);
                state = TEXT;
            }
            }
        }
        break;
        }
    }

    // Finally call strftime/wcsftime to format the rest of the string.

    gft_sp.fmt.swap (gft_sp.ret);
    std::size_t buffer_size = gft_sp.fmt.size () + 1;
    std::size_t len;

    // Limit how far can the buffer grow. This is necessary so that we
    // catch bad format string. Some implementations of strftime() signal
    // both too small buffer and invalid format string by returning 0
    // without changing errno.
    std::size_t const buffer_size_max
        = (std::max) (static_cast<std::size_t>(1024), buffer_size * 16);
    
    buffer_size = (std::max) (buffer_size, gft_sp.buffer.capacity());

    do
    {
        gft_sp.buffer.resize (buffer_size);
        errno = 0;
#ifdef UNICODE
        len = helpers::wcsftime(&gft_sp.buffer[0], buffer_size,
            gft_sp.fmt.c_str(), &time);
#else
        len = helpers::strftime(&gft_sp.buffer[0], buffer_size,
            gft_sp.fmt.c_str(), &time);
#endif
        if (len == 0)
        {
            int const eno = errno;
            buffer_size *= 2;
            if (buffer_size > buffer_size_max)
            {
                LogLog::getLogLog ()->error (
                    LOG4CPLUS_TEXT("Error in strftime(): ")
                    + convertIntegerToString (eno), true);
            }
        }
    }
    while (len == 0);

    return tstring (gft_sp.buffer.begin (), gft_sp.buffer.begin () + len);
}


} } // namespace log4cplus { namespace helpers {
