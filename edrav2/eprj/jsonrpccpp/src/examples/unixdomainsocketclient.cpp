/**
 * @file unixdomainsocketclient.cpp
 * @date 11.05.2015
 * @author Alexandre Poirot <alexandre.poirot@legrand.fr>
 * @brief unixdomainsocketclient.cpp
 */

#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/unixdomainsocketclient.h>

using namespace jsonrpc;
using namespace std;

int main() {
  UnixDomainSocketClient client("./unixdomainsocketexample");
  Client c(client);

  Json::Value params;
  params["name"] = "Peter";

  try {
    cout << c.CallMethod("sayHello", params) << endl;
  } catch (JsonRpcException &e) {
    cerr << e.what() << endl;
  }
}
