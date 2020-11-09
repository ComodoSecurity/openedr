/*
  This file is part of libmicrohttpd
  Copyright (C) 2007-2018 Daniel Pittman and Christian Grothoff

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
 * @file lib/response_from_buffer.c
 * @brief implementation of MHD_response_from_buffer()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Create a response object.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param sc status code to use for the response;
 *           #MHD_HTTP_NO_CONTENT is only valid if @a size is 0;
 * @param size size of the data portion of the response
 * @param buffer size bytes containing the response's data portion
 * @param mode flags for buffer management
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
struct MHD_Response *
MHD_response_from_buffer (enum MHD_HTTP_StatusCode sc,
			  size_t size,
			  void *buffer,
			  enum MHD_ResponseMemoryMode mode)
{
  struct MHD_Response *response;
  void *tmp;

  mhd_assert ( (NULL != buffer) ||
	       (0 == size) );
  if (NULL ==
      (response = MHD_calloc_ (1,
			       sizeof (struct MHD_Response))))
    return NULL;
  response->fd = -1;
  if (! MHD_mutex_init_ (&response->mutex))
    {
      free (response);
      return NULL;
    }
  if ( (MHD_RESPMEM_MUST_COPY == mode) &&
       (size > 0) )
    {
      if (NULL == (tmp = malloc (size)))
        {
          MHD_mutex_destroy_chk_ (&response->mutex);
          free (response);
          return NULL;
        }
      memcpy (tmp,
	      buffer,
	      size);
      buffer = tmp;
    }
  if (MHD_RESPMEM_PERSISTENT != mode)
    {
      response->crfc = &free;
      response->crc_cls = buffer;
    }
  response->status_code = sc;
  response->reference_count = 1;
  response->total_size = size;
  response->data = buffer;
  response->data_size = size;
  return response;
}

/* end of response_from_buffer.c */
