/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    codegenerator.h
 * @date    3/21/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_CODEGENERATOR_H
#define JSONRPC_CPP_CODEGENERATOR_H

#include <string>
#include <ostream>
#include <fstream>
#include <sstream>

namespace jsonrpc
{
    class CodeGenerator
    {
        public:
            CodeGenerator(const std::string &filename);
            CodeGenerator(std::ostream &outputstream);
            virtual ~CodeGenerator();

            void write (const std::string &line);
            void writeLine(const std::string &line);
            void writeNewLine();
            void increaseIndentation();
            void decreaseIndentation();

            void setIndentSymbol(const std::string &symbol);

        private:
            std::ostream *output;
            std::ofstream file;
            std::string indentSymbol;
            int indentation;
            bool atBeginning;
    };
}

#endif // JSONRPC_CPP_CODEGENERATOR_H
