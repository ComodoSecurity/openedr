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
 * @file lib/connection_add.c
 * @brief functions to add connection to our active set
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_add.h"
#include "connection_call_handlers.h"
#include "connection_close.h"
#include "connection_finish_forward.h"
#include "connection_update_last_activity.h"
#include "daemon_ip_limit.h"
#include "daemon_select.h"
#include "daemon_poll.h"
#include "mhd_sockets.h"


#ifdef UPGRADE_SUPPORT
/**
 * Main function of the thread that handles an individual connection
 * after it was "upgraded" when #MHD_USE_THREAD_PER_CONNECTION is set.
 * @remark To be called only from thread that process
 * connection's recv(), send() and response.
 *
 * @param con the connection this thread will handle
 */
static void
thread_main_connection_upgrade (struct MHD_Connection *con)
{
#ifdef HTTPS_SUPPORT
  struct MHD_Daemon *daemon = con->daemon;

  /* Here, we need to bi-directionally forward
     until the application tells us that it is done
     with the socket; */
  if ( (NULL != daemon->tls_api) &&
       (MHD_ELS_POLL != daemon->event_loop_syscall) )
    {
      MHD_daemon_upgrade_connection_with_select_ (con);
    }
#ifdef HAVE_POLL
  else if (NULL != daemon->tls_api)
    {
      MHD_daemon_upgrade_connection_with_poll_ (con);
    }
#endif
  /* end HTTPS */
#endif /* HTTPS_SUPPORT */
  /* TLS forwarding was finished. Cleanup socketpair. */
  MHD_connection_finish_forward_ (con);
  /* Do not set 'urh->clean_ready' yet as 'urh' will be used
   * in connection thread for a little while. */
}
#endif /* UPGRADE_SUPPORT */


/**
 * Main function of the thread that handles an individual
 * connection when #MHD_USE_THREAD_PER_CONNECTION is set.
 *
 * @param data the `struct MHD_Connection` this thread will handle
 * @return always 0
 */
