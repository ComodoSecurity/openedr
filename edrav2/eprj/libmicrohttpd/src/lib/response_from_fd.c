/*
  This file is part of libmicrohttpd
  Copyright (C) 2007-2019 Daniel Pittman, Christian Grothoff and
  Karlson2k (Evgeny Grin)

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
 * @file lib/response_from_fd.c
 * @brief implementation of MHD_response_from_fd()
 * @author Daniel Pittman
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */
#include "internal.h"


/**
 * Size of single file read operation for
 * file-backed responses.
 */
#ifndef MHD_FILE_READ_BLOCK_SIZE
#ifdef _WIN32
#define MHD_FILE_READ_BLOCK_SIZE 16384 /* 16k */
#else /* _WIN32 */
#define MHD_FILE_READ_BLOCK_SIZE 4096 /* 4k */
#endif /* _WIN32 */
#endif /* !MHD_FD_BLOCK_SIZE */

/**
 * Given a file descriptor, read data from the file
 * to generate the response.
 *
 * @param cls pointer to the response
 * @param pos offset in the file to access
 * @param buf where to write the data
 * @param max number of bytes to write at most
 * @return number of bytes written
 */
static ssize_t
file_reader (void *cls,
             uint64_t pos,
             char *buf,
             size_t max)
{
  struct MHD_Response *response = cls;
#if !defined(_WIN32) || defined(__CYGWIN__)
  ssize_t n;
#else  /* _WIN32 && !__CYGWIN__ */
  const HANDLE fh = (HANDLE) _get_osfhandle (response->fd);
#endif /* _WIN32 && !__CYGWIN__ */
  const int64_t offset64 = (int64_t)(pos + response->fd_off);

  if (offset64 < 0)
    return MHD_CONTENT_READER_END_WITH_ERROR; /* seek to required position is not possible */

#if !defined(_WIN32) || defined(__CYGWIN__)
  if (max > SSIZE_MAX)
    max = SSIZE_MAX; /* Clamp to maximum return value. */

#if defined(HAVE_PREAD64)
  n = pread64 (response->fd,
	       buf,
	       max,
	       offset64);
#elif defined(HAVE_PREAD)
  if ( (sizeof(off_t) < sizeof (uint64_t)) &&
       (offset64 > (uint64_t)INT32_MAX) )
    return MHD_CONTENT_READER_END_WITH_ERROR; /* Read at required position is not possible. */

  n = pread (response->fd,
	     buf,
	     max,
	     (off_t) offset64);
#else  /* ! HAVE_PREAD */
#if defined(HAVE_LSEEK64)
  if (lseek64 (response->fd,
               offset64,
               SEEK_SET) != offset64)
    return MHD_CONTENT_READER_END_WITH_ERROR; /* can't seek to required position */
#else  /* ! HAVE_LSEEK64 */
  if ( (sizeof(off_t) < sizeof (uint64_t)) &&
       (offset64 > (uint64_t)INT32_MAX) )
    return MHD_CONTENT_READER_END_WITH_ERROR; /* seek to required position is not possible */

  if (lseek (response->fd,
             (off_t) offset64,
             SEEK_SET) != (off_t) offset64)
    return MHD_CONTENT_READER_END_WITH_ERROR; /* can't seek to required position */
#endif /* ! HAVE_LSEEK64 */
  n = read (response->fd,
            buf,
            max);

#endif /* ! HAVE_PREAD */
  if (0 == n)
    return MHD_CONTENT_READER_END_OF_STREAM;
  if (n < 0)
    return MHD_CONTENT_READER_END_WITH_ERROR;
  return n;
#else /* _WIN32 && !__CYGWIN__ */
  if (INVALID_HANDLE_VALUE == fh)
    return MHD_CONTENT_READER_END_WITH_ERROR; /* Value of 'response->fd' is not valid. */
  else
    {
      OVERLAPPED f_ol = {0, 0, {{0, 0}}, 0}; /* Initialize to zero. */
      ULARGE_INTEGER pos_uli;
      DWORD toRead = (max > INT32_MAX) ? INT32_MAX : (DWORD) max;
      DWORD resRead;

      pos_uli.QuadPart = (uint64_t) offset64; /* Simple transformation 64bit -> 2x32bit. */
      f_ol.Offset = pos_uli.LowPart;
      f_ol.OffsetHigh = pos_uli.HighPart;
      if (! ReadFile (fh,
		      (void*)buf,
		      toRead,
		      &resRead,
		      &f_ol))
        return MHD_CONTENT_READER_END_WITH_ERROR; /* Read error. */
      if (0 == resRead)
        return MHD_CONTENT_READER_END_OF_STREAM;
      return (ssize_t) resRead;
    }
#endif /* _WIN32 && !__CYGWIN__ */
}


/**
 * Destroy file reader context.  Closes the file
 * descriptor.
 *
 * @param cls pointer to file descriptor
 */
static void
free_callback (void *cls)
{
  struct MHD_Response *response = cls;

  (void) close (response->fd);
  response->fd = -1;
}


/**
 * Create a response object based on an @a fd from which
 * data is read.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param sc status code to return
 * @param fd file descriptor referring to a file on disk with the
 *        data; will be closed when response is destroyed;
 *        fd should be in 'blocking' mode
 * @param offset offset to start reading from in the file;
 *        reading file beyond 2 GiB may be not supported by OS or
 *        MHD build; see ::MHD_FEATURE_LARGE_FILE
 * @param size size of the data portion of the response;
 *        sizes larger than 2 GiB may be not supported by OS or
 *        MHD build; see ::MHD_FEATURE_LARGE_FILE
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
struct MHD_Response *
MHD_response_from_fd (enum MHD_HTTP_StatusCode sc,
		      int fd,
		      uint64_t offset,
		      uint64_t size)
{
  struct MHD_Response *response;

  mhd_assert (-1 != fd);
#if !defined(HAVE___LSEEKI64) && !defined(HAVE_LSEEK64)
  if ( (sizeof (uint64_t) > sizeof (off_t)) &&
       ( (size > (uint64_t)INT32_MAX) ||
         (offset > (uint64_t)INT32_MAX) ||
         ((size + offset) >= (uint64_t)INT32_MAX) ) )
    return NULL;
#endif
  if ( ((int64_t) size < 0) ||
       ((int64_t) offset < 0) ||
       ((int64_t) (size + offset) < 0) )
    return NULL;

  response = MHD_response_from_callback (sc,
					 size,
					 MHD_FILE_READ_BLOCK_SIZE,
					 &file_reader,
					 NULL,
					 &free_callback);
  if (NULL == response)
    return NULL;
  response->fd = fd;
  response->fd_off = offset;
  response->crc_cls = response;
  return response;
}

/* end of response_from_fd.c */

