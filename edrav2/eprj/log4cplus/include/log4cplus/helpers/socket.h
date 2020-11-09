// -*- C++ -*-
// Module:  Log4CPLUS
// File:    socket.h
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

/** @file */

#ifndef LOG4CPLUS_HELPERS_SOCKET_HEADER_
#define LOG4CPLUS_HELPERS_SOCKET_HEADER_

#include <log4cplus/config.hxx>

#if defined (LOG4CPLUS_HAVE_PRAGMA_ONCE)
#pragma once
#endif

#include <array>

#include <log4cplus/tstring.h>
#include <log4cplus/helpers/socketbuffer.h>


namespace log4cplus {
    namespace helpers {

        enum SocketState { ok,
                           not_opened,
                           bad_address,
                           connection_failed,
                           broken_pipe,
                           invalid_access_mode,
                           message_truncated,
                           accept_interrupted
                         };

        typedef std::ptrdiff_t SOCKET_TYPE;

        extern LOG4CPLUS_EXPORT SOCKET_TYPE const INVALID_SOCKET_VALUE;

        class LOG4CPLUS_EXPORT AbstractSocket {
        public:
            AbstractSocket();
            AbstractSocket(SOCKET_TYPE sock, SocketState state, int err);
            AbstractSocket(AbstractSocket const &) = delete;
            AbstractSocket(AbstractSocket &&) LOG4CPLUS_NOEXCEPT;
            virtual ~AbstractSocket() = 0;

            /// Close socket
            virtual void close();
            virtual bool isOpen() const;
            virtual void shutdown();
            AbstractSocket & operator = (AbstractSocket && rhs) LOG4CPLUS_NOEXCEPT;

            void swap (AbstractSocket &);

        protected:
            SOCKET_TYPE sock;
            SocketState state;
            int err;
        };



        /**
         * This class implements client sockets (also called just "sockets").
         * A socket is an endpoint for communication between two machines.
         */
        class LOG4CPLUS_EXPORT Socket : public AbstractSocket {
        public:
          // ctor and dtor
            Socket();
            Socket(SOCKET_TYPE sock, SocketState state, int err);
            Socket(const tstring& address, unsigned short port,
                bool udp = false, bool ipv6 = false);
            Socket(Socket &&) LOG4CPLUS_NOEXCEPT;
            virtual ~Socket();

            Socket & operator = (Socket &&) LOG4CPLUS_NOEXCEPT;

          // methods
            virtual bool read(SocketBuffer& buffer);
            virtual bool write(const SocketBuffer& buffer);
            virtual bool write(const std::string & buffer);
            virtual bool write(std::size_t bufferCount,
                SocketBuffer const * const * buffers);

            template <typename... Args>
            static bool write(Socket & socket, Args &&... args)
            {
                SocketBuffer const * const buffers[sizeof... (args)] {
                    (&args)... };
                return socket.write (sizeof... (args), buffers);
            }
        };



        /**
         * This class implements server sockets. A server socket waits for
         * requests to come in over the network. It performs some operation
         * based on that request, and then possibly returns a result to the
         * requester.
         */
        class LOG4CPLUS_EXPORT ServerSocket : public AbstractSocket {
        public:
            ServerSocket(unsigned short port, bool udp = false,
                bool ipv6 = false, tstring const & host = tstring ());
            ServerSocket(ServerSocket &&) LOG4CPLUS_NOEXCEPT;
            virtual ~ServerSocket();

            ServerSocket & operator = (ServerSocket &&) LOG4CPLUS_NOEXCEPT;

            Socket accept();
            void interruptAccept ();
            void swap (ServerSocket &);

        protected:
            std::array<std::ptrdiff_t, 2> interruptHandles;
        };


        LOG4CPLUS_EXPORT SOCKET_TYPE openSocket(unsigned short port, bool udp,
            bool ipv6, SocketState& state);
        LOG4CPLUS_EXPORT SOCKET_TYPE openSocket(tstring const & host,
            unsigned short port, bool udp, bool ipv6, SocketState& state);

        LOG4CPLUS_EXPORT SOCKET_TYPE connectSocket(const log4cplus::tstring& hostn,
            unsigned short port, bool udp, bool ipv6, SocketState& state);
        LOG4CPLUS_EXPORT SOCKET_TYPE acceptSocket(SOCKET_TYPE sock, SocketState& state);
        LOG4CPLUS_EXPORT int closeSocket(SOCKET_TYPE sock);
        LOG4CPLUS_EXPORT int shutdownSocket(SOCKET_TYPE sock);

        LOG4CPLUS_EXPORT long read(SOCKET_TYPE sock, SocketBuffer& buffer);
        LOG4CPLUS_EXPORT long write(SOCKET_TYPE sock,
            const SocketBuffer& buffer);
        LOG4CPLUS_EXPORT long write(SOCKET_TYPE sock, std::size_t bufferCount,
            SocketBuffer const * const * buffers);
        LOG4CPLUS_EXPORT long write(SOCKET_TYPE sock,
            const std::string & buffer);

        LOG4CPLUS_EXPORT tstring getHostname (bool fqdn);
        LOG4CPLUS_EXPORT int setTCPNoDelay (SOCKET_TYPE, bool);

    } // end namespace helpers
} // end namespace log4cplus

#endif // LOG4CPLUS_HELPERS_SOCKET_HEADER_
