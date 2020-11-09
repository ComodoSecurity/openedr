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
 * @file lib/daemon_epoll.c
 * @brief functions to run epoll()-based event loop
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_add.h"
#include "connection_call_handlers.h"
#include "connection_finish_forward.h"
#include "daemon_epoll.h"
#include "upgrade_process.h"
#include "request_resume.h"

#ifdef EPOLL_SUPPORT

/**
 * How many events to we process at most per epoll() call?  Trade-off
 * between required stack-size and number of system calls we have to
 * make; 128 should be way enough to avoid more than one system call
 * for most scenarios, and still be moderate in stack size
 * consumption.  Embedded systems might want to choose a smaller value
 * --- but why use epoll() on such a system in the first place?
 */
#define MAX_EVENTS 128


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)

/**
 * Checks whether @a urh has some data to process.
 *
 * @param urh upgrade handler to analyse
 * @return 'true' if @a urh has some data to process,
 *         'false' otherwise
 */
static bool
is_urh_ready (struct MHD_UpgradeResponseHandle * const urh)
{
  const struct MHD_Connection * const connection = urh->connection;

  if ( (0 == urh->in_buffer_size) &&
       (0 == urh->out_buffer_size) &&
       (0 == urh->in_buffer_used) &&
       (0 == urh->out_buffer_used) )
    return false;

  if (connection->daemon->shutdown)
    return true;

  if ( ( (0 != (MHD_EPOLL_STATE_READ_READY & urh->app.celi)) ||
         (connection->tls_read_ready) ) &&
       (urh->in_buffer_used < urh->in_buffer_size) )
    return true;

  if ( (0 != (MHD_EPOLL_STATE_READ_READY & urh->mhd.celi)) &&
       (urh->out_buffer_used < urh->out_buffer_size) )
    return true;

  if ( (0 != (MHD_EPOLL_STATE_WRITE_READY & urh->app.celi)) &&
       (urh->out_buffer_used > 0) )
    return true;

  if ( (0 != (MHD_EPOLL_STATE_WRITE_READY & urh->mhd.celi)) &&
         (urh->in_buffer_used > 0) )
    return true;

  return false;
}


/**
 * Do epoll()-based processing for TLS connections that have been
 * upgraded.  This requires a separate epoll() invocation as we
 * cannot use the `struct MHD_Connection` data structures for
 * the `union epoll_data` in this case.
 * @remark To be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param daemon the daemmon for which we process connections
 * @return #MHD_SC_OK on success
 */
static enum MHD_StatusCode
run_epoll_for_upgrade (struct MHD_Daemon *daemon)
{
  struct epoll_event events[MAX_EVENTS];
  int num_events;
  struct MHD_UpgradeResponseHandle * pos;
  struct MHD_UpgradeResponseHandle * prev;

  num_events = MAX_EVENTS;
  while (MAX_EVENTS == num_events)
    {
      unsigned int i;
      
      /* update event masks */
      num_events = epoll_wait (daemon->epoll_upgrade_fd,
			       events,
                               MAX_EVENTS,
                               0);
      if (-1 == num_events)
	{
          const int err = MHD_socket_get_error_ ();
          if (MHD_SCKT_ERR_IS_EINTR_ (err))
	    return MHD_SC_OK;
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_UNEXPECTED_EPOLL_WAIT_ERROR,
                    _("Call to epoll_wait failed: %s\n"),
                    MHD_socket_strerr_ (err));
#endif
	  return MHD_SC_UNEXPECTED_EPOLL_WAIT_ERROR;
	}
      for (i = 0; i < (unsigned int) num_events; i++)
	{
          struct UpgradeEpollHandle * const ueh = events[i].data.ptr;
          struct MHD_UpgradeResponseHandle * const urh = ueh->urh;
          bool new_err_state = false;

          if (urh->clean_ready)
            continue;

          /* Update ueh state based on what is ready according to epoll() */
          if (0 != (events[i].events & EPOLLIN))
            ueh->celi |= MHD_EPOLL_STATE_READ_READY;
          if (0 != (events[i].events & EPOLLOUT))
            ueh->celi |= MHD_EPOLL_STATE_WRITE_READY;
          if (0 != (events[i].events & EPOLLHUP))
            ueh->celi |= MHD_EPOLL_STATE_READ_READY | MHD_EPOLL_STATE_WRITE_READY;

          if ( (0 == (ueh->celi & MHD_EPOLL_STATE_ERROR)) &&
               (0 != (events[i].events & (EPOLLERR | EPOLLPRI))) )
	    {
              /* Process new error state only one time
               * and avoid continuously marking this connection
               * as 'ready'. */
              ueh->celi |= MHD_EPOLL_STATE_ERROR;
              new_err_state = true;
	    }

          if (! urh->in_eready_list)
            {
              if (new_err_state ||
        	  is_urh_ready(urh))
        	{
        	  EDLL_insert (daemon->eready_urh_head,
			       daemon->eready_urh_tail,
			       urh);
        	  urh->in_eready_list = true;
        	}
            }
        }
    }
  prev = daemon->eready_urh_tail;
  while (NULL != (pos = prev))
    {
      prev = pos->prevE;
      MHD_upgrade_response_handle_process_ (pos);
      if (! is_urh_ready(pos))
      	{
      	  EDLL_remove (daemon->eready_urh_head,
      		       daemon->eready_urh_tail,
      		       pos);
      	  pos->in_eready_list = false;
      	}
      /* Finished forwarding? */
      if ( (0 == pos->in_buffer_size) &&
           (0 == pos->out_buffer_size) &&
           (0 == pos->in_buffer_used) &&
           (0 == pos->out_buffer_used) )
        {
          MHD_connection_finish_forward_ (pos->connection);
          pos->clean_ready = true;
          /* If 'pos->was_closed' already was set to true, connection
           * will be moved immediately to cleanup list. Otherwise
           * connection will stay in suspended list until 'pos' will
           * be marked with 'was_closed' by application. */
          MHD_request_resume (&pos->connection->request);
        }
    }

  return MHD_SC_OK;
}
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */


