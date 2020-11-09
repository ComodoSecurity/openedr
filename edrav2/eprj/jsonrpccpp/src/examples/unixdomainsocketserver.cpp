/**
 * @file unixdomainsocketserver.cpp
 * @date 11.05.2015
 * @author Alexandre Poirot
 * @brief unixdomainsocketserver.cpp
 */

#include <iostream>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/unixdomainsocketserver.h>
#include <stdio.h>
#include <string>

using namespace jsonrpc;
using namespace std;

class SampleServer : public AbstractServer<SampleServer> {
public:
  SampleServer(UnixDomainSocketServer &server)
      : AbstractServer<SampleServer>(server) {
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

int main() {
  try {
    UnixDomainSocketServer server("./unixdomainsocketexample");
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
