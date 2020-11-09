/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    specificationwriter.cpp
 * @date    30.04.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "specificationwriter.h"
#include "jsonparser.h"
#include <fstream>
#include <iostream>
#include <jsonrpccpp/common/jsonparser.h>

using namespace std;
using namespace jsonrpc;

Json::Value
SpecificationWriter::toJsonValue(const vector<Procedure> &procedures) {
  Json::Value result;
  Json::Value row;
  for (unsigned int i = 0; i < procedures.size(); i++) {
    procedureToJsonValue(procedures.at(i), row);
    result[i] = row;
    row.clear();
  }
  return result;
}
std::string SpecificationWriter::toString(const vector<Procedure> &procedures) {
  Json::StyledWriter wr;
  return wr.write(toJsonValue(procedures));
}
bool SpecificationWriter::toFile(const std::string &filename,
                                 const vector<Procedure> &procedures) {
  ofstream file;
  file.open(filename.c_str(), ios_base::out);
  if (!file.is_open())
    return false;
  file << toString(procedures);
  file.close();
  return true;
}
Json::Value SpecificationWriter::toJsonLiteral(jsontype_t type) {
  Json::Value literal;
  switch (type) {
  case JSON_BOOLEAN:
    literal = true;
    break;
  case JSON_STRING:
    literal = "somestring";
    break;
  case JSON_REAL:
    literal = 1.0;
    break;
  case JSON_ARRAY:
    literal = Json::arrayValue;
    break;
  case JSON_OBJECT:
    literal["objectkey"] = "objectvalue";
    break;
  case JSON_INTEGER:
    literal = 1;
    break;
  }
  return literal;
}
void SpecificationWriter::procedureToJsonValue(const Procedure &procedure,
                                               Json::Value &target) {
  target[KEY_SPEC_PROCEDURE_NAME] = procedure.GetProcedureName();
  if (procedure.GetProcedureType() == RPC_METHOD) {
    target[KEY_SPEC_RETURN_TYPE] = toJsonLiteral(procedure.GetReturnType());
  }
  for (parameterNameList_t::const_iterator it =
           procedure.GetParameters().begin();
       it != procedure.GetParameters().end(); ++it) {
    if (procedure.GetParameterDeclarationType() == PARAMS_BY_NAME) {
      target[KEY_SPEC_PROCEDURE_PARAMETERS][it->first] =
          toJsonLiteral(it->second);
    } else {
      target[KEY_SPEC_PROCEDURE_PARAMETERS].append(toJsonLiteral(it->second));
    }
  }
}
