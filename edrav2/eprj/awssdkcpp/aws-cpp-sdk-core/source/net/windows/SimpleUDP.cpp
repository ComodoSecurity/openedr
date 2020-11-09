/*
* Copyright 2010-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <Ws2tcpip.h>
#include <cassert>
#include <aws/core/net/SimpleUDP.h>
#include <aws/core/utils/logging/LogMacros.h>
namespace Aws
{
    namespace Net
    {
        static const char ALLOC_TAG[] = "SimpleUDP";
        static void BuildLocalAddrInfoIPV4(sockaddr_in* addrinfo, short port)
        {
            addrinfo->sin_family = AF_INET;
            addrinfo->sin_port = htons(port);
            InetPton(AF_INET, L"127.0.0.1", &addrinfo->sin_addr);
        }

        static void BuildLocalAddrInfoIPV6(sockaddr_in6* addrinfo, short port)
        {
            addrinfo->sin6_family = AF_INET6;
            addrinfo->sin6_port = htons(port);
            InetPton(AF_INET6, L"::1", &addrinfo->sin6_addr);
        }

        SimpleUDP::SimpleUDP(int addressFamily, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking):
            m_addressFamily(addressFamily), m_connected(false)
        {
            CreateSocket(addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::SimpleUDP(bool IPV4, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking) :
            m_addressFamily(IPV4 ? AF_INET : AF_INET6), m_connected(false)
        {
            CreateSocket(m_addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::~SimpleUDP()
        {
            closesocket(GetUnderlyingSocket());
        }

        void SimpleUDP::CreateSocket(int addressFamily, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking)
        {
            SOCKET sock = socket(addressFamily, SOCK_DGRAM, IPPROTO_UDP);
            assert(sock != INVALID_SOCKET);

            // Try to set sock to nonblocking mode.
            if (nonBlocking)
            {
                u_long enable = 1;
                ioctlsocket(sock, FIONBIO, &enable);
            }

            // if sendBufSize is not zero, try to set send buffer size
            if (sendBufSize)
            {
                int ret = setsockopt(sock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendBufSize), sizeof(sendBufSize));
                if (ret)
                {
                    AWS_LOGSTREAM_WARN(ALLOC_TAG, "Failed to set UDP send buffer size to " << sendBufSize << " for socket " << sock << " error code: " << WSAGetLastError());
                }
                assert(ret == 0);
            }

            // if receiveBufSize is not zero, try to set receive buffer size
            if (receiveBufSize)
            {
                int ret = setsockopt(sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&receiveBufSize), sizeof(receiveBufSize));
                if (ret)
                {
                    AWS_LOGSTREAM_WARN(ALLOC_TAG, "Failed to set UDP receive buffer size to " << receiveBufSize << " for socket " << sock << " error code: " << WSAGetLastError());
                }
                assert(ret == 0);
            }

            SetUnderlyingSocket(static_cast<int>(sock));
        }

        int SimpleUDP::Connect(const sockaddr* address, size_t addressLength)
        {
            int ret = connect(GetUnderlyingSocket(), address, static_cast<socklen_t>(addressLength));
            m_connected = ret ? false : true;
            return ret;
        }

        int SimpleUDP::ConnectToLocalHost(unsigned short port)
        {
            int ret;
            if (m_addressFamily == AF_INET6)
            {
                sockaddr_in6 addrinfo;
                BuildLocalAddrInfoIPV6(&addrinfo, port);
                ret = connect(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in6));
            }
            else
            {
                sockaddr_in addrinfo;
                BuildLocalAddrInfoIPV4(&addrinfo, port);
                ret = connect(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in));
            }
            m_connected = ret ? false : true;
            return ret;
        }

        int SimpleUDP::Bind(const sockaddr* address, size_t addressLength) const
        {
            return bind(GetUnderlyingSocket(), address, static_cast<socklen_t>(addressLength));
        }

        int SimpleUDP::BindToLocalHost(unsigned short port) const
        {
            if (m_addressFamily == AF_INET6)
            {
                sockaddr_in6 addrinfo;
                BuildLocalAddrInfoIPV6(&addrinfo, port);
                return bind(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in6));
            }
            else
            {
                sockaddr_in addrinfo;
                BuildLocalAddrInfoIPV4(&addrinfo, port);
                return bind(GetUnderlyingSocket(), reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in));
            }
        }

        int SimpleUDP::SendData(const uint8_t* data, size_t dataLen) const
        {
            return send(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0);
        }

        int SimpleUDP::SendDataTo(const sockaddr* address, size_t addressLength, const uint8_t* data, size_t dataLen) const
        {
            if (m_connected)
            {
                return SendData(data, dataLen);
            }
            else
            {
                return sendto(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0, address, static_cast<socklen_t>(addressLength));
            }
        }

        int SimpleUDP::SendDataToLocalHost(const uint8_t* data, size_t dataLen, unsigned short port) const
        {
            if (m_connected)
            {
                return SendData(data, dataLen);
            }
            else if (m_addressFamily == AF_INET6)
            {
                sockaddr_in6 addrinfo;
                BuildLocalAddrInfoIPV6(&addrinfo, port);
                return sendto(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0, reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in6));
            }
            else
            {
                sockaddr_in addrinfo;
                BuildLocalAddrInfoIPV4(&addrinfo, port);
                return sendto(GetUnderlyingSocket(), reinterpret_cast<const char*>(data), static_cast<int>(dataLen), 0, reinterpret_cast<sockaddr*>(&addrinfo), sizeof(sockaddr_in));
            }
        }

        int SimpleUDP::ReceiveData(uint8_t* buffer, size_t bufferLen) const
        {
            return recv(GetUnderlyingSocket(), reinterpret_cast<char*>(buffer), static_cast<int>(bufferLen), 0);
        }


        int SimpleUDP::ReceiveDataFrom(sockaddr* address, size_t* addressLength, uint8_t* buffer, size_t bufferLen) const
        {
            return recvfrom(GetUnderlyingSocket(), reinterpret_cast<char*>(buffer), static_cast<int>(bufferLen), 0, address, reinterpret_cast<socklen_t*>(addressLength));
        }
    }
}
