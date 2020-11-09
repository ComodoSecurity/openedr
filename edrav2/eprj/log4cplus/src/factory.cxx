// Module:  Log4CPLUS
// File:    factory.cxx
// Created: 2/2002
// Author:  Tad E. Smith
//
//
// Copyright 2002-2017 Tad E. Smith
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

#include <log4cplus/spi/factory.h>
#include <log4cplus/spi/loggerfactory.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/thread-config.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/asyncappender.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/nteventlogappender.h>
#include <log4cplus/nullappender.h>
#include <log4cplus/socketappender.h>
#include <log4cplus/syslogappender.h>
#include <log4cplus/win32debugappender.h>
#include <log4cplus/win32consoleappender.h>
#include <log4cplus/log4judpappender.h>


///////////////////////////////////////////////////////////////////////////////
// LOCAL file class definitions
///////////////////////////////////////////////////////////////////////////////

namespace log4cplus {

namespace spi {

BaseFactory::~BaseFactory()
{ }


AppenderFactory::AppenderFactory()
{ }

AppenderFactory::~AppenderFactory()
{ }


LayoutFactory::LayoutFactory()
{ }

LayoutFactory::~LayoutFactory()
{ }


FilterFactory::FilterFactory()
{ }

FilterFactory::~FilterFactory()
{ }


LocaleFactory::LocaleFactory()
{ }

LocaleFactory::~LocaleFactory()
{ }


LoggerFactory::~LoggerFactory()
{ }


namespace
{

class GlobalLocale
    : public LocalFactoryBase<LocaleFactory>
{
public:
    explicit
    GlobalLocale (tchar const * n)
        : LocalFactoryBase<LocaleFactory> (n)
    { }

    ProductPtr
    createObject (const log4cplus::helpers::Properties &) override
    {
        return std::locale ();
    }
};


class UserLocale
    : public LocalFactoryBase<LocaleFactory>
{
public:
    explicit
    UserLocale (tchar const * n)
        : LocalFactoryBase<LocaleFactory> (n)
    { }

    ProductPtr
    createObject (const log4cplus::helpers::Properties &) override
    {
        return std::locale ("");
    }
};


class ClassicLocale
    : public LocalFactoryBase<LocaleFactory>
{
public:
    explicit
    ClassicLocale (tchar const * n)
        : LocalFactoryBase<LocaleFactory> (n)
    { }

    ProductPtr
    createObject (const log4cplus::helpers::Properties &) override
    {
        return std::locale::classic ();
    }
};


} // namespace




} // namespace spi


namespace
{

template <typename Factory>
struct DisableFactoryLocking
{
    typedef Factory factory_type;

    explicit
    DisableFactoryLocking (factory_type & f)
        : factory (f)
    {
        factory._enableLocking (false);
    }

    ~DisableFactoryLocking ()
    {
        factory._enableLocking (true);
    }

    factory_type & factory;
};

} // namespace


void initializeFactoryRegistry()
{
    spi::AppenderFactoryRegistry& reg = spi::getAppenderFactoryRegistry();
    DisableFactoryLocking<spi::AppenderFactoryRegistry> dfl_reg (reg);
    LOG4CPLUS_REG_APPENDER (reg, ConsoleAppender);
    LOG4CPLUS_REG_APPENDER (reg, NullAppender);
    LOG4CPLUS_REG_APPENDER (reg, FileAppender);
    LOG4CPLUS_REG_APPENDER (reg, RollingFileAppender);
    LOG4CPLUS_REG_APPENDER (reg, DailyRollingFileAppender);
    LOG4CPLUS_REG_APPENDER (reg, TimeBasedRollingFileAppender);
    LOG4CPLUS_REG_APPENDER (reg, SocketAppender);
    LOG4CPLUS_REG_APPENDER (reg, SessionFileAppender);
    LOG4CPLUS_REG_APPENDER (reg, SessionRollingFileAppender);
#if defined(_WIN32)
#  if defined(LOG4CPLUS_HAVE_NT_EVENT_LOG)
    LOG4CPLUS_REG_APPENDER (reg, NTEventLogAppender);
#  endif
#  if defined(LOG4CPLUS_HAVE_WIN32_CONSOLE)
    LOG4CPLUS_REG_APPENDER (reg, Win32ConsoleAppender);
#  endif
    LOG4CPLUS_REG_APPENDER (reg, Win32DebugAppender);
#endif
    LOG4CPLUS_REG_APPENDER (reg, SysLogAppender);
#ifndef LOG4CPLUS_SINGLE_THREADED
    LOG4CPLUS_REG_APPENDER (reg, AsyncAppender);
#endif
    LOG4CPLUS_REG_APPENDER (reg, Log4jUdpAppender);

    spi::LayoutFactoryRegistry& reg2 = spi::getLayoutFactoryRegistry();
    DisableFactoryLocking<spi::LayoutFactoryRegistry> dfl_reg2 (reg2);
    LOG4CPLUS_REG_LAYOUT (reg2, SimpleLayout);
    LOG4CPLUS_REG_LAYOUT (reg2, TTCCLayout);
    LOG4CPLUS_REG_LAYOUT (reg2, PatternLayout);

    spi::FilterFactoryRegistry& reg3 = spi::getFilterFactoryRegistry();
    DisableFactoryLocking<spi::FilterFactoryRegistry> dfl_reg3 (reg3);
    LOG4CPLUS_REG_FILTER (reg3, DenyAllFilter);
    LOG4CPLUS_REG_FILTER (reg3, LogLevelMatchFilter);
    LOG4CPLUS_REG_FILTER (reg3, LogLevelRangeFilter);
    LOG4CPLUS_REG_FILTER (reg3, StringMatchFilter);
    LOG4CPLUS_REG_FILTER (reg3, NDCMatchFilter);
    LOG4CPLUS_REG_FILTER (reg3, MDCMatchFilter);

    spi::LocaleFactoryRegistry& reg4 = spi::getLocaleFactoryRegistry();
    DisableFactoryLocking<spi::LocaleFactoryRegistry> dfl_reg4 (reg4);
    LOG4CPLUS_REG_LOCALE (reg4, LOG4CPLUS_TEXT("GLOBAL"), spi::GlobalLocale);
    LOG4CPLUS_REG_LOCALE (reg4, LOG4CPLUS_TEXT("DEFAULT"), spi::GlobalLocale);
    LOG4CPLUS_REG_LOCALE (reg4, LOG4CPLUS_TEXT("USER"), spi::UserLocale);
    LOG4CPLUS_REG_LOCALE (reg4, LOG4CPLUS_TEXT("CLASSIC"), spi::ClassicLocale);
}


} // namespace log4cplus
