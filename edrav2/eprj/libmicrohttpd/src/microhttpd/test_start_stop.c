/*
     This file is part of libmicrohttpd
     Copyright (C) 2011 Christian Grothoff

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
 * @file test_start_stop.c
 * @brief  test for #1901 (start+stop)
 * @author Christian Grothoff
 */
#include "mhd_options.h"
#include "platform.h"
#include <microhttpd.h>

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size,
          void **unused)
{
  (void)cls;(void)connection;(void)url;          /* Unused. Silent compiler warning. */
  (void)method;(void)version;(void)upload_data;  /* Unused. Silent compiler warning. */
  (void)upload_data_size;(void)unused;           /* Unused. Silent compiler warning. */

  return MHD_NO;
}


#if defined(MHD_USE_POSIX_THREADS) || defined(MHD_USE_W32_THREADS)
static int
testInternalGet (int poll_flag)
{
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | poll_flag,
                        0, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 1;
  MHD_stop_daemon (d);
  return 0;
}

static int
testMultithreadedGet (int poll_flag)
{
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG  | poll_flag,
                        0, NULL, NULL, &ahc_echo, "GET", MHD_OPTION_END);
  if (d == NULL)
    return 2;
  MHD_stop_daemon (d);
  return 0;
}


static int
testMultithreadedPoolGet (int poll_flag)
{
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | poll_flag,
                        0, NULL, NULL, &ahc_echo, "GET",
                        MHD_OPTION_THREAD_POOL_SIZE, CPU_COUNT, MHD_OPTION_END);
  if (d == NULL)
    return 4;
  MHD_stop_daemon (d);
  return 0;
}
#endif


static int
testExternalGet ()
{
  struct MHD_Daemon *d;

  d = MHD_start_daemon (MHD_USE_ERROR_LOG,
                        0, NULL, NULL,
			&ahc_echo, "GET",
			MHD_OPTION_END);
  if (NULL == d)
    return 8;
  MHD_stop_daemon (d);
  return 0;
}


int
main (int argc,
      char *const *argv)
{
  unsigned int errorCount = 0;
  (void) argc;
  (void) argv; /* Unused. Silence compiler warning. */

#if defined(MHD_USE_POSIX_THREADS) || defined(MHD_USE_W32_THREADS)
  errorCount += testInternalGet (0);
  errorCount += testMultithreadedGet (0);
  errorCount += testMultithreadedPoolGet (0);
#endif
  errorCount += testExternalGet ();
#if defined (MHD_USE_POSIX_THREADS) || defined(MHD_USE_W32_THREADS)
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_POLL))
    {
      errorCount += testInternalGet(MHD_USE_POLL);
      errorCount += testMultithreadedGet(MHD_USE_POLL);
      errorCount += testMultithreadedPoolGet(MHD_USE_POLL);
    }
  if (MHD_YES == MHD_is_feature_supported(MHD_FEATURE_EPOLL))
    {
      errorCount += testInternalGet(MHD_USE_EPOLL);
      errorCount += testMultithreadedPoolGet(MHD_USE_EPOLL);
    }
#endif
  if (0 != errorCount)
    fprintf (stderr,
	     "Error (code: %u)\n",
	     errorCount);
  return errorCount != 0;       /* 0 == pass */
}
