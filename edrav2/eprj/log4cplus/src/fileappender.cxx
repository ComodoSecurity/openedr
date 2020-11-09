// Module:  Log4CPLUS
// File:    fileappender.cxx
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

#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/streams.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/helpers/fileinfo.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/internal/internal.h>
#include <log4cplus/internal/env.h>
#include <algorithm>
#include <sstream>
#include <cstdio>
#include <stdexcept>
#include <cmath> // std::fmod
#include <iomanip>
#include <set>
#include <filesystem>
#include <charconv>

// For _wrename() and _wremove() on Windows.
#include <stdio.h>
#include <cerrno>
#ifdef LOG4CPLUS_HAVE_ERRNO_H
#include <errno.h>
#endif


namespace log4cplus
{

using helpers::Properties;
using helpers::Time;


const long DEFAULT_ROLLING_LOG_SIZE = 10 * 1024 * 1024L;
const long MINIMUM_ROLLING_LOG_SIZE = 200*1024L;


///////////////////////////////////////////////////////////////////////////////
// File LOCAL definitions
///////////////////////////////////////////////////////////////////////////////

namespace
{

long const LOG4CPLUS_FILE_NOT_FOUND = ENOENT;


static
long
file_rename (tstring const & src, tstring const & target)
{
#if defined (UNICODE) && defined (_WIN32)
    if (_wrename (src.c_str (), target.c_str ()) == 0)
        return 0;
    else
        return errno;

#else
    if (std::rename (LOG4CPLUS_TSTRING_TO_STRING (src).c_str (),
            LOG4CPLUS_TSTRING_TO_STRING (target).c_str ()) == 0)
        return 0;
    else
        return errno;

#endif
}


static
long
file_remove (tstring const & src)
{
#if defined (UNICODE) && defined (_WIN32)
    if (_wremove (src.c_str ()) == 0)
        return 0;
    else
        return errno;

#else
    if (std::remove (LOG4CPLUS_TSTRING_TO_STRING (src).c_str ()) == 0)
        return 0;
    else
        return errno;

#endif
}


static
void
loglog_renaming_result (helpers::LogLog & loglog, tstring const & src,
    tstring const & target, long ret)
{
    if (ret == 0)
    {
        loglog.debug (
            LOG4CPLUS_TEXT("Renamed file ")
            + src
            + LOG4CPLUS_TEXT(" to ")
            + target);
    }
    else if (ret != LOG4CPLUS_FILE_NOT_FOUND)
    {
        tostringstream oss;
        oss << LOG4CPLUS_TEXT("Failed to rename file from ")
            << src
            << LOG4CPLUS_TEXT(" to ")
            << target
            << LOG4CPLUS_TEXT("; error ")
            << ret;
        loglog.error (oss.str ());
    }
}


static
void
loglog_opening_result (helpers::LogLog & loglog,
    log4cplus::tostream const & os, tstring const & filename)
{
    if (! os)
    {
        loglog.error (
            LOG4CPLUS_TEXT("Failed to open file ")
            + filename);
    }
}


static
void
rolloverFiles(const tstring& filename, unsigned int maxBackupIndex)
{
    helpers::LogLog * loglog = helpers::LogLog::getLogLog();

    // Delete the oldest file
    tostringstream buffer;
    buffer << filename << LOG4CPLUS_TEXT(".") << maxBackupIndex;
    long ret = file_remove (buffer.str ());

    tostringstream source_oss;
    tostringstream target_oss;

    // Map {(maxBackupIndex - 1), ..., 2, 1} to {maxBackupIndex, ..., 3, 2}
    for (int i = maxBackupIndex - 1; i >= 1; --i)
    {
        source_oss.str(internal::empty_str);
        target_oss.str(internal::empty_str);

        source_oss << filename << LOG4CPLUS_TEXT(".") << i;
        target_oss << filename << LOG4CPLUS_TEXT(".") << (i+1);

        tstring const source (source_oss.str ());
        tstring const target (target_oss.str ());

#if defined (_WIN32)
        // Try to remove the target first. It seems it is not
        // possible to rename over existing file.
        ret = file_remove (target);
#endif

        ret = file_rename (source, target);
        loglog_renaming_result (*loglog, source, target, ret);
    }
} // end rolloverFiles()


static
std::locale
get_locale_by_name (tstring const & locale_name)
{try
{
    spi::LocaleFactoryRegistry & reg = spi::getLocaleFactoryRegistry ();
    spi::LocaleFactory * fact = reg.get (locale_name);
    if (fact)
    {
        helpers::Properties props;
        props.setProperty (LOG4CPLUS_TEXT ("Locale"), locale_name);
        return fact->createObject (props);
    }
    else
        return std::locale (LOG4CPLUS_TSTRING_TO_STRING (locale_name).c_str ());
}
catch (std::runtime_error const &)
{
    helpers::getLogLog ().error (
        LOG4CPLUS_TEXT ("Failed to create locale " + locale_name));
    return std::locale ();
}}

} // namespace

///////////////////////////////////////////////////////////////////////////////
// FileAppenderBase utility functions
///////////////////////////////////////////////////////////////////////////////

static tstring
preprocessFilename(const tstring& filename)
{
    tostringstream result;

    for (size_t i = 0; i < filename.length(); )
    {
        tchar c = filename[i];

        if (c == LOG4CPLUS_TEXT('%') &&
            i < filename.length() - 1 &&
            filename[i + 1] == LOG4CPLUS_TEXT('U'))
        {
            using namespace std::chrono;
            auto t = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

            tchar const prev_fill = result.fill();
            std::ios_base::fmtflags const prev_flags = result.flags();
            result << std::setfill(LOG4CPLUS_TEXT('0')) << std::setw(16) << std::hex << t;
            result.fill(prev_fill);
            result.flags(prev_flags);
            i += 2;
        }
        else if (c == LOG4CPLUS_TEXT('%') &&
            i < filename.length() - 1 &&
            filename[i + 1] == LOG4CPLUS_TEXT('P'))
        {
            result << internal::get_process_id();
            i += 2;
        }
        else
        {
            result << c;
            i += 1;
        }
    }

    return result.str();
}


///////////////////////////////////////////////////////////////////////////////
// FileAppenderBase ctors and dtor
///////////////////////////////////////////////////////////////////////////////

FileAppenderBase::FileAppenderBase(const tstring& filename_,
    std::ios_base::openmode mode_, bool immediateFlush_, bool createDirs_)
    : immediateFlush(immediateFlush_)
    , createDirs (createDirs_)
    , reopenDelay(1)
    , bufferSize (0)
    , buffer (nullptr)
    , filename(preprocessFilename(filename_))
    , localeName (LOG4CPLUS_TEXT ("DEFAULT"))
    , fileOpenMode(mode_)
{
}


FileAppenderBase::FileAppenderBase(const Properties& props,
                                   std::ios_base::openmode mode_)
    : Appender(props)
    , immediateFlush(true)
    , createDirs (false)
    , reopenDelay(1)
    , bufferSize (0)
    , buffer (nullptr)
{
    filename = preprocessFilename(props.getProperty(LOG4CPLUS_TEXT("File")));
    lockFileName = props.getProperty (LOG4CPLUS_TEXT ("LockFile"));
    localeName = props.getProperty (LOG4CPLUS_TEXT ("Locale"), LOG4CPLUS_TEXT ("DEFAULT"));

    props.getBool (immediateFlush, LOG4CPLUS_TEXT("ImmediateFlush"));
    props.getBool (createDirs, LOG4CPLUS_TEXT("CreateDirs"));
    props.getInt (reopenDelay, LOG4CPLUS_TEXT("ReopenDelay"));
    props.getULong (bufferSize, LOG4CPLUS_TEXT("BufferSize"));

    bool app = (mode_ & (std::ios_base::app | std::ios_base::ate)) != 0;
    props.getBool (app, LOG4CPLUS_TEXT("Append"));
    fileOpenMode = app ? std::ios::app : std::ios::trunc;

    if (props.getProperty(LOG4CPLUS_TEXT("TextMode"), LOG4CPLUS_TEXT("Text"))
        == LOG4CPLUS_TEXT("Binary"))
        fileOpenMode |= std::ios_base::binary;
}


void
FileAppenderBase::init()
{
    if (useLockFile && lockFileName.empty ())
    {
        if (filename.empty())
        {
            getErrorHandler()->error(LOG4CPLUS_TEXT
                ("UseLockFile is true but neither LockFile nor File are specified"));
            return;
        }

        lockFileName = filename;
        lockFileName += LOG4CPLUS_TEXT(".lock");
    }

    if (bufferSize != 0)
    {
        buffer.reset (new tchar[bufferSize]);
        out.rdbuf ()->pubsetbuf (buffer.get (), bufferSize);
    }

    helpers::LockFileGuard guard;
    if (useLockFile && ! lockFile.get ())
    {
        if (createDirs)
            internal::make_dirs (lockFileName);

        try
        {
            lockFile.reset (new helpers::LockFile (lockFileName));
            guard.attach_and_lock (*lockFile);
        }
        catch (std::runtime_error const &)
        {
            // We do not need to do any logging here as the internals
            // of LockFile already use LogLog to report the failure.
            return;
        }
    }

    open(fileOpenMode);
    imbue (get_locale_by_name (localeName));
}

///////////////////////////////////////////////////////////////////////////////
// FileAppenderBase public methods
///////////////////////////////////////////////////////////////////////////////

void
FileAppenderBase::close()
{
    thread::MutexGuard guard (access_mutex);

    out.close();
    buffer.reset ();
    closed = true;
}


std::locale
FileAppenderBase::imbue(std::locale const& loc)
{
    return out.imbue (loc);
}


std::locale
FileAppenderBase::getloc () const
{
    return out.getloc ();
}


///////////////////////////////////////////////////////////////////////////////
// FileAppenderBase protected methods
///////////////////////////////////////////////////////////////////////////////

// This method does not need to be locked since it is called by
// doAppend() which performs the locking
void
FileAppenderBase::append(const spi::InternalLoggingEvent& event)
{
    if(!out.good()) {
        if(!reopen()) {
            getErrorHandler()->error(  LOG4CPLUS_TEXT("file is not open: ")
                                     + filename);
            return;
        }
        // Resets the error handler to make it
        // ready to handle a future append error.
        else
            getErrorHandler()->reset();
    }

    if (useLockFile)
        out.seekp (0, std::ios_base::end);

    layout->formatAndAppend(out, event);

    if(immediateFlush || useLockFile)
        out.flush();
}

void
FileAppenderBase::open(std::ios_base::openmode mode)
{
    if (createDirs)
        internal::make_dirs (filename);

    out.open(LOG4CPLUS_FSTREAM_PREFERED_FILE_NAME(filename).c_str(), mode);

    if(!out.good()) {
        getErrorHandler()->error(LOG4CPLUS_TEXT("Unable to open file: ") + filename);
        return;
    }
    helpers::getLogLog().debug(LOG4CPLUS_TEXT("Just opened file: ") + filename);
}

bool
FileAppenderBase::reopen()
{
    // When append never failed and the file re-open attempt must
    // be delayed, set the time when reopen should take place.
    if (reopen_time == log4cplus::helpers::Time () && reopenDelay != 0)
        reopen_time = log4cplus::helpers::now ()
            + helpers::chrono::seconds (reopenDelay);
    else
    {
        // Otherwise, check for end of the delay (or absence of delay)
        // to re-open the file.
        if (reopen_time <= log4cplus::helpers::now ()
            || reopenDelay == 0)
        {
            // Close the current file
            out.close();
            // reset flags since the C++ standard specified that all
            // the flags should remain unchanged on a close
            out.clear();

            // Re-open the file.
            open(std::ios_base::out | std::ios_base::ate | std::ios_base::app);

            // Reset last fail time.
            reopen_time = log4cplus::helpers::Time ();

            // Succeed if no errors are found.
            if(out.good())
                return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
// FileAppender ctors and dtor
///////////////////////////////////////////////////////////////////////////////

FileAppender::FileAppender(
    const tstring& filename_,
    std::ios_base::openmode mode_,
    bool immediateFlush_,
    bool createDirs_)
    : FileAppenderBase(filename_, mode_, immediateFlush_, createDirs_)
{
    init();
}


FileAppender::FileAppender(
    const Properties& props,
    std::ios_base::openmode mode_)
    : FileAppenderBase(props, mode_)
{
    init();
}

FileAppender::~FileAppender()
{
    destructorImpl();
}

///////////////////////////////////////////////////////////////////////////////
// FileAppender protected methods
///////////////////////////////////////////////////////////////////////////////

void
FileAppender::init()
{
    if (filename.empty())
    {
        getErrorHandler()->error( LOG4CPLUS_TEXT("Invalid filename") );
        return;
    }

    FileAppenderBase::init();
}

///////////////////////////////////////////////////////////////////////////////
// RollingFileAppender ctors and dtor
///////////////////////////////////////////////////////////////////////////////

RollingFileAppender::RollingFileAppender(const tstring& filename_,
    long maxFileSize_, int maxBackupIndex_, bool immediateFlush_,
    bool createDirs_)
    : FileAppender(filename_, std::ios_base::app, immediateFlush_, createDirs_)
{
    init(maxFileSize_, maxBackupIndex_);
}


RollingFileAppender::RollingFileAppender(const Properties& properties)
    : FileAppender(properties, std::ios_base::app)
{
    long tmpMaxFileSize = DEFAULT_ROLLING_LOG_SIZE;
    int tmpMaxBackupIndex = 1;
    tstring tmp (
        helpers::toUpper (
            properties.getProperty (LOG4CPLUS_TEXT ("MaxFileSize"))));
    if (! tmp.empty ())
    {
        tmpMaxFileSize = std::atoi(LOG4CPLUS_TSTRING_TO_STRING(tmp).c_str());
        if (tmpMaxFileSize != 0)
        {
            tstring::size_type const len = tmp.length();
            if (len > 2
                && tmp.compare (len - 2, 2, LOG4CPLUS_TEXT("MB")) == 0)
                tmpMaxFileSize *= (1024 * 1024); // convert to megabytes
            else if (len > 2
                && tmp.compare (len - 2, 2, LOG4CPLUS_TEXT("KB")) == 0)
                tmpMaxFileSize *= 1024; // convert to kilobytes
        }
    }

    properties.getInt (tmpMaxBackupIndex, LOG4CPLUS_TEXT("MaxBackupIndex"));

    init(tmpMaxFileSize, tmpMaxBackupIndex);
}

void
RollingFileAppender::init(long maxFileSize_, int maxBackupIndex_)
{
    if (maxFileSize_ < MINIMUM_ROLLING_LOG_SIZE)
    {
        tostringstream oss;
        oss << LOG4CPLUS_TEXT ("RollingFileAppender: MaxFileSize property")
            LOG4CPLUS_TEXT (" value is too small. Resetting to ")
            << MINIMUM_ROLLING_LOG_SIZE << ".";
        helpers::getLogLog ().warn (oss.str ());
        maxFileSize_ = MINIMUM_ROLLING_LOG_SIZE;
    }

    maxFileSize = maxFileSize_;
    maxBackupIndex = (std::max)(maxBackupIndex_, 1);
}


RollingFileAppender::~RollingFileAppender()
{
    destructorImpl();
}


///////////////////////////////////////////////////////////////////////////////
// RollingFileAppender protected methods
///////////////////////////////////////////////////////////////////////////////

// This method does not need to be locked since it is called by
// doAppend() which performs the locking
void
RollingFileAppender::append(const spi::InternalLoggingEvent& event)
{
    // Seek to the end of log file so that tellp() below returns the
    // right size.
    if (useLockFile)
        out.seekp (0, std::ios_base::end);

    // Rotate log file if needed before appending to it.
    if (out.tellp() > maxFileSize)
        rollover(true);

    FileAppender::append(event);

    // Rotate log file if needed after appending to it.
    if (out.tellp() > maxFileSize)
        rollover(true);
}


void
RollingFileAppender::rollover(bool alreadyLocked)
{
    helpers::LogLog & loglog = helpers::getLogLog();
    helpers::LockFileGuard guard;

    // Close the current file
    out.close();
    // Reset flags since the C++ standard specified that all the flags
    // should remain unchanged on a close.
    out.clear();

    if (useLockFile)
    {
        if (! alreadyLocked)
        {
            try
            {
                guard.attach_and_lock (*lockFile);
            }
            catch (std::runtime_error const &)
            {
                return;
            }
        }

        // Recheck the condition as there is a window where another
        // process can rollover the file before us.

        helpers::FileInfo fi;
        if (getFileInfo (&fi, filename) == -1
            || fi.size < maxFileSize)
        {
            // The file has already been rolled by another
            // process. Just reopen with the new file.

            // Open it up again.
            open (std::ios_base::out | std::ios_base::ate | std::ios_base::app);
            loglog_opening_result (loglog, out, filename);

            return;
        }
    }

    // If maxBackups <= 0, then there is no file renaming to be done.
    if (maxBackupIndex > 0)
    {
        rolloverFiles(filename, maxBackupIndex);

        // Rename fileName to fileName.1
        tstring target = filename + LOG4CPLUS_TEXT(".1");

        long ret;

#if defined (_WIN32)
        // Try to remove the target first. It seems it is not
        // possible to rename over existing file.
        ret = file_remove (target);
#endif

        loglog.debug (
            LOG4CPLUS_TEXT("Renaming file ")
            + filename
            + LOG4CPLUS_TEXT(" to ")
            + target);
        ret = file_rename (filename, target);
        loglog_renaming_result (loglog, filename, target, ret);
    }
    else
    {
        loglog.debug (filename + LOG4CPLUS_TEXT(" has no backups specified"));
    }

    // Open it up again in truncation mode
    open(std::ios::out | std::ios::trunc);
    loglog_opening_result (loglog, out, filename);
}


///////////////////////////////////////////////////////////////////////////////
// DailyRollingFileAppender ctors and dtor
///////////////////////////////////////////////////////////////////////////////

DailyRollingFileAppender::DailyRollingFileAppender(
    const tstring& filename_, DailyRollingFileSchedule schedule_,
    bool immediateFlush_, int maxBackupIndex_, bool createDirs_,
    bool rollOnClose_, const tstring& datePattern_)
    : FileAppender(filename_, std::ios_base::app, immediateFlush_, createDirs_)
    , maxBackupIndex(maxBackupIndex_)
    , rollOnClose(rollOnClose_)
    , datePattern(datePattern_)
{
    init(schedule_);
}



DailyRollingFileAppender::DailyRollingFileAppender(
    const Properties& properties)
    : FileAppender(properties, std::ios_base::app)
    , maxBackupIndex(10)
    , rollOnClose(true)
{
    DailyRollingFileSchedule theSchedule = DAILY;
    tstring scheduleStr (helpers::toUpper (
        properties.getProperty (LOG4CPLUS_TEXT ("Schedule"))));

    if(scheduleStr == LOG4CPLUS_TEXT("MONTHLY"))
        theSchedule = MONTHLY;
    else if(scheduleStr == LOG4CPLUS_TEXT("WEEKLY"))
        theSchedule = WEEKLY;
    else if(scheduleStr == LOG4CPLUS_TEXT("DAILY"))
        theSchedule = DAILY;
    else if(scheduleStr == LOG4CPLUS_TEXT("TWICE_DAILY"))
        theSchedule = TWICE_DAILY;
    else if(scheduleStr == LOG4CPLUS_TEXT("HOURLY"))
        theSchedule = HOURLY;
    else if(scheduleStr == LOG4CPLUS_TEXT("MINUTELY"))
        theSchedule = MINUTELY;
    else {
        helpers::getLogLog().warn(
            LOG4CPLUS_TEXT("DailyRollingFileAppender::ctor()")
            LOG4CPLUS_TEXT("- \"Schedule\" not valid: ")
            + properties.getProperty(LOG4CPLUS_TEXT("Schedule")));
        theSchedule = DAILY;
    }

    properties.getBool (rollOnClose, LOG4CPLUS_TEXT("RollOnClose"));
    properties.getString (datePattern, LOG4CPLUS_TEXT("DatePattern"));
    properties.getInt (maxBackupIndex, LOG4CPLUS_TEXT("MaxBackupIndex"));

    init(theSchedule);
}


namespace
{


static
Time
round_time (Time const & t_, helpers::chrono::seconds seconds)
{
    time_t const t = helpers::to_time_t (t_);
    return helpers::from_time_t (
        t - static_cast<time_t>(std::fmod (
                static_cast<double>(t),
                static_cast<double>(seconds.count ()))));
}


static
Time
round_time_and_add (Time const & t, helpers::chrono::seconds const & seconds)
{
    return round_time (t, seconds) + seconds;
}


static
std::chrono::seconds
local_time_offset (Time const & t)
{
    tm time_local, time_gmt;

    helpers::localTime (&time_local, t);
    helpers::gmTime (&time_gmt, t);

    Time t2 = helpers::from_struct_tm (&time_local);
    Time t3 = helpers::from_struct_tm (&time_gmt);

    return std::chrono::duration_cast<std::chrono::seconds> (t2 - t3);
}


static
Time
adjust_for_time_zone (Time const & t, std::chrono::seconds const & tzoffset)
{
    return helpers::time_cast (t - tzoffset);
}


} // namespace


void
DailyRollingFileAppender::init(DailyRollingFileSchedule sch)
{
    this->schedule = sch;
    Time now = helpers::truncate_fractions (helpers::now ());
    scheduledFilename = getFilename(now);
    nextRolloverTime = calculateNextRolloverTime(now);
}



DailyRollingFileAppender::~DailyRollingFileAppender()
{
    destructorImpl();
}




///////////////////////////////////////////////////////////////////////////////
// DailyRollingFileAppender public methods
///////////////////////////////////////////////////////////////////////////////

void
DailyRollingFileAppender::close()
{
    if (rollOnClose)
        rollover();
    FileAppender::close();
}



///////////////////////////////////////////////////////////////////////////////
// DailyRollingFileAppender protected methods
///////////////////////////////////////////////////////////////////////////////

// This method does not need to be locked since it is called by
// doAppend() which performs the locking
void
DailyRollingFileAppender::append(const spi::InternalLoggingEvent& event)
{
    if(event.getTimestamp() >= nextRolloverTime) {
        rollover(true);
    }

    FileAppender::append(event);
}



void
DailyRollingFileAppender::rollover(bool alreadyLocked)
{
    helpers::LockFileGuard guard;

    if (useLockFile && ! alreadyLocked)
    {
        try
        {
            guard.attach_and_lock (*lockFile);
        }
        catch (std::runtime_error const &)
        {
            return;
        }
    }

    // Close the current file
    out.close();
    // reset flags since the C++ standard specified that all the flags
    // should remain unchanged on a close
    out.clear();

    // If we've already rolled over this time period, we'll make sure that we
    // don't overwrite any of those previous files.
    // E.g. if "log.2009-11-07.1" already exists we rename it
    // to "log.2009-11-07.2", etc.
    rolloverFiles(scheduledFilename, maxBackupIndex);

    // Do not overwriet the newest file either, e.g. if "log.2009-11-07"
    // already exists rename it to "log.2009-11-07.1"
    tostringstream backup_target_oss;
    backup_target_oss << scheduledFilename << LOG4CPLUS_TEXT(".") << 1;
    tstring backupTarget = backup_target_oss.str();

    helpers::LogLog & loglog = helpers::getLogLog();
    long ret;

#if defined (_WIN32)
    // Try to remove the target first. It seems it is not
    // possible to rename over existing file, e.g. "log.2009-11-07.1".
    ret = file_remove (backupTarget);
#endif

    // Rename e.g. "log.2009-11-07" to "log.2009-11-07.1".
    ret = file_rename (scheduledFilename, backupTarget);
    loglog_renaming_result (loglog, scheduledFilename, backupTarget, ret);

#if defined (_WIN32)
    // Try to remove the target first. It seems it is not
    // possible to rename over existing file, e.g. "log.2009-11-07".
    ret = file_remove (scheduledFilename);
#endif

    // Rename filename to scheduledFilename,
    // e.g. rename "log" to "log.2009-11-07".
    loglog.debug(
        LOG4CPLUS_TEXT("Renaming file ")
        + filename
        + LOG4CPLUS_TEXT(" to ")
        + scheduledFilename);
    ret = file_rename (filename, scheduledFilename);
    loglog_renaming_result (loglog, filename, scheduledFilename, ret);

    // Open a new file, e.g. "log".
    open(std::ios::out | std::ios::trunc);
    loglog_opening_result (loglog, out, filename);

    // Calculate the next rollover time
    log4cplus::helpers::Time now = helpers::now ();
    if (now >= nextRolloverTime)
    {
        scheduledFilename = getFilename(now);
        nextRolloverTime = calculateNextRolloverTime(now);
    }
}


static
Time
calculateNextRolloverTime(const Time& t, DailyRollingFileSchedule schedule)
{
    namespace chrono = helpers::chrono;

    struct tm next;
    switch(schedule)
    {
    case MONTHLY:
    {
        helpers::localTime (&next, t);
        next.tm_mon += 1;
        next.tm_mday = 1; // Round up to next month start
        next.tm_hour = 0;
        next.tm_min = 0;
        next.tm_sec = 0;
        next.tm_isdst = -1;

        Time ret;
        try
        {
            ret = helpers::from_struct_tm (&next);
        }
        catch (std::runtime_error const & e)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("calculateNextRolloverTime()-")
                LOG4CPLUS_TEXT(" from_struct_tm() returned error: ")
                + LOG4CPLUS_C_STR_TO_TSTRING (e.what ()));
            // Set next rollover to 31 days in future.
            ret = round_time (t, chrono::seconds (24 * 60 * 60))
                + chrono::seconds (2678400);
        }
        return ret;
    }

    case WEEKLY:
    {
        helpers::localTime (&next, t);
        // Round up to next week
        next.tm_mday += (7 - next.tm_wday + 1);
        next.tm_hour = 0;
        next.tm_min = 0;
        next.tm_sec = 0;
        next.tm_isdst = -1;

        Time ret;
        try
        {
            ret = helpers::from_struct_tm (&next);
        }
        catch (std::runtime_error const & e)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("calculateNextRolloverTime()-")
                LOG4CPLUS_TEXT(" from_struct_tm() returned error: ")
                + LOG4CPLUS_C_STR_TO_TSTRING (e.what ()));
            // Set next rollover to 7 days in future.
            ret = round_time (t, chrono::seconds (24 * 60 * 60))
                + chrono::seconds (7 * 24 * 60 * 60);
        }
        return ret;
    }

    default:
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("calculateNextRolloverTime()-")
            LOG4CPLUS_TEXT (" unhandled or invalid schedule value"));
        // Fall through.

