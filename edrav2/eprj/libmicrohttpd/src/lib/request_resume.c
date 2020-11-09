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
 * @file lib/request_resume.c
 * @brief implementation of MHD_request_resume()
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_close.h"

/**
 * Resume handling of network data for suspended request.  It is
 * safe to resume a suspended request at any time.  Calling this
 * function on a request that was not previously suspended will
 * result in undefined behavior.
 *
 * If you are using this function in ``external'' select mode, you must
 * make sure to run #MHD_run() afterwards (before again calling
 * #MHD_get_fdset(), as otherwise the change may not be reflected in
 * the set returned by #MHD_get_fdset() and you may end up with a
 * request that is stuck until the next network activity.
 *
 * @param request the request to resume
 */
void
MHD_request_resume (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;

  if (daemon->disallow_suspend_resume)
    MHD_PANIC (_("Cannot resume connections without enabling MHD_ALLOW_SUSPEND_RESUME!\n"));
  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  request->connection->resuming = true;
  daemon->resuming = true;
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
  if ( (MHD_ITC_IS_VALID_(daemon->itc)) &&
       (! MHD_itc_activate_ (daemon->itc,
			     "r")) )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_ITC_USE_FAILED,
                _("Failed to signal resume via inter-thread communication channel."));
#endif
    }
}


/**
 * Run through the suspended connections and move any that are no
 * longer suspended back to the active state.
 *
 * @remark To be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param daemon daemon context
 * @return true if a connection was actually resumed
 */
bool
MHD_resume_suspended_connections_ (struct MHD_Daemon *daemon)
/* FIXME: rename connections -> requests? */
{
  struct MHD_Connection *pos;
  struct MHD_Connection *prev = NULL;
  bool ret;
  const bool used_thr_p_c = (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode);

  mhd_assert (NULL == daemon->worker_pool);
  ret = false;
  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);

  if (daemon->resuming)
    {
      prev = daemon->suspended_connections_tail;
      /* During shutdown check for resuming is forced. */
      mhd_assert((NULL != prev) || (daemon->shutdown));
    }

  daemon->resuming = false;

  while (NULL != (pos = prev))
    {
#ifdef UPGRADE_SUPPORT
      struct MHD_UpgradeResponseHandle * const urh = pos->request.urh;
#else  /* ! UPGRADE_SUPPORT */
      static const void * const urh = NULL;
#endif /* ! UPGRADE_SUPPORT */
      prev = pos->prev;
      if ( (! pos->resuming)
#ifdef UPGRADE_SUPPORT
          || ( (NULL != urh) &&
               ( (! urh->was_closed) ||
                 (! urh->clean_ready) ) )
#endif /* UPGRADE_SUPPORT */
         )
        continue;
      ret = true;
      mhd_assert (pos->suspended);
      DLL_remove (daemon->suspended_connections_head,
                  daemon->suspended_connections_tail,
                  pos);
      pos->suspended = false;
      if (NULL == urh)
        {
          DLL_insert (daemon->connections_head,
                      daemon->connections_tail,
                      pos);
          if (! used_thr_p_c)
            {
              /* Reset timeout timer on resume. */
              if (0 != pos->connection_timeout)
                pos->last_activity = MHD_monotonic_sec_counter();

              if (pos->connection_timeout == daemon->connection_default_timeout)
                XDLL_insert (daemon->normal_timeout_head,
                             daemon->normal_timeout_tail,
                             pos);
              else
                XDLL_insert (daemon->manual_timeout_head,
                             daemon->manual_timeout_tail,
                             pos);
            }
#ifdef EPOLL_SUPPORT
          if (MHD_ELS_EPOLL == daemon->event_loop_syscall)
            {
              if (0 != (pos->epoll_state & MHD_EPOLL_STATE_IN_EREADY_EDLL))
                MHD_PANIC ("Resumed connection was already in EREADY set\n");
              /* we always mark resumed connections as ready, as we
                 might have missed the edge poll event during suspension */
              EDLL_insert (daemon->eready_head,
                           daemon->eready_tail,
                           pos);
              pos->epoll_state |= MHD_EPOLL_STATE_IN_EREADY_EDLL | \
                  MHD_EPOLL_STATE_READ_READY | MHD_EPOLL_STATE_WRITE_READY;
              pos->epoll_state &= ~MHD_EPOLL_STATE_SUSPENDED;
            }
#endif
        }
#ifdef UPGRADE_SUPPORT
      else
        {
          struct MHD_Response *response = pos->request.response;

          /* Data forwarding was finished (for TLS connections) AND
           * application was closed upgraded connection.
           * Insert connection into cleanup list. */
          if ( (NULL != response) &&
               (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode) &&
               (NULL != response->termination_cb) )
            response->termination_cb (response->termination_cb_cls,
                                      MHD_REQUEST_TERMINATED_COMPLETED_OK,
                                      &pos->request.client_context);
          DLL_insert (daemon->cleanup_head,
                      daemon->cleanup_tail,
                      pos);

        }
#endif /* UPGRADE_SUPPORT */
      pos->resuming = false;
    }
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
  if ( (used_thr_p_c) &&
       (ret) )
    { /* Wake up suspended connections. */
      if (! MHD_itc_activate_(daemon->itc,
                              "w"))
	{
#ifdef HAVE_MESSAGES
	  MHD_DLOG (daemon,
		    MHD_SC_ITC_USE_FAILED,
		    _("Failed to signal resume of connection via inter-thread communication channel."));
#endif
	}
    }
  return ret;
}


/* end of request_resume.c */
