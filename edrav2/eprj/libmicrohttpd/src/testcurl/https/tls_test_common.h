/*
 This file is part of libmicrohttpd
 Copyright (C) 2007 Christian Grothoff

 libmicrohttpd is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 2, or (at your
 option) any later version.

 libmicrohttpd is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with libmicrohttpd; see the file COPYING.  If not, write to the
 Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef TLS_TEST_COMMON_H_
#define TLS_TEST_COMMON_H_

#include "platform.h"
#include "microhttpd.h"
#include <curl/curl.h>
#include <sys/stat.h>
#include <limits.h>
#include <gnutls/gnutls.h>

/* this enables verbos CURL version checking */
#define DEBUG_HTTPS_TEST 0
#define CURL_VERBOS_LEVEL 0

#define test_data "Hello World\n"
#define ca_cert_file_name "tmp_ca_cert.pem"

#define EMPTY_PAGE "<html><head><title>Empty page</title></head><body>Empty page</body></html>"
#define PAGE_NOT_FOUND "<html><head><title>File not found</title></head><body>File not found</body></html>"

#define MHD_E_MEM "Error: memory error\n"
#define MHD_E_SERVER_INIT "Error: failed to start server\n"
#define MHD_E_TEST_FILE_CREAT "Error: failed to setup test file\n"
#define MHD_E_CERT_FILE_CREAT "Error: failed to setup test certificate\n"
#define MHD_E_KEY_FILE_CREAT "Error: failed to setup test certificate\n"
#define MHD_E_FAILED_TO_CONNECT "Error: server connection could not be established\n"

/* TODO rm if unused */
struct https_test_data
{
  void *cls;
  int port;
  const char *cipher_suite;
  int proto_version;
};

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};

struct CipherDef
{
  int options[2];
  char *curlname;
};


int
curl_check_version (const char *req_version, ...);

int
curl_uses_nss_ssl (void);


FILE *
setup_ca_cert (void);

/**
 * perform cURL request for file
 */
int
test_daemon_get (void * cls,
		 const char *cipher_suite, int proto_version,
                 int port, int ver_peer);

void
print_test_result (int test_outcome, char *test_name);

size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx);

int
http_ahc (void *cls, struct MHD_Connection *connection,
          const char *url, const char *method, const char *upload_data,
          const char *version, size_t *upload_data_size, void **ptr);

int
http_dummy_ahc (void *cls, struct MHD_Connection *connection,
                const char *url, const char *method, const char *upload_data,
                const char *version, size_t *upload_data_size,
                void **ptr);


/**
 * compile test file url pointing to the current running directory path
 *
 * @param[out] url - char buffer into which the url is compiled
 * @param url_len number of bytes available in @a url
 * @param port port to use for the test
 * @return -1 on error
 */
int
gen_test_file_url (char *url,
                   size_t url_len,
                   int port);

int
send_curl_req (char *url, struct CBC *cbc, const char *cipher_suite,
               int proto_version);

int
test_https_transfer (void *cls, int port, const char *cipher_suite, int proto_version);

int
setup_testcase (struct MHD_Daemon **d, int port, int daemon_flags, va_list arg_list);

void
teardown_testcase (struct MHD_Daemon *d);


int
setup_session (gnutls_session_t * session,
               gnutls_datum_t * key,
               gnutls_datum_t * cert,
               gnutls_certificate_credentials_t * xcred);

int
teardown_session (gnutls_session_t session,
                  gnutls_datum_t * key,
                  gnutls_datum_t * cert,
                  gnutls_certificate_credentials_t xcred);

int
test_wrap (const char *test_name, int
           (*test_function) (void * cls, int port, const char *cipher_suite,
                             int proto_version), void * cls,
           int port,
           int daemon_flags, const char *cipher_suite, int proto_version, ...);

int testsuite_curl_global_init (void);

#endif /* TLS_TEST_COMMON_H_ */
