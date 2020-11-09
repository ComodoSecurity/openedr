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

/**
 * @file test_iplimit.c
 * @brief  Testcase for libmicrohttpd GET operations
 *         TODO: test parsing of query
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mhd_has_in_name.h"

#ifndef WINDOWS
#include <unistd.h>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /* !WIN32_LEAN_AND_MEAN */
#include <windows.h>
#endif

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

static int oneone;

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};

static size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
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
          const char *upload_data, size_t *upload_data_size,
          void **unused)
{
  static int ptr;
  const char *me = cls;
  struct MHD_Response *response;
  int ret;
  (void)version;(void)upload_data;(void)upload_data_size;       /* Unused. Silent compiler warning. */

  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  response = MHD_create_response_from_buffer (strlen (url),
					      (void *) url,
					      MHD_RESPMEM_MUST_COPY);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  if (ret == MHD_NO)
    abort ();
  return ret;
}

static int
testMultithreadedGet ()
{
  struct MHD_Daemon *d;
  char buf[2048];
  int k;
  unsigned int success;
  unsigned int failure;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1260;
      if (oneone)
        port += 5;
    }

  /* Test only valid for HTTP/1.1 (uses persistent connections) */
  if (!oneone)
    return 0;

  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL,
                        &ahc_echo, "GET",
                        MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
                        MHD_OPTION_END);
  if (d == NULL)
    return 16;
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      port = (int)dinfo->port;
    }

  for (k = 0; k < 3; ++k)
    {
      struct CBC cbc[3];
      CURL *cenv[3];
      int i;

      success = 0;
      failure = 0;
      for (i = 0; i < 3; ++i)
        {
          CURL *c;
          CURLcode errornum;

          cenv[i] = c = curl_easy_init ();
          cbc[i].buf = buf;
          cbc[i].size = 2048;
          cbc[i].pos = 0;

          curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1/hello_world");
          curl_easy_setopt (c, CURLOPT_PORT, (long)port);
          curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
          curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc[i]);
          curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
          curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
          curl_easy_setopt (c, CURLOPT_FORBID_REUSE, 0L);
          curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
          curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
          /* NOTE: use of CONNECTTIMEOUT without also
           *   setting NOSIGNAL results in really weird
           *   crashes on my system! */
          curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);

          errornum = curl_easy_perform (c);
          if (CURLE_OK == errornum)
            success++;
          else
            failure++;
        }

      /* Cleanup the environments */
      for (i = 0; i < 3; ++i)
        curl_easy_cleanup (cenv[i]);
      if ( (2 != success) ||
           (1 != failure) )
      {
        fprintf (stderr,
                 "Unexpected number of success (%u) or failure (%u)\n",
                 success,
                 failure);
        MHD_stop_daemon (d);
        return 32;
      }

      (void)sleep(2);

      for (i = 0; i < 2; ++i)
        {
          if (cbc[i].pos != strlen ("/hello_world"))
            {
              MHD_stop_daemon (d);
              return 64;
            }
          if (0 != strncmp ("/hello_world", cbc[i].buf, strlen ("/hello_world")))
            {
              MHD_stop_daemon (d);
              return 128;
            }
        }
    }
  MHD_stop_daemon (d);
  return 0;
}

static int
testMultithreadedPoolGet ()
{
  struct MHD_Daemon *d;
  char buf[2048];
  int k;
  int port;
  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1261;
      if (oneone)
        port += 5;
    }

  /* Test only valid for HTTP/1.1 (uses persistent connections) */
  if (!oneone)
    return 0;

  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_PER_IP_CONNECTION_LIMIT, 2,
                        MHD_OPTION_THREAD_POOL_SIZE, CPU_COUNT,
                        MHD_OPTION_END);
  if (d == NULL)
    return 16;
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      port = (int)dinfo->port;
    }

  for (k = 0; k < 3; ++k)
    {
      struct CBC cbc[3];
      CURL *cenv[3];
      int i;

      for (i = 0; i < 3; ++i)
        {
          CURL *c;
          CURLcode errornum;

          cenv[i] = c = curl_easy_init ();
          cbc[i].buf = buf;
          cbc[i].size = 2048;
          cbc[i].pos = 0;

          curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1/hello_world");
          curl_easy_setopt (c, CURLOPT_PORT, (long)port);
          curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
          curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc[i]);
          curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
          curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
          curl_easy_setopt (c, CURLOPT_FORBID_REUSE, 0L);
          curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
          curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
          /* NOTE: use of CONNECTTIMEOUT without also
           *   setting NOSIGNAL results in really weird
           *   crashes on my system! */
          curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);

          errornum = curl_easy_perform (c);
          if ( ( (CURLE_OK != errornum) && (i <  2) ) ||
	       ( (CURLE_OK == errornum) && (i == 2) ) )
            {
              int j;

              /* First 2 should succeed */
              if (i < 2)
                fprintf (stderr,
                         "curl_easy_perform failed: `%s'\n",
                         curl_easy_strerror (errornum));

              /* Last request should have failed */
              else
                fprintf (stderr,
                         "No error on IP address over limit\n");

              for (j = 0; j < i; ++j)
                curl_easy_cleanup (cenv[j]);
              MHD_stop_daemon (d);
              return 32;
            }
        }

      /* Cleanup the environments */
      for (i = 0; i < 3; ++i)
        curl_easy_cleanup (cenv[i]);

      (void)sleep(2);

      for (i = 0; i < 2; ++i)
        {
          if (cbc[i].pos != strlen ("/hello_world"))
            {
              MHD_stop_daemon (d);
              return 64;
            }
          if (0 != strncmp ("/hello_world", cbc[i].buf, strlen ("/hello_world")))
            {
              MHD_stop_daemon (d);
              return 128;
            }
        }


    }
  MHD_stop_daemon (d);
  return 0;
}

int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  (void)argc;   /* Unused. Silent compiler warning. */

  if (NULL == argv || 0 == argv[0])
    return 99;
  oneone = has_in_name (argv[0], "11");
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  errorCount |= testMultithreadedGet ();
  errorCount |= testMultithreadedPoolGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
