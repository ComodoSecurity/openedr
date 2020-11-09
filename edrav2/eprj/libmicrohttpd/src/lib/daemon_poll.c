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
 * @file lib/daemon_poll.c
 * @brief functions to run poll-based event loop
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_add.h"
#include "connection_call_handlers.h"
#include "connection_finish_forward.h"
#include "daemon_poll.h"
#include "upgrade_process.h"
#include "request_resume.h"


#ifdef HAVE_POLL


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)

/**
 * Set required 'event' members in 'pollfd' elements,
 * assuming that @a p[0].fd is MHD side of socketpair
 * and @a p[1].fd is TLS connected socket.
 *
 * @param urh upgrade handle to watch for
 * @param p pollfd array to update
 */
static void
urh_update_pollfd (struct MHD_UpgradeResponseHandle *urh,
		   struct pollfd p[2])
{
  p[0].events = 0;
  p[1].events = 0;

  if (urh->in_buffer_used < urh->in_buffer_size)
    p[0].events |= POLLIN;
  if (0 != urh->out_buffer_used)
    p[0].events |= POLLOUT;

  /* Do not monitor again for errors if error was detected before as
   * error state is remembered. */
  if ((0 == (urh->app.celi & MHD_EPOLL_STATE_ERROR)) &&
      ((0 != urh->in_buffer_size) ||
       (0 != urh->out_buffer_size) ||
       (0 != urh->out_buffer_used)))
    p[0].events |= MHD_POLL_EVENTS_ERR_DISC;

  if (urh->out_buffer_used < urh->out_buffer_size)
    p[1].events |= POLLIN;
  if (0 != urh->in_buffer_used)
    p[1].events |= POLLOUT;

  /* Do not monitor again for errors if error was detected before as
   * error state is remembered. */
  if ((0 == (urh->mhd.celi & MHD_EPOLL_STATE_ERROR)) &&
      ((0 != urh->out_buffer_size) ||
       (0 != urh->in_buffer_size) ||
       (0 != urh->in_buffer_used)))
    p[1].events |= MHD_POLL_EVENTS_ERR_DISC;
}


/**
 * Set @a p to watch for @a urh.
 *
 * @param urh upgrade handle to watch for
 * @param p pollfd array to set
 */
static void
urh_to_pollfd (struct MHD_UpgradeResponseHandle *urh,
	       struct pollfd p[2])
{
  p[0].fd = urh->connection->socket_fd;
  p[1].fd = urh->mhd.socket;
  urh_update_pollfd (urh,
		     p);
}


/**
 * Update ready state in @a urh based on pollfd.
 * @param urh upgrade handle to update
 * @param p 'poll()' processed pollfd.
 */
static void
urh_from_pollfd (struct MHD_UpgradeResponseHandle *urh,
		 struct pollfd p[2])
{
  /* Reset read/write ready, preserve error state. */
  urh->app.celi &= (~MHD_EPOLL_STATE_READ_READY & ~MHD_EPOLL_STATE_WRITE_READY);
  urh->mhd.celi &= (~MHD_EPOLL_STATE_READ_READY & ~MHD_EPOLL_STATE_WRITE_READY);

  if (0 != (p[0].revents & POLLIN))
    urh->app.celi |= MHD_EPOLL_STATE_READ_READY;
  if (0 != (p[0].revents & POLLOUT))
    urh->app.celi |= MHD_EPOLL_STATE_WRITE_READY;
  if (0 != (p[0].revents & POLLHUP))
    urh->app.celi |= MHD_EPOLL_STATE_READ_READY | MHD_EPOLL_STATE_WRITE_READY;
  if (0 != (p[0].revents & MHD_POLL_REVENTS_ERRROR))
    urh->app.celi |= MHD_EPOLL_STATE_ERROR;
  if (0 != (p[1].revents & POLLIN))
    urh->mhd.celi |= MHD_EPOLL_STATE_READ_READY;
  if (0 != (p[1].revents & POLLOUT))
    urh->mhd.celi |= MHD_EPOLL_STATE_WRITE_READY;
  if (0 != (p[1].revents & POLLHUP))
    urh->mhd.celi |= MHD_EPOLL_STATE_ERROR;
  if (0 != (p[1].revents & MHD_POLL_REVENTS_ERRROR))
    urh->mhd.celi |= MHD_EPOLL_STATE_READ_READY | MHD_EPOLL_STATE_WRITE_READY;
}
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */


