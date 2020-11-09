/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    requesthandlerfactory.cpp
 * @date    10/23/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "requesthandlerfactory.h"
#include "rpcprotocolserver12.h"
#include "rpcprotocolserverv1.h"
#include "rpcprotocolserverv2.h"

using namespace jsonrpc;

IProtocolHandler *RequestHandlerFactory::createProtocolHandler(
    serverVersion_t type, IProcedureInvokationHandler &handler) {
  IProtocolHandler *result = NULL;
  switch (type) {
  case JSONRPC_SERVER_V1:
    result = new RpcProtocolServerV1(handler);
    break;
  case JSONRPC_SERVER_V2:
    result = new RpcProtocolServerV2(handler);
    break;
  case JSONRPC_SERVER_V2_PROXY:
    result = new RpcProtocolServerV2Proxy(handler);
    break;
  case JSONRPC_SERVER_V1V2:
    result = new RpcProtocolServer12(handler);
    break;
  }
  return result;
}
