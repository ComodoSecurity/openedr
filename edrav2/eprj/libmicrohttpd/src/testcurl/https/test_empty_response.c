/*
 This file is part of libmicrohttpd
 Copyright (C) 2013 Christian Grothoff

 libmicrohttpd is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published
 by the Free Software Foundation; either version 3, or (at your
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

/**
 * @file test_empty_response.c
 * @brief  Testcase for libmicrohttpd HTTPS GET operations with emtpy reply
 * @author Christian Grothoff
 */
#include "platform.h"
#include "microhttpd.h"
#include <limits.h>
#include <sys/stat.h>
#include <curl/curl.h>
#ifdef MHD_HTTPS_REQUIRE_GRYPT
#include <gcrypt.h>
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
#include "tls_test_common.h"

extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

static int oneone;

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size,
          void **unused)
{
  struct MHD_Response *response;
  int ret;
  (void)cls;(void)url;(void)method;(void)version;               /* Unused. Silent compiler warning. */
  (void)upload_data;(void)upload_data_size;(void)unused;        /* Unused. Silent compiler warning. */

  response = MHD_create_response_from_buffer (0, NULL,
					      MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}


static int
testInternalSelectGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLM *multi;
  CURLMcode mret;
  fd_set rs;
  fd_set ws;
  fd_set es;
  int maxposixs; /* Max socket number unused on W32 */
  int running;
  struct CURLMsg *msg;
  time_t start;
  struct timeval tv;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 3000;

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_ERROR_LOG | MHD_USE_TLS | MHD_USE_INTERNAL_POLLING_THREAD,
                        port, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
			MHD_OPTION_END);
  if (d == NULL)
    return 256;

  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      port = (int)dinfo->port;
    }
  char *aes256_sha = "AES256-SHA";
  if (curl_uses_nss_ssl() == 0)
    {
      aes256_sha = "rsa_aes_256_sha";
    }

  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "https://127.0.0.1/hello_world");
  curl_easy_setopt (c, CURLOPT_PORT, (long)port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  /* TLS options */
  curl_easy_setopt (c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);
  curl_easy_setopt (c, CURLOPT_SSL_CIPHER_LIST, aes256_sha);
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);


  multi = curl_multi_init ();
  if (multi == NULL)
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 512;
    }
  mret = curl_multi_add_handle (multi, c);
  if (mret != CURLM_OK)
    {
      curl_multi_cleanup (multi);
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 1024;
    }
  start = time (NULL);
  while ((time (NULL) - start < 5) && (multi != NULL))
    {
      maxposixs = -1;
      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);
      mret = curl_multi_fdset (multi, &rs, &ws, &es, &maxposixs);
      if (mret != CURLM_OK)
        {
          curl_multi_remove_handle (multi, c);
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 2048;
        }
      tv.tv_sec = 0;
      tv.tv_usec = 1000;
      if (-1 != maxposixs)
        {
          if (-1 == select (maxposixs + 1, &rs, &ws, &es, &tv))
            {
#ifdef MHD_POSIX_SOCKETS
              if (EINTR != errno)
                abort ();
#else
              if (WSAEINVAL != WSAGetLastError() || 0 != rs.fd_count || 0 != ws.fd_count || 0 != es.fd_count)
                abort ();
              Sleep (1000);
#endif
            }
        }
      else
        (void)sleep (1);
      curl_multi_perform (multi, &running);
      if (running == 0)
        {
          msg = curl_multi_info_read (multi, &running);
          if (msg == NULL)
            break;
          if (msg->msg == CURLMSG_DONE)
            {
              if (msg->data.result != CURLE_OK)
                printf ("%s failed at %s:%d: `%s'\n",
                        "curl_multi_perform",
                        __FILE__,
                        __LINE__, curl_easy_strerror (msg->data.result));
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              c = NULL;
              multi = NULL;
            }
        }
    }
  if (multi != NULL)
    {
      curl_multi_remove_handle (multi, c);
      curl_easy_cleanup (c);
      curl_multi_cleanup (multi);
    }
  MHD_stop_daemon (d);
  if (cbc.pos != 0)
    return 8192;
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  (void)argc;   /* Unused. Silent compiler warning. */

  if (!testsuite_curl_global_init ())
    return 99;
  if (NULL == curl_version_info (CURLVERSION_NOW)->ssl_version)
    {
      fprintf (stderr, "Curl does not support SSL.  Cannot run the test.\n");
      curl_global_cleanup ();
      return 77;
    }
  if (0 != (errorCount = testInternalSelectGet ()))
    fprintf (stderr, "Failed test: %s, error: %u.\n", argv[0], errorCount);
  curl_global_cleanup ();
  return errorCount != 0 ? 1 : 0;
}
