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

#include <log4cplus/helpers/connectorthread.h>
#include <log4cplus/helpers/loglog.h>


#if ! defined (LOG4CPLUS_SINGLE_THREADED)

namespace log4cplus { namespace helpers {


IConnectorThreadClient::~IConnectorThreadClient ()
{ }

//
//
//

ConnectorThread::ConnectorThread (
    IConnectorThreadClient & client)
    : ctc (client)
    , exit_flag (false)
{ }


ConnectorThread::~ConnectorThread ()
{ }


void
ConnectorThread::run ()
{
    while (true)
    {
        trigger_ev.timed_wait (30 * 1000);

        helpers::getLogLog().debug (
            LOG4CPLUS_TEXT("ConnectorThread::run()- running..."));

        // Check exit condition as the very first thing.

        {
            thread::MutexGuard guard (access_mutex);
            if (exit_flag)
                return;
            trigger_ev.reset ();
        }

        // Do not try to re-open already open socket.

        helpers::Socket & client_socket = ctc.ctcGetSocket ();
        thread::Mutex const & client_access_mutex = ctc.ctcGetAccessMutex ();
        {
            thread::MutexGuard guard (client_access_mutex);
            if (client_socket.isOpen ())
                continue;
        }

        // The socket is not open, try to reconnect.

        helpers::Socket new_socket (ctc.ctcConnect ());
        if (! new_socket.isOpen ())
        {
            helpers::getLogLog().error(
                LOG4CPLUS_TEXT("ConnectorThread::run()")
                LOG4CPLUS_TEXT("- Cannot connect to server"));

            // Sleep for a short while after unsuccessful connection attempt
            // so that we do not try to reconnect after each logging attempt
            // which could be many times per second.
            std::this_thread::sleep_for (std::chrono::seconds (5));

            continue;
        }

        // Connection was successful, move the socket into client.

        {
            thread::MutexGuard guard (client_access_mutex);
            client_socket = std::move (new_socket);
            ctc.ctcSetConnected ();
        }
    }
}


void
ConnectorThread::terminate ()
{
    {
        thread::MutexGuard guard (access_mutex);
        exit_flag = true;
        trigger_ev.signal ();
    }
    join ();
}


void
ConnectorThread::trigger ()
{
    trigger_ev.signal ();
}


} } // namespace log4cplus { namespace helpers {

#endif // ! defined (LOG4CPLUS_SINGLE_THREADED)
