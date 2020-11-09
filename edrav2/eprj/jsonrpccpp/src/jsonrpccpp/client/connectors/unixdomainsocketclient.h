/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    unixdomainsocketclient.h
 * @date    11.05.2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_UNIXDOMAINSOCKETCLIENT_H_
#define JSONRPC_CPP_UNIXDOMAINSOCKETCLIENT_H_

#include "../iclientconnector.h"
#include <jsonrpccpp/common/exception.h>

namespace jsonrpc
{
    class UnixDomainSocketClient : public IClientConnector
    {
        public:
            UnixDomainSocketClient(const std::string& path);
            virtual ~UnixDomainSocketClient();
            virtual void SendRPCMessage(const std::string& message, std::string& result) ;

        private:
            std::string path;
    };

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_UNIXDOMAINSOCKETCLIENT_H_ */
