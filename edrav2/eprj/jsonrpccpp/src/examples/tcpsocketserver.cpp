/**
 * @file unixdomainsocketserver.cpp
 * @date 11.05.2015
 * @author Alexandre Poirot
 * @brief unixdomainsocketserver.cpp
 */

#include <cstdlib>
#include <iostream>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/tcpsocketserver.h>
#include <stdio.h>
#include <string>

using namespace jsonrpc;
using namespace std;

class SampleServer : public AbstractServer<SampleServer> {
public:
  SampleServer(TcpSocketServer &server) : AbstractServer<SampleServer>(server) {
    this->bindAndAddMethod(Procedure("sayHello", PARAMS_BY_NAME, JSON_STRING,
                                     "name", JSON_STRING, NULL),
                           &SampleServer::sayHello);
    this->bindAndAddNotification(
        Procedure("notifyServer", PARAMS_BY_NAME, NULL),
        &SampleServer::notifyServer);
  }

  // method
  void sayHello(const Json::Value &request, Json::Value &response) {
    response = "Hello: " + request["name"].asString();
  }

  // notification
  void notifyServer(const Json::Value &request) {
    (void)request;
    cout << "server received some Notification" << endl;
  }
};

int main(int argc, char **argv) {
  try {
    string ip;
    unsigned int port;

    if (argc == 3) {
      ip = string(argv[1]);
      port = atoi(argv[2]);
    } else {
      ip = "127.0.0.1";
      port = 6543;
    }

    cout << "Params are :" << endl;
    cout << "\t ip: " << ip << endl;
    cout << "\t port: " << port << endl;

    TcpSocketServer server(ip, port);
    SampleServer serv(server);
    if (serv.StartListening()) {
      cout << "Server started successfully" << endl;
      getchar();
      serv.StopListening();
    } else {
      cout << "Error starting Server" << endl;
    }
  } catch (jsonrpc::JsonRpcException &e) {
    cerr << e.what() << endl;
  }
}
