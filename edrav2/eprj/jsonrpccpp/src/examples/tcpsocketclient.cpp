/**
 * @file tcpsocketclient.cpp
 * @date 17.07.2015
 * @author Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @brief tcpsocketclient.cpp
 */

#include <cstdlib>
#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/tcpsocketclient.h>

using namespace jsonrpc;
using namespace std;

int main(int argc, char **argv) {
  string host;
  unsigned int port;

  if (argc == 3) {
    host = string(argv[1]);
    port = atoi(argv[2]);
  } else {
    host = "127.0.0.1";
    port = 6543;
  }

  cout << "Params are :" << endl;
  cout << "\t host: " << host << endl;
  cout << "\t port: " << port << endl;

  TcpSocketClient client(host, port);
  Client c(client);

  Json::Value params;
  params["name"] = "Peter";

  try {
    cout << c.CallMethod("sayHello", params) << endl;
  } catch (JsonRpcException &e) {
    cerr << e.what() << endl;
  }
}
