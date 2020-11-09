// Module:  Log4CPLUS
// File:    socket-unix.cxx
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


#include <log4cplus/config.hxx>
#if defined (LOG4CPLUS_USE_BSD_SOCKETS)

#include <cstring>
#include <vector>
#include <algorithm>
#include <cerrno>
#include <log4cplus/internal/socket.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/thread/syncprims-pub-impl.h>
#include <log4cplus/spi/loggingevent.h>
#include <log4cplus/helpers/stringhelper.h>

#ifdef LOG4CPLUS_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef LOG4CPLUS_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#if defined (LOG4CPLUS_HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined (LOG4CPLUS_HAVE_NETINET_TCP_H)
#include <netinet/tcp.h>
#endif

#if defined (LOG4CPLUS_HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined (LOG4CPLUS_HAVE_ERRNO_H)
#include <errno.h>
#endif

#ifdef LOG4CPLUS_HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef LOG4CPLUS_HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef LOG4CPLUS_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef LOG4CPLUS_HAVE_POLL_H
#include <poll.h>
#endif


namespace log4cplus { namespace helpers {

// from lockfile.cxx
LOG4CPLUS_PRIVATE bool trySetCloseOnExec (int fd);


namespace
{

static
int
get_host_by_name (char const * hostname, std::string * name,
    struct sockaddr_in * addr)
{
    struct addrinfo hints = addrinfo();
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_CANONNAME;

    if (inet_addr (hostname) != static_cast<in_addr_t>(-1))
        hints.ai_flags |= AI_NUMERICHOST;

    struct addrinfo * res = nullptr;
    int ret = getaddrinfo (hostname, nullptr, &hints, &res);
    if (ret != 0)
        return ret;

    std::unique_ptr<struct addrinfo, addrinfo_deleter> ai (res);

    if (name)
        *name = ai->ai_canonname;

    if (addr)
        std::memcpy (addr, ai->ai_addr, ai->ai_addrlen);

