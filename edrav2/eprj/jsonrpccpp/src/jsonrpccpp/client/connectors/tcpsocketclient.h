/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketclient.h
 * @date    17.10.2016
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_TCPSOCKETCLIENT_H_
#define JSONRPC_CPP_TCPSOCKETCLIENT_H_

#include <jsonrpccpp/client/iclientconnector.h>
#include <jsonrpccpp/common/exception.h>
#include <string>

namespace jsonrpc
{
        /**
         * This class provides an embedded TCP Socket Client that sends Requests over a tcp socket and read result from same socket.
         * It uses the delimiter character to distinct a full RPC Message over the tcp flow. This character is parametered on
         * compilation time in implementation files. The default value for this delimiter is 0x0A a.k.a. "new line".
         * This class hides OS specific features in real implementation of this client. Currently it has implementation for
         * both Linux and Windows.
         */
    class TcpSocketClient : public IClientConnector
    {
        public:
                        /**
                         * @brief TcpSocketClient, constructor for the included TcpSocketClient
                         *
                         * Instanciates the real implementation of TcpSocketClientPrivate depending on running OS.
                         *
                         * @param ipToConnect The ipv4 address on which the client should try to connect
                         * @param port The port on which the client should try to connect
                         */
            TcpSocketClient(const std::string& ipToConnect, const unsigned int &port);
                        /**
                         * @brief ~TcpSocketClient, the destructor of TcpSocketClient
                         */
                        virtual ~TcpSocketClient();
                        /**
                         * @brief The IClientConnector::SendRPCMessage method overload.
                         * @param message The message to send
                         * @param result The result of the call returned by the servsr
                         * @throw JsonRpcException Thrown when an issue is encounter with socket manipulation (see message of exception for more information about what happened).
                         */
            virtual void SendRPCMessage(const std::string& message, std::string& result) ;

        private:
            IClientConnector *realSocket; /*!< A pointer to the real implementation of this class depending of running OS*/
    };

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_TCPSOCKETCLIENT_H_ */
