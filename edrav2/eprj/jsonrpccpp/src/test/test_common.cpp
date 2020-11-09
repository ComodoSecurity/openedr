/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_common.cpp
 * @date    28.09.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "checkexception.h"
#include <catch.hpp>
#include <jsonrpccpp/common/exception.h>
#include <jsonrpccpp/common/procedure.h>
#include <jsonrpccpp/common/specificationparser.h>
#include <jsonrpccpp/common/specificationwriter.h>

#define TEST_MODULE "[common]"

using namespace jsonrpc;
using namespace std;

namespace testcommon {
bool check_exception1(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_RPC_JSON_PARSE_ERROR;
}

bool check_exception2(JsonRpcException const &ex) {
  return ex.GetCode() == Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX;
}
} // namespace testcommon

using namespace testcommon;

TEST_CASE("test_procedure_parametervalidation", TEST_MODULE) {
  Procedure proc1("someprocedure", PARAMS_BY_NAME, JSON_BOOLEAN, "name",
                  JSON_STRING, "ssnr", JSON_INTEGER, NULL);

  // Expected to pass validation
  Json::Value param1;
  param1["name"] = "Peter";
  param1["ssnr"] = 4711;
  CHECK(proc1.ValidateNamedParameters(param1) == true);

  // Expected to fail validation
  Json::Value param2;
  param2.append("Peter");
  param2.append(4711);
  CHECK(proc1.ValidateNamedParameters(param2) == false);

  // Expected to fail validation
  Json::Value param3;
  param3.append(4711);
  param3.append("Peter");
  CHECK(proc1.ValidateNamedParameters(param3) == false);

  Procedure proc2("someprocedure", PARAMS_BY_NAME, JSON_BOOLEAN, "bool",
                  JSON_BOOLEAN, "object", JSON_OBJECT, "array", JSON_ARRAY,
                  "real", JSON_REAL, "int", JSON_INTEGER, NULL);
  Json::Value param4;
  Json::Value array;
  array.append(0);
  param4["bool"] = true;
  param4["object"] = param1;
  param4["array"] = array;
  param4["real"] = 0.332;
  param4["int"] = 3;

  CHECK(proc2.ValidateNamedParameters(param4) == true);

  param4["bool"] = "String";
  CHECK(proc2.ValidateNamedParameters(param4) == false);
  param4["bool"] = true;

  param4["object"] = "String";
  CHECK(proc2.ValidateNamedParameters(param4) == false);
  param4["object"] = param1;

  param4["real"] = "String";
  CHECK(proc2.ValidateNamedParameters(param4) == false);
  param4["real"] = 0.322;

  param4["array"] = "String";
  CHECK(proc2.ValidateNamedParameters(param4) == false);
  param4["array"] = array;

  param4["int"] = "String";
  CHECK(proc2.ValidateNamedParameters(param4) == false);
}

TEST_CASE("test_exception", TEST_MODULE) {
  JsonRpcException ex(Errors::ERROR_CLIENT_CONNECTOR);
  CHECK(string(ex.what()) == "Exception -32003 : Client connector error");
  CHECK(string(ex.GetMessage()) == "Client connector error");
  CHECK(ex.GetCode() == -32003);

  JsonRpcException ex2(Errors::ERROR_CLIENT_CONNECTOR, "addInfo");
  CHECK(string(ex2.what()) ==
        "Exception -32003 : Client connector error: addInfo");

  JsonRpcException ex3("addInfo");
  CHECK(string(ex3.what()) == "addInfo");
  CHECK(ex3.GetMessage() == "addInfo");
  CHECK(ex3.GetCode() == 0);

  Json::Value data;
  data.append(13);
  data.append(41);
  JsonRpcException ex4(Errors::ERROR_RPC_INTERNAL_ERROR, "internal error",
                       data);
  CHECK(ex4.GetData().size() == 2);
  CHECK(ex4.GetData()[0].asInt() == 13);
  CHECK(ex4.GetData()[1].asInt() == 41);
}

