/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    windowstcpsocketclient.h
 * @date    17.10.2016
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_WINDOWSTCPSOCKETCLIENT_H_
#define JSONRPC_CPP_WINDOWSTCPSOCKETCLIENT_H_

#include <jsonrpccpp/common/exception.h>
#include <string>
#include <jsonrpccpp/client/connectors/tcpsocketclient.h>
#include <winsock2.h>

namespace jsonrpc
{
    /**
     * This class is the windows implementation of TCPSocketClient.
     * It uses the Winsock2 API to performs its job.
     */
    class WindowsTcpSocketClient : public IClientConnector
    {
        public:
            /**
             * @brief WindowsTcpSocketClient, constructor of the Windows implementation of class TcpSocketClient
             * @param hostToConnect The hostname or the ipv4 address on which the client should try to connect
             * @param port The port on which the client should try to connect
             */
            WindowsTcpSocketClient(const std::string& hostToConnect, const unsigned int &port);
            /**
             * @brief ~WindowsTcpSocketClient, the destructor of WindowsTcpSocketClient
             */
            virtual ~WindowsTcpSocketClient();
            /**
             * @brief The real implementation of TcpSocketClient::SendRPCMessage method.
             * @param message The message to send
             * @param result The result of the call returned by the server
             * @throw JsonRpcException Thrown when an issue is encounter with socket manipulation (see message of exception for more information about what happened).
             */
            virtual void SendRPCMessage(const std::string& message, std::string& result) ;

        private:
            std::string hostToConnect;    /*!< The hostname or the ipv4 address on which the client should try to connect*/
            unsigned int port;          /*!< The port on which the client should try to connect*/
			SOCKET socket_;
            /**
             * @brief A method to produce human readable messages from Winsock2 error values.
             * @param e A Winsock2 error value
             * @return The message matching the error value
             */
            static std::string GetErrorMessage(const int &e);
            /**
             * @brief Connects to the host and port provided by constructor parameters.
             *
             * This method detects if the hostToConnect attribute is either an IPv4 or a hostname.
             * On first case it tries to connect to the ip.
             * On second case it tries to resolve hostname to an ip and tries to connect to it if resolve was successful.
             *
             * @returns A file descriptor to the successfully connected socket
             * @throw JsonRpcException Thrown when an issue is encountered while trying to connect (see message of exception for more information about what happened).
             */
            SOCKET Connect() ;
            /**
             * @brief Connects to provided ip and port.
             *
             * This method tries to connect to the provided ip and port.
             *
             * @param ip The ipv4 address to connect to
             * @param port The port to connect to
             * @returns A file descriptor to the successfully connected socket
             * @throw JsonRpcException Thrown when an issue is encountered while trying to connect (see message of exception for more information about what happened).
             */
            SOCKET Connect(const std::string& ip, const int& port) ;
            /**
             * @brief Check if provided ip is an ipv4 address.
             *
             * @param ip The ipv4 address to check
             * @returns A boolean indicating if the provided ip is or is not an ipv4 address
             */
            bool IsIpv4Address(const std::string& ip);
 
			bool IsSocketActive();
	};

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_WINDOWSTCPSOCKETCLIENT_H_ */