    return 0;
}


} // namespace


/////////////////////////////////////////////////////////////////////////////
// Global Methods
/////////////////////////////////////////////////////////////////////////////

int const TYPE_SOCK_CLOEXEC =
#if defined (SOCK_CLOEXEC)
        SOCK_CLOEXEC
#else
        0
#endif
        ;


SOCKET_TYPE
openSocket(tstring const & host, unsigned short port, bool udp, bool ipv6,
    SocketState& state)
{
    struct addrinfo addr_info_hints = addrinfo();
    struct addrinfo * ai = nullptr;
    std::unique_ptr<struct addrinfo, addrinfo_deleter> addr_info;
    int const family = ipv6 ? AF_INET6 : AF_INET;
    int const socket_type = udp ? SOCK_DGRAM : SOCK_STREAM;
    int const protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
    std::string const port_str = convertIntegerToNarrowString(port);
    int retval;

    addr_info_hints.ai_family = family;
    addr_info_hints.ai_socktype = socket_type;
    addr_info_hints.ai_protocol = protocol;
    addr_info_hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;
    retval = getaddrinfo (
        host.empty () ? nullptr : LOG4CPLUS_TSTRING_TO_STRING (host).c_str (),
        port_str.c_str (), &addr_info_hints, &ai);
    if (retval != 0)
    {
        set_last_socket_error(retval);
        return INVALID_SOCKET_VALUE;
    }

    addr_info.reset (ai);

    socket_holder sock_holder (
        ::socket (ai->ai_family, ai->ai_socktype | TYPE_SOCK_CLOEXEC,
            ai->ai_protocol));
    if (sock_holder.sock < 0)
        return INVALID_SOCKET_VALUE;

#if ! defined (SOCK_CLOEXEC)
    trySetCloseOnExec (sock_holder.sock);
#endif

    int optval = 1;
    socklen_t optlen = sizeof (optval);
    int ret = setsockopt (sock_holder.sock, SOL_SOCKET, SO_REUSEADDR, &optval,
        optlen);
    if (ret != 0)
    {
        int const eno = errno;
        helpers::getLogLog ().warn (LOG4CPLUS_TEXT ("setsockopt() failed: ")
            + helpers::convertIntegerToString (eno));
    }

    retval = bind (sock_holder.sock, ai->ai_addr, ai->ai_addrlen);
    if (retval < 0)
        return INVALID_SOCKET_VALUE;

    if (::listen(sock_holder.sock, 10))
        return INVALID_SOCKET_VALUE;

    state = ok;
    return to_log4cplus_socket (sock_holder.detach ());
}


SOCKET_TYPE
connectSocket(const tstring& hostn, unsigned short port, bool udp, bool ipv6,
    SocketState& state)
{
    int retval;
    struct addrinfo addr_info_hints = addrinfo();
    struct addrinfo * ai = nullptr;
    std::unique_ptr<struct addrinfo, addrinfo_deleter> addr_info;
    int const family = ipv6 ? AF_INET6 : AF_INET;
    int const socket_type = udp ? SOCK_DGRAM : SOCK_STREAM;
    int const protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
    std::string const port_str = convertIntegerToNarrowString(port);

    addr_info_hints.ai_family = family;
    addr_info_hints.ai_socktype = socket_type;
    addr_info_hints.ai_protocol = protocol;
    addr_info_hints.ai_flags = AI_NUMERICSERV;
    retval = getaddrinfo (LOG4CPLUS_TSTRING_TO_STRING(hostn).c_str(),
        port_str.c_str(), &addr_info_hints, &ai);
    if (retval != 0)
    {
        set_last_socket_error (retval);
        return INVALID_SOCKET_VALUE;
    }

    addr_info.reset(ai);

    struct addrinfo * rp;
    socket_holder sock_holder;
    for (rp = ai; rp; rp = rp->ai_next)
    {
        sock_holder.reset (
            ::socket(rp->ai_family, rp->ai_socktype | TYPE_SOCK_CLOEXEC,
                rp->ai_protocol));
        if (sock_holder.sock < 0)
            continue;

#if ! defined (SOCK_CLOEXEC)
        trySetCloseOnExec (sock_holder.sock);
#endif

        while ((retval = ::connect (sock_holder.sock, rp->ai_addr,
                    rp->ai_addrlen)) == -1
            && (errno == EINTR))
            ;
        if (retval == 0)
            break;
    }

    if (rp == nullptr)
        // No address succeeded.
        return INVALID_SOCKET_VALUE;

    state = ok;
    return to_log4cplus_socket (sock_holder.detach ());
}


namespace
{

//! Helper for accept_wrap().
template <typename T, typename U>
struct socklen_var
{
    typedef T type;
};


template <typename U>
struct socklen_var<void, U>
{
    typedef U type;
};


// Some systems like HP-UX have socklen_t but accept() does not use it
// as type of its 3rd parameter. This wrapper works around this
// incompatibility.
template <typename accept_sockaddr_ptr_type, typename accept_socklen_type>
static
SOCKET_TYPE
accept_wrap (
    int (* accept_func) (int, accept_sockaddr_ptr_type, accept_socklen_type *),
    SOCKET_TYPE sock, struct sockaddr * sa, socklen_t * len)
{
    typedef typename socklen_var<accept_socklen_type, socklen_t>::type
        socklen_var_type;
    socklen_var_type l = static_cast<socklen_var_type>(*len);
    SOCKET_TYPE result
        = static_cast<SOCKET_TYPE>(
            accept_func (sock, sa,
                reinterpret_cast<accept_socklen_type *>(&l)));
    *len = static_cast<socklen_t>(l);
    return result;
}


} // namespace


SOCKET_TYPE
acceptSocket(SOCKET_TYPE sock, SocketState& state)
{
    struct sockaddr_in net_client;
    socklen_t len = sizeof(struct sockaddr);
    int clientSock;

    while(
        (clientSock = accept_wrap (accept, to_os_socket (sock),
            reinterpret_cast<struct sockaddr*>(&net_client), &len))
        == -1
        && (errno == EINTR))
        ;

    if(clientSock != INVALID_OS_SOCKET_VALUE) {
        state = ok;
    }

    return to_log4cplus_socket (clientSock);
}



int
closeSocket(SOCKET_TYPE sock)
{
    return ::close(to_os_socket (sock));
}


int
shutdownSocket(SOCKET_TYPE sock)
{
    return ::shutdown(to_os_socket (sock), SHUT_RDWR);
}


long
read(SOCKET_TYPE sock, SocketBuffer& buffer)
{
    long readbytes = 0;

    do
    {
        long const res = ::read(to_os_socket (sock),
            buffer.getBuffer() + readbytes,
            buffer.getMaxSize() - readbytes);
        if( res <= 0 ) {
            return res;
        }
        readbytes += res;
    } while( readbytes < static_cast<long>(buffer.getMaxSize()) );

    return readbytes;
}



long
write(SOCKET_TYPE sock, const SocketBuffer& buffer)
{
#if defined(MSG_NOSIGNAL)
    int flags = MSG_NOSIGNAL;
#else
    int flags = 0;
#endif
    return ::send( to_os_socket (sock), buffer.getBuffer(), buffer.getSize(),
        flags );
}


long
write(SOCKET_TYPE sock, std::size_t bufferCount,
    SocketBuffer const * const * buffers)
{
#if defined(MSG_NOSIGNAL)
    int flags = MSG_NOSIGNAL;
#else
    int flags = 0;
#endif

    std::vector<iovec> iovecs (bufferCount);
    for (std::size_t i = 0; i != bufferCount; ++i)
    {
        iovec & iov = iovecs[i];
        std::memset (&iov, 0, sizeof (iov));
        SocketBuffer const & buffer = *buffers[i];

        iov.iov_base = buffer.getBuffer();
        iov.iov_len = buffer.getSize();
    }

    msghdr message;
    std::memset (&message, 0, sizeof (message));
    message.msg_name = nullptr;
    message.msg_namelen = 0;
    message.msg_control = nullptr;
    message.msg_controllen = 0;
    message.msg_flags = 0;
    message.msg_iov = &iovecs[0];
    message.msg_iovlen = iovecs.size ();

    return sendmsg (to_os_socket (sock), &message, flags);
}


long
write(SOCKET_TYPE sock, const std::string & buffer)
{
#if defined(MSG_NOSIGNAL)
    int flags = MSG_NOSIGNAL;
#else
    int flags = 0;
#endif
    return ::send (to_os_socket (sock), buffer.c_str (), buffer.size (),
        flags);
}


tstring
getHostname (bool fqdn)
{
    char const * hostname = "unknown";
    int ret;
    std::vector<char> hn (1024, 0);

    while (true)
    {
        ret = ::gethostname (&hn[0], static_cast<int>(hn.size ()) - 1);
        if (ret == 0)
        {
            hostname = &hn[0];
            break;
        }
#if defined (LOG4CPLUS_HAVE_ENAMETOOLONG)
        else if (errno == ENAMETOOLONG)
            // Out buffer was too short. Retry with buffer twice the size.
            hn.resize (hn.size () * 2, 0);
#endif
        else
            break;
    }

    if (ret != 0 || (ret == 0 && ! fqdn))
        return LOG4CPLUS_STRING_TO_TSTRING (hostname);

    std::string full_hostname;
    ret = get_host_by_name (hostname, &full_hostname, nullptr);
    if (ret == 0)
        hostname = full_hostname.c_str ();

    return LOG4CPLUS_STRING_TO_TSTRING (hostname);
}


int
setTCPNoDelay (SOCKET_TYPE sock, bool val)
{
#if (defined (SOL_TCP) || defined (IPPROTO_TCP)) && defined (TCP_NODELAY)
#if defined (SOL_TCP)
    int level = SOL_TCP;

#elif defined (IPPROTO_TCP)
    int level = IPPROTO_TCP;

#endif

    int result;
    int enabled = static_cast<int>(val);
    if ((result = setsockopt(sock, level, TCP_NODELAY, &enabled,
                sizeof(enabled))) != 0)
        set_last_socket_error (errno);

    return result;

#else
    // No recognizable TCP_NODELAY option.
    return 0;

#endif
}


//
// ServerSocket OS dependent stuff
//

ServerSocket::ServerSocket(unsigned short port, bool udp /*= false*/,
    bool ipv6 /*= false*/, tstring const & host /*= tstring ()*/)
{
    // Initialize these here so that we do not try to close invalid handles
    // in dtor if the following `openSocket()` fails.
    interruptHandles[0] = -1;
    interruptHandles[1] = -1;

    int fds[2] = {-1, -1};
    int ret;

    sock = openSocket (host, port, udp, ipv6, state);
    if (sock == INVALID_SOCKET_VALUE)
        goto error;

#if defined (LOG4CPLUS_HAVE_PIPE2) && defined (O_CLOEXEC)
    ret = pipe2 (fds, O_CLOEXEC);
    if (ret != 0)
        goto error;

#elif defined (LOG4CPLUS_HAVE_PIPE)
    ret = pipe (fds);
    if (ret != 0)
        goto error;

    trySetCloseOnExec (fds[0]);
    trySetCloseOnExec (fds[1]);

#else
#  error You are missing both pipe() or pipe2().
#endif

    interruptHandles[0] = fds[0];
    interruptHandles[1] = fds[1];
    return;

error:;
    err = get_last_socket_error ();
    state = not_opened;

    if (sock != INVALID_SOCKET_VALUE)
        closeSocket (sock);

    if (fds[0] != -1)
        ::close (fds[0]);

    if (fds[1] != -1)
        ::close (fds[1]);
}

Socket
ServerSocket::accept ()
{
    struct pollfd pollfds[2];

    struct pollfd & interrupt_pipe = pollfds[0];
    interrupt_pipe.fd = interruptHandles[0];
    interrupt_pipe.events = POLLIN;
    interrupt_pipe.revents = 0;

    struct pollfd & accept_fd = pollfds[1];
    accept_fd.fd = to_os_socket (sock);
    accept_fd.events = POLLIN;
    accept_fd.revents = 0;

    do
    {
        interrupt_pipe.revents = 0;
        accept_fd.revents = 0;

        int ret = poll (pollfds, 2, -1);
        switch (ret)
        {
        // Error.
        case -1:
            if (errno == EINTR)
                // Signal has interrupted the call. Just re-run it.
                continue;

            set_last_socket_error (errno);
            return Socket (INVALID_SOCKET_VALUE, not_opened, errno);

        // Timeout. This should not happen though.
        case 0:
            continue;

        default:
            // Some descriptor is ready.

            if ((interrupt_pipe.revents & POLLIN) == POLLIN)
            {
                // Read byte from interruption pipe.

                helpers::getLogLog ().debug (
                    LOG4CPLUS_TEXT ("ServerSocket::accept- ")
                    LOG4CPLUS_TEXT ("accept() interrupted by other thread"));

                char ch;
                ret = ::read (interrupt_pipe.fd, &ch, 1);
                if (ret == -1)
                {
                    int const eno = errno;
                    helpers::getLogLog ().warn (
                        LOG4CPLUS_TEXT ("ServerSocket::accept- read() failed: ")
                        + helpers::convertIntegerToString (eno));
                    set_last_socket_error (eno);
                    return Socket (INVALID_SOCKET_VALUE, not_opened, eno);
                }

                // Return Socket with state set to accept_interrupted.

                return Socket (INVALID_SOCKET_VALUE, accept_interrupted, 0);
            }
            else if ((accept_fd.revents & POLLIN) == POLLIN)
            {
                helpers::getLogLog ().debug (
                    LOG4CPLUS_TEXT ("ServerSocket::accept- ")
                    LOG4CPLUS_TEXT ("accepting connection"));

                SocketState st = not_opened;
                SOCKET_TYPE clientSock = acceptSocket (sock, st);
                int eno = 0;
                if (clientSock == INVALID_SOCKET_VALUE)
                    eno = get_last_socket_error ();

                return Socket (clientSock, st, eno);
            }
            else
                return Socket (INVALID_SOCKET_VALUE, not_opened, 0);
        }
    }
    while (true);
}


void
ServerSocket::interruptAccept ()
{
    char ch = 'I';
    int ret;

    do
    {
        ret = ::write (interruptHandles[1], &ch, 1);
    }
    while (ret == -1 && errno == EINTR);

    if (ret == -1)
    {
        int const eno = errno;
        helpers::getLogLog ().warn (
            LOG4CPLUS_TEXT ("ServerSocket::interruptAccept- write() failed: ")
            + helpers::convertIntegerToString (eno));
    }
}


ServerSocket::~ServerSocket()
{
    if (interruptHandles[0] != -1)
        ::close (interruptHandles[0]);

    if (interruptHandles[1] != -1)
        ::close (interruptHandles[1]);
}

} } // namespace log4cplus

#endif // LOG4CPLUS_USE_BSD_SOCKETS
