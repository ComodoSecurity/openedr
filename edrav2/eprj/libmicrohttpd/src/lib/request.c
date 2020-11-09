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
 * @file requests.c
 * @brief  Methods for managing HTTP requests
 * @author Daniel Pittman
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */
#include "internal.h"


/**
 * Get all of the headers from the request.
 *
 * @param request request to get values from
 * @param kind types of values to iterate over, can be a bitmask
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to @a iterator
 * @return number of entries iterated over
 * @ingroup request
 */
unsigned int
MHD_request_get_values (struct MHD_Request *request,
			enum MHD_ValueKind kind,
			MHD_KeyValueIterator iterator,
			void *iterator_cls)
{
  int ret;
  struct MHD_HTTP_Header *pos;

  ret = 0;
  for (pos = request->headers_received;
       NULL != pos;
       pos = pos->next)
    {
      if (0 != (pos->kind & kind))
	{
	  ret++;
	  if ( (NULL != iterator) &&
	       (MHD_YES != iterator (iterator_cls,
				     pos->kind,
				     pos->header,
				     pos->value)) )
	    return ret;
	}
    }
  return ret;
}


/**
 * This function can be used to add an entry to the HTTP headers of a
 * request (so that the #MHD_request_get_values function will
 * return them -- and the `struct MHD_PostProcessor` will also see
 * them).  This maybe required in certain situations (see Mantis
 * #1399) where (broken) HTTP implementations fail to supply values
 * needed by the post processor (or other parts of the application).
 *
 * This function MUST only be called from within the
 * request callbacks (otherwise, access maybe improperly
 * synchronized).  Furthermore, the client must guarantee that the key
 * and value arguments are 0-terminated strings that are NOT freed
 * until the connection is closed.  (The easiest way to do this is by
 * passing only arguments to permanently allocated strings.).
 *
 * @param request the request for which a
 *  value should be set
 * @param kind kind of the value
 * @param key key for the value
 * @param value the value itself
 * @return #MHD_NO if the operation could not be
 *         performed due to insufficient memory;
 *         #MHD_YES on success
 * @ingroup request
 */
enum MHD_Bool
MHD_request_set_value (struct MHD_Request *request,
		       enum MHD_ValueKind kind,
		       const char *key,
		       const char *value)
{
  struct MHD_HTTP_Header *pos;

  pos = MHD_pool_allocate (request->connection->pool,
                           sizeof (struct MHD_HTTP_Header),
                           MHD_YES);
  if (NULL == pos)
    return MHD_NO;
  pos->header = (char *) key;
  pos->value = (char *) value;
  pos->kind = kind;
  pos->next = NULL;
  /* append 'pos' to the linked list of headers */
  if (NULL == request->headers_received_tail)
    {
      request->headers_received = pos;
      request->headers_received_tail = pos;
    }
  else
    {
      request->headers_received_tail->next = pos;
      request->headers_received_tail = pos;
    }
  return MHD_YES;
}


/**
 * Get a particular header value.  If multiple
 * values match the kind, return any one of them.
 *
 * @param request request to get values from
 * @param kind what kind of value are we looking for
 * @param key the header to look for, NULL to lookup 'trailing' value without a key
 * @return NULL if no such item was found
 * @ingroup request
 */
const char *
MHD_request_lookup_value (struct MHD_Request *request,
			  enum MHD_ValueKind kind,
			  const char *key)
{
  struct MHD_HTTP_Header *pos;

  for (pos = request->headers_received;
       NULL != pos;
       pos = pos->next)
    {
      if ((0 != (pos->kind & kind)) &&
	  ( (key == pos->header) ||
	    ( (NULL != pos->header) &&
	      (NULL != key) &&
	      (MHD_str_equal_caseless_(key,
				       pos->header)))))
	return pos->value;
    }
  return NULL;
}


/* end of request.c */