/**
 * Process all of our connections and possibly the server
 * socket using poll().
 *
 * @param daemon daemon to run poll loop for
 * @param may_block #MHD_YES if blocking, #MHD_NO if non-blocking
 * @return #MHD_SC_OK on success
 */
enum MHD_StatusCode
MHD_daemon_poll_all_ (struct MHD_Daemon *daemon,
		      bool may_block)
{
  unsigned int num_connections;
  struct MHD_Connection *pos;
  struct MHD_Connection *prev;
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  struct MHD_UpgradeResponseHandle *urh;
  struct MHD_UpgradeResponseHandle *urhn;
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */

  if ( (! daemon->disallow_suspend_resume) &&
       (MHD_resume_suspended_connections_ (daemon)) )
    may_block = false;

  /* count number of connections and thus determine poll set size */
  num_connections = 0;
  for (pos = daemon->connections_head; NULL != pos; pos = pos->next)
    num_connections++;
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  for (urh = daemon->urh_head; NULL != urh; urh = urh->next)
    num_connections += 2;
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
  {
    MHD_UNSIGNED_LONG_LONG ltimeout;
    unsigned int i;
    int timeout;
    unsigned int poll_server;
    int poll_listen;
    int poll_itc_idx;
    struct pollfd *p;
    MHD_socket ls;

    p = MHD_calloc_ ((2 + num_connections),
		     sizeof (struct pollfd));
    if (NULL == p)
      {
#ifdef HAVE_MESSAGES
        MHD_DLOG (daemon,
		  MHD_SC_POLL_MALLOC_FAILURE,
                  _("Error allocating memory: %s\n"),
                  MHD_strerror_(errno));
#endif
        return MHD_SC_POLL_MALLOC_FAILURE;
      }
    poll_server = 0;
    poll_listen = -1;
    if ( (MHD_INVALID_SOCKET != (ls = daemon->listen_socket)) &&
         (! daemon->was_quiesced) &&
	 (daemon->connections < daemon->global_connection_limit) &&
         (! daemon->at_limit) )
      {
	/* only listen if we are not at the connection limit */
	p[poll_server].fd = ls;
	p[poll_server].events = POLLIN;
	p[poll_server].revents = 0;
	poll_listen = (int) poll_server;
	poll_server++;
      }
    poll_itc_idx = -1;
    if (MHD_ITC_IS_VALID_(daemon->itc))
      {
	p[poll_server].fd = MHD_itc_r_fd_ (daemon->itc);
	p[poll_server].events = POLLIN;
	p[poll_server].revents = 0;
        poll_itc_idx = (int) poll_server;
	poll_server++;
      }
    if (! may_block)
      timeout = 0;
    else if ( (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode) ||
	      (MHD_SC_OK != /* FIXME: distinguish between NO_TIMEOUT and errors! */
	       MHD_daemon_get_timeout (daemon,
				       &ltimeout)) )
      timeout = -1;
    else
      timeout = (ltimeout > INT_MAX) ? INT_MAX : (int) ltimeout;

    i = 0;
    for (pos = daemon->connections_tail; NULL != pos; pos = pos->prev)
      {
	p[poll_server+i].fd = pos->socket_fd;
	switch (pos->request.event_loop_info)
	  {
	  case MHD_EVENT_LOOP_INFO_READ:
	    p[poll_server+i].events |= POLLIN | MHD_POLL_EVENTS_ERR_DISC;
	    break;
	  case MHD_EVENT_LOOP_INFO_WRITE:
	    p[poll_server+i].events |= POLLOUT | MHD_POLL_EVENTS_ERR_DISC;
	    break;
	  case MHD_EVENT_LOOP_INFO_BLOCK:
	    p[poll_server+i].events |=  MHD_POLL_EVENTS_ERR_DISC;
	    break;
	  case MHD_EVENT_LOOP_INFO_CLEANUP:
	    timeout = 0; /* clean up "pos" immediately */
	    break;
	  }
	i++;
      }
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
    for (urh = daemon->urh_tail; NULL != urh; urh = urh->prev)
      {
        urh_to_pollfd (urh,
		       &(p[poll_server+i]));
        i += 2;
      }
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
    if (0 == poll_server + num_connections)
      {
        free (p);
        return MHD_SC_OK;
      }
    if (MHD_sys_poll_(p,
                      poll_server + num_connections,
                      timeout) < 0)
      {
        const int err = MHD_socket_get_error_ ();
	if (MHD_SCKT_ERR_IS_EINTR_ (err))
      {
        free(p);
        return MHD_SC_OK;
      }
#ifdef HAVE_MESSAGES
	MHD_DLOG (daemon,
		  MHD_SC_UNEXPECTED_POLL_ERROR,
		  _("poll failed: %s\n"),
		  MHD_socket_strerr_ (err));
#endif
        free(p);
	return MHD_SC_UNEXPECTED_POLL_ERROR;
      }

    /* Reset. New value will be set when connections are processed. */
    daemon->data_already_pending = false;

    /* handle ITC FD */
    /* do it before any other processing so
       new signals will be processed in next loop */
    if ( (-1 != poll_itc_idx) &&
         (0 != (p[poll_itc_idx].revents & POLLIN)) )
      MHD_itc_clear_ (daemon->itc);

    /* handle shutdown */
    if (daemon->shutdown)
      {
        free(p);
        return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
      }
    i = 0;
    prev = daemon->connections_tail;
    while (NULL != (pos = prev))
      {
	prev = pos->prev;
        /* first, sanity checks */
        if (i >= num_connections)
          break; /* connection list changed somehow, retry later ... */
        if (p[poll_server+i].fd != pos->socket_fd)
          continue; /* fd mismatch, something else happened, retry later ... */
        MHD_connection_call_handlers_ (pos,
				       0 != (p[poll_server+i].revents & POLLIN),
				       0 != (p[poll_server+i].revents & POLLOUT),
				       0 != (p[poll_server+i].revents & MHD_POLL_REVENTS_ERR_DISC));
        i++;
      }
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
    for (urh = daemon->urh_tail; NULL != urh; urh = urhn)
      {
        if (i >= num_connections)
          break; /* connection list changed somehow, retry later ... */

        /* Get next connection here as connection can be removed
         * from 'daemon->urh_head' list. */
        urhn = urh->prev;
        /* Check for fd mismatch. FIXME: required for safety? */
        if ((p[poll_server+i].fd != urh->connection->socket_fd) ||
            (p[poll_server+i+1].fd != urh->mhd.socket))
          break;
        urh_from_pollfd (urh,
			 &p[poll_server+i]);
        i += 2;
        MHD_upgrade_response_handle_process_ (urh);
        /* Finished forwarding? */
        if ( (0 == urh->in_buffer_size) &&
             (0 == urh->out_buffer_size) &&
             (0 == urh->in_buffer_used) &&
             (0 == urh->out_buffer_used) )
          {
            /* MHD_connection_finish_forward_() will remove connection from
             * 'daemon->urh_head' list. */
            MHD_connection_finish_forward_ (urh->connection);
            urh->clean_ready = true;
            /* If 'urh->was_closed' already was set to true, connection will be
             * moved immediately to cleanup list. Otherwise connection
             * will stay in suspended list until 'urh' will be marked
             * with 'was_closed' by application. */
            MHD_request_resume (&urh->connection->request);
          }
      }
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
    /* handle 'listen' FD */
    if ( (-1 != poll_listen) &&
	 (0 != (p[poll_listen].revents & POLLIN)) )
      (void) MHD_accept_connection_ (daemon);

    free(p);
  }
  return MHD_SC_OK;
}


