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
 * @file lib/response_from_callback.c
 * @brief implementation of MHD_response_from_callback()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Create a response action.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param sc status code to return
 * @param size size of the data portion of the response, #MHD_SIZE_UNKNOWN for unknown
 * @param block_size preferred block size for querying crc (advisory only,
 *                   MHD may still call @a crc using smaller chunks); this
 *                   is essentially the buffer size used for IO, clients
 *                   should pick a value that is appropriate for IO and
 *                   memory performance requirements
 * @param crc callback to use to obtain response data
 * @param crc_cls extra argument to @a crc
 * @param crfc callback to call to free @a crc_cls resources
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
struct MHD_Response *
MHD_response_from_callback (enum MHD_HTTP_StatusCode sc,
			    uint64_t size,
			    size_t block_size,
			    MHD_ContentReaderCallback crc,
			    void *crc_cls,
			    MHD_ContentReaderFreeCallback crfc)
{
  struct MHD_Response *response;

  mhd_assert (NULL != crc);
  mhd_assert (0 != block_size);
  if (NULL ==
      (response = MHD_calloc_ (1,
			       sizeof (struct MHD_Response) +
			       block_size)))
    return NULL;
  response->fd = -1;
  response->status_code = sc;
  response->data = (void *) &response[1];
  response->data_buffer_size = block_size;
  if (! MHD_mutex_init_ (&response->mutex))
  {
    free (response);
    return NULL;
  }
  response->crc = crc;
  response->crfc = crfc;
  response->crc_cls = crc_cls;
  response->reference_count = 1;
  response->total_size = size;
  return response;
}


/* end of response_from_callback.c */
