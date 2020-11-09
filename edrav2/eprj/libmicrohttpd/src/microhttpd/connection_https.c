/*
     This file is part of libmicrohttpd
     Copyright (C) 2007, 2008, 2010 Daniel Pittman and Christian Grothoff

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
 * @file connection_https.c
 * @brief  Methods for managing SSL/TLS connections. This file is only
 *         compiled if ENABLE_HTTPS is set.
 * @author Sagie Amir
 * @author Christian Grothoff
 */

#include "internal.h"
#include "connection.h"
#include "connection_https.h"
#include "memorypool.h"
#include "response.h"
#include "mhd_mono_clock.h"
#include <gnutls/gnutls.h>


/**
 * Callback for receiving data from the socket.
 *
 * @param connection the MHD_Connection structure
 * @param other where to write received data to
 * @param i maximum size of other (in bytes)
 * @return positive value for number of bytes actually received or
 *         negative value for error number MHD_ERR_xxx_
 */
static ssize_t
recv_tls_adapter (struct MHD_Connection *connection,
                  void *other,
                  size_t i)
{
  ssize_t res;

  if (i > SSIZE_MAX)
    i = SSIZE_MAX;

  res = gnutls_record_recv (connection->tls_session,
                            other,
                            i);
  if ( (GNUTLS_E_AGAIN == res) ||
       (GNUTLS_E_INTERRUPTED == res) )
    {
#ifdef EPOLL_SUPPORT
      if (GNUTLS_E_AGAIN == res)
        connection->epoll_state &= ~MHD_EPOLL_STATE_READ_READY;
#endif
      /* Any network errors means that buffer is empty. */
      connection->tls_read_ready = false;
      return MHD_ERR_AGAIN_;
    }
  if (res < 0)
    {
      /* Likely 'GNUTLS_E_INVALID_SESSION' (client communication
         disrupted); interpret as a hard error */
      connection->tls_read_ready = false;
      return MHD_ERR_NOTCONN_;
    }

#ifdef EPOLL_SUPPORT
  /* Unlike non-TLS connections, do not reset "read-ready" if
   * received amount smaller than provided amount, as TLS
   * connections may receive data by fixed-size chunks. */
#endif /* EPOLL_SUPPORT */

  /* Check whether TLS buffers still have some unread data. */
  connection->tls_read_ready = ( ((size_t)res == i) &&
                                 (0 != gnutls_record_check_pending (connection->tls_session)) );
  return res;
}


/**
 * Callback for writing data to the socket.
 *
 * @param connection the MHD connection structure
 * @param other data to write
 * @param i number of bytes to write
 * @return positive value for number of bytes actually sent or
 *         negative value for error number MHD_ERR_xxx_
 */
static ssize_t
send_tls_adapter (struct MHD_Connection *connection,
                  const void *other,
                  size_t i)
{
  ssize_t res;

  if (i > SSIZE_MAX)
    i = SSIZE_MAX;

  res = gnutls_record_send (connection->tls_session,
                            other,
                            i);
  if ( (GNUTLS_E_AGAIN == res) ||
       (GNUTLS_E_INTERRUPTED == res) )
    {
#ifdef EPOLL_SUPPORT
      if (GNUTLS_E_AGAIN == res)
        connection->epoll_state &= ~MHD_EPOLL_STATE_WRITE_READY;
#endif
      return MHD_ERR_AGAIN_;
    }
  if (res < 0)
    {
      /* Likely 'GNUTLS_E_INVALID_SESSION' (client communication
         disrupted); interpret as a hard error */
      return MHD_ERR_NOTCONN_;
    }
#ifdef EPOLL_SUPPORT
  /* Unlike non-TLS connections, do not reset "write-ready" if
   * sent amount smaller than provided amount, as TLS
   * connections may break data into smaller parts for sending. */
#endif /* EPOLL_SUPPORT */
  return res;
}


/**
 * Give gnuTLS chance to work on the TLS handshake.
 *
 * @param connection connection to handshake on
 * @return true if the handshake has completed successfully
 *         and we should start to read/write data,
 *         false is handshake in progress or in case
 *         of error
 */
bool
MHD_run_tls_handshake_ (struct MHD_Connection *connection)
{
  int ret;

  if ((MHD_TLS_CONN_INIT == connection->tls_state) ||
      (MHD_TLS_CONN_HANDSHAKING == connection->tls_state))
    {
      ret = gnutls_handshake (connection->tls_session);
      if (ret == GNUTLS_E_SUCCESS)
	{
	  /* set connection TLS state to enable HTTP processing */
	  connection->tls_state = MHD_TLS_CONN_CONNECTED;
	  MHD_update_last_activity_ (connection);
	  return true;
	}
      if ( (GNUTLS_E_AGAIN == ret) ||
	   (GNUTLS_E_INTERRUPTED == ret) )
	{
          connection->tls_state = MHD_TLS_CONN_HANDSHAKING;
	  /* handshake not done */
	  return false;
	}
      /* handshake failed */
      connection->tls_state = MHD_TLS_CONN_TLS_FAILED;
#ifdef HAVE_MESSAGES
      MHD_DLOG (connection->daemon,
		_("Error: received handshake message out of context\n"));
#endif
      MHD_connection_close_ (connection,
                             MHD_REQUEST_TERMINATED_WITH_ERROR);
      return false;
    }
  return true;
}


/**
 * Set connection callback function to be used through out
 * the processing of this secure connection.
 *
 * @param connection which callbacks should be modified
 */
void
MHD_set_https_callbacks (struct MHD_Connection *connection)
{
  connection->recv_cls = &recv_tls_adapter;
  connection->send_cls = &send_tls_adapter;
}


/**
 * Initiate shutdown of TLS layer of connection.
 *
 * @param connection to use
 * @return true if succeed, false otherwise.
 */
bool
MHD_tls_connection_shutdown (struct MHD_Connection *connection)
{
  if (MHD_TLS_CONN_WR_CLOSED > connection->tls_state)
    {
      const int res =
          gnutls_bye (connection->tls_session, GNUTLS_SHUT_WR);
      if (GNUTLS_E_SUCCESS == res)
        {
          connection->tls_state = MHD_TLS_CONN_WR_CLOSED;
          return true;
        }
      if ((GNUTLS_E_AGAIN == res) ||
          (GNUTLS_E_INTERRUPTED == res))
        {
          connection->tls_state = MHD_TLS_CONN_WR_CLOSING;
          return true;
        }
      else
        connection->tls_state = MHD_TLS_CONN_TLS_FAILED;
    }
  return false;
}

/* end of connection_https.c */
