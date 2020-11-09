/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    abstractserverconnector.cpp
 * @date    31.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "abstractserverconnector.h"
#include <cstdlib>
#include <jsonrpccpp/common/specificationwriter.h>

using namespace std;
using namespace jsonrpc;

AbstractServerConnector::AbstractServerConnector() { this->handler = NULL; }

AbstractServerConnector::~AbstractServerConnector() {}

void AbstractServerConnector::ProcessRequest(const string &request,
                                             string &response) {
  if (this->handler != NULL) {
    this->handler->HandleRequest(request, response);
  }
}

void AbstractServerConnector::SetHandler(IClientConnectionHandler *handler) {
  this->handler = handler;
}

IClientConnectionHandler *AbstractServerConnector::GetHandler() {
  return this->handler;
}
