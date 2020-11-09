/*
     This file is part of libmicrohttpd
     Copyright (C) 2019 Christian Grothoff (and other contributing authors)

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
 * @file http_compression.c
 * @brief minimal example for how to compress HTTP response
 * @author Silvio Clecio (silvioprog)
 */

#include "platform.h"
#include <zlib.h>
#include <microhttpd.h>

#define PAGE                                                        \
  "<html><head><title>HTTP compression</title></head><body>Hello, " \
  "hello, hello. This is a 'hello world' message for the world, "   \
  "repeat, for the world.</body></html>"

static int
can_compress (struct MHD_Connection *con)
{
  const char *ae;
  const char *de;

  ae = MHD_lookup_connection_value (con,
                                    MHD_HEADER_KIND,
                                    MHD_HTTP_HEADER_ACCEPT_ENCODING);
  if (NULL == ae)
    return MHD_NO;
  if (0 == strcmp (ae,
                   "*"))
    return MHD_YES;
  de = strstr (ae,
               "deflate");
  if (NULL == de)
    return MHD_NO;
  if (((de == ae) ||
       (de[-1] == ',') ||
       (de[-1] == ' ')) &&
      ((de[strlen ("deflate")] == '\0') ||
       (de[strlen ("deflate")] == ',') ||
       (de[strlen ("deflate")] == ';')))
    return MHD_YES;
  return MHD_NO;
}

static int
body_compress (void **buf,
               size_t *buf_size)
{
  Bytef *cbuf;
  uLongf cbuf_size;
  int ret;

  cbuf_size = compressBound (*buf_size);
  cbuf = malloc (cbuf_size);
  if (NULL == cbuf)
    return MHD_NO;
  ret = compress (cbuf,
                  &cbuf_size,
                  (const Bytef *) *buf,
                  *buf_size);
  if ((Z_OK != ret) ||
      (cbuf_size >= *buf_size))
    {
      /* compression failed */
      free (cbuf);
      return MHD_NO;
    }
  free (*buf);
  *buf = (void *) cbuf;
  *buf_size = (size_t) cbuf_size;
  return MHD_YES;
}

static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  struct MHD_Response *response;
  int ret;
  int comp;
  size_t body_len;
  char *body_str;
  (void) cls;               /* Unused. Silent compiler warning. */
  (void) url;               /* Unused. Silent compiler warning. */
  (void) version;           /* Unused. Silent compiler warning. */
  (void) upload_data;       /* Unused. Silent compiler warning. */
  (void) upload_data_size;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, "GET"))
    return MHD_NO; /* unexpected method */
  if (!*ptr)
    {
      *ptr = (void *) 1;
      return MHD_YES;
    }
  *ptr = NULL;

  body_str = strdup (PAGE);
  if (NULL == body_str)
    {
      return MHD_NO;
    }
  body_len = strlen (body_str);
  /* try to compress the body */
  comp = MHD_NO;
  if (MHD_YES ==
      can_compress (connection))
    comp = body_compress ((void **) &body_str,
                          &body_len);
  response = MHD_create_response_from_buffer (body_len,
                                              body_str,
                                              MHD_RESPMEM_MUST_FREE);
  if (NULL == response)
    {
      free (body_str);
      return MHD_NO;
    }

  if (MHD_YES == comp)
    {
      /* Need to indicate to client that body is compressed */
      if (MHD_NO ==
          MHD_add_response_header (response,
                                   MHD_HTTP_HEADER_CONTENT_ENCODING,
                                   "deflate"))
        {
          MHD_destroy_response (response);
          return MHD_NO;
        }
    }
  ret = MHD_queue_response (connection,
                            200,
                            response);
  MHD_destroy_response (response);
  return ret;
}

int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  d = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        atoi (argv[1]), NULL, NULL,
                        &ahc_echo, NULL,
                        MHD_OPTION_END);
  if (NULL == d)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
