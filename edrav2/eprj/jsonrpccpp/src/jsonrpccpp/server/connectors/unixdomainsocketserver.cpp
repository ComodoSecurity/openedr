/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketserver.cpp
 * @date    07.05.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "unixdomainsocketserver.h"
#include "../../common/sharedconstants.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <unistd.h>

using namespace jsonrpc;
using namespace std;

UnixDomainSocketServer::UnixDomainSocketServer(const string &socket_path,
                                               size_t threads)
    : AbstractThreadedServer(threads), socket_path(socket_path), socket_fd(-1) {
}

UnixDomainSocketServer::~UnixDomainSocketServer() {
  if (this->socket_fd != -1)
    close(this->socket_fd);
  unlink(this->socket_path.c_str());
}

bool UnixDomainSocketServer::InitializeListener() {
  if (access(this->socket_path.c_str(), F_OK) != -1)
    return false;

  this->socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

  if (this->socket_fd < 0) {
    return false;
  }

  // Set to non-blocking mode
  fcntl(this->socket_fd, F_SETFL, FNDELAY);

  memset(&(this->address), 0, sizeof(struct sockaddr_un));
  this->address.sun_family = AF_UNIX;
  strncpy(this->address.sun_path, this->socket_path.c_str(), 107);

  if (::bind(this->socket_fd,
             reinterpret_cast<struct sockaddr *>(&(this->address)),
             sizeof(struct sockaddr_un)) != 0) {
    return false;
  }

  if (listen(this->socket_fd, 5) != 0) {
    return false;
  }
  return true;
}

int UnixDomainSocketServer::CheckForConnection() {
  socklen_t address_length = sizeof(this->address);

  int fd;
  fd = accept(this->socket_fd,
              reinterpret_cast<struct sockaddr *>(&(this->address)),
              &address_length);
  return fd;
}

void UnixDomainSocketServer::HandleConnection(int connection) {
  string request, response;
  StreamReader reader(DEFAULT_BUFFER_SIZE);
  reader.Read(request, connection, DEFAULT_DELIMITER_CHAR);
  this->ProcessRequest(request, response);

  response.append(1, DEFAULT_DELIMITER_CHAR);
  StreamWriter writer;
  writer.Write(response, connection);

  close(connection);
}
