/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    redisserver.h
 * @date    15.08.2017
 * @author  Jacques Software <software@jacques.com.au>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_CPP_REDISSERVERCONNECTOR_H_
#define JSONRPC_CPP_REDISSERVERCONNECTOR_H_

#include <hiredis/hiredis.h>
#include <string>
#include <pthread.h>

#include "../abstractserverconnector.h"

namespace jsonrpc
{
  /**
   * This class is the redis implementation of an AbstractServerConnector.
   * It uses hiredis to connect to a designated redis server.
   *
   * The RedisServer watches a designated queue for incoming requests in
   * ListenLoop by trying BRPOP. When it does receive a request it grabs the
   * response/return queue name, processes the request and then uses LPUSH to
   * send the result back to the client.
   */
  class RedisServer: public AbstractServerConnector
  {
    public:
      /**
       * RedisServer
       * @param host The ip address of the redis server.
       * @param port The port of the redis server.
       * @param queue The queue to listen on.
       */
      RedisServer(std::string host, int port, std::string queue);


      /**
       * This method launches the listening loop that will handle client connections.
       * @return true for success, false otherwise.
       */
      bool StartListening();


      /**
       * This method stops the listening loop that will handle client connections.
       * @return True if successful, false otherwise or if not listening.
       */
      bool StopListening();


      /**
       * This method sends the result of the RPC Call back to the client
       * @param response The response to send to the client.
       * @param ret_queue The queue to send the response to.
       * @return A boolean that indicates the success or the failure of the operation.
       */
      bool SendResponse(const std::string& response, const std::string& ret_queue);

    private:

      /**
       * Callback for listening thread to start ListenLoop.
       * @param p_data A pointer to this object.
       * @return Nothing.
       */
      static void* LaunchLoop(void *p_data);


      /**
       * Main loop listening for connections. The loop pops requests off the
       * servers' queue and processes them. It then calls this class's
       * OnRequest method which handles the request and sends the response
       * using this classes' SendResponse method.
       */
      void ListenLoop();

    private:

      /**
       * @brief Keeps track of whether the server is running.
       */
      bool running;

      /**
       * @brief Our listening thread
       */
      pthread_t listenning_thread;

      /**
       * @brief Ip address of the redis server
       */
      std::string host;

      /**
       * @brief port of the redis server
       */
      int port;

      /**
       * @brief Queue that we are messaging.
       */
      std::string queue;

      /**
       * @brief Our connection to the redis server
       */
      redisContext * con;

  };
}

#endif /* JSONRPC_CPP_REDISSERVERCONNECTOR_H_ */
