/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    linuxtcpsocketserver.cpp
 * @date    17.07.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "linuxtcpsocketserver.h"
#include "../../common/sharedconstants.h"
#include "../../common/streamreader.h"
#include "../../common/streamwriter.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <errno.h>
#include <iostream>
#include <sstream>
#include <string>

using namespace jsonrpc;
using namespace std;

LinuxTcpSocketServer::LinuxTcpSocketServer(const std::string &ipToBind,
                                           const unsigned int &port,
                                           size_t threads)
    : AbstractThreadedServer(threads), ipToBind(ipToBind), port(port) {}

LinuxTcpSocketServer::~LinuxTcpSocketServer() {
  shutdown(this->socket_fd, 2);
  close(this->socket_fd);
}

bool LinuxTcpSocketServer::InitializeListener() {
  this->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->socket_fd < 0) {
    return false;
  }

  fcntl(this->socket_fd, F_SETFL, FNDELAY);
  int reuseaddr = 1;
  setsockopt(this->socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr,
             sizeof(reuseaddr));

  /* start with a clean address structure */
  memset(&(this->address), 0, sizeof(struct sockaddr_in));

  this->address.sin_family = AF_INET;
  inet_aton(this->ipToBind.c_str(), &(this->address.sin_addr));
  this->address.sin_port = htons(this->port);

  if (::bind(this->socket_fd,
             reinterpret_cast<struct sockaddr *>(&(this->address)),
             sizeof(struct sockaddr_in)) != 0) {
    return false;
  }

  if (listen(this->socket_fd, 5) != 0) {
    return false;
  }
  return true;
}

int LinuxTcpSocketServer::CheckForConnection() {
  struct sockaddr_in connection_address;
  memset(&connection_address, 0, sizeof(struct sockaddr_in));
  socklen_t address_length = sizeof(connection_address);
  return accept(this->socket_fd,
                reinterpret_cast<struct sockaddr *>(&(connection_address)),
                &address_length);
}

void LinuxTcpSocketServer::HandleConnection(int connection) {
  StreamReader reader(DEFAULT_BUFFER_SIZE);
  string request, response;

  reader.Read(request, connection, DEFAULT_DELIMITER_CHAR);

  this->ProcessRequest(request, response);

  response.append(1, DEFAULT_DELIMITER_CHAR);
  StreamWriter writer;
  writer.Write(response, connection);
  CleanClose(connection);
}

bool LinuxTcpSocketServer::WaitClientClose(const int &fd, const int &timeout) {
  bool ret = false;
  int i = 0;
  while ((recv(fd, NULL, 0, 0) != 0) && i < timeout) {
    usleep(1);
    ++i;
    ret = true;
  }

  return ret;
}

int LinuxTcpSocketServer::CloseByReset(const int &fd) {
  struct linger so_linger;
  so_linger.l_onoff = 1;
  so_linger.l_linger = 0;

  int ret =
      setsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger));
  if (ret != 0)
    return ret;

  return close(fd);
}

int LinuxTcpSocketServer::CleanClose(const int &fd) {
  if (WaitClientClose(fd)) {
    return close(fd);
  } else {
    return CloseByReset(fd);
  }
}
