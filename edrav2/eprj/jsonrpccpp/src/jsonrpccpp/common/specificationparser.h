/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    specificationparser.h
 * @date    12.03.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_SPECIFICATIONPARSER_H
#define JSONRPC_CPP_SPECIFICATIONPARSER_H

#include "procedure.h"
#include "exception.h"

namespace jsonrpc {

    class SpecificationParser
    {
        public:
            static std::vector<Procedure> GetProceduresFromFile(const std::string& filename)    ;
            static std::vector<Procedure> GetProceduresFromString(const std::string& spec)      ;

            static void         GetFileContent  (const std::string& filename, std::string& target);

        private:
            static void         GetProcedure    (Json::Value& val, Procedure &target);
            static void         GetMethod       (Json::Value& val, Procedure &target);
            static void         GetNotification (Json::Value& val, Procedure &target);
            static jsontype_t   toJsonType      (Json::Value& val);

            static void         GetPositionalParameters (Json::Value &val, Procedure &target);
            static void         GetNamedParameters      (Json::Value &val, Procedure &target);
            static std::string  GetProcedureName        (Json::Value &signature);

    };
}
#endif // JSONRPC_CPP_SPECIFICATIONPARSER_H
