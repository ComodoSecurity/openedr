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
 * @file lib/daemon_select.c
 * @brief function to run select()-based event loop of a daemon
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_add.h"
#include "connection_call_handlers.h"
#include "connection_cleanup.h"
#include "connection_finish_forward.h"
#include "daemon_select.h"
#include "daemon_epoll.h"
#include "request_resume.h"
#include "upgrade_process.h"


/**
 * We defined a macro with the same name as a function we
 * are implementing here. Need to undef the macro to avoid
 * causing a conflict.
 */
#undef MHD_daemon_get_fdset

/**
 * Obtain the `select()` sets for this daemon.  Daemon's FDs will be
 * added to fd_sets. To get only daemon FDs in fd_sets, call FD_ZERO
 * for each fd_set before calling this function. FD_SETSIZE is assumed
 * to be platform's default.
 *
 * This function should only be called in when MHD is configured to
 * use external select with 'select()' or with 'epoll'.  In the latter
 * case, it will only add the single 'epoll()' file descriptor used by
 * MHD to the sets.  It's necessary to use #MHD_daemon_get_timeout() in
 * combination with this function.
 *
 * This function must be called only for daemon started without
 * #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value); can be NULL
 * @return #MHD_SC_OK on success, otherwise error code
 * @ingroup event
 */
enum MHD_StatusCode
MHD_daemon_get_fdset (struct MHD_Daemon *daemon,
		      fd_set *read_fd_set,
		      fd_set *write_fd_set,
		      fd_set *except_fd_set,
		      MHD_socket *max_fd)
{
  return MHD_daemon_get_fdset2 (daemon,
				read_fd_set,
				write_fd_set,
				except_fd_set,
				max_fd,
				_MHD_SYS_DEFAULT_FD_SETSIZE);
}


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Obtain the select() file descriptor sets for the
 * given @a urh.
 *
 * @param urh upgrade handle to wait for
 * @param[out] rs read set to initialize
 * @param[out] ws write set to initialize
 * @param[out] es except set to initialize
 * @param[out] max_fd maximum FD to update
 * @param fd_setsize value of FD_SETSIZE
 * @return true on success, false on error
 */
static bool
urh_to_fdset (struct MHD_UpgradeResponseHandle *urh,
              fd_set *rs,
              fd_set *ws,
              fd_set *es,
              MHD_socket *max_fd,
              unsigned int fd_setsize)
{
  const MHD_socket conn_sckt = urh->connection->socket_fd;
  const MHD_socket mhd_sckt = urh->mhd.socket;
  bool res = true;

  /* Do not add to 'es' only if socket is closed
   * or not used anymore. */
  if (MHD_INVALID_SOCKET != conn_sckt)
    {
      if ( (urh->in_buffer_used < urh->in_buffer_size) &&
           (! MHD_add_to_fd_set_ (conn_sckt,
                                  rs,
                                  max_fd,
                                  fd_setsize)) )
        res = false;
      if ( (0 != urh->out_buffer_used) &&
           (! MHD_add_to_fd_set_ (conn_sckt,
                                  ws,
                                  max_fd,
                                  fd_setsize)) )
        res = false;
      /* Do not monitor again for errors if error was detected before as
       * error state is remembered. */
      if ((0 == (urh->app.celi & MHD_EPOLL_STATE_ERROR)) &&
          ((0 != urh->in_buffer_size) ||
           (0 != urh->out_buffer_size) ||
           (0 != urh->out_buffer_used)))
        MHD_add_to_fd_set_ (conn_sckt,
                            es,
                            max_fd,
                            fd_setsize);
    }
  if (MHD_INVALID_SOCKET != mhd_sckt)
    {
      if ( (urh->out_buffer_used < urh->out_buffer_size) &&
           (! MHD_add_to_fd_set_ (mhd_sckt,
                                  rs,
                                  max_fd,
                                  fd_setsize)) )
        res = false;
      if ( (0 != urh->in_buffer_used) &&
           (! MHD_add_to_fd_set_ (mhd_sckt,
                                  ws,
                                  max_fd,
                                  fd_setsize)) )
        res = false;
      /* Do not monitor again for errors if error was detected before as
       * error state is remembered. */
      if ((0 == (urh->mhd.celi & MHD_EPOLL_STATE_ERROR)) &&
          ((0 != urh->out_buffer_size) ||
           (0 != urh->in_buffer_size) ||
           (0 != urh->in_buffer_used)))
        MHD_add_to_fd_set_ (mhd_sckt,
                            es,
                            max_fd,
                            fd_setsize);
    }

  return res;
}
#endif


