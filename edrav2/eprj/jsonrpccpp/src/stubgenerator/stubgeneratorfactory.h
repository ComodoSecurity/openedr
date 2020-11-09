/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    stubgeneratorfactory.h
 * @date    11/19/2014
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_STUBGENERATORFACTORY_H
#define JSONRPC_CPP_STUBGENERATORFACTORY_H

#include <vector>
#include "stubgenerator.h"

namespace jsonrpc {

    class StubGeneratorFactory
    {
        public:
            static bool createStubGenerators(int argc, char** argv, std::vector<Procedure> &procedures, std::vector<StubGenerator*> &stubgenerators, FILE* _stdout, FILE* _stderr);
            static void deleteStubGenerators(std::vector<StubGenerator*> &stubgenerators);
    };

} // namespace jsonrpc

#endif // JSONRPC_CPP_STUBGENERATORFACTORY_H
