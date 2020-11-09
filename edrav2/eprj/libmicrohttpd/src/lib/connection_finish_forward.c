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
 * @file lib/connection_finish_forward.c
 * @brief complete upgrade socket forwarding operation in TLS mode
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_finish_forward.h"


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Stop TLS forwarding on upgraded connection and
 * reflect remote disconnect state to socketpair.
 * @remark In thread-per-connection mode this function
 * can be called from any thread, in other modes this
 * function must be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param connection the upgraded connection
 */
void
MHD_connection_finish_forward_ (struct MHD_Connection *connection)
{
  struct MHD_Daemon *daemon = connection->daemon;
  struct MHD_UpgradeResponseHandle *urh = connection->request.urh;

  if (NULL == daemon->tls_api)
    return; /* Nothing to do with non-TLS connection. */

  if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
    DLL_remove (daemon->urh_head,
                daemon->urh_tail,
                urh);
#if EPOLL_SUPPORT
  if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
       (0 != epoll_ctl (daemon->epoll_upgrade_fd,
                        EPOLL_CTL_DEL,
                        connection->socket_fd,
                        NULL)) )
    {
      MHD_PANIC (_("Failed to remove FD from epoll set\n"));
    }
  if (urh->in_eready_list)
    {
      EDLL_remove (daemon->eready_urh_head,
		   daemon->eready_urh_tail,
		   urh);
      urh->in_eready_list = false;
    }
#endif /* EPOLL_SUPPORT */
  if (MHD_INVALID_SOCKET != urh->mhd.socket)
    {
#if EPOLL_SUPPORT
      if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
           (0 != epoll_ctl (daemon->epoll_upgrade_fd,
                            EPOLL_CTL_DEL,
                            urh->mhd.socket,
                            NULL)) )
        {
          MHD_PANIC (_("Failed to remove FD from epoll set\n"));
        }
#endif /* EPOLL_SUPPORT */
      /* Reflect remote disconnect to application by breaking
       * socketpair connection. */
      shutdown (urh->mhd.socket,
		SHUT_RDWR);
    }
  /* Socketpair sockets will remain open as they will be
   * used with MHD_UPGRADE_ACTION_CLOSE. They will be
   * closed by MHD_cleanup_upgraded_connection_() during
   * connection's final cleanup.
   */
}
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT*/

/* end of connection_finish_forward.c */
