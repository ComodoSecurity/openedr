/**
 * @file redisclient.cpp
 * @date 18.08.2017
 * @author Jacques Software <software@jacques.com.au>
 * @brief This is a simple redis client example.
 */

#include <iostream>
#include <jsonrpccpp/client.h>
#include <jsonrpccpp/client/connectors/redisclient.h>
#include <unistd.h>

using namespace jsonrpc;
using namespace std;

int main() {

  execlp("redis-server", "redis-server", "-v", NULL);

  RedisClient client("127.0.0.1", 6379, "example");
  client.SetTimeout(1);
  Client c(client);

  Json::Value params;
  params["name"] = "Peter";
  Json::Value params2;

  try {
    cout << c.CallMethod("sayHello", params) << endl;
    c.CallNotification("notifyServer", params2);
  } catch (JsonRpcException &e) {
    cerr << e.what() << endl;
  }
}
