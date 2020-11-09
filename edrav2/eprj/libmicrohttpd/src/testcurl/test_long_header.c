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
 * @file test_long_header.c
 * @brief  Testcase for libmicrohttpd handling of very long headers
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

/**
 * We will set the memory available per connection to
 * half of this value, so the actual value does not have
 * to be big at all...
 */
#define VERY_LONG (1024*10)

static int oneone;

static int
apc_all (void *cls, const struct sockaddr *addr, socklen_t addrlen)
{
  (void)cls;(void)addr;(void)addrlen;   /* Unused. Silent compiler warning. */
  return MHD_YES;
}

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};

static size_t
copyBuffer (void *ptr, size_t size, size_t nmemb, void *ctx)
{
  (void)ptr;(void)ctx;  /* Unused. Silent compiler warning. */
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
  const char *me = cls;
  struct MHD_Response *response;
  int ret;
  (void)version;(void)upload_data;      /* Unused. Silent compiler warning. */
  (void)upload_data_size;(void)unused;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  response = MHD_create_response_from_buffer (strlen (url),
					      (void *) url,
					      MHD_RESPMEM_MUST_COPY);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}


static int
testLongUrlGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  char *url;
  long code;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1330;
      if (oneone)
        port += 5;
    }
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD /* | MHD_USE_ERROR_LOG */ ,
                        port,
                        &apc_all,
                        NULL,
                        &ahc_echo,
                        "GET",
                        MHD_OPTION_CONNECTION_MEMORY_LIMIT,
                        (size_t) (VERY_LONG / 2), MHD_OPTION_END);
  if (d == NULL)
    return 1;
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      port = (int)dinfo->port;
    }
  c = curl_easy_init ();
  url = malloc (VERY_LONG);
  if (url == NULL)
    {
	MHD_stop_daemon (d);
 	return 1;
    }
  memset (url, 'a', VERY_LONG);
  url[VERY_LONG - 1] = '\0';
  memcpy (url, "http://127.0.0.1/", strlen ("http://127.0.0.1/"));
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_PORT, (long)port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  if (CURLE_OK == curl_easy_perform (c))
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      free (url);
      return 2;
    }
  if (CURLE_OK != curl_easy_getinfo (c, CURLINFO_RESPONSE_CODE, &code))
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      free (url);
      return 4;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  free (url);
  if (code != MHD_HTTP_URI_TOO_LONG)
    return 8;
  return 0;
}


static int
testLongHeaderGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  char *url;
  long code;
  struct curl_slist *header = NULL;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1331;
      if (oneone)
        port += 5;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD /* | MHD_USE_ERROR_LOG */ ,
                        port,
                        &apc_all,
                        NULL,
                        &ahc_echo,
                        "GET",
                        MHD_OPTION_CONNECTION_MEMORY_LIMIT,
                        (size_t) (VERY_LONG / 2), MHD_OPTION_END);
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
  c = curl_easy_init ();
  url = malloc (VERY_LONG);
  if (url == NULL)
     {
	MHD_stop_daemon (d);
	return 16;
     }
  memset (url, 'a', VERY_LONG);
  url[VERY_LONG - 1] = '\0';
  url[VERY_LONG / 2] = ':';
  url[VERY_LONG / 2 + 1] = ' ';
  header = curl_slist_append (header, url);

  curl_easy_setopt (c, CURLOPT_HTTPHEADER, header);
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1/hello_world");
  curl_easy_setopt (c, CURLOPT_PORT, (long)port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  if (CURLE_OK == curl_easy_perform (c))
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      curl_slist_free_all (header);
      free (url);
      return 32;
    }
  if (CURLE_OK != curl_easy_getinfo (c, CURLINFO_RESPONSE_CODE, &code))
    {
      curl_slist_free_all (header);
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      free (url);
      return 64;
    }
  curl_slist_free_all (header);
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  free (url);
  if (code != MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE)
    return 128;
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
  errorCount += testLongUrlGet ();
  errorCount += testLongHeaderGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
