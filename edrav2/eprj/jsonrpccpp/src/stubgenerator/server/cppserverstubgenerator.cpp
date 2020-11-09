/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    CPPServerStubGenerator.cpp
 * @date    01.05.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "cppserverstubgenerator.h"
#include "../helper/cpphelper.h"

#include <algorithm>
#include <jsonrpccpp/common/specificationwriter.h>
#include <sstream>

#define TEMPLATE_CPPSERVER_METHODBINDING                                       \
  "this->bindAndAddMethod(jsonrpc::Procedure(\"<rawprocedurename>\", "         \
  "<paramtype>, <returntype>, <parameterlist> NULL), "                         \
  "&<stubname>::<procedurename>I);"
#define TEMPLATE_CPPSERVER_NOTIFICATIONBINDING                                 \
  "this->bindAndAddNotification(jsonrpc::Procedure(\"<rawprocedurename>\", "   \
  "<paramtype>, <parameterlist> NULL), &<stubname>::<procedurename>I);"

#define TEMPLATE_CPPSERVER_SIGCLASS                                            \
  "class <stubname> : public jsonrpc::AbstractServer<<stubname>>"
#define TEMPLATE_CPPSERVER_SIGCONSTRUCTOR                                      \
  "<stubname>(jsonrpc::AbstractServerConnector &conn, "                        \
  "jsonrpc::serverVersion_t type = jsonrpc::JSONRPC_SERVER_V2) : "             \
  "jsonrpc::AbstractServer<<stubname>>(conn, type)"

#define TEMPLATE_CPPSERVER_SIGMETHOD                                           \
  "inline virtual void <procedurename>I(const Json::Value &request, "          \
  "Json::Value &response)"
#define TEMPLATE_CPPSERVER_SIGMETHOD_WITHOUT_PARAMS                            \
  "inline virtual void <procedurename>I(const Json::Value &/*request*/, "      \
  "Json::Value &response)"
#define TEMPLATE_CPPSERVER_SIGNOTIFICATION                                     \
  "inline virtual void <procedurename>I(const Json::Value &request)"
#define TEMPLATE_CPPSERVER_SIGNOTIFICATION_WITHOUT_PARAMS                      \
  "inline virtual void <procedurename>I(const Json::Value &/*request*/)"

#define TEMPLATE_SERVER_ABSTRACTDEFINITION                                     \
  "virtual <returntype> <procedurename>(<parameterlist>) = 0;"

using namespace std;
using namespace jsonrpc;

CPPServerStubGenerator::CPPServerStubGenerator(const std::string &stubname,
                                               vector<Procedure> &procedures,
                                               ostream &outputstream)
    : StubGenerator(stubname, procedures, outputstream) {}

CPPServerStubGenerator::CPPServerStubGenerator(
    const string &stubname, std::vector<Procedure> &procedures,
    const string &filename)
    : StubGenerator(stubname, procedures, filename) {}

void CPPServerStubGenerator::generateStub() {
  vector<string> classname = CPPHelper::splitPackages(this->stubname);
  CPPHelper::prolog(*this, this->stubname);

  this->writeLine("#include <jsonrpccpp/server.h>");
  this->writeNewLine();

  int depth = CPPHelper::namespaceOpen(*this, stubname);

  this->writeLine(replaceAll(TEMPLATE_CPPSERVER_SIGCLASS, "<stubname>",
                             classname.at(classname.size() - 1)));
  this->writeLine("{");
  this->increaseIndentation();
  this->writeLine("public:");
  this->increaseIndentation();

  this->writeLine(replaceAll(TEMPLATE_CPPSERVER_SIGCONSTRUCTOR, "<stubname>",
                             classname.at(classname.size() - 1)));
  this->writeLine("{");
  this->generateBindings();
  this->writeLine("}");

  this->writeNewLine();

  this->generateProcedureDefinitions();

  this->generateAbstractDefinitions();

  this->decreaseIndentation();
  this->decreaseIndentation();
  this->writeLine("};");
  this->writeNewLine();

  CPPHelper::namespaceClose(*this, depth);
  CPPHelper::epilog(*this, this->stubname);
}

