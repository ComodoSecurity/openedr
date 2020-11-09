/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    iclientconnectionhandler.h
 * @date    10/23/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_ICLIENTCONNECTIONHANDLER_H
#define JSONRPC_CPP_ICLIENTCONNECTIONHANDLER_H

#include <string>

namespace jsonrpc
{
    class Procedure;
    class IClientConnectionHandler {
        public:
            virtual ~IClientConnectionHandler() {}

            virtual void HandleRequest(const std::string& request, std::string& retValue) = 0;
    };

    class IProtocolHandler : public IClientConnectionHandler
    {
        public:
            virtual ~IProtocolHandler(){}

            virtual void AddProcedure(const Procedure& procedure) = 0;
    };
}

#endif // JSONRPC_CPP_ICLIENTCONNECTIONHANDLER_H
