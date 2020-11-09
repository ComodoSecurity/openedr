/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    iprocedureinvokationhandler.h
 * @date    23.10.2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_IPROCEDUREINVOKATIONHANDLER_H
#define JSONRPC_CPP_IPROCEDUREINVOKATIONHANDLER_H

#include <string>

namespace Json {
    class Value;
}

namespace jsonrpc {

    class Procedure;

    class IProcedureInvokationHandler {
        public:
            virtual ~IProcedureInvokationHandler() {}
            virtual void HandleMethodCall(Procedure& proc, const Json::Value& input, Json::Value& output) = 0;
            virtual void HandleMethodCall(const std::string& name, const Json::Value& input, Json::Value& output) = 0;
            virtual void HandleNotificationCall(Procedure& proc, const Json::Value& input) = 0;
    };
}

#endif //JSONRPC_CPP_IPROCEDUREINVOKATIONHANDLER_H
