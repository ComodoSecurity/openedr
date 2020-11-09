/*
     This file is part of libmicrohttpd
     Copyright (C) 2010, 2018 Christian Grothoff

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
 * @file daemontest_digestauth_sha256.c
 * @brief  Testcase for libmicrohttpd Digest Auth with SHA256
 * @author Amr Ali
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef MHD_HTTPS_REQUIRE_GRYPT
#ifdef HAVE_GCRYPT_H
#include <gcrypt.h>
#endif
#endif /* MHD_HTTPS_REQUIRE_GRYPT */

#ifndef WINDOWS
#include <sys/socket.h>
#include <unistd.h>
#else
#include <wincrypt.h>
#endif

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>Access granted</body></html>"

#define DENIED "<html><head><title>libmicrohttpd demo</title></head><body>Access denied</body></html>"

#define MY_OPAQUE "11733b200778ce33060f31c9af70a870ba96ddd4"

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};


static size_t
copyBuffer (void *ptr,
            size_t size,
            size_t nmemb,
            void *ctx)
{
  struct CBC *cbc = ctx;

  if (cbc->pos + size * nmemb > cbc->size)
    return 0;                   /* overflow */
  memcpy (&cbc->buf[cbc->pos], ptr, size * nmemb);
  cbc->pos += size * nmemb;
  return size * nmemb;
}


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data,
          size_t *upload_data_size,
          void **unused)
{
  struct MHD_Response *response;
  char *username;
  const char *password = "testpass";
  const char *realm = "test@example.com";
  int ret;
  (void)cls;(void)url;                          /* Unused. Silent compiler warning. */
  (void)method;(void)version;(void)upload_data; /* Unused. Silent compiler warning. */
  (void)upload_data_size;(void)unused;         /* Unused. Silent compiler warning. */

  username = MHD_digest_auth_get_username (connection);
  if ( (username == NULL) ||
       (0 != strcmp (username, "testuser")) )
    {
      response = MHD_create_response_from_buffer (strlen (DENIED),
                                                  DENIED,
                                                  MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_auth_fail_response2 (connection,
                                           realm,
                                           MY_OPAQUE,
                                           response,
                                           MHD_NO,
                                           MHD_DIGEST_ALG_SHA256);
      MHD_destroy_response(response);
      return ret;
    }
  ret = MHD_digest_auth_check2 (connection,
                                realm,
                                username,
                                password,
                                300,
                                MHD_DIGEST_ALG_SHA256);
  free (username);
  if ( (ret == MHD_INVALID_NONCE) ||
       (ret == MHD_NO) )
    {
      response = MHD_create_response_from_buffer (strlen (DENIED),
                                                  DENIED,
                                                  MHD_RESPMEM_PERSISTENT);
      if (NULL == response)
	return MHD_NO;
      ret = MHD_queue_auth_fail_response2 (connection,
                                           realm,
                                           MY_OPAQUE,
                                           response,
                                           (MHD_INVALID_NONCE == ret) ? MHD_YES : MHD_NO,
                                           MHD_DIGEST_ALG_SHA256);
      MHD_destroy_response(response);
      return ret;
    }
  response = MHD_create_response_from_buffer (strlen(PAGE),
                                              PAGE,
                                              MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);
  return ret;
}


static int
testDigestAuth ()
{
  CURL *c;
  CURLcode errornum;
  struct MHD_Daemon *d;
  struct CBC cbc;
  char buf[2048];
  char rnd[8];
  int port;
  char url[128];
#ifndef WINDOWS
  int fd;
  size_t len;
  size_t off = 0;
#endif /* ! WINDOWS */

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 1165;

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
#ifndef WINDOWS
  fd = open ("/dev/urandom",
             O_RDONLY);
  if (-1 == fd)
    {
      fprintf (stderr,
               "Failed to open `%s': %s\n",
               "/dev/urandom",
               strerror(errno));
      return 1;
    }
  while (off < 8)
    {
      len = read (fd,
                  rnd,
                  8);
      if (len == (size_t)-1)
        {
          fprintf (stderr,
                   "Failed to read `%s': %s\n",
                   "/dev/urandom",
                   strerror(errno));
          (void) close(fd);
          return 1;
        }
      off += len;
    }
  (void) close(fd);
#else
  {
    HCRYPTPROV cc;
    BOOL b;

    b = CryptAcquireContext (&cc,
                             NULL,
                             NULL,
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT);
    if (b == 0)
    {
      fprintf (stderr,
               "Failed to acquire crypto provider context: %lu\n",
               GetLastError ());
      return 1;
    }
    b = CryptGenRandom (cc, 8, (BYTE*)rnd);
    if (b == 0)
    {
      fprintf (stderr,
               "Failed to generate 8 random bytes: %lu\n",
               GetLastError ());
    }
    CryptReleaseContext (cc, 0);
    if (b == 0)
      return 1;
  }
#endif
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL,
                        &ahc_echo, PAGE,
			MHD_OPTION_DIGEST_AUTH_RANDOM, sizeof (rnd), rnd,
			MHD_OPTION_NONCE_NC_SIZE, 300,
			MHD_OPTION_END);
  if (d == NULL)
    return 1;
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;

      dinfo = MHD_get_daemon_info (d,
                                   MHD_DAEMON_INFO_BIND_PORT);
      if ( (NULL == dinfo) ||
           (0 == dinfo->port) )
        {
          MHD_stop_daemon (d);
          return 32;
        }
      port = (int)dinfo->port;
    }
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/bar%%20foo%%3Fkey%%3Dvalue",
            port);
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
  curl_easy_setopt (c, CURLOPT_USERPWD, "testuser:testpass");
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system!*/
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      fprintf (stderr,
               "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 2;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  if (cbc.pos != strlen (PAGE))
    return 4;
  if (0 != strncmp (PAGE, cbc.buf, strlen (PAGE)))
    return 8;
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  curl_version_info_data *d = curl_version_info (CURLVERSION_NOW);
  (void)argc; (void)argv; /* Unused. Silent compiler warning. */

  /* curl added SHA256 support in 7.57 = 7.0x39 */
  if (d->version_num < 0x073900)
    return 77; /* skip test, curl is too old */
#ifdef MHD_HTTPS_REQUIRE_GRYPT
#ifdef HAVE_GCRYPT_H
  gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);
#ifdef GCRYCTL_INITIALIZATION_FINISHED
  gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif
#endif
#endif /* MHD_HTTPS_REQUIRE_GRYPT */
if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  errorCount += testDigestAuth ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
