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
 * @file lib/request_info.c
 * @brief implementation of MHD_request_get_information_sz()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Obtain information about the given request.
 * Use wrapper macro #MHD_request_get_information() instead of direct use
 * of this function.
 *
 * @param request what request to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @param return_value_size size of union MHD_RequestInformation at compile
 *                          time
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
enum MHD_Bool
MHD_request_get_information_sz (struct MHD_Request *request,
			        enum MHD_RequestInformationType info_type,
			        union MHD_RequestInformation *return_value,
			        size_t return_value_size)
{
#define CHECK_SIZE(type) if (sizeof(type) < return_value_size)	\
    return MHD_NO

  switch (info_type)
  {
  case MHD_REQUEST_INFORMATION_CONNECTION:
    CHECK_SIZE (struct MHD_Connection *);
    return_value->connection = request->connection;
    return MHD_YES;
  case MHD_REQUEST_INFORMATION_CLIENT_CONTEXT:
    CHECK_SIZE (void **);
    return_value->request_context = &request->client_context;
    return MHD_YES;
  case MHD_REQUEST_INFORMATION_HTTP_VERSION:
    CHECK_SIZE (const char *);
    return_value->http_version = request->version_s;
    return MHD_YES;
  case MHD_REQUEST_INFORMATION_HTTP_METHOD:
    CHECK_SIZE (const char *);
    return_value->http_method = request->method_s;
    return MHD_YES;
  case MHD_REQUEST_INFORMATION_HEADER_SIZE:
    CHECK_SIZE (size_t);
    if ( (MHD_REQUEST_HEADERS_RECEIVED > request->state) ||
	 (MHD_REQUEST_CLOSED == request->state) )
        return MHD_NO; /* invalid, too early! */
    return_value->header_size = request->header_size;
    return MHD_YES;

  default:
    return MHD_NO;
  }

#undef CHECK_SIZE
}


/* end of request_info.c */
