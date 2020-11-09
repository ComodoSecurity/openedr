/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2008 Christian Grothoff

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
 * @file test_large_put.c
 * @brief  Testcase for libmicrohttpd PUT operations
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef WINDOWS
#include <unistd.h>
#endif

#include "test_helpers.h"

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

static int oneone;
static int incr_read; /* Use incremental read */
static int verbose; /* Be verbose */

#define PUT_SIZE (256 * 1024)

static char *put_buffer;

struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};

char*
alloc_init(size_t buf_size)
{
  static const char template[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_`abcdefghijklmnopqrstuvwxyz";
  static const size_t templ_size = sizeof(template) / sizeof(char) - 1;
  char *buf;
  char *fill_ptr;
  size_t to_fill;

  buf = malloc(buf_size);
  if (NULL == buf)
    return NULL;

  fill_ptr = buf;
  to_fill = buf_size;
  while (to_fill > 0)
    {
      const size_t to_copy = to_fill > templ_size ? templ_size : to_fill;
      memcpy (fill_ptr, template, to_copy);
      fill_ptr += to_copy;
      to_fill -= to_copy;
    }
  return buf;
}

static size_t
putBuffer (void *stream, size_t size, size_t nmemb, void *ptr)
{
  size_t *pos = (size_t *)ptr;
  size_t wrt;

  wrt = size * nmemb;
  /* Check for overflow. */
  if (wrt / size != nmemb)
    return 0;
  if (wrt > PUT_SIZE - (*pos))
    wrt = PUT_SIZE - (*pos);
  memcpy (stream, &put_buffer[*pos], wrt);
  (*pos) += wrt;
  return wrt;
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
          void **pparam)
{
  int *done = cls;
  struct MHD_Response *response;
  int ret;
  static size_t processed;
  (void)version;        /* Unused. Silent compiler warning. */

  if (0 != strcmp ("PUT", method))
    return MHD_NO;              /* unexpected method */
  if ((*done) == 0)
    {
      size_t *pproc;
      if (NULL == *pparam)
        {
          processed = 0;
          *pparam = &processed; /* Safe as long as only one parallel request served. */
        }
      pproc = (size_t*) *pparam;

      if (0 == *upload_data_size)
        return MHD_YES; /* No data to process. */

      if (*pproc + *upload_data_size > PUT_SIZE)
        {
          fprintf (stderr, "Incoming data larger than expected.\n");
          return MHD_NO;
        }
      if ( (!incr_read) && (*upload_data_size != PUT_SIZE) )
        return MHD_YES; /* Wait until whole request is received. */

      if (0 != memcmp(upload_data, put_buffer + (*pproc), *upload_data_size))
        {
          fprintf (stderr, "Incoming data does not match sent data.\n");
          return MHD_NO;
        }
      *pproc += *upload_data_size;
      *upload_data_size = 0; /* Current block of data is fully processed. */

      if (PUT_SIZE == *pproc)
        *done = 1; /* Whole request is processed. */
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
testPutInternalThread (unsigned int add_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  struct CBC cbc;
  size_t pos = 0;
  int done_flag = 0;
  CURLcode errornum;
  char buf[2048];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1270;
      if (oneone)
        port += 10;
      if (incr_read)
        port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | add_flag,
                        port,
                        NULL, NULL, &ahc_echo, &done_flag,
			MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t)(incr_read ? 1024 : (PUT_SIZE * 4)),
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
  curl_easy_setopt (c, CURLOPT_INFILESIZE, (long) PUT_SIZE);
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
      fprintf (stderr,
               "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
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
testPutThreadPerConn (unsigned int add_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  struct CBC cbc;
  size_t pos = 0;
  int done_flag = 0;
  CURLcode errornum;
  char buf[2048];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1271;
      if (oneone)
        port += 10;
      if (incr_read)
        port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD |
                          MHD_USE_ERROR_LOG | add_flag,
                        port,
                        NULL, NULL, &ahc_echo, &done_flag,
                        MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t)(incr_read ? 1024 : (PUT_SIZE * 4)),
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
  curl_easy_setopt (c, CURLOPT_READFUNCTION, &putBuffer);
  curl_easy_setopt (c, CURLOPT_READDATA, &pos);
  curl_easy_setopt (c, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt (c, CURLOPT_INFILESIZE, (long) PUT_SIZE);
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
      fprintf (stderr,
               "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 32;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    {
      fprintf (stderr, "Got invalid response `%.*s'\n", (int)cbc.pos, cbc.buf);
      return 64;
    }
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 128;
  return 0;
}

static int
testPutThreadPool (unsigned int add_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  struct CBC cbc;
  size_t pos = 0;
  int done_flag = 0;
  CURLcode errornum;
  char buf[2048];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1272;
      if (oneone)
        port += 10;
      if (incr_read)
        port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | add_flag,
                        port,
                        NULL, NULL, &ahc_echo, &done_flag,
                        MHD_OPTION_THREAD_POOL_SIZE, CPU_COUNT,
                        MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t)(incr_read ? 1024 : (PUT_SIZE * 4)),
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
  curl_easy_setopt (c, CURLOPT_READFUNCTION, &putBuffer);
  curl_easy_setopt (c, CURLOPT_READDATA, &pos);
  curl_easy_setopt (c, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt (c, CURLOPT_INFILESIZE, (long) PUT_SIZE);
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
      fprintf (stderr,
               "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 32;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    {
      fprintf (stderr, "Got invalid response `%.*s'\n", (int)cbc.pos, cbc.buf);
      return 64;
    }
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 128;
  return 0;
}

