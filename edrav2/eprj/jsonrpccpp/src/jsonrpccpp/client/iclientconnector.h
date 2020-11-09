/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    abstractclientconnector.h
 * @date    02.01.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_CLIENTCONNECTOR_H_
#define JSONRPC_CPP_CLIENTCONNECTOR_H_

#include <string>
#include <jsonrpccpp/common/exception.h>

namespace jsonrpc
{
    class IClientConnector
    {
        public:
            virtual ~IClientConnector(){}

            virtual void SendRPCMessage(const std::string& message, std::string& result)  = 0;
    };
} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_CLIENTCONNECTOR_H_ */
