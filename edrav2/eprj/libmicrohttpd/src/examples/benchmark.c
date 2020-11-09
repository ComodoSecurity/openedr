/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2013 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
 * @file benchmark.c
 * @brief minimal code to benchmark MHD GET performance
 * @author Christian Grothoff
 */

#include "platform.h"
#include <microhttpd.h>

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd demo</body></html>"


#define SMALL (1024 * 128)

/**
 * Number of threads to run in the thread pool.  Should (roughly) match
 * the number of cores on your system.
 */
#define NUMBER_OF_THREADS CPU_COUNT

static unsigned int small_deltas[SMALL];

static struct MHD_Response *response;



/**
 * Signature of the callback used by MHD to notify the
 * application about completed requests.
 *
 * @param cls client-defined closure
 * @param connection connection handle
 * @param con_cls value as set by the last call to
 *        the MHD_AccessHandlerCallback
 * @param toe reason for request termination
 * @see MHD_OPTION_NOTIFY_COMPLETED
 */
static void
completed_callback (void *cls,
		    struct MHD_Connection *connection,
		    void **con_cls,
		    enum MHD_RequestTerminationCode toe)
{
  struct timeval *tv = *con_cls;
  struct timeval tve;
  uint64_t delta;
  (void)cls;         /* Unused. Silent compiler warning. */
  (void)connection;  /* Unused. Silent compiler warning. */
  (void)toe;         /* Unused. Silent compiler warning. */

  if (NULL == tv)
    return;
  gettimeofday (&tve, NULL);

  delta = 0;
  if (tve.tv_usec >= tv->tv_usec)
    delta += (tve.tv_sec - tv->tv_sec) * 1000000LL
      + (tve.tv_usec - tv->tv_usec);
  else
    delta += (tve.tv_sec - tv->tv_sec) * 1000000LL
      - tv->tv_usec + tve.tv_usec;
  if (delta < SMALL)
    small_deltas[delta]++;
  else
    fprintf (stdout, "D: %llu 1\n", (unsigned long long) delta);
  free (tv);
}


static void *
uri_logger_cb (void *cls,
	       const char *uri)
{
  struct timeval *tv = malloc (sizeof (struct timeval));
  (void)cls; /* Unused. Silent compiler warning. */
  (void)uri; /* Unused. Silent compiler warning. */

  if (NULL != tv)
    gettimeofday (tv, NULL);
  return tv;
}


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)url;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */
  (void)ptr;               /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, "GET"))
    return MHD_NO;              /* unexpected method */
  return MHD_queue_response (connection, MHD_HTTP_OK, response);
}


int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  unsigned int i;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  response = MHD_create_response_from_buffer (strlen (PAGE),
					      (void *) PAGE,
					      MHD_RESPMEM_PERSISTENT);
#if 0
  (void) MHD_add_response_header (response,
				  MHD_HTTP_HEADER_CONNECTION,
				  "close");
#endif
  d = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SUPPRESS_DATE_NO_CLOCK
#ifdef EPOLL_SUPPORT
			| MHD_USE_EPOLL | MHD_USE_TURBO
#endif
			,
                        atoi (argv[1]),
                        NULL, NULL, &ahc_echo, NULL,
			MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
			MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) NUMBER_OF_THREADS,
			MHD_OPTION_URI_LOG_CALLBACK, &uri_logger_cb, NULL,
			MHD_OPTION_NOTIFY_COMPLETED, &completed_callback, NULL,
			MHD_OPTION_CONNECTION_LIMIT, (unsigned int) 1000,
			MHD_OPTION_END);
  if (d == NULL)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon (d);
  MHD_destroy_response (response);
  for (i=0;i<SMALL;i++)
    if (0 != small_deltas[i])
      fprintf (stdout, "D: %d %u\n", i, small_deltas[i]);
  return 0;
}
