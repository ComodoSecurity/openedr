/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    rpcprotocolserverv2.cpp
 * @date    31.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "rpcprotocolserverv2.h"
#include <iostream>
#include <jsonrpccpp/common/errors.h>

using namespace std;
using namespace jsonrpc;

RpcProtocolServerV2::RpcProtocolServerV2(IProcedureInvokationHandler &handler)
    : AbstractProtocolHandler(handler) {}

void RpcProtocolServerV2::HandleJsonRequest(const Json::Value &req,
                                            Json::Value &response) {
  // It could be a Batch Request
  if (req.isArray()) {
    this->HandleBatchRequest(req, response);
  } // It could be a simple Request
  else if (req.isObject()) {
    this->HandleSingleRequest(req, response);
  } else {
    this->WrapError(Json::nullValue, Errors::ERROR_RPC_INVALID_REQUEST,
                    Errors::GetErrorMessage(Errors::ERROR_RPC_INVALID_REQUEST),
                    response);
  }
}
void RpcProtocolServerV2::HandleSingleRequest(const Json::Value &req,
                                              Json::Value &response) {
  int error = this->ValidateRequest(req);
  if (error == 0) {
    try {
      this->ProcessRequest(req, response);
    } catch (const JsonRpcException &exc) {
      this->WrapException(req, exc, response);
    }
  } else {
    this->WrapError(req, error, Errors::GetErrorMessage(error), response);
  }
}
void RpcProtocolServerV2::HandleBatchRequest(const Json::Value &req,
                                             Json::Value &response) {
  if (req.size() == 0)
    this->WrapError(Json::nullValue, Errors::ERROR_RPC_INVALID_REQUEST,
                    Errors::GetErrorMessage(Errors::ERROR_RPC_INVALID_REQUEST),
                    response);
  else {
    for (unsigned int i = 0; i < req.size(); i++) {
      Json::Value result;
      this->HandleSingleRequest(req[i], result);
      if (result != Json::nullValue)
        response.append(result);
    }
  }
}
bool RpcProtocolServerV2::ValidateRequestFields(const Json::Value &request) {
  if (!request.isObject())
    return false;
  if (!(request.isMember(KEY_REQUEST_METHODNAME) &&
        request[KEY_REQUEST_METHODNAME].isString()))
    return false;
  if (!(request.isMember(KEY_REQUEST_VERSION) &&
        request[KEY_REQUEST_VERSION].isString() &&
        request[KEY_REQUEST_VERSION].asString() == JSON_RPC_VERSION2))
    return false;
  if (request.isMember(KEY_REQUEST_ID) &&
      !(request[KEY_REQUEST_ID].isIntegral() ||
        request[KEY_REQUEST_ID].isString() || request[KEY_REQUEST_ID].isNull()))
    return false;
  if (request.isMember(KEY_REQUEST_PARAMETERS) &&
      !(request[KEY_REQUEST_PARAMETERS].isObject() ||
        request[KEY_REQUEST_PARAMETERS].isArray() ||
        request[KEY_REQUEST_PARAMETERS].isNull()))
    return false;
  return true;
}

void RpcProtocolServerV2::WrapResult(const Json::Value &request,
                                     Json::Value &response,
                                     Json::Value &result) {
  response[KEY_REQUEST_VERSION] = JSON_RPC_VERSION2;
  response[KEY_RESPONSE_RESULT] = result;
  response[KEY_REQUEST_ID] = request[KEY_REQUEST_ID];
}

void RpcProtocolServerV2::WrapError(const Json::Value &request, int code,
                                    const string &message,
                                    Json::Value &result) {
  result["jsonrpc"] = "2.0";
  result["error"]["code"] = code;
  result["error"]["message"] = message;

  if (request.isObject() && request.isMember("id") &&
      (request["id"].isNull() || request["id"].isIntegral() ||
       request["id"].isString())) {
    result["id"] = request["id"];
  } else {
    result["id"] = Json::nullValue;
  }
}

void RpcProtocolServerV2::WrapException(const Json::Value &request,
                                        const JsonRpcException &exception,
                                        Json::Value &result) {
  this->WrapError(request, exception.GetCode(), exception.GetMessage(), result);
  result["error"]["data"] = exception.GetData();
}

procedure_t RpcProtocolServerV2::GetRequestType(const Json::Value &request) {
  if (request.isMember(KEY_REQUEST_ID))
    return RPC_METHOD;
  return RPC_NOTIFICATION;
}

