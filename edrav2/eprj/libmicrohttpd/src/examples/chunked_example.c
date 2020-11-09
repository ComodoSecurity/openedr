/*
     This file is part of libmicrohttpd
     Copyright (C) 2015 Christian Grothoff (and other contributing authors)

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
 * @file chunked_example.c
 * @brief example for generating chunked encoding with libmicrohttpd
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */

#include "platform.h"
#include <microhttpd.h>

struct ResponseContentCallbackParam
{
  const char *response_data;
  size_t response_size;
};


static ssize_t
callback (void *cls,
          uint64_t pos,
          char *buf,
          size_t buf_size)
{
  size_t size_to_copy;
  struct ResponseContentCallbackParam * const param =
      (struct ResponseContentCallbackParam *)cls;

  /* Note: 'pos' will never exceed size of transmitted data. */
  /* You can use 'pos == param->response_size' in next check. */
  if (pos >= param->response_size)
    { /* Whole response was sent. Signal end of response. */
      return MHD_CONTENT_READER_END_OF_STREAM;
    }

  /* Pseudo code.        *
  if (data_not_ready)
    {
      // Callback will be called again on next loop.
      // Consider suspending connection until data will be ready.
      return 0;
    }
   * End of pseudo code. */

  if (buf_size < (param->response_size - pos))
    size_to_copy = buf_size;
  else
    size_to_copy = param->response_size - pos;

  memcpy (buf, param->response_data + pos, size_to_copy);

  /* Pseudo code.        *
  if (error_preparing_response)
    {
      // Close connection with error.
      return MHD_CONTENT_READER_END_WITH_ERROR;
    }
   * End of pseudo code. */

  /* Return amount of data copied to buffer. */
  return size_to_copy;
}


static void
free_callback_param (void *cls)
{
  free(cls);
}


static const char simple_response_text[] = "<html><head><title>Simple response</title></head>"
                                           "<body>Simple response text</body></html>";


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data,
          size_t *upload_data_size,
          void **ptr)
{
  static int aptr;
  struct ResponseContentCallbackParam *callback_param;
  struct MHD_Response *response;
  int ret;
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)url;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, "GET"))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }

  callback_param = malloc (sizeof(struct ResponseContentCallbackParam));
  if (NULL == callback_param)
    return MHD_NO; /* Not enough memory. */

  callback_param->response_data = simple_response_text;
  callback_param->response_size = (sizeof(simple_response_text)/sizeof(char)) - 1;

  *ptr = NULL;                  /* reset when done */
  response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN,
                                                1024,
                                                &callback,
                                                callback_param,
                                                &free_callback_param);
  if (NULL == response)
  {
    free (callback_param);
    return MHD_NO;
  }
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);
  return ret;
}


int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  int port;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  port = atoi (argv[1]);
  if ( (1 > port) ||
       (port > UINT16_MAX) )
    {
      fprintf (stderr,
               "Port must be a number between 1 and 65535\n");
      return 1;
    }
  d = MHD_start_daemon (/* MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG, */
                        MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        /* MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | MHD_USE_POLL, */
			/* MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG | MHD_USE_POLL, */
			/* MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG, */
                        (uint16_t) port,
                        NULL, NULL,
                        &ahc_echo, NULL,
			MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
			MHD_OPTION_END);
  if (NULL == d)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
