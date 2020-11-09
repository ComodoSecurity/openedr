// -*- C++ -*-
// Module:  Log4cplus
// File:    msttsappender.h
// Created: 10/2012
// Author:  Vaclav Zeman
//
//
//  Copyright (C) 2012-2017, Vaclav Zeman. All rights reserved.
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

#ifndef LOG4CPLUS_MSTTSAPPENDER_H
#define LOG4CPLUS_MSTTSAPPENDER_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/appender.h>


#if defined (_WIN32)
  #if defined (log4cplusqt4debugappender_EXPORTS) \
      || defined (log4cplusqt4debugappenderU_EXPORTS) \
      || (defined (DLL_EXPORT) && defined (INSIDE_LOG4CPLUS_MSTTSAPPENDER))
    #undef LOG4CPLUS_MSTTSAPPENDER_BUILD_DLL
    #define LOG4CPLUS_MSTTSAPPENDER_BUILD_DLL
  #endif
  #if defined (LOG4CPLUS_MSTTSAPPENDER_BUILD_DLL)
    #if defined (INSIDE_LOG4CPLUS_MSTTSAPPENDER)
      #define LOG4CPLUS_MSTTSAPPENDER_EXPORT __declspec(dllexport)
    #else
      #define LOG4CPLUS_MSTTSAPPENDER_EXPORT __declspec(dllimport)
    #endif
  #else
    #define LOG4CPLUS_MSTTSAPPENDER_EXPORT
  #endif
#else
  #if defined (INSIDE_LOG4CPLUS_MSTTSAPPENDER)
    #define LOG4CPLUS_MSTTSAPPENDER_EXPORT LOG4CPLUS_DECLSPEC_EXPORT
  #else
    #define LOG4CPLUS_MSTTSAPPENDER_EXPORT LOG4CPLUS_DECLSPEC_IMPORT
  #endif // defined (INSIDE_LOG4CPLUS_MSTTSAPPENDER)
#endif // !_WIN32


namespace log4cplus
{


class LOG4CPLUS_MSTTSAPPENDER_EXPORT MSTTSAppender
    : public Appender
{
public:
    MSTTSAppender ();
    explicit MSTTSAppender (helpers::Properties const &);
    virtual ~MSTTSAppender ();

    virtual void close ();

    static void registerAppender ();

protected:
    virtual void append (spi::InternalLoggingEvent const &);

    struct Data;

    Data * data;

private:
    LOG4CPLUS_PRIVATE void init (long const * rate = 0,
        unsigned long const * volume = 0, bool speak_punc = false,
        bool async = false);

    MSTTSAppender (MSTTSAppender const &);
    MSTTSAppender & operator = (MSTTSAppender const &);
};


typedef helpers::SharedObjectPtr<MSTTSAppender> MSTTSAppenderPtr;


} // namespace log4cplus


#endif // LOG4CPLUS_MSTTSAPPENDER_H
