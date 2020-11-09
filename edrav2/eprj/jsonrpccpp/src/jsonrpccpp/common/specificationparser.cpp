/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    specificationparser.cpp
 * @date    12.03.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "specificationparser.h"
#include <fstream>
#include <iomanip>
#include <jsonrpccpp/common/jsonparser.h>

using namespace std;
using namespace jsonrpc;

vector<Procedure>
SpecificationParser::GetProceduresFromFile(const string &filename) {
  string content;
  GetFileContent(filename, content);
  return GetProceduresFromString(content);
}
vector<Procedure>
SpecificationParser::GetProceduresFromString(const string &content) {

  Json::Reader reader;
  Json::Value val;
  if (!reader.parse(content, val)) {
    throw JsonRpcException(Errors::ERROR_RPC_JSON_PARSE_ERROR,
                           " specification file contains syntax errors");
  }

  if (!val.isArray()) {
    throw JsonRpcException(Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX,
                           " top level json value is not an array");
  }

  vector<Procedure> result;
  map<string, Procedure> procnames;
  for (unsigned int i = 0; i < val.size(); i++) {
    Procedure proc;
    GetProcedure(val[i], proc);
    if (procnames.find(proc.GetProcedureName()) != procnames.end()) {
      throw JsonRpcException(
          Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX,
          "Procedurename not unique: " + proc.GetProcedureName());
    }
    procnames[proc.GetProcedureName()] = proc;
    result.push_back(proc);
  }
  return result;
}
void SpecificationParser::GetProcedure(Json::Value &signature,
                                       Procedure &result) {
  if (signature.isObject() && GetProcedureName(signature) != "") {
    result.SetProcedureName(GetProcedureName(signature));
    if (signature.isMember(KEY_SPEC_RETURN_TYPE)) {
      result.SetProcedureType(RPC_METHOD);
      result.SetReturnType(toJsonType(signature[KEY_SPEC_RETURN_TYPE]));
    } else {
      result.SetProcedureType(RPC_NOTIFICATION);
    }
    if (signature.isMember(KEY_SPEC_PROCEDURE_PARAMETERS)) {
      if (signature[KEY_SPEC_PROCEDURE_PARAMETERS].isObject() ||
          signature[KEY_SPEC_PROCEDURE_PARAMETERS].isArray()) {
        if (signature[KEY_SPEC_PROCEDURE_PARAMETERS].isArray()) {
          result.SetParameterDeclarationType(PARAMS_BY_POSITION);
          GetPositionalParameters(signature, result);
        } else if (signature[KEY_SPEC_PROCEDURE_PARAMETERS].isObject()) {
          result.SetParameterDeclarationType(PARAMS_BY_NAME);
          GetNamedParameters(signature, result);
        }
      } else {
        throw JsonRpcException(
            Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX,
            "Invalid signature types in fileds: " + signature.toStyledString());
      }
    }
  } else {
    throw JsonRpcException(
        Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX,
        "procedure declaration does not contain name or parameters: " +
            signature.toStyledString());
  }
}
void SpecificationParser::GetFileContent(const std::string &filename,
                                         std::string &target) {
  ifstream config(filename.c_str());

  if (config) {
    config.open(filename.c_str(), ios::in);
    target.assign((std::istreambuf_iterator<char>(config)),
                  (std::istreambuf_iterator<char>()));
  } else {
    throw JsonRpcException(
        Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_NOT_FOUND, filename);
  }
}
jsontype_t SpecificationParser::toJsonType(Json::Value &val) {
  jsontype_t result;
  switch (val.type()) {
  case Json::uintValue:
  case Json::intValue:
    result = JSON_INTEGER;
    break;
  case Json::realValue:
    result = JSON_REAL;
    break;
  case Json::stringValue:
    result = JSON_STRING;
    break;
  case Json::booleanValue:
    result = JSON_BOOLEAN;
    break;
  case Json::arrayValue:
    result = JSON_ARRAY;
    break;
  case Json::objectValue:
    result = JSON_OBJECT;
    break;
  default:
    throw JsonRpcException(Errors::ERROR_SERVER_PROCEDURE_SPECIFICATION_SYNTAX,
                           "Unknown parameter type: " + val.toStyledString());
  }
  return result;
}
void SpecificationParser::GetPositionalParameters(Json::Value &val,
                                                  Procedure &result) {
  // Positional parameters
  for (unsigned int i = 0; i < val[KEY_SPEC_PROCEDURE_PARAMETERS].size(); i++) {
    stringstream paramname;
    paramname << "param" << std::setfill('0') << std::setw(2) << (i + 1);
    result.AddParameter(paramname.str(),
                        toJsonType(val[KEY_SPEC_PROCEDURE_PARAMETERS][i]));
  }
}
void SpecificationParser::GetNamedParameters(Json::Value &val,
                                             Procedure &result) {
  vector<string> parameters =
      val[KEY_SPEC_PROCEDURE_PARAMETERS].getMemberNames();
  for (unsigned int i = 0; i < parameters.size(); ++i) {
    result.AddParameter(
        parameters.at(i),
        toJsonType(val[KEY_SPEC_PROCEDURE_PARAMETERS][parameters.at(i)]));
  }
}

string SpecificationParser::GetProcedureName(Json::Value &signature) {
  if (signature[KEY_SPEC_PROCEDURE_NAME].isString())
    return signature[KEY_SPEC_PROCEDURE_NAME].asString();

  if (signature[KEY_SPEC_PROCEDURE_METHOD].isString())
    return signature[KEY_SPEC_PROCEDURE_METHOD].asString();

  if (signature[KEY_SPEC_PROCEDURE_NOTIFICATION].isString())
    return signature[KEY_SPEC_PROCEDURE_NOTIFICATION].asString();
  return "";
}