static MHD_THRD_RTRN_TYPE_ MHD_THRD_CALL_SPEC_
thread_main_handle_connection (void *data)
{
  struct MHD_Connection *con = data;
  struct MHD_Daemon *daemon = con->daemon;
  int num_ready;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket maxsock;
  struct timeval tv;
  struct timeval *tvp;
  time_t now;
#if WINDOWS
#ifdef HAVE_POLL
  int extra_slot;
#endif /* HAVE_POLL */
#define EXTRA_SLOTS 1
#else  /* !WINDOWS */
#define EXTRA_SLOTS 0
#endif /* !WINDOWS */
#ifdef HAVE_POLL
  struct pollfd p[1 + EXTRA_SLOTS];
#endif
#undef EXTRA_SLOTS
#ifdef HAVE_POLL
  const bool use_poll = (MHD_ELS_POLL == daemon->event_loop_syscall);
#else  /* ! HAVE_POLL */
  const bool use_poll = false;
#endif /* ! HAVE_POLL */
  bool was_suspended = false;

  MHD_thread_init_(&con->pid);

  while ( (! daemon->shutdown) &&
	  (MHD_REQUEST_CLOSED != con->request.state) )
    {
      const time_t timeout = daemon->connection_default_timeout;
#ifdef UPGRADE_SUPPORT
      struct MHD_UpgradeResponseHandle * const urh = con->request.urh;
#else  /* ! UPGRADE_SUPPORT */
      static const void * const urh = NULL;
#endif /* ! UPGRADE_SUPPORT */

      if ( (con->suspended) &&
           (NULL == urh) )
        {
          /* Connection was suspended, wait for resume. */
          was_suspended = true;
          if (! use_poll)
            {
              FD_ZERO (&rs);
              if (! MHD_add_to_fd_set_ (MHD_itc_r_fd_ (daemon->itc),
                                        &rs,
                                        NULL,
                                        FD_SETSIZE))
                {
  #ifdef HAVE_MESSAGES
                  MHD_DLOG (con->daemon,
			    MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE,
                            _("Failed to add FD to fd_set\n"));
  #endif
                  goto exit;
                }
              if (0 > MHD_SYS_select_ (MHD_itc_r_fd_ (daemon->itc) + 1,
                                       &rs,
                                       NULL,
                                       NULL,
                                       NULL))
                {
                  const int err = MHD_socket_get_error_();

                  if (MHD_SCKT_ERR_IS_EINTR_(err))
                    continue;
#ifdef HAVE_MESSAGES
                  MHD_DLOG (con->daemon,
			    MHD_SC_UNEXPECTED_SELECT_ERROR,
                            _("Error during select (%d): `%s'\n"),
                            err,
                            MHD_socket_strerr_ (err));
#endif
                  break;
                }
            }
#ifdef HAVE_POLL
          else /* use_poll */
            {
              p[0].events = POLLIN;
              p[0].fd = MHD_itc_r_fd_ (daemon->itc);
              p[0].revents = 0;
              if (0 > MHD_sys_poll_ (p,
                                     1,
                                     -1))
                {
                  if (MHD_SCKT_LAST_ERR_IS_(MHD_SCKT_EINTR_))
                    continue;
#ifdef HAVE_MESSAGES
                  MHD_DLOG (con->daemon,
			    MHD_SC_UNEXPECTED_POLL_ERROR,
                            _("Error during poll: `%s'\n"),
                            MHD_socket_last_strerr_ ());
#endif
                  break;
                }
            }
#endif /* HAVE_POLL */
          MHD_itc_clear_ (daemon->itc);
          continue; /* Check again for resume. */
        } /* End of "suspended" branch. */

      if (was_suspended)
        {
          MHD_connection_update_last_activity_ (con); /* Reset timeout timer. */
          /* Process response queued during suspend and update states. */
          MHD_request_handle_idle_ (&con->request);
          was_suspended = false;
        }

      tvp = NULL;

      if ( (MHD_EVENT_LOOP_INFO_BLOCK == con->request.event_loop_info)
#ifdef HTTPS_SUPPORT
           || ( (con->tls_read_ready) &&
                (MHD_EVENT_LOOP_INFO_READ == con->request.event_loop_info) )
#endif /* HTTPS_SUPPORT */
         )
	{
	  /* do not block: more data may be inside of TLS buffers waiting or
	   * application must provide response data */
	  tv.tv_sec = 0;
	  tv.tv_usec = 0;
	  tvp = &tv;
	}
      if ( (NULL == tvp) &&
           (timeout > 0) )
	{
	  now = MHD_monotonic_sec_counter();
	  if (now - con->last_activity > timeout)
	    tv.tv_sec = 0;
          else
            {
              const time_t seconds_left = timeout - (now - con->last_activity);
#if !defined(_WIN32) || defined(__CYGWIN__)
              tv.tv_sec = seconds_left;
#else  /* _WIN32 && !__CYGWIN__ */
              if (seconds_left > TIMEVAL_TV_SEC_MAX)
                tv.tv_sec = TIMEVAL_TV_SEC_MAX;
              else
                tv.tv_sec = (_MHD_TIMEVAL_TV_SEC_TYPE) seconds_left;
#endif /* _WIN32 && ! __CYGWIN__  */
            }
	  tv.tv_usec = 0;
	  tvp = &tv;
	}
      if (! use_poll)
	{
	  /* use select */
	  bool err_state = false;

	  FD_ZERO (&rs);
	  FD_ZERO (&ws);
          FD_ZERO (&es);
	  maxsock = MHD_INVALID_SOCKET;
	  switch (con->request.event_loop_info)
	    {
	    case MHD_EVENT_LOOP_INFO_READ:
	      if (! MHD_add_to_fd_set_ (con->socket_fd,
                                        &rs,
                                        &maxsock,
                                        FD_SETSIZE))
	        err_state = true;
	      break;
	    case MHD_EVENT_LOOP_INFO_WRITE:
	      if (! MHD_add_to_fd_set_ (con->socket_fd,
                                        &ws,
                                        &maxsock,
                                        FD_SETSIZE))
                err_state = true;
	      break;
	    case MHD_EVENT_LOOP_INFO_BLOCK:
	      if (! MHD_add_to_fd_set_ (con->socket_fd,
                                        &es,
                                        &maxsock,
                                        FD_SETSIZE))
	        err_state = true;
	      break;
	    case MHD_EVENT_LOOP_INFO_CLEANUP:
	      /* how did we get here!? */
	      goto exit;
	    }
#if WINDOWS
          if (MHD_ITC_IS_VALID_(daemon->itc) )
            {
              if (! MHD_add_to_fd_set_ (MHD_itc_r_fd_ (daemon->itc),
                                        &rs,
                                        &maxsock,
                                        FD_SETSIZE))
                err_state = 1;
            }
#endif
            if (err_state)
              {
#ifdef HAVE_MESSAGES
                MHD_DLOG (con->daemon,
			  MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE,
                          _("Failed to add FD to fd_set\n"));
#endif
                goto exit;
              }

	  num_ready = MHD_SYS_select_ (maxsock + 1,
                                       &rs,
                                       &ws,
                                       &es,
                                       tvp);
	  if (num_ready < 0)
	    {
	      const int err = MHD_socket_get_error_();

	      if (MHD_SCKT_ERR_IS_EINTR_(err))
		continue;
#ifdef HAVE_MESSAGES
	      MHD_DLOG (con->daemon,
			MHD_SC_UNEXPECTED_SELECT_ERROR,
			_("Error during select (%d): `%s'\n"),
			err,
			MHD_socket_strerr_ (err));
#endif
	      break;
	    }
#if WINDOWS
          /* Clear ITC before other processing so additional
           * signals will trigger select() again */
          if ( (MHD_ITC_IS_VALID_(daemon->itc)) &&
               (FD_ISSET (MHD_itc_r_fd_ (daemon->itc),
                          &rs)) )
            MHD_itc_clear_ (daemon->itc);
#endif
          if (MHD_NO ==
              MHD_connection_call_handlers_ (con,
					     FD_ISSET (con->socket_fd,
						       &rs),
					     FD_ISSET (con->socket_fd,
						       &ws),
					     FD_ISSET (con->socket_fd,
						       &es)) )
            goto exit;
	}
#ifdef HAVE_POLL
      else
	{
	  /* use poll */
	  memset (&p,
                  0,
                  sizeof (p));
	  p[0].fd = con->socket_fd;
	  switch (con->request.event_loop_info)
	    {
	    case MHD_EVENT_LOOP_INFO_READ:
	      p[0].events |= POLLIN | MHD_POLL_EVENTS_ERR_DISC;
	      break;
	    case MHD_EVENT_LOOP_INFO_WRITE:
	      p[0].events |= POLLOUT | MHD_POLL_EVENTS_ERR_DISC;
	      break;
	    case MHD_EVENT_LOOP_INFO_BLOCK:
	      p[0].events |= MHD_POLL_EVENTS_ERR_DISC;
	      break;
	    case MHD_EVENT_LOOP_INFO_CLEANUP:
	      /* how did we get here!? */
	      goto exit;
	    }
#if WINDOWS
          extra_slot = 0;
          if (MHD_ITC_IS_VALID_(daemon->itc))
            {
              p[1].events |= POLLIN;
              p[1].fd = MHD_itc_r_fd_ (daemon->itc);
              p[1].revents = 0;
              extra_slot = 1;
            }
#endif
	  if (MHD_sys_poll_ (p,
#if WINDOWS
                             1 + extra_slot,
#else
                             1,
#endif
                             (NULL == tvp) ? -1 : tv.tv_sec * 1000) < 0)
	    {
	      if (MHD_SCKT_LAST_ERR_IS_(MHD_SCKT_EINTR_))
		continue;
#ifdef HAVE_MESSAGES
	      MHD_DLOG (con->daemon,
			MHD_SC_UNEXPECTED_POLL_ERROR,
                        _("Error during poll: `%s'\n"),
			MHD_socket_last_strerr_ ());
#endif
	      break;
	    }
#if WINDOWS
          /* Clear ITC before other processing so additional
           * signals will trigger poll() again */
          if ( (MHD_ITC_IS_VALID_(daemon->itc)) &&
               (0 != (p[1].revents & (POLLERR | POLLHUP | POLLIN))) )
            MHD_itc_clear_ (daemon->itc);
#endif
          if (MHD_NO ==
              MHD_connection_call_handlers_ (con,
					     0 != (p[0].revents & POLLIN),
					     0 != (p[0].revents & POLLOUT),
					     0 != (p[0].revents & (POLLERR | MHD_POLL_REVENTS_ERR_DISC))))
            goto exit;
	}
#endif
#ifdef UPGRADE_SUPPORT
      if (MHD_REQUEST_UPGRADE == con->request.state)
        {
          /* Normal HTTP processing is finished,
           * notify application. */
          if (NULL != con->request.response->termination_cb)
            con->request.response->termination_cb
	      (con->request.response->termination_cb_cls,
	       MHD_REQUEST_TERMINATED_COMPLETED_OK,
	       con->request.client_context);
          thread_main_connection_upgrade (con);
          /* MHD_connection_finish_forward_() was called by thread_main_connection_upgrade(). */

          /* "Upgraded" data will not be used in this thread from this point. */
          con->request.urh->clean_ready = true;
          /* If 'urh->was_closed' set to true, connection will be
           * moved immediately to cleanup list. Otherwise connection
           * will stay in suspended list until 'urh' will be marked
           * with 'was_closed' by application. */
          MHD_request_resume (&con->request);

          /* skip usual clean up  */
          return (MHD_THRD_RTRN_TYPE_) 0;
        }
#endif /* UPGRADE_SUPPORT */
    }
#if DEBUG_CLOSE
#ifdef HAVE_MESSAGES
  MHD_DLOG (con->daemon,
            MHD_SC_THREAD_TERMINATING,
            _("Processing thread terminating. Closing connection\n"));
#endif
#endif
  if (MHD_REQUEST_CLOSED != con->request.state)
    MHD_connection_close_ (con,
                           (daemon->shutdown) ?
                           MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN:
                           MHD_REQUEST_TERMINATED_WITH_ERROR);
  MHD_request_handle_idle_ (&con->request);
exit:
  if (NULL != con->request.response)
    {
      MHD_response_queue_for_destroy (con->request.response);
      con->request.response = NULL;
    }

  if (MHD_INVALID_SOCKET != con->socket_fd)
    {
      shutdown (con->socket_fd,
                SHUT_WR);
      /* 'socket_fd' can be used in other thread to signal shutdown.
       * To avoid data races, do not close socket here. Daemon will
       * use more connections only after cleanup anyway. */
    }
  return (MHD_THRD_RTRN_TYPE_) 0;
}


