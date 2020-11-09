/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    serverstubgenerator.h
 * @date    01.05.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_SERVERSTUBGENERATOR_H
#define JSONRPC_CPP_SERVERSTUBGENERATOR_H

#include "../stubgenerator.h"
#include "../codegenerator.h"

namespace jsonrpc
{
    class CPPServerStubGenerator : public StubGenerator
    {
        public:
            CPPServerStubGenerator(const std::string& stubname, std::vector<Procedure> &procedures, std::ostream &outputstream);
            CPPServerStubGenerator(const std::string& stubname, std::vector<Procedure> &procedures, const std::string &filename);

            virtual void generateStub();

            void generateBindings();
            void generateProcedureDefinitions();
            void generateAbstractDefinitions();
            std::string generateBindingParameterlist(const Procedure &proc);
            void generateParameterMapping(const Procedure &proc);
    };
}

#endif // JSONRPC_CPP_SERVERSTUBGENERATOR_H
