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
 * @file lib/connection_options.c
 * @brief functions to set per-connection options
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Set custom timeout for the given connection.  Specified as the
 * number of seconds.  Use zero for no timeout.  Calling this function
 * will reset timeout timer.
 *
 * @param connection connection to configure timeout for
 * @param timeout_s new timeout in seconds
 */
void
MHD_connection_set_timeout (struct MHD_Connection *connection,
			    unsigned int timeout_s)
{
  struct MHD_Daemon *daemon = connection->daemon;
  
  connection->last_activity = MHD_monotonic_sec_counter();
  if (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode)
    {
      /* Simple case, no need to lock to update DLLs */
      connection->connection_timeout = (time_t) timeout_s;
      return;
    }

  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  if (! connection->suspended)
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
  connection->connection_timeout = (time_t) timeout_s;
  if (! connection->suspended)
    {
      if (connection->connection_timeout ==
	  daemon->connection_default_timeout)
	XDLL_insert (daemon->normal_timeout_head,
		     daemon->normal_timeout_tail,
		     connection);
      else
	XDLL_insert (daemon->manual_timeout_head,
		     daemon->manual_timeout_tail,
		     connection);
    }
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
}


/**
 * Update the 'last_activity' field of the connection to the current
 * time and move the connection to the head of the 'normal_timeout'
 * list if the timeout for the connection uses the default value.
 *
 * @param connection the connection that saw some activity
 */
void
MHD_update_last_activity_ (struct MHD_Connection *connection)
{
  struct MHD_Daemon *daemon = connection->daemon;

  if (0 == connection->connection_timeout)
    return; /* Skip update of activity for connections
               without timeout timer. */
  if (connection->suspended)
    return; /* no activity on suspended connections */

  connection->last_activity = MHD_monotonic_sec_counter();
  if (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode)
    return; /* each connection has personal timeout */

  if (connection->connection_timeout !=
      daemon->connection_default_timeout)
    return; /* custom timeout, no need to move it in "normal" DLL */

  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  /* move connection to head of timeout list (by remove + add operation) */
  XDLL_remove (daemon->normal_timeout_head,
	       daemon->normal_timeout_tail,
	       connection);
  XDLL_insert (daemon->normal_timeout_head,
	       daemon->normal_timeout_tail,
	       connection);
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
}

/* end of connection_options.c */
