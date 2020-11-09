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
 * @file lib/connection_close.c
 * @brief functions to close a connection
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_close.h"


/**
 * Mark connection as "closed".
 *
 * @remark To be called from any thread.
 *
 * @param connection connection to close
 */
void
MHD_connection_mark_closed_ (struct MHD_Connection *connection)
{
  const struct MHD_Daemon *daemon = connection->daemon;

  connection->request.state = MHD_REQUEST_CLOSED;
  connection->request.event_loop_info = MHD_EVENT_LOOP_INFO_CLEANUP;
  if (! daemon->enable_turbo)
    {
#ifdef HTTPS_SUPPORT
      struct MHD_TLS_Plugin *tls;

      /* For TLS connection use shutdown of TLS layer
       * and do not shutdown TCP socket. This give more
       * chances to send TLS closure data to remote side.
       * Closure of TLS layer will be interpreted by
       * remote side as end of transmission. */
      if (NULL != (tls = daemon->tls_api))
        {
          if (MHD_YES !=
	      tls->shutdown_connection (tls->cls,
					connection->tls_cs))
	    {
	      (void) shutdown (connection->socket_fd,
			       SHUT_WR);
	      /* FIXME: log errors */
	    }
        }
      else /* Combined with next 'shutdown()'. */
#endif /* HTTPS_SUPPORT */
	{
	  (void) shutdown (connection->socket_fd,
			   SHUT_WR); /* FIXME: log errors */
	}
    }
}


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
{
  struct MHD_Daemon *daemon = connection->daemon;
  struct MHD_Response *resp = connection->request.response;

  (void) rtc; // FIXME
  MHD_connection_mark_closed_ (connection);
  if (NULL != resp)
    {
      connection->request.response = NULL;
      MHD_response_queue_for_destroy (resp);
    }
  if (NULL != daemon->notify_connection_cb)
    daemon->notify_connection_cb (daemon->notify_connection_cb_cls,
				  connection,
				  MHD_CONNECTION_NOTIFY_CLOSED);
}

/* end of connection_close.c */
