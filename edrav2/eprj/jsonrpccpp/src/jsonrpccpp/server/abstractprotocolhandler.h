/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    abstractprotocolhandler.h
 * @date    10/23/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_ABSTRACTPROTOCOLHANDLER_H
#define JSONRPC_CPP_ABSTRACTPROTOCOLHANDLER_H

#include "iprocedureinvokationhandler.h"
#include "iclientconnectionhandler.h"
#include <map>
#include <string>
#include <jsonrpccpp/common/procedure.h>

#define KEY_REQUEST_METHODNAME  "method"
#define KEY_REQUEST_ID          "id"
#define KEY_REQUEST_PARAMETERS  "params"
#define KEY_RESPONSE_ERROR      "error"
#define KEY_RESPONSE_RESULT     "result"

namespace jsonrpc {

    class AbstractProtocolHandler : public IProtocolHandler
    {
        public:
            AbstractProtocolHandler(IProcedureInvokationHandler &handler);
            virtual ~AbstractProtocolHandler();

            void HandleRequest(const std::string& request, std::string& retValue);

            virtual void AddProcedure(const Procedure& procedure);

            virtual void HandleJsonRequest(const Json::Value& request, Json::Value& response) = 0;
            virtual bool ValidateRequestFields(const Json::Value &val) = 0;
            virtual void WrapResult(const Json::Value& request, Json::Value& response, Json::Value& retValue) = 0;
            virtual void WrapError(const Json::Value& request, int code, const std::string &message, Json::Value& result) = 0;
            virtual procedure_t GetRequestType(const Json::Value& request) = 0;

        protected:
            IProcedureInvokationHandler &handler;
            std::map<std::string, Procedure> procedures;

            virtual void ProcessRequest(const Json::Value &request, Json::Value &retValue);
            virtual int ValidateRequest(const Json::Value &val);

    };

    class AbstractProtocolProxyHandler : public AbstractProtocolHandler
    {
        public:
            AbstractProtocolProxyHandler(IProcedureInvokationHandler &handler);
            virtual ~AbstractProtocolProxyHandler();

        protected:
            void ProcessRequest(const Json::Value &request, Json::Value &retValue) override;
            int ValidateRequest(const Json::Value &val) override;
    };


} // namespace jsonrpc

#endif // JSONRPC_CPP_ABSTRACTPROTOCOLHANDLER_H
