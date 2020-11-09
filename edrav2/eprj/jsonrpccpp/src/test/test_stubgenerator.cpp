/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    test_connector_http.cpp
 * @date    28.09.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifdef STUBGEN_TESTING
#include <catch.hpp>

#include <jsonrpccpp/common/specificationparser.h>
#include <stubgenerator/client/cppclientstubgenerator.h>
#include <stubgenerator/client/jsclientstubgenerator.h>
#include <stubgenerator/client/pyclientstubgenerator.h>
#include <stubgenerator/helper/cpphelper.h>
#include <stubgenerator/server/cppserverstubgenerator.h>
#include <stubgenerator/stubgeneratorfactory.h>

#include <sstream>

using namespace jsonrpc;
using namespace std;

namespace teststubgen {
struct F {
  FILE *stdout;
  FILE *stderr;
  vector<StubGenerator *> stubgens;
  vector<Procedure> procedures;
  F() {
    stdout = fopen("stdout.txt", "w");
    stderr = fopen("stderr.txt", "w");
  }

  ~F() {
    fclose(stdout);
    fclose(stderr);
  }
};
} // namespace teststubgen

using namespace teststubgen;

#define TEST_MODULE "[stubgenerator]"

TEST_CASE("test stubgen cppclient", TEST_MODULE) {
  stringstream stream;
  vector<Procedure> procedures =
      SpecificationParser::GetProceduresFromFile("testspec6.json");
  CPPClientStubGenerator stubgen("ns1::ns2::TestStubClient", procedures,
                                 stream);
  stubgen.generateStub();
  string result = stream.str();

  CHECK(result.find("#ifndef JSONRPC_CPP_STUB_NS1_NS2_TESTSTUBCLIENT_H_") !=
        string::npos);
  CHECK(result.find("#define JSONRPC_CPP_STUB_NS1_NS2_TESTSTUBCLIENT_H_") !=
        string::npos);
  CHECK(result.find("namespace ns1") != string::npos);
  CHECK(result.find("namespace ns2") != string::npos);
  CHECK(result.find("class TestStubClient : public jsonrpc::Client") !=
        string::npos);
  CHECK(result.find("std::string test_method(const std::string& name) ") !=
        string::npos);
  CHECK(result.find("void test_notification(const std::string& name) ") !=
        string::npos);
  CHECK(result.find("double test_method2(const Json::Value& object, const "
                    "Json::Value& values) ") != string::npos);
  CHECK(result.find("void test_notification2(const Json::Value& object, const "
                    "Json::Value& values) ") != string::npos);
  CHECK(result.find("#endif //JSONRPC_CPP_STUB_NS1_NS2_TESTSTUBCLIENT_H_") !=
        string::npos);

  CHECK(CPPHelper::class2Filename("ns1::ns2::TestClass") == "testclass.h");
}

