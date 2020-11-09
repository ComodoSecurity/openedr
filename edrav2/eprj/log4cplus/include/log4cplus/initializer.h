//  Copyright (C) 2015-2017, Vaclav Haisman. All rights reserved.
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

#ifndef LOG4CPLUS_INITIALIZER_HXX
#define LOG4CPLUS_INITIALIZER_HXX

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <memory>


namespace log4cplus
{

/**
   This class helps with initialization and shutdown of log4cplus. Its
   constructor calls `log4cplus::initialize()` and its destructor calls
   `log4cplus::Logger::shutdown()`. Use this class as the first thing in your
   `main()`. It will ensure shutdown of log4cplus at the end of
   `main()`. This is particularly important on Windows, where shutdown of
   standard threads outside `main()` is impossible.
 */
class LOG4CPLUS_EXPORT Initializer
{
public:
    Initializer ();
    ~Initializer ();

    Initializer (Initializer const &) = delete;
    Initializer (Initializer &&) = delete;
    Initializer & operator = (Initializer const &) = delete;
    Initializer & operator = (Initializer &&) = delete;
};

} // namespace log4cplus


#endif // LOG4CPLUS_INITIALIZER_HXX
