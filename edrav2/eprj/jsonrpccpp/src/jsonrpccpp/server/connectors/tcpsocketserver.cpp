/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    tcpsocketserver.cpp
 * @date    17.07.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "tcpsocketserver.h"
#ifdef _WIN32
#include "windowstcpsocketserver.h"
#else
#include "linuxtcpsocketserver.h"
#endif
#include <string>

using namespace jsonrpc;
using namespace std;

TcpSocketServer::TcpSocketServer(const std::string &ipToBind,
                                 const unsigned int &port)
    : AbstractServerConnector() {
#ifdef _WIN32
  this->realSocket = new WindowsTcpSocketServer(ipToBind, port);
#else
  this->realSocket = new LinuxTcpSocketServer(ipToBind, port);
#endif
}

TcpSocketServer::~TcpSocketServer() {
 delete this->realSocket;
}

bool TcpSocketServer::StartListening() {
  if (this->realSocket != NULL) {
    this->realSocket->SetHandler(this->GetHandler());
    return this->realSocket->StartListening();
  } else
    return false;
}

bool TcpSocketServer::StopListening() {
  if (this->realSocket != NULL)
    return this->realSocket->StopListening();
  else
    return false;
}