/**
 * Internal version of #MHD_daemon_get_fdset2().
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value); can be NULL
 * @param fd_setsize value of FD_SETSIZE
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
static enum MHD_StatusCode
internal_get_fdset2 (struct MHD_Daemon *daemon,
                     fd_set *read_fd_set,
                     fd_set *write_fd_set,
                     fd_set *except_fd_set,
                     MHD_socket *max_fd,
                     unsigned int fd_setsize)

{
  struct MHD_Connection *pos;
  struct MHD_Connection *posn;
  enum MHD_StatusCode result = MHD_SC_OK;
  MHD_socket ls;

  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;

  ls = daemon->listen_socket;
  if ( (MHD_INVALID_SOCKET != ls) &&
       (! daemon->was_quiesced) &&
       (! MHD_add_to_fd_set_ (ls,
                              read_fd_set,
                              max_fd,
                              fd_setsize)) )
    result = MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;

  /* Add all sockets to 'except_fd_set' as well to watch for
   * out-of-band data. However, ignore errors if INFO_READ
   * or INFO_WRITE sockets will not fit 'except_fd_set'. */
  /* Start from oldest connections. Make sense for W32 FDSETs. */
  for (pos = daemon->connections_tail; NULL != pos; pos = posn)
    {
      posn = pos->prev;

      switch (pos->request.event_loop_info)
	{
	case MHD_EVENT_LOOP_INFO_READ:
	  if (! MHD_add_to_fd_set_ (pos->socket_fd,
                                    read_fd_set,
                                    max_fd,
                                    fd_setsize))
	    result = MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
#ifdef MHD_POSIX_SOCKETS
          MHD_add_to_fd_set_ (pos->socket_fd,
                              except_fd_set,
                              max_fd,
                              fd_setsize);
#endif /* MHD_POSIX_SOCKETS */
	  break;
	case MHD_EVENT_LOOP_INFO_WRITE:
	  if (! MHD_add_to_fd_set_ (pos->socket_fd,
                                    write_fd_set,
                                    max_fd,
                                    fd_setsize))
	    result = MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
#ifdef MHD_POSIX_SOCKETS
          MHD_add_to_fd_set_ (pos->socket_fd,
                              except_fd_set,
                              max_fd,
                              fd_setsize);
#endif /* MHD_POSIX_SOCKETS */
	  break;
	case MHD_EVENT_LOOP_INFO_BLOCK:
	  if ( (NULL == except_fd_set) ||
	      ! MHD_add_to_fd_set_ (pos->socket_fd,
	                            except_fd_set,
                                    max_fd,
                                    fd_setsize))
            result = MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
	  break;
	case MHD_EVENT_LOOP_INFO_CLEANUP:
	  /* this should never happen */
	  break;
	}
    }
#ifdef MHD_WINSOCK_SOCKETS
  /* W32 use limited array for fd_set so add INFO_READ/INFO_WRITE sockets
   * only after INFO_BLOCK sockets to ensure that INFO_BLOCK sockets will
   * not be pushed out. */
  for (pos = daemon->connections_tail; NULL != pos; pos = posn)
    {
      posn = pos->prev;
      MHD_add_to_fd_set_ (pos->socket_fd,
                          except_fd_set,
                          max_fd,
                          fd_setsize);
    }
#endif /* MHD_WINSOCK_SOCKETS */
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  {
    struct MHD_UpgradeResponseHandle *urh;

    for (urh = daemon->urh_tail; NULL != urh; urh = urh->prev)
      {
        if (! urh_to_fdset (urh,
			    read_fd_set,
			    write_fd_set,
			    except_fd_set,
			    max_fd,
			    fd_setsize))
          result = MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
      }
  }
