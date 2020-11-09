/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    jsclientstubgenerator.h
 * @date    10/22/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_JSCLIENTSTUBGENERATOR_H
#define JSONRPC_CPP_JSCLIENTSTUBGENERATOR_H

#include "../stubgenerator.h"

namespace jsonrpc {

    class JSClientStubGenerator : public StubGenerator
    {
        public:
            JSClientStubGenerator(const std::string& stubname, std::vector<Procedure> &procedures, std::ostream &outputstream);
            JSClientStubGenerator(const std::string& stubname, std::vector<Procedure> &procedures, const std::string &filename);

            static std::string class2Filename(const std::string &classname);

            virtual void generateStub();

        private:
            virtual void generateMethod(Procedure &proc);
            static std::string noramlizeJsLiteral(const std::string &literal);

    };

} // namespace jsonrpc

#endif // JSONRPC_JSCLIENTSTUBGENERATOR_H
