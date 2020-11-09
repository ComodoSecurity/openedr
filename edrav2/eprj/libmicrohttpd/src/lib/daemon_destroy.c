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
 * @file lib/daemon_destroy.c
 * @brief main functions to destroy a daemon
 * @author Christian Grothoff
 */
#include "internal.h"
#include "request_resume.h"
#include "daemon_close_all_connections.h"


/**
 * Stop all worker threads from the worker pool.
 *
 * @param daemon master daemon controling the workers
 */
static void
stop_workers (struct MHD_Daemon *daemon)
{
  MHD_socket fd;
  unsigned int i;

  mhd_assert (1 < daemon->worker_pool_size);
  mhd_assert (1 < daemon->threading_mode);
  if (daemon->was_quiesced)
    fd = MHD_INVALID_SOCKET; /* Do not use FD if daemon was quiesced */
  else
    fd = daemon->listen_socket;
  /* Let workers shutdown in parallel. */
  for (i = 0; i < daemon->worker_pool_size; i++)
    {
      daemon->worker_pool[i].shutdown = true;
      if (MHD_ITC_IS_VALID_(daemon->worker_pool[i].itc))
	{
	  if (! MHD_itc_activate_ (daemon->worker_pool[i].itc,
				   "e"))
	    MHD_PANIC (_("Failed to signal shutdown via inter-thread communication channel."));
	}
      else
	{
	  /* Better hope shutdown() works... */
	  mhd_assert (MHD_INVALID_SOCKET != fd);
	}
    }
#ifdef HAVE_LISTEN_SHUTDOWN
  if (MHD_INVALID_SOCKET != fd)
    {
      (void) shutdown (fd,
		       SHUT_RDWR);
    }
#endif /* HAVE_LISTEN_SHUTDOWN */
  for (i = 0; i < daemon->worker_pool_size; ++i)
    {
      MHD_daemon_destroy (&daemon->worker_pool[i]);
    }
  free (daemon->worker_pool);
  daemon->worker_pool = NULL;
  /* FIXME: does this still hold? */
  mhd_assert (MHD_ITC_IS_INVALID_(daemon->itc));
#ifdef EPOLL_SUPPORT
  mhd_assert (-1 == daemon->epoll_fd);
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  mhd_assert (-1 == daemon->epoll_upgrade_fd);
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
#endif /* EPOLL_SUPPORT */
}


/**
 * Shutdown and destroy an HTTP daemon.
 *
 * @param daemon daemon to stop
 * @ingroup event
 */
void
MHD_daemon_destroy (struct MHD_Daemon *daemon)
{
  MHD_socket fd;

  daemon->shutdown = true;
  if (daemon->was_quiesced)
    fd = MHD_INVALID_SOCKET; /* Do not use FD if daemon was quiesced */
  else
    fd = daemon->listen_socket;

  /* FIXME: convert from here to microhttpd2-style API! */

  if (NULL != daemon->worker_pool)
    { /* Master daemon with worker pool. */
      stop_workers (daemon);
    }
  else
    {
      mhd_assert (0 == daemon->worker_pool_size);
      /* Worker daemon or single-thread daemon. */
      if (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode)
        {
	  /* Worker daemon or single daemon with internal thread(s). */
	  /* Separate thread(s) is used for polling sockets. */
	  if (MHD_ITC_IS_VALID_(daemon->itc))
	    {
	      if (! MHD_itc_activate_ (daemon->itc,
				       "e"))
		MHD_PANIC (_("Failed to signal shutdown via inter-thread communication channel"));
	    }
	  else
	    {
#ifdef HAVE_LISTEN_SHUTDOWN
	      if (MHD_INVALID_SOCKET != fd)
		{
		  if (NULL == daemon->master)
		    (void) shutdown (fd,
				     SHUT_RDWR);
		}
	      else
#endif /* HAVE_LISTEN_SHUTDOWN */
		mhd_assert (false); /* Should never happen */
	    }

	  if (! MHD_join_thread_ (daemon->pid.handle))
	    {
	      MHD_PANIC (_("Failed to join a thread\n"));
	    }
	  /* close_all_connections() was called in daemon thread. */
	}
      else
        {
          /* No internal threads are used for polling sockets
	     (external event loop) */
          MHD_daemon_close_all_connections_ (daemon);
        }
      if (MHD_ITC_IS_VALID_ (daemon->itc))
        MHD_itc_destroy_chk_ (daemon->itc);

#ifdef EPOLL_SUPPORT
      if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
           (-1 != daemon->epoll_fd) )
        MHD_socket_close_chk_ (daemon->epoll_fd);
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
      if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
           (-1 != daemon->epoll_upgrade_fd) )
        MHD_socket_close_chk_ (daemon->epoll_upgrade_fd);
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
#endif /* EPOLL_SUPPORT */

      MHD_mutex_destroy_chk_ (&daemon->cleanup_connection_mutex);
    }

  if (NULL != daemon->master)
    return;
  /* Cleanup that should be done only one time in master/single daemon.
   * Do not perform this cleanup in worker daemons. */

  if (MHD_INVALID_SOCKET != fd)
    MHD_socket_close_chk_ (fd);

  /* TLS clean up */
#ifdef HTTPS_SUPPORT
  if (NULL != daemon->tls_api)
    {
#if FIXME_TLS_API
      if (daemon->have_dhparams)
	{
	  gnutls_dh_params_deinit (daemon->https_mem_dhparams);
	  daemon->have_dhparams = false;
	}
      gnutls_priority_deinit (daemon->priority_cache);
      if (daemon->x509_cred)
	gnutls_certificate_free_credentials (daemon->x509_cred);
#endif
    }
#endif /* HTTPS_SUPPORT */

#ifdef DAUTH_SUPPORT
  free (daemon->nnc);
  MHD_mutex_destroy_chk_ (&daemon->nnc_lock);
#endif
  MHD_mutex_destroy_chk_ (&daemon->per_ip_connection_mutex);
  free (daemon);
}

/* end of daemon_destroy.c */
