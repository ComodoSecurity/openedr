/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    clientstubgenerator.h
 * @date    01.05.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_CLIENTSTUBGENERATOR_H
#define JSONRPC_CPP_CLIENTSTUBGENERATOR_H

#include "../stubgenerator.h"
#include "../codegenerator.h"

namespace jsonrpc
{
    class CPPClientStubGenerator : public StubGenerator
    {
        public:


            CPPClientStubGenerator(const std::string& stubname, std::vector<Procedure> &procedures, std::ostream& outputstream);
            CPPClientStubGenerator(const std::string& stubname, std::vector<Procedure> &procedures, const std::string filename);

            virtual void generateStub();

            void generateMethod(Procedure& proc);
            void generateAssignments(Procedure& proc);
            void generateProcCall(Procedure &proc);
    };
}
#endif // JSONRPC_CPP_CLIENTSTUBGENERATOR_H
