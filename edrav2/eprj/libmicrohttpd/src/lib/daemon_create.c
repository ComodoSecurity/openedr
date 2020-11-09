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
 * @file lib/daemon_create.c
 * @brief main functions to create a daemon
 * @author Christian Grothoff
 */
#include "internal.h"
#include "init.h"


/**
 * Logging implementation that logs to a file given
 * as the @a cls.
 *
 * @param cls a `FILE *` to log to
 * @param sc status code of the event (ignored)
 * @param fm format string (`printf()`-style)
 * @param ap arguments to @a fm
 * @ingroup logging
 */
static void
file_logger (void *cls,
	     enum MHD_StatusCode sc,
	     const char *fm,
	     va_list ap)
{
  FILE *f = cls;

  (void) sc;
  (void) vfprintf (f,
		   fm,
		   ap);
}


/**
 * Process escape sequences ('%HH') Updates val in place; the
 * result should be UTF-8 encoded and cannot be larger than the input.
 * The result must also still be 0-terminated.
 *
 * @param cls closure (use NULL)
 * @param req handle to request, not used
 * @param val value to unescape (modified in the process)
 * @return length of the resulting val (strlen(val) maybe
 *  shorter afterwards due to elimination of escape sequences)
 */
static size_t
unescape_wrapper (void *cls,
                  struct MHD_Request *req,
                  char *val)
{
  (void) cls; /* Mute compiler warning. */
  (void) req; /* Mute compiler warning. */
  return MHD_http_unescape (val);
}


/**
 * Create (but do not yet start) an MHD daemon.
 * Usually, you will want to set various options before
 * starting the daemon with #MHD_daemon_start().
 *
 * @param cb function to be called for incoming requests
 * @param cb_cls closure for @a cb
 * @return NULL on error
 */
struct MHD_Daemon *
MHD_daemon_create (MHD_RequestCallback cb,
		   void *cb_cls)
{
  struct MHD_Daemon *daemon;

  MHD_check_global_init_();
  if (NULL == (daemon = malloc (sizeof (struct MHD_Daemon))))
    return NULL;
  memset (daemon,
	  0,
	  sizeof (struct MHD_Daemon));
#ifdef EPOLL_SUPPORT
  daemon->epoll_itc_marker = "itc_marker";
#endif
  daemon->rc = cb;
  daemon->rc_cls = cb_cls;
  daemon->logger = &file_logger;
  daemon->logger_cls = stderr;
  daemon->unescape_cb = &unescape_wrapper;
  daemon->connection_memory_limit_b = POOL_SIZE_DEFAULT;
  daemon->connection_memory_increment_b = BUF_INC_SIZE_DEFAULT;
#if ENABLE_DAUTH
  daemon->digest_nc_length = DIGEST_NC_LENGTH_DEFAULT;
#endif
  daemon->listen_backlog = LISTEN_BACKLOG_DEFAULT;  
  daemon->fo_queue_length = FO_QUEUE_LENGTH_DEFAULT;
  daemon->listen_socket = MHD_INVALID_SOCKET;

  if (! MHD_mutex_init_ (&daemon->cleanup_connection_mutex))
    {
      free (daemon);
      return NULL;
    }  
  if (! MHD_mutex_init_ (&daemon->per_ip_connection_mutex))
    {
      (void) MHD_mutex_destroy_ (&daemon->cleanup_connection_mutex);
      free (daemon);
      return NULL;
    }
#ifdef DAUTH_SUPPORT
  if (! MHD_mutex_init_ (&daemon->nnc_lock))
    {
      (void) MHD_mutex_destroy_ (&daemon->cleanup_connection_mutex);
      (void) MHD_mutex_destroy_ (&daemon->per_ip_connection_mutex);
      free (daemon);
      return NULL;
    }
#endif
  return daemon;
}


/* end of daemon_create.c */
