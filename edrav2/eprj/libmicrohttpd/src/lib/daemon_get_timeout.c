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
 * @file lib/daemon_get_timeout.c
 * @brief function to obtain timeout for event loop
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Obtain timeout value for polling function for this daemon.
 * This function set value to amount of milliseconds for which polling
 * function (`select()` or `poll()`) should at most block, not the
 * timeout value set for connections.
 * It is important to always use this function, even if connection
 * timeout is not set, as in some cases MHD may already have more
 * data to process on next turn (data pending in TLS buffers,
 * connections are already ready with epoll etc.) and returned timeout
 * will be zero.
 *
 * @param daemon daemon to query for timeout
 * @param timeout set to the timeout (in milliseconds)
 * @return #MHD_SC_OK on success, #MHD_SC_NO_TIMEOUT if timeouts are
 *        not used (or no connections exist that would
 *        necessitate the use of a timeout right now), otherwise
 *        an error code
 * @ingroup event
 */
enum MHD_StatusCode
MHD_daemon_get_timeout (struct MHD_Daemon *daemon,
			MHD_UNSIGNED_LONG_LONG *timeout)
{
  time_t earliest_deadline;
  time_t now;
  struct MHD_Connection *pos;
  bool have_timeout;

  if (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode)
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_CONFIGURATION_MISMATCH_FOR_GET_TIMEOUT,
                _("Illegal call to MHD_get_timeout\n"));
#endif
      return MHD_SC_CONFIGURATION_MISSMATCH_FOR_GET_TIMEOUT;
    }

  if (daemon->data_already_pending)
    {
      /* Some data already waiting to be processed. */
      *timeout = 0;
      return MHD_SC_OK;
    }

#ifdef EPOLL_SUPPORT
  if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
       ((NULL != daemon->eready_head)
#if defined(UPGRADE_SUPPORT) && defined(HTTPS_SUPPORT)
	 || (NULL != daemon->eready_urh_head)
#endif /* UPGRADE_SUPPORT && HTTPS_SUPPORT */
	 ) )
    {
	  /* Some connection(s) already have some data pending. */
      *timeout = 0;
      return MHD_SC_OK;
    }
#endif /* EPOLL_SUPPORT */

  have_timeout = false;
  earliest_deadline = 0; /* avoid compiler warnings */
  for (pos = daemon->manual_timeout_tail; NULL != pos; pos = pos->prevX)
    {
      if (0 != pos->connection_timeout)
	{
	  if ( (! have_timeout) ||
	       (earliest_deadline - pos->last_activity > pos->connection_timeout) )
	    earliest_deadline = pos->last_activity + pos->connection_timeout;
	  have_timeout = true;
	}
    }
  /* normal timeouts are sorted, so we only need to look at the 'tail' (oldest) */
  pos = daemon->normal_timeout_tail;
  if ( (NULL != pos) &&
       (0 != pos->connection_timeout) )
    {
      if ( (! have_timeout) ||
	   (earliest_deadline - pos->connection_timeout > pos->last_activity) )
	earliest_deadline = pos->last_activity + pos->connection_timeout;
      have_timeout = true;
    }

  if (! have_timeout)
    return MHD_SC_NO_TIMEOUT;
  now = MHD_monotonic_sec_counter();
  if (earliest_deadline < now)
    *timeout = 0;
  else
    {
      const time_t second_left = earliest_deadline - now;
      if (second_left > ULLONG_MAX / 1000) /* Ignore compiler warning: 'second_left' is always positive. */
        *timeout = ULLONG_MAX;
      else
        *timeout = 1000LL * second_left;
  }
  return MHD_SC_OK;
}

/* end of daemon_get_timeout.c */
