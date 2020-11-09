// -*- C++ -*-
// Module:  Log4CPLUS
// File:    fileappender.h
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

#ifndef LOG4CPLUS_FILE_APPENDER_HEADER_
#define LOG4CPLUS_FILE_APPENDER_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/appender.h>
#include <log4cplus/fstreams.h>
#include <log4cplus/helpers/timehelper.h>
#include <log4cplus/helpers/lockfile.h>
#include <fstream>
#include <locale>
#include <memory>


namespace log4cplus
{

    /**
     * Base class for Appenders writing log events to a file.
     * It is constructed with uninitialized file object, so all
     * classes derived from FileAppenderBase _must_ call init() method.
     *
     * <h3>Properties</h3>
     * <dl>
     * <dt><tt>File</tt></dt>
     * <dd>This property specifies output file name.</dd>
     *
     * <dt><tt>ImmediateFlush</tt></dt>
     * <dd>When it is set true, output stream will be flushed after
     * each appended event.</dd>
     *
     * <dt><tt>Append</tt></dt>
     * <dd>When it is set true, output file will be appended to
     * instead of being truncated at opening.</dd>
     *
     * <dt><tt>ReopenDelay</tt></dt>
     * <dd>This property sets a delay after which the appender will try
     * to reopen log file again, after last logging failure.
     * </dd>
     *
     * <dt><tt>BufferSize</tt></dt>
     * <dd>Non-zero value of this property sets up buffering of output
     * stream using a buffer of given size.
     * </dd>
     *
     * <dt><tt>UseLockFile</tt></dt>
     * <dd>Set this property to <tt>true</tt> if you want your output
     * to go into a log file shared by multiple processes. When this
     * property is set to true then log4cplus uses OS specific
     * facilities (e.g., <code>lockf()</code>) to provide
     * inter-process file locking.
     * \sa Appender
     * </dd>
     *
     * <dt><tt>LockFile</tt></dt>
     * <dd>This property specifies lock file, file used for
     * inter-process synchronization of log file access. When this
     * property is not specified, the value is derived from
     * <tt>File</tt> property by addition of ".lock" suffix. The
     * property is only used when <tt>UseLockFile</tt> is set to true.
     * \sa Appender
     * </dd>
     *
     * <dt><tt>Locale</tt></dt>
     * <dd>This property specifies a locale name that will be imbued
     * into output stream. Locale can be specified either by system
     * specific locale name, e.g., <tt>en_US.UTF-8</tt>, or by one of
     * four recognized keywords: <tt>GLOBAL</tt>, <tt>DEFAULT</tt>
     * (which is an alias for <tt>GLOBAL</tt>), <tt>USER</tt> and
     * <tt>CLASSIC</tt>. When specified locale is not available,
     * <tt>GLOBAL</tt> is used instead. It is possible to register
     * additional locale keywords by registering an instance of
     * <code>spi::LocaleFactory</code> in
     * <code>spi::LocaleFactoryRegistry</code>.
     * \sa spi::getLocaleFactoryRegistry()
     * </dd>
     *
     * <dt><tt>CreateDirs</tt></dt>
     * <dd>Set this property to <tt>true</tt> if you want to create
     * missing directories in path leading to log file and lock file.
     * </dd>
     *
     * <dt><tt>TextMode</tt></dt>
     * <dd>Set this property to <tt>Binary</tt> if the underlying stream should
     * not translate EOLs to OS specific character sequence. The default value
     * is <tt>Text</tt> and the underlying stream will be opened in text
     * mode.</dd>
     * </dl>
     */
    class LOG4CPLUS_EXPORT FileAppenderBase : public Appender {
    public:
      // Methods
        virtual void close();

      //! Redefine default locale for output stream. It may be a good idea to
      //! provide UTF-8 locale in case UNICODE macro is defined.
        virtual std::locale imbue(std::locale const& loc);

      //! \returns Locale imbued in fstream.
        virtual std::locale getloc () const;

    protected:
      // Ctors
        FileAppenderBase(const log4cplus::tstring& filename,
                         std::ios_base::openmode mode = std::ios_base::trunc,
                         bool immediateFlush = true,
                         bool createDirs = false);
        FileAppenderBase(const log4cplus::helpers::Properties& properties,
                         std::ios_base::openmode mode = std::ios_base::trunc);

        void init();

        virtual void append(const spi::InternalLoggingEvent& event);

        virtual void open(std::ios_base::openmode mode);
        bool reopen();

