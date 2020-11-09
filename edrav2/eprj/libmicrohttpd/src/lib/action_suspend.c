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
 * @file lib/action_suspend.c
 * @brief implementation of MHD_action_suspend()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * The suspend action is being run.  Suspend handling
 * of the given request.
 *
 * @param cls NULL
 * @param request the request to apply the action to
 * @return #MHD_SC_OK on success
 */
static enum MHD_StatusCode
suspend_action (void *cls,
		struct MHD_Request *request)
{
  (void) cls;
  struct MHD_Connection *connection = request->connection;
  struct MHD_Daemon *daemon = connection->daemon;

  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  if (connection->resuming)
    {
      /* suspending again while we didn't even complete resuming yet */
      connection->resuming = false;
      MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
      return MHD_SC_OK;
    }
  if (daemon->threading_mode != MHD_TM_THREAD_PER_CONNECTION)
    {
      if (connection->connection_timeout ==
	  daemon->connection_default_timeout)
        XDLL_remove (daemon->normal_timeout_head,
                     daemon->normal_timeout_tail,
                     connection);
      else
        XDLL_remove (daemon->manual_timeout_head,
                     daemon->manual_timeout_tail,
                     connection);
    }
  DLL_remove (daemon->connections_head,
              daemon->connections_tail,
              connection);
  mhd_assert (! connection->suspended);
  DLL_insert (daemon->suspended_connections_head,
              daemon->suspended_connections_tail,
              connection);
  connection->suspended = true;
#ifdef EPOLL_SUPPORT
  if (MHD_ELS_EPOLL == daemon->event_loop_syscall)
    {
      if (0 != (connection->epoll_state & MHD_EPOLL_STATE_IN_EREADY_EDLL))
        {
          EDLL_remove (daemon->eready_head,
                       daemon->eready_tail,
                       connection);
          connection->epoll_state &= ~MHD_EPOLL_STATE_IN_EREADY_EDLL;
        }
      if (0 != (connection->epoll_state & MHD_EPOLL_STATE_IN_EPOLL_SET))
        {
          if (0 != epoll_ctl (daemon->epoll_fd,
                              EPOLL_CTL_DEL,
                              connection->socket_fd,
                              NULL))
            MHD_PANIC (_("Failed to remove FD from epoll set\n"));
          connection->epoll_state &= ~MHD_EPOLL_STATE_IN_EPOLL_SET;
        }
      connection->epoll_state |= MHD_EPOLL_STATE_SUSPENDED;
    }
#endif
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
  return MHD_SC_OK;
}


/**
 * Suspend handling of network data for a given request.  This can
 * be used to dequeue a request from MHD's event loop for a while.
 *
 * If you use this API in conjunction with a internal select or a
 * thread pool, you must set the option #MHD_USE_ITC to
 * ensure that a resumed request is immediately processed by MHD.
 *
 * Suspended requests continue to count against the total number of
 * requests allowed (per daemon, as well as per IP, if such limits
 * are set).  Suspended requests will NOT time out; timeouts will
 * restart when the request handling is resumed.  While a
 * request is suspended, MHD will not detect disconnects by the
 * client.
 *
 * The only safe time to suspend a request is from either a
 * #MHD_RequestHeaderCallback, #MHD_UploadCallback, or a
 * #MHD_RequestfetchResponseCallback.  Suspending a request
 * at any other time will cause an assertion failure.
 *
 * Finally, it is an API violation to call #MHD_daemon_stop() while
 * having suspended requests (this will at least create memory and
 * socket leaks or lead to undefined behavior).  You must explicitly
 * resume all requests before stopping the daemon.
 *
 * @return action to cause a request to be suspended.
 */
const struct MHD_Action *
MHD_action_suspend (void)
{
  static const struct MHD_Action suspend = {
    .action = &suspend_action,
    .action_cls = NULL
  };

  return &suspend;
}

/* end of action_suspend.c */