static int
testPutExternal (void)
{
  struct MHD_Daemon *d;
  CURL *c;
  struct CBC cbc;
  CURLM *multi;
  CURLMcode mret;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket maxsock;
#ifdef MHD_WINSOCK_SOCKETS
  int maxposixs; /* Max socket number unused on W32 */
#else  /* MHD_POSIX_SOCKETS */
#define maxposixs maxsock
#endif /* MHD_POSIX_SOCKETS */
  int running;
  struct CURLMsg *msg;
  time_t start;
  struct timeval tv;
  size_t pos = 0;
  int done_flag = 0;
  char buf[2048];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1273;
      if (oneone)
        port += 10;
      if (incr_read)
        port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  multi = NULL;
  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        port,
                        NULL, NULL, &ahc_echo, &done_flag,
                        MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t)(incr_read ? 1024 : (PUT_SIZE * 4)),
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
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1/hello_world");
  curl_easy_setopt (c, CURLOPT_PORT, (long)port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_READFUNCTION, &putBuffer);
  curl_easy_setopt (c, CURLOPT_READDATA, &pos);
  curl_easy_setopt (c, CURLOPT_UPLOAD, 1L);
  curl_easy_setopt (c, CURLOPT_INFILESIZE, (long) PUT_SIZE);
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
      maxsock = MHD_INVALID_SOCKET;
      maxposixs = -1;
      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);
      curl_multi_perform (multi, &running);
      mret = curl_multi_fdset (multi, &rs, &ws, &es, &maxposixs);
      if (mret != CURLM_OK)
        {
          curl_multi_remove_handle (multi, c);
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 2048;
        }
      if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &maxsock))
        {
          curl_multi_remove_handle (multi, c);
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          MHD_stop_daemon (d);
          return 4096;
        }
      tv.tv_sec = 0;
      tv.tv_usec = 1000;
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
      MHD_run (d);
    }
  if (multi != NULL)
    {
      curl_multi_remove_handle (multi, c);
      curl_easy_cleanup (c);
      curl_multi_cleanup (multi);
    }
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    {
      fprintf (stderr, "Got invalid response `%.*s'\n", (int)cbc.pos, cbc.buf);
      return 8192;
    }
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 16384;
  return 0;
}



int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  unsigned int lastErr;

  oneone = has_in_name(argv[0], "11");
  incr_read = has_in_name(argv[0], "_inc");
  verbose = has_param(argc, argv, "-v");
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 99;
  put_buffer = alloc_init (PUT_SIZE);
  if (NULL == put_buffer)
    return 99;
  lastErr = testPutExternal ();
  if (verbose && 0 != lastErr)
    fprintf (stderr, "Error during testing with external select().\n");
  errorCount += lastErr;
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_THREADS))
    {
      lastErr = testPutInternalThread (0);
      if (verbose && 0 != lastErr)
	fprintf (stderr, "Error during testing with internal thread with select().\n");
      errorCount += lastErr;
      lastErr = testPutThreadPerConn (0);
      if (verbose && 0 != lastErr)
	fprintf (stderr, "Error during testing with internal thread per connection with select().\n");
      errorCount += lastErr;
      lastErr = testPutThreadPool (0);
      if (verbose && 0 != lastErr)
	fprintf (stderr, "Error during testing with thread pool per connection with select().\n");
      errorCount += lastErr;
      if (MHD_is_feature_supported(MHD_FEATURE_POLL))
	{
	  lastErr = testPutInternalThread (MHD_USE_POLL);
	  if (verbose && 0 != lastErr)
	    fprintf (stderr, "Error during testing with internal thread with poll().\n");
	  errorCount += lastErr;
	  lastErr = testPutThreadPerConn (MHD_USE_POLL);
	  if (verbose && 0 != lastErr)
	    fprintf (stderr, "Error during testing with internal thread per connection with poll().\n");
	  errorCount += lastErr;
	  lastErr = testPutThreadPool (MHD_USE_POLL);
	  if (verbose && 0 != lastErr)
	    fprintf (stderr, "Error during testing with thread pool per connection with poll().\n");
	  errorCount += lastErr;
	}
      if (MHD_is_feature_supported(MHD_FEATURE_EPOLL))
	{
	  lastErr = testPutInternalThread (MHD_USE_EPOLL);
	  if (verbose && 0 != lastErr)
	    fprintf (stderr, "Error during testing with internal thread with epoll.\n");
	  errorCount += lastErr;
	  lastErr = testPutThreadPool (MHD_USE_EPOLL);
	  if (verbose && 0 != lastErr)
	    fprintf (stderr, "Error during testing with thread pool per connection with epoll.\n");
	  errorCount += lastErr;
	}
    }
  free (put_buffer);
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  else if (verbose)
    printf ("All checks passed successfully.\n");
  curl_global_cleanup ();
  return (errorCount == 0) ? 0 : 1;
}
