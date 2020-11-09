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
 * @file test_get.c
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
#include "test_helpers.h"
#include "mhd_sockets.h" /* only macros used */


#define EXPECTED_URI_PATH "/hello_world?a=%26&b=c"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /* !WIN32_LEAN_AND_MEAN */
#include <windows.h>
#endif

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

static int oneone;
static int global_port;

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


static void *
log_cb (void *cls,
        const char *uri,
        struct MHD_Connection *con)
{
  (void) cls;
  (void) con;
  if (0 != strcmp (uri,
                   EXPECTED_URI_PATH))
    {
      fprintf (stderr,
               "Wrong URI: `%s'\n",
               uri);
      _exit (22);
    }
  return NULL;
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
  const char *v;
  (void) version;
  (void) upload_data;
  (void) upload_data_size;       /* Unused. Silence compiler warning. */

  if (0 != strcasecmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  v = MHD_lookup_connection_value (connection,
                                   MHD_GET_ARGUMENT_KIND,
                                   "a");
  if ( (NULL == v) ||
       (0 != strcmp ("&",
                     v)) )
    {
      fprintf (stderr, "Found while looking for 'a=&': 'a=%s'\n",
               NULL == v ? "NULL" : v);
      _exit (17);
    }
  v = NULL;
  if (MHD_YES != MHD_lookup_connection_value_n (connection,
                                                MHD_GET_ARGUMENT_KIND,
                                                "b",
                                                1,
                                                &v,
                                                NULL))
    {
      fprintf (stderr, "Not found 'b' GET argument.\n");
      _exit (18);
    }
  if ( (NULL == v) ||
       (0 != strcmp ("c",
                     v)) )
    {
      fprintf (stderr, "Found while looking for 'b=c': 'b=%s'\n",
               NULL == v ? "NULL" : v);
      _exit (19);
    }
  response = MHD_create_response_from_buffer (strlen (url),
					      (void *) url,
					      MHD_RESPMEM_MUST_COPY);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);
  if (ret == MHD_NO)
    {
      fprintf (stderr, "Failed to queue response.\n");
      _exit (19);
    }
  return ret;
}


static int
testInternalGet (int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;

  if ( (0 == global_port) &&
       (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT)) )
    {
      global_port = 1220;
      if (oneone)
        global_port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        global_port, NULL, NULL,
                        &ahc_echo, "GET",
                        MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                        MHD_OPTION_END);
  if (d == NULL)
    return 1;
  if (0 == global_port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      global_port = (int)dinfo->port;
    }
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1" EXPECTED_URI_PATH);
  curl_easy_setopt (c, CURLOPT_PORT, (long)global_port);
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
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 4;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 8;
  return 0;
}


static int
testMultithreadedGet (int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;

  if ( (0 == global_port) &&
       (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT)) )
    {
      global_port = 1221;
      if (oneone)
        global_port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        global_port, NULL, NULL,
                        &ahc_echo, "GET",
                        MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                        MHD_OPTION_END);
  if (d == NULL)
    return 16;
  if (0 == global_port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      global_port = (int)dinfo->port;
    }
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1" EXPECTED_URI_PATH);
  curl_easy_setopt (c, CURLOPT_PORT, (long)global_port);
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
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 64;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 128;
  return 0;
}


static int
testMultithreadedPoolGet (int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;

  if ( (0 == global_port) &&
       (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT)) )
    {
      global_port = 1222;
      if (oneone)
        global_port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | poll_flag,
                        global_port, NULL, NULL,
                        &ahc_echo, "GET",
                        MHD_OPTION_THREAD_POOL_SIZE, CPU_COUNT,
                        MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                        MHD_OPTION_END);
  if (d == NULL)
    return 16;
  if (0 == global_port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      global_port = (int)dinfo->port;
    }
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1" EXPECTED_URI_PATH);
  curl_easy_setopt (c, CURLOPT_PORT, (long)global_port);
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
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 64;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 128;
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
  MHD_socket maxsock;
  int maxposixs; 
  int running;
  struct CURLMsg *msg;
  time_t start;
  struct timeval tv;

  if ( (0 == global_port) &&
       (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT)) )
    {
      global_port = 1223;
      if (oneone)
        global_port += 20;
    }

  multi = NULL;
  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        global_port, NULL, NULL,
                        &ahc_echo, "GET",
                        MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                        MHD_OPTION_END);
  if (d == NULL)
    return 256;
  if (0 == global_port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      global_port = (int)dinfo->port;
    }
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1" EXPECTED_URI_PATH);
  curl_easy_setopt (c, CURLOPT_PORT, (long)global_port);
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
#ifdef MHD_POSIX_SOCKETS
      if (maxsock > maxposixs)
        maxposixs = maxsock;
