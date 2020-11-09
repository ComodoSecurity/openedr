#include "streamreader.h"
#include <stdlib.h>
#include <string.h>
#include "unistd.h"

using namespace jsonrpc;
using namespace std;

StreamReader::StreamReader(size_t buffersize)
    : buffersize(buffersize), buffer(static_cast<char *>(malloc(buffersize))) {}

StreamReader::~StreamReader() { free(buffer); }

bool StreamReader::Read(std::string &target, int fd, char delimiter) {
  ssize_t bytesRead;
  do {
    bytesRead = read(fd, this->buffer, buffersize);
    if (bytesRead < 0) {
      return false;
    } else {
      target.append(buffer, static_cast<size_t>(bytesRead));
    }
  } while (memchr(buffer, delimiter, bytesRead) == NULL);//(target.find(delimiter) == string::npos && bytesRead > 0);

  target.pop_back();
  return true;
}
