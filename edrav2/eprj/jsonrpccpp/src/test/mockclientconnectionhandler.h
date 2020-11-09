/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    mockclientconnectionhandler.h
 * @date    10/29/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_MOCKCLIENTCONNECTIONHANDLER_H
#define JSONRPC_CPP_MOCKCLIENTCONNECTIONHANDLER_H

#include <jsonrpccpp/server.h>
#include <string>

namespace jsonrpc
{
    class MockClientConnectionHandler : public IClientConnectionHandler
    {
        public:
            MockClientConnectionHandler();

            virtual void HandleRequest(const std::string& request, std::string& retValue);

            std::string response;
            std::string request;
            long timeout;
    };


}
#endif // JSONRPC_CPP_MOCKCLIENTCONNECTIONHANDLER_H
