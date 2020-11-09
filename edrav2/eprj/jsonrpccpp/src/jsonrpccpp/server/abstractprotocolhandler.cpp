/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    abstractprotocolhandler.cpp
 * @date    10/23/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "abstractprotocolhandler.h"
#include <jsonrpccpp/common/errors.h>
#include <jsonrpccpp/common/jsonparser.h>

#include <map>

using namespace jsonrpc;
using namespace std;

AbstractProtocolHandler::AbstractProtocolHandler(
    IProcedureInvokationHandler &handler)
    : handler(handler) {}

AbstractProtocolHandler::~AbstractProtocolHandler() {}

void AbstractProtocolHandler::AddProcedure(const Procedure &procedure) {
  this->procedures[procedure.GetProcedureName()] = procedure;
}

void AbstractProtocolHandler::HandleRequest(const std::string &request,
                                            std::string &retValue) {
  Json::Reader reader;
  Json::Value req;
  Json::Value resp;
  Json::FastWriter w;

  if (reader.parse(request, req, false)) {
    this->HandleJsonRequest(req, resp);
  } else {
    this->WrapError(Json::nullValue, Errors::ERROR_RPC_JSON_PARSE_ERROR,
                    Errors::GetErrorMessage(Errors::ERROR_RPC_JSON_PARSE_ERROR),
                    resp);
  }

  if (resp != Json::nullValue)
    retValue = w.write(resp);
}

void AbstractProtocolHandler::ProcessRequest(const Json::Value &request,
                                             Json::Value &response) {
  Procedure &method =
      this->procedures[request[KEY_REQUEST_METHODNAME].asString()];
  Json::Value result;

  if (method.GetProcedureType() == RPC_METHOD) {
    handler.HandleMethodCall(method, request[KEY_REQUEST_PARAMETERS], result);
    this->WrapResult(request, response, result);
  } else {
    handler.HandleNotificationCall(method, request[KEY_REQUEST_PARAMETERS]);
    response = Json::nullValue;
  }
}

int AbstractProtocolHandler::ValidateRequest(const Json::Value &request) {
  int error = 0;
  Procedure proc;
  if (!this->ValidateRequestFields(request)) {
    error = Errors::ERROR_RPC_INVALID_REQUEST;
  } else {
    map<string, Procedure>::iterator it =
        this->procedures.find(request[KEY_REQUEST_METHODNAME].asString());
    if (it != this->procedures.end()) {
      proc = it->second;
      if (this->GetRequestType(request) == RPC_METHOD &&
          proc.GetProcedureType() == RPC_NOTIFICATION) {
        error = Errors::ERROR_SERVER_PROCEDURE_IS_NOTIFICATION;
      } else if (this->GetRequestType(request) == RPC_NOTIFICATION &&
                 proc.GetProcedureType() == RPC_METHOD) {
        error = Errors::ERROR_SERVER_PROCEDURE_IS_METHOD;
      } else if (!proc.ValdiateParameters(request[KEY_REQUEST_PARAMETERS])) {
        error = Errors::ERROR_RPC_INVALID_PARAMS;
      }
    } else {
      error = Errors::ERROR_RPC_METHOD_NOT_FOUND;
    }
  }
  return error;
}

AbstractProtocolProxyHandler::AbstractProtocolProxyHandler(
    IProcedureInvokationHandler &handler)
    : AbstractProtocolHandler(handler) {}

AbstractProtocolProxyHandler::~AbstractProtocolProxyHandler() {}

void AbstractProtocolProxyHandler::ProcessRequest(const Json::Value &request,
                                                  Json::Value &response) {
  Json::Value result;
  handler.HandleMethodCall(request[KEY_REQUEST_METHODNAME].asString(),
    request[KEY_REQUEST_PARAMETERS], result);
  this->WrapResult(request, response, result);
}

int AbstractProtocolProxyHandler::ValidateRequest(const Json::Value &request) {
  if (!this->ValidateRequestFields(request))
    return Errors::ERROR_RPC_INVALID_REQUEST;
  return 0;
}