/**
 * Process only the listen socket using poll().
 *
 * @param daemon daemon to run poll loop for
 * @param may_block true if blocking, false if non-blocking
 * @return #MHD_SC_OK on success
 */
enum MHD_StatusCode
MHD_daemon_poll_listen_socket_ (struct MHD_Daemon *daemon,
				bool may_block)
{
  struct pollfd p[2];
  int timeout;
  unsigned int poll_count;
  int poll_listen;
  int poll_itc_idx;
  MHD_socket ls;

  memset (&p,
          0,
          sizeof (p));
  poll_count = 0;
  poll_listen = -1;
  poll_itc_idx = -1;
  if ( (MHD_INVALID_SOCKET != (ls = daemon->listen_socket)) &&
       (! daemon->was_quiesced) )

    {
      p[poll_count].fd = ls;
      p[poll_count].events = POLLIN;
      p[poll_count].revents = 0;
      poll_listen = poll_count;
      poll_count++;
    }
  if (MHD_ITC_IS_VALID_(daemon->itc))
    {
      p[poll_count].fd = MHD_itc_r_fd_ (daemon->itc);
      p[poll_count].events = POLLIN;
      p[poll_count].revents = 0;
      poll_itc_idx = poll_count;
      poll_count++;
    }

  if (! daemon->disallow_suspend_resume)
    (void) MHD_resume_suspended_connections_ (daemon);

  if (! may_block)
    timeout = 0;
  else
    timeout = -1;
  if (0 == poll_count)
    return MHD_SC_OK;
  if (MHD_sys_poll_(p,
                    poll_count,
                    timeout) < 0)
    {
      const int err = MHD_socket_get_error_ ();

      if (MHD_SCKT_ERR_IS_EINTR_ (err))
	return MHD_SC_OK;
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_UNEXPECTED_POLL_ERROR,
                _("poll failed: %s\n"),
                MHD_socket_strerr_ (err));
#endif
      return MHD_SC_UNEXPECTED_POLL_ERROR;
    }
  if ( (-1 != poll_itc_idx) &&
       (0 != (p[poll_itc_idx].revents & POLLIN)) )
    MHD_itc_clear_ (daemon->itc);

  /* handle shutdown */
  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
  if ( (-1 != poll_listen) &&
       (0 != (p[poll_listen].revents & POLLIN)) )
    (void) MHD_accept_connection_ (daemon);
  return MHD_SC_OK;
}
#endif


