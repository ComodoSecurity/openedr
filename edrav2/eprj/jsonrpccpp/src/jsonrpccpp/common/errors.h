/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    errors.h
 * @date    31.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_ERRORS_H_
#define JSONRPC_CPP_ERRORS_H_

#include <map>
#include <string>

#include "jsonparser.h"

namespace jsonrpc
{   
    class JsonRpcException;

    class Errors
    {
        public:
            /**
             * @return error message to corresponding error code.
             */
            static std::string GetErrorMessage(int errorCode);

            static class _init
            {
                public:
                _init();
            } _initializer;

            /**
             * Official JSON-RPC 2.0 Errors
             */
            static const int ERROR_RPC_JSON_PARSE_ERROR;
            static const int ERROR_RPC_METHOD_NOT_FOUND;
            static const int ERROR_RPC_INVALID_REQUEST;
            static const int ERROR_RPC_INVALID_PARAMS;
            static const int ERROR_RPC_INTERNAL_ERROR;

            /**
             * Server Library Errors
             */
            static const int ERROR_SERVER_PROCEDURE_IS_METHOD;
            static const int ERROR_SERVER_PROCEDURE_IS_NOTIFICATION;
            static const int ERROR_SERVER_PROCEDURE_POINTER_IS_NULL;
            static const int ERROR_SERVER_PROCEDURE_SPECIFICATION_NOT_FOUND;
            static const int ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX;
            static const int ERROR_SERVER_CONNECTOR;

            /**
             * Client Library Errors
             */
            static const int ERROR_CLIENT_CONNECTOR;
            static const int ERROR_CLIENT_INVALID_RESPONSE;
        private:
            static std::map<int, std::string> possibleErrors;
    };
} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_ERRORS_H_ */
