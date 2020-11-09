/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    CPPClientStubGenerator.cpp
 * @date    01.05.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "cppclientstubgenerator.h"
#include "../helper/cpphelper.h"

#define TEMPLATE_CPPCLIENT_SIGCLASS "class <stubname> : public jsonrpc::Client"

#define TEMPLATE_CPPCLIENT_SIGCONSTRUCTOR                                      \
  "<stubname>(jsonrpc::IClientConnector &conn, jsonrpc::clientVersion_t type " \
  "= jsonrpc::JSONRPC_CLIENT_V2) : jsonrpc::Client(conn, type) {}"

#define TEMPLATE_CPPCLIENT_SIGMETHOD "<returntype> <methodname>(<parameters>) "

#define TEMPLATE_NAMED_ASSIGNMENT "p[\"<paramname>\"] = <paramname>;"
#define TEMPLATE_POSITION_ASSIGNMENT "p.append(<paramname>);"

#define TEMPLATE_METHODCALL                                                    \
  "Json::Value result = this->CallMethod(\"<name>\",p);"
#define TEMPLATE_NOTIFICATIONCALL "this->CallNotification(\"<name>\",p);"

#define TEMPLATE_RETURNCHECK "if (result<cast>)"
#define TEMPLATE_RETURN "return result<cast>;"

using namespace std;
using namespace jsonrpc;

CPPClientStubGenerator::CPPClientStubGenerator(
    const string &stubname, std::vector<Procedure> &procedures,
    std::ostream &outputstream)
    : StubGenerator(stubname, procedures, outputstream) {}

CPPClientStubGenerator::CPPClientStubGenerator(
    const string &stubname, std::vector<Procedure> &procedures,
    const string filename)
    : StubGenerator(stubname, procedures, filename) {}

void CPPClientStubGenerator::generateStub() {
  vector<string> classname = CPPHelper::splitPackages(this->stubname);
  CPPHelper::prolog(*this, this->stubname);
  this->writeLine("#include <jsonrpccpp/client.h>");
  this->writeNewLine();

  int depth = CPPHelper::namespaceOpen(*this, stubname);

  this->writeLine(replaceAll(TEMPLATE_CPPCLIENT_SIGCLASS, "<stubname>",
                             classname.at(classname.size() - 1)));
  this->writeLine("{");
  this->increaseIndentation();
  this->writeLine("public:");
  this->increaseIndentation();

  this->writeLine(replaceAll(TEMPLATE_CPPCLIENT_SIGCONSTRUCTOR, "<stubname>",
                             classname.at(classname.size() - 1)));
  this->writeNewLine();

  for (unsigned int i = 0; i < procedures.size(); i++) {
    this->generateMethod(procedures[i]);
  }

  this->decreaseIndentation();
  this->decreaseIndentation();
  this->writeLine("};");
  this->writeNewLine();

  CPPHelper::namespaceClose(*this, depth);
  CPPHelper::epilog(*this, this->stubname);
}

void CPPClientStubGenerator::generateMethod(Procedure &proc) {
  string procsignature = TEMPLATE_CPPCLIENT_SIGMETHOD;
  string returntype = CPPHelper::toCppReturntype(proc.GetReturnType());
  if (proc.GetProcedureType() == RPC_NOTIFICATION)
    returntype = "void";

  replaceAll2(procsignature, "<returntype>", returntype);
  replaceAll2(procsignature, "<methodname>",
              CPPHelper::normalizeString(proc.GetProcedureName()));
  replaceAll2(procsignature, "<parameters>",
              CPPHelper::generateParameterDeclarationList(proc));

  this->writeLine(procsignature);
  this->writeLine("{");
  this->increaseIndentation();

  this->writeLine("Json::Value p;");

  generateAssignments(proc);
  generateProcCall(proc);

  this->decreaseIndentation();
  this->writeLine("}");
}

void CPPClientStubGenerator::generateAssignments(Procedure &proc) {
  string assignment;
  parameterNameList_t list = proc.GetParameters();
  if (list.size() > 0) {
    for (parameterNameList_t::iterator it = list.begin(); it != list.end();
         ++it) {

      if (proc.GetParameterDeclarationType() == PARAMS_BY_NAME) {
        assignment = TEMPLATE_NAMED_ASSIGNMENT;
      } else {
        assignment = TEMPLATE_POSITION_ASSIGNMENT;
      }
      replaceAll2(assignment, "<paramname>", it->first);
      this->writeLine(assignment);
    }
  } else {
    this->writeLine("p = Json::nullValue;");
  }
}

void CPPClientStubGenerator::generateProcCall(Procedure &proc) {
  string call;
  if (proc.GetProcedureType() == RPC_METHOD) {
    call = TEMPLATE_METHODCALL;
    this->writeLine(replaceAll(call, "<name>", proc.GetProcedureName()));
    call = TEMPLATE_RETURNCHECK;
    replaceAll2(call, "<cast>",
                CPPHelper::isCppConversion(proc.GetReturnType()));
    this->writeLine(call);
    this->increaseIndentation();
    call = TEMPLATE_RETURN;
    replaceAll2(call, "<cast>",
                CPPHelper::toCppConversion(proc.GetReturnType()));
    this->writeLine(call);
    this->decreaseIndentation();
    this->writeLine("else");
    this->increaseIndentation();
    this->writeLine("throw "
                    "jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_"
                    "INVALID_RESPONSE, result.toStyledString());");
    this->decreaseIndentation();
  } else {
    call = TEMPLATE_NOTIFICATIONCALL;
    replaceAll2(call, "<name>", proc.GetProcedureName());
    this->writeLine(call);
  }
}
