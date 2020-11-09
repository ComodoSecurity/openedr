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
 * @file lib/response_option.c
 * @brief implementation of response options
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Only respond in conservative HTTP 1.0-mode.  In
 * particular, do not (automatically) sent "Connection" headers and
 * always close the connection after generating the response.
 *
 * @param request the request for which we force HTTP 1.0 to be used
 */
void
MHD_response_option_v10_only (struct MHD_Response *response)
{
  response->v10_only = true;
}


/**
 * Set a function to be called once MHD is finished with the
 * request.
 *
 * @param response which response to set the callback for
 * @param termination_cb function to call
 * @param termination_cb_cls closure for @e termination_cb
 */
void
MHD_response_option_termination_callback (struct MHD_Response *response,
					  MHD_RequestTerminationCallback termination_cb,
					  void *termination_cb_cls)
{
  /* Q: should we assert termination_cb non-NULL? */
  response->termination_cb = termination_cb;
  response->termination_cb_cls = termination_cb_cls;
}


/* end of response_option.c */
