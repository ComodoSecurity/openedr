// -*- C++ -*-
// Module:  Log4cplus
// File:    qt5debugappender.h
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


//

/** @file */

#ifndef LOG4CPLUS_QT5DEBUGAPPENDER_H
#define LOG4CPLUS_QT5DEBUGAPPENDER_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/appender.h>

#if defined (_WIN32)
  #if defined (log4cplusqt5debugappender_EXPORTS) \
      || defined (log4cplusqt5debugappenderU_EXPORTS) \
      || (defined (DLL_EXPORT) && defined (INSIDE_LOG4CPLUS_QT5DEBUGAPPENDER))
    #undef LOG4CPLUS_QT5DEBUGAPPENDER_BUILD_DLL
    #define LOG4CPLUS_QT5DEBUGAPPENDER_BUILD_DLL
  #endif
  #if defined (LOG4CPLUS_QT5DEBUGAPPENDER_BUILD_DLL)
    #if defined (INSIDE_LOG4CPLUS_QT5DEBUGAPPENDER)
      #define LOG4CPLUS_QT5DEBUGAPPENDER_EXPORT __declspec(dllexport)
    #else
      #define LOG4CPLUS_QT5DEBUGAPPENDER_EXPORT __declspec(dllimport)
    #endif
  #else
    #define LOG4CPLUS_QT5DEBUGAPPENDER_EXPORT
  #endif
#else
  #if defined (INSIDE_LOG4CPLUS_QT5DEBUGAPPENDER)
    #define LOG4CPLUS_QT5DEBUGAPPENDER_EXPORT LOG4CPLUS_DECLSPEC_EXPORT
  #else
    #define LOG4CPLUS_QT5DEBUGAPPENDER_EXPORT LOG4CPLUS_DECLSPEC_IMPORT
  #endif // defined (INSIDE_LOG4CPLUS_QT5DEBUGAPPENDER)
#endif // !_WIN32


namespace log4cplus
{


class LOG4CPLUS_QT5DEBUGAPPENDER_EXPORT Qt5DebugAppender
    : public Appender
{
public:
    Qt5DebugAppender ();
    explicit Qt5DebugAppender (helpers::Properties const &);
    virtual ~Qt5DebugAppender ();

    virtual void close ();

    static void registerAppender ();

protected:
    virtual void append (spi::InternalLoggingEvent const &);

private:
    Qt5DebugAppender (Qt5DebugAppender const &);
    Qt5DebugAppender & operator = (Qt5DebugAppender const &);
};


typedef helpers::SharedObjectPtr<Qt5DebugAppender> Qt5DebugAppenderPtr;


} // namespace log4cplus


#endif // LOG4CPLUS_QT5DEBUGAPPENDER_H
