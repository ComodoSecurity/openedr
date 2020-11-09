/*
  This file is part of libmicrohttpd
  Copyright (C) 2007 Christian Grothoff

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
 * @file test_https_get.c
 * @brief  Testcase for libmicrohttpd HTTPS GET operations
 * @author Sagie Amir
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

extern const char srv_signed_cert_pem[];
extern const char srv_signed_key_pem[];

/* perform a HTTP GET request via SSL/TLS */
static int
test_secure_get (FILE * test_fd,
		 const char *cipher_suite,
		 int proto_version)
{
  int ret;
  struct MHD_Daemon *d;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 3041;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS |
                        MHD_USE_ERROR_LOG, port,
                        NULL, NULL, &http_ahc, NULL,
                        MHD_OPTION_HTTPS_MEM_KEY, srv_signed_key_pem,
                        MHD_OPTION_HTTPS_MEM_CERT, srv_signed_cert_pem,
                        MHD_OPTION_END);

  if (d == NULL)
    {
      fprintf (stderr, MHD_E_SERVER_INIT);
      return -1;
    }
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return -1; }
      port = (int)dinfo->port;
    }

  ret = test_https_transfer (test_fd, port, cipher_suite, proto_version);

  MHD_stop_daemon (d);
  return ret;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  const char *aes256_sha_tlsv1   = "AES256-SHA";
  (void)argc;   /* Unused. Silent compiler warning. */

#ifdef MHD_HTTPS_REQUIRE_GRYPT
  gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);
#ifdef GCRYCTL_INITIALIZATION_FINISHED
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
  if (!testsuite_curl_global_init ())
    return 99;
  if (NULL == curl_version_info (CURLVERSION_NOW)->ssl_version)
    {
      fprintf (stderr, "Curl does not support SSL.  Cannot run the test.\n");
      curl_global_cleanup ();
      return 77;
    }

  if (curl_uses_nss_ssl() == 0)
    {
      aes256_sha_tlsv1 = "rsa_aes_256_sha";
    }

  errorCount +=
    test_secure_get (NULL, aes256_sha_tlsv1, CURL_SSLVERSION_TLSv1);
  print_test_result (errorCount, argv[0]);

  curl_global_cleanup ();

  return errorCount != 0 ? 1 : 0;
}
