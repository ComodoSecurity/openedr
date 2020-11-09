/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    testredisserver.cpp
 * @date    24.08.2017
 * @author  Jacques Software <software@jacques.com.au>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "testredisserver.h"

#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace jsonrpc;

#define REDIS_BIN "redis-server"
#define REDIS_BIN_DIR "/usr/bin/"
#define REDIS_CONF "redis.conf"
#define REDIS_MAXLINES 255
#define REDIS_KEY "accept connections"

TestRedisServer::TestRedisServer()
    : pid(0), maxlines(REDIS_MAXLINES), key(REDIS_KEY) {
  this->Start();
}

TestRedisServer::~TestRedisServer() { this->Stop(); }

bool TestRedisServer::Start() {
  int pipefd[2];

  pipe(pipefd);

  pid = fork();
  if (pid < 0) {
    cerr << "ERROR: Failed to fork for redis server!" << endl;
    return -1;
  }

  if (pid == 0) {
    this->StartProcess(pipefd);
  }

  int ret = this->WaitProcess(pipefd);
  if (ret == false) {
    cerr << "ERROR: Server failed to start.\n";
    this->Stop();
    return -1;
  }

  return true;
}

bool TestRedisServer::Stop() {
  if (pid != 0) {
    kill(pid, 9);
    pid = 0;
    return true;
  }
  return false;
}

bool TestRedisServer::WaitProcess(int pipefd[2]) {
  close(pipefd[1]);
  FILE *output = fdopen(pipefd[0], "r");

  unsigned int i = 0;
  do {
    i++;
    char *buffer = NULL;
    size_t n;
    getline(&buffer, &n, output);
    if (n <= 0) {
      free(buffer);
      continue;
    }
    string line(buffer);
    free(buffer);

    size_t found = line.find(key);
    if (found != string::npos) {
      return true;
    }

  } while (i < maxlines);

  return false;
}

void TestRedisServer::StartProcess(int pipefd[2]) {
  // Close the unused read end
  close(pipefd[0]);

  // We don't want input going to this process or errors from it.
  FILE *f_null = fopen("/dev/null", "r+");
  if (f_null == NULL) {
    cerr << "ERROR: Failed to open /dev/null for redis server!" << endl;
  }
  dup2(fileno(f_null), STDIN_FILENO);
  dup2(fileno(f_null), STDERR_FILENO);
  fclose(f_null);

  // Redirect output to our filedescriptor
  dup2(pipefd[1], STDOUT_FILENO);

  // Start redis server with our redis.conf
  execlp(REDIS_BIN, REDIS_BIN, REDIS_CONF, (char *)NULL);
  exit(-1);
}