/**
 * Callback for receiving data from the socket.
 *
 * @param connection the MHD connection structure
 * @param other where to write received data to
 * @param i maximum size of other (in bytes)
 * @return positive value for number of bytes actually received or
 *         negative value for error number MHD_ERR_xxx_
 */
static ssize_t
recv_param_adapter (struct MHD_Connection *connection,
                    void *other,
                    size_t i)
{
  ssize_t ret;

  if ( (MHD_INVALID_SOCKET == connection->socket_fd) ||
       (MHD_REQUEST_CLOSED == connection->request.state) )
    {
      return MHD_ERR_NOTCONN_;
    }
  if (i > MHD_SCKT_SEND_MAX_SIZE_)
    i = MHD_SCKT_SEND_MAX_SIZE_; /* return value limit */

  ret = MHD_recv_ (connection->socket_fd,
                   other,
                   i);
  if (0 > ret)
    {
      const int err = MHD_socket_get_error_ ();
      if (MHD_SCKT_ERR_IS_EAGAIN_ (err))
        {
#ifdef EPOLL_SUPPORT
          /* Got EAGAIN --- no longer read-ready */
          connection->epoll_state &= ~MHD_EPOLL_STATE_READ_READY;
#endif /* EPOLL_SUPPORT */
          return MHD_ERR_AGAIN_;
        }
      if (MHD_SCKT_ERR_IS_EINTR_ (err))
        return MHD_ERR_AGAIN_;
      if (MHD_SCKT_ERR_IS_ (err, MHD_SCKT_ECONNRESET_))
        return MHD_ERR_CONNRESET_;
      /* Treat any other error as hard error. */
      return MHD_ERR_NOTCONN_;
    }
#ifdef EPOLL_SUPPORT
  else if (i > (size_t)ret)
    connection->epoll_state &= ~MHD_EPOLL_STATE_READ_READY;
#endif /* EPOLL_SUPPORT */
  return ret;
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
send_param_adapter (struct MHD_Connection *connection,
                    const void *other,
                    size_t i)
{
  ssize_t ret;

  if ( (MHD_INVALID_SOCKET == connection->socket_fd) ||
       (MHD_REQUEST_CLOSED == connection->request.state) )
    {
      return MHD_ERR_NOTCONN_;
    }
  if (i > MHD_SCKT_SEND_MAX_SIZE_)
    i = MHD_SCKT_SEND_MAX_SIZE_; /* return value limit */

  ret = MHD_send_ (connection->socket_fd,
                   other,
                   i);
  if (0 > ret)
    {
      const int err = MHD_socket_get_error_();

      if (MHD_SCKT_ERR_IS_EAGAIN_(err))
        {
#ifdef EPOLL_SUPPORT
          /* EAGAIN --- no longer write-ready */
          connection->epoll_state &= ~MHD_EPOLL_STATE_WRITE_READY;
#endif /* EPOLL_SUPPORT */
          return MHD_ERR_AGAIN_;
        }
      if (MHD_SCKT_ERR_IS_EINTR_ (err))
        return MHD_ERR_AGAIN_;
      if (MHD_SCKT_ERR_IS_ (err, MHD_SCKT_ECONNRESET_))
        return MHD_ERR_CONNRESET_;
      /* Treat any other error as hard error. */
      return MHD_ERR_NOTCONN_;
    }
#ifdef EPOLL_SUPPORT
  else if (i > (size_t)ret)
    connection->epoll_state &= ~MHD_EPOLL_STATE_WRITE_READY;
#endif /* EPOLL_SUPPORT */
  return ret;
}


/**
 * Add another client connection to the set of connections
 * managed by MHD.  This API is usually not needed (since
 * MHD will accept inbound connections on the server socket).
 * Use this API in special cases, for example if your HTTP
 * server is behind NAT and needs to connect out to the
 * HTTP client.
 *
 * The given client socket will be managed (and closed!) by MHD after
 * this call and must no longer be used directly by the application
 * afterwards.
 *
 * @param daemon daemon that manages the connection
 * @param client_socket socket to manage (MHD will expect
 *        to receive an HTTP request from this socket next).
 * @param addr IP address of the client
 * @param addrlen number of bytes in @a addr
 * @param external_add perform additional operations needed due
 *        to the application calling us directly
 * @param non_blck indicate that socket in non-blocking mode
 * @return #MHD_SC_OK on success
 */
static enum MHD_StatusCode
internal_add_connection (struct MHD_Daemon *daemon,
			 MHD_socket client_socket,
			 const struct sockaddr *addr,
			 socklen_t addrlen,
			 bool external_add,
			 bool non_blck)
{
  enum MHD_StatusCode sc;
  struct MHD_Connection *connection;
  int eno = 0;

  /* Direct add to master daemon could happen only with "external" add mode. */
  mhd_assert ( (NULL == daemon->worker_pool) ||
	       (external_add) );
  if ( (external_add) &&
       (NULL != daemon->worker_pool) )
    {
      unsigned int i;

      /* have a pool, try to find a pool with capacity; we use the
	 socket as the initial offset into the pool for load
	 balancing */
      for (i = 0; i < daemon->worker_pool_size; ++i)
        {
          struct MHD_Daemon * const worker =
                &daemon->worker_pool[(i + client_socket) % daemon->worker_pool_size];
          if (worker->connections < worker->global_connection_limit)
            return internal_add_connection (worker,
                                            client_socket,
                                            addr,
                                            addrlen,
                                            true,
                                            non_blck);
        }
      /* all pools are at their connection limit, must refuse */
      MHD_socket_close_chk_ (client_socket);
#if ENFILE
      errno = ENFILE;
#endif
      return MHD_SC_LIMIT_CONNECTIONS_REACHED;
    }

  if ( (! MHD_SCKT_FD_FITS_FDSET_(client_socket,
                                  NULL)) &&
       (MHD_ELS_SELECT == daemon->event_loop_syscall) )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE,
		_("Socket descriptor larger than FD_SETSIZE: %d > %d\n"),
		(int) client_socket,
		(int) FD_SETSIZE);
#endif
      MHD_socket_close_chk_ (client_socket);
#if EINVAL
      errno = EINVAL;
#endif
      return MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
    }

