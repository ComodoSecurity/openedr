/**
 * @file filedescriptorclient.cpp
 * @date 27.10.2016
 * @author Jean-Daniel Michaud <jean.daniel,michaud@gmail.com>
 * @brief An example implementation of a client based on plain old file
 * descriptors
 */

/*
 * This example demonstrate the use of simple file descriptor to connect
 * processes with jsonrpc.
 *
 * First create a named pipe:
 *
 * ```
 * mkfifo FOO
 * ```
 *
 * Then launch the client and the server by connecting them to each other
 * this way:
 * ```
 * filedescriptorclientsample < PIPE | filedescriptorserversample > PIPE
 * ```
 *
 * The way the standard output of the client is connected to the standard
 * input of the server, and /vice versa/.
 *
 * You can even launch the server in standalone and enter json-rpc
 * compliant json string in the standard input to test it.
 */

#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/filedescriptorclient.h>
#include <unistd.h>

using namespace jsonrpc;
using namespace std;

int main() {
  FileDescriptorClient client(STDIN_FILENO, STDOUT_FILENO);
  Client c(client);

  Json::Value params;
  params["name"] = "Peter";

  try {
    cerr << "client:" << c.CallMethod("sayHello", params) << endl;
  } catch (JsonRpcException &e) {
    cerr << e.what() << endl;
  }
}
