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
 * @file lib/connection_call_handlers.h
 * @brief function to call event handlers based on event mask
 * @author Christian Grothoff
 */

#ifndef CONNECTION_CALL_HANDLERS_H
#define CONNECTION_CALL_HANDLERS_H

/**
 * Call the handlers for a connection in the appropriate order based
 * on the readiness as detected by the event loop.
 *
 * @param con connection to handle
 * @param read_ready set if the socket is ready for reading
 * @param write_ready set if the socket is ready for writing
 * @param force_close set if a hard error was detected on the socket;
 *        if this information is not available, simply pass #MHD_NO
 * @return #MHD_YES to continue normally,
 *         #MHD_NO if a serious error was encountered and the
 *         connection is to be closed.
 */
int
MHD_connection_call_handlers_ (struct MHD_Connection *con,
			       bool read_ready,
			       bool write_ready,
			       bool force_close)
  MHD_NONNULL (1);


/**
 * This function was created to handle per-request processing that
 * has to happen even if the socket cannot be read or written to.
 * @remark To be called only from thread that process request's
 * recv(), send() and response.
 *
 * @param request the request to handle
 * @return true if we should continue to process the
 *         request (not dead yet), false if it died
 */
bool
MHD_request_handle_idle_ (struct MHD_Request *request)
  MHD_NONNULL (1);

  
#endif