    case DAILY:
    {
        helpers::localTime(&next, t);
        next.tm_mday += 1;
        next.tm_hour = 0;
        next.tm_min = 0;
        next.tm_sec = 0;
        next.tm_isdst = -1;

        Time ret;
        try
        {
            ret = helpers::from_struct_tm (&next);
        }
        catch (std::runtime_error const & e)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("calculateNextRolloverTime()-")
                LOG4CPLUS_TEXT(" from_struct_tm() returned error: ")
                + LOG4CPLUS_C_STR_TO_TSTRING (e.what ()));
            // Set next rollover to 24 hours in future.
            ret = round_time (t, chrono::seconds (60 * 60))
                + chrono::seconds (24 * 60 * 60);
        }

        return ret;
    }

    case TWICE_DAILY:
    {
        helpers::localTime(&next, t);
        if (next.tm_hour < 12)
            next.tm_hour = 12;
        else
            next.tm_hour = 24;
        next.tm_min = 0;
        next.tm_sec = 0;
        next.tm_isdst = -1;

        Time ret;
        try
        {
            ret = helpers::from_struct_tm (&next);
        }
        catch (std::runtime_error const & e)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("calculateNextRolloverTime()-")
                LOG4CPLUS_TEXT(" from_struct_tm() returned error: ")
                + LOG4CPLUS_C_STR_TO_TSTRING (e.what ()));
            // Set next rollover to 12 hours in future.
            ret = round_time (t, chrono::seconds (60 * 60))
                + chrono::seconds (12 * 60 * 60);
        }

        return ret;
    }

    case HOURLY:
    {
        helpers::localTime(&next, t);
        next.tm_hour += 1;
        next.tm_min = 0;
        next.tm_sec = 0;
        next.tm_isdst = -1;

        Time ret;
        try
        {
            ret = helpers::from_struct_tm (&next);
        }
        catch (std::runtime_error const & e)
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("calculateNextRolloverTime()-")
                LOG4CPLUS_TEXT(" from_struct_tm() returned error: ")
                + LOG4CPLUS_C_STR_TO_TSTRING (e.what ()));
            // Set next rollover to 60 minutes in future.
            ret = round_time (t, chrono::seconds (60 * 60))
                + chrono::seconds (60 * 60);
        }

        return ret;
    }

    case MINUTELY:
        return round_time_and_add (t, chrono::seconds (60));
    };
}


