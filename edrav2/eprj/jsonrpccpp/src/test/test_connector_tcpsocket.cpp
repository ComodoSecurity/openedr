/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_connector_tcpsocket.cpp
 * @date    27/07/2015
 * @author  Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifdef TCPSOCKET_TESTING
#include "mockclientconnectionhandler.h"
#include <catch.hpp>
#include <jsonrpccpp/client/connectors/tcpsocketclient.h>
#include <jsonrpccpp/server/connectors/tcpsocketserver.h>

#include "checkexception.h"

using namespace jsonrpc;
using namespace std;

#ifndef DELIMITER_CHAR
#define DELIMITER_CHAR char(0x0A)
#endif

#define TEST_MODULE "[connector_tcpsocket]"

#define IP "127.0.0.1"
#define PORT 50000

namespace testtcpsocketserver {
struct F {
  TcpSocketServer server;
  TcpSocketClient client;
  MockClientConnectionHandler handler;

  F() : server(IP, PORT), client(IP, PORT) {
    server.SetHandler(&handler);
    REQUIRE(server.StartListening());
  }
  ~F() { server.StopListening(); }
};

bool check_exception1(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_CLIENT_CONNECTOR;
}
} // namespace testtcpsocketserver
using namespace testtcpsocketserver;

TEST_CASE_METHOD(F, "test_tcpsocket_success", TEST_MODULE) {
  handler.response = "exampleresponse";
  handler.timeout = 100;
  string result;
  string request = "examplerequest";
  string expectedResult = "exampleresponse";

  client.SendRPCMessage(request, result);

  CHECK(handler.request == request);
  CHECK(result == expectedResult);
}

TEST_CASE("test_tcpsocket_server_multiplestart", TEST_MODULE) {
  TcpSocketServer server(IP, PORT);
  CHECK(server.StartListening() == true);
  CHECK(server.StartListening() == false);

  TcpSocketServer server2(IP, PORT);
  CHECK(server2.StartListening() == false);
  CHECK(server2.StopListening() == false);

  CHECK(server.StopListening() == true);
}

TEST_CASE("test_tcpsocket_client_invalid", TEST_MODULE) {
  TcpSocketClient client("127.0.0.1", 40000); // If this test fails, check that
                                              // port 40000 is really unused. If
                                              // it is used, change this port
                                              // value to an unused port,
                                              // recompile tests and run tests
                                              // again.
  string result;
  CHECK_EXCEPTION_TYPE(client.SendRPCMessage("foobar", result),
                       JsonRpcException, check_exception1);
}

#endif