#endif
  return result;
}


/**
 * Obtain the `select()` sets for this daemon.  Daemon's FDs will be
 * added to fd_sets. To get only daemon FDs in fd_sets, call FD_ZERO
 * for each fd_set before calling this function.
 *
 * Passing custom FD_SETSIZE as @a fd_setsize allow usage of
 * larger/smaller than platform's default fd_sets.
 *
 * This function should only be called in when MHD is configured to
 * use external select with 'select()' or with 'epoll'.  In the latter
 * case, it will only add the single 'epoll' file descriptor used by
 * MHD to the sets.  It's necessary to use #MHD_get_timeout() in
 * combination with this function.
 *
 * This function must be called only for daemon started
 * without #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value); can be NULL
 * @param fd_setsize value of FD_SETSIZE
 * @return #MHD_SC_OK on success, otherwise error code
 * @ingroup event
 */
enum MHD_StatusCode
MHD_daemon_get_fdset2 (struct MHD_Daemon *daemon,
		       fd_set *read_fd_set,
		       fd_set *write_fd_set,
		       fd_set *except_fd_set,
		       MHD_socket *max_fd,
		       unsigned int fd_setsize)
{
  if ( (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode) ||
       (MHD_ELS_POLL == daemon->event_loop_syscall) )
    return MHD_SC_CONFIGURATION_MISMATCH_FOR_GET_FDSET;

#ifdef EPOLL_SUPPORT
  if (MHD_ELS_EPOLL == daemon->event_loop_syscall)
    {
      if (daemon->shutdown)
        return MHD_SC_DAEMON_ALREADY_SHUTDOWN;

      /* we're in epoll mode, use the epoll FD as a stand-in for
         the entire event set */

      return MHD_add_to_fd_set_ (daemon->epoll_fd,
                                 read_fd_set,
                                 max_fd,
                                 fd_setsize)
	? MHD_SC_OK
	: MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
    }
#endif

  return internal_get_fdset2 (daemon,
			      read_fd_set,
                              write_fd_set,
			      except_fd_set,
                              max_fd,
			      fd_setsize);
}


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Update the @a urh based on the ready FDs in
 * the @a rs, @a ws, and @a es.
 *
 * @param urh upgrade handle to update
 * @param rs read result from select()
 * @param ws write result from select()
 * @param es except result from select()
 */
static void
urh_from_fdset (struct MHD_UpgradeResponseHandle *urh,
                const fd_set *rs,
                const fd_set *ws,
                const fd_set *es)
{
  const MHD_socket conn_sckt = urh->connection->socket_fd;
  const MHD_socket mhd_sckt = urh->mhd.socket;

  /* Reset read/write ready, preserve error state. */
  urh->app.celi &= (~MHD_EPOLL_STATE_READ_READY & ~MHD_EPOLL_STATE_WRITE_READY);
  urh->mhd.celi &= (~MHD_EPOLL_STATE_READ_READY & ~MHD_EPOLL_STATE_WRITE_READY);

  if (MHD_INVALID_SOCKET != conn_sckt)
    {
      if (FD_ISSET (conn_sckt, rs))
        urh->app.celi |= MHD_EPOLL_STATE_READ_READY;
      if (FD_ISSET (conn_sckt, ws))
        urh->app.celi |= MHD_EPOLL_STATE_WRITE_READY;
      if (FD_ISSET (conn_sckt, es))
        urh->app.celi |= MHD_EPOLL_STATE_ERROR;
    }
  if ((MHD_INVALID_SOCKET != mhd_sckt))
    {
      if (FD_ISSET (mhd_sckt, rs))
        urh->mhd.celi |= MHD_EPOLL_STATE_READ_READY;
      if (FD_ISSET (mhd_sckt, ws))
        urh->mhd.celi |= MHD_EPOLL_STATE_WRITE_READY;
      if (FD_ISSET (mhd_sckt, es))
        urh->mhd.celi |= MHD_EPOLL_STATE_ERROR;
    }
}
#endif


