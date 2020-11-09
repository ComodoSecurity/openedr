/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    stubgenerator.cpp
 * @date    01.05.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include <algorithm>
#include <fstream>
#include <jsonrpccpp/common/specificationparser.h>

#include <argtable2.h>

#include "client/cppclientstubgenerator.h"
#include "client/jsclientstubgenerator.h"
#include "client/pyclientstubgenerator.h"
#include "helper/cpphelper.h"
#include "server/cppserverstubgenerator.h"
#include "stubgenerator.h"

using namespace std;
using namespace jsonrpc;

#define EXIT_ERROR(X)                                                          \
  cerr << X << endl;                                                           \
  arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));             \
  return 1;

StubGenerator::StubGenerator(const string &stubname,
                             std::vector<Procedure> &procedures,
                             ostream &outputstream)
    : CodeGenerator(outputstream), stubname(stubname), procedures(procedures) {}

StubGenerator::StubGenerator(const string &stubname,
                             std::vector<Procedure> &procedures,
                             const std::string &filename)
    : CodeGenerator(filename), stubname(stubname), procedures(procedures) {}

StubGenerator::~StubGenerator() {}

string StubGenerator::replaceAll(const string &text, const string &fnd,
                                 const string &rep) {
  string result = text;
  replaceAll2(result, fnd, rep);
  return result;
}

void StubGenerator::replaceAll2(string &result, const string &find,
                                const string &replace) {
  size_t pos = result.find(find);
  while (pos != string::npos) {
    result.replace(pos, find.length(), replace);
    pos = result.find(find, pos + replace.length());
  }
}