TEST_CASE("test stubgen cppserver", TEST_MODULE) {
  stringstream stream;
  vector<Procedure> procedures =
      SpecificationParser::GetProceduresFromFile("testspec6.json");
  CPPServerStubGenerator stubgen("ns1::ns2::TestStubServer", procedures,
                                 stream);
  stubgen.generateStub();
  string result = stream.str();

  CHECK(result.find("#ifndef JSONRPC_CPP_STUB_NS1_NS2_TESTSTUBSERVER_H_") !=
        string::npos);
  CHECK(result.find("#define JSONRPC_CPP_STUB_NS1_NS2_TESTSTUBSERVER_H_") !=
        string::npos);
  CHECK(result.find("namespace ns1") != string::npos);
  CHECK(result.find("namespace ns2") != string::npos);
  CHECK(result.find("class TestStubServer : public "
                    "jsonrpc::AbstractServer<TestStubServer>") != string::npos);
  CHECK(result.find(
            "virtual std::string test_method(const std::string& name) = 0;") !=
        string::npos);
  CHECK(result.find(
            "virtual void test_notification(const std::string& name) = 0;") !=
        string::npos);
  CHECK(result.find("virtual double test_method2(const Json::Value& object, "
                    "const Json::Value& values) = 0;") != string::npos);
  CHECK(result.find("virtual void test_notification2(const Json::Value& "
                    "object, const Json::Value& values) = 0;") != string::npos);
  CHECK(result.find("this->bindAndAddMethod(jsonrpc::Procedure(\"test.method\","
                    " jsonrpc::PARAMS_BY_NAME, jsonrpc::JSON_STRING, "
                    "\"name\",jsonrpc::JSON_STRING, NULL), "
                    "&ns1::ns2::TestStubServer::test_methodI);") !=
        string::npos);
  CHECK(result.find("inline virtual void test_notificationI("
                    "const Json::Value &request)") !=
        string::npos);
  CHECK(result.find("inline virtual void test_methodI("
                    "const Json::Value &request, "
                    "Json::Value &response)") !=
        string::npos);
  CHECK(result.find("inline virtual void testmethod5I("
                     "const Json::Value &/*request*/, "
                     "Json::Value &response)") !=
        string::npos);
  CHECK(result.find("inline virtual void testmethod6I("
                    "const Json::Value &/*request*/)") !=
        string::npos);
  CHECK(result.find("#endif //JSONRPC_CPP_STUB_NS1_NS2_TESTSTUBSERVER_H_") !=
        string::npos);
}

TEST_CASE("test_stubgen_jsclient", TEST_MODULE) {
  stringstream stream;
  vector<Procedure> procedures =
      SpecificationParser::GetProceduresFromFile("testspec6.json");
  JSClientStubGenerator stubgen("TestStubClient", procedures, stream);
  stubgen.generateStub();
  string result = stream.str();

  CHECK(result.find("function TestStubClient(url) {") != string::npos);
  CHECK(result.find("TestStubClient.prototype.test_method = function(name, "
                    "callbackSuccess, callbackError)") != string::npos);
  CHECK(result.find("TestStubClient.prototype.test_notification = "
                    "function(name, callbackSuccess, callbackError)") !=
        string::npos);
  CHECK(result.find("TestStubClient.prototype.test_method2 = function(object, "
                    "values, callbackSuccess, callbackError)") != string::npos);
  CHECK(result.find("TestStubClient.prototype.test_notification2 = "
                    "function(object, values, callbackSuccess, "
                    "callbackError)") != string::npos);

  CHECK(JSClientStubGenerator::class2Filename("TestClass") == "testclass.js");
}

TEST_CASE("test_stubgen_pyclient", TEST_MODULE) {
  stringstream stream;
  vector<Procedure> procedures =
      SpecificationParser::GetProceduresFromFile("testspec6.json");
  PythonClientStubGenerator stubgen("TestStubClient", procedures, stream);
  stubgen.generateStub();
  string result = stream.str();

  CHECK(result.find("from jsonrpc_pyclient import client") != string::npos);
  CHECK(result.find("class TestStubClient(client.Client):") != string::npos);
  CHECK(result.find("def __init__(self, connector, version='2.0'):") !=
        string::npos);
  CHECK(result.find("def test_method(self, name):") != string::npos);
  CHECK(result.find("def test_notification(self, name):") != string::npos);
  CHECK(result.find("def test_method2(self, object, values):") != string::npos);
  CHECK(result.find("def test_notification2(self, object, values):") !=
        string::npos);

  CHECK(PythonClientStubGenerator::class2Filename("TestClass") ==
        "testclass.py");
}

TEST_CASE("test_stubgen_indentation", TEST_MODULE) {
  stringstream stream;
  CodeGenerator cg(stream);
  cg.setIndentSymbol("   ");
  cg.increaseIndentation();
  cg.write("abc");
  CHECK(stream.str() == "   abc");

  stringstream stream2;
  CodeGenerator cg2(stream2);
  cg2.setIndentSymbol("\t");
  cg2.increaseIndentation();
  cg2.write("abc");
  CHECK(stream2.str() == "\tabc");
}

