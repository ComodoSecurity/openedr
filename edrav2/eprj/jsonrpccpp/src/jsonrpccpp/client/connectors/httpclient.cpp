/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file    httpclient.cpp
 * @date    02.01.2013
 * @author  Peter Spiess-Knafl <dev@spiessknafl.at>
 * @license See attached LICENSE.txt
 ************************************************************************/

#include "httpclient.h"
#include <cstdlib>
#include <curl/curl.h>
#include <string.h>
#include <string>

#include <iostream>

using namespace jsonrpc;

class curl_initializer {
public:
  curl_initializer() { curl_global_init(CURL_GLOBAL_ALL); }
  ~curl_initializer() { curl_global_cleanup(); }
};

// See here: http://curl.haxx.se/libcurl/c/curl_global_init.html
static curl_initializer _curl_init = curl_initializer();

/**
 * taken from
 * http://stackoverflow.com/questions/2329571/c-libcurl-get-output-into-a-string
 */
struct string {
  char *ptr;
  size_t len;
};

static size_t writefunc(void *ptr, size_t size, size_t nmemb,
                        struct string *s) {
  size_t new_len = s->len + size * nmemb;
  s->ptr = (char *)realloc(s->ptr, new_len + 1);
  memcpy(s->ptr + s->len, ptr, size * nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;
  return size * nmemb;
}

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = static_cast<char *>(malloc(s->len + 1));
  s->ptr[0] = '\0';
}

HttpClient::HttpClient(const std::string &url, bool verbose) : url(url) {
  this->timeout = 10000;
  this->verbose = verbose;
  curl = curl_easy_init();
}

HttpClient::~HttpClient() { curl_easy_cleanup(curl); }

void HttpClient::SendRPCMessage(const std::string &message,
                                std::string &result) {

  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_URL, this->url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);

  CURLcode res;

  struct string s;
  init_string(&s);

  struct curl_slist *headers = NULL;

  for (std::map<std::string, std::string>::iterator header =
           this->headers.begin();
       header != this->headers.end(); ++header) {
    headers = curl_slist_append(
        headers, (header->first + ": " + header->second).c_str());
  }

  headers = curl_slist_append(headers, "Content-Type: application/json");
  headers = curl_slist_append(headers, "charsets: utf-8");

  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout);
  //curl_easy_setopt(curl, CURLOPT_MAXCONNECTS, 26);
  if (this->verbose)
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  res = curl_easy_perform(curl);

  result = s.ptr;
  free(s.ptr);
  curl_slist_free_all(headers);
  if (res != CURLE_OK) {
    std::stringstream str;
    str << "libcurl error " << res;

    if (res == 7)
      str << " - Could not connect to " << this->url;
    else if (res == 28)
      str << " - Operation timed out";
    throw JsonRpcException(Errors::ERROR_CLIENT_CONNECTOR, str.str());
  }

  long http_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

  if (http_code != 200) {
    throw JsonRpcException(Errors::ERROR_RPC_INTERNAL_ERROR, result);
  }
}

void HttpClient::SetUrl(const std::string &url) { this->url = url; }

void HttpClient::SetTimeout(long timeout) { this->timeout = timeout; }

void HttpClient::AddHeader(const std::string &attr, const std::string &val) {
  this->headers[attr] = val;
}

void HttpClient::RemoveHeader(const std::string &attr) {
  this->headers.erase(attr);
}
