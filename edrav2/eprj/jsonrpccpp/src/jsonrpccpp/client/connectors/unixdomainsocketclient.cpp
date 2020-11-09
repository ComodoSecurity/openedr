/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketclient.cpp
 * @date    11.05.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "unixdomainsocketclient.h"
#include "../../common/sharedconstants.h"
#include "../../common/streamreader.h"
#include "../../common/streamwriter.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace jsonrpc;
using namespace std;

UnixDomainSocketClient::UnixDomainSocketClient(const std::string &path)
    : path(path) {}

UnixDomainSocketClient::~UnixDomainSocketClient() {}

void UnixDomainSocketClient::SendRPCMessage(const std::string &message,
                                            std::string &result) {
  sockaddr_un address;
  int socket_fd;
  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                           "Could not create unix domain socket");
  }
  memset(&address, 0, sizeof(sockaddr_un));

  address.sun_family = AF_UNIX;
  strncpy(address.sun_path, this->path.c_str(), 107);

  if (connect(socket_fd, (struct sockaddr *)&address, sizeof(sockaddr_un)) !=
      0) {
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                           "Could not connect to: " + this->path);
  }

  StreamWriter writer;
  string toSend = message + DEFAULT_DELIMITER_CHAR;
  if (!writer.Write(toSend, socket_fd)) {
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                           "Could not write request");
  }

  StreamReader reader(DEFAULT_BUFFER_SIZE);
  if (!reader.Read(result, socket_fd, DEFAULT_DELIMITER_CHAR)) {
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                           "Could not read response");
  }
  close(socket_fd);
}
