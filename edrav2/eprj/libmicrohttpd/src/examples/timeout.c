/*
     This file is part of libmicrohttpd
     Copyright (C) 2016, 2017 Christian Grothoff,
     Silvio Clecio (silvioprog), Karlson2k (Evgeny Grin)

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
 * @file timeout.c
 * @brief example for how to use libmicrohttpd request timeout
 * @author Christian Grothoff, Silvio Clecio (silvioprog), Karlson2k (Evgeny Grin)
 */

#include <microhttpd.h>
#include <stdio.h>
#include <string.h>

#define PORT 8080

static int
answer_to_connection(void *cls,
                     struct MHD_Connection *connection,
                     const char *url,
                     const char *method,
                     const char *version,
                     const char *upload_data,
                     size_t *upload_data_size,
                     void **con_cls)
{
  const char *page = "<html><body>Hello timeout!</body></html>";
  struct MHD_Response *response;
  int ret;
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)url;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)method;            /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */
  (void)con_cls;           /* Unused. Silent compiler warning. */

  response = MHD_create_response_from_buffer (strlen(page),
                                              (void *) page,
                                              MHD_RESPMEM_PERSISTENT);
  MHD_add_response_header (response,
                           MHD_HTTP_HEADER_CONTENT_TYPE,
                           "text/html");
  ret = MHD_queue_response (connection,
                            MHD_HTTP_OK,
                            response);
  MHD_destroy_response(response);
  return ret;
}


int
main (void)
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD,
                             PORT,
                             NULL, NULL,
                             &answer_to_connection, NULL,
                             /* 3 seconds */
                             MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 3,
                             MHD_OPTION_END);
  if (NULL == daemon)
    return 1;
  (void) getchar();
  MHD_stop_daemon (daemon);
  return 0;
}
