/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    filedescriptorclient.h
 * @date    26.10.2016
 * @author  Jean-Daniel Michaud <jean.daniel.michaud@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_FILEDESCRITPTORCLIENT_H_
#define JSONRPC_CPP_FILEDESCRITPTORCLIENT_H_

#include "../iclientconnector.h"
#include <jsonrpccpp/common/exception.h>

namespace jsonrpc
{
  class FileDescriptorClient : public IClientConnector
  {
    public:
      FileDescriptorClient(int inputfd, int outputfd);
      virtual ~FileDescriptorClient();
      virtual void SendRPCMessage(const std::string& message, std::string& result) ;

    private:
      int inputfd;
      int outputfd;

      bool IsReadable(int fd);
  };

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_FILEDESCRITPTORCLIENT_H_ */
