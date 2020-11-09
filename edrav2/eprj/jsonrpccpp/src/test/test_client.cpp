/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_client.cpp
 * @date    28.09.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "checkexception.h"
#include "mockclientconnector.h"
#include <catch.hpp>
#include <jsonrpccpp/client.h>

#define TEST_MODULE "[client]"

using namespace jsonrpc;
using namespace std;

namespace testclient {
bool check_exception1(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_RPC_JSON_PARSE_ERROR;
}

bool check_exception2(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_CLIENT_INVALID_RESPONSE;
}

bool check_exception3(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_RPC_INVALID_REQUEST &&
         ex.GetData().size() == 2;
}

struct F {
  MockClientConnector c;
  Client client;
  Json::Value params;

  F() : client(c, JSONRPC_CLIENT_V2) {}
};

struct F1 {
  MockClientConnector c;
  Client client;
  Json::Value params;

  F1() : client(c, JSONRPC_CLIENT_V1) {}
};
} // namespace testclient
using namespace testclient;

TEST_CASE_METHOD(F, "test_client_id", TEST_MODULE) {
  params.append(33);
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"id\": \"1\", \"result\": 23}");
  Json::Value response = client.CallMethod("abcd", params);
  CHECK(response.asInt() == 23);

  c.SetResponse("{\"jsonrpc\":\"2.0\", \"id\": 1, \"result\": 24}");
  response = client.CallMethod("abcd", params);
  CHECK(response.asInt() == 24);
}

TEST_CASE_METHOD(F, "test_client_v2_method_success", TEST_MODULE) {
  params.append(23);
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"id\": 1, \"result\": 23}");
  Json::Value response = client.CallMethod("abcd", params);
  Json::Value v = c.GetJsonRequest();

  CHECK(v["method"].asString() == "abcd");
  CHECK(v["params"][0].asInt() == 23);
  CHECK(v["jsonrpc"].asString() == "2.0");
  CHECK(v["id"].asInt() == 1);
}

TEST_CASE_METHOD(F, "test_client_v2_notification_success", TEST_MODULE) {
  params.append(23);
  client.CallNotification("abcd", params);
  Json::Value v = c.GetJsonRequest();

  CHECK(v["method"].asString() == "abcd");
  CHECK(v["params"][0].asInt() == 23);
  CHECK(v["jsonrpc"].asString() == "2.0");
  CHECK(v.isMember("id") == false);
}

TEST_CASE_METHOD(F, "test_client_v2_errorresponse", TEST_MODULE) {
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"error\": {\"code\": -32600, "
                "\"message\": \"Invalid Request\", \"data\": [1,2]}, \"id\": "
                "null}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception3);
}

TEST_CASE_METHOD(F, "test_client_v2_invalidjson", TEST_MODULE) {
  c.SetResponse("{\"method\":234");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception1);
}

TEST_CASE_METHOD(F, "test_client_v2_invalidresponse", TEST_MODULE) {
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"id\": 1, \"resulto\": 23}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"id2\": 1, \"result\": 23}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"jsonrpc\":\"1.0\", \"id\": 1, \"result\": 23}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"id\": 1, \"result\": 23}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse(
      "{\"jsonrpc\":\"2.0\", \"id\": 1, \"result\": 23, \"error\": {}}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"id\": 1, \"error\": {}}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"jsonrpc\":\"2.0\", \"result\": 23}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("[]");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("23");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
}

TEST_CASE_METHOD(F, "test_client_v2_batchcall_success", TEST_MODULE) {
  BatchCall bc;
  CHECK(bc.addCall("abc", Json::nullValue, false) == 1);
  CHECK(bc.addCall("def", Json::nullValue, true) == -1);
  CHECK(bc.addCall("abc", Json::nullValue, false) == 2);

  c.SetResponse("[{\"jsonrpc\":\"2.0\", \"id\": 1, \"result\": "
                "23},{\"jsonrpc\":\"2.0\", \"id\": 2, \"result\": 24}]");

  BatchResponse response = client.CallProcedures(bc);

  CHECK(response.hasErrors() == false);
  CHECK(response.getResult(1).asInt() == 23);
  CHECK(response.getResult(2).asInt() == 24);
  CHECK(response.getResult(3).isNull() == true);

  Json::Value request = c.GetJsonRequest();
  CHECK(request.size() == 3);
  CHECK(request[0]["method"].asString() == "abc");
  CHECK(request[0]["id"].asInt() == 1);
  CHECK(request[1]["method"].asString() == "def");
  CHECK(request[1]["id"].isNull() == true);
  CHECK(request[2]["id"].asInt() == 2);

  bc.toString(false);
}

