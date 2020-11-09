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
 * @file lib/connection_cleanup.c
 * @brief function to clean up completed connections
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_cleanup.h"
#include "daemon_ip_limit.h"


#ifdef UPGRADE_SUPPORT
/**
 * Finally cleanup upgrade-related resources. It should
 * be called when TLS buffers have been drained and
 * application signaled MHD by #MHD_UPGRADE_ACTION_CLOSE.
 *
 * @param connection handle to the upgraded connection to clean
 */
static void
connection_cleanup_upgraded (struct MHD_Connection *connection)
{
  struct MHD_UpgradeResponseHandle *urh = connection->request.urh;

  if (NULL == urh)
    return;
#ifdef HTTPS_SUPPORT
  /* Signal remote client the end of TLS connection by
   * gracefully closing TLS session. */
  {
    struct MHD_TLS_Plugin *tls;

    if (NULL != (tls = connection->daemon->tls_api))
      (void) tls->shutdown_connection (tls->cls,
				       connection->tls_cs);
  }
  if (MHD_INVALID_SOCKET != urh->mhd.socket)
    MHD_socket_close_chk_ (urh->mhd.socket);
  if (MHD_INVALID_SOCKET != urh->app.socket)
    MHD_socket_close_chk_ (urh->app.socket);
#endif /* HTTPS_SUPPORT */
  connection->request.urh = NULL;
  free (urh);
}
#endif /* UPGRADE_SUPPORT */


/**
 * Free resources associated with all closed connections.  (destroy
 * responses, free buffers, etc.).  All closed connections are kept in
 * the "cleanup" doubly-linked list. 
 *
 * @remark To be called only from thread that process daemon's
 * select()/poll()/etc.
 *
 * @param daemon daemon to clean up
 */
void
MHD_connection_cleanup_ (struct MHD_Daemon *daemon)
{
  struct MHD_Connection *pos;

  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  while (NULL != (pos = daemon->cleanup_tail))
    {
      DLL_remove (daemon->cleanup_head,
		  daemon->cleanup_tail,
		  pos);
      MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);

      if ( (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode) &&
	   (! pos->thread_joined) &&
           (! MHD_join_thread_ (pos->pid.handle)) )
        MHD_PANIC (_("Failed to join a thread\n"));
#ifdef UPGRADE_SUPPORT
      connection_cleanup_upgraded (pos);
#endif /* UPGRADE_SUPPORT */
      MHD_pool_destroy (pos->pool);
#ifdef HTTPS_SUPPORT
      {
	struct MHD_TLS_Plugin *tls;
	
	if (NULL != (tls = daemon->tls_api))
	  tls->teardown_connection (tls->cls,
				    pos->tls_cs);
      }
#endif /* HTTPS_SUPPORT */

      /* clean up the connection */
      if (NULL != daemon->notify_connection_cb)
        daemon->notify_connection_cb (daemon->notify_connection_cb_cls,
				      pos,
				      MHD_CONNECTION_NOTIFY_CLOSED);
      MHD_ip_limit_del (daemon,
                        (const struct sockaddr *) &pos->addr,
                        pos->addr_len);
#ifdef EPOLL_SUPPORT
      if (MHD_ELS_EPOLL == daemon->event_loop_syscall)
        {
          if (0 != (pos->epoll_state & MHD_EPOLL_STATE_IN_EREADY_EDLL))
            {
              EDLL_remove (daemon->eready_head,
                           daemon->eready_tail,
                           pos);
              pos->epoll_state &= ~MHD_EPOLL_STATE_IN_EREADY_EDLL;
            }
          if ( (-1 != daemon->epoll_fd) &&
               (0 != (pos->epoll_state & MHD_EPOLL_STATE_IN_EPOLL_SET)) )
            {
              /* epoll documentation suggests that closing a FD
                 automatically removes it from the epoll set; however,
                 this is not true as if we fail to do manually remove it,
                 we are still seeing an event for this fd in epoll,
                 causing grief (use-after-free...) --- at least on my
                 system. */
              if (0 != epoll_ctl (daemon->epoll_fd,
                                  EPOLL_CTL_DEL,
                                  pos->socket_fd,
                                  NULL))
                MHD_PANIC (_("Failed to remove FD from epoll set\n"));
              pos->epoll_state &= ~MHD_EPOLL_STATE_IN_EPOLL_SET;
            }
        }
#endif
      if (NULL != pos->request.response)
	{
	  MHD_response_queue_for_destroy (pos->request.response);
	  pos->request.response = NULL;
	}
      if (MHD_INVALID_SOCKET != pos->socket_fd)
        MHD_socket_close_chk_ (pos->socket_fd);
      free (pos);

      MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
      daemon->connections--;
      daemon->at_limit = false;
    }
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
}

/* end of connection_cleanup.c */
