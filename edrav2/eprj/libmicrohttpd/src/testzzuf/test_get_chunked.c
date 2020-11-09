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
 * @file daemontest_get_chunked.c
 * @brief  Testcase for libmicrohttpd GET operations with chunked content encoding
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

#include "socat.c"

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

/**
 * MHD content reader callback that returns
 * data in chunks.
 */
static ssize_t
crc (void *cls, uint64_t pos, char *buf, size_t max)
{
  struct MHD_Response **responseptr = cls;

  if (pos == 128 * 10)
    {
      MHD_add_response_header (*responseptr, "Footer", "working");
      return MHD_CONTENT_READER_END_OF_STREAM;
    }
  if (max < 128)
    abort ();                   /* should not happen in this testcase... */
  memset (buf, 'A' + (pos / 128), 128);
  return 128;
}

/**
 * Dummy function that does nothing.
 */
static void
crcf (void *ptr)
{
  free (ptr);
}

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  static int aptr;
  const char *me = cls;
  struct MHD_Response *response;
  struct MHD_Response **responseptr;
  int ret;

  (void) url;
  (void) version;                      /* Unused. Silent compiler warning. */
  (void) upload_data;
  (void) upload_data_size;     /* Unused. Silent compiler warning. */

  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  responseptr = malloc (sizeof (struct MHD_Response *));
  if (NULL == responseptr)
    return MHD_NO;
  response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN,
                                                1024,
                                                &crc, responseptr, &crcf);
  if (NULL == response)
  {
    free (responseptr);
    return MHD_NO;
  }
  *responseptr = response;
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);
  return ret;
}

static int
testInternalGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  int i;

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD /* | MHD_USE_ERROR_LOG */ ,
                        11080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 1;
  zzuf_socat_start ();
  for (i = 0; i < LOOP_COUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1:11081/hello_world");
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
      curl_easy_setopt (c, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT_MS, CURL_TIMEOUT);
      curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      /* NOTE: use of CONNECTTIMEOUT without also
       *   setting NOSIGNAL results in really weird
       *   crashes on my system! */
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
      curl_easy_perform (c);
      curl_easy_cleanup (c);
    }
  fprintf (stderr, "\n");
  zzuf_socat_stop ();
  MHD_stop_daemon (d);
  return 0;
}

static int
testMultithreadedGet ()
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  int i;

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD /* | MHD_USE_ERROR_LOG */ ,
                        11080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 16;
  zzuf_socat_start ();
  for (i = 0; i < LOOP_COUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1:11081/hello_world");
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
      curl_easy_setopt (c, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT);
      curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT_MS, CURL_TIMEOUT);
      /* NOTE: use of CONNECTTIMEOUT without also
       *   setting NOSIGNAL results in really weird
       *   crashes on my system! */
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
      curl_easy_perform (c);
      curl_easy_cleanup (c);
    }
  fprintf (stderr, "\n");
  zzuf_socat_stop ();
  MHD_stop_daemon (d);
  return 0;
}


static int
testExternalGet ()
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
  int max;
  int running;
  time_t start;
  struct timeval tv;
  int i;

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_NO_FLAG /* | MHD_USE_ERROR_LOG */ ,
                        11080, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 256;
  multi = curl_multi_init ();
  if (multi == NULL)
    {
      MHD_stop_daemon (d);
      return 512;
    }
  zzuf_socat_start ();
  for (i = 0; i < LOOP_COUNT; i++)
    {
      fprintf (stderr, ".");
      c = curl_easy_init ();
      curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1:11081/hello_world");
      curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
      curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
      curl_easy_setopt (c, CURLOPT_FAILONERROR, 1L);
      curl_easy_setopt (c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
      curl_easy_setopt (c, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT);
      curl_easy_setopt (c, CURLOPT_CONNECTTIMEOUT_MS, CURL_TIMEOUT);
      /* NOTE: use of CONNECTTIMEOUT without also
       *   setting NOSIGNAL results in really weird
       *   crashes on my system! */
      curl_easy_setopt (c, CURLOPT_NOSIGNAL, 1L);
      mret = curl_multi_add_handle (multi, c);
      if (mret != CURLM_OK)
        {
          curl_multi_cleanup (multi);
          curl_easy_cleanup (c);
          zzuf_socat_stop ();
          MHD_stop_daemon (d);
          return 1024;
        }
      start = time (NULL);
      while ((time (NULL) - start < 5) && (c != NULL))
        {
          max = 0;
          FD_ZERO (&rs);
          FD_ZERO (&ws);
          FD_ZERO (&es);
          curl_multi_perform (multi, &running);
          mret = curl_multi_fdset (multi, &rs, &ws, &es, &max);
          if (mret != CURLM_OK)
            {
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              zzuf_socat_stop ();
              MHD_stop_daemon (d);
              return 2048;
            }
          if (MHD_YES != MHD_get_fdset (d, &rs, &ws, &es, &max))
            {
              curl_multi_remove_handle (multi, c);
              curl_multi_cleanup (multi);
              curl_easy_cleanup (c);
              zzuf_socat_stop ();
              MHD_stop_daemon (d);
              return 4096;
            }
          tv.tv_sec = 0;
          tv.tv_usec = 1000;
          select (max + 1, &rs, &ws, &es, &tv);
          curl_multi_perform (multi, &running);
          if (running == 0)
            {
              curl_multi_info_read (multi, &running);
              curl_multi_remove_handle (multi, c);
              curl_easy_cleanup (c);
              c = NULL;
            }
          MHD_run (d);
        }
      if (c != NULL)
        {
          curl_multi_remove_handle (multi, c);
          curl_easy_cleanup (c);
        }
    }
  fprintf (stderr, "\n");
  curl_multi_cleanup (multi);
  zzuf_socat_stop ();
  MHD_stop_daemon (d);
  return 0;
}



int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  (void)argc; (void)argv; /* Unused. Silent compiler warning. */

  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_THREADS))
    {
      errorCount += testInternalGet ();
      errorCount += testMultithreadedGet ();
    }
  errorCount += testExternalGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
