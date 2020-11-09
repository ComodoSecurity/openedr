/*
 This file is part of libmicrohttpd
 Copyright (C) 2007, 2016 Christian Grothoff

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

/**
 * @file mhds_session_info_test.c
 * @brief  Testcase for libmicrohttpd HTTPS connection querying operations
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include <curl/curl.h>
#ifdef MHD_HTTPS_REQUIRE_GRYPT
#include <gcrypt.h>
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
#include "tls_test_common.h"

extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

struct MHD_Daemon *d;

/*
 * HTTP access handler call back
 * used to query negotiated security parameters
 */
static int
query_session_ahc (void *cls, struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version, const char *upload_data,
                   size_t *upload_data_size, void **ptr)
{
  struct MHD_Response *response;
  int ret;
  (void)cls;(void)url;(void)method;(void)version;       /* Unused. Silent compiler warning. */
  (void)upload_data;(void)upload_data_size;             /* Unused. Silent compiler warning. */

  if (NULL == *ptr)
    {
      *ptr = (void*)&query_session_ahc;
      return MHD_YES;
    }

  if (GNUTLS_TLS1_1 !=
      (ret = MHD_get_connection_info
       (connection,
	MHD_CONNECTION_INFO_PROTOCOL)->protocol))
    {
      if (GNUTLS_TLS1_2 == ret)
      {
        /* as usual, TLS implementations sometimes don't
           quite do what was asked, just mildly complain... */
        fprintf (stderr,
                 "Warning: requested TLS 1.1, got TLS 1.2\n");
      }
      else
      {
        /* really different version... */
        fprintf (stderr,
                 "Error: requested protocol mismatch (wanted %d, got %d)\n",
                 GNUTLS_TLS1_1,
                 ret);
        return -1;
      }
    }

  response = MHD_create_response_from_buffer (strlen (EMPTY_PAGE),
					      (void *) EMPTY_PAGE,
					      MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}


/**
 * negotiate a secure connection with server & query negotiated security parameters
 */
#if LIBCURL_VERSION_NUM >= 0x072200
static int
test_query_session ()
{
  CURL *c;
  struct CBC cbc;
  CURLcode errornum;
  char url[256];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 3060;

  if (NULL == (cbc.buf = malloc (sizeof (char) * 255)))
    return 16;
  cbc.size = 255;
  cbc.pos = 0;

  /* setup test */
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS |
                        MHD_USE_ERROR_LOG, port,
                        NULL, NULL, &query_session_ahc, NULL,
			MHD_OPTION_HTTPS_PRIORITIES, "NORMAL:+ARCFOUR-128",
                        MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    {
      free (cbc.buf);
      return 2;
    }
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        {
          MHD_stop_daemon (d);
          free (cbc.buf);
          return 32;
        }
      port = (int)dinfo->port;
    }

  const char *aes256_sha = "AES256-SHA";
  if (curl_uses_nss_ssl() == 0)
    {
      aes256_sha = "rsa_aes_256_sha";
    }

  gen_test_file_url (url,
                     sizeof (url),
                     port);
  c = curl_easy_init ();
#if DEBUG_HTTPS_TEST
  curl_easy_setopt (c, CURLOPT_VERBOSE, 1L);
#endif
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 10L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_FILE, &cbc);
  /* TLS options */
  curl_easy_setopt (c, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_1);
  curl_easy_setopt (c, CURLOPT_SSL_CIPHER_LIST, aes256_sha);
  /* currently skip any peer authentication */
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt (c, CURLOPT_SSL_VERIFYHOST, 0L);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);

  /* NOTE: use of CONNECTTIMEOUT without also
   * setting NOSIGNAL results in really weird
   * crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      fprintf (stderr, "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));

      MHD_stop_daemon (d);
      curl_easy_cleanup (c);
      free (cbc.buf);
      return -1;
    }

  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  free (cbc.buf);
  return 0;
}
#endif

int
main (int argc, char *const *argv)
{
#if LIBCURL_VERSION_NUM >= 0x072200
  unsigned int errorCount = 0;
  const char *ssl_version;
  (void)argc;   /* Unused. Silent compiler warning. */

#ifdef MHD_HTTPS_REQUIRE_GRYPT
  gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);
#ifdef GCRYCTL_INITIALIZATION_FINISHED
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
  if (!testsuite_curl_global_init ())
    return 99;

  ssl_version = curl_version_info (CURLVERSION_NOW)->ssl_version;
  if (NULL == ssl_version)
  {
    fprintf (stderr, "Curl does not support SSL.  Cannot run the test.\n");
    curl_global_cleanup ();
    return 77;
  }
  if (0 != strncmp (ssl_version, "GnuTLS", 6))
  {
    fprintf (stderr, "This test can be run only with libcurl-gnutls.\n");
    curl_global_cleanup ();
    return 77;
  }
  errorCount += test_query_session ();
  print_test_result (errorCount, argv[0]);
  curl_global_cleanup ();
  return errorCount != 0 ? 1 : 0;
#else  /* LIBCURL_VERSION_NUM < 0x072200 */
  return 77;
#endif /* LIBCURL_VERSION_NUM < 0x072200 */
}
