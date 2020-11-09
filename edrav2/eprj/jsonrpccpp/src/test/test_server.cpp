/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_server.cpp
 * @date    28.09.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "mockserverconnector.h"
#include "testserver.h"
#include <catch.hpp>

#define TEST_MODULE "[server]"

using namespace jsonrpc;
using namespace std;

namespace testserver {
struct F {
  MockServerConnector c;
  TestServer server;

  F() : server(c) {}
};

struct F1 {
  MockServerConnector c;
  TestServer server;

  F1() : server(c, JSONRPC_SERVER_V1) {}
};
} // namespace testserver
using namespace testserver;

TEST_CASE_METHOD(F, "test_server_v2_method_success", TEST_MODULE) {
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["result"].asString() == "Hello: Peter!");
  CHECK(c.GetJsonResponse()["id"].asInt() == 1);
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"add\",\"params\":{\"value1\":5,\"value2\":7}}");
  CHECK(c.GetJsonResponse()["result"].asInt() == 12);
  CHECK(c.GetJsonResponse()["id"].asInt() == 1);
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);

  c.SetRequest(
      "{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": \"sub\",\"params\":[5,7]}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asInt() == 1);
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": null, \"method\": "
               "\"sub\",\"params\":[5,7]}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].isNull() == true);
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": \"1\", \"method\": "
               "\"sub\",\"params\":[5,7]}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asString() == "1");
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 4294967295, \"method\": "
               "\"sub\",\"params\":[5,7]}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asLargestUInt() == (unsigned long)4294967295);
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);
}

TEST_CASE_METHOD(F, "test_server_v2_notification_success", TEST_MODULE) {
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"method\": "
               "\"initCounter\",\"params\":{\"value\": 33}}");
  CHECK(server.getCnt() == 33);
  CHECK(c.GetResponse() == "");

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"method\": "
               "\"incrementCounter\",\"params\":{\"value\": 33}}");
  CHECK(server.getCnt() == 66);
  CHECK(c.GetResponse() == "");
}

TEST_CASE_METHOD(F, "test_server_v2_invalidjson", TEST_MODULE) {
  c.SetRequest("{\"jsonrpc\":\"2.");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32700);
  CHECK(c.GetJsonResponse().isMember("result") == false);
}

TEST_CASE_METHOD(F, "test_server_v2_invalidrequest", TEST_MODULE) {
  // wrong rpc version
  c.SetRequest("{\"jsonrpc\":\"1.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // wrong rpc version type
  c.SetRequest("{\"jsonrpc\":2.0, \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // no method name
  c.SetRequest(
      "{\"jsonrpc\":\"2.0\", \"id\": 1,\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // wrong method name type
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": {}, "
               "\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // invalid param structure
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":1}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  //
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 3.2, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "3,\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("{}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("[]");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("23");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);
}

TEST_CASE_METHOD(F, "test_server_v2_method_error", TEST_MODULE) {
  // invalid methodname
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello2\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32601);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // call notification as procedure
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"initCounter\",\"params\":{\"value\":3}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32605);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // call procedure as notification
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32604);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"method\": "
               "\"sub\",\"params\":{\"value1\":3, \"value\": 4}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32604);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"method\": \"add\",\"params\":[3,4]}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32604);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // userspace exception
  c.SetRequest(
      "{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": \"exceptionMethod\"}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32099);
  CHECK(c.GetJsonResponse()["error"]["message"] == "User exception");
  CHECK(c.GetJsonResponse()["error"]["data"][0] == 33);
  CHECK(c.GetJsonResponse().isMember("result") == false);
}

TEST_CASE_METHOD(F, "test_server_v2_params_error", TEST_MODULE) {
  // invalid param type
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":23}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32602);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // invalid param name
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name2\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32602);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // invalid parameter passing mode (array instead of object)
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":[\"Peter\"]}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32602);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  // missing parameter field
  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": \"sayHello\"}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32602);
  CHECK(c.GetJsonResponse().isMember("result") == false);
}

