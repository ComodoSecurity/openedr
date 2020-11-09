/*
     This file is part of libmicrohttpd
     Copyright (C) 2007 Daniel Pittman and Christian Grothoff

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
 * @file connection.h
 * @brief  Methods for managing connections
 * @author Daniel Pittman
 * @author Christian Grothoff
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include "internal.h"

/**
 * Error code similar to EGAIN or EINTR
 */
#define MHD_ERR_AGAIN_ (-3073)

/**
 * Connection was hard-closed by remote peer.
 */
#define MHD_ERR_CONNRESET_ (-3074)

/**
 * Connection is not connected anymore due to
 * network error or any other reason.
 */
#define MHD_ERR_NOTCONN_ (-3075)

/**
 * "Not enough memory" error code
 */
#define MHD_ERR_NOMEM_ (-3076)

/**
 * "Bad FD" error code
 */
#define MHD_ERR_BADF_ (-3077)

/**
 * Error code similar to EINVAL
 */
#define MHD_ERR_INVAL_ (-3078)


#ifdef HAVE_FREEBSD_SENDFILE
/**
 * Initialises static variables
 */
void
MHD_conn_init_static_ (void);
#endif /* HAVE_FREEBSD_SENDFILE */


/**
 * Set callbacks for this connection to those for HTTP.
 *
 * @param connection connection to initialize
 */
void
MHD_set_http_callbacks_ (struct MHD_Connection *connection);


/**
 * This function handles a particular connection when it has been
 * determined that there is data to be read off a socket. All
 * implementations (multithreaded, external select, internal select)
 * call this function to handle reads.
 *
 * @param connection connection to handle
 */
void
MHD_connection_handle_read (struct MHD_Connection *connection);


/**
 * This function was created to handle writes to sockets when it has
 * been determined that the socket can be written to. All
 * implementations (multithreaded, external select, internal select)
 * call this function
 *
 * @param connection connection to handle
 */
void
MHD_connection_handle_write (struct MHD_Connection *connection);


/**
 * This function was created to handle per-connection processing that
 * has to happen even if the socket cannot be read or written to.  All
 * implementations (multithreaded, external select, internal select)
 * call this function.
 * @remark To be called only from thread that process connection's
 * recv(), send() and response.
 *
 * @param connection connection to handle
 * @return MHD_YES if we should continue to process the
 *         connection (not dead yet), MHD_NO if it died
 */
int
MHD_connection_handle_idle (struct MHD_Connection *connection);


/**
 * Mark connection as "closed".
 * @remark To be called from any thread.
 *
 * @param connection connection to close
 */
void
MHD_connection_mark_closed_ (struct MHD_Connection *connection);


/**
 * Close the given connection and give the
 * specified termination code to the user.
 * @remark To be called only from thread that
 * process connection's recv(), send() and response.
 *
 * @param connection connection to close
 * @param termination_code termination reason to give
 */
void
MHD_connection_close_ (struct MHD_Connection *connection,
                       enum MHD_RequestTerminationCode termination_code);


#ifdef HTTPS_SUPPORT
/**
 * Stop TLS forwarding on upgraded connection and
 * reflect remote disconnect state to socketpair.
 * @param connection the upgraded connection
 */
void
MHD_connection_finish_forward_ (struct MHD_Connection *connection);
#else  /* ! HTTPS_SUPPORT */
#define MHD_connection_finish_forward_(conn) (void)conn
#endif /* ! HTTPS_SUPPORT */


#ifdef EPOLL_SUPPORT
/**
 * Perform epoll processing, possibly moving the connection back into
 * the epoll set if needed.
 *
 * @param connection connection to process
 * @return MHD_YES if we should continue to process the
 *         connection (not dead yet), MHD_NO if it died
 */
int
MHD_connection_epoll_update_ (struct MHD_Connection *connection);
#endif

/**
 * Update the 'last_activity' field of the connection to the current time
 * and move the connection to the head of the 'normal_timeout' list if
 * the timeout for the connection uses the default value.
 *
 * @param connection the connection that saw some activity
 */
void
MHD_update_last_activity_ (struct MHD_Connection *connection);

#endif
