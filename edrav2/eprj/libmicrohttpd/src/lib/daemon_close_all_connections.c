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
 * @file lib/daemon_close_all_connections.c
 * @brief function to close all connections open at a daemon
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_cleanup.h"
#include "connection_close.h"
#include "connection_finish_forward.h"
#include "daemon_close_all_connections.h"
#include "request_resume.h"
#include "upgrade_process.h"


/**
 * Close the given connection, remove it from all of its
 * DLLs and move it into the cleanup queue.
 * @remark To be called only from thread that
 * process daemon's select()/poll()/etc.
 *
 * @param pos connection to move to cleanup
 */
static void
close_connection (struct MHD_Connection *pos)
{
  struct MHD_Daemon *daemon = pos->daemon;

  if (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode)
    {
      MHD_connection_mark_closed_ (pos);
      return; /* must let thread to do the rest */
    }
  MHD_connection_close_ (pos,
                         MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN);

  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);

  mhd_assert (! pos->suspended);
  mhd_assert (! pos->resuming);
  if (pos->connection_timeout ==
      pos->daemon->connection_default_timeout)
    XDLL_remove (daemon->normal_timeout_head,
		 daemon->normal_timeout_tail,
		 pos);
  else
    XDLL_remove (daemon->manual_timeout_head,
		 daemon->manual_timeout_tail,
		 pos);
  DLL_remove (daemon->connections_head,
	      daemon->connections_tail,
	      pos);
  DLL_insert (daemon->cleanup_head,
	      daemon->cleanup_tail,
	      pos);

  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
}


/**
 * Close all connections for the daemon.  Must only be called when
 * MHD_Daemon::shutdown was set to true.
 *
 * @remark To be called only from thread that process daemon's
 * select()/poll()/etc.
 *
 * @param daemon daemon to close down
 */
void
MHD_daemon_close_all_connections_ (struct MHD_Daemon *daemon)
{
  struct MHD_Connection *pos;
  const bool used_thr_p_c = (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode);
#ifdef UPGRADE_SUPPORT
  const bool upg_allowed = (! daemon->disallow_upgrade);
#endif /* UPGRADE_SUPPORT */
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  struct MHD_UpgradeResponseHandle *urh;
  struct MHD_UpgradeResponseHandle *urhn;
  const bool used_tls = (NULL != daemon->tls_api);

  mhd_assert (NULL == daemon->worker_pool);
  mhd_assert (daemon->shutdown);
  /* give upgraded HTTPS connections a chance to finish */
  /* 'daemon->urh_head' is not used in thread-per-connection mode. */
  for (urh = daemon->urh_tail; NULL != urh; urh = urhn)
    {
      urhn = urh->prev;
      /* call generic forwarding function for passing data
         with chance to detect that application is done. */
      MHD_upgrade_response_handle_process_ (urh);
      MHD_connection_finish_forward_ (urh->connection);
      urh->clean_ready = true;
      /* Resuming will move connection to cleanup list. */
      MHD_request_resume (&urh->connection->request);
    }
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */

  /* Give suspended connections a chance to resume to avoid
     running into the check for there not being any suspended
     connections left in case of a tight race with a recently
     resumed connection. */
  if (! daemon->disallow_suspend_resume)
    {
      daemon->resuming = true; /* Force check for pending resume. */
      MHD_resume_suspended_connections_ (daemon);
    }
  /* first, make sure all threads are aware of shutdown; need to
     traverse DLLs in peace... */
  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
#ifdef UPGRADE_SUPPORT
  if (upg_allowed)
    {
      struct MHD_Connection * susp;

      susp = daemon->suspended_connections_tail;
      while (NULL != susp)
        {
          if (NULL == susp->request.urh) /* "Upgraded" connection? */
            MHD_PANIC (_("MHD_stop_daemon() called while we have suspended connections.\n"));
#ifdef HTTPS_SUPPORT
          else if (used_tls &&
                   used_thr_p_c &&
                   (! susp->request.urh->clean_ready) )
            shutdown (susp->request.urh->app.socket,
                      SHUT_RDWR); /* Wake thread by shutdown of app socket. */
#endif /* HTTPS_SUPPORT */
          else
            {
#ifdef HAVE_MESSAGES
              if (! susp->request.urh->was_closed)
                MHD_DLOG (daemon,
			  MHD_SC_SHUTDOWN_WITH_OPEN_UPGRADED_CONNECTION,
                          _("Initiated daemon shutdown while \"upgraded\" connection was not closed.\n"));
#endif
              susp->request.urh->was_closed = true;
              /* If thread-per-connection is used, connection's thread
               * may still processing "upgrade" (exiting). */
              if (! used_thr_p_c)
                MHD_connection_finish_forward_ (susp);
              /* Do not use MHD_resume_connection() as mutex is
               * already locked. */
              susp->resuming = true;
              daemon->resuming = true;
            }
          susp = susp->prev;
        }
    }
  else /* This 'else' is combined with next 'if' */
#endif /* UPGRADE_SUPPORT */
  if (NULL != daemon->suspended_connections_head)
    MHD_PANIC (_("MHD_stop_daemon() called while we have suspended connections.\n"));
  for (pos = daemon->connections_tail; NULL != pos; pos = pos->prev)
    {
      shutdown (pos->socket_fd,
                SHUT_RDWR);
#if MHD_WINSOCK_SOCKETS
      if ( (used_thr_p_c) &&
           (MHD_ITC_IS_VALID_(daemon->itc)) &&
           (! MHD_itc_activate_ (daemon->itc,
				 "e")) )
        MHD_PANIC (_("Failed to signal shutdown via inter-thread communication channel"));
#endif
    }

  /* now, collect per-connection threads */
  if (used_thr_p_c)
    {
      pos = daemon->connections_tail;
      while (NULL != pos)
      {
        if (! pos->thread_joined)
          {
            MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
            if (! MHD_join_thread_ (pos->pid.handle))
              MHD_PANIC (_("Failed to join a thread\n"));
            MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
            pos->thread_joined = true;
            /* The thread may have concurrently modified the DLL,
               need to restart from the beginning */
            pos = daemon->connections_tail;
            continue;
          }
        pos = pos->prev;
      }
    }
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);

#ifdef UPGRADE_SUPPORT
  /* Finished threads with "upgraded" connections need to be moved
   * to cleanup list by resume_suspended_connections(). */
  /* "Upgraded" connections that were not closed explicitly by
   * application should be moved to cleanup list too. */
  if (upg_allowed)
    {
      daemon->resuming = true; /* Force check for pending resume. */
      MHD_resume_suspended_connections_ (daemon);
    }
#endif /* UPGRADE_SUPPORT */

  /* now that we're alone, move everyone to cleanup */
  while (NULL != (pos = daemon->connections_tail))
  {
    if ( (used_thr_p_c) &&
         (! pos->thread_joined) )
      MHD_PANIC (_("Failed to join a thread\n"));
    close_connection (pos);
  }
  MHD_connection_cleanup_ (daemon);
}

/* end of daemon_close_all_connections.c */