/**
 * Do epoll()-based processing (this function is allowed to
 * block if @a may_block is set to #MHD_YES).
 *
 * @param daemon daemon to run poll loop for
 * @param may_block true if blocking, false if non-blocking
 * @return #MHD_SC_OK on success
 */
enum MHD_StatusCode
MHD_daemon_epoll_ (struct MHD_Daemon *daemon,
		   bool may_block)
{
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  static const char * const upgrade_marker = "upgrade_ptr";
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
  struct MHD_Connection *pos;
  struct MHD_Connection *prev;
  struct epoll_event events[MAX_EVENTS];
  struct epoll_event event;
  int timeout_ms;
  MHD_UNSIGNED_LONG_LONG timeout_ll;
  int num_events;
  unsigned int i;
  MHD_socket ls;
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  bool run_upgraded = false;
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */

  if (-1 == daemon->epoll_fd)
    return MHD_SC_EPOLL_FD_INVALID; /* we're down! */
  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
  if ( (MHD_INVALID_SOCKET != (ls = daemon->listen_socket)) &&
       (! daemon->was_quiesced) &&
       (daemon->connections < daemon->global_connection_limit) &&
       (! daemon->listen_socket_in_epoll) &&
       (! daemon->at_limit) )
    {
      event.events = EPOLLIN;
      event.data.ptr = daemon;
      if (0 != epoll_ctl (daemon->epoll_fd,
			  EPOLL_CTL_ADD,
			  ls,
			  &event))
	{
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_EPOLL_CTL_ADD_FAILED,
                    _("Call to epoll_ctl failed: %s\n"),
                    MHD_socket_last_strerr_ ());
#endif
	  return MHD_SC_EPOLL_CTL_ADD_FAILED;
	}
      daemon->listen_socket_in_epoll = true;
    }
  if ( (daemon->was_quiesced) &&
       (daemon->listen_socket_in_epoll) )
  {
    if (0 != epoll_ctl (daemon->epoll_fd,
                        EPOLL_CTL_DEL,
                        ls,
                        NULL))
      MHD_PANIC ("Failed to remove listen FD from epoll set\n");
    daemon->listen_socket_in_epoll = false;
  }