/**
 * Internal version of #MHD_run_from_select().
 *
 * @param daemon daemon to run select loop for
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
static enum MHD_StatusCode
internal_run_from_select (struct MHD_Daemon *daemon,
                          const fd_set *read_fd_set,
                          const fd_set *write_fd_set,
                          const fd_set *except_fd_set)
{
  MHD_socket ds;
  struct MHD_Connection *pos;
  struct MHD_Connection *prev;
#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  struct MHD_UpgradeResponseHandle *urh;
  struct MHD_UpgradeResponseHandle *urhn;
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
  /* Reset. New value will be set when connections are processed. */
  /* Note: no-op for thread-per-connection as it is always false in that mode. */
  daemon->data_already_pending = false;

  /* Clear ITC to avoid spinning select */
  /* Do it before any other processing so new signals
     will trigger select again and will be processed */
  if ( (MHD_ITC_IS_VALID_(daemon->itc)) &&
       (FD_ISSET (MHD_itc_r_fd_ (daemon->itc),
                  read_fd_set)) )
    MHD_itc_clear_ (daemon->itc);

  /* select connection thread handling type */
  if ( (MHD_INVALID_SOCKET != (ds = daemon->listen_socket)) &&
       (! daemon->was_quiesced) &&
       (FD_ISSET (ds,
                  read_fd_set)) )
    (void) MHD_accept_connection_ (daemon);

  if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
    {
      /* do not have a thread per connection, process all connections now */
      prev = daemon->connections_tail;
      while (NULL != (pos = prev))
        {
	  prev = pos->prev;
          ds = pos->socket_fd;
          if (MHD_INVALID_SOCKET == ds)
	    continue;
          MHD_connection_call_handlers_ (pos,
					 FD_ISSET (ds,
						   read_fd_set),
					 FD_ISSET (ds,
						   write_fd_set),
					 FD_ISSET (ds,
						   except_fd_set));
        }
    }

#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
  /* handle upgraded HTTPS connections */
  for (urh = daemon->urh_tail; NULL != urh; urh = urhn)
    {
      urhn = urh->prev;
      /* update urh state based on select() output */
      urh_from_fdset (urh,
                      read_fd_set,
                      write_fd_set,
                      except_fd_set);
      /* call generic forwarding function for passing data */
      MHD_upgrade_response_handle_process_ (urh);
      /* Finished forwarding? */
      if ( (0 == urh->in_buffer_size) &&
           (0 == urh->out_buffer_size) &&
           (0 == urh->in_buffer_used) &&
           (0 == urh->out_buffer_used) )
        {
          MHD_connection_finish_forward_ (urh->connection);
          urh->clean_ready = true;
          /* Resuming will move connection to cleanup list. */
          MHD_request_resume (&urh->connection->request);
        }
    }
#endif /* HTTPS_SUPPORT && UPGRADE_SUPPORT */
  MHD_connection_cleanup_ (daemon);
  return MHD_SC_OK;
}


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Process upgraded connection with a select loop.
 * We are in our own thread, only processing @a con
 *
 * @param con connection to process
 */
void
MHD_daemon_upgrade_connection_with_select_ (struct MHD_Connection *con)
{
  struct MHD_UpgradeResponseHandle *urh = con->request.urh;

  while ( (0 != urh->in_buffer_size) ||
	  (0 != urh->out_buffer_size) ||
	  (0 != urh->in_buffer_used) ||
	  (0 != urh->out_buffer_used) )
    {
      /* use select */
      fd_set rs;
      fd_set ws;
      fd_set es;
      MHD_socket max_fd;
      int num_ready;
      bool result;

      FD_ZERO (&rs);
      FD_ZERO (&ws);
      FD_ZERO (&es);
      max_fd = MHD_INVALID_SOCKET;
      result = urh_to_fdset (urh,
			     &rs,
			     &ws,
			     &es,
			     &max_fd,
			     FD_SETSIZE);
      if (! result)
	{
#ifdef HAVE_MESSAGES
	  MHD_DLOG (con->daemon,
		    MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE,
		    _("Error preparing select\n"));
#endif
	  break;
	}
      /* FIXME: does this check really needed? */
      if (MHD_INVALID_SOCKET != max_fd)
	{
	  struct timeval* tvp;
	  struct timeval tv;
	  if ( (con->tls_read_ready) &&
	       (urh->in_buffer_used < urh->in_buffer_size))
	    { /* No need to wait if incoming data is already pending in TLS buffers. */
	      tv.tv_sec = 0;
	      tv.tv_usec = 0;
	      tvp = &tv;
	    }
	  else
	    tvp = NULL;
	  num_ready = MHD_SYS_select_ (max_fd + 1,
				       &rs,
				       &ws,
				       &es,
				       tvp);
	}
      else
	num_ready = 0;
      if (num_ready < 0)
	{
	  const int err = MHD_socket_get_error_();

	  if (MHD_SCKT_ERR_IS_EINTR_(err))
	    continue;
#ifdef HAVE_MESSAGES
	  MHD_DLOG (con->daemon,
		    MHD_SC_UNEXPECTED_SELECT_ERROR,
		    _("Error during select (%d): `%s'\n"),
		    err,
		    MHD_socket_strerr_ (err));
#endif
	  break;
	}
      urh_from_fdset (urh,
		      &rs,
		      &ws,
		      &es);
      MHD_upgrade_response_handle_process_ (urh);
    }
}
#endif


