// Module:  Log4CPLUS
// File:    socket-win32.cxx
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
#if defined (LOG4CPLUS_USE_WINSOCK)

#include <cassert>
#include <cerrno>
#include <vector>
#include <cstring>
#include <atomic>
#include <log4cplus/internal/socket.h>
#include <log4cplus/helpers/loglog.h>
#include <log4cplus/thread/threads.h>
#include <log4cplus/helpers/stringhelper.h>
#include <log4cplus/internal/internal.h>


/////////////////////////////////////////////////////////////////////////////
// file LOCAL Classes
/////////////////////////////////////////////////////////////////////////////

namespace
{

enum WSInitStates
{
    WS_UNINITIALIZED,
    WS_INITIALIZING,
    WS_INITIALIZED
};


static WSADATA wsa;
static std::atomic<WSInitStates> winsock_state (WS_UNINITIALIZED);


static inline
WSInitStates
interlocked_compare_exchange (std::atomic<WSInitStates> & var,
    WSInitStates expected, WSInitStates desired)
{
    var.compare_exchange_strong (expected, desired);
    return expected;
}



static
void
init_winsock_worker ()
{
    log4cplus::helpers::LogLog * loglog
        = log4cplus::helpers::LogLog::getLogLog ();

    // Try to change the state to WS_INITIALIZING.
    WSInitStates val = interlocked_compare_exchange (winsock_state,
        WS_UNINITIALIZED, WS_INITIALIZING);
    switch (val)
    {
    case WS_UNINITIALIZED:
    {
        int ret = WSAStartup (MAKEWORD (2, 2), &wsa);
        if (ret != 0)
        {
            // Revert the state back to WS_UNINITIALIZED to unblock other
            // threads and let them throw exception.
            val = interlocked_compare_exchange (winsock_state, WS_INITIALIZING,
                WS_UNINITIALIZED);
            assert (val == WS_INITIALIZING);
            loglog->error (LOG4CPLUS_TEXT ("Could not initialize WinSock."),
                true);
        }

        // WinSock is initialized, change the state to WS_INITIALIZED.
        val = interlocked_compare_exchange (winsock_state, WS_INITIALIZING,
            WS_INITIALIZED);
        assert (val == WS_INITIALIZING);
        return;
    }

    case WS_INITIALIZING:
        // Wait for state change.
        while (true)
        {
            switch (winsock_state)
            {
            case WS_INITIALIZED:
                return;

            case WS_INITIALIZING:
                log4cplus::thread::yield ();
                continue;

            default:
                assert (0);
                loglog->error (LOG4CPLUS_TEXT ("Unknown WinSock state."), true);
            }
        }

    case WS_INITIALIZED:
        // WinSock is already initialized.
        return;

    default:
        assert (0);
        loglog->error (LOG4CPLUS_TEXT ("Unknown WinSock state."), true);
    }
}


static
void
init_winsock ()
{
    // Quick check first to avoid the expensive interlocked compare
    // and exchange.
    if (LOG4CPLUS_LIKELY (winsock_state == WS_INITIALIZED))
        return;
    else
        init_winsock_worker ();
}


struct WinSockInitializer
{
    ~WinSockInitializer ()
    {
        if (winsock_state == WS_INITIALIZED)
            WSACleanup ();
    }

    static WinSockInitializer winSockInitializer;
};


WinSockInitializer WinSockInitializer::winSockInitializer;


} // namespace