      // Data
        /**
         * Immediate flush means that the underlying writer or output stream
         * will be flushed at the end of each append operation. Immediate
         * flush is slower but ensures that each append request is actually
         * written. If <code>immediateFlush</code> is set to
         * <code>false</code>, then there is a good chance that the last few
         * logs events are not actually written to persistent media if and
         * when the application crashes.
         *
         * The <code>immediateFlush</code> variable is set to
         * <code>true</code> by default.
         */
        bool immediateFlush;

        /**
         * When this variable is true, FileAppender will try to create
         * missing directories in path leading to log file.
         *
         * The `createDirs` variable is set to `false` by default.
         */
        bool createDirs;

        /**
         * When any append operation fails, <code>reopenDelay</code> says
         * for how many seconds the next attempt to re-open the log file and
         * resume logging will be delayed. If <code>reopenDelay</code> is zero,
         * each failed append operation will cause log file to be re-opened.
         * By default, <code>reopenDelay</code> is 1 second.
         */
        int reopenDelay;

        unsigned long bufferSize;
        std::unique_ptr<log4cplus::tchar[]> buffer;

        log4cplus::tofstream out;
        log4cplus::tstring filename;
        log4cplus::tstring localeName;
        log4cplus::tstring lockFileName;
        std::ios_base::openmode fileOpenMode;

        log4cplus::helpers::Time reopen_time;

    private:
      // Disallow copying of instances of this class
        FileAppenderBase(const FileAppenderBase&);
        FileAppenderBase& operator=(const FileAppenderBase&);
    };


    /**
     * Appends log events to a file.
     *
     * <h3>Properties</h3>
     * <p>It has no properties additional to {@link FileAppenderBase}.
     */
    class LOG4CPLUS_EXPORT FileAppender : public FileAppenderBase {
    public:
      // Ctors
        FileAppender(const log4cplus::tstring& filename,
                     std::ios_base::openmode mode = std::ios_base::trunc,
                     bool immediateFlush = true,
                     bool createDirs = false);
        FileAppender(const log4cplus::helpers::Properties& properties,
                     std::ios_base::openmode mode = std::ios_base::trunc);

      // Dtor
        virtual ~FileAppender();

    protected:
        void init();
    };

    typedef helpers::SharedObjectPtr<FileAppender> SharedFileAppenderPtr;



    /**
     * RollingFileAppender extends FileAppender to backup the log
     * files when they reach a certain size.
     *
     * <h3>Properties</h3>
     * <p>Properties additional to {@link FileAppender}'s properties:
     *
     * <dl>
     * <dt><tt>MaxFileSize</tt></dt>
     * <dd>This property specifies maximal size of output file. The
     * value is in bytes. It is possible to use <tt>MB</tt> and
     * <tt>KB</tt> suffixes to specify the value in megabytes or
     * kilobytes instead.</dd>
     *
     * <dt><tt>MaxBackupIndex</tt></dt>
     * <dd>This property limits the number of backup output
     * files; e.g. how many <tt>log.1</tt>, <tt>log.2</tt> etc. files
     * will be kept.</dd>
     * </dl>
     */
    class LOG4CPLUS_EXPORT RollingFileAppender : public FileAppender {
    public:
      // Ctors
        RollingFileAppender(const log4cplus::tstring& filename,
                            long maxFileSize = 10*1024*1024, // 10 MB
                            int maxBackupIndex = 1,
                            bool immediateFlush = true,
                            bool createDirs = false);
        RollingFileAppender(const log4cplus::helpers::Properties& properties);

      // Dtor
        virtual ~RollingFileAppender();

    protected:
        virtual void append(const spi::InternalLoggingEvent& event);
        void rollover(bool alreadyLocked = false);

      // Data
        long maxFileSize;
        int maxBackupIndex;

    private:
        LOG4CPLUS_PRIVATE void init(long maxFileSize, int maxBackupIndex);
    };


    typedef helpers::SharedObjectPtr<RollingFileAppender>
        SharedRollingFileAppenderPtr;


    enum DailyRollingFileSchedule { MONTHLY, WEEKLY, DAILY,
                                    TWICE_DAILY, HOURLY, MINUTELY};