void CPPServerStubGenerator::generateBindings() {
  string tmp;
  this->increaseIndentation();
  for (vector<Procedure>::const_iterator it = this->procedures.begin();
       it != this->procedures.end(); ++it) {
    const Procedure &proc = *it;
    if (proc.GetProcedureType() == RPC_METHOD) {
      tmp = TEMPLATE_CPPSERVER_METHODBINDING;
    } else {
      tmp = TEMPLATE_CPPSERVER_NOTIFICATIONBINDING;
    }
    replaceAll2(tmp, "<rawprocedurename>", proc.GetProcedureName());
    replaceAll2(tmp, "<procedurename>",
                CPPHelper::normalizeString(proc.GetProcedureName()));
    replaceAll2(tmp, "<returntype>", CPPHelper::toString(proc.GetReturnType()));
    replaceAll2(tmp, "<parameterlist>", generateBindingParameterlist(proc));
    replaceAll2(tmp, "<stubname>", this->stubname);

    if (proc.GetParameterDeclarationType() == PARAMS_BY_NAME) {
      replaceAll2(tmp, "<paramtype>", "jsonrpc::PARAMS_BY_NAME");
    } else {
      replaceAll2(tmp, "<paramtype>", "jsonrpc::PARAMS_BY_POSITION");
    }

    this->writeLine(tmp);
  }
  this->decreaseIndentation();
}

void CPPServerStubGenerator::generateProcedureDefinitions() {
  for (vector<Procedure>::const_iterator it = this->procedures.begin();
       it != this->procedures.end(); ++it) {
    const Procedure &proc = *it;
    if (proc.GetProcedureType() == RPC_METHOD) {
      if (!proc.GetParameters().empty()) {
        this->writeLine(
            replaceAll(TEMPLATE_CPPSERVER_SIGMETHOD, "<procedurename>",
                      CPPHelper::normalizeString(proc.GetProcedureName())));
      }
      else {
        this->writeLine(
            replaceAll(TEMPLATE_CPPSERVER_SIGMETHOD_WITHOUT_PARAMS, "<procedurename>",
                      CPPHelper::normalizeString(proc.GetProcedureName())));
      }
    }
    else {
      if (!proc.GetParameters().empty()) {
        this->writeLine(
            replaceAll(TEMPLATE_CPPSERVER_SIGNOTIFICATION, "<procedurename>",
                      CPPHelper::normalizeString(proc.GetProcedureName())));
      }
      else {
        this->writeLine(
            replaceAll(TEMPLATE_CPPSERVER_SIGNOTIFICATION_WITHOUT_PARAMS, "<procedurename>",
                      CPPHelper::normalizeString(proc.GetProcedureName())));
      }
    }

    this->writeLine("{");
    this->increaseIndentation();

    if (proc.GetProcedureType() == RPC_METHOD)
      this->write("response = ");
    this->write("this->");
    this->write(CPPHelper::normalizeString(proc.GetProcedureName()) + "(");
    this->generateParameterMapping(proc);
    this->writeLine(");");

    this->decreaseIndentation();
    this->writeLine("}");
  }
}

void CPPServerStubGenerator::generateAbstractDefinitions() {
  string tmp;
  for (vector<Procedure>::iterator it = this->procedures.begin();
       it != this->procedures.end(); ++it) {
    Procedure &proc = *it;
    tmp = TEMPLATE_SERVER_ABSTRACTDEFINITION;
    string returntype = "void";
    if (proc.GetProcedureType() == RPC_METHOD) {
      returntype = CPPHelper::toCppReturntype(proc.GetReturnType());
    }
    replaceAll2(tmp, "<returntype>", returntype);
    replaceAll2(tmp, "<procedurename>",
                CPPHelper::normalizeString(proc.GetProcedureName()));
    replaceAll2(tmp, "<parameterlist>",
                CPPHelper::generateParameterDeclarationList(proc));
    this->writeLine(tmp);
  }
}

string CPPServerStubGenerator::generateBindingParameterlist(const Procedure &proc) {
  stringstream parameter;
  const parameterNameList_t &list = proc.GetParameters();

  for (parameterNameList_t::const_iterator it2 = list.begin();
       it2 != list.end(); ++it2) {
    parameter << "\"" << it2->first << "\"," << CPPHelper::toString(it2->second)
              << ",";
  }
  return parameter.str();
}

void CPPServerStubGenerator::generateParameterMapping(const Procedure &proc) {
  string tmp;
  const parameterNameList_t &params = proc.GetParameters();
  int i = 0;
  for (parameterNameList_t::const_iterator it2 = params.begin();
       it2 != params.end(); ++it2) {
    if (proc.GetParameterDeclarationType() == PARAMS_BY_NAME) {
      tmp = "request[\"" + it2->first + "\"]" +
            CPPHelper::toCppConversion(it2->second);
    } else {
      stringstream tmp2;
      tmp2 << "request[" << i << "u]"
           << CPPHelper::toCppConversion(it2->second);
      tmp = tmp2.str();
    }
    this->write(tmp);
    if (it2 != --params.end()) {
      this->write(", ");
    }
    i++;
  }
}