#endif /* MHD_POSIX_SOCKETS */
      if (-1 == select (maxposixs + 1, &rs, &ws, &es, &tv))
        {
#ifdef MHD_POSIX_SOCKETS
          if (EINTR != errno)
            abort ();
#else
          if (WSAEINVAL != WSAGetLastError() || 0 != rs.fd_count || 0 != ws.fd_count || 0 != es.fd_count)
            _exit (99);
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
    return 8192;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 16384;
  return 0;
}


static int
testUnknownPortGet (int poll_flag)
{
  struct MHD_Daemon *d;
  const union MHD_DaemonInfo *di;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int port;

  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  addr.sin_addr.s_addr = INADDR_ANY;

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        0, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_SOCK_ADDR, &addr,
                        MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                        MHD_OPTION_END);
  if (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    {
      di = MHD_get_daemon_info (d, MHD_DAEMON_INFO_LISTEN_FD);
      if (di == NULL)
        return 65536;

      if (0 != getsockname(di->listen_fd, (struct sockaddr *) &addr, &addr_len))
        return 131072;

      if (addr.sin_family != AF_INET)
        return 26214;
      port = (int)ntohs(addr.sin_port);
    }
  else
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      port = (int)dinfo->port;
    }

  snprintf(buf,
           sizeof(buf),
           "http://127.0.0.1:%d%s",
           port,
           EXPECTED_URI_PATH);

  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, buf);
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
  if (CURLE_OK != (errornum = curl_easy_perform (c)))
    {
      fprintf (stderr,
               "curl_easy_perform failed: `%s'\n",
               curl_easy_strerror (errornum));
      curl_easy_cleanup (c);
      MHD_stop_daemon (d);
      return 524288;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  if (cbc.pos != strlen ("/hello_world"))
    return 1048576;
  if (0 != strncmp ("/hello_world", cbc.buf, strlen ("/hello_world")))
    return 2097152;
  return 0;
}


static int
testStopRace (int poll_flag)
{
    struct sockaddr_in sin;
    MHD_socket fd;
    struct MHD_Daemon *d;

    if ( (0 == global_port) &&
         (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT)) )
      {
        global_port = 1224;
        if (oneone)
          global_port += 20;
      }

    d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | poll_flag,
                         global_port, NULL, NULL,
                          &ahc_echo, "GET",
                          MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                          MHD_OPTION_END);
    if (d == NULL)
       return 16;
    if (0 == global_port)
      {
        const union MHD_DaemonInfo *dinfo;
        dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
        if (NULL == dinfo || 0 == dinfo->port)
          { MHD_stop_daemon (d); return 32; }
        global_port = (int)dinfo->port;
      }

    fd = socket (PF_INET, SOCK_STREAM, 0);
    if (fd == MHD_INVALID_SOCKET)
    {
       fprintf(stderr, "socket error\n");
       return 256;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(global_port);
    sin.sin_addr.s_addr = htonl(0x7f000001);

    if (connect (fd, (struct sockaddr *)(&sin), sizeof(sin)) < 0)
    {
       fprintf(stderr, "connect error\n");
       MHD_socket_close_chk_ (fd);
       return 512;
    }

    /*  printf("Waiting\n"); */
    /* Let the thread get going. */
    usleep(500000);

    /* printf("Stopping daemon\n"); */
    MHD_stop_daemon (d);

    MHD_socket_close_chk_ (fd);

    /* printf("good\n"); */
    return 0;
}