namespace log4cplus { namespace helpers {


/////////////////////////////////////////////////////////////////////////////
// Global Methods
/////////////////////////////////////////////////////////////////////////////

SOCKET_TYPE
openSocket(tstring const & host, unsigned short port, bool udp, bool ipv6,
    SocketState& state)
{
    ADDRINFOT addr_info_hints{};
    PADDRINFOT ai = nullptr;
    std::unique_ptr<ADDRINFOT, ADDRINFOT_deleter> addr_info;
    int const family = ipv6 ? AF_INET6 : AF_INET;
    int const socket_type = udp ? SOCK_DGRAM : SOCK_STREAM;
    int const protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
    tstring const port_str = convertIntegerToString(port);
    int retval;
    DWORD const wsa_socket_flags =
#if defined (WSA_FLAG_NO_HANDLE_INHERIT)
        WSA_FLAG_NO_HANDLE_INHERIT;
#else
        0;
#endif

    init_winsock ();

    addr_info_hints.ai_family = family;
    addr_info_hints.ai_socktype = socket_type;
    addr_info_hints.ai_protocol = protocol;
    addr_info_hints.ai_flags = AI_PASSIVE;
    retval = GetAddrInfo (host.empty () ? nullptr : host.c_str (),
        port_str.c_str (), &addr_info_hints, &ai);
    if (retval != 0)
    {
        set_last_socket_error(retval);
        return INVALID_SOCKET_VALUE;
    }

    addr_info.reset(ai);

    socket_holder sock_holder (
        WSASocketW (ai->ai_family, ai->ai_socktype, ai->ai_protocol, nullptr,
            0, wsa_socket_flags));
    if (sock_holder.sock == INVALID_OS_SOCKET_VALUE)
        goto error;

    if (bind(sock_holder.sock, ai->ai_addr, static_cast<int>(ai->ai_addrlen)) != 0)
        goto error;

    if (::listen(sock_holder.sock, 10) != 0)
        goto error;

    state = ok;
    return to_log4cplus_socket (sock_holder.detach());

error:
    int eno = WSAGetLastError ();
    set_last_socket_error (eno);
    return INVALID_SOCKET_VALUE;
}


SOCKET_TYPE
connectSocket(const tstring& hostn, unsigned short port, bool udp, bool ipv6,
    SocketState& state)
{
    ADDRINFOT addr_info_hints{};
    PADDRINFOT ai = nullptr;
    std::unique_ptr<ADDRINFOT, ADDRINFOT_deleter> addr_info;
    int const family = ipv6 ? AF_INET6 : AF_INET;
    int const socket_type = udp ? SOCK_DGRAM : SOCK_STREAM;
    int const protocol = udp ? IPPROTO_UDP : IPPROTO_TCP;
    tstring const port_str = convertIntegerToString (port);
    int retval;
    DWORD const wsa_socket_flags =
#if defined (WSA_FLAG_NO_HANDLE_INHERIT)
        WSA_FLAG_NO_HANDLE_INHERIT;
#else
        0;
#endif

    init_winsock ();

    addr_info_hints.ai_family = family;
    addr_info_hints.ai_socktype = socket_type;
    addr_info_hints.ai_protocol = protocol;
    addr_info_hints.ai_flags = AI_NUMERICSERV;
    retval = GetAddrInfo(hostn.c_str(), port_str.c_str(), &addr_info_hints,
        &ai);
    if (retval != 0)
    {
        set_last_socket_error(retval);
        return INVALID_SOCKET_VALUE;
    }

    addr_info.reset(ai);

    socket_holder sock_holder;
    for (ADDRINFOT * rp = ai; rp; rp = rp->ai_next)
    {
        sock_holder.reset(
            WSASocketW(rp->ai_family, rp->ai_socktype, rp->ai_protocol,
                nullptr, 0, wsa_socket_flags));
        if (sock_holder.sock == INVALID_OS_SOCKET_VALUE)
            continue;

        while (
            (retval = ::connect(sock_holder.sock, rp->ai_addr,
                static_cast<int>(rp->ai_addrlen))) == -1
            && (WSAGetLastError() == WSAEINTR))
            ;
        if (retval != SOCKET_ERROR)
            break;
    }

    if (sock_holder.sock == INVALID_OS_SOCKET_VALUE)
    {
        DWORD const eno = WSAGetLastError();
        set_last_socket_error(eno);
        return INVALID_SOCKET_VALUE;
    }

    state = ok;
    return to_log4cplus_socket (sock_holder.detach());
}


SOCKET_TYPE
acceptSocket(SOCKET_TYPE sock, SocketState & state)
{
    init_winsock ();

    SOCKET connected_socket = ::accept (to_os_socket (sock), nullptr, nullptr);

    if (connected_socket != INVALID_OS_SOCKET_VALUE)
        state = ok;
    else
        set_last_socket_error (WSAGetLastError ());

    return to_log4cplus_socket (connected_socket);
}



int
closeSocket(SOCKET_TYPE sock)
{
    return ::closesocket (to_os_socket (sock));
}


int
shutdownSocket(SOCKET_TYPE sock)
{
    return ::shutdown (to_os_socket (sock), SD_BOTH);
}


long
read(SOCKET_TYPE sock, SocketBuffer& buffer)
{
    long read = 0;
    os_socket_type const osSocket = to_os_socket (sock);

    do
    {
        long const res = ::recv(osSocket,
            buffer.getBuffer() + read,
            static_cast<int>(buffer.getMaxSize() - read),
            0);
        if (res == SOCKET_ERROR)
        {
            set_last_socket_error (WSAGetLastError ());
            return res;
        }

        // A return of 0 indicates the socket is closed,
        // return to prevent an infinite loop.
        if (res == 0)
            return read;

        read += res;
    }
    while (read < static_cast<long>(buffer.getMaxSize()));

    return read;
}



long
write(SOCKET_TYPE sock, const SocketBuffer& buffer)
{
    long ret = ::send (to_os_socket (sock), buffer.getBuffer(),
        static_cast<int>(buffer.getSize()), 0);
    if (ret == SOCKET_ERROR)
        set_last_socket_error (WSAGetLastError ());
    return ret;
}


long
write (SOCKET_TYPE sock, std::size_t bufferCount,
    SocketBuffer const * const * buffers)
{
    std::vector<WSABUF> wsabufs (bufferCount);
    for (std::size_t i = 0; i != bufferCount; ++i)
    {
        WSABUF & wsabuf = wsabufs[i];
        std::memset (&wsabuf, 0, sizeof (wsabuf));
        SocketBuffer const & buffer = *buffers[i];

        wsabuf.buf = buffer.getBuffer ();
        wsabuf.len = static_cast<ULONG>(buffer.getSize ());
    }

    DWORD bytes_sent = 0;
    int ret = WSASend (sock, &wsabufs[0], static_cast<DWORD>(bufferCount),
        &bytes_sent, 0, nullptr, nullptr);
    if (ret == SOCKET_ERROR)
    {
        set_last_socket_error (WSAGetLastError ());
        return 0;
    }
    else
        return static_cast<long>(bytes_sent);
}


long
write(SOCKET_TYPE sock, const std::string & buffer)
{
    long ret = ::send (to_os_socket (sock), buffer.c_str (),
        static_cast<int>(buffer.size ()), 0);
    if (ret == SOCKET_ERROR)
        set_last_socket_error (WSAGetLastError ());
    return ret;
}


static
bool
verifyWindowsVersionAtLeast (DWORD major, DWORD minor)
{
    OSVERSIONINFOEX os{};
    os.dwOSVersionInfoSize = sizeof (os);
    os.dwMajorVersion = major;
    os.dwMinorVersion = minor;

    ULONGLONG cond_mask
        = VerSetConditionMask (0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond_mask = VerSetConditionMask (cond_mask, VER_MINORVERSION,
        VER_GREATER_EQUAL);

    bool result = !! VerifyVersionInfo (&os,
        VER_MAJORVERSION | VER_MINORVERSION, cond_mask);
    return result;
}


tstring
getHostname (bool fqdn)
{
    init_winsock ();

    char const * hostname = "unknown";
    int ret;
    // The initial size is based on information in the Microsoft article
    // <https://msdn.microsoft.com/en-us/library/ms738527(v=vs.85).aspx>
    std::vector<char> hn (256, 0);

    while (true)
    {
        ret = ::gethostname (&hn[0], static_cast<int>(hn.size ()) - 1);
        if (ret == 0)
        {
            hostname = &hn[0];
            break;
        }
        else
        {
            int const wsaeno = WSAGetLastError();
            if (wsaeno == WSAEFAULT && hn.size () <= 1024 * 8)
                // Out buffer was too short. Retry with buffer twice the size.
                hn.resize (hn.size () * 2, 0);
            else
            {
                helpers::getLogLog().error(
                    LOG4CPLUS_TEXT("Failed to get own hostname. Error: ")
                    + convertIntegerToString (wsaeno));
                return LOG4CPLUS_STRING_TO_TSTRING (hostname);
            }
        }
    }

    if (ret != 0 || (ret == 0 && ! fqdn))
        return LOG4CPLUS_STRING_TO_TSTRING (hostname);

    ADDRINFOT addr_info_hints{ };
    addr_info_hints.ai_family = AF_INET;
    // The AI_FQDN flag is available only on Windows 7 and later.
#if defined (AI_FQDN)
    if (verifyWindowsVersionAtLeast (6, 1))
        addr_info_hints.ai_flags = AI_FQDN;
    else
#endif
        addr_info_hints.ai_flags = AI_CANONNAME;

    std::unique_ptr<ADDRINFOT, ADDRINFOT_deleter> addr_info;
    ADDRINFOT * ai = nullptr;
    ret = GetAddrInfo (LOG4CPLUS_C_STR_TO_TSTRING (hostname).c_str (), nullptr,
        &addr_info_hints, &ai);
    if (ret != 0)
    {
        WSASetLastError (ret);
        helpers::getLogLog ().error (
            LOG4CPLUS_TEXT ("Failed to resolve own hostname. Error: ")
            + convertIntegerToString (ret));
        return LOG4CPLUS_STRING_TO_TSTRING (hostname);
    }

    addr_info.reset (ai);
    return addr_info->ai_canonname;
}


int
setTCPNoDelay (SOCKET_TYPE sock, bool val)
{
    int result;
    int enabled = static_cast<int>(val);
    if ((result = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
            reinterpret_cast<char*>(&enabled),sizeof(enabled))) != 0)
    {
        int eno = WSAGetLastError ();
        set_last_socket_error (eno);
    }

    return result;
}


//
// ServerSocket OS dependent stuff
//

namespace
{

static
bool
setSocketBlocking (SOCKET_TYPE s)
{
    u_long val = 0;
    int ret = ioctlsocket (to_os_socket (s), FIONBIO, &val);
    if (ret == SOCKET_ERROR)
    {
        set_last_socket_error (WSAGetLastError ());
        return false;
    }
    else
        return true;
}

static
bool
removeSocketEvents (SOCKET_TYPE s, HANDLE ev)
{
    // Clean up socket events handling.

    int ret = WSAEventSelect (to_os_socket (s), ev, 0);
    if (ret == SOCKET_ERROR)
    {
        set_last_socket_error (WSAGetLastError ());
        return false;
    }
    else
        return true;
}


static
bool
socketEventHandlingCleanup (SOCKET_TYPE s, HANDLE ev)
{
    bool ret = removeSocketEvents (s, ev);
    ret = setSocketBlocking (s) && ret;
    ret = WSACloseEvent (ev) && ret;
    return ret;
}


} // namespace


ServerSocket::ServerSocket(unsigned short port, bool udp /*= false*/,
    bool ipv6 /*= false*/, tstring const & host /*= tstring ()*/)
{
    // Initialize these here so that we do not try to close invalid handles
    // in dtor if the following `openSocket()` fails.
    interruptHandles[0] = 0;
    interruptHandles[1] = 0;

    sock = openSocket (host, port, udp, ipv6, state);
    if (sock == INVALID_SOCKET_VALUE)
    {
        err = get_last_socket_error ();
        return;
    }

    HANDLE ev = WSACreateEvent ();
    if (ev == WSA_INVALID_EVENT)
    {
        err = WSAGetLastError ();
        closeSocket (sock);
        sock = INVALID_SOCKET_VALUE;
    }
    else
    {
        static_assert (sizeof (std::ptrdiff_t) >= sizeof (HANDLE),
            "Size of std::ptrdiff_t must be larger than size of HANDLE.");
        interruptHandles[0] = reinterpret_cast<std::ptrdiff_t>(ev);
    }
}

Socket
ServerSocket::accept ()
{
    int const N_EVENTS = 2;
    HANDLE events[N_EVENTS] = {
        reinterpret_cast<HANDLE>(interruptHandles[0]) };
    HANDLE & accept_ev = events[1];
    int ret;

    // Create event and prime socket to set the event on FD_ACCEPT.

    accept_ev = WSACreateEvent ();
    if (accept_ev == WSA_INVALID_EVENT)
    {
        set_last_socket_error (WSAGetLastError ());
        goto error;
    }

    ret = WSAEventSelect (to_os_socket (sock), accept_ev, FD_ACCEPT);
    if (ret == SOCKET_ERROR)
    {
        set_last_socket_error (WSAGetLastError ());
        goto error;
    }

    do
    {
        // Wait either for interrupt event or actual connection coming in.

        DWORD wsawfme = WSAWaitForMultipleEvents (N_EVENTS, events, FALSE,
            WSA_INFINITE, TRUE);
        switch (wsawfme)
        {
        case WSA_WAIT_TIMEOUT:
        case WSA_WAIT_IO_COMPLETION:
            // Retry after timeout or APC.
            continue;

        // This is interrupt signal/event.
        case WSA_WAIT_EVENT_0:
        {
            // Reset the interrupt event back to non-signalled state.

            ret = WSAResetEvent (reinterpret_cast<HANDLE>(interruptHandles[0]));

            // Clean up socket events handling.

            ret = socketEventHandlingCleanup (sock, accept_ev);

            // Return Socket with state set to accept_interrupted.

            return Socket (INVALID_SOCKET_VALUE, accept_interrupted, 0);
        }

        // This is accept_ev.
        case WSA_WAIT_EVENT_0 + 1:
        {
            // Clean up socket events handling.

            ret = socketEventHandlingCleanup (sock, accept_ev);

            // Finally, call accept().

            SocketState st = not_opened;
            SOCKET_TYPE clientSock = acceptSocket (sock, st);
            int eno = 0;
            if (clientSock == INVALID_SOCKET_VALUE)
                eno = get_last_socket_error ();

            return Socket (clientSock, st, eno);
        }

        case WSA_WAIT_FAILED:
        default:
            set_last_socket_error (WSAGetLastError ());
            goto error;
        }
    }
    while (true);


error:;
    DWORD eno = get_last_socket_error ();

    // Clean up socket events handling.

    if (sock != INVALID_SOCKET_VALUE)
    {
        (void) removeSocketEvents (sock, accept_ev);
        (void) setSocketBlocking (sock);
    }

    if (accept_ev != WSA_INVALID_EVENT)
        WSACloseEvent (accept_ev);

    set_last_socket_error (eno);
    return Socket (INVALID_SOCKET_VALUE, not_opened, eno);
}


void
ServerSocket::interruptAccept ()
{
    (void) WSASetEvent (reinterpret_cast<HANDLE>(interruptHandles[0]));
}


ServerSocket::~ServerSocket()
{
    if (interruptHandles[0] != 0)
        (void) WSACloseEvent(reinterpret_cast<HANDLE>(interruptHandles[0]));
}



} } // namespace log4cplus { namespace helpers {

#endif // LOG4CPLUS_USE_WINSOCK
