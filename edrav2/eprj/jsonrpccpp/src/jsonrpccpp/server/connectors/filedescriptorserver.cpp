/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    filedescriptorserver.h
 * @date    25.10.2016
 * @author  Jean-Daniel Michaud <jean.daniel.michaud@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "filedescriptorserver.h"
#include "../../common/sharedconstants.h"

#include <fcntl.h>
#include <string>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace jsonrpc;
using namespace std;

#define BUFFER_SIZE 1024
#ifndef DELIMITER_CHAR
#define DELIMITER_CHAR char(0x0A)
#endif
#define READ_TIMEOUT 0.001 // Set timeout in seconds

FileDescriptorServer::FileDescriptorServer(int inputfd, int outputfd)
    : AbstractThreadedServer(0), inputfd(inputfd), outputfd(outputfd),
      reader(DEFAULT_BUFFER_SIZE) {}

bool FileDescriptorServer::InitializeListener() {
  if (!IsReadable(inputfd) || !IsWritable(outputfd))
    return false;
  return true;
}

int FileDescriptorServer::CheckForConnection() {
  FD_ZERO(&read_fds);
  FD_ZERO(&write_fds);
  FD_ZERO(&except_fds);
  FD_SET(inputfd, &read_fds);
  timeout.tv_sec = 0;
  timeout.tv_usec = (suseconds_t)(READ_TIMEOUT * 1000000);
  // Wait for something to read
  return select(inputfd + 1, &read_fds, &write_fds, &except_fds, &timeout);
}

void FileDescriptorServer::HandleConnection(int connection) {
  (void)(connection);
  string request, response;
  reader.Read(request, inputfd, DEFAULT_DELIMITER_CHAR);
  this->ProcessRequest(request, response);
  response.append(1, DEFAULT_DELIMITER_CHAR);
  writer.Write(response, outputfd);
}

bool FileDescriptorServer::IsReadable(int fd) {
  int o_accmode = 0;
  int ret = fcntl(fd, F_GETFL, &o_accmode);
  if (ret == -1)
    return false;
  return ((o_accmode & O_ACCMODE) == O_RDONLY ||
          (o_accmode & O_ACCMODE) == O_RDWR);
}

bool FileDescriptorServer::IsWritable(int fd) {
  int ret = fcntl(fd, F_GETFL);
  if (ret == -1)
    return false;
  return ((ret & O_WRONLY) || (ret & O_RDWR));
}
