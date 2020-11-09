/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_connector_unixdomainsocket.cpp
 * @date    26/10/2016
 * @author  Jean-Daniel Michaud <jean.daniel.michaud@gmail.com>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifdef FILEDESCRIPTOR_TESTING
#include "mockclientconnectionhandler.h"
#include <catch.hpp>
#include <jsonrpccpp/client/connectors/filedescriptorclient.h>
#include <jsonrpccpp/common/sharedconstants.h>
#include <jsonrpccpp/server/connectors/filedescriptorserver.h>
#include <string.h>
#include <unistd.h>

#include "checkexception.h"

using namespace jsonrpc;
using namespace std;

#define TEST_MODULE "[connector_filedescriptor]"

namespace testfiledescriptorserver {
struct F {
  FileDescriptorServer *server;
  FileDescriptorClient *client;
  MockClientConnectionHandler handler;

  F() {
    pipe(c2sfd);
    pipe(s2cfd);
    server = new FileDescriptorServer(c2sfd[0], s2cfd[1]);
    client = new FileDescriptorClient(s2cfd[0], c2sfd[1]);
    server->SetHandler(&handler);
    REQUIRE(server->StartListening());
  }
  ~F() {
    server->StopListening();
    delete server;
    delete client;
    close(c2sfd[0]);
    close(c2sfd[1]);
    close(s2cfd[0]);
    close(s2cfd[1]);
  }

  int c2sfd[2]; // Client to server fd
  int s2cfd[2]; // Server to client fd
};

bool check_exception1(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_CLIENT_CONNECTOR;
}
} // namespace testfiledescriptorserver
using namespace testfiledescriptorserver;

TEST_CASE_METHOD(F, "test_filedescriptor_success", TEST_MODULE) {
  handler.response = "exampleresponse";
  handler.timeout = 1;
  string result;
  string request = "examplerequest";
  string expectedResult = "exampleresponse";

  client->SendRPCMessage(request, result);

  CHECK(handler.request == request);
  CHECK(result == expectedResult);
}

TEST_CASE_METHOD(F, "test_filedescriptor_longpost", TEST_MODULE) {
  int mb = 2;
  unsigned long size = mb * 1024 * 1024;
  char *str = (char *)malloc(size * sizeof(char));
  if (str == NULL) {
    FAIL("Could not allocate enough memory for test");
  }
  for (unsigned long i = 0; i < size; i++) {
    str[i] = (char)('a' + (i % 26));
  }
  str[size - 1] = '\0';
  handler.response = str;
  string response;
  client->SendRPCMessage(str, response);

  CHECK(handler.request == str);
  CHECK(response.size() == size - 1);
  CHECK(response == handler.response);

  free(str);
}

TEST_CASE_METHOD(F, "test_filedescriptor_success_with_delimiter", TEST_MODULE) {
  handler.response = "exampleresponse";
  handler.response.push_back(DEFAULT_DELIMITER_CHAR);
  handler.timeout = 100;
  string result;
  string request = "examplerequest";
  request.push_back(DEFAULT_DELIMITER_CHAR);
  string expectedResult = "exampleresponse";
  expectedResult.push_back(DEFAULT_DELIMITER_CHAR);

  client->SendRPCMessage(request, result);

  CHECK(handler.request == request);
  CHECK(result == expectedResult);
}

TEST_CASE("test_filedescriptor_server_multiplestart", TEST_MODULE) {
  int fds[2];
  pipe(fds);
  FileDescriptorServer server(fds[0], fds[1]);
  CHECK(server.StartListening() == true);
  CHECK(server.StartListening() == false);

  CHECK(server.StopListening() == true);
  CHECK(server.StopListening() == false);

  // server.Wait();

  close(fds[0]);
  close(fds[1]);
}

TEST_CASE("test_filedescriptor_server_input_fd_invalid_while_listening",
          TEST_MODULE) {
  int c2sfd[2];
  pipe(c2sfd);
  int s2cfd[2];
  pipe(s2cfd);

  FileDescriptorServer server(c2sfd[0], s2cfd[1]);
  // Close the writing side of the client to server pipe, as if the client
  // crashed.
  CHECK(server.StartListening() == true);
  close(c2sfd[0]);
  // server.Wait();

  close(c2sfd[1]);
  close(s2cfd[1]);
  close(s2cfd[0]);
}

TEST_CASE("test_filedescriptor_server_input_fd_invalid", TEST_MODULE) {
  int c2sfd[2];
  pipe(c2sfd);
  int s2cfd[2];
  pipe(s2cfd);

  FileDescriptorServer server(c2sfd[0], s2cfd[1]);
  // Close the writing side of the client to server pipe, as if the client
  // crashed.
  close(c2sfd[0]);
  CHECK(server.StartListening() == false);
  close(c2sfd[1]);
  close(s2cfd[1]);
  close(s2cfd[0]);
}

TEST_CASE("test_filedescriptor_server_output_fd_invalid", TEST_MODULE) {
  int c2sfd[2];
  pipe(c2sfd);
  int s2cfd[2];
  pipe(s2cfd);

  FileDescriptorServer server(c2sfd[0], s2cfd[1]);
  // Close the writing side of the server to client pipe, as if the client
  // crashed.
  close(s2cfd[1]);
  CHECK(server.StartListening() == false);
  close(c2sfd[0]);
  close(c2sfd[1]);
  close(s2cfd[0]);
}

TEST_CASE("test_filedescriptor_client_output_fd_invalid", TEST_MODULE) {
  FileDescriptorClient client(2, 6);
  string result;
  CHECK_EXCEPTION_TYPE(client.SendRPCMessage("foobar", result),
                       JsonRpcException, check_exception1);
}

TEST_CASE("test_filedescriptor_client_input_fd_invalid", TEST_MODULE) {
  int c2sfd[2];
  pipe(c2sfd);
  int s2cfd[2];
  pipe(s2cfd);

  FileDescriptorClient client(s2cfd[0], c2sfd[1]);
  // Close the reading side of the server to client pipe, as if the server
  // crashed.
  close(s2cfd[0]);
  string result;
  CHECK_EXCEPTION_TYPE(client.SendRPCMessage("foobar", result),
                       JsonRpcException, check_exception1);
  close(c2sfd[1]);
  close(s2cfd[1]);
  close(c2sfd[0]);
}

#endif // FILEDESCRIPTOR_TESTING
