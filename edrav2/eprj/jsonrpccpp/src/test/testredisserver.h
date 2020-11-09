/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    testredisserver.h
 * @date    24.08.2017
 * @author  Jacques Software <software@jacques.com.au>
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef JSONRPC_TESTREDISSERVER_H
#define JSONRPC_TESTREDISSERVER_H

#include <string>
#include <sys/types.h>

namespace jsonrpc {

    /**
     * This class is used to spawn a redis-server in the background and is
     * used to aid in running unit tests.
     */
    class TestRedisServer
    {
        public:
            TestRedisServer();
            ~TestRedisServer();

            /**
             * This method is used to spawn a redis-server in the background.
             * @return true on success, false for failure.
             */
            bool Start();


            /**
             * This method will kill the redis server when we are done with it.
             * @return true on success, false for failure.
             */
            bool Stop();

        private:

            /**
             * This is a helper method for Start. This should be called
             * by the child process and will setup the pipes for communication and
             * start the redis server.
             * @param pipefd The pipe that will be used for communication.
             */
            void StartProcess(int pipefd[2]);


            /**
             * This is a helper method for Start. This method is used to
             * block until the redis-server has finished starting up.
             * Uses key - Scan for this string to signal success.
             * Uses maxlines - Scan this many lines before giving up.
             * @param pipefd The pipe that will be used for communication.
             * @return true on success, false for failure.
             */
            bool WaitProcess(int pipefd[2]);

        private:

            /**
             * @brief The pid of the background redis-server.
             */
            pid_t pid;


            /**
             * @brief The maximum number of lines to search for the key.
             */
            unsigned int maxlines;

            /**
             * @brief The key string to indicate the server is ready
             */
            std::string key;
    };

} // namespace jsonrpc

#endif // JSONRPC_TESTREDISSERVER_H