static int
ahc_empty (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size,
          void **unused)
{
  static int ptr;
  struct MHD_Response *response;
  int ret;
  (void)cls;(void)url;(void)url;(void)version;  /* Unused. Silent compiler warning. */
  (void)upload_data;(void)upload_data_size;     /* Unused. Silent compiler warning. */

  if (0 != strcasecmp ("GET", method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  response = MHD_create_response_from_buffer (0,
					      NULL,
					      MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  if (ret == MHD_NO)
    {
      fprintf (stderr, "Failed to queue response.\n");
      _exit (20);
    }
  return ret;
}


static int
curlExcessFound(CURL *c, curl_infotype type, char *data, size_t size, void *cls)
{
  static const char *excess_found = "Excess found";
  const size_t str_size = strlen (excess_found);
  (void)c;      /* Unused. Silent compiler warning. */

  if (CURLINFO_TEXT == type
      && size >= str_size
      && 0 == strncmp(excess_found, data, str_size))
    *(int *)cls = 1;
  return 0;
}


static int
testEmptyGet (int poll_flag)
{
  struct MHD_Daemon *d;
  CURL *c;
  char buf[2048];
  struct CBC cbc;
  CURLcode errornum;
  int excess_found = 0;

  if ( (0 == global_port) &&
       (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT)) )
    {
      global_port = 1225;
      if (oneone)
        global_port += 20;
    }

  cbc.buf = buf;
  cbc.size = 2048;
  cbc.pos = 0;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        global_port, NULL, NULL,
                        &ahc_empty, NULL,
                        MHD_OPTION_URI_LOG_CALLBACK, &log_cb, NULL,
                        MHD_OPTION_END);
  if (d == NULL)
    return 4194304;
  if (0 == global_port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (d); return 32; }
      global_port = (int)dinfo->port;
    }
  c = curl_easy_init ();
  curl_easy_setopt (c, CURLOPT_URL, "http://127.0.0.1" EXPECTED_URI_PATH);
  curl_easy_setopt (c, CURLOPT_PORT, (long)global_port);
  curl_easy_setopt (c, CURLOPT_WRITEFUNCTION, &copyBuffer);
  curl_easy_setopt (c, CURLOPT_WRITEDATA, &cbc);
  curl_easy_setopt (c, CURLOPT_DEBUGFUNCTION, &curlExcessFound);
  curl_easy_setopt (c, CURLOPT_DEBUGDATA, &excess_found);
  curl_easy_setopt (c, CURLOPT_VERBOSE, 1L);
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
      return 8388608;
    }
  curl_easy_cleanup (c);
  MHD_stop_daemon (d);
  if (cbc.pos != 0)
    return 16777216;
  if (excess_found)
    return 33554432;
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  unsigned int test_result = 0;
  int verbose = 0;

  if (NULL == argv || 0 == argv[0])
    return 99;
  oneone = has_in_name (argv[0], "11");
  verbose = has_param (argc, argv, "-v") || has_param (argc, argv, "--verbose");
  if (0 != curl_global_init (CURL_GLOBAL_WIN32))
    return 2;
  global_port = 0;
  test_result = testExternalGet ();
  if (test_result)
    fprintf (stderr, "FAILED: testExternalGet () - %u.\n", test_result);
  else if (verbose)
    printf ("PASSED: testExternalGet ().\n");
  errorCount += test_result;
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_THREADS))
    {
      test_result += testInternalGet (0);
      if (test_result)
        fprintf (stderr, "FAILED: testInternalGet (0) - %u.\n", test_result);
      else if (verbose)
        printf ("PASSED: testInternalGet (0).\n");
      errorCount += test_result;
      test_result += testMultithreadedGet (0);
      if (test_result)
        fprintf (stderr, "FAILED: testMultithreadedGet (0) - %u.\n", test_result);
      else if (verbose)
        printf ("PASSED: testMultithreadedGet (0).\n");
      errorCount += test_result;
      test_result += testMultithreadedPoolGet (0);
      if (test_result)
        fprintf (stderr, "FAILED: testMultithreadedPoolGet (0) - %u.\n", test_result);
      else if (verbose)
        printf ("PASSED: testMultithreadedPoolGet (0).\n");
      errorCount += test_result;
      test_result += testUnknownPortGet (0);
      if (test_result)
        fprintf (stderr, "FAILED: testUnknownPortGet (0) - %u.\n", test_result);
      else if (verbose)
        printf ("PASSED: testUnknownPortGet (0).\n");
      errorCount += test_result;
      test_result += testStopRace (0);
      if (test_result)
        fprintf (stderr, "FAILED: testStopRace (0) - %u.\n", test_result);
      else if (verbose)
        printf ("PASSED: testStopRace (0).\n");
      errorCount += test_result;
      test_result += testEmptyGet (0);
      if (test_result)
        fprintf (stderr, "FAILED: testEmptyGet (0) - %u.\n", test_result);
      else if (verbose)
        printf ("PASSED: testEmptyGet (0).\n");
      errorCount += test_result;
      if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_POLL))
	{
          test_result += testInternalGet(MHD_USE_POLL);
          if (test_result)
            fprintf (stderr, "FAILED: testInternalGet (MHD_USE_POLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testInternalGet (MHD_USE_POLL).\n");
          errorCount += test_result;
          test_result += testMultithreadedGet(MHD_USE_POLL);
          if (test_result)
            fprintf (stderr, "FAILED: testMultithreadedGet (MHD_USE_POLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testMultithreadedGet (MHD_USE_POLL).\n");
          errorCount += test_result;
          test_result += testMultithreadedPoolGet(MHD_USE_POLL);
          if (test_result)
            fprintf (stderr, "FAILED: testMultithreadedPoolGet (MHD_USE_POLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testMultithreadedPoolGet (MHD_USE_POLL).\n");
          errorCount += test_result;
          test_result += testUnknownPortGet(MHD_USE_POLL);
          if (test_result)
            fprintf (stderr, "FAILED: testUnknownPortGet (MHD_USE_POLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testUnknownPortGet (MHD_USE_POLL).\n");
          errorCount += test_result;
          test_result += testStopRace(MHD_USE_POLL);
          if (test_result)
            fprintf (stderr, "FAILED: testStopRace (MHD_USE_POLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testStopRace (MHD_USE_POLL).\n");
          errorCount += test_result;
          test_result += testEmptyGet(MHD_USE_POLL);
          if (test_result)
            fprintf (stderr, "FAILED: testEmptyGet (MHD_USE_POLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testEmptyGet (MHD_USE_POLL).\n");
          errorCount += test_result;
	}
      if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_EPOLL))
	{
          test_result += testInternalGet(MHD_USE_EPOLL);
          if (test_result)
            fprintf (stderr, "FAILED: testInternalGet (MHD_USE_EPOLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testInternalGet (MHD_USE_EPOLL).\n");
          errorCount += test_result;
          test_result += testMultithreadedPoolGet(MHD_USE_EPOLL);
          if (test_result)
            fprintf (stderr, "FAILED: testMultithreadedPoolGet (MHD_USE_EPOLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testMultithreadedPoolGet (MHD_USE_EPOLL).\n");
          errorCount += test_result;
          test_result += testUnknownPortGet(MHD_USE_EPOLL);
          if (test_result)
            fprintf (stderr, "FAILED: testUnknownPortGet (MHD_USE_EPOLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testUnknownPortGet (MHD_USE_EPOLL).\n");
          errorCount += test_result;
          test_result += testEmptyGet(MHD_USE_EPOLL);
          if (test_result)
            fprintf (stderr, "FAILED: testEmptyGet (MHD_USE_EPOLL) - %u.\n", test_result);
          else if (verbose)
            printf ("PASSED: testEmptyGet (MHD_USE_EPOLL).\n");
          errorCount += test_result;
	}
    }
  if (0 != errorCount)
    fprintf (stderr,
	     "Error (code: %u)\n",
	     errorCount);
  else if (verbose)
    printf ("All tests passed.\n");
  curl_global_cleanup ();
  return errorCount != 0;       /* 0 == pass */
}
