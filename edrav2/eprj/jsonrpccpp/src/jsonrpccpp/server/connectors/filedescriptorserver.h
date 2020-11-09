/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    filedescriptorserver.h
 * @date    25.10.2016
 * @author  Jean-Daniel Michaud <jean.daniel.michaud@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_FILEDESCRIPTORSERVERCONNECTOR_H_
#define JSONRPC_CPP_FILEDESCRIPTORSERVERCONNECTOR_H_

#include "../../common/streamreader.h"
#include "../../common/streamwriter.h"
#include "../abstractthreadedserver.h"
#include <pthread.h>
#include <string>
#include <sys/select.h>
#include <sys/time.h>

namespace jsonrpc {
/**
 * This class is the file descriptor implementation of an
 * AbstractServerConnector.
 * It uses the POSIX file API and POSIX thread API to performs its job.
 * Each client request is handled in a new thread.
 */
class FileDescriptorServer : public AbstractThreadedServer {
public:
  /**
   * AbstractServerConnector
   * @param inputfd The file descriptor already open for us to read
   * @param outputfd The file descriptor already open for us to write
   */
  FileDescriptorServer(int inputfd, int outputfd);

  virtual bool InitializeListener();
  virtual int CheckForConnection();
  virtual void HandleConnection(int connection);

private:
  int inputfd;
  int outputfd;
  StreamReader reader;
  StreamWriter writer;

  bool IsReadable(int fd);
  bool IsWritable(int fd);

  // For select operation
  fd_set read_fds;
  fd_set write_fds;
  fd_set except_fds;
  struct timeval timeout;
};
}

#endif /* JSONRPC_CPP_FILEDESCRIPTORSERVERCONNECTOR_H_ */
