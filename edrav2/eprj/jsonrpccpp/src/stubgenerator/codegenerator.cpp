/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    codegenerator.cpp
 * @date    3/21/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "codegenerator.h"

using namespace jsonrpc;
using namespace std;

CodeGenerator::CodeGenerator(const ::string &filename)
    : indentation(0), atBeginning(true) {
  this->file.open(filename.c_str());
  this->output = &this->file;
  this->indentSymbol = "    ";
}

CodeGenerator::CodeGenerator(::ostream &outputstream)
    : output(&outputstream), indentSymbol("    "), indentation(0),
      atBeginning(true) {}

CodeGenerator::~CodeGenerator() {
  this->output->flush();
  if (this->file.is_open()) {
    this->file.close();
  }
}

void CodeGenerator::write(const ::string &line) {
  if (this->atBeginning) {
    this->atBeginning = false;
    for (int i = 0; i < this->indentation; i++)
      *this->output << this->indentSymbol;
  }
  *this->output << line;
}

void CodeGenerator::writeLine(const ::string &line) {
  this->write(line);
  this->writeNewLine();
}

void CodeGenerator::writeNewLine() {
  *this->output << endl;
  this->atBeginning = true;
}

void CodeGenerator::increaseIndentation() { this->indentation++; }

void CodeGenerator::decreaseIndentation() { this->indentation--; }

void CodeGenerator::setIndentSymbol(const ::string &symbol) {
  this->indentSymbol = symbol;
}