/**
 * Run webserver operations. This method should be called by clients
 * in combination with #MHD_get_fdset and #MHD_get_timeout() if the
 * client-controlled select method is used.
 *
 * You can use this function instead of #MHD_run if you called
 * `select()` on the result from #MHD_get_fdset.  File descriptors in
 * the sets that are not controlled by MHD will be ignored.  Calling
 * this function instead of #MHD_run is more efficient as MHD will not
 * have to call `select()` again to determine which operations are
 * ready.
 *
 * This function cannot be used with daemon started with
 * #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to run select loop for
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
enum MHD_StatusCode
MHD_daemon_run_from_select (struct MHD_Daemon *daemon,
			    const fd_set *read_fd_set,


			    const fd_set *write_fd_set,
			    const fd_set *except_fd_set)
{
  if ( (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode) ||
       (MHD_ELS_POLL == daemon->event_loop_syscall) )
    return MHD_SC_CONFIGURATION_MISSMATCH_FOR_RUN_SELECT;
  if (MHD_ELS_EPOLL == daemon->event_loop_syscall)
    {
#ifdef EPOLL_SUPPORT
      enum MHD_StatusCode sc;

      sc = MHD_daemon_epoll_ (daemon,
			      MHD_NO);
      MHD_connection_cleanup_ (daemon);
      return sc;
#else  /* ! EPOLL_SUPPORT */
      return MHD_NO;
#endif /* ! EPOLL_SUPPORT */
    }

  /* Resuming external connections when using an extern mainloop  */
  if (! daemon->disallow_suspend_resume)
    (void) MHD_resume_suspended_connections_ (daemon);

  return internal_run_from_select (daemon,
				   read_fd_set,
                                   write_fd_set,
				   except_fd_set);
}


/**
 * Main internal select() call.  Will compute select sets, call
 * select() and then #internal_run_from_select() with the result.
 *
 * @param daemon daemon to run select() loop for
 * @param may_block #MHD_YES if blocking, #MHD_NO if non-blocking
 * @return #MHD_SC_OK on success
 */
