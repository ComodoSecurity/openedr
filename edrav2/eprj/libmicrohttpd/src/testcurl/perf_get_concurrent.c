/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2009, 2011 Christian Grothoff

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
 * @file perf_get_concurrent.c
 * @brief benchmark concurrent GET operations
 *        Note that we run libcurl on the machine at the
 *        same time, so the execution time may be influenced
 *        by the concurrent activity; it is quite possible
 *        that more time is spend with libcurl than with MHD,
 *        so the performance scores calculated with this code
 *        should NOT be used to compare with other HTTP servers
 *        (since MHD is actually better); only the relative
 *        scores between MHD versions are meaningful.
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "gauger.h"
#include "mhd_has_in_name.h"

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

/**
 * How many rounds of operations do we do for each
 * test (total number of requests will be ROUNDS * PAR).
 * Ensure that free ports are not exhausted during test.
 */
#if CPU_COUNT > 8
#define ROUNDS (1 + (30000 / 12) / CPU_COUNT)
#else
#define ROUNDS 500
#endif

/**
 * How many requests do we do in parallel?
 */
#define PAR CPU_COUNT

/**
 * Do we use HTTP 1.1?
 */
static int oneone;

/**
 * Response to return (re-used).
 */
static struct MHD_Response *response;

/**
 * Time this round was started.
 */
static unsigned long long start_time;

/**
 * Set to 1 if the worker threads are done.
 */
static volatile int signal_done;


/**
 * Get the current timestamp
 *
 * @return current time in ms
 */
static unsigned long long
now ()
{
  struct timeval tv;

  gettimeofday (&tv, NULL);
  return (((unsigned long long) tv.tv_sec * 1000LL) +
	  ((unsigned long long) tv.tv_usec / 1000LL));
}


/**
 * Start the timer.
 */
static void
start_timer()
{
  start_time = now ();
}


/**
 * Stop the timer and report performance
 *
 * @param desc description of the threading mode we used
 */
static void
stop (const char *desc)
{
  double rps = ((double) (PAR * ROUNDS * 1000)) / ((double) (now() - start_time));

  fprintf (stderr,
	   "Parallel GETs using %s: %f %s\n",
	   desc,
	   rps,
	   "requests/s");
  GAUGER (desc,
	  "Parallel GETs",
	  rps,
	  "requests/s");
}


static size_t
copyBuffer (void *ptr,
	    size_t size, size_t nmemb,
	    void *ctx)
{
  (void)ptr;(void)ctx;          /* Unused. Silent compiler warning. */
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
  int ret;
  (void)url;(void)version;                      /* Unused. Silent compiler warning. */
  (void)upload_data;(void)upload_data_size;     /* Unused. Silent compiler warning. */

  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  if (ret == MHD_NO)
    abort ();
  return ret;
}


static void *
thread_gets (void *param)
{
  CURL *c;
  CURLcode errornum;
  unsigned int i;
  char * const url = (char*) param;

  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  for (i=0;i<ROUNDS;i++)
    {
      if (CURLE_OK != (errornum = curl_easy_perform (c)))
        {
          fprintf (stderr,
                   "curl_easy_perform failed: `%s'\n",
                   curl_easy_strerror (errornum));
          curl_easy_cleanup (c);
          return "curl error";
        }
    }
  curl_easy_cleanup (c);

  return NULL;
}


static void *
do_gets (void * param)
{
  int j;
  pthread_t par[PAR];
  char url[64];
  int port = (int)(intptr_t)param;
  char *err = NULL;

  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/hello_world",
            port);
  for (j=0;j<PAR;j++)
    {
      if (0 != pthread_create(&par[j], NULL, &thread_gets, (void*)url))
        {
          for (j--; j >= 0; j--)
            pthread_join(par[j], NULL);
          return "pthread_create error";
        }
    }
  for (j=0;j<PAR;j++)
    {
      char *ret_val;
      if (0 != pthread_join(par[j], (void**)&ret_val) ||
          NULL != ret_val)
        err = ret_val;
    }
  signal_done = 1;
  return err;
}


static int
testInternalGet (int port, int poll_flag)
{
  struct MHD_Daemon *d;
  const char * const test_desc = ((poll_flag & MHD_USE_AUTO) ? "internal thread with 'auto'" :
                                  (poll_flag & MHD_USE_POLL) ? "internal thread with poll()" :
                                  (poll_flag & MHD_USE_EPOLL) ? "internal thread with epoll" : "internal thread with select()");
  const char * ret_val;

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  signal_done = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        port, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
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
  start_timer ();
  ret_val = do_gets ((void*)(intptr_t)port);
  if (!ret_val)
    stop (test_desc);
  MHD_stop_daemon (d);
  if (ret_val)
    {
      fprintf (stderr,
               "Error performing %s test: %s\n", test_desc, ret_val);
      return 4;
    }
  return 0;
}


