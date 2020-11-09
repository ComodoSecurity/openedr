// Module:  Log4cplus
// File:    qt5debugappender.cxx
// Created: 4/2013
// Author:  Vaclav Zeman
//
//
//  Copyright (C) 2013-2017, Vaclav Zeman. All rights reserved.
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

#include <log4cplus/config.hxx>
#include <log4cplus/qt5debugappender.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/helpers/property.h>
#include <log4cplus/spi/factory.h>
#include <log4cplus/spi/loggingevent.h>
#include <sstream>
#include <iomanip>
#include <QtGlobal>
#include <log4cplus/config/windowsh-inc.h>


// Forward Declarations
namespace log4cplus
{


Qt5DebugAppender::Qt5DebugAppender ()
    : Appender ()
{ }


Qt5DebugAppender::Qt5DebugAppender (helpers::Properties const & props)
    : Appender (props)
{ }


Qt5DebugAppender::~Qt5DebugAppender ()
{
    destructorImpl ();
}


void
Qt5DebugAppender::close ()
{ }


void
Qt5DebugAppender::append (spi::InternalLoggingEvent const & ev)
{
    // TODO: Expose log4cplus' internal TLS to use here.
    tostringstream oss;    
    layout->formatAndAppend(oss, ev);

    LogLevel const ll = ev.getLogLevel ();
    std::string const & file = LOG4CPLUS_TSTRING_TO_STRING (ev.getFile ());
    std::string const & func = LOG4CPLUS_TSTRING_TO_STRING (ev.getFunction ());
    std::string const & logger
        = LOG4CPLUS_TSTRING_TO_STRING (ev.getLoggerName ());
    int const line = ev.getLine ();

    QMessageLogger qlogger (file.c_str (), line, func.c_str (),
        logger.c_str ());
    void (QMessageLogger:: * log_func) (const char *, ...) const = 0;

    if (ll >= ERROR_LOG_LEVEL)
        log_func = &QMessageLogger::critical;
    else if (ll >= WARN_LOG_LEVEL)
        log_func = &QMessageLogger::warning;
    else
        log_func = &QMessageLogger::debug;
    
    (qlogger.*log_func) ("%s",
        LOG4CPLUS_TSTRING_TO_STRING (oss.str ()).c_str ());
}


void
Qt5DebugAppender::registerAppender ()
{
    log4cplus::spi::AppenderFactoryRegistry & reg
        = log4cplus::spi::getAppenderFactoryRegistry ();
    LOG4CPLUS_REG_APPENDER (reg, Qt5DebugAppender);
}


} // namespace log4cplus


#if defined (_WIN32)
extern "C"
BOOL WINAPI DllMain(LOG4CPLUS_DLLMAIN_HINSTANCE,  // handle to DLL module
                    DWORD fdwReason,     // reason for calling function
                    LPVOID)  // reserved
{
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
    case DLL_PROCESS_ATTACH:
    {
        // We cannot do this here because it causes the thread to deadlock 
        // when compiled with Visual Studio due to use of C++11 threading 
        // facilities.

        //log4cplus::Qt5DebugAppender::registerAppender ();
        break;
    }

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

#endif // defined (_WIN32)
