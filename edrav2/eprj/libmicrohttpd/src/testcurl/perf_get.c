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
 * @file perf_get.c
 * @brief benchmark simple GET operations (sequential access).
 *        Note that we run libcurl in the same process at the
 *        same time, so the execution time given is the combined
 *        time for both MHD and libcurl; it is quite possible
 *        that more time is spend with libcurl than with MHD,
 *        so the performance scores calculated with this code
 *        should NOT be used to compare with other HTTP servers
 *        (since MHD is actually better); only the relative
 *        scores between MHD versions are meaningful.
 *        Furthermore, this code ONLY tests MHD processing
 *        a single request at a time.  This is again
 *        not universally meaningful (i.e. when comparing
 *        multithreaded vs. single-threaded or select/poll).
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <curl/curl.h>
#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "gauger.h"
#include "mhd_has_in_name.h"

#ifndef WINDOWS
#include <unistd.h>
#include <sys/socket.h>
#endif

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

/**
 * How many rounds of operations do we do for each
 * test?
 */
#define ROUNDS 500

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
  double rps = ((double) (ROUNDS * 1000)) / ((double) (now() - start_time));

  fprintf (stderr,
	   "Sequential GETs using %s: %f %s\n",
	   desc,
	   rps,
	   "requests/s");
  GAUGER (desc,
	  "Sequential GETs",
	  rps,
	  "requests/s");
}


struct CBC
{
  char *buf;
  size_t pos;
  size_t size;
};


static size_t
copyBuffer (void *ptr,
	    size_t size, size_t nmemb,
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


static int
testInternalGet (int port, int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  unsigned int i;
  char url[64];

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  cbc.buf = buf;
  cbc.size = 2048;
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
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/hello_world",
            port);
  start_timer ();
  for (i=0;i<ROUNDS;i++)
    {
      cbc.pos = 0;
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, url);
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
    }
  stop (poll_flag == MHD_USE_AUTO ? "internal thread with 'auto'" :
        poll_flag == MHD_USE_POLL ? "internal thread with poll()" :
	poll_flag == MHD_USE_EPOLL ? "internal thread with epoll" : "internal thread with select()");
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 4;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 8;
  return 0;
}


static int
testMultithreadedGet (int port, int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  unsigned int i;
  char url[64];

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  cbc.buf = buf;
  cbc.size = 2048;
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
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/hello_world",
            port);
  start_timer ();
  for (i=0;i<ROUNDS;i++)
    {
      cbc.pos = 0;
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
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
    }
  stop ((poll_flag & MHD_USE_AUTO) ? "internal thread with 'auto' and thread per connection" :
        (poll_flag & MHD_USE_POLL) ? "internal thread with poll() and thread per connection" :
	(poll_flag & MHD_USE_EPOLL) ? "internal thread with epoll and thread per connection" :
	    "internal thread with select() and thread per connection");
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 64;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 128;
  return 0;
}

static int
testMultithreadedPoolGet (int port, int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  unsigned int i;
  char url[64];

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  cbc.buf = buf;
  cbc.size = 2048;
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
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/hello_world",
            port);
  start_timer ();
  for (i=0;i<ROUNDS;i++)
    {
      cbc.pos = 0;
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
      curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
      if (oneone)
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
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
	  return 32;
	}
      curl_easy_cleanup (c);
    }
  stop (0 != (poll_flag & MHD_USE_AUTO) ? "internal thread pool with 'auto'" :
        0 != (poll_flag & MHD_USE_POLL) ? "internal thread pool with poll()" :
	0 != (poll_flag & MHD_USE_EPOLL) ? "internal thread pool with epoll" : "internal thread pool with select()");
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 64;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 128;
  return 0;
}

static int
testExternalGet (int port)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
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
  unsigned int i;
  char url[64];

  if (MHD_NO != MHD_is_feature_supported(MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        port, NULL, NULL,
                        &ahc_echo, "GET",
                        MHD_OPTION_END);
  if (NULL == d)
    return 256;
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      port = (int)dinfo->port;
    }
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/hello_world",
            port);
  start_timer ();
  multi = curl_multi_init ();
  if (multi == NULL)
    {
      MHD_stop_daemon (d);
      return 512;
    }
  for (i=0;i<ROUNDS;i++)
    {
      cbc.pos = 0;
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
      if (oneone)
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      else
	curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
      curl_easy_setopt (c, CURLOPT_TIMEOUT, 150L);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT, 150L);
      /* NOTE: use of CONNECTTIMEOUT without also
	 setting NOSIGNAL results in really weird
	 crashes on my system! */
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
      mret = curl_multi_add_handle (multi, c);
      if (mret != CURLM_OK)
	{
	  curl_multi_cleanup (multi);
	  curl_easy_cleanup (c);
	  MHD_stop_daemon (d);
	  return 1024;
	}
      start = time (NULL);
      while ((time (NULL) - start < 5) && (c != NULL))
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
		  curl_easy_cleanup (c);
		  c = NULL;
		}
	    }
	  /* two possibilities here; as select sets are
	     tiny, this makes virtually no difference
	     in actual runtime right now, even though the
	     number of select calls is virtually cut in half
	     (and 'select' is the most expensive of our system
	     calls according to 'strace') */
	  if (0)
	    MHD_run (d);
	  else
	    MHD_run_from_select (d, &rs, &ws, &es);
	}
      if (NULL != c)
	{
	  curl_multi_remove_handle (multi, c);
	  curl_easy_cleanup (c);
	  fprintf (stderr, "Timeout!?\n");
	}
    }
  stop ("external select");
  if (multi != NULL)
    {
      curl_multi_cleanup (multi);
    }
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 8192;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 16384;
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  int port = 1130;
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
  errorCount += testExternalGet (port++);
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_THREADS))
    {
      errorCount += testInternalGet (port++, MHD_USE_AUTO);
      errorCount += testMultithreadedGet (port++, MHD_USE_AUTO);
      errorCount += testMultithreadedPoolGet (port++, MHD_USE_AUTO);
      errorCount += testInternalGet (port++, 0);
      errorCount += testMultithreadedGet (port++, 0);
      errorCount += testMultithreadedPoolGet (port++, 0);
      if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_POLL))
	{
	  errorCount += testInternalGet(port++, MHD_USE_POLL);
	  errorCount += testMultithreadedGet(port++, MHD_USE_POLL);
	  errorCount += testMultithreadedPoolGet(port++, MHD_USE_POLL);
	}
      if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_EPOLL))
	{
	  errorCount += testInternalGet(port++, MHD_USE_EPOLL);
	  errorCount += testMultithreadedPoolGet(port++, MHD_USE_EPOLL);
	}
    }
  MHD_destroy_response (response);
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