#ifdef MHD_socket_nosignal_
  if (! MHD_socket_nosignal_ (client_socket))
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_CONFIGURE_NOSIGPIPE_FAILED,
                _("Failed to set SO_NOSIGPIPE on accepted socket: %s\n"),
                MHD_socket_last_strerr_());
#endif
#ifndef MSG_NOSIGNAL
      /* Cannot use socket as it can produce SIGPIPE. */
#ifdef ENOTSOCK
      errno = ENOTSOCK;
#endif /* ENOTSOCK */
      return MHD_SC_ACCEPT_CONFIGURE_NOSIGPIPE_FAILED;
#endif /* ! MSG_NOSIGNAL */
    }
#endif /* MHD_socket_nosignal_ */


#ifdef HAVE_MESSAGES
#if DEBUG_CONNECT
  MHD_DLOG (daemon,
	    MHD_SC_CONNECTION_ACCEPTED,
            _("Accepted connection on socket %d\n"),
            client_socket);
#endif
#endif
  if ( (daemon->connections == daemon->global_connection_limit) ||
       (MHD_NO == MHD_ip_limit_add (daemon,
                                    addr,
                                    addrlen)) )
    {
      /* above connection limit - reject */
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_LIMIT_CONNECTIONS_REACHED,
                _("Server reached connection limit. Closing inbound connection.\n"));