enum MHD_StatusCode
MHD_daemon_select_ (struct MHD_Daemon *daemon,
		    int may_block)
{
  int num_ready;
  fd_set rs;
  fd_set ws;
  fd_set es;
  MHD_socket maxsock;
  struct timeval timeout;
  struct timeval *tv;
  MHD_UNSIGNED_LONG_LONG ltimeout;
  MHD_socket ls;
  enum MHD_StatusCode sc;
  enum MHD_StatusCode sc2;

  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
  FD_ZERO (&rs);
  FD_ZERO (&ws);
  FD_ZERO (&es);
  maxsock = MHD_INVALID_SOCKET;
  sc = MHD_SC_OK;
  if ( (! daemon->disallow_suspend_resume) &&
       (MHD_resume_suspended_connections_ (daemon)) &&
       (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode) )
    may_block = MHD_NO;

  if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
    {

      /* single-threaded, go over everything */
      if (MHD_SC_OK !=
	  (sc = internal_get_fdset2 (daemon,
				     &rs,
				     &ws,
				     &es,
				     &maxsock,
				     FD_SETSIZE)))
        {
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    sc,
		    _("Could not obtain daemon fdsets"));
#endif
        }
    }
  else
    {
      /* accept only, have one thread per connection */
      if ( (MHD_INVALID_SOCKET != (ls = daemon->listen_socket)) &&
           (! daemon->was_quiesced) &&
           (! MHD_add_to_fd_set_ (ls,
                                  &rs,
                                  &maxsock,
                                  FD_SETSIZE)) )
        {
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE,
                    _("Could not add listen socket to fdset"));
#endif
          return MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
        }
    }
  if ( (MHD_ITC_IS_VALID_(daemon->itc)) &&
       (! MHD_add_to_fd_set_ (MHD_itc_r_fd_ (daemon->itc),
                              &rs,
                              &maxsock,
                              FD_SETSIZE)) )
    {
#if defined(MHD_WINSOCK_SOCKETS)
      /* fdset limit reached, new connections
         cannot be handled. Remove listen socket FD
         from fdset and retry to add ITC FD. */
      if ( (MHD_INVALID_SOCKET != (ls = daemon->listen_socket)) &&
           (! daemon->was_quiesced) )
        {
          FD_CLR (ls,
                  &rs);
          if (! MHD_add_to_fd_set_ (MHD_itc_r_fd_(daemon->itc),
                                    &rs,
                                    &maxsock,
                                    FD_SETSIZE))
            {
#endif /* MHD_WINSOCK_SOCKETS */
              sc = MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE;
#ifdef HAVE_MESSAGES
              MHD_DLOG (daemon,
			sc,
                        _("Could not add control inter-thread communication channel FD to fdset"));
#endif
#if defined(MHD_WINSOCK_SOCKETS)
            }
        }
#endif /* MHD_WINSOCK_SOCKETS */
    }
  /* Stop listening if we are at the configured connection limit */
  /* If we're at the connection limit, no point in really
     accepting new connections; however, make sure we do not miss
     the shutdown OR the termination of an existing connection; so
     only do this optimization if we have a signaling ITC in
     place. */
  if ( (MHD_INVALID_SOCKET != (ls = daemon->listen_socket)) &&
       (MHD_ITC_IS_VALID_(daemon->itc)) &&
       ( (daemon->connections == daemon->global_connection_limit) ||
         (daemon->at_limit) ) )
    {
      FD_CLR (ls,
              &rs);
    }
  tv = NULL;
  if (MHD_SC_OK != sc)
    may_block = MHD_NO;
  if (MHD_NO == may_block)
    {
      timeout.tv_usec = 0;
      timeout.tv_sec = 0;
      tv = &timeout;
    }
  else if ( (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode) &&
	    (MHD_SC_OK ==
	     MHD_daemon_get_timeout (daemon,
				     &ltimeout)) )
    {
      /* ltimeout is in ms */
      timeout.tv_usec = (ltimeout % 1000) * 1000;
      if (ltimeout / 1000 > TIMEVAL_TV_SEC_MAX)
        timeout.tv_sec = TIMEVAL_TV_SEC_MAX;
      else
        timeout.tv_sec = (_MHD_TIMEVAL_TV_SEC_TYPE)(ltimeout / 1000);
      tv = &timeout;
    }
  num_ready = MHD_SYS_select_ (maxsock + 1,
                               &rs,
                               &ws,
                               &es,
                               tv);
  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;
  if (num_ready < 0)
    {
      const int err = MHD_socket_get_error_ ();

      if (MHD_SCKT_ERR_IS_EINTR_(err))
        return sc;
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_UNEXPECTED_SELECT_ERROR,
                _("select failed: %s\n"),
                MHD_socket_strerr_ (err));
#endif
      return MHD_SC_UNEXPECTED_SELECT_ERROR;
    }
  if (MHD_SC_OK !=
      (sc2 = internal_run_from_select (daemon,
				       &rs,
				       &ws,
				       &es)))
    return sc2;
  return sc;
}

/* end of daemon_select.c */
