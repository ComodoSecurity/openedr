// -*- C++ -*-
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

#ifndef LOG4CPLUS_HELPERS_CONNECTORTHREAD_H
#define LOG4CPLUS_HELPERS_CONNECTORTHREAD_H

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <log4cplus/thread/syncprims.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/helpers/socket.h>


#if ! defined (LOG4CPLUS_SINGLE_THREADED)

namespace log4cplus { namespace helpers {


class LOG4CPLUS_EXPORT ConnectorThread;

//! Interface implemented by users of ConnectorThread.
class LOG4CPLUS_EXPORT IConnectorThreadClient
{
protected:
    virtual ~IConnectorThreadClient ();

    //! \return Mutex for synchronization between ConnectorThread and
    //! its client object. This is usually SharedObject::access_mutex.
    virtual thread::Mutex const & ctcGetAccessMutex () const = 0;

    //! \return Socket variable in ConnectorThread client to maintain.
    virtual helpers::Socket & ctcGetSocket () = 0;

    //! \return ConnectorThread client's function returning connected
    //! socket.
    virtual helpers::Socket ctcConnect () = 0;

    //! Sets connected flag to true in ConnectorThread's client.
    virtual void ctcSetConnected () = 0;

    friend class LOG4CPLUS_EXPORT ConnectorThread;
};


//! This class is used by SocketAppender and (remote) SysLogAppender
//! to provide asynchronous re-connection.
class LOG4CPLUS_EXPORT ConnectorThread
    : public thread::AbstractThread
{
public:
    //! \param client reference to ConnectorThread's client object
    ConnectorThread (IConnectorThreadClient & client);
    virtual ~ConnectorThread ();
    
    virtual void run();

    //! Call this function to terminate ConnectorThread. The function
    //! sets `exit_flag` and then triggers `trigger_ev` to wake up the
    //! ConnectorThread.
    void terminate ();

    //! This function triggers (`trigger_ev`) connection check and
    //! attempt to re-connect a broken connection, when necessary.
    void trigger ();
    
protected:
    //! reference to ConnectorThread's client
    IConnectorThreadClient & ctc;

    //! This event is the re-connection trigger.
    thread::ManualResetEvent trigger_ev;

    //! When this variable set to true when ConnectorThread is signaled to 
    bool exit_flag;
};


} } // namespace log4cplus { namespace helpers {

#endif // ! defined (LOG4CPLUS_SINGLE_THREADED)

#endif // LOG4CPLUS_HELPERS_CONNECTORTHREAD_H
