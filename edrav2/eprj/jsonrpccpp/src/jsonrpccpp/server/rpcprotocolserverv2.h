/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    rpcprotocolserverv2.h
 * @date    31.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_RPCPROTOCOLSERVERV2_H_
#define JSONRPC_CPP_RPCPROTOCOLSERVERV2_H_

#include <string>
#include <vector>
#include <map>

#include <jsonrpccpp/common/exception.h>
#include "abstractprotocolhandler.h"


#define KEY_REQUEST_VERSION     "jsonrpc"
#define JSON_RPC_VERSION2        "2.0"

namespace jsonrpc
{
    class RpcProtocolServerV2 : public AbstractProtocolHandler
    {
        public:
            RpcProtocolServerV2(IProcedureInvokationHandler &handler);

            void HandleJsonRequest(const Json::Value& request, Json::Value& response);
            bool ValidateRequestFields(const Json::Value &val);
            void WrapResult(const Json::Value& request, Json::Value& response, Json::Value& retValue);
            void WrapError(const Json::Value& request, int code, const std::string &message, Json::Value& result);
            void WrapException(const Json::Value& request, const JsonRpcException &exception, Json::Value& result);
            procedure_t GetRequestType(const Json::Value& request);

        private:
            void HandleSingleRequest(const Json::Value& request, Json::Value& response);
            void HandleBatchRequest(const Json::Value& requests, Json::Value& response);
    };

    class RpcProtocolServerV2Proxy : public AbstractProtocolProxyHandler
    {
        public:
            RpcProtocolServerV2Proxy(IProcedureInvokationHandler &handler);

            void HandleJsonRequest(const Json::Value& request, Json::Value& response);
            bool ValidateRequestFields(const Json::Value &val);
            void WrapResult(const Json::Value& request, Json::Value& response, Json::Value& retValue);
            void WrapError(const Json::Value& request, int code, const std::string &message, Json::Value& result);
            void WrapException(const Json::Value& request, const JsonRpcException &exception, Json::Value& result);
            procedure_t GetRequestType(const Json::Value& request);

        private:
            void HandleSingleRequest(const Json::Value& request, Json::Value& response);
            void HandleBatchRequest(const Json::Value& requests, Json::Value& response);
    };
} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_RPCPROTOCOLSERVERV2_H_ */
