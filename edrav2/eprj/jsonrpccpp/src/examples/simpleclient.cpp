/**
 * @file simpleclient.cpp
 * @date 03.01.2013
 * @author Peter Spiess-Knafl <dev@spiessknafl.at>
 * @brief This is a simple client example.
 */

#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/httpclient.h>

using namespace jsonrpc;
using namespace std;

int main() {
  HttpClient client("http://localhost:8383");
  Client c(client);

  Json::Value params;
  params["name"] = "Peter";

  try {
    cout << c.CallMethod("sayHello", params) << endl;
  } catch (JsonRpcException &e) {
    cerr << e.what() << endl;
  }
}
