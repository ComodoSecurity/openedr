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
 * @file daemontest_post_loop.c
 * @brief  Testcase for libmicrohttpd POST operations using URL-encoding
 * @author Christian Grothoff (inspired by bug report #1296)
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
#endif

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

#define POST_DATA "<?xml version='1.0' ?>\n<xml>\n<data-id>1</data-id>\n</xml>\n"

#define LOOPCOUNT 1000

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
          void **mptr)
{
  static int marker;
  struct MHD_Response *response;
  int ret;
  (void)cls;(void)url;(void)version;            /* Unused. Silent compiler warning. */
  (void)upload_data;(void)upload_data_size;     /* Unused. Silent compiler warning. */

  if (0 != strcmp ("POST", method))
    {
      printf ("METHOD: %s\n", method);
      return MHD_NO;            /* unexpected method */
    }
  if ((*mptr != NULL) && (0 == *upload_data_size))
    {
      if (*mptr != &marker)
        abort ();
      response = MHD_create_response_from_buffer (2, "OK",
						  MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
      *mptr = NULL;
      return ret;
    }
  if (strlen (POST_DATA) != *upload_data_size)
    return MHD_YES;
  *upload_data_size = 0;
  *mptr = &marker;
  return MHD_YES;
}


static int
testInternalPost ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int i;
  char url[1024];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1350;
      if (oneone)
        port += 10;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, NULL, MHD_OPTION_END);
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
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
        fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      snprintf (url,
                sizeof (url),
                "http://127.0.0.1:%d/hw%d",
                port,
                i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
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
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          MHD_stop_daemon (d);
          return 4;
        }
    }
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}

static int
testMultithreadedPost ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int i;
  char url[1024];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1351;
      if (oneone)
        port += 10;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL,
                        &ahc_echo, NULL,
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
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
        fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      snprintf (url,
                sizeof (url),
                "http://127.0.0.1:%d/hw%d",
                port,
                i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
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
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          MHD_stop_daemon (d);
          return 64;
        }
    }
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}

static int
testMultithreadedPoolPost ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int i;
  char url[1024];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1352;
      if (oneone)
        port += 10;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, NULL,
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
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
        fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      snprintf (url,
                sizeof (url),
                "http://127.0.0.1:%d/hw%d",
                port,
                i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
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
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          MHD_stop_daemon (d);
          return 64;
        }
    }
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}

static int
testExternalPost ()
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
  int i;
  unsigned long long timeout;
  long ctimeout;
  char url[1024];
  int port;

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1353;
      if (oneone)
        port += 10;
    }

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, NULL, MHD_OPTION_END);
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
  multi = curl_multi_init ();
  if (multi == NULL)
    {
      MHD_stop_daemon (d);
      return 512;
    }
  for (i = 0; i < LOOPCOUNT; i++)
    {
      if (99 == i % 100)
	fprintf (stderr, ".");
      c = curl_easy_init ();
      cbc.pos = 0;
      buf[0] = '\0';
      snprintf (url,
                sizeof (url),
                "http://127.0.0.1:%d/hw%d",
                port,
                i);
      curl_easy_setopt (c, CURLOPT_URL, url);
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_POSTFIELDS, POST_DATA);
      curl_easy_setopt (c, CURLOPT_POSTFIELDSIZE, strlen (POST_DATA));
      curl_easy_setopt (c, CURLOPT_POST, 1L);
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
          while (CURLM_CALL_MULTI_PERFORM ==
                 curl_multi_perform (multi, &running));
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
          if (MHD_NO == MHD_get_timeout (d, &timeout))
            timeout = 100;      /* 100ms == INFTY -- CURL bug... */
          if ((CURLM_OK == curl_multi_timeout (multi, &ctimeout)) &&
              (ctimeout < (long long)timeout) && (ctimeout >= 0))
            timeout = ctimeout;
	  if ( (c == NULL) || (running == 0) )
	    timeout = 0; /* terminate quickly... */
          tv.tv_sec = timeout / 1000;
          tv.tv_usec = (timeout % 1000) * 1000;
          if (-1 == select (maxposixs + 1, &rs, &ws, &es, &tv))
	    {
	      if (EINTR == errno)
		continue;
	      fprintf (stderr,
		       "select failed: %s\n",
		       strerror (errno));
	      break;
	    }
          while (CURLM_CALL_MULTI_PERFORM ==
                 curl_multi_perform (multi, &running));
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
          MHD_run (d);
        }
      if (c != NULL)
        {
          curl_multi_remove_handle (multi, c);
          curl_easy_cleanup (c);
        }
      if ((buf[0] != 'O') || (buf[1] != 'K'))
        {
          curl_multi_cleanup (multi);
          MHD_stop_daemon (d);
          return 8192;
        }
    }
  curl_multi_cleanup (multi);
  MHD_stop_daemon (d);
  if (LOOPCOUNT >= 99)
    fprintf (stderr, "\n");
  return 0;
}


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
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_THREADS))
    {
      start_time = now();
      errorCount += testInternalPost ();
      fprintf (stderr,
	       oneone ? "%s: Sequential POSTs (http/1.1) %f/s\n" : "%s: Sequential POSTs (http/1.0) %f/s\n",
	       "internal select",
	       (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0));
      GAUGER ("internal select",
	      oneone ? "Sequential POSTs (http/1.1)" : "Sequential POSTs (http/1.0)",
	      (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0),
	      "requests/s");
      start_time = now();
      errorCount += testMultithreadedPost ();
      fprintf (stderr,
	       oneone ? "%s: Sequential POSTs (http/1.1) %f/s\n" : "%s: Sequential POSTs (http/1.0) %f/s\n",
	       "multithreaded post",
	       (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0));
      GAUGER ("Multithreaded select",
	      oneone ? "Sequential POSTs (http/1.1)" : "Sequential POSTs (http/1.0)",
	      (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0),
	      "requests/s");
      start_time = now();
      errorCount += testMultithreadedPoolPost ();
      fprintf (stderr,
	       oneone ? "%s: Sequential POSTs (http/1.1) %f/s\n" : "%s: Sequential POSTs (http/1.0) %f/s\n",
	       "thread with pool",
	       (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0));
      GAUGER ("thread with pool",
	      oneone ? "Sequential POSTs (http/1.1)" : "Sequential POSTs (http/1.0)",
	      (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0),
	      "requests/s");
    }
  start_time = now();
  errorCount += testExternalPost ();
  fprintf (stderr,
	   oneone ? "%s: Sequential POSTs (http/1.1) %f/s\n" : "%s: Sequential POSTs (http/1.0) %f/s\n",
	   "external select",
	   (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0));
  GAUGER ("external select",
	  oneone ? "Sequential POSTs (http/1.1)" : "Sequential POSTs (http/1.0)",
	  (double) 1000 * LOOPCOUNT / (now() - start_time + 1.0),
	  "requests/s");
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
