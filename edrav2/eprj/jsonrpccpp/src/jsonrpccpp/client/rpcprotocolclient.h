/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    responsehandler.h
 * @date    13.03.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef RESPONSEHANDLER_H
#define RESPONSEHANDLER_H

#include <jsonrpccpp/common/exception.h>
#include <jsonrpccpp/common/jsonparser.h>
#include <string>
#include "client.h"

namespace jsonrpc {


    /**
     * @brief The RpcProtocolClient class handles the json-rpc 2.0 protocol for the client side.
     */
    class RpcProtocolClient
    {
        public:
            RpcProtocolClient(clientVersion_t version = JSONRPC_CLIENT_V2, bool omitEndingLineFeed = false);

            /**
             * @brief This method builds a valid json-rpc 2.0 request object based on passed parameters.
             * The id starts at 1 and is incremented for each request. To reset this value to one, call
             * the jsonrpc::RpcProRpcProtocolClient::resetId() method.
             * @param method - name of method or notification to be called
             * @param parameter - parameters represented as json objects
             * @return the string representation of the request to be built.
             */
            std::string BuildRequest(const std::string& method, const Json::Value& parameter, bool isNotification);

            /**
             * @brief BuildRequest does the same as std::string jsonrpc::RpcProRpcProtocolClient::BuildRequest(const std::string& method, const Json::Value& parameter);
             * The only difference here is that the result is returend by value, using the result parameter.
             * @param method - name of method or notification to be called
             * @param parameter - parameters represented as json objects
             * @param result - the string representation will be hold within this reference.
             */
            void BuildRequest(const std::string& method, const Json::Value& parameter, std::string& result, bool isNotification);


            /**
             * @brief Does the same as Json::Value RpcProtocolClient::HandleResponse(const std::string& response) throw(Exception)
             * but returns result as reference for performance speed up.
             */
            void HandleResponse(const std::string &response, Json::Value &result) ;

            /**
             * @brief HandleResponse
             * @param response
             * @param result
             * @return response id
             */
            Json::Value HandleResponse(const Json::Value &response, Json::Value &result) ;

            static const std::string KEY_PROTOCOL_VERSION;
            static const std::string KEY_PROCEDURE_NAME;
            static const std::string KEY_ID;
            static const std::string KEY_PARAMETER;
            static const std::string KEY_AUTH;
            static const std::string KEY_RESULT;
            static const std::string KEY_ERROR;
            static const std::string KEY_ERROR_CODE;
            static const std::string KEY_ERROR_MESSAGE;
            static const std::string KEY_ERROR_DATA;

        private:
            clientVersion_t version;
            bool omitEndingLineFeed;

            void BuildRequest(int id, const std::string& method, const Json::Value& parameter, Json::Value& result, bool isNotification);
            bool ValidateResponse(const Json::Value &response);
            bool HasError(const Json::Value &response);
            void throwErrorException(const Json::Value &response);
    };
}
#endif // RESPONSEHANDLER_H