#endif
      MHD_socket_close_chk_ (client_socket);
#if ENFILE
      errno = ENFILE;
#endif
      return MHD_SC_LIMIT_CONNECTIONS_REACHED;
    }

  /* apply connection acceptance policy if present */
  if ( (NULL != daemon->accept_policy_cb) &&
       (MHD_NO ==
	daemon->accept_policy_cb (daemon->accept_policy_cb_cls,
				  addr,
				  addrlen)) )
    {
#if DEBUG_CLOSE
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_POLICY_REJECTED,
                _("Connection rejected by application. Closing connection.\n"));
#endif
#endif
      MHD_socket_close_chk_ (client_socket);
      MHD_ip_limit_del (daemon,
                        addr,
                        addrlen);
#if EACCESS
      errno = EACCESS;
#endif
      return MHD_SC_ACCEPT_POLICY_REJECTED;
    }

  if (NULL ==
      (connection = MHD_calloc_ (1,
				 sizeof (struct MHD_Connection))))
    {
      eno = errno;
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_CONNECTION_MALLOC_FAILURE,
		"Error allocating memory: %s\n",
		MHD_strerror_ (errno));
#endif
      MHD_socket_close_chk_ (client_socket);
      MHD_ip_limit_del (daemon,
                        addr,
                        addrlen);
      errno = eno;
      return MHD_SC_CONNECTION_MALLOC_FAILURE;
    }
  connection->pool
    = MHD_pool_create (daemon->connection_memory_limit_b);
  if (NULL == connection->pool)
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_POOL_MALLOC_FAILURE,
		_("Error allocating memory: %s\n"),
		MHD_strerror_ (errno));