/**
 * Do poll()-based processing.
 *
 * @param daemon daemon to run poll()-loop for
 * @param may_block true if blocking, false if non-blocking
 * @return #MHD_SC_OK on success
 */
enum MHD_StatusCode
MHD_daemon_poll_ (struct MHD_Daemon *daemon,
		  bool may_block)
{
#ifdef HAVE_POLL
  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
  if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
    return MHD_daemon_poll_all_ (daemon,
				 may_block);
  return MHD_daemon_poll_listen_socket_ (daemon,
					 may_block);
#else
  /* This code should be dead, as we should have checked
     this earlier... */
  return MHD_SC_POLL_NOT_SUPPORTED;
#endif
}


#ifdef HAVE_POLL
#ifdef HTTPS_SUPPORT
/**
 * Process upgraded connection with a poll() loop.
 * We are in our own thread, only processing @a con
 *
 * @param con connection to process
 */
void
MHD_daemon_upgrade_connection_with_poll_ (struct MHD_Connection *con)
{
  struct MHD_UpgradeResponseHandle *urh = con->request.urh;
  struct pollfd p[2];

  memset (p,
	  0,
	  sizeof (p));
  p[0].fd = urh->connection->socket_fd;
  p[1].fd = urh->mhd.socket;

  while ( (0 != urh->in_buffer_size) ||
	  (0 != urh->out_buffer_size) ||
	  (0 != urh->in_buffer_used) ||
	  (0 != urh->out_buffer_used) )
    {
      int timeout;

      urh_update_pollfd (urh,
			 p);

      if ( (con->tls_read_ready) &&
	   (urh->in_buffer_used < urh->in_buffer_size))
	timeout = 0; /* No need to wait if incoming data is already pending in TLS buffers. */
      else
	timeout = -1;

      if (MHD_sys_poll_ (p,
			 2,
			 timeout) < 0)
	{
	  const int err = MHD_socket_get_error_ ();

	  if (MHD_SCKT_ERR_IS_EINTR_ (err))
	    continue;
#ifdef HAVE_MESSAGES
	  MHD_DLOG (con->daemon,
		    MHD_SC_UNEXPECTED_POLL_ERROR,
		    _("Error during poll: `%s'\n"),
		    MHD_socket_strerr_ (err));
#endif
	  break;
	}
      urh_from_pollfd (urh,
		       p);
      MHD_upgrade_response_handle_process_ (urh);
    }
}
#endif
#endif

/* end of daemon_poll.c */
