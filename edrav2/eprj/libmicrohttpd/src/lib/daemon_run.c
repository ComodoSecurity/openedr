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
 * @file lib/daemon_run.c
 * @brief generic function to run event loop of a daemon
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_cleanup.h"
#include "daemon_epoll.h"
#include "daemon_poll.h"
#include "daemon_select.h"


/**
 * Run webserver operations (without blocking unless in client
 * callbacks).  This method should be called by clients in combination
 * with #MHD_get_fdset if the client-controlled select method is used
 * and #MHD_get_timeout().
 *
 * This function is a convenience method, which is useful if the
 * fd_sets from #MHD_get_fdset were not directly passed to `select()`;
 * with this function, MHD will internally do the appropriate `select()`
 * call itself again.  While it is always safe to call #MHD_run (if
 * #MHD_USE_INTERNAL_POLLING_THREAD is not set), you should call
 * #MHD_run_from_select if performance is important (as it saves an
 * expensive call to `select()`).
 *
 * @param daemon daemon to run
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
enum MHD_StatusCode
MHD_daemon_run (struct MHD_Daemon *daemon)
{
  enum MHD_StatusCode sc;

  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
  if (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode)
    return MHD_SC_CONFIGURATION_MISMATCH_FOR_RUN_EXTERNAL;
  switch (daemon->event_loop_syscall)
    {
    case MHD_ELS_POLL:
      sc = MHD_daemon_poll_ (daemon,
 			     MHD_NO);
      MHD_connection_cleanup_ (daemon);
      return sc;
#ifdef EPOLL_SUPPORT
    case MHD_ELS_EPOLL:
      sc = MHD_daemon_epoll_ (daemon,
  	 	              MHD_NO);
      MHD_connection_cleanup_ (daemon);
      return sc;
#endif
    case MHD_ELS_SELECT:
      return MHD_daemon_select_ (daemon,
 			         MHD_NO);
      /* MHD_select does MHD_connection_cleanup_ already */
    default:
      return MHD_SC_CONFIGURATION_UNEXPECTED_ELS;
    }
}

/* end of daemon_run.c */
