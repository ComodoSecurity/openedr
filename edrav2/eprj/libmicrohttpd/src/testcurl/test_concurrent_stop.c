/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2009, 2011, 2015, 2016 Christian Grothoff

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
 * @file test_concurrent_stop.c
 * @brief test stopping server while concurrent GETs are ongoing
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

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

/**
 * How many rounds of operations do we do for each
 * test (total number of requests will be ROUNDS * PAR).
 */
#define ROUNDS 50000

/**
 * How many requests do we do in parallel?
 */
#define PAR (CPU_COUNT * 4)

/**
 * Do we use HTTP 1.1?
 */
static int oneone;

/**
 * Response to return (re-used).
 */
static struct MHD_Response *response;

/**
 * Continue generating new requests?
 */
static volatile int continue_requesting;

/**
 * Continue waiting in watchdog thread?
 */
static volatile int watchdog_continue;

static const char *watchdog_obj;

static void *
thread_watchdog (void *param)
{
  int seconds_passed;
  const int timeout_val = (int) (intptr_t) param;

  seconds_passed = 0;
  while (watchdog_continue) /* Poor threads sync, but works for testing. */
    {
      if (0 == sleep (1)) /* Poor accuracy, but enough for testing. */
        seconds_passed++;
      if (timeout_val < seconds_passed)
        {
          fprintf (stderr, "%s timeout expired.\n", watchdog_obj ? watchdog_obj : "Watchdog");
          fflush (stderr);
          _exit(16);
        }
    }
  return NULL;
}

pthread_t watchdog_tid;

static void
start_watchdog(int timeout, const char *obj_name)
{
  watchdog_continue = 1;
  watchdog_obj = obj_name;
  if (0 != pthread_create(&watchdog_tid, NULL, &thread_watchdog, (void*)(intptr_t)timeout))
    {
      fprintf(stderr, "Failed to start watchdog.\n");
      _exit (99);
    }
}

static void
stop_watchdog(void)
{
  watchdog_continue = 0;
  if (0 != pthread_join (watchdog_tid, NULL))
    {
      fprintf(stderr, "Failed to stop watchdog.\n");
      _exit (99);
    }
}

static size_t
copyBuffer (void *ptr,
	    size_t size, size_t nmemb,
	    void *ctx)
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
          const char *upload_data,
          size_t *upload_data_size,
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
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  if (ret == MHD_NO)
    abort ();
  return ret;
}

static void *
thread_gets (void *param)
{
  CURL *c;
  CURLcode errornum;
  char * const url = (char*) param;

  c = NULL;
  c = curl_easy_init ();
  if (NULL == c)
    {
      fprintf(stderr, "curl_easy_init failed.\n");
      _exit(99);
    }
  curl_easy_setopt (c, CURLOPT_URL, url);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, NULL);
  curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt (c, CURLOPT_TIMEOUT, 2L);
  if (oneone)
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 2L);
  /* NOTE: use of CONNECTTIMEOUT without also
     setting NOSIGNAL results in really weird
     crashes on my system! */
  curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
  while (continue_requesting)
    {
      errornum = curl_easy_perform (c);
      if (CURLE_OK != errornum)
        {
          curl_easy_cleanup (c);
          return NULL;
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

  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/hello_world",
            port);

  for (j=0;j<PAR;j++)
    {
      if (0 != pthread_create(&par[j], NULL, &thread_gets, (void*)url))
        {
          fprintf(stderr, "pthread_create failed.\n");
          continue_requesting = 0;
          for (j--; j >= 0; j--)
            {
              pthread_join(par[j], NULL);
            }
          _exit(99);
        }
    }
  (void)sleep (1);
  for (j=0;j<PAR;j++)
    {
      pthread_join(par[j], NULL);
    }
  return NULL;
}


pthread_t start_gets(int port)
{
  pthread_t tid;
  continue_requesting = 1;
  if (0 != pthread_create(&tid, NULL, &do_gets, (void*)(intptr_t)port))
    {
      fprintf(stderr, "pthread_create failed.\n");
      _exit(99);
    }
  return tid;
}


static int
testMultithreadedGet (int port,
                      int poll_flag)
{
  struct MHD_Daemon *d;
  pthread_t p;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        port,
                        NULL, NULL,
                        &ahc_echo, "GET",
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
  p = start_gets (port);
  (void)sleep (1);
  start_watchdog(10, "daemon_stop() in testMultithreadedGet");
  MHD_stop_daemon (d);
  stop_watchdog();
  continue_requesting = 0;
  pthread_join (p, NULL);
  return 0;
}


static int
testMultithreadedPoolGet (int port,
                          int poll_flag)
{
  struct MHD_Daemon *d;
  pthread_t p;

  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | poll_flag,
                        port,
                        NULL, NULL,
                        &ahc_echo, "GET",
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
  p = start_gets (port);
  (void)sleep (1);
  start_watchdog(10, "daemon_stop() in testMultithreadedPoolGet");
  MHD_stop_daemon (d);
  stop_watchdog();
  continue_requesting = 0;
  pthread_join (p, NULL);
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  int port;
  (void)argc;   /* Unused. Silent compiler warning. */
  (void)argv;   /* Unused. Silent compiler warning. */

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 1142;

  /* Do reuse connection, otherwise all available local ports may exhausted. */
  oneone = 1;

  if (0 != port && oneone)
    port += 5;
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  response = MHD_create_response_from_buffer (strlen ("/hello_world"),
					      "/hello_world",
					      MHD_RESPMEM_MUST_COPY);
  errorCount += testMultithreadedGet (port, 0);
  if (0 != port) port++;
  errorCount += testMultithreadedPoolGet (port, 0);
  MHD_destroy_response (response);
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