    /**
     * DailyRollingFileAppender extends {@link FileAppender} so that the
     * underlying file is rolled over at a user chosen frequency.
     *
     * <h3>Properties</h3>
     * <p>Properties additional to {@link FileAppender}'s properties:
     *
     * <dl>
     * <dt><tt>Schedule</tt></dt>
     * <dd>This property specifies rollover schedule. The possible
     * values are <tt>MONTHLY</tt>, <tt>WEEKLY</tt>, <tt>DAILY</tt>,
     * <tt>TWICE_DAILY</tt>, <tt>HOURLY</tt> and
     * <tt>MINUTELY</tt>.</dd>
     *
     * <dt><tt>MaxBackupIndex</tt></dt>
     * <dd>This property limits how many backup files are kept per
     * single logging period; e.g. how many <tt>log.2009-11-07.1</tt>,
     * <tt>log.2009-11-07.2</tt> etc. files are kept.</dd>
     *
     * <dt><tt>RollOnClose</tt></dt>
     * <dd>This property specifies whether to rollover log files upon
     * shutdown. By default it's set to <code>true</code> to retain compatibility
     * with legacy code, however it may lead to undesired behaviour
     * as described in the github issue #120.</dd>
     *
     * <dt><tt>DatePattern</tt></dt>
     * <dd>This property specifies filename suffix pattern to use for
     * periodical backups of the logfile. The patern should be in
     * format supported by {@link log4cplus::helpers::Time::getFormatterTime()}</code>.
     * Please notice that the format of the pattern is similar but not identical
     * to the one used for this option in the corresponding Log4J class.
     * If the property isn't specified a reasonable default for a given
     * schedule type is used.</dd>
     *
     * </dl>
     */
    class LOG4CPLUS_EXPORT DailyRollingFileAppender : public FileAppender {
    public:
      // Ctors
        DailyRollingFileAppender(const log4cplus::tstring& filename,
                                 DailyRollingFileSchedule schedule = DAILY,
                                 bool immediateFlush = true,
                                 int maxBackupIndex = 10,
                                 bool createDirs = false,
                                 bool rollOnClose = true,
                                 const log4cplus::tstring& datePattern = log4cplus::tstring());
        DailyRollingFileAppender(const log4cplus::helpers::Properties& properties);

      // Dtor
        virtual ~DailyRollingFileAppender();

      // Methods
        virtual void close();

    protected:
        virtual void append(const spi::InternalLoggingEvent& event);
        void rollover(bool alreadyLocked = false);
        log4cplus::helpers::Time calculateNextRolloverTime(const log4cplus::helpers::Time& t) const;
        log4cplus::tstring getFilename(const log4cplus::helpers::Time& t) const;

      // Data
        DailyRollingFileSchedule schedule;
        log4cplus::tstring scheduledFilename;
        log4cplus::helpers::Time nextRolloverTime;
        int maxBackupIndex;
        bool rollOnClose;
        log4cplus::tstring datePattern;

    private:
        LOG4CPLUS_PRIVATE void init(DailyRollingFileSchedule schedule);
    };

    typedef helpers::SharedObjectPtr<DailyRollingFileAppender>
        SharedDailyRollingFileAppenderPtr;


    /**
     * TimeBasedRollingFileAppender extends {@link FileAppenderBase} so that
     * the underlying file is rolled over at a user chosen frequency while also
     * keeping in check a total maximum number of produced files.
     *
     * <h3>Properties</h3>
     * <p>Properties additional to {@link FileAppenderBase}'s properties:
     *
     * <dl>
     *
     * <dt><tt>FilenamePattern</tt></dt>
     * <dd>The mandatory fileNamePattern property defines the name of the
     * rolled-over (archived) log files. Its value should consist of the name
     * of the file, plus a suitably placed %d conversion specifier. The %d
     * conversion specifier may contain a date-and-time pattern as specified by
     * the java's SimpleDateFormat.  The rollover period is inferred from the
     * value of fileNamePattern.</dd>
     *
     * <dt><tt>MaxHistory</tt></dt>
     * <dd>The optional maxHistory property controls the maximum number of
     * archive files to keep, deleting older files.</dd>
     *
     * <dt><tt>CleanHistoryOnStart</tt></dt>
     * <dd>If set to true, archive removal will be executed on appender start
     * up.  By default this property is set to false. </dd>
     *
     * <dt><tt>RollOnClose</tt></dt>
     * <dd>This property specifies whether to rollover log files upon
     * shutdown. By default it's set to <code>true</code> to retain compatibility
     * with legacy code, however it may lead to undesired behaviour
     * as described in the github issue #120.</dd>
     *
     * </dl>
     */
    class LOG4CPLUS_EXPORT TimeBasedRollingFileAppender : public FileAppenderBase {
    public:
      // Ctors
        TimeBasedRollingFileAppender(const tstring& filename = LOG4CPLUS_TEXT(""),
                                     const tstring& filenamePattern = LOG4CPLUS_TEXT("%d.log"),
                                     int maxHistory = 10,
                                     bool cleanHistoryOnStart = false,
                                     bool immediateFlush = true,
                                     bool createDirs = false,
                                     bool rollOnClose = true);
        TimeBasedRollingFileAppender(const helpers::Properties& properties);

