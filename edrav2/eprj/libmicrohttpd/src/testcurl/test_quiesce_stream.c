/*
     This file is part of libmicrohttpd
     Copyright (C) 2016 Christian Grothoff

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
 * @file test_quiesce_stream.c
 * @brief  Testcase for libmicrohttpd quiescing
 * @author Markus Doppelbauer
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */
#include "mhd_options.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <microhttpd.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#elif defined(_WIN32)
#include <windows.h>
#define sleep(s) (Sleep((s)*1000), 0)
#endif /* _WIN32 */


static volatile unsigned int request_counter;


static void
http_PanicCallback (void *cls,
                    const char *file,
                    unsigned int line,
                    const char *reason)
{
  (void)cls;    /* Unused. Silent compiler warning. */
  fprintf( stderr,
           "PANIC: exit process: %s at %s:%u\n",
           reason,
           file,
           line);
  exit (EXIT_FAILURE);
}


static void *
resume_connection (void *arg)
{
  struct MHD_Connection *connection = arg;

  /* fprintf (stderr, "Calling resume\n"); */
  MHD_resume_connection (connection);
  return NULL;
}


static void
suspend_connection (struct MHD_Connection *connection)
{
  pthread_t thread_id;

  /* fprintf (stderr, "Calling suspend\n"); */
  MHD_suspend_connection (connection);
  int status = pthread_create (&thread_id,
                               NULL,
                               &resume_connection,
                               connection);
  if (0 != status)
    {
      fprintf (stderr,
               "Could not create thead\n");
      exit( EXIT_FAILURE );
    }
  pthread_detach (thread_id);
}


struct ContentReaderUserdata
{
  int bytes_written;
  struct MHD_Connection *connection;
};


static ssize_t
http_ContentReaderCallback (void *cls,
                            uint64_t pos,
                            char *buf,
                            size_t max)
{
  static const char alphabet[] = "\nABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  struct ContentReaderUserdata *userdata = cls;
  (void)pos;(void)max;  /* Unused. Silent compiler warning. */

  if( userdata->bytes_written >= 1024)
    {
      fprintf( stderr,
               "finish: %d\n",
               request_counter);
      return MHD_CONTENT_READER_END_OF_STREAM;
    }
  userdata->bytes_written++;
  buf[0] = alphabet[userdata->bytes_written % (sizeof(alphabet) - 1)];
  suspend_connection (userdata->connection);

  return 1;
}


static void
free_crc_data (void *crc_data)
{
  struct ContentReaderUserdata *userdata = crc_data;

  free (userdata);
}


static int
http_AccessHandlerCallback (void *cls,
                            struct MHD_Connection *connection,
                            const char *url,
                            const char *method,
                            const char *version,
                            const char *upload_data,
                            size_t *upload_data_size,
                            void **con_cls )
{
  int ret;
  (void)cls;(void)url;                          /* Unused. Silent compiler warning. */
  (void)method;(void)version;(void)upload_data; /* Unused. Silent compiler warning. */
  (void)upload_data_size;                       /* Unused. Silent compiler warning. */

  /* Never respond on first call */
  if (NULL == *con_cls)
  {
    fprintf (stderr,
             "start: %d\n",
              ++request_counter);

    struct ContentReaderUserdata *userdata = malloc (sizeof(struct ContentReaderUserdata));

    if (NULL == userdata)
      return MHD_NO;
    userdata->bytes_written = 0;
    userdata->connection = connection;
    *con_cls = userdata;
    return MHD_YES;
  }

  /* Second call: create response */
  struct MHD_Response *response
    = MHD_create_response_from_callback (-1,
                                         32 * 1024,
                                         &http_ContentReaderCallback,
                                         *con_cls,
                                         &free_crc_data);
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response (response);

  suspend_connection (connection);
  return ret;
}


int
main(void)
{
  int port;
  char command_line[1024];

  if (MHD_NO != MHD_is_feature_supported (MHD_FEATURE_AUTODETECT_BIND_PORT))
    port = 0;
  else
    port = 1470;

  /* Panic callback */
  MHD_set_panic_func (&http_PanicCallback,
                      NULL);

  /* Flags */
  unsigned int daemon_flags
    = MHD_USE_INTERNAL_POLLING_THREAD
    | MHD_USE_AUTO
    | MHD_ALLOW_SUSPEND_RESUME
    | MHD_USE_ITC;

  /* Create daemon */
  struct MHD_Daemon *daemon = MHD_start_daemon (daemon_flags,
                                                port,
                                                NULL,
                                                NULL,
                                                &http_AccessHandlerCallback,
                                                NULL,
                                                MHD_OPTION_END);
  if (NULL == daemon)
    return 1;
  if (0 == port)
    {
      const union MHD_DaemonInfo *dinfo;
      dinfo = MHD_get_daemon_info (daemon, MHD_DAEMON_INFO_BIND_PORT);
      if (NULL == dinfo || 0 == dinfo->port)
        { MHD_stop_daemon (daemon); return 32; }
      port = (int)dinfo->port;
    }
  snprintf (command_line,
            sizeof (command_line),
            "curl -s http://127.0.0.1:%d",
            port);

  if (0 != system (command_line))
    {
      MHD_stop_daemon (daemon);
      return 1;
    }
  /* wait for a request */
  while (0 == request_counter)
    (void)sleep (1);

  fprintf (stderr,
           "quiesce\n");
  MHD_quiesce_daemon (daemon);

  /* wait a second */
  (void)sleep (1);

  fprintf (stderr,
           "stopping daemon\n");
  MHD_stop_daemon (daemon);

  return 0;
}