#endif
      MHD_socket_close_chk_ (client_socket);
      MHD_ip_limit_del (daemon,
                        addr,
                        addrlen);
      free (connection);
#if ENOMEM
      errno = ENOMEM;
#endif
      return MHD_SC_POOL_MALLOC_FAILURE;
    }

  connection->connection_timeout = daemon->connection_default_timeout;
  memcpy (&connection->addr,
          addr,
          addrlen);
  connection->addr_len = addrlen;
  connection->socket_fd = client_socket;
  connection->sk_nonblck = non_blck;
  connection->daemon = daemon;
  connection->last_activity = MHD_monotonic_sec_counter();

#ifdef HTTPS_SUPPORT
  if (NULL != daemon->tls_api)
    {
      connection->tls_cs
	= daemon->tls_api->setup_connection (daemon->tls_api->cls,
					     NULL /* FIXME */);
      if (NULL == connection->tls_cs)
	{
	  eno = EINVAL;
	  sc = -1; // FIXME!
	  goto cleanup;
	}
    }
  else
#endif /* ! HTTPS_SUPPORT */
    {
      /* set default connection handlers  */
      connection->recv_cls = &recv_param_adapter;
      connection->send_cls = &send_param_adapter;
    }
  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  /* Firm check under lock. */
  if (daemon->connections >= daemon->global_connection_limit)
    {
      MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
      /* above connection limit - reject */
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_LIMIT_CONNECTIONS_REACHED,
                _("Server reached connection limit. Closing inbound connection.\n"));
#endif
#if ENFILE
      eno = ENFILE;
#endif
      sc = MHD_SC_LIMIT_CONNECTIONS_REACHED;
      goto cleanup;
    }
  daemon->connections++;
  if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
    {
      XDLL_insert (daemon->normal_timeout_head,
                   daemon->normal_timeout_tail,
                   connection);
    }
  DLL_insert (daemon->connections_head,
	      daemon->connections_tail,
	      connection);
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);

  if (NULL != daemon->notify_connection_cb)
    daemon->notify_connection_cb (daemon->notify_connection_cb_cls,
				  connection,
				  MHD_CONNECTION_NOTIFY_STARTED);

  /* attempt to create handler thread */
  if (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode)
    {
      if (! MHD_create_named_thread_ (&connection->pid,
                                      "MHD-connection",
                                      daemon->thread_stack_limit_b,
                                      &thread_main_handle_connection,
                                      connection))
        {
	  eno = errno;
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_THREAD_LAUNCH_FAILURE,
                    "Failed to create a thread: %s\n",
                    MHD_strerror_ (eno));
#endif
	  sc = MHD_SC_THREAD_LAUNCH_FAILURE;
	  goto cleanup;
        }
    }
  else
    {
      connection->pid = daemon->pid;
    }
#ifdef EPOLL_SUPPORT
  if (MHD_ELS_EPOLL == daemon->event_loop_syscall)
    {
      if ( (! daemon->enable_turbo) ||
	   (external_add))
	{ /* Do not manipulate EReady DL-list in 'external_add' mode. */
	  struct epoll_event event;

	  event.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLET;
	  event.data.ptr = connection;
	  if (0 != epoll_ctl (daemon->epoll_fd,
			      EPOLL_CTL_ADD,
			      client_socket,
			      &event))
	    {
	      eno = errno;
#ifdef HAVE_MESSAGES
              MHD_DLOG (daemon,
			MHD_SC_EPOLL_CTL_ADD_FAILED,
                        _("Call to epoll_ctl failed: %s\n"),
                        MHD_socket_last_strerr_ ());
#endif
	      sc = MHD_SC_EPOLL_CTL_ADD_FAILED;
	      goto cleanup;
	    }
	  connection->epoll_state |= MHD_EPOLL_STATE_IN_EPOLL_SET;
	}
      else
	{
	  connection->epoll_state |= MHD_EPOLL_STATE_READ_READY | MHD_EPOLL_STATE_WRITE_READY
	    | MHD_EPOLL_STATE_IN_EREADY_EDLL;
	  EDLL_insert (daemon->eready_head,
		       daemon->eready_tail,
		       connection);
	}
    }
  else /* This 'else' is combined with next 'if'. */
