/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    redisserver.cpp
 * @date    15.08.2017
 * @author  Jacques Software <software@jacques.com.au>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "redisserver.h"

using namespace jsonrpc;

/**
 * This is a helper method for the ListenLoop. Checks that the request is
 * valid and then retusn the request string and the queue to return the
 * response to.
 * @param req Redis request that has been received.
 * @param ret_queue The return queue is returned here.
 * @param request The request is returned here.
 * @return Returns true on success, false otherwise.
 */
bool ProcessRedisReply(redisReply *req, std::string &ret_queue,
                       std::string &request) {

  // The return from hiredis is strange in that it's always an array of
  // length 2, with the first element the name of the key as a string,
  // and the second element the actual element that we popped.
  if (req->type != REDIS_REPLY_ARRAY) {
    return false;
  }

  if (req->elements != 2) {
    return false;
  }

  // It's the second element that we care about
  redisReply *data = req->element[1];

  // It should be a json string
  if (data->type != REDIS_REPLY_STRING) {
    return false;
  }

  std::string json(data->str, data->str + data->len * sizeof data->str[0]);

  size_t pos = json.find("!");
  if (pos == std::string::npos) {
    return false;
  }

  ret_queue = json.substr(0, pos);
  request = json.substr(pos + 1);

  return true;
}

RedisServer::RedisServer(std::string host, int port, std::string queue)
    : running(false), host(host), port(port), queue(queue), con(NULL) {}

bool RedisServer::StartListening() {
  if (this->running) {
    return this->running;
  }

  con = redisConnect(host.c_str(), port);
  if (con == NULL) {
    return false;
  }
  if (con->err != 0) {
    redisFree(con);
    con = NULL;
    return false;
  }

  this->running = true;
  int ret = pthread_create(&(this->listenning_thread), NULL,
                           RedisServer::LaunchLoop, this);
  this->running = static_cast<bool>(ret == 0);

  return this->running;
}

bool RedisServer::StopListening() {
  if (!this->running) {
    return true;
  }
  this->running = false;
  pthread_join(this->listenning_thread, NULL);
  if (con != NULL) {
    redisFree(con);
  }
  return !(this->running);
}

bool RedisServer::SendResponse(const std::string &response,
                               const std::string &ret_queue) {
  redisReply *ret;
  ret = (redisReply *)redisCommand(con, "LPUSH %s %s", ret_queue.c_str(),
                                   response.c_str());

  if (ret == NULL) {
    return false;
  }

  if (ret->type != REDIS_REPLY_INTEGER || ret->integer <= 0) {
    freeReplyObject(ret);
    return false;
  }

  freeReplyObject(ret);
  return true;
}

void *RedisServer::LaunchLoop(void *p_data) {
  RedisServer *instance = reinterpret_cast<RedisServer *>(p_data);
  ;
  instance->ListenLoop();
  return NULL;
}

void RedisServer::ListenLoop() {
  while (this->running) {
    redisReply *req = NULL;
    std::string request;
    req = (redisReply *)redisCommand(con, "BRPOP %s 1", queue.c_str());
    if (req == NULL) {
      continue;
    }

    if (req->type == REDIS_REPLY_NIL) {
      freeReplyObject(req);
      continue;
    }

    std::string ret_queue;
    bool ret = ProcessRedisReply(req, ret_queue, request);
    freeReplyObject(req);
    if (ret == false) {
      continue;
    }

    if (this->running) {
      std::string response;
      this->ProcessRequest(request, response);
      this->SendResponse(response, ret_queue);
    }
  }
}
