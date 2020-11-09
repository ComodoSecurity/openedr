/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    testhttpserver.h
 * @date    11/16/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_TESTHTTPSERVER_H
#define JSONRPC_TESTHTTPSERVER_H

#include <jsonrpccpp/server/abstractserverconnector.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <map>

namespace jsonrpc {

    class TestHttpServer : public AbstractServerConnector
    {
        public:
            TestHttpServer(int port);

            virtual bool StartListening();
            virtual bool StopListening();

            bool virtual SendResponse(const std::string& response,
                    void* addInfo = NULL);

            void SetResponse(const std::string &response);
            std::string GetHeader(const std::string &key);

        private:
            int port;
            MHD_Daemon* daemon;
            std::map<std::string,std::string> headers;
            std::string response;

            static int callback(void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);

            static int header_iterator (void *cls, enum MHD_ValueKind kind, const char *key, const char *value);
    };

} // namespace jsonrpc

#endif // JSONRPC_TESTHTTPSERVER_H