#endif
  if ( (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode) &&
       (external_add) &&
       (MHD_ITC_IS_VALID_(daemon->itc)) &&
       (! MHD_itc_activate_ (daemon->itc,
			     "n")) )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ITC_USE_FAILED,
                _("Failed to signal new connection via inter-thread communication channel (not necessarily fatal, continuing anyway)."));
#endif
    }
  return MHD_SC_OK;

 cleanup:
  if (NULL != daemon->notify_connection_cb)
    daemon->notify_connection_cb (daemon->notify_connection_cb_cls,
				  connection,
				  MHD_CONNECTION_NOTIFY_CLOSED);
#ifdef HTTPS_SUPPORT
  if ( (NULL != daemon->tls_api) &&
       (NULL != connection->tls_cs) )
   daemon->tls_api->teardown_connection (daemon->tls_api->cls,
					 connection->tls_cs);
#endif /* HTTPS_SUPPORT */
  MHD_socket_close_chk_ (client_socket);
  MHD_ip_limit_del (daemon,
                    addr,
                    addrlen);
  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
    {
      XDLL_remove (daemon->normal_timeout_head,
                   daemon->normal_timeout_tail,
                   connection);
    }
  DLL_remove (daemon->connections_head,
	      daemon->connections_tail,
	      connection);
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
  MHD_pool_destroy (connection->pool);
  free (connection);
  if (0 != eno)
    errno = eno;
  else
    errno  = EINVAL;
  return sc;
}


/**
 * Add another client connection to the set of connections managed by
 * MHD.  This API is usually not needed (since MHD will accept inbound
 * connections on the server socket).  Use this API in special cases,
 * for example if your HTTP server is behind NAT and needs to connect
 * out to the HTTP client, or if you are building a proxy.
 *
 * If you use this API in conjunction with a internal select or a
 * thread pool, you must set the option #MHD_USE_ITC to ensure that
 * the freshly added connection is immediately processed by MHD.
 *
 * The given client socket will be managed (and closed!) by MHD after
 * this call and must no longer be used directly by the application
 * afterwards.
 *
 * @param daemon daemon that manages the connection
 * @param client_socket socket to manage (MHD will expect
 *        to receive an HTTP request from this socket next).
 * @param addr IP address of the client
 * @param addrlen number of bytes in @a addr
 * @return #MHD_YES on success, #MHD_NO if this daemon could
 *        not handle the connection (i.e. malloc() failed, etc).
 *        The socket will be closed in any case; `errno` is
 *        set to indicate further details about the error.
 * @ingroup specialized
 */
enum MHD_StatusCode
MHD_daemon_add_connection (struct MHD_Daemon *daemon,
			   MHD_socket client_socket,
			   const struct sockaddr *addr,
			   socklen_t addrlen)
{
  bool sk_nonbl;

  if (! MHD_socket_nonblocking_ (client_socket))
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_CONFIGURE_NONBLOCKING_FAILED,
                _("Failed to set nonblocking mode on new client socket: %s\n"),
                MHD_socket_last_strerr_());
#endif
      sk_nonbl = false;
    }
  else
    {
      sk_nonbl = true;
    }

  if ( (daemon->enable_turbo) &&
       (! MHD_socket_noninheritable_ (client_socket)) )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_CONFIGURE_NOINHERIT_FAILED,
                _("Failed to set noninheritable mode on new client socket.\n"));
#endif
    }
  return internal_add_connection (daemon,
				  client_socket,
				  addr,
                                  addrlen,
				  true,
				  sk_nonbl);
}


/**
 * Accept an incoming connection and create the MHD_Connection object
 * for it.  This function also enforces policy by way of checking with
 * the accept policy callback.  @remark To be called only from thread
 * that process daemon's select()/poll()/etc.
 *
 * @param daemon handle with the listen socket
 * @return #MHD_SC_OK on success
 */
