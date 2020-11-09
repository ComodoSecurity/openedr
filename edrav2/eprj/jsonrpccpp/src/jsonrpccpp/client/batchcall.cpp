/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    batchcall.cpp
 * @date    15.10.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "batchcall.h"
#include "rpcprotocolclient.h"

using namespace jsonrpc;
using namespace std;

BatchCall::BatchCall() : id(1) {}

int BatchCall::addCall(const string &methodname, const Json::Value &params,
                       bool isNotification) {
  Json::Value call;
  call[RpcProtocolClient::KEY_PROTOCOL_VERSION] = "2.0";
  call[RpcProtocolClient::KEY_PROCEDURE_NAME] = methodname;

  if(params.isNull() || params.size() > 0)
    call[RpcProtocolClient::KEY_PARAMETER] = params;

  if (!isNotification) {
    call[RpcProtocolClient::KEY_ID] = this->id++;
  }
  result.append(call);

  if (isNotification)
    return -1;
  return call[RpcProtocolClient::KEY_ID].asInt();
}

string BatchCall::toString(bool fast) const {
  string result;
  if (fast) {
    Json::FastWriter writer;
    result = writer.write(this->result);
  } else {
    Json::StyledWriter writer;
    result = writer.write(this->result);
  }
  return result;
}
