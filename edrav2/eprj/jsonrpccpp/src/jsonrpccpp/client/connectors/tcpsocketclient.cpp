/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    tcpsocketclient.cpp
 * @date    17.07.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "tcpsocketclient.h"

#ifdef _WIN32
#include "windowstcpsocketclient.h"
#else
#include "linuxtcpsocketclient.h"
#endif

using namespace jsonrpc;
using namespace std;

TcpSocketClient::TcpSocketClient(const std::string &ipToConnect,
                                 const unsigned int &port) {
#ifdef _WIN32
  this->realSocket = new WindowsTcpSocketClient(ipToConnect, port);
#else
  this->realSocket = new LinuxTcpSocketClient(ipToConnect, port);
#endif
}

TcpSocketClient::~TcpSocketClient() {
  delete this->realSocket;
}

void TcpSocketClient::SendRPCMessage(const std::string &message,
                                     std::string &result) {
  if (this->realSocket != NULL) {
    this->realSocket->SendRPCMessage(message, result);
  }
}
