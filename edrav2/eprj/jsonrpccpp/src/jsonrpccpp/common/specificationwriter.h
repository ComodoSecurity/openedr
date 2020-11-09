/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    specificationwriter.h
 * @date    30.04.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_SPECIFICATIONWRITER_H
#define JSONRPC_CPP_SPECIFICATIONWRITER_H

#include "procedure.h"
#include "specification.h"

namespace jsonrpc {
    class SpecificationWriter
    {
        public:
            static Json::Value  toJsonValue (const std::vector<Procedure>& procedures);
            static std::string  toString    (const std::vector<Procedure>& procedures);
            static bool         toFile      (const std::string& filename, const std::vector<Procedure>& procedures);

        private:
            static Json::Value  toJsonLiteral           (jsontype_t type);
            static void         procedureToJsonValue    (const Procedure& procedure, Json::Value& target);
    };
}

#endif // JSONRPC_CPP_SPECIFICATIONWRITER_H
