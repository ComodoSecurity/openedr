/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    redisclient.h
 * @date    12.08.2017
 * @author  Jacques Software <software@jacques.com.au>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_REDISCLIENT_H_
#define JSONRPC_CPP_REDISCLIENT_H_

#include "../iclientconnector.h"
#include <jsonrpccpp/common/exception.h>
#include <hiredis/hiredis.h>

namespace jsonrpc
{
    /**
     * This class is the redis implementation of an AbstractClientConnector.
     * It uses hiredis to connect to a designated redis server.
     *
     * The RedisClient sends the json string to the RedisServer by pushing
     * it to the RedisServer's queue using LPUSH. The RedisServer will pop
     * the item off the queue, handle it, then send its response back via
     * another queue using LPUSH. The client will wait for this response for
     * `timeout` seconds using BRPOP.
     *
     * The queue used to handle the responses must be unique, therefore the
     * RedisClient generates a random queue name with the prefix of the
     * RedisServer's queue, and ensures that it is unique using EXISTS.
     * It then prepends the response/return queue name to the json string
     * with an exlimation mark used as a seperator.
     */
    class RedisClient : public IClientConnector
    {
        public:

            /**
             * RedisClient
             * @param host The ip address of the redis server.
             * @param port The port of the redis server.
             * @param queue The queue to send to.
             */
            RedisClient(const std::string& host, int port, const std::string& queue);
            virtual ~RedisClient();


            /**
             * This method will send an rpc message to the RedisServer and return the result.
             * @param message The message to send.
             * @param result The returned message from the server.
             */
            virtual void SendRPCMessage(const std::string& message, std::string& result);


            /**
             * Set the queue that we are messaging with.
             * @param queue The queue to send to.
             */
            void SetQueue(const std::string& queue);


            /**
             * Set how long we should wait for a response.
             * @param timeout The length of time to wait in seconds
             */
            void SetTimeout(long timeout);

        private:

            /**
             * @brief Queue that we are messaging
             */
            std::string queue;

            /**
             * @brief Timeout for http request in milliseconds
             */
            long timeout;

            /**
             * @brief Our connection to the redis server
             */
            redisContext * con;
    };

    // Exported here for unit testing purposes.
    void GetReturnQueue(redisContext * con, const std::string& prefix, std::string& ret_queue);

} /* namespace jsonrpc */
#endif /* JSONRPC_CPP_REDISCLIENT_H_ */
