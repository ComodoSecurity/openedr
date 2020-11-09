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
 * @file lib/response.c
 * @brief implementation of general response functions
 * @author Daniel Pittman
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */
#include "internal.h"


/**
 * Add a header or footer line to the response.
 *
 * @param response response to add a header to
 * @param kind header or footer
 * @param header the header to add
 * @param content value to add
 * @return false on error (i.e. invalid header or content format).
 */
static bool
add_response_entry (struct MHD_Response *response,
		    enum MHD_ValueKind kind,
		    const char *header,
		    const char *content)
{
  struct MHD_HTTP_Header *hdr;

  if ( (NULL == header) ||
       (NULL == content) ||
       (0 == header[0]) ||
       (0 == content[0]) ||
       (NULL != strchr (header, '\t')) ||
       (NULL != strchr (header, '\r')) ||
       (NULL != strchr (header, '\n')) ||
       (NULL != strchr (content, '\t')) ||
       (NULL != strchr (content, '\r')) ||
       (NULL != strchr (content, '\n')) )
    return false;
  if (NULL == (hdr = malloc (sizeof (struct MHD_HTTP_Header))))
    return false;
  if (NULL == (hdr->header = strdup (header)))
    {
      free (hdr);
      return false;
    }
  if (NULL == (hdr->value = strdup (content)))
    {
      free (hdr->header);
      free (hdr);
      return false;
    }
  hdr->kind = kind;
  hdr->next = response->first_header;
  response->first_header = hdr;
  return true;
}


/**
 * Explicitly decrease reference counter of a response object.  If the
 * counter hits zero, destroys a response object and associated
 * resources.  Usually, this is implicitly done by converting a
 * response to an action and returning the action to MHD.
 *
 * @param response response to decrement RC of
 * @ingroup response
 */
void
MHD_response_queue_for_destroy (struct MHD_Response *response)
{
  struct MHD_HTTP_Header *pos;

  MHD_mutex_lock_chk_ (&response->mutex);
  if (0 != --(response->reference_count))
    {
      MHD_mutex_unlock_chk_ (&response->mutex);
      return;
    }
  MHD_mutex_unlock_chk_ (&response->mutex);
  MHD_mutex_destroy_chk_ (&response->mutex);
  if (NULL != response->crfc)
    response->crfc (response->crc_cls);
  while (NULL != response->first_header)
    {
      pos = response->first_header;
      response->first_header = pos->next;
      free (pos->header);
      free (pos->value);
      free (pos);
    }
  free (response);
}


/**
 * Add a header line to the response.
 *
 * @param response response to add a header to
 * @param header the header to add
 * @param content value to add
 * @return #MHD_NO on error (i.e. invalid header or content format),
 *         or out of memory
 * @ingroup response
 */
enum MHD_Bool
MHD_response_add_header (struct MHD_Response *response,
                         const char *header,
			 const char *content)
{
  return add_response_entry (response,
			     MHD_HEADER_KIND,
			     header,
			     content) ? MHD_YES : MHD_NO;
}


/**
 * Add a tailer line to the response.
 *
 * @param response response to add a footer to
 * @param footer the footer to add
 * @param content value to add
 * @return #MHD_NO on error (i.e. invalid footer or content format),
 *         or out of memory
 * @ingroup response
 */
enum MHD_Bool
MHD_response_add_trailer (struct MHD_Response *response,
                          const char *footer,
                          const char *content)
{
  return add_response_entry (response,
			     MHD_FOOTER_KIND,
			     footer,
			     content) ? MHD_YES : MHD_NO;
}


/**
 * Delete a header (or footer) line from the response.
 *
 * @param response response to remove a header from
 * @param header the header to delete
 * @param content value to delete
 * @return #MHD_NO on error (no such header known)
 * @ingroup response
 */
enum MHD_Bool
MHD_response_del_header (struct MHD_Response *response,
                         const char *header,
			 const char *content)
{
  struct MHD_HTTP_Header *pos;
  struct MHD_HTTP_Header *prev;

  prev = NULL;
  pos = response->first_header;
  while (NULL != pos)
    {
      if ((0 == strcmp (header,
                        pos->header)) &&
          (0 == strcmp (content,
                        pos->value)))
        {
          free (pos->header);
          free (pos->value);
          if (NULL == prev)
            response->first_header = pos->next;
          else
            prev->next = pos->next;
          free (pos);
          return MHD_YES;
        }
      prev = pos;
      pos = pos->next;
    }
  return MHD_NO;
}


/**
 * Get all of the headers (and footers) added to a response.
 *
 * @param response response to query
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to @a iterator
 * @return number of entries iterated over
 * @ingroup response
 */
unsigned int
MHD_response_get_headers (struct MHD_Response *response,
                          MHD_KeyValueIterator iterator,
			  void *iterator_cls)
{
  unsigned int numHeaders = 0;
  struct MHD_HTTP_Header *pos;

  for (pos = response->first_header;
       NULL != pos;
       pos = pos->next)
    {
      numHeaders++;
      if ( (NULL != iterator) &&
	   (MHD_YES != iterator (iterator_cls,
				 pos->kind,
				 pos->header,
				 pos->value)) )
        break;
    }
  return numHeaders;
}


/**
 * Get a particular header (or footer) from the response.
 *
 * @param response response to query
 * @param key which header to get
 * @return NULL if header does not exist
 * @ingroup response
 */
const char *
MHD_response_get_header (struct MHD_Response *response,
			 const char *key)
{
  struct MHD_HTTP_Header *pos;

  for (pos = response->first_header;
       NULL != pos;
       pos = pos->next)
    {
      if (MHD_str_equal_caseless_ (pos->header,
				   key))
        return pos->value;
    }
  return NULL;
}

/* end of response.c */
