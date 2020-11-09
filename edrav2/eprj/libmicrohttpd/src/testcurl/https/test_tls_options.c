/*
  This file is part of libmicrohttpd
  Copyright (C) 2007, 2010, 2016 Christian Grothoff

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
 * @file tls_daemon_options_test.c
 * @brief  Testcase for libmicrohttpd HTTPS GET operations
 * @author Sagie Amir
 */

#include "platform.h"
#include "microhttpd.h"
#include <sys/stat.h>
#include <limits.h>
#ifdef MHD_HTTPS_REQUIRE_GRYPT
#include <gcrypt.h>
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
#include "tls_test_common.h"

extern const char srv_key_pem[];
extern const char srv_self_signed_cert_pem[];

int curl_check_version (const char *req_version, ...);

/**
 * test server refuses to negotiate connections with unsupported protocol versions
 *
 */
static int
test_unmatching_ssl_version (void * cls, int port, const char *cipher_suite,
                             int curl_req_ssl_version)
{
  struct CBC cbc;
  (void)cls;    /* Unused. Silent compiler warning. */
  if (NULL == (cbc.buf = malloc (sizeof (char) * 256)))
    {
      fprintf (stderr, "Error: failed to allocate: %s\n",
               strerror (errno));
      return -1;
    }
  cbc.size = 256;
  cbc.pos = 0;

  char url[255];
  if (gen_test_file_url (url,
                         sizeof (url),
                         port))
    {
      free (cbc.buf);
      fprintf (stderr,
               "Internal error in gen_test_file_url\n");
      return -1;
    }

  /* assert daemon *rejected* request */
  if (CURLE_OK ==
      send_curl_req (url, &cbc, cipher_suite, curl_req_ssl_version))
    {
      free (cbc.buf);
      fprintf (stderr, "cURL failed to reject request despite SSL version mismatch!\n");
      return -1;
    }

  free (cbc.buf);
  return 0;
}


/* setup a temporary transfer test file */
int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  const char *ssl_version;
  int daemon_flags =
    MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS | MHD_USE_ERROR_LOG;
  int port;
  (void)argc; (void)argv;       /* Unused. Silent compiler warning. */

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 3010;

#ifdef MHD_HTTPS_REQUIRE_GRYPT
  gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
  gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);
#ifdef GCRYCTL_INITIALIZATION_FINISHED
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
 if (curl_check_version (MHD_REQ_CURL_VERSION))
    {
      return 77;
    }
  ssl_version = curl_version_info (CURLVERSION_NOW)->ssl_version;
  if (NULL == ssl_version)
  {
    fprintf (stderr, "Curl does not support SSL.  Cannot run the test.\n");
    return 77;
  }
  if (0 != strncmp (ssl_version, "GnuTLS", 6))
  {
    fprintf (stderr, "This test can be run only with libcurl-gnutls.\n");
    return 77;
  }

  if (!testsuite_curl_global_init ())
    return 99;

  const char *aes128_sha = "AES128-SHA";
  const char *aes256_sha = "AES256-SHA";
  if (curl_uses_nss_ssl() == 0)
    {
      aes128_sha = "rsa_aes_128_sha";
      aes256_sha = "rsa_aes_256_sha";
    }


  if (0 !=
    test_wrap ("TLS1.0-AES-SHA1",
	       &test_https_transfer, NULL, port, daemon_flags,
	       aes128_sha,
	       CURL_SSLVERSION_TLSv1,
	       MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
	       MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
	       MHD_OPTION_HTTPS_PRIORITIES, "NONE:+VERS-TLS1.0:+AES-128-CBC:+SHA1:+RSA:+COMP-NULL",
	       MHD_OPTION_END))
    {
      fprintf (stderr, "TLS1.0-AES-SHA1 test failed\n");
      errorCount++;
    }
  fprintf (stderr,
	   "The following handshake should fail (and print an error message)...\n");
  if (0 !=
    test_wrap ("TLS1.0 vs SSL3",
	       &test_unmatching_ssl_version, NULL, port, daemon_flags,
	       aes256_sha,
	       CURL_SSLVERSION_SSLv3,
	       MHD_OPTION_HTTPS_MEM_KEY, srv_key_pem,
	       MHD_OPTION_HTTPS_MEM_CERT, srv_self_signed_cert_pem,
	       MHD_OPTION_HTTPS_PRIORITIES, "NONE:+VERS-TLS1.0:+AES-256-CBC:+SHA1:+RSA:+COMP-NULL",
	       MHD_OPTION_END))
    {
      fprintf (stderr, "TLS1.0 vs SSL3 test failed\n");
      errorCount++;
    }
  curl_global_cleanup ();

  return errorCount != 0 ? 1 : 0;
}
