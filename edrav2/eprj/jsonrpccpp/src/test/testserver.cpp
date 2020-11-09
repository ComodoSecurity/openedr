/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    testserver.cpp
 * @date    08.03.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "testserver.h"
#include <iostream>

using namespace std;
using namespace jsonrpc;

TestServer::TestServer(AbstractServerConnector &connector, serverVersion_t type)
    : AbstractServer<TestServer>(connector, type), cnt(-1) {
  this->bindAndAddMethod(Procedure("sayHello", PARAMS_BY_NAME, JSON_STRING,
                                   "name", JSON_STRING, NULL),
                         &TestServer::sayHello);
  this->bindAndAddMethod(
      Procedure("getCounterValue", PARAMS_BY_NAME, JSON_INTEGER, NULL),
      &TestServer::getCounterValue);
  this->bindAndAddMethod(Procedure("add", PARAMS_BY_NAME, JSON_INTEGER,
                                   "value1", JSON_INTEGER, "value2",
                                   JSON_INTEGER, NULL),
                         &TestServer::add);
  this->bindAndAddMethod(Procedure("sub", PARAMS_BY_POSITION, JSON_INTEGER,
                                   "value1", JSON_INTEGER, "value2",
                                   JSON_INTEGER, NULL),
                         &TestServer::sub);
  this->bindAndAddMethod(
      Procedure("exceptionMethod", PARAMS_BY_POSITION, JSON_INTEGER, NULL),
      &TestServer::exceptionMethod);

  this->bindAndAddNotification(
      Procedure("initCounter", PARAMS_BY_NAME, "value", JSON_INTEGER, NULL),
      &TestServer::initCounter);
  this->bindAndAddNotification(Procedure("incrementCounter", PARAMS_BY_NAME,
                                         "value", JSON_INTEGER, NULL),
                               &TestServer::incrementCounter);
  this->bindAndAddNotification(Procedure("initZero", PARAMS_BY_POSITION, NULL),
                               &TestServer::initZero);
}

void TestServer::sayHello(const Json::Value &request, Json::Value &response) {
  response = "Hello: " + request["name"].asString() + "!";
}

void TestServer::getCounterValue(const Json::Value &request,
                                 Json::Value &response) {
  (void)request;
  response = cnt;
}

void TestServer::add(const Json::Value &request, Json::Value &response) {
  response = request["value1"].asInt() + request["value2"].asInt();
}

void TestServer::sub(const Json::Value &request, Json::Value &response) {
  response = request[0].asInt() - request[1].asInt();
}

void TestServer::exceptionMethod(const Json::Value &request,
                                 Json::Value &response) {
  (void)request;
  (void)response;
  Json::Value data;
  data.append(33);
  throw JsonRpcException(-32099, "User exception", data);
}

void TestServer::initCounter(const Json::Value &request) {
  cnt = request["value"].asInt();
}

void TestServer::incrementCounter(const Json::Value &request) {
  cnt += request["value"].asInt();
}

void TestServer::initZero(const Json::Value &request) {
  (void)request;
  cnt = 0;
}

int TestServer::getCnt() { return cnt; }

bool TestServer::bindAndAddMethod(
    const Procedure &proc,
    AbstractServer<TestServer>::methodPointer_t pointer) {
  return AbstractServer<TestServer>::bindAndAddMethod(proc, pointer);
}

bool TestServer::bindAndAddNotification(
    const Procedure &proc,
    AbstractServer<TestServer>::notificationPointer_t pointer) {
  return AbstractServer<TestServer>::bindAndAddNotification(proc, pointer);
}