TEST_CASE_METHOD(F, "test_stubgen_factory_help", TEST_MODULE) {
  const char *argv[2] = {"jsonrpcstub", "-h"};
  CHECK(StubGeneratorFactory::createStubGenerators(
            2, (char **)argv, procedures, stubgens, stdout, stderr) == true);
  CHECK(stubgens.empty() == true);
  CHECK(procedures.empty() == true);
}

TEST_CASE_METHOD(F, "test_stubgen_factory_version", TEST_MODULE) {
  const char *argv[2] = {"jsonrpcstub", "--version"};

  CHECK(StubGeneratorFactory::createStubGenerators(
            2, (char **)argv, procedures, stubgens, stdout, stderr) == true);
  CHECK(stubgens.empty() == true);
  CHECK(procedures.empty() == true);
}

TEST_CASE_METHOD(F, "test_stubgen_factory_error", TEST_MODULE) {
  const char *argv[2] = {"jsonrpcstub", "--cpp-client=TestClient"};

  CHECK(StubGeneratorFactory::createStubGenerators(
            2, (char **)argv, procedures, stubgens, stdout, stderr) == false);
  CHECK(stubgens.empty() == true);
  CHECK(procedures.empty() == true);

  vector<StubGenerator *> stubgens2;
  vector<Procedure> procedures2;
  const char *argv2[2] = {"jsonrpcstub", "--cpxp-client=TestClient"};

  CHECK(StubGeneratorFactory::createStubGenerators(2, (char **)argv2,
                                                   procedures2, stubgens2,
                                                   stdout, stderr) == false);
  CHECK(stubgens2.empty() == true);
  CHECK(procedures2.empty() == true);

  vector<StubGenerator *> stubgens3;
  vector<Procedure> procedures3;
  const char *argv3[3] = {"jsonrpcstub", "testspec1.json",
                          "--cpp-client=TestClient"};

  CHECK(StubGeneratorFactory::createStubGenerators(3, (char **)argv3,
                                                   procedures3, stubgens3,
                                                   stdout, stderr) == false);
  CHECK(stubgens3.empty() == true);
  CHECK(procedures3.empty() == true);
}

TEST_CASE_METHOD(F, "test_stubgen_factory_success", TEST_MODULE) {
  vector<StubGenerator *> stubgens;
  vector<Procedure> procedures;
  const char *argv[5] = {"jsonrpcstub", "testspec6.json",
                         "--js-client=TestClient", "--cpp-client=TestClient",
                         "--cpp-server=TestServer"};

  CHECK(StubGeneratorFactory::createStubGenerators(
            5, (char **)argv, procedures, stubgens, stdout, stderr) == true);
  CHECK(stubgens.size() == 3);
  CHECK(procedures.size() == 8);

  StubGeneratorFactory::deleteStubGenerators(stubgens);
}

TEST_CASE_METHOD(F, "test_stubgen_factory_fileoverride", TEST_MODULE) {
  vector<StubGenerator *> stubgens;
  vector<Procedure> procedures;
  const char *argv[9] = {"jsonrpcstub",
                         "testspec6.json",
                         "--js-client=TestClient",
                         "--cpp-client=TestClient",
                         "--cpp-server=TestServer",
                         "--cpp-client-file=client.h",
                         "--cpp-server-file=server.h",
                         "--js-client-file=client.js",
                         "-v"};

  CHECK(StubGeneratorFactory::createStubGenerators(
            9, (char **)argv, procedures, stubgens, stdout, stderr) == true);
  CHECK(stubgens.size() == 3);
  CHECK(procedures.size() == 8);
  StubGeneratorFactory::deleteStubGenerators(stubgens);
}

#endif
