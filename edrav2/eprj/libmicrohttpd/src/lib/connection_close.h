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
 * @file lib/connection_close.h
 * @brief functions to close connection
 * @author Christian Grothoff
 */
#ifndef CONNECTION_CLOSE_H
#define CONNECTION_CLOSE_H

#include "microhttpd2.h"

/**
 * Mark connection as "closed".
 *
 * @remark To be called from any thread.
 *
 * @param connection connection to close
 */
void
MHD_connection_mark_closed_ (struct MHD_Connection *connection)
  MHD_NONNULL (1);


/**
 * Close the given connection and give the specified termination code
 * to the user.
 *
 * @remark To be called only from thread that process
 * connection's recv(), send() and response.
 *
 * @param connection connection to close
 * @param rtc termination reason to give
 */
void
MHD_connection_close_ (struct MHD_Connection *connection,
                       enum MHD_RequestTerminationCode rtc)
  MHD_NONNULL (1);

#endif
