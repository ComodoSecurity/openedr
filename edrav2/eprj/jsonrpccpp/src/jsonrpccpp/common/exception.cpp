/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    exception.cpp
 * @date    31.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "exception.h"

using namespace jsonrpc;

JsonRpcException::JsonRpcException(int code)
    : code(code), message(Errors::GetErrorMessage(code)) {
  this->setWhatMessage();
}

JsonRpcException::JsonRpcException(int code, const std::string &message)
    : code(code), message(Errors::GetErrorMessage(code)) {
  if (this->message != "")
    this->message = this->message + " (" + message + ")";
  else
    this->message = this->message + message;
  this->setWhatMessage();
}

JsonRpcException::JsonRpcException(int code, const std::string &message,
                                   const Json::Value &data)
    : code(code), message(Errors::GetErrorMessage(code)), data(data) {
  if (this->message != "")
    this->message = this->message + " (" + message + ")";
  else
    this->message = this->message + message;
  this->setWhatMessage();
}

JsonRpcException::JsonRpcException(const std::string &message)
    : code(0), message(message) {
  this->setWhatMessage();
}

JsonRpcException::~JsonRpcException() throw() {}

int JsonRpcException::GetCode() const { return code; }

const std::string &JsonRpcException::GetMessage() const { return message; }

const Json::Value &JsonRpcException::GetData() const { return data; }

const char *JsonRpcException::what() const throw() {
  return this->whatString.c_str();
}

void JsonRpcException::setWhatMessage() {
  if (this->code != 0) {
    std::stringstream ss;
    ss << "JSON-RPC error: " << this->code << " - " << this->message;
    if (data != Json::nullValue)
      ss << " data=" << data.toStyledString();
    this->whatString = ss.str();
  } else {
    this->whatString = this->message;
  }
}
