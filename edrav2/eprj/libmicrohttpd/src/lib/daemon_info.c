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
 * @file lib/daemon_info.c
 * @brief implementation of MHD_daemon_get_information_sz()
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_cleanup.h"


/**
 * Obtain information about the given daemon.
 * Use wrapper macro #MHD_daemon_get_information() instead of direct use
 * of this function.
 *
 * @param daemon what daemon to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @param return_value_size size of union MHD_DaemonInformation at compile
 *                          time
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
enum MHD_Bool
MHD_daemon_get_information_sz (struct MHD_Daemon *daemon,
			       enum MHD_DaemonInformationType info_type,
			       union MHD_DaemonInformation *return_value,
			       size_t return_value_size)
{
#define CHECK_SIZE(type) if (sizeof(type) < return_value_size)	\
    return MHD_NO

  switch (info_type)
    {
    case MHD_DAEMON_INFORMATION_LISTEN_SOCKET:
      CHECK_SIZE (MHD_socket);
      return_value->listen_socket
	= daemon->listen_socket;
      return MHD_YES;
#ifdef EPOLL_SUPPORT
    case MHD_DAEMON_INFORMATION_EPOLL_FD:
      CHECK_SIZE (int);
      // FIXME: maybe return MHD_NO if we are not using EPOLL?
      return_value->epoll_fd = daemon->epoll_fd;
      return MHD_YES;
#endif
    case MHD_DAEMON_INFORMATION_CURRENT_CONNECTIONS:
      CHECK_SIZE (unsigned int);
      if (MHD_TM_EXTERNAL_EVENT_LOOP == daemon->threading_mode)
        {
          /* Assumes that MHD_run() in not called in other thread
	     (of the application) at the same time. */
          MHD_connection_cleanup_ (daemon);
	  return_value->num_connections
	    = daemon->connections;
        }
      else if (daemon->worker_pool)
        {
          unsigned int i;
          /* Collect the connection information stored in the workers. */
	  return_value->num_connections = 0;
	  for (i = 0; i < daemon->worker_pool_size; i++)
            {
              /* FIXME: next line is thread-safe only if read is atomic. */
              return_value->num_connections
		+= daemon->worker_pool[i].connections;
            }
        }
      else
	return_value->num_connections
	  = daemon->connections;
      return MHD_YES;
    case MHD_DAEMON_INFORMATION_BIND_PORT:
      CHECK_SIZE (uint16_t);
      // FIXME: return MHD_NO if port is not known/UNIX?
      return_value->port = daemon->listen_port;
      return MHD_YES;
    default:
      return MHD_NO;
    }
  
#undef CHECK_SIZE
}

/* end of daemon_info.c */