#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  if ( (! daemon->upgrade_fd_in_epoll) &&
       (-1 != daemon->epoll_upgrade_fd) )
    {
      event.events = EPOLLIN | EPOLLOUT;
      event.data.ptr = (void *) upgrade_marker;
      if (0 != epoll_ctl (daemon->epoll_fd,
			  EPOLL_CTL_ADD,
			  daemon->epoll_upgrade_fd,
			  &event))
	{
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_EPOLL_CTL_ADD_FAILED,
                    _("Call to epoll_ctl failed: %s\n"),
                    MHD_socket_last_strerr_ ());
#endif
	  return MHD_SC_EPOLL_CTL_ADD_FAILED;
	}
      daemon->upgrade_fd_in_epoll = true;
    }
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
  if ( (daemon->listen_socket_in_epoll) &&
       ( (daemon->connections == daemon->global_connection_limit) ||
         (daemon->at_limit) ||
         (daemon->was_quiesced) ) )
    {
      /* we're at the connection limit, disable listen socket
	 for event loop for now */
      if (0 != epoll_ctl (daemon->epoll_fd,
			  EPOLL_CTL_DEL,
			  ls,
			  NULL))
	MHD_PANIC (_("Failed to remove listen FD from epoll set\n"));
      daemon->listen_socket_in_epoll = false;
    }

  if ( (! daemon->disallow_suspend_resume) &&
       (MHD_resume_suspended_connections_ (daemon)) )
    may_block = false;

  if (may_block)
    {
      if (MHD_SC_OK == /* FIXME: distinguish between NO_TIMEOUT and errors */
	  MHD_daemon_get_timeout (daemon,
				  &timeout_ll))
	{
	  if (timeout_ll >= (MHD_UNSIGNED_LONG_LONG) INT_MAX)
	    timeout_ms = INT_MAX;
	  else
	    timeout_ms = (int) timeout_ll;
	}
      else
	timeout_ms = -1;
    }
  else
    timeout_ms = 0;

  /* Reset. New value will be set when connections are processed. */
  /* Note: Used mostly for uniformity here as same situation is
   * signaled in epoll mode by non-empty eready DLL. */
  daemon->data_already_pending = false;

  /* drain 'epoll' event queue; need to iterate as we get at most
     MAX_EVENTS in one system call here; in practice this should
     pretty much mean only one round, but better an extra loop here
     than unfair behavior... */
  num_events = MAX_EVENTS;
  while (MAX_EVENTS == num_events)
    {
      /* update event masks */
      num_events = epoll_wait (daemon->epoll_fd,
			       events,
                               MAX_EVENTS,
                               timeout_ms);
      if (-1 == num_events)
	{
          const int err = MHD_socket_get_error_ ();
          if (MHD_SCKT_ERR_IS_EINTR_ (err))
	    return MHD_SC_OK;
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_UNEXPECTED_EPOLL_WAIT_ERROR,
                    _("Call to epoll_wait failed: %s\n"),
                    MHD_socket_strerr_ (err));
#endif
	  return MHD_SC_UNEXPECTED_EPOLL_WAIT_ERROR;
	}
      for (i=0;i<(unsigned int) num_events;i++)
	{
          /* First, check for the values of `ptr` that would indicate
             that this event is not about a normal connection. */
	  if (NULL == events[i].data.ptr)
	    continue; /* shutdown signal! */
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
          if (upgrade_marker == events[i].data.ptr)
            {
              /* activity on an upgraded connection, we process
                 those in a separate epoll() */
              run_upgraded = true;
              continue;
            }
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
          if (daemon->epoll_itc_marker == events[i].data.ptr)
            {
              /* It's OK to clear ITC here as all external
                 conditions will be processed later. */
              MHD_itc_clear_ (daemon->itc);
              continue;
            }
	  if (daemon == events[i].data.ptr)
	    {
              /* Check for error conditions on listen socket. */
              /* FIXME: Initiate MHD_quiesce_daemon() to prevent busy waiting? */
              if (0 == (events[i].events & (EPOLLERR | EPOLLHUP)))
                {
                  unsigned int series_length = 0;
                  /* Run 'accept' until it fails or daemon at limit of connections.
                   * Do not accept more then 10 connections at once. The rest will
                   * be accepted on next turn (level trigger is used for listen
                   * socket). */
                  while ( (MHD_SC_OK ==
			   MHD_accept_connection_ (daemon)) &&
                          (series_length < 10) &&
                          (daemon->connections < daemon->global_connection_limit) &&
                          (! daemon->at_limit) )
                    series_length++;
	        }
              continue;
	    }
          /* this is an event relating to a 'normal' connection,
             remember the event and if appropriate mark the
             connection as 'eready'. */
          pos = events[i].data.ptr;
          /* normal processing: update read/write data */
          if (0 != (events[i].events & (EPOLLPRI | EPOLLERR | EPOLLHUP)))
            {
              pos->epoll_state |= MHD_EPOLL_STATE_ERROR;
              if (0 == (pos->epoll_state & MHD_EPOLL_STATE_IN_EREADY_EDLL))
                {
                  EDLL_insert (daemon->eready_head,
                               daemon->eready_tail,
                               pos);
                  pos->epoll_state |= MHD_EPOLL_STATE_IN_EREADY_EDLL;
                }
            }
          else
            {
              if (0 != (events[i].events & EPOLLIN))
                {
                  pos->epoll_state |= MHD_EPOLL_STATE_READ_READY;
                  if ( ( (MHD_EVENT_LOOP_INFO_READ == pos->request.event_loop_info) ||
                         (pos->request.read_buffer_size > pos->request.read_buffer_offset) ) &&
                       (0 == (pos->epoll_state & MHD_EPOLL_STATE_IN_EREADY_EDLL) ) )
                    {
                      EDLL_insert (daemon->eready_head,
                                   daemon->eready_tail,
                                   pos);
                      pos->epoll_state |= MHD_EPOLL_STATE_IN_EREADY_EDLL;
                    }
                }
              if (0 != (events[i].events & EPOLLOUT))
                {
                  pos->epoll_state |= MHD_EPOLL_STATE_WRITE_READY;
                  if ( (MHD_EVENT_LOOP_INFO_WRITE == pos->request.event_loop_info) &&
                       (0 == (pos->epoll_state & MHD_EPOLL_STATE_IN_EREADY_EDLL) ) )
                    {
                      EDLL_insert (daemon->eready_head,
                                   daemon->eready_tail,
                                   pos);
                      pos->epoll_state |= MHD_EPOLL_STATE_IN_EREADY_EDLL;
                    }
                }
            }
        }
    }

