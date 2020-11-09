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
 * @file http_chunked_compression.c
 * @brief example for how to compress a chunked HTTP response
 * @author Silvio Clecio (silvioprog)
 */

#include "platform.h"
#include <zlib.h>
#include <microhttpd.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif /* HAVE_LIMITS_H */
#include <stddef.h>

#ifndef SSIZE_MAX
#ifdef __SSIZE_MAX__
#define SSIZE_MAX __SSIZE_MAX__
#elif defined(PTRDIFF_MAX)
#define SSIZE_MAX PTRDIFF_MAX
#elif defined(INTPTR_MAX)
#define SSIZE_MAX INTPTR_MAX
#else
#define SSIZE_MAX ((ssize_t)(((size_t)-1)>>1))
#endif
#endif /* ! SSIZE_MAX */

#define CHUNK 16384

struct Holder {
    FILE *file;
    z_stream stream;
    void *buf;
};

static int
compress_buf (z_stream *strm, const void *src, size_t src_size, size_t *offset, void **dest, size_t *dest_size,
              void *tmp)
{
  unsigned int have;
  int ret;
  int flush;
  void *tmp_dest;
  *dest = NULL;
  *dest_size = 0;
  do
    {
      if (src_size > CHUNK)
        {
          strm->avail_in = CHUNK;
          src_size -= CHUNK;
          flush = Z_NO_FLUSH;
        }
      else
        {
          strm->avail_in = (uInt) src_size;
          flush = Z_SYNC_FLUSH;
        }
      *offset += strm->avail_in;
      strm->next_in = (Bytef *) src;
      do
        {
          strm->avail_out = CHUNK;
          strm->next_out = tmp;
          ret = deflate (strm, flush);
          have = CHUNK - strm->avail_out;
          *dest_size += have;
          tmp_dest = realloc (*dest, *dest_size);
          if (NULL == tmp_dest)
            {
              free (*dest);
              *dest = NULL;
              return MHD_NO;
            }
          *dest = tmp_dest;
          memcpy ((*dest) + ((*dest_size) - have), tmp, have);
        }
      while (0 == strm->avail_out);
    }
  while (flush != Z_SYNC_FLUSH);
  return (Z_OK == ret) ? MHD_YES : MHD_NO;
}

static ssize_t
read_cb (void *cls, uint64_t pos, char *mem, size_t size)
{
  struct Holder *holder = cls;
  void *src;
  void *buf;
  ssize_t ret;
  size_t offset;
  if (pos > SSIZE_MAX)
    return MHD_CONTENT_READER_END_WITH_ERROR;
  offset = (size_t) pos;
  src = malloc (size);
  if (NULL == src)
    return MHD_CONTENT_READER_END_WITH_ERROR;
  ret = fread (src, 1, size, holder->file);
  if (ret < 0)
    {
      ret = MHD_CONTENT_READER_END_WITH_ERROR;
      goto done;
    }
  if (0 == size)
    {
      ret = MHD_CONTENT_READER_END_OF_STREAM;
      goto done;
    }
  if (MHD_YES != compress_buf (&holder->stream, src, ret, &offset, &buf, &size, holder->buf))
    ret = MHD_CONTENT_READER_END_WITH_ERROR;
  else
    {
      memcpy (mem, buf, size);
      ret = size;
    }
  free (buf); /* Buf may be set even on error return. */
done:
  free (src);
  return ret;
}

static void
free_cb (void *cls)
{
  struct Holder *holder = cls;
  fclose (holder->file);
  deflateEnd (&holder->stream);
  free (holder->buf);
  free (holder);
}

static int
ahc_echo (void *cls, struct MHD_Connection *con, const char *url, const char *method, const char *version,
          const char *upload_data, size_t *upload_size, void **ptr)
{
  struct Holder *holder;
  struct MHD_Response *res;
  int ret;
  (void) cls;
  (void) url;
  (void) method;
  (void) version;
  (void) upload_data;
  (void) upload_size;
  if (NULL == *ptr)
    {
      *ptr = (void *) 1;
      return MHD_YES;
    }
  *ptr = NULL;
  holder = calloc (1, sizeof (struct Holder));
  if (!holder)
    return MHD_NO;
  holder->file = fopen (__FILE__, "rb");
  if (NULL == holder->file)
    goto file_error;
  ret = deflateInit(&holder->stream, Z_BEST_COMPRESSION);
  if (ret != Z_OK)
    goto stream_error;
  holder->buf = malloc (CHUNK);
  if (NULL == holder->buf)
    goto buf_error;
  res = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN, 1024, &read_cb, holder, &free_cb);
  if (NULL == res)
    goto error;
  ret = MHD_add_response_header (res, MHD_HTTP_HEADER_CONTENT_ENCODING, "deflate");
  if (MHD_YES != ret)
    goto res_error;
  ret = MHD_add_response_header (res, MHD_HTTP_HEADER_CONTENT_TYPE, "text/x-c");
  if (MHD_YES != ret)
    goto res_error;
  ret = MHD_queue_response (con, MHD_HTTP_OK, res);
res_error:
  MHD_destroy_response (res);
  return ret;
error:
  free (holder->buf);
buf_error:
  deflateEnd (&holder->stream);
stream_error:
  fclose (holder->file);
file_error:
  free (holder);
  return MHD_NO;
}

int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  unsigned int port;
  if ((argc != 2) ||
      (1 != sscanf (argv[1], "%u", &port)) ||
      (UINT16_MAX < port))
    {
      fprintf (stderr, "%s PORT\n", argv[0]);
      return 1;
    }
  d = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD, (uint16_t) port, NULL, NULL,
                        &ahc_echo, NULL,
                        MHD_OPTION_END);
  if (NULL == d)
    return 1;
  if (0 == port)
    MHD_get_daemon_info (d, MHD_DAEMON_INFO_BIND_PORT, &port);
  fprintf (stdout, "HTTP server running at http://localhost:%u\n\nPress ENTER to stop the server ...\n", port);
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
