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

#include <cstddef>
#include <aws/core/utils/UnreferencedParam.h>
#include <aws/core/net/SimpleUDP.h>

namespace Aws
{
    namespace Net
    {
        SimpleUDP::SimpleUDP(int addressFamily, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking):
            m_addressFamily(addressFamily), m_connected(false)
        {
            CreateSocket(addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::SimpleUDP(bool IPV4, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking) :
            m_addressFamily(IPV4 ? 2 : 10), m_connected(false)
        {
            CreateSocket(m_addressFamily, sendBufSize, receiveBufSize, nonBlocking);
        }

        SimpleUDP::~SimpleUDP()
        {
        }

        void SimpleUDP::CreateSocket(int addressFamily, size_t sendBufSize, size_t receiveBufSize, bool nonBlocking)
        {
            AWS_UNREFERENCED_PARAM(addressFamily);
            AWS_UNREFERENCED_PARAM(sendBufSize);
            AWS_UNREFERENCED_PARAM(receiveBufSize);
            AWS_UNREFERENCED_PARAM(nonBlocking);
        }

        int SimpleUDP::Connect(const sockaddr* address, size_t addressLength)
        {
            AWS_UNREFERENCED_PARAM(address);
            AWS_UNREFERENCED_PARAM(addressLength);
            return -1;
        }

        int SimpleUDP::ConnectToLocalHost(unsigned short port)
        {
            AWS_UNREFERENCED_PARAM(port);
            return -1;
        }

        int SimpleUDP::Bind(const sockaddr* address, size_t addressLength) const
        {
            AWS_UNREFERENCED_PARAM(address);
            AWS_UNREFERENCED_PARAM(addressLength);
            return -1;
        }

        int SimpleUDP::BindToLocalHost(unsigned short port) const
        {
            AWS_UNREFERENCED_PARAM(port);
            return -1;
        }

        int SimpleUDP::SendData(const uint8_t* data, size_t dataLen) const
        {
            AWS_UNREFERENCED_PARAM(data);
            AWS_UNREFERENCED_PARAM(dataLen);
            return -1;
        }

        int SimpleUDP::SendDataTo(const sockaddr* address, size_t addressLength, const uint8_t* data, size_t dataLen) const
        {
            AWS_UNREFERENCED_PARAM(address);
            AWS_UNREFERENCED_PARAM(addressLength);
            AWS_UNREFERENCED_PARAM(data);
            AWS_UNREFERENCED_PARAM(dataLen);
            return -1;
        }

        int SimpleUDP::SendDataToLocalHost(const uint8_t* data, size_t dataLen, unsigned short port) const
        {
            AWS_UNREFERENCED_PARAM(data);
            AWS_UNREFERENCED_PARAM(dataLen);
            AWS_UNREFERENCED_PARAM(port);
            return -1;
        }

        int SimpleUDP::ReceiveData(uint8_t* buffer, size_t bufferLen) const
        {
            AWS_UNREFERENCED_PARAM(buffer);
            AWS_UNREFERENCED_PARAM(bufferLen);
            return -1;
        }

        int SimpleUDP::ReceiveDataFrom(sockaddr* address, size_t* addressLength, uint8_t* buffer, size_t bufferLen) const
        {
            AWS_UNREFERENCED_PARAM(address);
            AWS_UNREFERENCED_PARAM(addressLength);
            AWS_UNREFERENCED_PARAM(buffer);
            AWS_UNREFERENCED_PARAM(bufferLen);
            return -1;
        }
    }
}