enum MHD_StatusCode
MHD_accept_connection_ (struct MHD_Daemon *daemon)
{
  struct sockaddr_storage addrstorage;
  struct sockaddr *addr = (struct sockaddr *) &addrstorage;
  socklen_t addrlen;
  MHD_socket s;
  MHD_socket fd;
  bool sk_nonbl;

  addrlen = sizeof (addrstorage);
  memset (addr,
          0,
          sizeof (addrstorage));
  if ( (MHD_INVALID_SOCKET == (fd = daemon->listen_socket)) ||
       (daemon->was_quiesced) )
    return MHD_SC_DAEMON_ALREADY_QUIESCED;
#ifdef USE_ACCEPT4
  s = accept4 (fd,
               addr,
               &addrlen,
               MAYBE_SOCK_CLOEXEC | MAYBE_SOCK_NONBLOCK);
  sk_nonbl = (0 != MAYBE_SOCK_NONBLOCK);
#else  /* ! USE_ACCEPT4 */
  s = accept (fd,
              addr,
              &addrlen);
  sk_nonbl = false;
#endif /* ! USE_ACCEPT4 */
  if ( (MHD_INVALID_SOCKET == s) ||
       (addrlen <= 0) )
    {
      const int err = MHD_socket_get_error_ ();

      /* This could be a common occurance with multiple worker threads */
      if (MHD_SCKT_ERR_IS_ (err,
                            MHD_SCKT_EINVAL_))
        return MHD_SC_DAEMON_ALREADY_SHUTDOWN; /* can happen during shutdown, let's hope this is the cause... */
      if (MHD_SCKT_ERR_IS_DISCNN_BEFORE_ACCEPT_(err))
        return MHD_SC_ACCEPT_FAST_DISCONNECT; /* do not print error if client just disconnected early */
      if (MHD_SCKT_ERR_IS_EAGAIN_ (err) )
	return MHD_SC_ACCEPT_FAILED_EAGAIN;
      if (MHD_INVALID_SOCKET != s)
        {
          MHD_socket_close_chk_ (s);
        }
      if ( MHD_SCKT_ERR_IS_LOW_RESOURCES_ (err) )
        {
          /* system/process out of resources */
          if (0 == daemon->connections)
            {
#ifdef HAVE_MESSAGES
              /* Not setting 'at_limit' flag, as there is no way it
                 would ever be cleared.  Instead trying to produce
                 bit fat ugly warning. */
              MHD_DLOG (daemon,
			MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED_INSTANTLY,
                        _("Hit process or system resource limit at FIRST connection. This is really bad as there is no sane way to proceed. Will try busy waiting for system resources to become magically available.\n"));
#endif
	      return MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED_INSTANTLY;
            }
          else
            {
              MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
              daemon->at_limit = true;
              MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
#ifdef HAVE_MESSAGES
              MHD_DLOG (daemon,
			MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED,
                        _("Hit process or system resource limit at %u connections, temporarily suspending accept(). Consider setting a lower MHD_OPTION_CONNECTION_LIMIT.\n"),
                        (unsigned int) daemon->connections);
#endif
	      return MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED;
            }
        }
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_FAILED_UNEXPECTEDLY,
		_("Error accepting connection: %s\n"),
		MHD_socket_strerr_(err));
#endif
      return MHD_SC_ACCEPT_FAILED_UNEXPECTEDLY;
    }
#if !defined(USE_ACCEPT4) || !defined(HAVE_SOCK_NONBLOCK)
  if (! MHD_socket_nonblocking_ (s))
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_CONFIGURE_NONBLOCKING_FAILED,
                _("Failed to set nonblocking mode on incoming connection socket: %s\n"),
                MHD_socket_last_strerr_());
#endif
    }
  else
    sk_nonbl = true;
#endif /* !USE_ACCEPT4 || !HAVE_SOCK_NONBLOCK */
#if !defined(USE_ACCEPT4) || !defined(SOCK_CLOEXEC)
  if (! MHD_socket_noninheritable_ (s))
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ACCEPT_CONFIGURE_NOINHERIT_FAILED,
                _("Failed to set noninheritable mode on incoming connection socket.\n"));
#endif
    }
#endif /* !USE_ACCEPT4 || !SOCK_CLOEXEC */
#ifdef HAVE_MESSAGES
#if DEBUG_CONNECT
  MHD_DLOG (daemon,
	    MHD_SC_CONNECTION_ACCEPTED,
            _("Accepted connection on socket %d\n"),
            s);
#endif
#endif
  return internal_add_connection (daemon,
                                  s,
				  addr,
                                  addrlen,
				  false,
                                  sk_nonbl);
}

/* end of connection_add.c */