Time
DailyRollingFileAppender::calculateNextRolloverTime(const Time& t) const
{
    return helpers::truncate_fractions (
        log4cplus::calculateNextRolloverTime (t, schedule));
}


tstring
DailyRollingFileAppender::getFilename(const Time& t) const
{
    tchar const * pattern = nullptr;
    if (datePattern.empty())
    {
        switch (schedule)
        {
        case MONTHLY:
            pattern = LOG4CPLUS_TEXT("%Y-%m");
            break;

        case WEEKLY:
            pattern = LOG4CPLUS_TEXT("%Y-%W");
            break;

        default:
            helpers::getLogLog ().error (
                LOG4CPLUS_TEXT ("DailyRollingFileAppender::getFilename()-")
                LOG4CPLUS_TEXT (" invalid schedule value"));
            // Fall through.

        case DAILY:
            pattern = LOG4CPLUS_TEXT("%Y-%m-%d");
            break;

        case TWICE_DAILY:
            pattern = LOG4CPLUS_TEXT("%Y-%m-%d-%p");
            break;

        case HOURLY:
            pattern = LOG4CPLUS_TEXT("%Y-%m-%d-%H");
            break;

        case MINUTELY:
            pattern = LOG4CPLUS_TEXT("%Y-%m-%d-%H-%M");
            break;
        };
    }
    else
        pattern = datePattern.c_str();

    tstring result (filename);
    result += LOG4CPLUS_TEXT(".");
    result += helpers::getFormattedTime(pattern, t, false);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
// TimeBasedRollingFileAppender utility functions
///////////////////////////////////////////////////////////////////////////////

static tstring
preprocessDateTimePattern(const tstring& pattern, DailyRollingFileSchedule& schedule)
{
    // Example: "yyyy-MM-dd HH:mm:ss,aux"
    // Patterns from java.text.SimpleDateFormat not implemented here: Y, F, k, K, S, X

    tostringstream result;

    bool auxilary = (pattern.find(LOG4CPLUS_TEXT(",aux")) == pattern.length()-4);
    bool has_week = false, has_day = false, has_hour = false, has_minute = false;

    for (size_t i = 0; i < pattern.length(); )
    {
        tchar c = pattern[i];
        size_t end_pos = pattern.find_first_not_of(c, i);
        size_t len = (end_pos == tstring::npos ? pattern.length() : end_pos) - i;

        switch (c)
        {
        case LOG4CPLUS_TEXT('y'): // Year number
            if (len == 2)
                result << LOG4CPLUS_TEXT("%y");
            else if (len == 4)
                result << LOG4CPLUS_TEXT("%Y");
            break;
        case LOG4CPLUS_TEXT('Y'): // Week year
            if (len == 2)
                result << LOG4CPLUS_TEXT("%g");
            else if (len == 4)
                result << LOG4CPLUS_TEXT("%G");
            break;
        case LOG4CPLUS_TEXT('M'): // Month in year
            if (len == 2)
                result << LOG4CPLUS_TEXT("%m");
            else if (len == 3)
                result << LOG4CPLUS_TEXT("%b");
            else if (len > 3)
                result << LOG4CPLUS_TEXT("%B");
            break;
        case LOG4CPLUS_TEXT('w'): // Week in year
            if (len == 2) {
                result << LOG4CPLUS_TEXT("%W");
                has_week = true;
            }
            break;
        case LOG4CPLUS_TEXT('D'): // Day in year
            if (len == 3) {
                result << LOG4CPLUS_TEXT("%j");
                has_day = true;
            }
            break;
        case LOG4CPLUS_TEXT('d'): // Day in month
            if (len == 2) {
                result << LOG4CPLUS_TEXT("%d");
                has_day = true;
            }
            break;
        case LOG4CPLUS_TEXT('E'): // Day name in week
            if (len == 3) {
                result << LOG4CPLUS_TEXT("%a");
                has_day = true;
            } else if (len > 3) {
                result << LOG4CPLUS_TEXT("%A");
                has_day = true;
            }
            break;
        case LOG4CPLUS_TEXT('u'): // Day number of week
            if (len == 1) {
                result << LOG4CPLUS_TEXT("%u");
                has_day = true;
            }
            break;
        case LOG4CPLUS_TEXT('a'): // AM/PM marker
            if (len == 2)
                result << LOG4CPLUS_TEXT("%p");
            break;
        case LOG4CPLUS_TEXT('H'): // Hour in day
            if (len == 2) {
                result << LOG4CPLUS_TEXT("%H");
                has_hour = true;
            }
            break;
        case LOG4CPLUS_TEXT('h'): // Hour in am/pm (01-12)
            if (len == 2) {
                result << LOG4CPLUS_TEXT("%I");
                has_hour = true;
            }
            break;
        case LOG4CPLUS_TEXT('m'): // Minute in hour
            if (len == 2) {
                result << LOG4CPLUS_TEXT("%M");
                has_minute = true;
            }
            break;
        case LOG4CPLUS_TEXT('s'): // Second in minute
            if (len == 2)
                result << LOG4CPLUS_TEXT("%S");
            break;
        case LOG4CPLUS_TEXT('z'): // Time zone name
            if (len == 1)
                result << LOG4CPLUS_TEXT("%Z");
            break;
        case LOG4CPLUS_TEXT('Z'): // Time zone offset
            if (len == 1)
                result << LOG4CPLUS_TEXT("%z");
            break;
        default:
            result << c;
        }

        i += len;
    }

    if (!auxilary)
    {
        if (has_minute)
            schedule = MINUTELY;
        else if (has_hour)
            schedule = HOURLY;
        else if (has_day)
            schedule = DAILY;
        else if (has_week)
            schedule = WEEKLY;
        else
            schedule = MONTHLY;
    }

    return result.str();
}

static tstring
preprocessFilenamePattern(const tstring& pattern, DailyRollingFileSchedule& schedule)
{
    tostringstream result;

    for (size_t i = 0; i < pattern.length(); )
    {
        tchar c = pattern[i];

        if (c == LOG4CPLUS_TEXT('%') &&
            i < pattern.length()-1 &&
            pattern[i+1] == LOG4CPLUS_TEXT('d'))
        {
            if (i < pattern.length()-2 && pattern[i+2] == LOG4CPLUS_TEXT('{'))
            {
                size_t closingBracketPos = pattern.find(LOG4CPLUS_TEXT("}"), i+2);
                if (closingBracketPos == std::string::npos)
                {
                    break; // Malformed conversion specifier
                }
                else
                {
                    result << preprocessDateTimePattern(pattern.substr(i+3, closingBracketPos-(i+3)), schedule);
                    i = closingBracketPos + 1;
                }
            }
            else
            {
                // Default conversion specifier
                result << preprocessDateTimePattern(LOG4CPLUS_TEXT("yyyy-MM-dd"), schedule);
                i += 2;
            }
        }
        else
        {
            result << c;
            i += 1;
        }
    }

    return result.str();
}


///////////////////////////////////////////////////////////////////////////////
// TimeBasedRollingFileAppender ctors and dtor
///////////////////////////////////////////////////////////////////////////////

TimeBasedRollingFileAppender::TimeBasedRollingFileAppender(
    const tstring& filename_,
    const tstring& filenamePattern_,
    int maxHistory_,
    bool cleanHistoryOnStart_,
    bool immediateFlush_,
    bool createDirs_,
    bool rollOnClose_)
    : FileAppenderBase(filename_, std::ios_base::app, immediateFlush_, createDirs_)
    , filenamePattern(filenamePattern_)
    , schedule(DAILY)
    , maxHistory(maxHistory_)
    , cleanHistoryOnStart(cleanHistoryOnStart_)
    , rollOnClose(rollOnClose_)
{
    filenamePattern = preprocessFilenamePattern(filenamePattern, schedule);
    init();
}

TimeBasedRollingFileAppender::TimeBasedRollingFileAppender(
    const log4cplus::helpers::Properties& properties)
    : FileAppenderBase(properties, std::ios_base::app)
    , filenamePattern(LOG4CPLUS_TEXT("%d.log"))
    , schedule(DAILY)
    , maxHistory(10)
    , cleanHistoryOnStart(false)
    , rollOnClose(true)
{
    filenamePattern = properties.getProperty(LOG4CPLUS_TEXT("FilenamePattern"));
    properties.getInt(maxHistory, LOG4CPLUS_TEXT("MaxHistory"));
    properties.getBool(cleanHistoryOnStart, LOG4CPLUS_TEXT("CleanHistoryOnStart"));
    properties.getBool(rollOnClose, LOG4CPLUS_TEXT("RollOnClose"));
    filenamePattern = preprocessFilenamePattern(filenamePattern, schedule);

    init();
}

TimeBasedRollingFileAppender::~TimeBasedRollingFileAppender()
{
    destructorImpl();
}

///////////////////////////////////////////////////////////////////////////////
// TimeBasedRollingFileAppender protected methods
///////////////////////////////////////////////////////////////////////////////

void
TimeBasedRollingFileAppender::init()
{
    if (filenamePattern.empty())
    {
        getErrorHandler()->error( LOG4CPLUS_TEXT("Invalid filename/filenamePattern values") );
        return;
    }

    FileAppenderBase::init();

    Time now = helpers::now();
    nextRolloverTime = calculateNextRolloverTime(now);

    if (cleanHistoryOnStart)
    {
        clean(now + maxHistory*getRolloverPeriodDuration());
    }

    lastHeartBeat = now;
}

void
TimeBasedRollingFileAppender::append(const spi::InternalLoggingEvent& event)
{
    if(event.getTimestamp() >= nextRolloverTime) {
        rollover(true);
    }

    FileAppenderBase::append(event);
}

void
TimeBasedRollingFileAppender::open(std::ios_base::openmode mode)
{
    scheduledFilename = helpers::getFormattedTime(filenamePattern, helpers::now(), false);
    tstring currentFilename = filename.empty() ? scheduledFilename : filename;

    if (createDirs)
        internal::make_dirs (currentFilename);

    out.open(LOG4CPLUS_FSTREAM_PREFERED_FILE_NAME(currentFilename).c_str(), mode);
    if(!out.good())
    {
        getErrorHandler()->error(LOG4CPLUS_TEXT("Unable to open file: ") + currentFilename);
        return;
    }
    helpers::getLogLog().debug(LOG4CPLUS_TEXT("Just opened file: ") + currentFilename);
}

void
TimeBasedRollingFileAppender::close()
{
    if (rollOnClose)
        rollover();
    FileAppenderBase::close();
}

void
TimeBasedRollingFileAppender::rollover(bool alreadyLocked)
{
    helpers::LockFileGuard guard;

    if (useLockFile && ! alreadyLocked)
    {
        try
        {
            guard.attach_and_lock (*lockFile);
        }
        catch (std::runtime_error const &)
        {
            return;
        }
    }

    // Close the current file
    out.close();
    // reset flags since the C++ standard specified that all the flags
    // should remain unchanged on a close
    out.clear();

    if (! filename.empty())
    {
        helpers::LogLog & loglog = helpers::getLogLog();
        long ret;

#if defined (_WIN32)
        // Try to remove the target first. It seems it is not
        // possible to rename over existing file.
        ret = file_remove (scheduledFilename);
#endif

        loglog.debug(
            LOG4CPLUS_TEXT("Renaming file ")
            + filename
            + LOG4CPLUS_TEXT(" to ")
            + scheduledFilename);
        ret = file_rename (filename, scheduledFilename);
        loglog_renaming_result (loglog, filename, scheduledFilename, ret);
    }

    Time now = helpers::now();
    clean(now);

    open(std::ios::out | std::ios::trunc);

    nextRolloverTime = calculateNextRolloverTime(now);
}

void
TimeBasedRollingFileAppender::clean(Time time)
{
    Time::duration interval = std::chrono::hours{31*24}; // ~1 month
    if (lastHeartBeat != Time{})
    {
        interval = time - lastHeartBeat + std::chrono::seconds{1};
    }

    Time::duration period = getRolloverPeriodDuration();
    long periods = long(interval.count () / period.count ());

    helpers::LogLog & loglog = helpers::getLogLog();
    for (long i = 0; i < periods; i++)
    {
        long periodToRemove = (-maxHistory - 1) - i;
        Time timeToRemove = time + periodToRemove * period;
        tstring filenameToRemove = helpers::getFormattedTime(filenamePattern, timeToRemove, false);
        loglog.debug(LOG4CPLUS_TEXT("Removing file ") + filenameToRemove);
        file_remove(filenameToRemove);
    }

    lastHeartBeat = time;
}

Time::duration
TimeBasedRollingFileAppender::getRolloverPeriodDuration() const
{
    switch (schedule)
    {
    case MONTHLY:
        return std::chrono::hours{31*24};
    case WEEKLY:
        return std::chrono::hours{7*24};
    default:
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("TimeBasedRollingFileAppender::getRolloverPeriodDuration()-")
            LOG4CPLUS_TEXT (" invalid schedule value"));
        // Fall through.
    case DAILY:
        return std::chrono::hours{24};
    case HOURLY:
        return std::chrono::hours{1};
    case MINUTELY:
        return std::chrono::minutes{1};
    }
}

