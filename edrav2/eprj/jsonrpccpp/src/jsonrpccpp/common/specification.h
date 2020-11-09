/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    specification.h
 * @date    30.04.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_SPECIFICATION_H
#define JSONRPC_CPP_SPECIFICATION_H

#define KEY_SPEC_PROCEDURE_NAME          "name"
#define KEY_SPEC_PROCEDURE_METHOD        "method"       //legacy format -> use name now
#define KEY_SPEC_PROCEDURE_NOTIFICATION  "notification" //legacy format -> use name now
#define KEY_SPEC_PROCEDURE_PARAMETERS    "params"
#define KEY_SPEC_RETURN_TYPE             "returns"

namespace jsonrpc
{
    /**
     * This enum describes whether a Procdeure is a notification procdeure or a method procdeure
     * @see http://groups.google.com/group/json-rpc/web/json-rpc-2-0
     */
    typedef enum
    {
        RPC_METHOD, RPC_NOTIFICATION
    } procedure_t;

    /**
     * This enum represents all processable json Types of this framework.
     */
    enum jsontype_t
    {
        JSON_STRING = 1,
        JSON_BOOLEAN = 2,
        JSON_INTEGER = 3,
        JSON_REAL = 4,
        JSON_OBJECT = 5,
        JSON_ARRAY = 6
    } ;
}

#endif // JSONRPC_CPP_SPECIFICATION_H
