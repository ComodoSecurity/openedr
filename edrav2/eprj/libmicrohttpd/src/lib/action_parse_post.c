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
 * @file lib/action_parse_post.c
 * @brief implementation of MHD_action_parse_post()
 * @author Christian Grothoff
 */
#include "internal.h"



/**
 * Create an action that parses a POST request.
 *
 * This action can be used to (incrementally) parse the data portion
 * of a POST request.  Note that some buggy browsers fail to set the
 * encoding type.  If you want to support those, you may have to call
 * #MHD_set_connection_value with the proper encoding type before
 * returning this action (if no supported encoding type is detected,
 * returning this action will cause a bad request to be returned to
 * the client).
 *
 * @param buffer_size maximum number of bytes to use for
 *        internal buffering (used only for the parsing,
 *        specifically the parsing of the keys).  A
 *        tiny value (256-1024) should be sufficient.
 *        Do NOT use a value smaller than 256.  For good
 *        performance, use 32 or 64k (i.e. 65536).
 * @param iter iterator to be called with the parsed data,
 *        Must NOT be NULL.
 * @param iter_cls first argument to @a iter
 * @return NULL on error (out of memory, unsupported encoding),
 *         otherwise a PP handle
 * @ingroup request
 */
const struct MHD_Action *
MHD_action_parse_post (size_t buffer_size,
		       MHD_PostDataIterator iter,
		       void *iter_cls)
{
  return NULL; /* not yet implemented */
}

/* end of action_parse_post.c */