TEST_CASE_METHOD(F, "test_server_v2_batchcall_success", TEST_MODULE) {
  // Simple Batchcall with only methods
  c.SetRequest("[{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}},{\"jsonrpc\":\"2."
               "0\", \"id\": 2, \"method\": "
               "\"add\",\"params\":{\"value1\":23,\"value2\": 33}}]");

  CHECK(c.GetJsonResponse().size() == 2);
  CHECK(c.GetJsonResponse()[0]["result"].asString() == "Hello: Peter!");
  CHECK(c.GetJsonResponse()[0]["id"].asInt() == 1);
  CHECK(c.GetJsonResponse()[1]["result"].asInt() == 56);
  CHECK(c.GetJsonResponse()[1]["id"].asInt() == 2);

  // Batchcall containing methods and notifications
  c.SetRequest("[{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}},{\"jsonrpc\":\"2."
               "0\", \"method\": \"initCounter\",\"params\":{\"value\":23}}]");

  CHECK(c.GetJsonResponse().size() == 1);
  CHECK(c.GetJsonResponse()[0]["result"].asString() == "Hello: Peter!");
  CHECK(c.GetJsonResponse()[0]["id"].asInt() == 1);
  CHECK(server.getCnt() == 23);

  // Batchcall containing only notifications
  c.SetRequest("[{\"jsonrpc\":\"2.0\", \"method\": "
               "\"initCounter\",\"params\":{\"value\":23}},{\"jsonrpc\":\"2."
               "0\", \"method\": \"initCounter\",\"params\":{\"value\":23}}]");

  CHECK(c.GetResponse() == "");
}

TEST_CASE_METHOD(F, "test_server_v2_batchcall_error", TEST_MODULE) {
  // success and error responses
  c.SetRequest("[{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}},{},{\"jsonrpc\":"
               "\"2.0\", \"id\": 3, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter3\"}}]");
  CHECK(c.GetJsonResponse().size() == 3);
  CHECK(c.GetJsonResponse()[0]["result"].asString() == "Hello: Peter!");
  CHECK(c.GetJsonResponse()[0]["id"].asInt() == 1);
  CHECK(c.GetJsonResponse()[1]["error"]["code"].asInt() == -32600);
  CHECK(c.GetJsonResponse()[1]["id"].isNull() == true);
  CHECK(c.GetJsonResponse()[2]["result"].asString() == "Hello: Peter3!");
  CHECK(c.GetJsonResponse()[2]["id"].asInt() == 3);

  // only invalid requests
  c.SetRequest("[1,2,3]");
  CHECK(c.GetJsonResponse().size() == 3);
  CHECK(c.GetJsonResponse()[0]["error"]["code"].asInt() == -32600);
  CHECK(c.GetJsonResponse()[1]["error"]["code"].asInt() == -32600);
  CHECK(c.GetJsonResponse()[2]["error"]["code"].asInt() == -32600);
}

