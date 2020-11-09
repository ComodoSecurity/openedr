/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    testhttpserver.cpp
 * @date    11/16/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "testhttpserver.h"

using namespace jsonrpc;

TestHttpServer::TestHttpServer(int port) : port(port) {}

bool TestHttpServer::StartListening() {
  this->daemon =
      MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, this->port, NULL, NULL,
                       TestHttpServer::callback, this, MHD_OPTION_END);
  return (this->daemon != NULL);
}

bool TestHttpServer::StopListening() {
  MHD_stop_daemon(this->daemon);
  return true;
}

bool TestHttpServer::SendResponse(const std::string &response, void *addInfo) {
  (void)response;
  (void)addInfo;
  return true;
}

void TestHttpServer::SetResponse(const std::string &response) {
  this->response = response;
}

std::string TestHttpServer::GetHeader(const std::string &key) {
  if (this->headers.find(key) != this->headers.end())
    return this->headers[key];
  return "";
}

int TestHttpServer::callback(void *cls, MHD_Connection *connection,
                             const char *url, const char *method,
                             const char *version, const char *upload_data,
                             size_t *upload_data_size, void **con_cls) {
  (void)upload_data;
  (void)upload_data_size;
  (void)url;
  (void)method;
  (void)version;
  TestHttpServer *_this = static_cast<TestHttpServer *>(cls);
  if (*con_cls == NULL) {
    *con_cls = cls;
    _this->headers.clear();
  } else {
    MHD_get_connection_values(connection, MHD_HEADER_KIND, header_iterator,
                              cls);
    struct MHD_Response *result = MHD_create_response_from_buffer(
        _this->response.size(), (void *)_this->response.c_str(),
        MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(result, "Content-Type", "application/json");
    MHD_add_response_header(result, "Access-Control-Allow-Origin", "*");
    MHD_queue_response(connection, MHD_HTTP_OK, result);
    MHD_destroy_response(result);
  }

  return MHD_YES;
}

int TestHttpServer::header_iterator(void *cls, MHD_ValueKind kind,
                                    const char *key, const char *value) {
  (void)kind;
  TestHttpServer *_this = static_cast<TestHttpServer *>(cls);
  _this->headers[key] = value;
  return MHD_YES;
}
