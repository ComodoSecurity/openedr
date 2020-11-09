/* DO NOT CHANGE THIS LINE */
/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2009 Christian Grothoff

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
 * @file daemontest_get_response_cleanup.c
 * @brief  Testcase for libmicrohttpd response cleanup
 * @author Christian Grothoff
 */

#include "MHD_config.h"
#include "platform.h"
#include <microhttpd.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#ifndef _WIN32
#include <signal.h>
#endif /* _WIN32 */

#ifndef WINDOWS
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /* !WIN32_LEAN_AND_MEAN */
#include <windows.h>
#endif

#include "mhd_has_in_name.h"

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

#define TESTSTR "/* DO NOT CHANGE THIS LINE */"

static int oneone;

static int ok;


static pid_t
fork_curl (const char *url)
{
  pid_t ret;

  ret = fork();
  if (ret != 0)
    return ret;
  execlp ("curl", "curl", "-s", "-N", "-o", "/dev/null", "-GET", url, NULL);
  fprintf (stderr,
	   "Failed to exec curl: %s\n",
	   strerror (errno));
  _exit (-1);
}


static void
kill_curl (pid_t pid)
{
  int status;

  //fprintf (stderr, "Killing curl\n");
  kill (pid, SIGTERM);
  waitpid (pid, &status, 0);
}


static ssize_t
push_callback (void *cls, uint64_t pos, char *buf, size_t max)
{
  (void)cls;(void)pos;	/* Unused. Silent compiler warning. */

  if (max == 0)
    return 0;
  buf[0] = 'd';
  return 1;
}


static void
push_free_callback (void *cls)
{
  int *ok = cls;

  //fprintf (stderr, "Cleanup callback called!\n");
  *ok = 0;
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
  (void)url;(void)version;                      /* Unused. Silent compiler warning. */
  (void)upload_data;(void)upload_data_size;     /* Unused. Silent compiler warning. */

  //fprintf (stderr, "In CB: %s!\n", method);
  if (0 != strcmp (me, method))
    return MHD_NO;              /* unexpected method */
  if (&ptr != *unused)
    {
      *unused = &ptr;
      return MHD_YES;
    }
  *unused = NULL;
  response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN,
						32 * 1024,
						&push_callback,
						&ok,
						&push_free_callback);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  if (ret == MHD_NO)
    abort ();
  return ret;
}


static int
testInternalGet ()
{
  struct MHD_Daemon *d;
  pid_t curl;
  int port;
  char url[127];

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1180;
      if (oneone)
        port += 10;
    }

  ok = 1;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
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
            "http://127.0.0.1:%d/",
            port);
  curl = fork_curl (url);
  (void)sleep (1);
  kill_curl (curl);
  (void)sleep (1);
  /* fprintf (stderr, "Stopping daemon!\n"); */
  MHD_stop_daemon (d);
  if (ok != 0)
    return 2;
  return 0;
}


static int
testMultithreadedGet ()
{
  struct MHD_Daemon *d;
  pid_t curl;
  int port;
  char url[127];

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1181;
      if (oneone)
        port += 10;
    }

  ok = 1;
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, "GET",
			MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 2,
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
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/",
            port);
  //fprintf (stderr, "Forking cURL!\n");
  curl = fork_curl (url);
  (void)sleep (1);
  kill_curl (curl);
  (void)sleep (1);
  curl = fork_curl (url);
  (void)sleep (1);
  if (ok != 0)
    {
      kill_curl (curl);
      MHD_stop_daemon (d);
      return 64;
    }
  kill_curl (curl);
  (void)sleep (1);
  //fprintf (stderr, "Stopping daemon!\n");
  MHD_stop_daemon (d);
  if (ok != 0)
    return 32;

  return 0;
}


static int
testMultithreadedPoolGet ()
{
  struct MHD_Daemon *d;
  pid_t curl;
  int port;
  char url[127];

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1182;
      if (oneone)
        port += 10;
    }

  ok = 1;
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_THREAD_POOL_SIZE, CPU_COUNT, MHD_OPTION_END);
  if (d == NULL)
    return 64;
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
            "http://127.0.0.1:%d/",
            port);
  curl = fork_curl (url);
  (void)sleep (1);
  kill_curl (curl);
  (void)sleep (1);
  //fprintf (stderr, "Stopping daemon!\n");
  MHD_stop_daemon (d);
  if (ok != 0)
    return 128;
  return 0;
}


static int
testExternalGet ()
{
  struct MHD_Daemon *d;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket max;
  time_t start;
  struct timeval tv;
  pid_t curl;
  int port;
  char url[127];

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    {
      port = 1183;
      if (oneone)
        port += 10;
    }

  ok = 1;
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
  snprintf (url,
            sizeof (url),
            "http://127.0.0.1:%d/",
            port);
  curl = fork_curl (url);

  start = time (NULL);
  while ((time (NULL) - start < 2))
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
      tv.tv_sec = 0;
      tv.tv_usec = 1000;
      if (-1 == select (max + 1, &rs, &ws, &es, &tv))
        {
          if (EINTR != errno)
            abort ();
        }
      MHD_run (d);
    }
  kill_curl (curl);
  start = time (NULL);
  while ((time (NULL) - start < 2))
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
      tv.tv_sec = 0;
      tv.tv_usec = 1000;
      if (-1 == select (max + 1, &rs, &ws, &es, &tv))
        {
          if (EINTR != errno)
            abort ();
        }
      MHD_run (d);
    }
  /* fprintf (stderr, "Stopping daemon!\n"); */
  MHD_stop_daemon (d);
  if (ok != 0)
    return 1024;
  return 0;
}


int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;
  (void)argc;   /* Unused. Silent compiler warning. */

#ifndef _WIN32
  /* Solaris has no way to disable SIGPIPE on socket disconnect. */
  if (MHD_NO == MHD_is_feature_supported (MHD_FEATURE_AUTOSUPPRESS_SIGPIPE))
    {
      struct sigaction act;

      act.sa_handler = SIG_IGN;
      sigaction(SIGPIPE, &act, NULL);
    }
#endif /* _WIN32 */

  if (NULL == argv || 0 == argv[0])
    return 99;
  oneone = has_in_name (argv[0], "11");
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_THREADS))
    {
      errorCount += testInternalGet ();
      errorCount += testMultithreadedGet ();
      errorCount += testMultithreadedPoolGet ();
    }
  errorCount += testExternalGet ();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  return errorCount != 0;       /* 0 == pass */
}