TEST_CASE_METHOD(F1, "test_server_v1_method_success", TEST_MODULE) {
  c.SetRequest("{\"id\": 1, \"method\": \"sub\",\"params\":[5,7]}}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asInt() == 1);
  CHECK(c.GetJsonResponse().isMember("jsonrpc") == false);
  CHECK(c.GetJsonResponse().isMember("error") == true);
  CHECK(c.GetJsonRequest()["error"] == Json::nullValue);

  c.SetRequest("{\"id\": \"1\", \"method\": \"sub\",\"params\":[5,7]}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asString() == "1");
  CHECK(c.GetJsonResponse().isMember("jsonrpc") == false);
  CHECK(c.GetJsonResponse().isMember("error") == true);
  CHECK(c.GetJsonRequest()["error"] == Json::nullValue);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": \"1\", \"method\": "
               "\"sub\",\"params\":[5,7]}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asString() == "1");
  CHECK(c.GetJsonResponse().isMember("jsonrpc") == false);
  CHECK(c.GetJsonResponse().isMember("error") == true);
  CHECK(c.GetJsonRequest()["error"] == Json::nullValue);
}

TEST_CASE_METHOD(F1, "test_server_v1_notification_success", TEST_MODULE) {
  c.SetRequest("{\"id\": null, \"method\": \"initZero\", \"params\": null}");
  CHECK(server.getCnt() == 0);
  CHECK(c.GetResponse() == "");
}

TEST_CASE_METHOD(F1, "test_server_v1_method_invalid_request", TEST_MODULE) {
  c.SetRequest("{\"method\": \"sub\", \"params\": []}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("{\"id\": 1, \"method\": \"sub\", \"params\": {\"foo\": true}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("{\"id\": 1, \"method\": \"sub\", \"params\": true}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("{\"id\": 1, \"params\": []}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("{}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("[]");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("23");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);
}

TEST_CASE_METHOD(F1, "test_server_v1_method_error", TEST_MODULE) {
  c.SetRequest("{\"id\": 1, \"method\": \"sub\", \"params\": [33]}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32602);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("{\"id\": 1, \"method\": \"sub\", \"params\": [33, \"foo\"]}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32602);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  c.SetRequest("{\"id\": 1, \"method\": \"sub\"}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);

  // userspace exception
  c.SetRequest("{\"id\": 1, \"method\": \"exceptionMethod\",\"params\":null}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32099);
  CHECK(c.GetJsonResponse()["error"]["message"] == "User exception");
  CHECK(c.GetJsonResponse()["result"] == Json::nullValue);
}

TEST_CASE("test_server_hybrid", TEST_MODULE) {
  MockServerConnector c;
  TestServer server(c, JSONRPC_SERVER_V1V2);

  c.SetRequest("{\"id\": 1, \"method\": \"sub\",\"params\":[5,7]}}");
  CHECK(c.GetJsonResponse()["result"].asInt() == -2);
  CHECK(c.GetJsonResponse()["id"].asInt() == 1);
  CHECK(c.GetJsonResponse().isMember("jsonrpc") == false);
  CHECK(c.GetJsonResponse().isMember("error") == true);
  CHECK(c.GetJsonRequest()["error"] == Json::nullValue);

  c.SetRequest("{\"id\": null, \"method\": \"initZero\", \"params\": null}");
  CHECK(server.getCnt() == 0);
  CHECK(c.GetResponse() == "");

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"id\": 1, \"method\": "
               "\"sayHello\",\"params\":{\"name\":\"Peter\"}}");
  CHECK(c.GetJsonResponse()["result"].asString() == "Hello: Peter!");
  CHECK(c.GetJsonResponse()["id"].asInt() == 1);
  CHECK(c.GetJsonResponse()["jsonrpc"].asString() == "2.0");
  CHECK(c.GetJsonResponse().isMember("error") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"method\": "
               "\"initCounter\",\"params\":{\"value\": 33}}");
  CHECK(server.getCnt() == 33);
  CHECK(c.GetResponse() == "");

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"params\":{\"value\": 33}}");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32600);
  CHECK(c.GetJsonResponse().isMember("result") == false);

  c.SetRequest("{\"jsonrpc\":\"2.0\", \"params\":{\"value\": 33");
  CHECK(c.GetJsonResponse()["error"]["code"] == -32700);
  CHECK(c.GetJsonResponse().isMember("result") == false);
}

TEST_CASE("test_server_abstractserver", TEST_MODULE) {
  MockServerConnector c;
  TestServer server(c, JSONRPC_SERVER_V1V2);

  CHECK(server.bindAndAddNotification(Procedure("testMethod", PARAMS_BY_NAME,
                                                JSON_STRING, "name",
                                                JSON_STRING, NULL),
                                      &TestServer::initCounter) == false);
  CHECK(server.bindAndAddMethod(Procedure("initCounter", PARAMS_BY_NAME,
                                          "value", JSON_INTEGER, NULL),
                                &TestServer::sayHello) == false);

  CHECK(
      server.bindAndAddMethod(Procedure("testMethod", PARAMS_BY_NAME,
                                        JSON_STRING, "name", JSON_STRING, NULL),
                              &TestServer::sayHello) == true);
  CHECK(
      server.bindAndAddMethod(Procedure("testMethod", PARAMS_BY_NAME,
                                        JSON_STRING, "name", JSON_STRING, NULL),
                              &TestServer::sayHello) == false);

  CHECK(server.bindAndAddNotification(Procedure("testNotification",
                                                PARAMS_BY_NAME, "value",
                                                JSON_INTEGER, NULL),
                                      &TestServer::initCounter) == true);
  CHECK(server.bindAndAddNotification(Procedure("testNotification",
                                                PARAMS_BY_NAME, "value",
                                                JSON_INTEGER, NULL),
                                      &TestServer::initCounter) == false);

  CHECK(server.StartListening() == true);
  CHECK(server.StopListening() == true);
}
