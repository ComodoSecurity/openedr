/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_connector_redis.cpp
 * @date    13.08.2017
 * @author  Jacques Software <software@jacques.com.au>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifdef REDIS_TESTING
#include <catch.hpp>
#include <jsonrpccpp/client/connectors/redisclient.h>
#include <jsonrpccpp/server/connectors/redisserver.h>

#include "checkexception.h"
#include "mockclientconnectionhandler.h"
#include "testredisserver.h"

#include <iostream>
#include <sstream>

using namespace jsonrpc;
using namespace std;

#define TEST_PORT 6380
#define TEST_HOST "127.0.0.1"
#define TEST_QUEUE "mytest"

#define TEST_MODULE "[connector_redis]"

namespace testredisserver {
///////////////////////////////////////////////////////////////////////////
// Prepare redis fixture
//
// This fixture shall:
//   1. Start up a redis-server in the backgroun
//   2. Create a standard RedisServer connection which can be used
//   3. Create a standard RedisClient connection which can be used
//   4. Create a hiredis connection which can also be used
//
struct F {
  TestRedisServer redis_server;
  redisContext *con;
  RedisServer server;
  RedisClient client;
  MockClientConnectionHandler handler;

  F()
      : redis_server(), server(TEST_HOST, TEST_PORT, TEST_QUEUE),
        client(TEST_HOST, TEST_PORT, TEST_QUEUE) {
    con = redisConnect(TEST_HOST, TEST_PORT);
    server.SetHandler(&handler);
    server.StartListening();
    client.SetTimeout(1);
  }
  ~F() {
    redisFree(con);
    server.StopListening();
    redis_server.Stop();
  }
};

bool check_exception1(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_CLIENT_CONNECTOR;
}

bool check_exception2(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_RPC_INTERNAL_ERROR;
}
} // namespace testredisserver

using namespace testredisserver;

TEST_CASE_METHOD(F, "test_redis_success", TEST_MODULE) {
  handler.response = "exampleresponse";
  string result;
  client.SendRPCMessage("examplerequest", result);

  CHECK(handler.request == "examplerequest");
  CHECK(result == "exampleresponse");
}

TEST_CASE_METHOD(F, "test_redis_multiple", TEST_MODULE) {
  for (int i = 0; i < 50; i++) {
    stringstream request;
    stringstream response;
    request << "examplerequest" << i;
    response << "exampleresponse" << i;
    handler.response = response.str();
    string result;
    client.SendRPCMessage(request.str(), result);
    CHECK(handler.request == request.str());
    CHECK(result == response.str());
  }
}

TEST_CASE("test_redis_client_error", TEST_MODULE) {
  CHECK_EXCEPTION_TYPE(RedisClient client(TEST_HOST, TEST_PORT + 1, TEST_QUEUE),
                       JsonRpcException, check_exception1);
}

#ifdef __APPLE__
#define RAND_FIRST "2PN0bdwPY7CA8M06"
#define RAND_SECOND "zVKEkhHgZVgtV1iT"
#else
#define RAND_FIRST "fa37JncCHryDsbza"
#define RAND_SECOND "yy4cBWDxS22JjzhM"
#endif

TEST_CASE_METHOD(F, "test_redis_client_ret_queue", TEST_MODULE) {
  redisReply *reply;
  string retqueue;

  // Check correct behaviour from a known point
  srand(0);
  GetReturnQueue(con, TEST_QUEUE, retqueue);
  CHECK(retqueue == TEST_QUEUE "_" RAND_FIRST);

  GetReturnQueue(con, TEST_QUEUE, retqueue);
  CHECK(retqueue == TEST_QUEUE "_" RAND_SECOND);

  // Check behaviour when queue already exists
  reply = (redisReply *)redisCommand(con, "LPUSH %s %s",
                                     TEST_QUEUE "_" RAND_FIRST, "test");
  freeReplyObject(reply);

  srand(0);
  GetReturnQueue(con, TEST_QUEUE, retqueue);
  CHECK(retqueue == TEST_QUEUE "_" RAND_SECOND);

  // Check for a failure with the redis context
  redis_server.Stop();
  CHECK_EXCEPTION_TYPE(GetReturnQueue(con, TEST_QUEUE, retqueue),
                       JsonRpcException, check_exception1);
}

TEST_CASE_METHOD(F, "test_redis_client_set_queue", TEST_MODULE) {
  RedisServer server2(TEST_HOST, TEST_PORT, TEST_QUEUE "_other");
  MockClientConnectionHandler handler2;
  server2.SetHandler(&handler2);
  server2.StartListening();

  client.SetQueue(TEST_QUEUE "_other");

  handler2.response = "exampleresponse";
  string result;
  client.SendRPCMessage("examplerequest", result);

  CHECK(handler2.request == "examplerequest");
  CHECK(result == "exampleresponse");

  server2.StopListening();
}

TEST_CASE_METHOD(F, "test_redis_server_error", TEST_MODULE) {
  RedisServer server2(TEST_HOST, TEST_PORT + 1, TEST_QUEUE);

  CHECK(server2.StartListening() == false);
  CHECK(server.StartListening() == true);
  CHECK(server2.StopListening() == true);
  CHECK(server.StopListening() == true);
  CHECK(server.StopListening() == true);
}

TEST_CASE_METHOD(F, "test_redis_server_bad_request", TEST_MODULE) {
  string queue = TEST_QUEUE;
  string req1 = "test";

  redisReply *reply;

  reply = (redisReply *)redisCommand(con, "LPUSH %s %s", queue.c_str(),
                                     req1.c_str());
  CHECK(handler.request == "");
  freeReplyObject(reply);
  reply = (redisReply *)redisCommand(con, "DEL %s", queue.c_str());
  freeReplyObject(reply);

  reply =
      (redisReply *)redisCommand(con, "SET %s %s", queue.c_str(), req1.c_str());
  CHECK(handler.request == "");
  freeReplyObject(reply);
  reply = (redisReply *)redisCommand(con, "DEL %s", queue.c_str());
  freeReplyObject(reply);
}

TEST_CASE_METHOD(F, "test_redis_client_timeout", TEST_MODULE) {
  RedisClient client2(TEST_HOST, TEST_PORT, "invalid_queue");
  client2.SetTimeout(1);

  string result;
  CHECK_EXCEPTION_TYPE(client2.SendRPCMessage("examplerequest", result),
                       JsonRpcException, check_exception1);
}

TEST_CASE_METHOD(F, "test_redis_server_longrequest", TEST_MODULE) {
  int mb = 5;
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
  client.SetTimeout(10);
  client.SendRPCMessage(str, response);

  CHECK(handler.request == str);
  CHECK(response == handler.response);
  CHECK(response.size() == size - 1);

  free(str);
}

#endif
