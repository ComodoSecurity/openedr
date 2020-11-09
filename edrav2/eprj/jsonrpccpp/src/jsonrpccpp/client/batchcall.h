/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    batchcall.h
 * @date    15.10.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_BATCHCALL_H
#define JSONRPC_CPP_BATCHCALL_H

#include <jsonrpccpp/common/jsonparser.h>

namespace jsonrpc
{
    class BatchCall
    {
        public:
            BatchCall   ();

            /**
             * @brief addCall
             * @param methodname
             * @param params
             * @param isNotification
             * @return the id of the geneared request inside the batchcall
             */
            int         addCall     (const std::string &methodname, const Json::Value &params, bool isNotification = false);
            std::string toString    (bool fast = true) const;

        private:
            Json::Value result;
            int id;
    };
}

#endif // JSONRPC_CPP_BATCHCALL_H
