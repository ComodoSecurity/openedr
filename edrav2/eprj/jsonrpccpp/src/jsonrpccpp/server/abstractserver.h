/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    abstractserver.h
 * @date    30.12.2012
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_ABSTRACTSERVER_H_
#define JSONRPC_CPP_ABSTRACTSERVER_H_

#include <map>
#include <string>
#include <vector>
#include <jsonrpccpp/common/procedure.h>
#include "abstractserverconnector.h"
#include "iprocedureinvokationhandler.h"
#include "iclientconnectionhandler.h"
#include "requesthandlerfactory.h"

namespace jsonrpc
{

    template<class S>
    class AbstractServer : public IProcedureInvokationHandler
    {
        public:
            typedef void(S::*methodPointer_t)       (const Json::Value &parameter, Json::Value &result);
            typedef void(S::*notificationPointer_t) (const Json::Value &parameter);

            AbstractServer(AbstractServerConnector &connector, serverVersion_t type = JSONRPC_SERVER_V2) :
                connection(connector)
            {
                this->handler = RequestHandlerFactory::createProtocolHandler(type, *this);
                connector.SetHandler(this->handler);
            }

            virtual ~AbstractServer()
            {
                delete this->handler;
            }

            bool StartListening()
            {
                return connection.StartListening();
            }

            bool StopListening()
            {
                return connection.StopListening();
            }

            virtual void HandleMethodCall(Procedure &proc, const Json::Value& input, Json::Value& output)
            {
                S* instance = dynamic_cast<S*>(this);
                (instance->*methods[proc.GetProcedureName()])(input, output);
            }

            virtual void HandleNotificationCall(Procedure &proc, const Json::Value& input)
            {
                S* instance = dynamic_cast<S*>(this);
                (instance->*notifications[proc.GetProcedureName()])(input);
            }

            virtual void HandleMethodCall(const std::string &name, const Json::Value& input, Json::Value& output)
            {
                if (methods.find(name) == methods.end())
                    throw JsonRpcException(Errors::ERROR_RPC_METHOD_NOT_FOUND);

                S* instance = dynamic_cast<S*>(this);
                (instance->*methods[name])(input, output);
            }

        protected:
            bool bindAndAddMethod(const Procedure& proc, methodPointer_t pointer)
            {
                if(proc.GetProcedureType() == RPC_METHOD && !this->symbolExists(proc.GetProcedureName()))
                {
                    this->handler->AddProcedure(proc);
                    this->methods[proc.GetProcedureName()] = pointer;
                    return true;
                }
                return false;
            }

            bool bindAndAddNotification(const Procedure& proc, notificationPointer_t pointer)
            {
                if(proc.GetProcedureType() == RPC_NOTIFICATION && !this->symbolExists(proc.GetProcedureName()))
                {
                    this->handler->AddProcedure(proc);
                    this->notifications[proc.GetProcedureName()] = pointer;
                    return true;
                }
                return false;
            }

        private:
            AbstractServerConnector                         &connection;
            IProtocolHandler                                *handler;
            std::map<std::string, methodPointer_t>          methods;
            std::map<std::string, notificationPointer_t>    notifications;

            bool symbolExists(const std::string &name)
            {
                if (methods.find(name) != methods.end())
                    return true;
                if (notifications.find(name) != notifications.end())
                    return true;
                return false;
            }
    };

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_ABSTRACTSERVER_H_ */
