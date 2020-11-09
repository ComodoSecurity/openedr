/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    filedescriptorclient.cpp
 * @date    26.10.2016
 * @author  Jean-Daniel Michaud <jean.daniel.michaud@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "filedescriptorclient.h"
#include "../../common/sharedconstants.h"
#include "../../common/streamreader.h"
#include "../../common/streamwriter.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <unistd.h>

#define MEX_ERR_MSG 255

using namespace jsonrpc;
using namespace std;

FileDescriptorClient::FileDescriptorClient(int inputfd, int outputfd)
    : inputfd(inputfd), outputfd(outputfd) {}

FileDescriptorClient::~FileDescriptorClient() {}

void FileDescriptorClient::SendRPCMessage(const std::string &message,
                                          std::string &result) {

  string toSend = message + DEFAULT_DELIMITER_CHAR;
  StreamWriter writer;

  if (!writer.Write(toSend, outputfd)) {
    throw JsonRpcException(
        Errors::ERROR_CLIENT_CONNECTOR,
        "Unknown error occurred while writing to the output file descriptor");
  }

  if (!IsReadable(inputfd))
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR,
                           "The input file descriptor is not readable");

  StreamReader reader(DEFAULT_BUFFER_SIZE);
  if (!reader.Read(result, inputfd, DEFAULT_DELIMITER_CHAR)) {
    throw JsonRpcException(
        Errors::ERROR_CLIENT_CONNECTOR,
        "Unknown error occurred while reading from input file descriptor");
  }
}

bool FileDescriptorClient::IsReadable(int fd) {
  int o_accmode = 0;
  int ret = fcntl(fd, F_GETFL, &o_accmode);
  if (ret == -1)
    return false;
  return ((o_accmode & O_ACCMODE) == O_RDONLY ||
          (o_accmode & O_ACCMODE) == O_RDWR);
}
