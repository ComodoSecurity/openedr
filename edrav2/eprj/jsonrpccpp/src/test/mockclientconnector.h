/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    mockclientconnector.h
 * @date    10/9/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_MOCKCLIENTCONNECTOR_H
#define JSONRPC_MOCKCLIENTCONNECTOR_H

#include <jsonrpccpp/client/iclientconnector.h>

namespace jsonrpc {

    class MockClientConnector : public IClientConnector
    {
        public:
            MockClientConnector();

            void SetResponse(const std::string &response);

            std::string GetRequest();
            Json::Value GetJsonRequest();
            virtual void SendRPCMessage(const std::string& message, std::string& result) ;

        private:
            std::string response;
            std::string request;
    };

} // namespace jsonrpc

#endif // JSONRPC_MOCKCLIENTCONNECTOR_H
