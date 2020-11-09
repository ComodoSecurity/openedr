// Module:  Log4CPLUS
// File:    socket.cxx
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

#include <log4cplus/helpers/loglog.h>
#include <log4cplus/internal/socket.h>
#include <log4cplus/internal/internal.h>


namespace log4cplus { namespace helpers {


extern LOG4CPLUS_EXPORT SOCKET_TYPE const INVALID_SOCKET_VALUE
#if defined(_WIN32)
    = static_cast<SOCKET_TYPE>(INVALID_SOCKET);
#else
    = static_cast<SOCKET_TYPE>(-1);
#endif


//////////////////////////////////////////////////////////////////////////////
// AbstractSocket ctors and dtor
//////////////////////////////////////////////////////////////////////////////

AbstractSocket::AbstractSocket()
    : sock(INVALID_SOCKET_VALUE),
      state(not_opened),
      err(0)
{
}



AbstractSocket::AbstractSocket(SOCKET_TYPE sock_, SocketState state_, int err_)
    : sock(sock_),
      state(state_),
      err(err_)
{
}



AbstractSocket::AbstractSocket(AbstractSocket && rhs) LOG4CPLUS_NOEXCEPT
    : AbstractSocket ()
{
    swap (rhs);
}


AbstractSocket::~AbstractSocket()
{
    close();
}



//////////////////////////////////////////////////////////////////////////////
// AbstractSocket methods
//////////////////////////////////////////////////////////////////////////////

void
AbstractSocket::close()
{
    if (sock != INVALID_SOCKET_VALUE)
    {
        closeSocket(sock);
        sock = INVALID_SOCKET_VALUE;
        state = not_opened;
    }
}


void
AbstractSocket::shutdown()
{
    if (sock != INVALID_SOCKET_VALUE)
        shutdownSocket(sock);
}


bool
AbstractSocket::isOpen() const
{
    return sock != INVALID_SOCKET_VALUE;
}


AbstractSocket &
AbstractSocket::operator = (AbstractSocket && rhs) LOG4CPLUS_NOEXCEPT
{
    swap (rhs);
    return *this;
}


void
AbstractSocket::swap (AbstractSocket & rhs)
{
    using std::swap;

    swap (sock, rhs.sock);
    swap (state, rhs.state);
    swap (err, rhs.err);
}


//////////////////////////////////////////////////////////////////////////////
// Socket ctors and dtor
//////////////////////////////////////////////////////////////////////////////

Socket::Socket()
    : AbstractSocket()
{ }


Socket::Socket(const tstring& address, unsigned short port,
    bool udp /*= false*/, bool ipv6 /*= false */)
    : AbstractSocket()
{
    sock = connectSocket(address, port, udp, ipv6, state);
    if (sock == INVALID_SOCKET_VALUE)
        goto error;

    if (! udp && setTCPNoDelay (sock, true) != 0)
        goto error;

    return;

error:
    err = get_last_socket_error ();
}


Socket::Socket(SOCKET_TYPE sock_, SocketState state_, int err_)
    : AbstractSocket(sock_, state_, err_)
{ }


Socket::Socket (Socket && other) LOG4CPLUS_NOEXCEPT
    : AbstractSocket (std::move (other))
{ }


Socket::~Socket()
{ }


Socket &
Socket::operator = (Socket && other) LOG4CPLUS_NOEXCEPT
{
    swap (other);
    return *this;
}


//////////////////////////////////////////////////////////////////////////////
// Socket methods
//////////////////////////////////////////////////////////////////////////////

bool
Socket::read(SocketBuffer& buffer)
{
    long retval = helpers::read(sock, buffer);
    if(retval <= 0) {
        close();
    }
    else {
        buffer.setSize(retval);
    }

    return (retval > 0);
}



bool
Socket::write(const SocketBuffer& buffer)
{
    long retval = helpers::write(sock, buffer);
    if(retval <= 0) {
        close();
    }

    return (retval > 0);
}


bool
Socket::write(std::size_t bufferCount, SocketBuffer const * const * buffers)
{
    long retval = helpers::write(sock, bufferCount, buffers);
    if (retval <= 0)
        close ();

    return retval > 0;
}


bool
Socket::write(const std::string & buffer)
{
    long retval = helpers::write (sock, buffer);
    if (retval <= 0)
        close();

    return retval > 0;
}


//
//
//

ServerSocket::ServerSocket (ServerSocket && other) LOG4CPLUS_NOEXCEPT
    : AbstractSocket (std::move (other))
{
    interruptHandles[0] = -1;
    interruptHandles[1] = -1;
    interruptHandles.swap (other.interruptHandles);
}


ServerSocket &
ServerSocket::operator = (ServerSocket && other) LOG4CPLUS_NOEXCEPT
{
    swap (other);
    return *this;
}


void
ServerSocket::swap (ServerSocket & other)
{
    AbstractSocket::swap (other);
    interruptHandles.swap (other.interruptHandles);
}


//
//
//

SOCKET_TYPE
openSocket(unsigned short port, bool udp, bool ipv6, SocketState& state)
{
    return openSocket(log4cplus::internal::empty_str, port, udp, ipv6, state);
}


} } // namespace log4cplus { namespace helpers {