static int
testMultithreadedGet (int port, int poll_flag)
{
  struct MHD_Daemon *d;
  const char * const test_desc = ((poll_flag & MHD_USE_AUTO) ? "internal thread with 'auto' and thread per connection" :
                                  (poll_flag & MHD_USE_POLL) ? "internal thread with poll() and thread per connection" :
                                  (poll_flag & MHD_USE_EPOLL) ? "internal thread with epoll and thread per connection"
                                      : "internal thread with select() and thread per connection");
  const char * ret_val;

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  signal_done = 0;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        port, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
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
  start_timer ();
  ret_val = do_gets ((void*)(intptr_t)port);
  if (!ret_val)
    stop (test_desc);
  MHD_stop_daemon (d);
  if (ret_val)
    {
      fprintf (stderr,
               "Error performing %s test: %s\n", test_desc, ret_val);
      return 4;
    }
  return 0;
}


static int
testMultithreadedPoolGet (int port, int poll_flag)
{
  struct MHD_Daemon *d;
  const char * const test_desc = ((poll_flag & MHD_USE_AUTO) ? "internal thread pool with 'auto'" :
                                  (poll_flag & MHD_USE_POLL) ? "internal thread pool with poll()" :
                                  (poll_flag & MHD_USE_EPOLL) ? "internal thread poll with epoll" : "internal thread pool with select()");
  const char * ret_val;

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  signal_done = 0 ;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | poll_flag,
                        port, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_THREAD_POOL_SIZE, CPU_COUNT, MHD_OPTION_END);
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
  start_timer ();
  ret_val = do_gets ((void*)(intptr_t)port);
  if (!ret_val)
    stop (test_desc);
  MHD_stop_daemon (d);
  if (ret_val)
    {
      fprintf (stderr,
               "Error performing %s test: %s\n", test_desc, ret_val);
      return 4;
    }
  return 0;
}


static int
testExternalGet (int port)
{
  struct MHD_Daemon *d;
  pthread_t pid;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket max;
  struct timeval tv;
  MHD_UNSIGNED_LONG_LONG tt;
  int tret;
  char *ret_val;
  int ret = 0;

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  signal_done = 0;
  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
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
  if (0 != pthread_create (&pid, NULL,
			   &do_gets, (void*)(intptr_t)port))
    {
      MHD_stop_daemon(d);
      return 512;
    }
  start_timer ();

  while (0 == signal_done)
    {
      max = 0;
      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);
      if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
	{
	  MHD_stop_daemon (d);
	  return 4096;
	}
      tret = MHD_get_timeout (d, &tt);
      if (MHD_YES != tret) tt = 1;
      tv.tv_sec = tt / 1000;
      tv.tv_usec = 1000 * (tt % 1000);
      if (-1 == select (max + 1, &rs, &ws, &es, &tv))
	{
#ifdef MHD_POSIX_SOCKETS
          if (EINTR == errno)
            continue;
          fprintf (stderr,
                   "select failed: %s\n",
                   strerror (errno));
#else
          if (WSAEINVAL == WSAGetLastError() && 0 == rs.fd_count && 0 == ws.fd_count && 0 == es.fd_count)
            {
              Sleep (1000);
              continue;
            }
#endif
	  ret |= 1024;
	  break;
	}
      MHD_run_from_select(d, &rs, &ws, &es);
    }

  stop ("external select");
  MHD_stop_daemon (d);
  if (0 != pthread_join(pid, (void**)&ret_val) ||
      NULL != ret_val)
    {
      fprintf (stderr,
               "%s\n", ret_val);
      ret |= 8;
    }
  if (ret)
    fprintf (stderr, "Error performing test.\n");
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  int port = 1100;
  (void)argc;   /* Unused. Silent compiler warning. */

  if (NULL == argv || 0 == argv[0])
    return 99;
  oneone = has_in_name (argv[0], "11");
  if (oneone)
    port += 15;
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  response = MHD_create_response_from_buffer (strlen ("/hello_world"),
					      "/hello_world",
					      MHD_RESPMEM_MUST_COPY);
  errorCount += testInternalGet (port++, 0);
  errorCount += testMultithreadedGet (port++, 0);
  errorCount += testMultithreadedPoolGet (port++, 0);
  errorCount += testExternalGet (port++);
  errorCount += testInternalGet (port++, MHD_USE_AUTO);
  errorCount += testMultithreadedGet (port++, MHD_USE_AUTO);
  errorCount += testMultithreadedPoolGet (port++, MHD_USE_AUTO);
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_POLL))
    {
      errorCount += testInternalGet (port++, MHD_USE_POLL);
      errorCount += testMultithreadedGet (port++, MHD_USE_POLL);
      errorCount += testMultithreadedPoolGet (port++, MHD_USE_POLL);
    }
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_EPOLL))
    {
      errorCount += testInternalGet (port++, MHD_USE_EPOLL);
      errorCount += testMultithreadedPoolGet (port++, MHD_USE_EPOLL);
    }
  MHD_destroy_response (response);
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