#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  if (run_upgraded)
    run_epoll_for_upgrade (daemon); /* FIXME: return value? */
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */

  /* process events for connections */
  prev = daemon->eready_tail;
  while (NULL != (pos = prev))
    {
      prev = pos->prevE;
      MHD_connection_call_handlers_ (pos,
				     0 != (pos->epoll_state & MHD_EPOLL_STATE_READ_READY),
				     0 != (pos->epoll_state & MHD_EPOLL_STATE_WRITE_READY),
				     0 != (pos->epoll_state & MHD_EPOLL_STATE_ERROR));
      if (MHD_EPOLL_STATE_IN_EREADY_EDLL ==
            (pos->epoll_state & (MHD_EPOLL_STATE_SUSPENDED | MHD_EPOLL_STATE_IN_EREADY_EDLL)))
        {
          if ( (MHD_EVENT_LOOP_INFO_READ == pos->request.event_loop_info &&
		0 == (pos->epoll_state & MHD_EPOLL_STATE_READ_READY) ) ||
               (MHD_EVENT_LOOP_INFO_WRITE == pos->request.event_loop_info &&
		0 == (pos->epoll_state & MHD_EPOLL_STATE_WRITE_READY) ) ||
               MHD_EVENT_LOOP_INFO_CLEANUP == pos->request.event_loop_info)
            {
              EDLL_remove (daemon->eready_head,
                           daemon->eready_tail,
                           pos);
              pos->epoll_state &= ~MHD_EPOLL_STATE_IN_EREADY_EDLL;
            }
        }
    }

  /* Finally, handle timed-out connections; we need to do this here
     as the epoll mechanism won't call the 'MHD_request_handle_idle_()' on everything,
     as the other event loops do.  As timeouts do not get an explicit
     event, we need to find those connections that might have timed out
     here.

     Connections with custom timeouts must all be looked at, as we
     do not bother to sort that (presumably very short) list. */
  prev = daemon->manual_timeout_tail;
  while (NULL != (pos = prev))
    {
      prev = pos->prevX;
      MHD_request_handle_idle_ (&pos->request);
    }
  /* Connections with the default timeout are sorted by prepending
     them to the head of the list whenever we touch the connection;
     thus it suffices to iterate from the tail until the first
     connection is NOT timed out */
  prev = daemon->normal_timeout_tail;
  while (NULL != (pos = prev))
    {
      prev = pos->prevX;
      MHD_request_handle_idle_ (&pos->request);
      if (MHD_REQUEST_CLOSED != pos->request.state)
	break; /* sorted by timeout, no need to visit the rest! */
    }
  return MHD_SC_OK;
}
#endif

/* end of daemon_epoll.c */