TEST_CASE("test_specificationparser_errors", TEST_MODULE) {
  CHECK_EXCEPTION_TYPE(
      SpecificationParser::GetProceduresFromFile("testspec1.json"),
      JsonRpcException, check_exception1);
  CHECK_EXCEPTION_TYPE(
      SpecificationParser::GetProceduresFromFile("testspec2.json"),
      JsonRpcException, check_exception2);
  CHECK_EXCEPTION_TYPE(
      SpecificationParser::GetProceduresFromFile("testspec3.json"),
      JsonRpcException, check_exception2);
  CHECK_EXCEPTION_TYPE(
      SpecificationParser::GetProceduresFromFile("testspec4.json"),
      JsonRpcException, check_exception2);
  CHECK_EXCEPTION_TYPE(SpecificationParser::GetProceduresFromString("{}"),
                       JsonRpcException, check_exception2);

  CHECK_EXCEPTION_TYPE(SpecificationParser::GetProceduresFromString(
                           "[{\"name\":\"proc1\"},{\"name\":\"proc1\"}]"),
                       JsonRpcException, check_exception2);
  CHECK_EXCEPTION_TYPE(
      SpecificationParser::GetProceduresFromString(
          "[{\"name\":\"proc1\", \"params\": {\"param1\": null}}]"),
      JsonRpcException, check_exception2);
  CHECK_EXCEPTION_TYPE(SpecificationParser::GetProceduresFromString(
                           "[{\"name\":\"proc1\", \"params\": 23}]"),
                       JsonRpcException, check_exception2);
}

TEST_CASE("test_specificationparser_success", TEST_MODULE) {
  std::vector<Procedure> procs =
      SpecificationParser::GetProceduresFromFile("testspec5.json");
  REQUIRE(procs.size() == 4);

  CHECK(procs[0].GetProcedureName() == "testmethod");
  CHECK(procs[0].GetReturnType() == JSON_STRING);
  CHECK(procs[0].GetProcedureType() == RPC_METHOD);
  CHECK(procs[0].GetParameterDeclarationType() == PARAMS_BY_NAME);

  CHECK(procs[2].GetProcedureName() == "testmethod2");
  CHECK(procs[2].GetReturnType() == JSON_REAL);
  CHECK(procs[2].GetProcedureType() == RPC_METHOD);
  CHECK(procs[2].GetParameterDeclarationType() == PARAMS_BY_NAME);

  CHECK(procs[1].GetProcedureName() == "testnotification");
  CHECK(procs[1].GetProcedureType() == RPC_NOTIFICATION);
  CHECK(procs[1].GetParameterDeclarationType() == PARAMS_BY_NAME);

  CHECK(procs[3].GetProcedureName() == "testnotification2");
  CHECK(procs[3].GetProcedureType() == RPC_NOTIFICATION);
  CHECK(procs[3].GetParameterDeclarationType() == PARAMS_BY_NAME);
}

TEST_CASE("test_specificationwriter", TEST_MODULE) {
  vector<Procedure> procedures;

  procedures.push_back(Procedure("testmethod1", PARAMS_BY_NAME, JSON_INTEGER,
                                 "param1", JSON_INTEGER, "param2", JSON_REAL,
                                 NULL));
  procedures.push_back(Procedure("testmethod2", PARAMS_BY_POSITION,
                                 JSON_INTEGER, "param1", JSON_OBJECT, "param2",
                                 JSON_ARRAY, NULL));

  procedures.push_back(Procedure("testnotification1", PARAMS_BY_NAME, "param1",
                                 JSON_BOOLEAN, "param2", JSON_STRING, NULL));
  procedures.push_back(Procedure("testnotification2", PARAMS_BY_POSITION,
                                 "param1", JSON_INTEGER, "param2", JSON_STRING,
                                 NULL));

  procedures.push_back(
      Procedure("testnotification3", PARAMS_BY_POSITION, NULL));

  Json::Value result = SpecificationWriter::toJsonValue(procedures);

  REQUIRE(result.isArray() == true);
  REQUIRE(result.size() == procedures.size());

  CHECK(result[0]["name"].asString() == "testmethod1");
  CHECK(result[1]["name"].asString() == "testmethod2");
  CHECK(result[2]["name"].asString() == "testnotification1");
  CHECK(result[3]["name"].asString() == "testnotification2");
  CHECK(result[4]["name"].asString() == "testnotification3");

  REQUIRE(result[0]["params"].isObject() == true);
  CHECK(result[0]["params"]["param1"].isIntegral() == true);
  CHECK(result[0]["params"]["param2"].isDouble() == true);

  REQUIRE(result[1]["params"].isArray() == true);
  CHECK(result[1]["params"][0].isObject() == true);
  CHECK(result[1]["params"][1].isArray() == true);

  REQUIRE(result[2]["params"].isObject() == true);
  CHECK(result[2]["params"]["param1"].isBool() == true);
  CHECK(result[2]["params"]["param2"].isString() == true);

  REQUIRE(result[3]["params"].isArray() == true);
  CHECK(result[3]["params"][0].isIntegral() == true);
  CHECK(result[3]["params"][1].isString() == true);

  CHECK(result[4].isMember("params") == false);

  CHECK(result[0]["returns"].isIntegral() == true);
  CHECK(result[1]["returns"].isIntegral() == true);

  CHECK(SpecificationWriter::toFile("testspec.json", procedures) == true);
  CHECK(SpecificationWriter::toFile("/a/b/c/testspec.json", procedures) ==
        false);
}