TEST_CASE_METHOD(F, "test_client_v2_batchcall_error", TEST_MODULE) {
  BatchCall bc;
  CHECK(bc.addCall("abc", Json::nullValue, false) == 1);
  CHECK(bc.addCall("def", Json::nullValue, false) == 2);
  CHECK(bc.addCall("abc", Json::nullValue, false) == 3);

  c.SetResponse("[{\"jsonrpc\":\"2.0\", \"id\": 1, \"result\": "
                "23},{\"jsonrpc\":\"2.0\", \"id\": 2, \"error\": {\"code\": "
                "-32001, \"message\": \"error1\"}},{\"jsonrpc\":\"2.0\", "
                "\"id\": null, \"error\": {\"code\": -32002, \"message\": "
                "\"error2\"}}]");

  BatchResponse response = client.CallProcedures(bc);

  CHECK(response.hasErrors() == true);
  CHECK(response.getResult(1).asInt() == 23);
  CHECK(response.getResult(2).isNull() == true);
  CHECK(response.getResult(3).isNull() == true);
  CHECK(response.getErrorMessage(2) == "error1");
  CHECK(response.getErrorMessage(3) == "");

  c.SetResponse("{}");
  CHECK_EXCEPTION_TYPE(client.CallProcedures(bc), JsonRpcException,
                       check_exception2);

  c.SetResponse("[1,2,3]");
  CHECK_EXCEPTION_TYPE(client.CallProcedures(bc), JsonRpcException,
                       check_exception2);

  c.SetResponse("[[],[],[]]");
  CHECK_EXCEPTION_TYPE(client.CallProcedures(bc), JsonRpcException,
                       check_exception2);
}

TEST_CASE_METHOD(F1, "test_client_v1_method_success", TEST_MODULE) {
  params.append(23);
  c.SetResponse("{\"id\": 1, \"result\": 23, \"error\": null}");

  Json::Value response = client.CallMethod("abcd", params);
  Json::Value v = c.GetJsonRequest();

  CHECK(v["method"].asString() == "abcd");
  CHECK(v["params"][0].asInt() == 23);
  CHECK(v.isMember("jsonrpc") == false);
  CHECK(v["id"].asInt() == 1);

  CHECK(response.asInt() == 23);
}

TEST_CASE_METHOD(F1, "test_client_v1_notification_success", TEST_MODULE) {
  params.append(23);

  client.CallNotification("abcd", params);

  Json::Value v = c.GetJsonRequest();

  CHECK(v["method"].asString() == "abcd");
  CHECK(v["params"][0].asInt() == 23);
  CHECK(v.isMember("id") == true);
  CHECK(v["id"] == Json::nullValue);
}

TEST_CASE_METHOD(F1, "test_client_v1_errorresponse", TEST_MODULE) {
  c.SetResponse("{\"result\": null, \"error\": {\"code\": -32600, \"message\": "
                "\"Invalid Request\", \"data\": [1,2]}, \"id\": null}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception3);

  c.SetResponse("{\"result\": null, \"error\": {\"code\": -32600, \"message\": "
                "\"Invalid Request\", \"data\": [1,2]}, \"id\": null}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception3);
}

TEST_CASE_METHOD(F1, "test_client_v1_invalidresponse", TEST_MODULE) {
  c.SetResponse("{\"id\": 1, \"resulto\": 23, \"error\": null}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"id\": 1, \"result\": 23}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"id\": 1, \"error\": null}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"id\": 1}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"id\": 1, \"result\": 23, \"error\": {}}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{\"id\": 1, \"result\": null, \"error\": {}}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("{}");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("[]");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
  c.SetResponse("23");
  CHECK_EXCEPTION_TYPE(client.CallMethod("abcd", Json::nullValue),
                       JsonRpcException, check_exception2);
}
