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
 * @file test_timeout.c
 * @brief  Testcase for libmicrohttpd PUT operations
 * @author Matthias Wachs
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

static int oneone;

static int withTimeout = 1;

static int withoutTimeout = 1;

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};


static void 
termination_cb (void *cls,
		struct MHD_Connection *connection,
		void **con_cls,
		enum MHD_RequestTerminationCode toe)
{
  int *test = cls;
  (void)connection;(void)con_cls;       /* Unused. Silent compiler warning. */

  switch (toe)
    {
    case MHD_REQUEST_TERMINATED_COMPLETED_OK :
      if (test == &withoutTimeout)
	{
	  withoutTimeout = 0;
	}
      break;
    case MHD_REQUEST_TERMINATED_WITH_ERROR :
    case MHD_REQUEST_TERMINATED_READ_ERROR :
      break;
    case MHD_REQUEST_TERMINATED_TIMEOUT_REACHED :
      if (test == &withTimeout)
	{
	  withTimeout = 0;
	}
      break;
    case MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN:
      break;
    case MHD_REQUEST_TERMINATED_CLIENT_ABORT:
      break;
    }
}


static size_t
putBuffer (void *stream, size_t size, size_t nmemb, void *ptr)
{
  unsigned int *pos = ptr;
  unsigned int wrt;

  wrt = size * nmemb;
  if (wrt > 8 - (*pos))
	wrt = 8 - (*pos);
  memcpy (stream, &("Hello123"[*pos]), wrt);
  (*pos) += wrt;
  return wrt;
}


static size_t
putBuffer_fail (void *stream, size_t size, size_t nmemb, void *ptr)
{
  (void)stream;(void)size;(void)nmemb;(void)ptr;        /* Unused. Silent compiler warning. */
  return 0;
}


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
  int *done = cls;
  struct MHD_Response *response;
  int ret;
  (void)version;(void)unused;   /* Unused. Silent compiler warning. */

  if (0 != strcmp ("PUT", method))
    return MHD_NO;              /* unexpected method */
  if ((*done) == 0)
    {
      if (*upload_data_size != 8)
        return MHD_YES;         /* not yet ready */
      if (0 == memcmp (upload_data, "Hello123", 8))
        {
          *upload_data_size = 0;
        }
      else
        {
          printf ("Invalid upload data `%8s'!\n", upload_data);
          return MHD_NO;
        }
      *done = 1;
      return MHD_YES;
    }
  response = MHD_create_response_from_buffer (strlen (url),
					      (void *) url, 
					      MHD_RESPMEM_MUST_COPY);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}


static int
testWithoutTimeout ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  unsigned int pos = 0;
  int done_flag = 0;
  CURLcode errornum;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1500;
      if (oneone)
        port += 5;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port,
                        NULL, NULL, &ahc_echo, &done_flag,
                        MHD_OPTION_CONNECTION_TIMEOUT, 2,
                        MHD_OPTION_NOTIFY_COMPLETED, &termination_cb, &withTimeout,
                        MHD_OPTION_END);
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
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1/hello_world");
  curl_easy_setopt (c, CURLOPT_PORT, (long)port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_READFUNCTION, &putBuffer);
  curl_easy_setopt (c, CURLOPT_READDATA, &pos);
  curl_easy_setopt (c, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt (c, CURLOPT_INFILESIZE_LARGE, (curl_off_t) 8L);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  /* NOTE: use of CONNECTTIMEOUT without also
   *   setting NOSIGNAL results in really weird
   *   crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 2;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 4;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 8;
  return 0;
}

static int
testWithTimeout ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  int done_flag = 0;
  CURLcode errornum;
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1501;
      if (oneone)
        port += 5;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port,
                        NULL, NULL, &ahc_echo, &done_flag,
                        MHD_OPTION_CONNECTION_TIMEOUT, 2,
                        MHD_OPTION_NOTIFY_COMPLETED, &termination_cb, &withoutTimeout,
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
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1/hello_world");
  curl_easy_setopt (c, CURLOPT_PORT, (long)port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_READFUNCTION, &putBuffer_fail);
  curl_easy_setopt (c, CURLOPT_READDATA, &testWithTimeout);
  curl_easy_setopt (c, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt (c, CURLOPT_INFILESIZE_LARGE, (curl_off_t) 8L);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  /* NOTE: use of CONNECTTIMEOUT without also
   *   setting NOSIGNAL results in really weird
   *   crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      if (errornum == CURLE_GOT_NOTHING)
    	  /* mhd had the timeout */
    	  return 0;
      else
    	  /* curl had the timeout first */
    	  return 32;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  return 64;
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
    return 16;
  errorCount += testWithoutTimeout ();
  errorCount += testWithTimeout ();
  if (errorCount != 0)
    fprintf (stderr, 
	     "Error during test execution (code: %u)\n",
	     errorCount);
  curl_global_cleanup ();
  if ((withTimeout == 0) && (withoutTimeout == 0))
    return 0;
  else
    return errorCount;       /* 0 == pass */
}
