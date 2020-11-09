/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    rpcprotocolserver12.h
 * @date    10/25/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_RPCPROTOCOLSERVER12_H
#define JSONRPC_RPCPROTOCOLSERVER12_H

#include "abstractprotocolhandler.h"
#include "rpcprotocolserverv1.h"
#include "rpcprotocolserverv2.h"

namespace jsonrpc {

    class RpcProtocolServer12 : public IProtocolHandler
    {
        public:
            RpcProtocolServer12(IProcedureInvokationHandler &handler);

            void AddProcedure(const Procedure& procedure);
            void HandleRequest(const std::string& request, std::string& retValue);

        private:
            RpcProtocolServerV1 rpc1;
            RpcProtocolServerV2 rpc2;

            AbstractProtocolHandler& GetHandler(const Json::Value& request);
    };

} // namespace jsonrpc

#endif // JSONRPC_RPCPROTOCOLSERVER12_H