Time
TimeBasedRollingFileAppender::calculateNextRolloverTime(const Time& t) const
{
    return helpers::truncate_fractions (
        log4cplus::calculateNextRolloverTime (t, schedule));
}

///////////////////////////////////////////////////////////////////////////////
// SessionFileAppender and SessionRollingFileAppender utility functions
///////////////////////////////////////////////////////////////////////////////

static void cleanupOldSessionFiles(const tstring& filename,
    const tstring& filenamePrefix, int maxSessionCount)
{
    namespace fs = std::filesystem;

    if (maxSessionCount <= 0)
        return;

    fs::path pathFilename(filename);
    std::vector<tstring> vecFiles;
    auto pathDir = pathFilename.parent_path();
    for (const auto& p : fs::directory_iterator(pathDir))
    {
        if (!p.is_regular_file())
            continue;
        auto curFilename = p.path().filename().string();
        if (!curFilename.compare(0, filenamePrefix.size(), filenamePrefix))
            vecFiles.push_back(curFilename);
    }
    if (vecFiles.size() <= maxSessionCount)
        return;

    std::sort(vecFiles.begin(), vecFiles.end());
    std::set<tstring> protectedSessions;
    for (auto& iter = vecFiles.crbegin(); iter != vecFiles.crend(); ++iter)
    {
        tstring curFilename = *iter;

        tstring curSessionName;
        size_t dotPos = curFilename.find_last_of(LOG4CPLUS_TEXT("."));
        if (dotPos != curFilename.npos)
        {
            int backupIndex = 0;
            const auto res = std::from_chars(curFilename.data() + dotPos + 1,
                curFilename.data() + curFilename.size(), backupIndex);
            curSessionName = (res.ec == std::errc()) ? curFilename.substr(0, dotPos) : curFilename;
        }
        else
            curSessionName = curFilename;

        if (protectedSessions.find(curSessionName) != protectedSessions.end())
            continue;

        if (protectedSessions.size() < maxSessionCount)
        {
            protectedSessions.emplace(curSessionName);
            continue;
        }

        auto pathCurFile = pathDir / curFilename;
        auto res = file_remove(pathCurFile.string());
    }
}