RpcProtocolServerV2Proxy::RpcProtocolServerV2Proxy(IProcedureInvokationHandler &handler)
    : AbstractProtocolProxyHandler(handler) {}

void RpcProtocolServerV2Proxy::HandleJsonRequest(const Json::Value &req,
                                            Json::Value &response) {
  // It could be a Batch Request
  if (req.isArray()) {
    this->HandleBatchRequest(req, response);
  } // It could be a simple Request
  else if (req.isObject()) {
    this->HandleSingleRequest(req, response);
  } else {
    this->WrapError(Json::nullValue, Errors::ERROR_RPC_INVALID_REQUEST,
                    Errors::GetErrorMessage(Errors::ERROR_RPC_INVALID_REQUEST),
                    response);
  }
}
void RpcProtocolServerV2Proxy::HandleSingleRequest(const Json::Value &req,
                                              Json::Value &response) {
  int error = this->ValidateRequest(req);
  if (error == 0) {
    try {
      this->ProcessRequest(req, response);
    } catch (const JsonRpcException &exc) {
      this->WrapException(req, exc, response);
    }
  } else {
    this->WrapError(req, error, Errors::GetErrorMessage(error), response);
  }
}
void RpcProtocolServerV2Proxy::HandleBatchRequest(const Json::Value &req,
                                             Json::Value &response) {
  if (req.size() == 0)
    this->WrapError(Json::nullValue, Errors::ERROR_RPC_INVALID_REQUEST,
                    Errors::GetErrorMessage(Errors::ERROR_RPC_INVALID_REQUEST),
                    response);
  else {
    for (unsigned int i = 0; i < req.size(); i++) {
      Json::Value result;
      this->HandleSingleRequest(req[i], result);
      if (result != Json::nullValue)
        response.append(result);
    }
  }
}
bool RpcProtocolServerV2Proxy::ValidateRequestFields(const Json::Value &request) {
  if (!request.isObject())
    return false;
  if (!(request.isMember(KEY_REQUEST_METHODNAME) &&
        request[KEY_REQUEST_METHODNAME].isString()))
    return false;
  if (!(request.isMember(KEY_REQUEST_VERSION) &&
        request[KEY_REQUEST_VERSION].isString() &&
        request[KEY_REQUEST_VERSION].asString() == JSON_RPC_VERSION2))
    return false;
  if (request.isMember(KEY_REQUEST_ID) &&
      !(request[KEY_REQUEST_ID].isIntegral() ||
        request[KEY_REQUEST_ID].isString() || request[KEY_REQUEST_ID].isNull()))
    return false;
  if (request.isMember(KEY_REQUEST_PARAMETERS) &&
      !(request[KEY_REQUEST_PARAMETERS].isObject() ||
        request[KEY_REQUEST_PARAMETERS].isArray() ||
        request[KEY_REQUEST_PARAMETERS].isNull()))
    return false;
  return true;
}

void RpcProtocolServerV2Proxy::WrapResult(const Json::Value &request,
                                     Json::Value &response,
                                     Json::Value &result) {
  response[KEY_REQUEST_VERSION] = JSON_RPC_VERSION2;
  response[KEY_RESPONSE_RESULT] = result;
  response[KEY_REQUEST_ID] = request[KEY_REQUEST_ID];
}

void RpcProtocolServerV2Proxy::WrapError(const Json::Value &request, int code,
                                    const string &message,
                                    Json::Value &result) {
  result["jsonrpc"] = "2.0";
  result["error"]["code"] = code;
  result["error"]["message"] = message;

  if (request.isObject() && request.isMember("id") &&
      (request["id"].isNull() || request["id"].isIntegral() ||
       request["id"].isString())) {
    result["id"] = request["id"];
  } else {
    result["id"] = Json::nullValue;
  }
}

void RpcProtocolServerV2Proxy::WrapException(const Json::Value &request,
                                        const JsonRpcException &exception,
                                        Json::Value &result) {
  this->WrapError(request, exception.GetCode(), exception.GetMessage(), result);
  result["error"]["data"] = exception.GetData();
}

procedure_t RpcProtocolServerV2Proxy::GetRequestType(const Json::Value &request) {
  if (request.isMember(KEY_REQUEST_ID))
    return RPC_METHOD;
  return RPC_NOTIFICATION;
}
