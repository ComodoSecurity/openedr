/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketserver.h
 * @date    07.05.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_UNIXDOMAINSOCKETSERVERCONNECTOR_H_
#define JSONRPC_CPP_UNIXDOMAINSOCKETSERVERCONNECTOR_H_

#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "../../common/streamreader.h"
#include "../../common/streamwriter.h"
#include "../abstractthreadedserver.h"

namespace jsonrpc {
/**
 * This class provides an embedded Unix Domain Socket Server,to handle incoming
 * Requests.
 */
class UnixDomainSocketServer : public AbstractThreadedServer {
public:
  /**
   * @brief UnixDomainSocketServer, constructor for the included
   * UnixDomainSocketServer
   * @param socket_path, a string containing the path to the unix socket
   */
  UnixDomainSocketServer(const std::string &socket_path, size_t threads = 1);
  virtual ~UnixDomainSocketServer();

  virtual bool InitializeListener();
  virtual int CheckForConnection();
  virtual void HandleConnection(int connection);

private:
  std::string socket_path;
  int socket_fd;
  struct sockaddr_un address;
};

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_HTTPSERVERCONNECTOR_H_ */
