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
 * @file lib/upgrade_process.c
 * @brief function to process upgrade activity (over TLS)
 * @author Christian Grothoff
 */
#include "internal.h"
#include "upgrade_process.h"


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Performs bi-directional forwarding on upgraded HTTPS connections
 * based on the readyness state stored in the @a urh handle.
 * @remark To be called only from thread that process
 * connection's recv(), send() and response.
 *
 * @param urh handle to process
 */
void
MHD_upgrade_response_handle_process_ (struct MHD_UpgradeResponseHandle *urh)
{
  /* Help compiler to optimize:
   * pointers to 'connection' and 'daemon' are not changed
   * during this processing, so no need to chain dereference
   * each time. */
  struct MHD_Connection * const connection = urh->connection;
  struct MHD_Daemon * const daemon = connection->daemon;
  /* Prevent data races: use same value of 'was_closed' throughout
   * this function. If 'was_closed' changed externally in the middle
   * of processing - it will be processed on next iteration. */
  bool was_closed;
  struct MHD_TLS_Plugin *tls = daemon->tls_api;
  
  if (daemon->shutdown)
    {
      /* Daemon shutting down, application will not receive any more data. */
#ifdef HAVE_MESSAGES
      if (! urh->was_closed)
        {
          MHD_DLOG (daemon,
		    MHD_SC_DAEMON_ALREADY_SHUTDOWN,
                    _("Initiated daemon shutdown while \"upgraded\" connection was not closed.\n"));
        }
#endif
      urh->was_closed = true;
    }
  was_closed = urh->was_closed;
  if (was_closed)
    {
      /* Application was closed connections: no more data
       * can be forwarded to application socket. */
      if (0 < urh->in_buffer_used)
        {
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_UPGRADE_FORWARD_INCOMPLETE,
                    _("Failed to forward to application " MHD_UNSIGNED_LONG_LONG_PRINTF \
                        " bytes of data received from remote side: application shut down socket\n"),
                    (MHD_UNSIGNED_LONG_LONG) urh->in_buffer_used);
#endif

        }
      /* If application signaled MHD about socket closure then
       * check for any pending data even if socket is not marked
       * as 'ready' (signal may arrive after poll()/select()).
       * Socketpair for forwarding is always in non-blocking mode
       * so no risk that recv() will block the thread. */
      if (0 != urh->out_buffer_size)
        urh->mhd.celi |= MHD_EPOLL_STATE_READ_READY;
      /* Discard any data received form remote. */
      urh->in_buffer_used = 0;
      /* Do not try to push data to application. */
      urh->mhd.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
      /* Reading from remote client is not required anymore. */
      urh->in_buffer_size = 0;
      urh->app.celi &= ~MHD_EPOLL_STATE_READ_READY;
      connection->tls_read_ready = false;
    }

  /* On some platforms (W32, possibly Darwin) failed send() (send() will always
   * fail after remote disconnect was detected) may discard data in system
   * buffers received by system but not yet read by recv().
   * So, before trying send() on any socket, recv() must be performed at first
   * otherwise last part of incoming data may be lost. */

  /* If disconnect or error was detected - try to read from socket
   * to dry data possibly pending is system buffers. */
  if (0 != (MHD_EPOLL_STATE_ERROR & urh->app.celi))
    urh->app.celi |= MHD_EPOLL_STATE_READ_READY;
  if (0 != (MHD_EPOLL_STATE_ERROR & urh->mhd.celi))
    urh->mhd.celi |= MHD_EPOLL_STATE_READ_READY;

  /*
   * handle reading from remote TLS client
   */
  if ( ( (0 != (MHD_EPOLL_STATE_READ_READY & urh->app.celi)) ||
         (connection->tls_read_ready) ) &&
       (urh->in_buffer_used < urh->in_buffer_size) )
    {
      ssize_t res;
      size_t buf_size;

      buf_size = urh->in_buffer_size - urh->in_buffer_used;
      if (buf_size > SSIZE_MAX)
        buf_size = SSIZE_MAX;

      connection->tls_read_ready = false;
      res = tls->recv (tls->cls,
		       connection->tls_cs,
		       &urh->in_buffer[urh->in_buffer_used],
		       buf_size);
      if (0 >= res)
        {
	  // FIXME: define GNUTLS-independent error codes!
          if (GNUTLS_E_INTERRUPTED != res)
            {
              urh->app.celi &= ~MHD_EPOLL_STATE_READ_READY;
              if (GNUTLS_E_AGAIN != res)
                {
                  /* Unrecoverable error on socket was detected or
                   * socket was disconnected/shut down. */
                  /* Stop trying to read from this TLS socket. */
                  urh->in_buffer_size = 0;
                }
            }
        }
      else /* 0 < res */
        {
          urh->in_buffer_used += res;
          if (buf_size > (size_t)res)
            urh->app.celi &= ~MHD_EPOLL_STATE_READ_READY;
          else if (0 < tls->check_record_pending (tls->cls,
						  connection->tls_cs))
            connection->tls_read_ready = true;
        }
      if (MHD_EPOLL_STATE_ERROR ==
          ((MHD_EPOLL_STATE_ERROR | MHD_EPOLL_STATE_READ_READY) & urh->app.celi))
        {
          /* Unrecoverable error on socket was detected and all
           * pending data was read from system buffers. */
          /* Stop trying to read from this TLS socket. */
          urh->in_buffer_size = 0;
        }
    }

  /*
   * handle reading from application
   */
  if ( (0 != (MHD_EPOLL_STATE_READ_READY & urh->mhd.celi)) &&
       (urh->out_buffer_used < urh->out_buffer_size) )
    {
      ssize_t res;
      size_t buf_size;

      buf_size = urh->out_buffer_size - urh->out_buffer_used;
      if (buf_size > MHD_SCKT_SEND_MAX_SIZE_)
        buf_size = MHD_SCKT_SEND_MAX_SIZE_;

      res = MHD_recv_ (urh->mhd.socket,
                       &urh->out_buffer[urh->out_buffer_used],
                       buf_size);
      if (0 >= res)
        {
          const int err = MHD_socket_get_error_ ();
          if ((0 == res) ||
              ((! MHD_SCKT_ERR_IS_EINTR_ (err)) &&
               (! MHD_SCKT_ERR_IS_LOW_RESOURCES_(err))))
            {
              urh->mhd.celi &= ~MHD_EPOLL_STATE_READ_READY;
              if ((0 == res) ||
                  (was_closed) ||
                  (0 != (MHD_EPOLL_STATE_ERROR & urh->mhd.celi)) ||
                  (! MHD_SCKT_ERR_IS_EAGAIN_ (err)))
                {
                  /* Socket disconnect/shutdown was detected;
                   * Application signaled about closure of 'upgraded' socket;
                   * or persistent / unrecoverable error. */
                  /* Do not try to pull more data from application. */
                  urh->out_buffer_size = 0;
                }
            }
        }
      else /* 0 < res */
        {
          urh->out_buffer_used += res;
          if (buf_size > (size_t)res)
            urh->mhd.celi &= ~MHD_EPOLL_STATE_READ_READY;
        }
      if ( (0 == (MHD_EPOLL_STATE_READ_READY & urh->mhd.celi)) &&
           ( (0 != (MHD_EPOLL_STATE_ERROR & urh->mhd.celi)) ||
             (was_closed) ) )
        {
          /* Unrecoverable error on socket was detected and all
           * pending data was read from system buffers. */
          /* Do not try to pull more data from application. */
          urh->out_buffer_size = 0;
        }
    }

  /*
   * handle writing to remote HTTPS client
   */
  if ( (0 != (MHD_EPOLL_STATE_WRITE_READY & urh->app.celi)) &&
       (urh->out_buffer_used > 0) )
    {
      ssize_t res;
      size_t data_size;

      data_size = urh->out_buffer_used;
      if (data_size > SSIZE_MAX)
        data_size = SSIZE_MAX;

      res = tls->send (tls->cls,
		       connection->tls_cs,
		       urh->out_buffer,
		       data_size);
      if (0 >= res)
        {
	  // FIXME: define GNUTLS-independent error codes!
          if (GNUTLS_E_INTERRUPTED != res)
            {
              urh->app.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
              if (GNUTLS_E_INTERRUPTED != res)
                {
                  /* TLS connection shut down or
                   * persistent / unrecoverable error. */
#ifdef HAVE_MESSAGES
                  MHD_DLOG (daemon,
			    MHD_SC_UPGRADE_FORWARD_INCOMPLETE,
                            _("Failed to forward to remote client " MHD_UNSIGNED_LONG_LONG_PRINTF \
                                " bytes of data received from application: %s\n"),
                            (MHD_UNSIGNED_LONG_LONG) urh->out_buffer_used,
                            tls->strerror (tls->cls,
					   res));
#endif
                  /* Discard any data unsent to remote. */
                  urh->out_buffer_used = 0;
                  /* Do not try to pull more data from application. */
                  urh->out_buffer_size = 0;
                  urh->mhd.celi &= ~MHD_EPOLL_STATE_READ_READY;
                }
            }
        }
      else /* 0 < res */
        {
          const size_t next_out_buffer_used = urh->out_buffer_used - res;
          if (0 != next_out_buffer_used)
            {
              memmove (urh->out_buffer,
                       &urh->out_buffer[res],
                       next_out_buffer_used);
              if (data_size > (size_t)res)
                urh->app.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
            }
          urh->out_buffer_used = next_out_buffer_used;
        }
      if ( (0 == urh->out_buffer_used) &&
           (0 != (MHD_EPOLL_STATE_ERROR & urh->app.celi)) )
        {
          /* Unrecoverable error on socket was detected and all
           * pending data was sent to remote. */
          /* Do not try to send to remote anymore. */
          urh->app.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
          /* Do not try to pull more data from application. */
          urh->out_buffer_size = 0;
          urh->mhd.celi &= ~MHD_EPOLL_STATE_READ_READY;
        }
    }

  /*
   * handle writing to application
   */
  if ( (0 != (MHD_EPOLL_STATE_WRITE_READY & urh->mhd.celi)) &&
         (urh->in_buffer_used > 0) )
    {
      ssize_t res;
      size_t data_size;

      data_size = urh->in_buffer_used;
      if (data_size > MHD_SCKT_SEND_MAX_SIZE_)
        data_size = MHD_SCKT_SEND_MAX_SIZE_;

      res = MHD_send_ (urh->mhd.socket,
                       urh->in_buffer,
                       data_size);
      if (0 >= res)
        {
          const int err = MHD_socket_get_error_ ();
          if ( (! MHD_SCKT_ERR_IS_EINTR_ (err)) &&
               (! MHD_SCKT_ERR_IS_LOW_RESOURCES_(err)) )
            {
              urh->mhd.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
              if (! MHD_SCKT_ERR_IS_EAGAIN_ (err))
                {
                  /* Socketpair connection shut down or
                   * persistent / unrecoverable error. */
#ifdef HAVE_MESSAGES
                  MHD_DLOG (daemon,
			    MHD_SC_UPGRADE_FORWARD_INCOMPLETE,
                            _("Failed to forward to application " MHD_UNSIGNED_LONG_LONG_PRINTF \
                                " bytes of data received from remote side: %s\n"),
                            (MHD_UNSIGNED_LONG_LONG) urh->in_buffer_used,
                            MHD_socket_strerr_ (err));
#endif
                  /* Discard any data received form remote. */
                  urh->in_buffer_used = 0;
                  /* Reading from remote client is not required anymore. */
                  urh->in_buffer_size = 0;
                  urh->app.celi &= ~MHD_EPOLL_STATE_READ_READY;
                  connection->tls_read_ready = false;
                }
            }
        }
      else /* 0 < res */
        {
          const size_t next_in_buffer_used = urh->in_buffer_used - res;
          if (0 != next_in_buffer_used)
            {
              memmove (urh->in_buffer,
                       &urh->in_buffer[res],
                       next_in_buffer_used);
              if (data_size > (size_t)res)
                urh->mhd.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
            }
          urh->in_buffer_used = next_in_buffer_used;
        }
      if ( (0 == urh->in_buffer_used) &&
           (0 != (MHD_EPOLL_STATE_ERROR & urh->mhd.celi)) )
        {
          /* Do not try to push data to application. */
          urh->mhd.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
          /* Reading from remote client is not required anymore. */
          urh->in_buffer_size = 0;
          urh->app.celi &= ~MHD_EPOLL_STATE_READ_READY;
          connection->tls_read_ready = false;
        }
    }

  /* Check whether data is present in TLS buffers
   * and incoming forward buffer have some space. */
  if ( (connection->tls_read_ready) &&
       (urh->in_buffer_used < urh->in_buffer_size) &&
       (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode) )
    daemon->data_already_pending = true;

  if ( (daemon->shutdown) &&
       ( (0 != urh->out_buffer_size) ||
         (0 != urh->out_buffer_used) ) )
    {
      /* Daemon shutting down, discard any remaining forward data. */
#ifdef HAVE_MESSAGES
      if (0 < urh->out_buffer_used)
        MHD_DLOG (daemon,
		  MHD_SC_UPGRADE_FORWARD_INCOMPLETE,
                  _("Failed to forward to remote client " MHD_UNSIGNED_LONG_LONG_PRINTF \
                      " bytes of data received from application: daemon shut down\n"),
                  (MHD_UNSIGNED_LONG_LONG) urh->out_buffer_used);
#endif
      /* Discard any data unsent to remote. */
      urh->out_buffer_used = 0;
      /* Do not try to sent to remote anymore. */
      urh->app.celi &= ~MHD_EPOLL_STATE_WRITE_READY;
      /* Do not try to pull more data from application. */
      urh->out_buffer_size = 0;
      urh->mhd.celi &= ~MHD_EPOLL_STATE_READ_READY;
    }
}
#endif /* HTTPS_SUPPORT  && UPGRADE_SUPPORT */

/* end of upgrade_process.c */