///////////////////////////////////////////////////////////////////////////////
// SessionFileAppender
///////////////////////////////////////////////////////////////////////////////

SessionFileAppender::SessionFileAppender(const tstring& filename_,
    const tstring& filenamePrefix_, bool immediateFlush_, bool createDirs_,
    int maxSessionCount_)
    : FileAppender(filename_, std::ios_base::app, immediateFlush_, createDirs_)
    , filenamePrefix(filenamePrefix_)
    , maxSessionCount(maxSessionCount_)
{
}


SessionFileAppender::SessionFileAppender(const Properties& properties)
    : FileAppender(properties, std::ios_base::app)
    , filenamePrefix(LOG4CPLUS_TEXT(""))
    , maxSessionCount(-1)
{
    filenamePrefix = properties.getProperty(LOG4CPLUS_TEXT("FilenamePrefix"));
    properties.getInt(maxSessionCount, LOG4CPLUS_TEXT("MaxSessionCount"));
}


SessionFileAppender::~SessionFileAppender()
{
    destructorImpl();
}


void SessionFileAppender::close()
{
    if (maxSessionCount > 0)
        cleanupOldSessionFiles(filename, filenamePrefix, maxSessionCount);
    FileAppender::close();
}

///////////////////////////////////////////////////////////////////////////////
// SessionRollingFileAppender
///////////////////////////////////////////////////////////////////////////////

SessionRollingFileAppender::SessionRollingFileAppender(const tstring& filename_,
    const tstring& filenamePrefix_, long maxFileSize_, int maxBackupIndex_,
    bool immediateFlush_, bool createDirs_, int maxSessionCount_)
    : RollingFileAppender(filename_, maxFileSize_, maxBackupIndex_, immediateFlush_,
        createDirs_)
    , filenamePrefix(filenamePrefix_)
    , maxSessionCount(maxSessionCount_)
{
}


SessionRollingFileAppender::SessionRollingFileAppender(const Properties& properties)
    : RollingFileAppender(properties)
    , filenamePrefix(LOG4CPLUS_TEXT(""))
    , maxSessionCount(-1)
{
    filenamePrefix = properties.getProperty(LOG4CPLUS_TEXT("FilenamePrefix"));
    properties.getInt(maxSessionCount, LOG4CPLUS_TEXT("MaxSessionCount"));
}


SessionRollingFileAppender::~SessionRollingFileAppender()
{
    destructorImpl();
}


void SessionRollingFileAppender::close()
{
    if (maxSessionCount > 0)
        cleanupOldSessionFiles(filename, filenamePrefix, maxSessionCount);
    RollingFileAppender::close();
}

} // namespace log4cplus
