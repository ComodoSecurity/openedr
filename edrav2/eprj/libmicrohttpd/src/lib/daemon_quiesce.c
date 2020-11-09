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
 * @file lib/daemon_quiesce.c
 * @brief main functions to quiesce a daemon
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Stop accepting connections from the listening socket.  Allows
 * clients to continue processing, but stops accepting new
 * connections.  Note that the caller is responsible for closing the
 * returned socket; however, if MHD is run using threads (anything but
 * external select mode), it must not be closed until AFTER
 * #MHD_stop_daemon has been called (as it is theoretically possible
 * that an existing thread is still using it).
 *
 * Note that some thread modes require the caller to have passed
 * #MHD_USE_ITC when using this API.  If this daemon is
 * in one of those modes and this option was not given to
 * #MHD_start_daemon, this function will return #MHD_INVALID_SOCKET.
 *
 * @param daemon daemon to stop accepting new connections for
 * @return old listen socket on success, #MHD_INVALID_SOCKET if
 *         the daemon was already not listening anymore, or
 *         was never started
 * @ingroup specialized
 */
MHD_socket
MHD_daemon_quiesce (struct MHD_Daemon *daemon)
{
  MHD_socket listen_socket;

  if (MHD_INVALID_SOCKET == (listen_socket = daemon->listen_socket))
    return MHD_INVALID_SOCKET;
  if ( (daemon->disable_itc) &&
       (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode) )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_SYSCALL_QUIESCE_REQUIRES_ITC,
		"Using MHD_quiesce_daemon in this mode requires ITC\n");
#endif
      return MHD_INVALID_SOCKET;
    }

  if (NULL != daemon->worker_pool)
    {
      unsigned int i;

      for (i = 0; i < daemon->worker_pool_size; i++)
	{
	  struct MHD_Daemon *worker = &daemon->worker_pool[i];
	  
	  worker->was_quiesced = true;
#ifdef EPOLL_SUPPORT
	  if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
	       (-1 != worker->epoll_fd) &&
	       (worker->listen_socket_in_epoll) )
	    {
	      if (0 != epoll_ctl (worker->epoll_fd,
				  EPOLL_CTL_DEL,
				  listen_socket,
				  NULL))
		MHD_PANIC (_("Failed to remove listen FD from epoll set\n"));
	      worker->listen_socket_in_epoll = false;
	    }
	  else
#endif
	    if (MHD_ITC_IS_VALID_(worker->itc))
	      {
		if (! MHD_itc_activate_ (worker->itc,
					 "q"))
		  MHD_PANIC (_("Failed to signal quiesce via inter-thread communication channel"));
	      }
	}
      daemon->was_quiesced = true;
#ifdef EPOLL_SUPPORT
      if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
	   (-1 != daemon->epoll_fd) &&
	   (daemon->listen_socket_in_epoll) )
	{
	  if (0 != epoll_ctl (daemon->epoll_fd,
			      EPOLL_CTL_DEL,
			      listen_socket,
			      NULL))
	    MHD_PANIC ("Failed to remove listen FD from epoll set\n");
	  daemon->listen_socket_in_epoll = false;
	}
#endif
    }
  
  if ( (MHD_ITC_IS_VALID_(daemon->itc)) &&
       (! MHD_itc_activate_ (daemon->itc,
			     "q")) )
    MHD_PANIC (_("Failed to signal quiesce via inter-thread communication channel"));

  /* FIXME: we might want some bi-directional communication here
     (in both the thread-pool and single-thread case!) 
     to be sure that the threads have stopped using the listen
     socket, otherwise there is still the possibility of a race
     between a thread accept()ing and the caller closing and
     re-binding the socket. */
  
  return listen_socket;
}

/* end of daemon_quiesce.c */