      // Dtor
        ~TimeBasedRollingFileAppender();

    protected:
        void append(const spi::InternalLoggingEvent& event);
        void open(std::ios_base::openmode mode);
        void close();
        void rollover(bool alreadyLocked = false);
        void clean(helpers::Time time);
        helpers::Time::duration getRolloverPeriodDuration() const;
        helpers::Time calculateNextRolloverTime(const helpers::Time& t) const;

      // Data
        tstring filenamePattern;
        DailyRollingFileSchedule schedule;
        tstring scheduledFilename;
        int maxHistory;
        bool cleanHistoryOnStart;
        log4cplus::helpers::Time lastHeartBeat;
        log4cplus::helpers::Time nextRolloverTime;
        bool rollOnClose;

    private:
        LOG4CPLUS_PRIVATE void init();
    };

    typedef helpers::SharedObjectPtr<TimeBasedRollingFileAppender>
        SharedTimeBasedRollingFileAppenderPtr;

    /**
     * SessionFileAppender extends FileAppender to create the separate log file
     * for current app session and optional cleanup of log files from old sessions.
     *
     * <h3>Properties</h3>
     * <p>Properties additional to {@link FileAppender}'s properties:
     *
     * <dt><tt>FilenamePrefix</tt></dt>
     * <dd>This property specifies filename prefix to identify the group of sessions
     * for further cleanup.</dd>
     *
     * <dt><tt>MaxSessionCount</tt></dt>
     * <dd>This property limits the number of session files named with specified
     * prefix. By default, the value -1 is to store all the log files
     * (cleanup is off).</dd>
     * </dl>
     */
    class LOG4CPLUS_EXPORT SessionFileAppender : public FileAppender {
    public:
        // Ctors
        SessionFileAppender(const tstring& filename = LOG4CPLUS_TEXT(""),
            const tstring& filenamePrefix = LOG4CPLUS_TEXT(""),
            bool immediateFlush = true,
            bool createDirs = false,
            int maxSessionCount = -1);
        SessionFileAppender(const log4cplus::helpers::Properties& properties);

        // Dtor
        virtual ~SessionFileAppender();

        // Methods
        void close() override;

    protected:
        tstring filenamePrefix;
        int maxSessionCount;
    };

    typedef helpers::SharedObjectPtr<SessionFileAppender>
        SharedSessionFileAppenderPtr;

    /**
     * SessionRollingFileAppender extends RollingFileAppender to create
     * the separate log files for current app session and optional cleanup
     * of old log files.
     *
     * <h3>Properties</h3>
     * <p>Properties additional to {@link RollingFileAppender}'s properties:
     *
     * <dt><tt>FilenamePrefix</tt></dt>
     * <dd>This property specifies filename prefix to identify the group of sessions
     * for further cleanup.</dd>
     *
     * <dt><tt>MaxSessionCount</tt></dt>
     * <dd>This property limits the number of session files named with specified
     * prefix. By default, the value -1 is to store all the log files
     * (cleanup is off).</dd>
     * </dl>
     */
    class LOG4CPLUS_EXPORT SessionRollingFileAppender : public RollingFileAppender {
    public:
        // Ctors
        SessionRollingFileAppender(const tstring& filename = LOG4CPLUS_TEXT(""),
            const tstring& filenamePrefix = LOG4CPLUS_TEXT(""),
            long maxFileSize = 10 * 1024 * 1024, // 10 MB
            int maxBackupIndex = 1,
            bool immediateFlush = true,
            bool createDirs = false,
            int maxSessionCount = -1);
        SessionRollingFileAppender(const log4cplus::helpers::Properties& properties);

        // Dtor
        virtual ~SessionRollingFileAppender();

        // Methods
        void close() override;

    protected:
        tstring filenamePrefix;
        int maxSessionCount;
    };

    typedef helpers::SharedObjectPtr<SessionRollingFileAppender>
        SharedSessionRollingFileAppenderPtr;

} // end namespace log4cplus

#endif // LOG4CPLUS_FILE_APPENDER_HEADER_
