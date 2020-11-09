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
 * @file lib/connection_call_handlers.c
 * @brief call the connection's handlers based on the event trigger
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_call_handlers.h"
#include "connection_update_last_activity.h"
#include "connection_close.h"


#ifdef MHD_LINUX_SOLARIS_SENDFILE
#include <sys/sendfile.h>
#endif /* MHD_LINUX_SOLARIS_SENDFILE */
#if defined(HAVE_FREEBSD_SENDFILE) || defined(HAVE_DARWIN_SENDFILE)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#endif /* HAVE_FREEBSD_SENDFILE || HAVE_DARWIN_SENDFILE */


/**
 * sendfile() chuck size
 */
#define MHD_SENFILE_CHUNK_         (0x20000)

/**
 * sendfile() chuck size for thread-per-connection
 */
#define MHD_SENFILE_CHUNK_THR_P_C_ (0x200000)


/**
 * Response text used when the request (http header) is too big to
 * be processed.
 *
 * Intentionally empty here to keep our memory footprint
 * minimal.
 */
#ifdef HAVE_MESSAGES
#define REQUEST_TOO_BIG "<html><head><title>Request too big</title></head><body>Your HTTP header was too big for the memory constraints of this webserver.</body></html>"
#else
#define REQUEST_TOO_BIG ""
#endif

/**
 * Response text used when the request (http header) does not
 * contain a "Host:" header and still claims to be HTTP 1.1.
 *
 * Intentionally empty here to keep our memory footprint
 * minimal.
 */
#ifdef HAVE_MESSAGES
#define REQUEST_LACKS_HOST "<html><head><title>&quot;Host:&quot; header required</title></head><body>In HTTP 1.1, requests must include a &quot;Host:&quot; header, and your HTTP 1.1 request lacked such a header.</body></html>"
#else
#define REQUEST_LACKS_HOST ""
#endif

/**
 * Response text used when the request (http header) is
 * malformed.
 *
 * Intentionally empty here to keep our memory footprint
 * minimal.
 */
#ifdef HAVE_MESSAGES
#define REQUEST_MALFORMED "<html><head><title>Request malformed</title></head><body>Your HTTP request was syntactically incorrect.</body></html>"
#else
#define REQUEST_MALFORMED ""
#endif

/**
 * Response text used when there is an internal server error.
 *
 * Intentionally empty here to keep our memory footprint
 * minimal.
 */
#ifdef HAVE_MESSAGES
#define INTERNAL_ERROR "<html><head><title>Internal server error</title></head><body>Please ask the developer of this Web server to carefully read the GNU libmicrohttpd documentation about connection management and blocking.</body></html>"
#else
#define INTERNAL_ERROR ""
#endif


#ifdef HAVE_FREEBSD_SENDFILE
#ifdef SF_FLAGS
/**
 * FreeBSD sendfile() flags
 */
static int freebsd_sendfile_flags_;

/**
 * FreeBSD sendfile() flags for thread-per-connection
 */
static int freebsd_sendfile_flags_thd_p_c_;
#endif /* SF_FLAGS */


/**
 * Initialises static variables.
 *
 * FIXME: make sure its actually called!
 */
void
MHD_conn_init_static_ (void)
{
/* FreeBSD 11 and later allow to specify read-ahead size
 * and handles SF_NODISKIO differently.
 * SF_FLAGS defined only on FreeBSD 11 and later. */
#ifdef SF_FLAGS
  long sys_page_size = sysconf (_SC_PAGESIZE);
  if (0 > sys_page_size)
    { /* Failed to get page size. */
      freebsd_sendfile_flags_ = SF_NODISKIO;
      freebsd_sendfile_flags_thd_p_c_ = SF_NODISKIO;
    }
  else
    {
      freebsd_sendfile_flags_ =
          SF_FLAGS((uint16_t)(MHD_SENFILE_CHUNK_ / sys_page_size), SF_NODISKIO);
      freebsd_sendfile_flags_thd_p_c_ =
          SF_FLAGS((uint16_t)(MHD_SENFILE_CHUNK_THR_P_C_ / sys_page_size), SF_NODISKIO);
    }
#endif /* SF_FLAGS */
}
#endif /* HAVE_FREEBSD_SENDFILE */



/**
 * Message to transmit when http 1.1 request is received
 */
#define HTTP_100_CONTINUE "HTTP/1.1 100 Continue\r\n\r\n"


/**
 * A serious error occured, close the
 * connection (and notify the application).
 *
 * @param connection connection to close with error
 * @param sc the reason for closing the connection
 * @param emsg error message (can be NULL)
 */
static void
connection_close_error (struct MHD_Connection *connection,
			enum MHD_StatusCode sc,
			const char *emsg)
{
#ifdef HAVE_MESSAGES
  if (NULL != emsg)
    MHD_DLOG (connection->daemon,
	      sc,
              emsg);
#else  /* ! HAVE_MESSAGES */
  (void) emsg; /* Mute compiler warning. */
  (void) sc;
#endif /* ! HAVE_MESSAGES */
  MHD_connection_close_ (connection,
                         MHD_REQUEST_TERMINATED_WITH_ERROR);
}


/**
 * Macro to only include error message in call to
 * #connection_close_error() if we have HAVE_MESSAGES.
 */
#ifdef HAVE_MESSAGES
#define CONNECTION_CLOSE_ERROR(c, sc, emsg) connection_close_error (c, sc, emsg)
#else
#define CONNECTION_CLOSE_ERROR(c, sc, emsg) connection_close_error (c, sc, NULL)
#endif


/**
 * Try growing the read buffer.  We initially claim half the available
 * buffer space for the read buffer (the other half being left for
 * management data structures; the write buffer can in the end take
 * virtually everything as the read buffer can be reduced to the
 * minimum necessary at that point.
 *
 * @param request the request for which to grow the buffer
 * @return true on success, false on failure
 */
static bool
try_grow_read_buffer (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  void *buf;
  size_t new_size;

  if (0 == request->read_buffer_size)
    new_size = daemon->connection_memory_limit_b / 2;
  else
    new_size = request->read_buffer_size +
      daemon->connection_memory_increment_b;
  buf = MHD_pool_reallocate (request->connection->pool,
                             request->read_buffer,
                             request->read_buffer_size,
                             new_size);
  if (NULL == buf)
    return false;
  /* we can actually grow the buffer, do it! */
  request->read_buffer = buf;
  request->read_buffer_size = new_size;
  return true;
}


/**
 * This function handles a particular request when it has been
 * determined that there is data to be read off a socket.
 *
 * @param request request to handle
 */
static void
MHD_request_handle_read_ (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Connection *connection = request->connection;
  ssize_t bytes_read;

  if ( (MHD_REQUEST_CLOSED == request->state) ||
       (connection->suspended) )
    return;
#ifdef HTTPS_SUPPORT
  {
    struct MHD_TLS_Plugin *tls;

    if ( (NULL != (tls = daemon->tls_api)) &&
	 (! tls->handshake (tls->cls,
			    connection->tls_cs)) )
      return;
  }
#endif /* HTTPS_SUPPORT */

  /* make sure "read" has a reasonable number of bytes
     in buffer to use per system call (if possible) */
  if (request->read_buffer_offset +
      daemon->connection_memory_increment_b >
      request->read_buffer_size)
    try_grow_read_buffer (request);

  if (request->read_buffer_size == request->read_buffer_offset)
    return; /* No space for receiving data. */
  bytes_read = connection->recv_cls (connection,
                                     &request->read_buffer
                                     [request->read_buffer_offset],
                                     request->read_buffer_size -
                                     request->read_buffer_offset);
  if (bytes_read < 0)
    {
      if (MHD_ERR_AGAIN_ == bytes_read)
          return; /* No new data to process. */
      if (MHD_ERR_CONNRESET_ == bytes_read)
        {
	  CONNECTION_CLOSE_ERROR (connection,
				  (MHD_REQUEST_INIT == request->state)
				  ? MHD_SC_CONNECTION_CLOSED
				  : MHD_SC_CONNECTION_RESET_CLOSED,
				  (MHD_REQUEST_INIT == request->state)
				  ? NULL
				  : _("Socket disconnected while reading request.\n"));
           return;
        }
      CONNECTION_CLOSE_ERROR (connection,
			      (MHD_REQUEST_INIT == request->state)
			      ? MHD_SC_CONNECTION_CLOSED
			      : MHD_SC_CONNECTION_READ_FAIL_CLOSED,
                              (MHD_REQUEST_INIT == request->state)
			      ? NULL
                              : _("Connection socket is closed due to error when reading request.\n"));
      return;
    }

  if (0 == bytes_read)
    { /* Remote side closed connection. */
      connection->read_closed = true;
      MHD_connection_close_ (connection,
                             MHD_REQUEST_TERMINATED_CLIENT_ABORT);
      return;
    }
  request->read_buffer_offset += bytes_read;
  MHD_connection_update_last_activity_ (connection);
#if DEBUG_STATES
  MHD_DLOG (daemon,
	    MHD_SC_STATE_MACHINE_STATUS_REPORT,
            _("In function %s handling connection at state: %s\n"),
            __FUNCTION__,
            MHD_state_to_string (request->state));
#endif
  switch (request->state)
    {
    case MHD_REQUEST_INIT:
    case MHD_REQUEST_URL_RECEIVED:
    case MHD_REQUEST_HEADER_PART_RECEIVED:
    case MHD_REQUEST_HEADERS_RECEIVED:
    case MHD_REQUEST_HEADERS_PROCESSED:
    case MHD_REQUEST_CONTINUE_SENDING:
    case MHD_REQUEST_CONTINUE_SENT:
    case MHD_REQUEST_BODY_RECEIVED:
    case MHD_REQUEST_FOOTER_PART_RECEIVED:
      /* nothing to do but default action */
      if (connection->read_closed)
        {
          MHD_connection_close_ (connection,
                                 MHD_REQUEST_TERMINATED_READ_ERROR);
        }
      return;
    case MHD_REQUEST_CLOSED:
      return;
#ifdef UPGRADE_SUPPORT
    case MHD_REQUEST_UPGRADE:
      mhd_assert (0);
      return;
#endif /* UPGRADE_SUPPORT */
    default:
      /* shrink read buffer to how much is actually used */
      MHD_pool_reallocate (connection->pool,
                           request->read_buffer,
                           request->read_buffer_size + 1,
                           request->read_buffer_offset);
      break;
    }
  return;
}


#if defined(_MHD_HAVE_SENDFILE)
/**
 * Function for sending responses backed by file FD.
 *
 * @param connection the MHD connection structure
 * @return actual number of bytes sent
 */
static ssize_t
sendfile_adapter (struct MHD_Connection *connection)
{
  struct MHD_Daemon *daemon = connection->daemon;
  struct MHD_Request *request = &connection->request;
  struct MHD_Response *response = request->response;
  ssize_t ret;
  const int file_fd = response->fd;
  uint64_t left;
  uint64_t offsetu64;
#ifndef HAVE_SENDFILE64
  const uint64_t max_off_t = (uint64_t)OFF_T_MAX;
#else  /* HAVE_SENDFILE64 */
  const uint64_t max_off_t = (uint64_t)OFF64_T_MAX;
#endif /* HAVE_SENDFILE64 */
#ifdef MHD_LINUX_SOLARIS_SENDFILE
#ifndef HAVE_SENDFILE64
  off_t offset;
#else  /* HAVE_SENDFILE64 */
  off64_t offset;
#endif /* HAVE_SENDFILE64 */
#endif /* MHD_LINUX_SOLARIS_SENDFILE */
#ifdef HAVE_FREEBSD_SENDFILE
  off_t sent_bytes;
  int flags = 0;
#endif
#ifdef HAVE_DARWIN_SENDFILE
  off_t len;
#endif /* HAVE_DARWIN_SENDFILE */
  const bool used_thr_p_c = (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode);
  const size_t chunk_size = used_thr_p_c ? MHD_SENFILE_CHUNK_THR_P_C_ : MHD_SENFILE_CHUNK_;
  size_t send_size = 0;

  mhd_assert (MHD_resp_sender_sendfile == request->resp_sender);
  offsetu64 = request->response_write_position + response->fd_off;
  left = response->total_size - request->response_write_position;
  /* Do not allow system to stick sending on single fast connection:
   * use 128KiB chunks (2MiB for thread-per-connection). */
  send_size = (left > chunk_size) ? chunk_size : (size_t) left;
  if (max_off_t < offsetu64)
    { /* Retry to send with standard 'send()'. */
      request->resp_sender = MHD_resp_sender_std;
      return MHD_ERR_AGAIN_;
    }
#ifdef MHD_LINUX_SOLARIS_SENDFILE
#ifndef HAVE_SENDFILE64
  offset = (off_t) offsetu64;
  ret = sendfile (connection->socket_fd,
                  file_fd,
                  &offset,
                  send_size);
#else  /* HAVE_SENDFILE64 */
  offset = (off64_t) offsetu64;
  ret = sendfile64 (connection->socket_fd,
                    file_fd,
                    &offset,
                    send_size);
#endif /* HAVE_SENDFILE64 */
  if (0 > ret)
    {
      const int err = MHD_socket_get_error_();

      if (MHD_SCKT_ERR_IS_EAGAIN_(err))
        {
#ifdef EPOLL_SUPPORT
          /* EAGAIN --- no longer write-ready */
          connection->epoll_state &= ~MHD_EPOLL_STATE_WRITE_READY;
#endif /* EPOLL_SUPPORT */
          return MHD_ERR_AGAIN_;
        }
      if (MHD_SCKT_ERR_IS_EINTR_ (err))
        return MHD_ERR_AGAIN_;
#ifdef HAVE_LINUX_SENDFILE
      if (MHD_SCKT_ERR_IS_(err,
                           MHD_SCKT_EBADF_))
        return MHD_ERR_BADF_;
      /* sendfile() failed with EINVAL if mmap()-like operations are not
         supported for FD or other 'unusual' errors occurred, so we should try
         to fall back to 'SEND'; see also this thread for info on
         odd libc/Linux behavior with sendfile:
         http://lists.gnu.org/archive/html/libmicrohttpd/2011-02/msg00015.html */
      request->resp_sender = MHD_resp_sender_std;
      return MHD_ERR_AGAIN_;
#else  /* HAVE_SOLARIS_SENDFILE */
      if ( (EAFNOSUPPORT == err) ||
           (EINVAL == err) ||
           (EOPNOTSUPP == err) )
        { /* Retry with standard file reader. */
          request->resp_sender = MHD_resp_sender_std;
          return MHD_ERR_AGAIN_;
        }
      if ( (ENOTCONN == err) ||
           (EPIPE == err) )
        {
          return MHD_ERR_CONNRESET_;
        }
      return MHD_ERR_BADF_; /* Fail hard */
#endif /* HAVE_SOLARIS_SENDFILE */
    }
#ifdef EPOLL_SUPPORT
  else if (send_size > (size_t)ret)
        connection->epoll_state &= ~MHD_EPOLL_STATE_WRITE_READY;
#endif /* EPOLL_SUPPORT */
#elif defined(HAVE_FREEBSD_SENDFILE)
#ifdef SF_FLAGS
  flags = used_thr_p_c ?
      freebsd_sendfile_flags_thd_p_c_ : freebsd_sendfile_flags_;
#endif /* SF_FLAGS */
  if (0 != sendfile (file_fd,
                     connection->socket_fd,
                     (off_t) offsetu64,
                     send_size,
                     NULL,
                     &sent_bytes,
                     flags))
    {
      const int err = MHD_socket_get_error_();
      if (MHD_SCKT_ERR_IS_EAGAIN_(err) ||
          MHD_SCKT_ERR_IS_EINTR_(err) ||
          EBUSY == err)
        {
          mhd_assert (SSIZE_MAX >= sent_bytes);
          if (0 != sent_bytes)
            return (ssize_t)sent_bytes;

          return MHD_ERR_AGAIN_;
        }
      /* Some unrecoverable error. Possibly file FD is not suitable
       * for sendfile(). Retry with standard send(). */
      request->resp_sender = MHD_resp_sender_std;
      return MHD_ERR_AGAIN_;
    }
  mhd_assert (0 < sent_bytes);
  mhd_assert (SSIZE_MAX >= sent_bytes);
  ret = (ssize_t)sent_bytes;
#elif defined(HAVE_DARWIN_SENDFILE)
  len = (off_t) send_size; /* chunk always fit */
  if (0 != sendfile (file_fd,
                     connection->socket_fd,
                     (off_t) offsetu64,
                     &len,
                     NULL,
                     0))
    {
      const int err = MHD_socket_get_error_();
      if (MHD_SCKT_ERR_IS_EAGAIN_(err) ||
          MHD_SCKT_ERR_IS_EINTR_(err))
        {
          mhd_assert (0 <= len);
          mhd_assert (SSIZE_MAX >= len);
          mhd_assert (send_size >= (size_t)len);
          if (0 != len)
            return (ssize_t)len;

          return MHD_ERR_AGAIN_;
        }
      if (ENOTCONN == err ||
          EPIPE == err)
        return MHD_ERR_CONNRESET_;
      if (ENOTSUP == err ||
          EOPNOTSUPP == err)
        { /* This file FD is not suitable for sendfile().
           * Retry with standard send(). */
          request->resp_sender = MHD_resp_sender_std;
          return MHD_ERR_AGAIN_;
        }
      return MHD_ERR_BADF_; /* Return hard error. */
    }
  mhd_assert (0 <= len);
  mhd_assert (SSIZE_MAX >= len);
  mhd_assert (send_size >= (size_t)len);
  ret = (ssize_t)len;
#endif /* HAVE_FREEBSD_SENDFILE */
  return ret;
}
#endif /* _MHD_HAVE_SENDFILE */


/**
 * Check if we are done sending the write-buffer.  If so, transition
 * into "next_state".
 *
 * @param connection connection to check write status for
 * @param next_state the next state to transition to
 * @return false if we are not done, true if we are
 */
static bool
check_write_done (struct MHD_Request *request,
                  enum MHD_REQUEST_STATE next_state)
{
  if (request->write_buffer_append_offset !=
      request->write_buffer_send_offset)
    return false;
  request->write_buffer_append_offset = 0;
  request->write_buffer_send_offset = 0;
  request->state = next_state;
  MHD_pool_reallocate (request->connection->pool,
		       request->write_buffer,
                       request->write_buffer_size,
                       0);
  request->write_buffer = NULL;
  request->write_buffer_size = 0;
  return true;
}


/**
 * Prepare the response buffer of this request for sending.  Assumes
 * that the response mutex is already held.  If the transmission is
 * complete, this function may close the socket (and return false).
 *
 * @param request the request handle
 * @return false if readying the response failed (the
 *  lock on the response will have been released already
 *  in this case).
 */
static bool
try_ready_normal_body (struct MHD_Request *request)
{
  struct MHD_Response *response = request->response;
  struct MHD_Connection *connection = request->connection;
  ssize_t ret;

  if (NULL == response->crc)
    return true;
  if ( (0 == response->total_size) ||
       (request->response_write_position == response->total_size) )
    return true; /* 0-byte response is always ready */
  if ( (response->data_start <=
	request->response_write_position) &&
       (response->data_size + response->data_start >
	request->response_write_position) )
    return true; /* response already ready */
#if defined(_MHD_HAVE_SENDFILE)
  if (MHD_resp_sender_sendfile == request->resp_sender)
    {
      /* will use sendfile, no need to bother response crc */
      return true;
    }
#endif /* _MHD_HAVE_SENDFILE */

  ret = response->crc (response->crc_cls,
                       request->response_write_position,
                       response->data,
                       (size_t) MHD_MIN ((uint64_t) response->data_buffer_size,
                                         response->total_size -
                                         request->response_write_position));
  if ( (((ssize_t) MHD_CONTENT_READER_END_OF_STREAM) == ret) ||
       (((ssize_t) MHD_CONTENT_READER_END_WITH_ERROR) == ret) )
    {
      /* either error or http 1.0 transfer, close socket! */
      response->total_size = request->response_write_position;
      MHD_mutex_unlock_chk_ (&response->mutex);
      if ( ((ssize_t)MHD_CONTENT_READER_END_OF_STREAM) == ret)
	MHD_connection_close_ (connection,
                               MHD_REQUEST_TERMINATED_COMPLETED_OK);
      else
	CONNECTION_CLOSE_ERROR (connection,
				MHD_SC_APPLICATION_DATA_GENERATION_FAILURE_CLOSED,
				_("Closing connection (application reported error generating data)\n"));
      return false;
    }
  response->data_start = request->response_write_position;
  response->data_size = ret;
  if (0 == ret)
    {
      request->state = MHD_REQUEST_NORMAL_BODY_UNREADY;
      MHD_mutex_unlock_chk_ (&response->mutex);
      return false;
    }
  return true;
}


/**
 * Prepare the response buffer of this request for sending.  Assumes
 * that the response mutex is already held.  If the transmission is
 * complete, this function may close the socket (and return false).
 *
 * @param connection the connection
 * @return false if readying the response failed
 */
static bool
try_ready_chunked_body (struct MHD_Request *request)
{
  struct MHD_Connection *connection = request->connection;
  struct MHD_Response *response = request->response;
  struct MHD_Daemon *daemon = request->daemon;
  ssize_t ret;
  char *buf;
  size_t size;
  char cbuf[10];                /* 10: max strlen of "%x\r\n" */
  int cblen;

  if (NULL == response->crc)
    return true;
  if (0 == request->write_buffer_size)
    {
      size = MHD_MIN (daemon->connection_memory_limit_b,
                      2 * (0xFFFFFF + sizeof(cbuf) + 2));
      do
        {
          size /= 2;
          if (size < 128)
            {
              MHD_mutex_unlock_chk_ (&response->mutex);
              /* not enough memory */
              CONNECTION_CLOSE_ERROR (connection,
				      MHD_SC_CONNECTION_POOL_MALLOC_FAILURE,
				      _("Closing connection (out of memory)\n"));
              return false;
            }
          buf = MHD_pool_allocate (connection->pool,
                                   size,
                                   MHD_NO);
        }
      while (NULL == buf);
      request->write_buffer_size = size;
      request->write_buffer = buf;
    }

  if (0 == response->total_size)
    ret = 0; /* response must be empty, don't bother calling crc */
  else if ( (response->data_start <=
	request->response_write_position) &&
       (response->data_start + response->data_size >
	request->response_write_position) )
    {
      /* difference between response_write_position and data_start is less
         than data_size which is size_t type, no need to check for overflow */
      const size_t data_write_offset
        = (size_t)(request->response_write_position - response->data_start);
      /* buffer already ready, use what is there for the chunk */
      ret = response->data_size - data_write_offset;
      if ( ((size_t) ret) > request->write_buffer_size - sizeof (cbuf) - 2 )
	ret = request->write_buffer_size - sizeof (cbuf) - 2;
      memcpy (&request->write_buffer[sizeof (cbuf)],
              &response->data[data_write_offset],
              ret);
    }
  else
    {
      /* buffer not in range, try to fill it */
      ret = response->crc (response->crc_cls,
                           request->response_write_position,
                           &request->write_buffer[sizeof (cbuf)],
                           request->write_buffer_size - sizeof (cbuf) - 2);
    }
  if ( ((ssize_t) MHD_CONTENT_READER_END_WITH_ERROR) == ret)
    {
      /* error, close socket! */
      response->total_size = request->response_write_position;
      MHD_mutex_unlock_chk_ (&response->mutex);
      CONNECTION_CLOSE_ERROR (connection,
			      MHD_SC_APPLICATION_DATA_GENERATION_FAILURE_CLOSED,
			      _("Closing connection (application error generating response)\n"));
      return false;
    }
  if ( (((ssize_t) MHD_CONTENT_READER_END_OF_STREAM) == ret) ||
       (0 == response->total_size) )
    {
      /* end of message, signal other side! */
      memcpy (request->write_buffer,
              "0\r\n",
              3);
      request->write_buffer_append_offset = 3;
      request->write_buffer_send_offset = 0;
      response->total_size = request->response_write_position;
      return true;
    }
  if (0 == ret)
    {
      request->state = MHD_REQUEST_CHUNKED_BODY_UNREADY;
      MHD_mutex_unlock_chk_ (&response->mutex);
      return false;
    }
  if (ret > 0xFFFFFF)
    ret = 0xFFFFFF;
  cblen = MHD_snprintf_(cbuf,
                        sizeof (cbuf),
                        "%X\r\n",
                        (unsigned int) ret);
  mhd_assert(cblen > 0);
  mhd_assert((size_t)cblen < sizeof(cbuf));
  memcpy (&request->write_buffer[sizeof (cbuf) - cblen],
          cbuf,
          cblen);
  memcpy (&request->write_buffer[sizeof (cbuf) + ret],
          "\r\n",
          2);
  request->response_write_position += ret;
  request->write_buffer_send_offset = sizeof (cbuf) - cblen;
  request->write_buffer_append_offset = sizeof (cbuf) + ret + 2;
  return true;
}


/**
 * This function was created to handle writes to sockets when it has
 * been determined that the socket can be written to.
 *
 * @param request the request to handle
 */
static void
MHD_request_handle_write_ (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Connection *connection = request->connection;
  struct MHD_Response *response;
  ssize_t ret;

  if (connection->suspended)
    return;
#ifdef HTTPS_SUPPORT
  {
    struct MHD_TLS_Plugin *tls;

    if ( (NULL != (tls = daemon->tls_api)) &&
	 (! tls->handshake (tls->cls,
			    connection->tls_cs)) )
      return;
  }
#endif /* HTTPS_SUPPORT */

#if DEBUG_STATES
  MHD_DLOG (daemon,
	    MHD_SC_STATE_MACHINE_STATUS_REPORT,
            _("In function %s handling connection at state: %s\n"),
            __FUNCTION__,
            MHD_state_to_string (request->state));
#endif
  switch (request->state)
    {
    case MHD_REQUEST_INIT:
    case MHD_REQUEST_URL_RECEIVED:
    case MHD_REQUEST_HEADER_PART_RECEIVED:
    case MHD_REQUEST_HEADERS_RECEIVED:
      mhd_assert (0);
      return;
    case MHD_REQUEST_HEADERS_PROCESSED:
      return;
    case MHD_REQUEST_CONTINUE_SENDING:
      ret = connection->send_cls (connection,
                                  &HTTP_100_CONTINUE
                                  [request->continue_message_write_offset],
                                  MHD_STATICSTR_LEN_ (HTTP_100_CONTINUE) -
                                  request->continue_message_write_offset);
      if (ret < 0)
        {
          if (MHD_ERR_AGAIN_ == ret)
            return;
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                    _("Failed to send data in request for %s.\n"),
                    request->url);
#endif
          CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                                  NULL);
          return;
        }
      request->continue_message_write_offset += ret;
      MHD_connection_update_last_activity_ (connection);
      return;
    case MHD_REQUEST_CONTINUE_SENT:
    case MHD_REQUEST_BODY_RECEIVED:
    case MHD_REQUEST_FOOTER_PART_RECEIVED:
    case MHD_REQUEST_FOOTERS_RECEIVED:
      mhd_assert (0);
      return;
    case MHD_REQUEST_HEADERS_SENDING:
      ret = connection->send_cls (connection,
                                  &request->write_buffer
                                  [request->write_buffer_send_offset],
                                  request->write_buffer_append_offset -
                                    request->write_buffer_send_offset);
      if (ret < 0)
        {
          if (MHD_ERR_AGAIN_ == ret)
            return;
          CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                                  _("Connection was closed while sending response headers.\n"));
          return;
        }
      request->write_buffer_send_offset += ret;
      MHD_connection_update_last_activity_ (connection);
      if (MHD_REQUEST_HEADERS_SENDING != request->state)
        return;
      check_write_done (request,
                        MHD_REQUEST_HEADERS_SENT);
      return;
    case MHD_REQUEST_HEADERS_SENT:
      return;
    case MHD_REQUEST_NORMAL_BODY_READY:
      response = request->response;
      if (request->response_write_position <
          request->response->total_size)
        {
          uint64_t data_write_offset;

          if (NULL != response->crc)
            MHD_mutex_lock_chk_ (&response->mutex);
          if (! try_ready_normal_body (request))
            {
              /* mutex was already unlocked by try_ready_normal_body */
              return;
            }
#if defined(_MHD_HAVE_SENDFILE)
          if (MHD_resp_sender_sendfile == request->resp_sender)
            {
              ret = sendfile_adapter (connection);
            }
          else
#else  /* ! _MHD_HAVE_SENDFILE */
          if (1)
#endif /* ! _MHD_HAVE_SENDFILE */
            {
              data_write_offset = request->response_write_position
                                  - response->data_start;
              if (data_write_offset > (uint64_t)SIZE_MAX)
                MHD_PANIC (_("Data offset exceeds limit"));
              ret = connection->send_cls (connection,
                                          &response->data
                                          [(size_t)data_write_offset],
                                          response->data_size -
                                          (size_t)data_write_offset);
#if DEBUG_SEND_DATA
              if (ret > 0)
                fprintf (stderr,
                         _("Sent %d-byte DATA response: `%.*s'\n"),
                         (int) ret,
                         (int) ret,
                         &response->data[request->response_write_position -
                                         response->data_start]);
#endif
            }
          if (NULL != response->crc)
            MHD_mutex_unlock_chk_ (&response->mutex);
          if (ret < 0)
            {
              if (MHD_ERR_AGAIN_ == ret)
                return;
#ifdef HAVE_MESSAGES
              MHD_DLOG (daemon,
			MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                        _("Failed to send data in request for `%s'.\n"),
                        request->url);
#endif
              CONNECTION_CLOSE_ERROR (connection,
				      MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                                      NULL);
              return;
            }
          request->response_write_position += ret;
          MHD_connection_update_last_activity_ (connection);
        }
      if (request->response_write_position ==
          request->response->total_size)
        request->state = MHD_REQUEST_FOOTERS_SENT; /* have no footers */
      return;
    case MHD_REQUEST_NORMAL_BODY_UNREADY:
      mhd_assert (0);
      return;
    case MHD_REQUEST_CHUNKED_BODY_READY:
      ret = connection->send_cls (connection,
                                  &request->write_buffer
                                  [request->write_buffer_send_offset],
                                  request->write_buffer_append_offset -
                                    request->write_buffer_send_offset);
      if (ret < 0)
        {
          if (MHD_ERR_AGAIN_ == ret)
            return;
          CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                                  _("Connection was closed while sending response body.\n"));
          return;
        }
      request->write_buffer_send_offset += ret;
      MHD_connection_update_last_activity_ (connection);
      if (MHD_REQUEST_CHUNKED_BODY_READY != request->state)
        return;
      check_write_done (request,
                        (request->response->total_size ==
                         request->response_write_position) ?
                        MHD_REQUEST_BODY_SENT :
                        MHD_REQUEST_CHUNKED_BODY_UNREADY);
      return;
    case MHD_REQUEST_CHUNKED_BODY_UNREADY:
    case MHD_REQUEST_BODY_SENT:
      mhd_assert (0);
      return;
    case MHD_REQUEST_FOOTERS_SENDING:
      ret = connection->send_cls (connection,
                                  &request->write_buffer
                                  [request->write_buffer_send_offset],
                                  request->write_buffer_append_offset -
                                    request->write_buffer_send_offset);
      if (ret < 0)
        {
          if (MHD_ERR_AGAIN_ == ret)
            return;
          CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_CONNECTION_WRITE_FAIL_CLOSED,
                                  _("Connection was closed while sending response body.\n"));
          return;
        }
      request->write_buffer_send_offset += ret;
      MHD_connection_update_last_activity_ (connection);
      if (MHD_REQUEST_FOOTERS_SENDING != request->state)
        return;
      check_write_done (request,
                        MHD_REQUEST_FOOTERS_SENT);
      return;
    case MHD_REQUEST_FOOTERS_SENT:
      mhd_assert (0);
      return;
    case MHD_REQUEST_CLOSED:
      return;
#ifdef UPGRADE_SUPPORT
    case MHD_REQUEST_UPGRADE:
      mhd_assert (0);
      return;
#endif /* UPGRADE_SUPPORT */
    default:
      mhd_assert (0);
      CONNECTION_CLOSE_ERROR (connection,
			      MHD_SC_STATEMACHINE_FAILURE_CONNECTION_CLOSED,
                              _("Internal error\n"));
      break;
    }
}


/**
 * Check whether request header contains particular token.
 *
 * Token could be surrounded by spaces and tabs and delimited by comma.
 * Case-insensitive match used for header names and tokens.
 * @param request    the request to get values from
 * @param header     the header name
 * @param token      the token to find
 * @param token_len  the length of token, not including optional
 *                   terminating null-character.
 * @return true if token is found in specified header,
 *         false otherwise
 */
static bool
MHD_lookup_header_token_ci (const struct MHD_Request *request,
			    const char *header,
			    const char *token,
			    size_t token_len)
{
  struct MHD_HTTP_Header *pos;

  if ( (NULL == request) || /* FIXME: require non-null? */
       (NULL == header) || /* FIXME: require non-null? */
       (0 == header[0]) ||
       (NULL == token) ||
       (0 == token[0]) )
    return false;
  for (pos = request->headers_received; NULL != pos; pos = pos->next)
    {
      if ( (0 != (pos->kind & MHD_HEADER_KIND)) &&
	   ( (header == pos->header) ||
	     (MHD_str_equal_caseless_(header,
				      pos->header)) ) &&
	   (MHD_str_has_token_caseless_ (pos->value,
					 token,
					 token_len)) )
        return true;
    }
  return false;
}


/**
 * Check whether request header contains particular static @a tkn.
 *
 * Token could be surrounded by spaces and tabs and delimited by comma.
 * Case-insensitive match used for header names and tokens.
 * @param r   the request to get values from
 * @param h   the header name
 * @param tkn the static string of token to find
 * @return true if token is found in specified header,
 *         false otherwise
 */
#define MHD_lookup_header_s_token_ci(r,h,tkn) \
    MHD_lookup_header_token_ci((r),(h),(tkn),MHD_STATICSTR_LEN_(tkn))


/**
 * Are we allowed to keep the given connection alive?  We can use the
 * TCP stream for a second request if the connection is HTTP 1.1 and
 * the "Connection" header either does not exist or is not set to
 * "close", or if the connection is HTTP 1.0 and the "Connection"
 * header is explicitly set to "keep-alive".  If no HTTP version is
 * specified (or if it is not 1.0 or 1.1), we definitively close the
 * connection.  If the "Connection" header is not exactly "close" or
 * "keep-alive", we proceed to use the default for the respective HTTP
 * version (which is conservative for HTTP 1.0, but might be a bit
 * optimistic for HTTP 1.1).
 *
 * @param request the request to check for keepalive
 * @return #MHD_YES if (based on the request), a keepalive is
 *        legal
 */
static bool
keepalive_possible (struct MHD_Request *request)
{
  if (MHD_CONN_MUST_CLOSE == request->keepalive)
    return false;
  if (NULL == request->version_s)
    return false;
  if ( (NULL != request->response) &&
       (request->response->v10_only) )
    return false;

  if (MHD_str_equal_caseless_ (request->version_s,
			       MHD_HTTP_VERSION_1_1))
    {
      if (MHD_lookup_header_s_token_ci (request,
                                        MHD_HTTP_HEADER_CONNECTION,
                                        "upgrade"))
        return false;
      if (MHD_lookup_header_s_token_ci (request,
                                        MHD_HTTP_HEADER_CONNECTION,
                                        "close"))
        return false;
      return true;
    }
  if (MHD_str_equal_caseless_ (request->version_s,
			       MHD_HTTP_VERSION_1_0))
    {
      if (MHD_lookup_header_s_token_ci (request,
                                        MHD_HTTP_HEADER_CONNECTION,
                                        "Keep-Alive"))
        return true;
      return false;
    }
  return false;
}


/**
 * Produce HTTP time stamp.
 *
 * @param date where to write the header, with
 *        at least 128 bytes available space.
 * @param date_len number of bytes in @a date
 */
static void
get_date_string (char *date,
		 size_t date_len)
{
  static const char *const days[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char *const mons[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  struct tm now;
  time_t t;
#if !defined(HAVE_C11_GMTIME_S) && !defined(HAVE_W32_GMTIME_S) && !defined(HAVE_GMTIME_R)
  struct tm* pNow;
#endif

  date[0] = 0;
  time (&t);
#if defined(HAVE_C11_GMTIME_S)
  if (NULL == gmtime_s (&t,
                        &now))
    return;
#elif defined(HAVE_W32_GMTIME_S)
  if (0 != gmtime_s (&now,
                     &t))
    return;
#elif defined(HAVE_GMTIME_R)
  if (NULL == gmtime_r(&t,
                       &now))
    return;
#else
  pNow = gmtime(&t);
  if (NULL == pNow)
    return;
  now = *pNow;
#endif
  MHD_snprintf_ (date,
		 date_len,
		 "Date: %3s, %02u %3s %04u %02u:%02u:%02u GMT\r\n",
		 days[now.tm_wday % 7],
		 (unsigned int) now.tm_mday,
		 mons[now.tm_mon % 12],
		 (unsigned int) (1900 + now.tm_year),
		 (unsigned int) now.tm_hour,
		 (unsigned int) now.tm_min,
		 (unsigned int) now.tm_sec);
}


/**
 * Check whether response header contains particular @a token.
 *
 * Token could be surrounded by spaces and tabs and delimited by comma.
 * Case-insensitive match used for header names and tokens.
 * @param response  the response to query
 * @param key       header name
 * @param token     the token to find
 * @param token_len the length of token, not including optional
 *                  terminating null-character.
 * @return true if token is found in specified header,
 *         false otherwise
 */
static bool
check_response_header_token_ci (const struct MHD_Response *response,
				const char *key,
				const char *token,
				size_t token_len)
{
  struct MHD_HTTP_Header *pos;

  if ( (NULL == key) ||
       ('\0' == key[0]) ||
       (NULL == token) ||
       ('\0' == token[0]) )
    return false;

  for (pos = response->first_header;
       NULL != pos;
       pos = pos->next)
    {
      if ( (pos->kind == MHD_HEADER_KIND) &&
           MHD_str_equal_caseless_ (pos->header,
                                    key) &&
           MHD_str_has_token_caseless_ (pos->value,
                                        token,
                                        token_len) )
        return true;
    }
  return false;
}


/**
 * Check whether response header contains particular static @a tkn.
 *
 * Token could be surrounded by spaces and tabs and delimited by comma.
 * Case-insensitive match used for header names and tokens.
 * @param r   the response to query
 * @param k   header name
 * @param tkn the static string of token to find
 * @return true if token is found in specified header,
 *         false otherwise
 */
#define check_response_header_s_token_ci(r,k,tkn) \
    check_response_header_token_ci((r),(k),(tkn),MHD_STATICSTR_LEN_(tkn))


/**
 * Allocate the connection's write buffer and fill it with all of the
 * headers (or footers, if we have already sent the body) from the
 * HTTPd's response.  If headers are missing in the response supplied
 * by the application, additional headers may be added here.
 *
 * @param request the request for which to build the response header
 * @return true on success, false on failure (out of memory)
 */
static bool
build_header_response (struct MHD_Request *request)
{
  struct MHD_Connection *connection = request->connection;
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Response *response = request->response;
  size_t size;
  size_t off;
  struct MHD_HTTP_Header *pos;
  char code[256];
  char date[128];
  size_t datelen;
  char content_length_buf[128];
  size_t content_length_len;
  char *data;
  enum MHD_ValueKind kind;
  bool client_requested_close;
  bool response_has_close;
  bool response_has_keepalive;
  const char *have_encoding;
  const char *have_content_length;
  bool must_add_close;
  bool must_add_chunked_encoding;
  bool must_add_keep_alive;
  bool must_add_content_length;

  mhd_assert (NULL != request->version_s);
  if (0 == request->version_s[0])
    {
      data = MHD_pool_allocate (connection->pool,
                                0,
                                MHD_YES);
      request->write_buffer = data;
      request->write_buffer_append_offset = 0;
      request->write_buffer_send_offset = 0;
      request->write_buffer_size = 0;
      return true;
    }
  if (MHD_REQUEST_FOOTERS_RECEIVED == request->state)
    {
      const char *reason_phrase;
      const char *version;

      reason_phrase
	= MHD_get_reason_phrase_for (response->status_code);
      version
	= (response->icy)
	? "ICY"
	: ( (MHD_str_equal_caseless_ (MHD_HTTP_VERSION_1_0,
				      request->version_s))
	    ? MHD_HTTP_VERSION_1_0
	    : MHD_HTTP_VERSION_1_1);
      MHD_snprintf_ (code,
		     sizeof (code),
		     "%s %u %s\r\n",
		     version,
		     response->status_code,
		     reason_phrase);
      off = strlen (code);
      /* estimate size */
      size = off + 2;           /* +2 for extra "\r\n" at the end */
      kind = MHD_HEADER_KIND;
      if ( (! daemon->suppress_date) &&
	   (NULL == MHD_response_get_header (response,
					     MHD_HTTP_HEADER_DATE)) )
        get_date_string (date,
			 sizeof (date));
      else
        date[0] = '\0';
      datelen = strlen (date);
      size += datelen;
    }
  else
    {
      /* 2 bytes for final CRLF of a Chunked-Body */
      size = 2;
      kind = MHD_FOOTER_KIND;
      off = 0;
      datelen = 0;
    }

  /* calculate extra headers we need to add, such as 'Connection: close',
     first see what was explicitly requested by the application */
  must_add_close = false;
  must_add_chunked_encoding = false;
  must_add_keep_alive = false;
  must_add_content_length = false;
  response_has_close = false;
  switch (request->state)
    {
    case MHD_REQUEST_FOOTERS_RECEIVED:
      response_has_close
	= check_response_header_s_token_ci (response,
					    MHD_HTTP_HEADER_CONNECTION,
					    "close");
      response_has_keepalive
	= check_response_header_s_token_ci (response,
					    MHD_HTTP_HEADER_CONNECTION,
					    "Keep-Alive");
      client_requested_close
	= MHD_lookup_header_s_token_ci (request,
					MHD_HTTP_HEADER_CONNECTION,
					"close");

      if (response->v10_only)
        request->keepalive = MHD_CONN_MUST_CLOSE;
#ifdef UPGRADE_SUPPORT
      else if (NULL != response->upgrade_handler)
        /* If this connection will not be "upgraded", it must be closed. */
        request->keepalive = MHD_CONN_MUST_CLOSE;
#endif /* UPGRADE_SUPPORT */

      /* now analyze chunked encoding situation */
      request->have_chunked_upload = false;

      if ( (MHD_SIZE_UNKNOWN == response->total_size) &&
#ifdef UPGRADE_SUPPORT
           (NULL == response->upgrade_handler) &&
#endif /* UPGRADE_SUPPORT */
           (! response_has_close) &&
           (! client_requested_close) )
        {
          /* size is unknown, and close was not explicitly requested;
             need to either to HTTP 1.1 chunked encoding or
             close the connection */
          /* 'close' header doesn't exist yet, see if we need to add one;
             if the client asked for a close, no need to start chunk'ing */
          if ( (keepalive_possible (request)) &&
               (MHD_str_equal_caseless_ (MHD_HTTP_VERSION_1_1,
                                         request->version_s)) )
            {
              have_encoding
		= MHD_response_get_header (response,
					   MHD_HTTP_HEADER_TRANSFER_ENCODING);
              if (NULL == have_encoding)
                {
                  must_add_chunked_encoding = true;
                  request->have_chunked_upload = true;
                }
              else if (MHD_str_equal_caseless_ (have_encoding,
                                                "identity"))
                {
                  /* application forced identity encoding, can't do 'chunked' */
                  must_add_close = true;
                }
              else
                {
                  request->have_chunked_upload = true;
                }
            }
          else
            {
              /* Keep alive or chunking not possible
                 => set close header if not present */
              if (! response_has_close)
                must_add_close = true;
            }
        }

      /* check for other reasons to add 'close' header */
      if ( ( (client_requested_close) ||
             (connection->read_closed) ||
             (MHD_CONN_MUST_CLOSE == request->keepalive)) &&
           (! response_has_close) &&
#ifdef UPGRADE_SUPPORT
           (NULL == response->upgrade_handler) &&
#endif /* UPGRADE_SUPPORT */
           (! response->v10_only) )
        must_add_close = true;

      /* check if we should add a 'content length' header */
      have_content_length
	= MHD_response_get_header (response,
				   MHD_HTTP_HEADER_CONTENT_LENGTH);

      /* MHD_HTTP_NO_CONTENT, MHD_HTTP_NOT_MODIFIED and 1xx-status
         codes SHOULD NOT have a Content-Length according to spec;
         also chunked encoding / unknown length or CONNECT... */
      if ( (MHD_SIZE_UNKNOWN != response->total_size) &&
           (MHD_HTTP_NO_CONTENT != response->status_code) &&
           (MHD_HTTP_NOT_MODIFIED != response->status_code) &&
           (MHD_HTTP_OK <= response->status_code) &&
           (NULL == have_content_length) &&
           (request->method != MHD_METHOD_CONNECT) )
        {
          /*
            Here we add a content-length if one is missing; however,
            for 'connect' methods, the responses MUST NOT include a
            content-length header *if* the response code is 2xx (in
            which case we expect there to be no body).  Still,
            as we don't know the response code here in some cases, we
            simply only force adding a content-length header if this
            is not a 'connect' or if the response is not empty
            (which is kind of more sane, because if some crazy
            application did return content with a 2xx status code,
            then having a content-length might again be a good idea).

            Note that the change from 'SHOULD NOT' to 'MUST NOT' is
            a recent development of the HTTP 1.1 specification.
          */
          content_length_len
            = MHD_snprintf_ (content_length_buf,
			     sizeof (content_length_buf),
			     MHD_HTTP_HEADER_CONTENT_LENGTH ": " MHD_UNSIGNED_LONG_LONG_PRINTF "\r\n",
			     (MHD_UNSIGNED_LONG_LONG) response->total_size);
          must_add_content_length = true;
        }

      /* check for adding keep alive */
      if ( (! response_has_keepalive) &&
           (! response_has_close) &&
           (! must_add_close) &&
           (MHD_CONN_MUST_CLOSE != request->keepalive) &&
#ifdef UPGRADE_SUPPORT
           (NULL == response->upgrade_handler) &&
#endif /* UPGRADE_SUPPORT */
           (keepalive_possible (request)) )
        must_add_keep_alive = true;
      break;
    case MHD_REQUEST_BODY_SENT:
      response_has_keepalive = false;
      break;
    default:
      mhd_assert (0);
      return MHD_NO;
    }

  if (MHD_CONN_MUST_CLOSE != request->keepalive)
    {
      if ( (must_add_close) ||
	   (response_has_close) )
        request->keepalive = MHD_CONN_MUST_CLOSE;
      else if ( (must_add_keep_alive) ||
		(response_has_keepalive) )
        request->keepalive = MHD_CONN_USE_KEEPALIVE;
    }

  if (must_add_close)
    size += MHD_STATICSTR_LEN_ ("Connection: close\r\n");
  if (must_add_keep_alive)
    size += MHD_STATICSTR_LEN_ ("Connection: Keep-Alive\r\n");
  if (must_add_chunked_encoding)
    size += MHD_STATICSTR_LEN_ ("Transfer-Encoding: chunked\r\n");
  if (must_add_content_length)
    size += content_length_len;
  mhd_assert (! (must_add_close && must_add_keep_alive) );
  mhd_assert (! (must_add_chunked_encoding && must_add_content_length) );

  for (pos = response->first_header; NULL != pos; pos = pos->next)
    {
      /* TODO: add proper support for excluding "Keep-Alive" token. */
      if ( (pos->kind == kind) &&
           (! ( (must_add_close) &&
                (response_has_keepalive) &&
                (MHD_str_equal_caseless_(pos->header,
                                         MHD_HTTP_HEADER_CONNECTION)) &&
                (MHD_str_equal_caseless_(pos->value,
                                         "Keep-Alive")) ) ) )
        size += strlen (pos->header) + strlen (pos->value) + 4; /* colon, space, linefeeds */
    }
  /* produce data */
  data = MHD_pool_allocate (connection->pool,
                            size + 1,
                            MHD_NO);
  if (NULL == data)
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_CONNECTION_POOL_MALLOC_FAILURE,
                "Not enough memory for write!\n");
#endif
      return false;
    }
  if (MHD_REQUEST_FOOTERS_RECEIVED == request->state)
    {
      memcpy (data,
              code,
              off);
    }
  if (must_add_close)
    {
      /* we must add the 'Connection: close' header */
      memcpy (&data[off],
              "Connection: close\r\n",
              MHD_STATICSTR_LEN_ ("Connection: close\r\n"));
      off += MHD_STATICSTR_LEN_ ("Connection: close\r\n");
    }
  if (must_add_keep_alive)
    {
      /* we must add the 'Connection: Keep-Alive' header */
      memcpy (&data[off],
              "Connection: Keep-Alive\r\n",
              MHD_STATICSTR_LEN_ ("Connection: Keep-Alive\r\n"));
      off += MHD_STATICSTR_LEN_ ("Connection: Keep-Alive\r\n");
    }
  if (must_add_chunked_encoding)
    {
      /* we must add the 'Transfer-Encoding: chunked' header */
      memcpy (&data[off],
              "Transfer-Encoding: chunked\r\n",
              MHD_STATICSTR_LEN_ ("Transfer-Encoding: chunked\r\n"));
      off += MHD_STATICSTR_LEN_ ("Transfer-Encoding: chunked\r\n");
    }
  if (must_add_content_length)
    {
      /* we must add the 'Content-Length' header */
      memcpy (&data[off],
              content_length_buf,
	      content_length_len);
      off += content_length_len;
    }
  for (pos = response->first_header; NULL != pos; pos = pos->next)
    {
      /* TODO: add proper support for excluding "Keep-Alive" token. */
      if ( (pos->kind == kind) &&
           (! ( (must_add_close) &&
                (response_has_keepalive) &&
                (MHD_str_equal_caseless_(pos->header,
                                         MHD_HTTP_HEADER_CONNECTION)) &&
                (MHD_str_equal_caseless_(pos->value,
                                         "Keep-Alive")) ) ) )
        off += MHD_snprintf_ (&data[off],
                              size - off,
                              "%s: %s\r\n",
                              pos->header,
                              pos->value);
    }
  if (MHD_REQUEST_FOOTERS_RECEIVED == request->state)
    {
      memcpy (&data[off],
              date,
	      datelen);
      off += datelen;
    }
  memcpy (&data[off],
          "\r\n",
          2);
  off += 2;

  if (off != size)
    mhd_panic (mhd_panic_cls,
               __FILE__,
               __LINE__,
               NULL);
  request->write_buffer = data;
  request->write_buffer_append_offset = size;
  request->write_buffer_send_offset = 0;
  request->write_buffer_size = size + 1;
  return true;
}


/**
 * We encountered an error processing the request.  Handle it properly
 * by stopping to read data and sending the indicated response code
 * and message.
 *
 * @param request the request
 * @param ec error code for MHD
 * @param status_code the response code to send (400, 413 or 414)
 * @param message the error message to send
 */
static void
transmit_error_response (struct MHD_Request *request,
			 enum MHD_StatusCode ec,
                         enum MHD_HTTP_StatusCode status_code,
			 const char *message)
{
  struct MHD_Response *response;

  if (NULL == request->version_s)
    {
      /* we were unable to process the full header line, so we don't
	 really know what version the client speaks; assume 1.0 */
      request->version_s = MHD_HTTP_VERSION_1_0;
    }
  request->state = MHD_REQUEST_FOOTERS_RECEIVED;
  request->connection->read_closed = true;
#ifdef HAVE_MESSAGES
  MHD_DLOG (request->daemon,
	    ec,
            _("Error processing request (HTTP response code is %u (`%s')). Closing connection.\n"),
            status_code,
            message);
#endif
  if (NULL != request->response)
    {
      MHD_response_queue_for_destroy (request->response);
      request->response = NULL;
    }
  response = MHD_response_from_buffer (status_code,
				       strlen (message),
				       (void *) message,
				       MHD_RESPMEM_PERSISTENT);
  request->response = response;
  /* Do not reuse this connection. */
  request->keepalive = MHD_CONN_MUST_CLOSE;
  if (! build_header_response (request))
    {
      /* oops - close! */
      CONNECTION_CLOSE_ERROR (request->connection,
			      ec,
			      _("Closing connection (failed to create response header)\n"));
    }
  else
    {
      request->state = MHD_REQUEST_HEADERS_SENDING;
    }
}


/**
 * Convert @a method to the respective enum value.
 *
 * @param method the method string to look up enum value for
 * @return resulting enum, or generic value for "unknown"
 */
static enum MHD_Method
method_string_to_enum (const char *method)
{
  static const struct {
    const char *key;
    enum MHD_Method value;
  } methods[] = {
    { "OPTIONS", MHD_METHOD_OPTIONS },
    { "GET", MHD_METHOD_GET },
    { "HEAD", MHD_METHOD_HEAD },
    { "POST", MHD_METHOD_POST },
    { "PUT", MHD_METHOD_PUT },
    { "DELETE", MHD_METHOD_DELETE },
    { "TRACE", MHD_METHOD_TRACE },
    { "CONNECT", MHD_METHOD_CONNECT },
    { "ACL", MHD_METHOD_ACL },
    { "BASELINE_CONTROL", MHD_METHOD_BASELINE_CONTROL },
    { "BIND", MHD_METHOD_BIND },
    { "CHECKIN", MHD_METHOD_CHECKIN },
    { "CHECKOUT", MHD_METHOD_CHECKOUT },
    { "COPY", MHD_METHOD_COPY },
    { "LABEL", MHD_METHOD_LABEL },
    { "LINK", MHD_METHOD_LINK },
    { "LOCK", MHD_METHOD_LOCK },
    { "MERGE", MHD_METHOD_MERGE },
    { "MKACTIVITY", MHD_METHOD_MKACTIVITY },
    { "MKCOL", MHD_METHOD_MKCOL },
    { "MKREDIRECTREF", MHD_METHOD_MKREDIRECTREF },
    { "MKWORKSPACE", MHD_METHOD_MKWORKSPACE },
    { "MOVE", MHD_METHOD_MOVE },
    { "ORDERPATCH", MHD_METHOD_ORDERPATCH },
    { "PRI", MHD_METHOD_PRI },
    { "PROPFIND", MHD_METHOD_PROPFIND },
    { "PROPPATCH", MHD_METHOD_PROPPATCH },
    { "REBIND", MHD_METHOD_REBIND },
    { "REPORT", MHD_METHOD_REPORT },
    { "SEARCH", MHD_METHOD_SEARCH },
    { "UNBIND", MHD_METHOD_UNBIND },
    { "UNCHECKOUT", MHD_METHOD_UNCHECKOUT },
    { "UNLINK", MHD_METHOD_UNLINK },
    { "UNLOCK", MHD_METHOD_UNLOCK },
    { "UPDATE", MHD_METHOD_UPDATE },
    { "UPDATEDIRECTREF", MHD_METHOD_UPDATEDIRECTREF },
    { "VERSION-CONTROL", MHD_METHOD_VERSION_CONTROL },
    { NULL, MHD_METHOD_UNKNOWN } /* must be last! */
  };
  unsigned int i;

  for (i=0;NULL != methods[i].key;i++)
    if (0 ==
	MHD_str_equal_caseless_ (method,
				 methods[i].key))
      return methods[i].value;
  return MHD_METHOD_UNKNOWN;
}


/**
 * Add an entry to the HTTP headers of a request.  If this fails,
 * transmit an error response (request too big).
 *
 * @param request the request for which a value should be set
 * @param kind kind of the value
 * @param key key for the value
 * @param value the value itself
 * @return false on failure (out of memory), true for success
 */
static bool
request_add_header (struct MHD_Request *request,
		    const char *key,
		    const char *value,
		    enum MHD_ValueKind kind)
{
  if (MHD_NO ==
      MHD_request_set_value (request,
			     kind,
			     key,
			     value))
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (request->daemon,
		MHD_SC_CONNECTION_POOL_MALLOC_FAILURE,
                _("Not enough memory in pool to allocate header record!\n"));
#endif
      transmit_error_response (request,
			       MHD_SC_CLIENT_HEADER_TOO_BIG,
                               MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE,
                               REQUEST_TOO_BIG);
      return false;
    }
  return true;
}


/**
 * Parse the first line of the HTTP HEADER.
 *
 * @param connection the connection (updated)
 * @param line the first line, not 0-terminated
 * @param line_len length of the first @a line
 * @return true if the line is ok, false if it is malformed
 */
static bool
parse_initial_message_line (struct MHD_Request *request,
                            char *line,
                            size_t line_len)
{
  struct MHD_Daemon *daemon = request->daemon;
  const char *curi;
  char *uri;
  char *http_version;
  char *args;
  unsigned int unused_num_headers;
  size_t url_end;

  if (NULL == (uri = memchr (line,
                             ' ',
                             line_len)))
    return false;              /* serious error */
  uri[0] = '\0';
  request->method_s = line;
  request->method = method_string_to_enum (line);
  uri++;
  /* Skip any spaces. Not required by standard but allow
     to be more tolerant. */
  while ( (' ' == uri[0]) &&
          ( (size_t)(uri - line) < line_len) )
    uri++;
  if ((size_t)(uri - line) == line_len)
    {
      curi = "";
      uri = NULL;
      request->version_s = "";
      args = NULL;
      url_end = line_len - (line - uri); // EH, this is garbage. FIXME!
    }
  else
    {
      curi = uri;
      /* Search from back to accept misformed URI with space */
      http_version = line + line_len - 1;
      /* Skip any trailing spaces */
      while ( (' ' == http_version[0]) &&
              (http_version > uri) )
        http_version--;
      /* Find first space in reverse direction */
      while ( (' ' != http_version[0]) &&
              (http_version > uri) )
        http_version--;
      if (http_version > uri)
        {
          http_version[0] = '\0';
          request->version_s = http_version + 1;
          args = memchr (uri,
                         '?',
                         http_version - uri);
        }
      else
        {
          request->version_s = "";
          args = memchr (uri,
                         '?',
                         line_len - (uri - line));
        }
      url_end = http_version - uri;
    }
  if ( (MHD_PSL_STRICT == daemon->protocol_strict_level) &&
       (NULL != memchr (curi,
                        ' ',
                        url_end)) )
    {
      /* space exists in URI and we are supposed to be strict, reject */
      return MHD_NO;
    }
  if (NULL != daemon->early_uri_logger_cb)
    {
      request->client_context
        = daemon->early_uri_logger_cb (daemon->early_uri_logger_cb_cls,
				       curi,
				       request);
    }
  if (NULL != args)
    {
      args[0] = '\0';
      args++;
      /* note that this call clobbers 'args' */
      MHD_parse_arguments_ (request,
			    MHD_GET_ARGUMENT_KIND,
			    args,
			    &request_add_header,
			    &unused_num_headers);
    }
  if (NULL != uri)
    daemon->unescape_cb (daemon->unescape_cb_cls,
			 request,
			 uri);
  request->url = curi;
  return true;
}


/**
 * We have received (possibly the beginning of) a line in the
 * header (or footer).  Validate (check for ":") and prepare
 * to process.
 *
 * @param request the request we're processing
 * @param line line from the header to process
 * @return true on success, false on error (malformed @a line)
 */
static bool
process_header_line (struct MHD_Request *request,
                     char *line)
{
  struct MHD_Connection *connection = request->connection;
  char *colon;

  /* line should be normal header line, find colon */
  colon = strchr (line,
		  ':');
  if (NULL == colon)
    {
      /* error in header line, die hard */
      CONNECTION_CLOSE_ERROR (connection,
			      MHD_SC_CONNECTION_PARSE_FAIL_CLOSED,
			      _("Received malformed line (no colon). Closing connection.\n"));
      return false;
    }
  if (MHD_PSL_PERMISSIVE != request->daemon->protocol_strict_level)
    {
      /* check for whitespace before colon, which is not allowed
	 by RFC 7230 section 3.2.4; we count space ' ' and
	 tab '\t', but not '\r\n' as those would have ended the line. */
      const char *white;

      white = strchr (line,
		      (unsigned char) ' ');
      if ( (NULL != white) &&
	   (white < colon) )
	{
	  CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_CONNECTION_PARSE_FAIL_CLOSED,
				  _("Whitespace before colon forbidden by RFC 7230. Closing connection.\n"));
	  return false;
	}
      white = strchr (line,
		      (unsigned char) '\t');
      if ( (NULL != white) &&
	   (white < colon) )
	{
	  CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_CONNECTION_PARSE_FAIL_CLOSED,
				  _("Tab before colon forbidden by RFC 7230. Closing connection.\n"));
	  return false;
	}
    }
  /* zero-terminate header */
  colon[0] = '\0';
  colon++;                      /* advance to value */
  while ( ('\0' != colon[0]) &&
          ( (' ' == colon[0]) ||
            ('\t' == colon[0]) ) )
    colon++;
  /* we do the actual adding of the connection
     header at the beginning of the while
     loop since we need to be able to inspect
     the *next* header line (in case it starts
     with a space...) */
  request->last = line;
  request->colon = colon;
  return true;
}


/**
 * Process a header value that spans multiple lines.
 * The previous line(s) are in connection->last.
 *
 * @param request the request we're processing
 * @param line the current input line
 * @param kind if the line is complete, add a header
 *        of the given kind
 * @return true if the line was processed successfully
 */
static bool
process_broken_line (struct MHD_Request *request,
                     char *line,
                     enum MHD_ValueKind kind)
{
  struct MHD_Connection *connection = request->connection;
  char *last;
  char *tmp;
  size_t last_len;
  size_t tmp_len;

  last = request->last;
  if ( (' ' == line[0]) ||
       ('\t' == line[0]) )
    {
      /* value was continued on the next line, see
         http://www.jmarshall.com/easy/http/ */
      last_len = strlen (last);
      /* skip whitespace at start of 2nd line */
      tmp = line;
      while ( (' ' == tmp[0]) ||
              ('\t' == tmp[0]) )
        tmp++;
      tmp_len = strlen (tmp);
      /* FIXME: we might be able to do this better (faster!), as most
	 likely 'last' and 'line' should already be adjacent in
	 memory; however, doing this right gets tricky if we have a
	 value continued over multiple lines (in which case we need to
	 record how often we have done this so we can check for
	 adjacency); also, in the case where these are not adjacent
	 (not sure how it can happen!), we would want to allocate from
	 the end of the pool, so as to not destroy the read-buffer's
	 ability to grow nicely. */
      last = MHD_pool_reallocate (connection->pool,
                                  last,
                                  last_len + 1,
                                  last_len + tmp_len + 1);
      if (NULL == last)
        {
          transmit_error_response (request,
				   MHD_SC_CLIENT_HEADER_TOO_BIG,
                                   MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE,
                                   REQUEST_TOO_BIG);
          return MHD_NO;
        }
      memcpy (&last[last_len],
	      tmp,
	      tmp_len + 1);
      request->last = last;
      return MHD_YES;           /* possibly more than 2 lines... */
    }
  mhd_assert ( (NULL != last) &&
                (NULL != request->colon) );
  if (! request_add_header (request,
			    last,
			    request->colon,
			    kind))
    {
      transmit_error_response (request,
			       MHD_SC_CLIENT_HEADER_TOO_BIG,
                               MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE,
                               REQUEST_TOO_BIG);
      return false;
    }
  /* we still have the current line to deal with... */
  if ('\0' != line[0])
    {
      if (! process_header_line (request,
				 line))
        {
          transmit_error_response (request,
				   MHD_SC_CONNECTION_PARSE_FAIL_CLOSED,
                                   MHD_HTTP_BAD_REQUEST,
                                   REQUEST_MALFORMED);
          return false;
        }
    }
  return true;
}


/**
 * Parse a single line of the HTTP header.  Advance read_buffer (!)
 * appropriately.  If the current line does not fit, consider growing
 * the buffer.  If the line is far too long, close the connection.  If
 * no line is found (incomplete, buffer too small, line too long),
 * return NULL.  Otherwise return a pointer to the line.
 *
 * @param request request we're processing
 * @param[out] line_len pointer to variable that receive
 *             length of line or NULL
 * @return NULL if no full line is available; note that the returned
 *         string will not be 0-termianted
 */
static char *
get_next_header_line (struct MHD_Request *request,
                      size_t *line_len)
{
  char *rbuf;
  size_t pos;

  if (0 == request->read_buffer_offset)
    return NULL;
  pos = 0;
  rbuf = request->read_buffer;
  while ( (pos < request->read_buffer_offset - 1) &&
          ('\r' != rbuf[pos]) &&
          ('\n' != rbuf[pos]) )
    pos++;
  if ( (pos == request->read_buffer_offset - 1) &&
       ('\n' != rbuf[pos]) )
    {
      /* not found, consider growing... */
      if ( (request->read_buffer_offset == request->read_buffer_size) &&
	   (! try_grow_read_buffer (request)) )
	{
	  transmit_error_response (request,
				   MHD_SC_CLIENT_HEADER_TOO_BIG,
				   (NULL != request->url)
				   ? MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE
				   : MHD_HTTP_URI_TOO_LONG,
				   REQUEST_TOO_BIG);
	}
      if (line_len)
        *line_len = 0;
      return NULL;
    }

  if (line_len)
    *line_len = pos;
  /* found, check if we have proper LFCR */
  if ( ('\r' == rbuf[pos]) &&
       ('\n' == rbuf[pos + 1]) )
    rbuf[pos++] = '\0';         /* skip both r and n */
  rbuf[pos++] = '\0';
  request->read_buffer += pos;
  request->read_buffer_size -= pos;
  request->read_buffer_offset -= pos;
  return rbuf;
}


/**
 * Check whether is possible to force push socket buffer content as
 * partial packet.
 * MHD use different buffering logic depending on whether flushing of
 * socket buffer is possible or not.
 * If flushing IS possible than MHD activates extra buffering before
 * sending data to prevent sending partial packets and flush pending
 * data in socket buffer to push last partial packet to client after
 * sending logical completed part of data (for example: after sending
 * full response header or full response message).
 * If flushing IS NOT possible than MHD activates no buffering (no
 * delay sending) when it going to send formed fully completed logical
 * part of data and activate normal buffering after sending.
 * For idled keep-alive connection MHD always activate normal
 * buffering.
 *
 * @param connection connection to check
 * @return true if force push is possible, false otherwise
 */
static bool
socket_flush_possible(struct MHD_Connection *connection)
{
  (void) connection; /* Mute compiler warning. */
#if defined(TCP_CORK) || defined(TCP_PUSH)
  return true;
#else  /* !TCP_CORK && !TCP_PUSH */
  return false;
#endif /* !TCP_CORK && !TCP_PUSH */
}


/**
 * Activate extra buffering mode on connection socket to prevent
 * sending of partial packets.
 *
 * @param connection connection to be processed
 * @return true on success, false otherwise
 */
static bool
socket_start_extra_buffering (struct MHD_Connection *connection)
{
  bool res = false;
  (void)connection; /* Mute compiler warning. */
#if defined(TCP_CORK) || defined(TCP_NOPUSH)
  const MHD_SCKT_OPT_BOOL_ on_val = 1;
#if defined(TCP_NODELAY)
  const MHD_SCKT_OPT_BOOL_ off_val = 0;
#endif /* TCP_NODELAY */
  mhd_assert(NULL != connection);
#if defined(TCP_NOPUSH) && !defined(TCP_CORK)
  /* Buffer data before sending */
  res = (0 == setsockopt (connection->socket_fd,
                          IPPROTO_TCP,
                          TCP_NOPUSH,
                          (const void *) &on_val,
                          sizeof (on_val)))
    ? true : false;
#if defined(TCP_NODELAY)
  /* Enable Nagle's algorithm */
  /* TCP_NODELAY may interfere with TCP_NOPUSH */
  res &= (0 == setsockopt (connection->socket_fd,
                           IPPROTO_TCP,
                           TCP_NODELAY,
                           (const void *) &off_val,
                           sizeof (off_val)))
    ? true : false;
#endif /* TCP_NODELAY */
#else /* TCP_CORK */
#if defined(TCP_NODELAY)
  /* Enable Nagle's algorithm */
  /* TCP_NODELAY may prevent enabling TCP_CORK. Resulting buffering mode depends
     solely on TCP_CORK result, so ignoring return code here. */
  (void) setsockopt (connection->socket_fd,
                     IPPROTO_TCP,
                     TCP_NODELAY,
                     (const void *) &off_val,
                     sizeof (off_val));
#endif /* TCP_NODELAY */
  /* Send only full packets */
  res = (0 == setsockopt (connection->socket_fd,
                          IPPROTO_TCP,
                          TCP_CORK,
                          (const void *) &on_val,
                          sizeof (on_val)))
    ? true : false;
#endif /* TCP_CORK */
#endif /* TCP_CORK || TCP_NOPUSH */
  return res;
}


/**
 * Activate no buffering mode (no delay sending) on connection socket.
 *
 * @param connection connection to be processed
 * @return true on success, false otherwise
 */
static bool
socket_start_no_buffering (struct MHD_Connection *connection)
{
#if defined(TCP_NODELAY)
  bool res = true;
  const MHD_SCKT_OPT_BOOL_ on_val = 1;
#if defined(TCP_CORK) || defined(TCP_NOPUSH)
  const MHD_SCKT_OPT_BOOL_ off_val = 0;
#endif /* TCP_CORK || TCP_NOPUSH */

  (void)connection; /* Mute compiler warning. */
  mhd_assert(NULL != connection);
#if defined(TCP_CORK)
  /* Allow partial packets */
  res &= (0 == setsockopt (connection->socket_fd,
                           IPPROTO_TCP,
                           TCP_CORK,
                           (const void *) &off_val,
                           sizeof (off_val)))
    ? true : false;
#endif /* TCP_CORK */
#if defined(TCP_NODELAY)
  /* Disable Nagle's algorithm for sending packets without delay */
  res &= (0 == setsockopt (connection->socket_fd,
                           IPPROTO_TCP,
                           TCP_NODELAY,
                           (const void *) &on_val,
                           sizeof (on_val)))
    ? true : false;
#endif /* TCP_NODELAY */
#if defined(TCP_NOPUSH) && !defined(TCP_CORK)
  /* Disable extra buffering */
  res &= (0 == setsockopt (connection->socket_fd,
                           IPPROTO_TCP,
                           TCP_NOPUSH,
                           (const void *) &off_val,
                           sizeof (off_val)))
    ? true : false;
#endif /* TCP_NOPUSH  && !TCP_CORK */
  return res;
#else  /* !TCP_NODELAY */
  return false;
#endif /* !TCP_NODELAY */
}


/**
 * Activate no buffering mode (no delay sending) on connection socket
 * and push to client data pending in socket buffer.
 *
 * @param connection connection to be processed
 * @return true on success, false otherwise
 */
static bool
socket_start_no_buffering_flush (struct MHD_Connection *connection)
{
  bool res = true;
#if defined(TCP_NOPUSH) && !defined(TCP_CORK)
  const int dummy = 0;
#endif /* !TCP_CORK */

  if (NULL == connection)
    return false; /* FIXME: use MHD_NONNULL? */
  res = socket_start_no_buffering (connection);
#if defined(TCP_NOPUSH) && !defined(TCP_CORK)
  /* Force flush data with zero send otherwise Darwin and some BSD systems
     will add 5 seconds delay. Not required with TCP_CORK as switching off
     TCP_CORK always flushes socket buffer. */
  res &= (0 <= MHD_send_ (connection->socket_fd,
                          &dummy,
                          0))
    ? true : false;
#endif /* TCP_NOPUSH && !TCP_CORK*/
  return res;
}


/**
 * Activate normal buffering mode on connection socket.
 *
 * @param connection connection to be processed
 * @return true on success, false otherwise
 */
static bool
socket_start_normal_buffering (struct MHD_Connection *connection)
{
#if defined(TCP_NODELAY)
  bool res = true;
  const MHD_SCKT_OPT_BOOL_ off_val = 0;
#if defined(TCP_CORK)
  MHD_SCKT_OPT_BOOL_ cork_val = 0;
  socklen_t param_size = sizeof (cork_val);
#endif /* TCP_CORK */

  mhd_assert(NULL != connection);
#if defined(TCP_CORK)
  /* Allow partial packets */
  /* Disabling TCP_CORK will flush partial packet even if TCP_CORK wasn't enabled before
     so try to check current value of TCP_CORK to prevent unrequested flushing */
  if ( (0 != getsockopt (connection->socket_fd,
                         IPPROTO_TCP,
                         TCP_CORK,
                         (void*)&cork_val,
                         &param_size)) ||
       (0 != cork_val))
    res &= (0 == setsockopt (connection->socket_fd,
                             IPPROTO_TCP,
                             TCP_CORK,
                             (const void *) &off_val,
                             sizeof (off_val)))
      ? true : false;
#elif defined(TCP_NOPUSH)
  /* Disable extra buffering */
  /* No need to check current value as disabling TCP_NOPUSH will not flush partial
     packet if TCP_NOPUSH wasn't enabled before */
  res &= (0 == setsockopt (connection->socket_fd,
                           IPPROTO_TCP,
                           TCP_NOPUSH,
                           (const void *) &off_val,
                           sizeof (off_val)))
    ? true : false;
#endif /* TCP_NOPUSH && !TCP_CORK */
  /* Enable Nagle's algorithm for normal buffering */
  res &= (0 == setsockopt (connection->socket_fd,
                           IPPROTO_TCP,
                           TCP_NODELAY,
                           (const void *) &off_val,
                           sizeof (off_val)))
    ? true : false;
  return res;
#else  /* !TCP_NODELAY */
  return false;
#endif /* !TCP_NODELAY */
}


/**
 * Do we (still) need to send a 100 continue
 * message for this request?
 *
 * @param request the request to test
 * @return false if we don't need 100 CONTINUE, true if we do
 */
static bool
need_100_continue (struct MHD_Request *request)
{
  const char *expect;

  return ( (NULL == request->response) &&
	   (NULL != request->version_s) &&
       (MHD_str_equal_caseless_(request->version_s,
				MHD_HTTP_VERSION_1_1)) &&
	   (NULL != (expect = MHD_request_lookup_value (request,
							MHD_HEADER_KIND,
							MHD_HTTP_HEADER_EXPECT))) &&
	   (MHD_str_equal_caseless_(expect,
                                    "100-continue")) &&
	   (request->continue_message_write_offset <
	    MHD_STATICSTR_LEN_ (HTTP_100_CONTINUE)) );
}


/**
 * Parse the cookie header (see RFC 2109).
 *
 * @param request request to parse header of
 * @return true for success, false for failure (malformed, out of memory)
 */
static int
parse_cookie_header (struct MHD_Request *request)
{
  const char *hdr;
  char *cpy;
  char *pos;
  char *sce;
  char *semicolon;
  char *equals;
  char *ekill;
  char old;
  int quotes;

  hdr = MHD_request_lookup_value (request,
				  MHD_HEADER_KIND,
				  MHD_HTTP_HEADER_COOKIE);
  if (NULL == hdr)
    return true;
  cpy = MHD_pool_allocate (request->connection->pool,
                           strlen (hdr) + 1,
                           MHD_YES);
  if (NULL == cpy)
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (request->daemon,
		MHD_SC_COOKIE_POOL_ALLOCATION_FAILURE,
                _("Not enough memory in pool to parse cookies!\n"));
#endif
      transmit_error_response (request,
			       MHD_SC_COOKIE_POOL_ALLOCATION_FAILURE,
                               MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE,
                               REQUEST_TOO_BIG);
      return false;
    }
  memcpy (cpy,
          hdr,
          strlen (hdr) + 1);
  pos = cpy;
  while (NULL != pos)
    {
      while (' ' == *pos)
        pos++;                  /* skip spaces */

      sce = pos;
      while ( ((*sce) != '\0') &&
              ((*sce) != ',') &&
              ((*sce) != ';') &&
              ((*sce) != '=') )
        sce++;
      /* remove tailing whitespace (if any) from key */
      ekill = sce - 1;
      while ( (*ekill == ' ') &&
              (ekill >= pos) )
        *(ekill--) = '\0';
      old = *sce;
      *sce = '\0';
      if (old != '=')
        {
          /* value part omitted, use empty string... */
          if (! request_add_header (request,
				    pos,
				    "",
				    MHD_COOKIE_KIND))
            return false;
          if (old == '\0')
            break;
          pos = sce + 1;
          continue;
        }
      equals = sce + 1;
      quotes = 0;
      semicolon = equals;
      while ( ('\0' != semicolon[0]) &&
              ( (0 != quotes) ||
                ( (';' != semicolon[0]) &&
                  (',' != semicolon[0]) ) ) )
        {
          if ('"' == semicolon[0])
            quotes = (quotes + 1) & 1;
          semicolon++;
        }
      if ('\0' == semicolon[0])
        semicolon = NULL;
      if (NULL != semicolon)
        {
          semicolon[0] = '\0';
          semicolon++;
        }
      /* remove quotes */
      if ( ('"' == equals[0]) &&
           ('"' == equals[strlen (equals) - 1]) )
        {
          equals[strlen (equals) - 1] = '\0';
          equals++;
        }
      if (! request_add_header (request,
				pos,
				equals,
				MHD_COOKIE_KIND))
        return false;
      pos = semicolon;
    }
  return true;
}


/**
 * Parse the various headers; figure out the size
 * of the upload and make sure the headers follow
 * the protocol.  Advance to the appropriate state.
 *
 * @param request request we're processing
 */
static void
parse_request_headers (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Connection *connection = request->connection;
  const char *clen;
  struct MHD_Response *response;
  const char *enc;
  const char *end;

  parse_cookie_header (request); /* FIXME: return value ignored! */
  if ( (MHD_PSL_STRICT == daemon->protocol_strict_level) &&
       (NULL != request->version_s) &&
       (MHD_str_equal_caseless_(MHD_HTTP_VERSION_1_1,
                                request->version_s)) &&
       (NULL ==
        MHD_request_lookup_value (request,
				  MHD_HEADER_KIND,
				  MHD_HTTP_HEADER_HOST)) )
    {
      /* die, http 1.1 request without host and we are pedantic */
      request->state = MHD_REQUEST_FOOTERS_RECEIVED;
      connection->read_closed = true;
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_HOST_HEADER_MISSING,
                _("Received HTTP 1.1 request without `Host' header.\n"));
#endif
      mhd_assert (NULL == request->response);
      response =
        MHD_response_from_buffer (MHD_HTTP_BAD_REQUEST,
				  MHD_STATICSTR_LEN_ (REQUEST_LACKS_HOST),
				  REQUEST_LACKS_HOST,
				  MHD_RESPMEM_PERSISTENT);
      request->response = response;
      // FIXME: state machine advance?
      return;
    }

  request->remaining_upload_size = 0;
  enc = MHD_request_lookup_value (request,
				  MHD_HEADER_KIND,
				  MHD_HTTP_HEADER_TRANSFER_ENCODING);
  if (NULL != enc)
    {
      request->remaining_upload_size = MHD_SIZE_UNKNOWN;
      if (MHD_str_equal_caseless_ (enc,
				   "chunked"))
        request->have_chunked_upload = true;
      return;
    }
  clen = MHD_request_lookup_value (request,
				   MHD_HEADER_KIND,
				   MHD_HTTP_HEADER_CONTENT_LENGTH);
  if (NULL == clen)
    return;
  end = clen + MHD_str_to_uint64_ (clen,
				   &request->remaining_upload_size);
  if ( (clen == end) ||
       ('\0' != *end) )
    {
      request->remaining_upload_size = 0;
#ifdef HAVE_MESSAGES
      MHD_DLOG (request->daemon,
		MHD_SC_CONTENT_LENGTH_MALFORMED,
		"Failed to parse `Content-Length' header. Closing connection.\n");
#endif
      CONNECTION_CLOSE_ERROR (connection,
			      MHD_SC_CONTENT_LENGTH_MALFORMED,
			      NULL);
    }
}


/**
 * Call the handler of the application for this
 * request.
 *
 * @param request request we're processing
 */
static void
call_request_handler (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Connection *connection = request->connection;
  const struct MHD_Action *action;

  if (NULL != request->response)
    return;                     /* already queued a response */
  if (NULL == (action =
	       daemon->rc (daemon->rc_cls,
			   request,
			   request->url,
			   request->method)))
    {
      /* serious internal error, close connection */
      CONNECTION_CLOSE_ERROR (connection,
			      MHD_SC_APPLICATION_CALLBACK_FAILURE_CLOSED,
			      _("Application reported internal error, closing connection.\n"));
      return;
    }
  action->action (action->action_cls,
		  request);
}


/**
 * Call the handler of the application for this request.  Handles
 * chunking of the upload as well as normal uploads.
 *
 * @param request request we're processing
 */
static void
process_request_body (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Connection *connection = request->connection;
  size_t available;
  bool instant_retry;
  char *buffer_head;

  if (NULL != request->response)
    return;                     /* already queued a response */

  buffer_head = request->read_buffer;
  available = request->read_buffer_offset;
  do
    {
      size_t to_be_processed;
      size_t left_unprocessed;
      size_t processed_size;

      instant_retry = false;
      if ( (request->have_chunked_upload) &&
           (MHD_SIZE_UNKNOWN == request->remaining_upload_size) )
        {
          if ( (request->current_chunk_offset == request->current_chunk_size) &&
               (0LLU != request->current_chunk_offset) &&
               (available >= 2) )
            {
              size_t i;

              /* skip new line at the *end* of a chunk */
              i = 0;
              if ( ('\r' == buffer_head[i]) ||
                   ('\n' == buffer_head[i]) )
                i++;            /* skip 1st part of line feed */
              if ( ('\r' == buffer_head[i]) ||
                   ('\n' == buffer_head[i]) )
                i++;            /* skip 2nd part of line feed */
              if (0 == i)
                {
                  /* malformed encoding */
                  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CHUNKED_ENCODING_MALFORMED,
					  _("Received malformed HTTP request (bad chunked encoding). Closing connection.\n"));
                  return;
                }
              available -= i;
              buffer_head += i;
              request->current_chunk_offset = 0;
              request->current_chunk_size = 0;
            }
          if (request->current_chunk_offset <
              request->current_chunk_size)
            {
              uint64_t cur_chunk_left;

              /* we are in the middle of a chunk, give
                 as much as possible to the client (without
                 crossing chunk boundaries) */
              cur_chunk_left
                = request->current_chunk_size - request->current_chunk_offset;
              if (cur_chunk_left > available)
		{
		  to_be_processed = available;
		}
              else
                { /* cur_chunk_left <= (size_t)available */
                  to_be_processed = (size_t)cur_chunk_left;
                  if (available > to_be_processed)
                    instant_retry = true;
                }
            }
          else
            {
              size_t i;
              size_t end_size;
              bool malformed;

              /* we need to read chunk boundaries */
              i = 0;
              while (i < available)
                {
                  if ( ('\r' == buffer_head[i]) ||
                       ('\n' == buffer_head[i]) ||
		       (';' == buffer_head[i]) )
                    break;
                  i++;
                  if (i >= 16)
                    break;
                }
	      end_size = i;
	      /* find beginning of CRLF (skip over chunk extensions) */
	      if (';' == buffer_head[i])
		{
		  while (i < available)
		  {
		    if ( ('\r' == buffer_head[i]) ||
			 ('\n' == buffer_head[i]) )
		      break;
		    i++;
		  }
		}
              /* take '\n' into account; if '\n' is the unavailable
                 character, we will need to wait until we have it
                 before going further */
              if ( (i + 1 >= available) &&
                   ! ( (1 == i) &&
                       (2 == available) &&
                       ('0' == buffer_head[0]) ) )
                break;          /* need more data... */
	      i++;
              malformed = (end_size >= 16);
              if (! malformed)
                {
                  size_t num_dig = MHD_strx_to_uint64_n_ (buffer_head,
							  end_size,
							  &request->current_chunk_size);
                  malformed = (end_size != num_dig);
                }
              if (malformed)
                {
                  /* malformed encoding */
                  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CHUNKED_ENCODING_MALFORMED,
					  _("Received malformed HTTP request (bad chunked encoding). Closing connection.\n"));
                  return;
                }
	      /* skip 2nd part of line feed */
              if ( (i < available) &&
                   ( ('\r' == buffer_head[i]) ||
                     ('\n' == buffer_head[i]) ) )
                i++;

              buffer_head += i;
              available -= i;
              request->current_chunk_offset = 0;

              if (available > 0)
                instant_retry = true;
              if (0LLU == request->current_chunk_size)
                {
                  request->remaining_upload_size = 0;
                  break;
                }
              continue;
            }
        }
      else
        {
          /* no chunked encoding, give all to the client */
          if ( (0 != request->remaining_upload_size) &&
	       (MHD_SIZE_UNKNOWN != request->remaining_upload_size) &&
	       (request->remaining_upload_size < available) )
	    {
              to_be_processed = (size_t)request->remaining_upload_size;
	    }
          else
	    {
              /**
               * 1. no chunked encoding, give all to the client
               * 2. client may send large chunked data, but only a smaller part is available at one time.
               */
              to_be_processed = available;
	    }
        }
      left_unprocessed = to_be_processed;
#if FIXME_OLD_STYLE
      if (MHD_NO ==
          daemon->rc (daemon->rc_cls,
		      request,
		      request->url,
		      request->method,
		      request->version,
		      buffer_head,
		      &left_unprocessed,
		      &request->client_context))
        {
          /* serious internal error, close connection */
          CONNECTION_CLOSE_ERROR (connection,
				  MHD_SC_APPLICATION_CALLBACK_FAILURE_CLOSED,
                                  _("Application reported internal error, closing connection.\n"));
          return;
        }
#endif
      if (left_unprocessed > to_be_processed)
        mhd_panic (mhd_panic_cls,
                   __FILE__,
                   __LINE__
#ifdef HAVE_MESSAGES
		   , _("libmicrohttpd API violation")
#else
		   , NULL
#endif
		   );
      if (0 != left_unprocessed)
	{
	  instant_retry = false; /* client did not process everything */
#ifdef HAVE_MESSAGES
	  /* client did not process all upload data, complain if
	     the setup was incorrect, which may prevent us from
	     handling the rest of the request */
	  if ( (MHD_TM_EXTERNAL_EVENT_LOOP == daemon->threading_mode) &&
	       (! connection->suspended) )
	    MHD_DLOG (daemon,
		      MHD_SC_APPLICATION_HUNG_CONNECTION,
		      _("WARNING: incomplete upload processing and connection not suspended may result in hung connection.\n"));
#endif
	}
      processed_size = to_be_processed - left_unprocessed;
      if (request->have_chunked_upload)
        request->current_chunk_offset += processed_size;
      /* dh left "processed" bytes in buffer for next time... */
      buffer_head += processed_size;
      available -= processed_size;
      if (MHD_SIZE_UNKNOWN != request->remaining_upload_size)
        request->remaining_upload_size -= processed_size;
    }
  while (instant_retry);
  if (available > 0)
    memmove (request->read_buffer,
             buffer_head,
             available);
  request->read_buffer_offset = available;
}


/**
 * Clean up the state of the given connection and move it into the
 * clean up queue for final disposal.
 * @remark To be called only from thread that process connection's
 * recv(), send() and response.
 *
 * @param connection handle for the connection to clean up
 */
static void
cleanup_connection (struct MHD_Connection *connection)
{
  struct MHD_Daemon *daemon = connection->daemon;

  if (connection->request.in_cleanup)
    return; /* Prevent double cleanup. */
  connection->request.in_cleanup = true;
  if (NULL != connection->request.response)
    {
      MHD_response_queue_for_destroy (connection->request.response);
      connection->request.response = NULL;
    }
  MHD_mutex_lock_chk_ (&daemon->cleanup_connection_mutex);
  if (connection->suspended)
    {
      DLL_remove (daemon->suspended_connections_head,
                  daemon->suspended_connections_tail,
                  connection);
      connection->suspended = false;
    }
  else
    {
      if (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode)
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
      DLL_remove (daemon->connections_head,
                  daemon->connections_tail,
                  connection);
    }
  DLL_insert (daemon->cleanup_head,
	      daemon->cleanup_tail,
	      connection);
  connection->resuming = false;
  connection->request.in_idle = false;
  MHD_mutex_unlock_chk_ (&daemon->cleanup_connection_mutex);
  if (MHD_TM_THREAD_PER_CONNECTION == daemon->threading_mode)
    {
      /* if we were at the connection limit before and are in
         thread-per-connection mode, signal the main thread
         to resume accepting connections */
      if ( (MHD_ITC_IS_VALID_ (daemon->itc)) &&
           (! MHD_itc_activate_ (daemon->itc,
				 "c")) )
        {
#ifdef HAVE_MESSAGES
          MHD_DLOG (daemon,
		    MHD_SC_ITC_USE_FAILED,
                    _("Failed to signal end of connection via inter-thread communication channel"));
#endif
        }
    }
}


#ifdef EPOLL_SUPPORT
/**
 * Perform epoll() processing, possibly moving the connection back into
 * the epoll() set if needed.
 *
 * @param connection connection to process
 * @return true if we should continue to process the
 *         connection (not dead yet), false if it died
 */
static bool
connection_epoll_update_ (struct MHD_Connection *connection)
{
  struct MHD_Daemon *daemon = connection->daemon;

  if ( (MHD_ELS_EPOLL == daemon->event_loop_syscall) &&
       (0 == (connection->epoll_state & MHD_EPOLL_STATE_IN_EPOLL_SET)) &&
       (0 == (connection->epoll_state & MHD_EPOLL_STATE_SUSPENDED)) &&
       ( ( (MHD_EVENT_LOOP_INFO_WRITE == connection->request.event_loop_info) &&
           (0 == (connection->epoll_state & MHD_EPOLL_STATE_WRITE_READY))) ||
	 ( (MHD_EVENT_LOOP_INFO_READ == connection->request.event_loop_info) &&
	   (0 == (connection->epoll_state & MHD_EPOLL_STATE_READ_READY)) ) ) )
    {
      /* add to epoll set */
      struct epoll_event event;

      event.events = EPOLLIN | EPOLLOUT | EPOLLPRI | EPOLLET;
      event.data.ptr = connection;
      if (0 != epoll_ctl (daemon->epoll_fd,
			  EPOLL_CTL_ADD,
			  connection->socket_fd,
			  &event))
	{
#ifdef HAVE_MESSAGES
	  MHD_DLOG (daemon,
		    MHD_SC_EPOLL_CTL_ADD_FAILED,
		    _("Call to epoll_ctl failed: %s\n"),
		    MHD_socket_last_strerr_ ());
#endif
	  connection->request.state = MHD_REQUEST_CLOSED;
	  cleanup_connection (connection);
	  return false;
	}
      connection->epoll_state |= MHD_EPOLL_STATE_IN_EPOLL_SET;
    }
  return true;
}
#endif


/**
 * Update the 'event_loop_info' field of this connection based on the
 * state that the connection is now in.  May also close the connection
 * or perform other updates to the connection if needed to prepare for
 * the next round of the event loop.
 *
 * @param connection connection to get poll set for
 */
static void
connection_update_event_loop_info (struct MHD_Connection *connection)
{
  struct MHD_Daemon *daemon = connection->daemon;
  struct MHD_Request *request = &connection->request;

  /* Do not update states of suspended connection */
  if (connection->suspended)
    return; /* States will be updated after resume. */
#ifdef HTTPS_SUPPORT
  {
    struct MHD_TLS_Plugin *tls;

    if ( (NULL != (tls = daemon->tls_api)) &&
	 (tls->update_event_loop_info (tls->cls,
				       connection->tls_cs,
				       &request->event_loop_info)) )
      return; /* TLS has decided what to do */
  }
#endif /* HTTPS_SUPPORT */
  while (1)
    {
#if DEBUG_STATES
      MHD_DLOG (daemon,
		MHD_SC_STATE_MACHINE_STATUS_REPORT,
                _("In function %s handling connection at state: %s\n"),
                __FUNCTION__,
                MHD_state_to_string (request->state));
#endif
      switch (request->state)
        {
        case MHD_REQUEST_INIT:
        case MHD_REQUEST_URL_RECEIVED:
        case MHD_REQUEST_HEADER_PART_RECEIVED:
          /* while reading headers, we always grow the
             read buffer if needed, no size-check required */
          if ( (request->read_buffer_offset == request->read_buffer_size) &&
	       (! try_grow_read_buffer (request)) )
            {
              transmit_error_response (request,
				       MHD_SC_CLIENT_HEADER_TOO_BIG,
                                       (NULL != request->url)
                                       ? MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE
                                       : MHD_HTTP_URI_TOO_LONG,
                                       REQUEST_TOO_BIG);
              continue;
            }
	  if (! connection->read_closed)
	    request->event_loop_info = MHD_EVENT_LOOP_INFO_READ;
	  else
	    request->event_loop_info = MHD_EVENT_LOOP_INFO_BLOCK;
          break;
        case MHD_REQUEST_HEADERS_RECEIVED:
          mhd_assert (0);
          break;
        case MHD_REQUEST_HEADERS_PROCESSED:
          mhd_assert (0);
          break;
        case MHD_REQUEST_CONTINUE_SENDING:
          request->event_loop_info = MHD_EVENT_LOOP_INFO_WRITE;
          break;
        case MHD_REQUEST_CONTINUE_SENT:
          if (request->read_buffer_offset == request->read_buffer_size)
            {
              if ( (! try_grow_read_buffer (request)) &&
		   (MHD_TM_EXTERNAL_EVENT_LOOP != daemon->threading_mode) )
                {
                  /* failed to grow the read buffer, and the client
                     which is supposed to handle the received data in
                     a *blocking* fashion (in this mode) did not
                     handle the data as it was supposed to!

                     => we would either have to do busy-waiting
                     (on the client, which would likely fail),
                     or if we do nothing, we would just timeout
                     on the connection (if a timeout is even set!).

                     Solution: we kill the connection with an error */
                  transmit_error_response (request,
					   MHD_SC_APPLICATION_HUNG_CONNECTION_CLOSED,
                                           MHD_HTTP_INTERNAL_SERVER_ERROR,
                                           INTERNAL_ERROR);
                  continue;
                }
            }
          if ( (request->read_buffer_offset < request->read_buffer_size) &&
	       (! connection->read_closed) )
	    request->event_loop_info = MHD_EVENT_LOOP_INFO_READ;
	  else
	    request->event_loop_info = MHD_EVENT_LOOP_INFO_BLOCK;
          break;
        case MHD_REQUEST_BODY_RECEIVED:
        case MHD_REQUEST_FOOTER_PART_RECEIVED:
          /* while reading footers, we always grow the
             read buffer if needed, no size-check required */
          if (connection->read_closed)
            {
	      CONNECTION_CLOSE_ERROR (connection,
				      MHD_SC_CONNECTION_READ_FAIL_CLOSED,
				      NULL);
              continue;
            }
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_READ;
          /* transition to FOOTERS_RECEIVED
             happens in read handler */
          break;
        case MHD_REQUEST_FOOTERS_RECEIVED:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_BLOCK;
          break;
        case MHD_REQUEST_HEADERS_SENDING:
          /* headers in buffer, keep writing */
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_WRITE;
          break;
        case MHD_REQUEST_HEADERS_SENT:
          mhd_assert (0);
          break;
        case MHD_REQUEST_NORMAL_BODY_READY:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_WRITE;
          break;
        case MHD_REQUEST_NORMAL_BODY_UNREADY:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_BLOCK;
          break;
        case MHD_REQUEST_CHUNKED_BODY_READY:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_WRITE;
          break;
        case MHD_REQUEST_CHUNKED_BODY_UNREADY:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_BLOCK;
          break;
        case MHD_REQUEST_BODY_SENT:
          mhd_assert (0);
          break;
        case MHD_REQUEST_FOOTERS_SENDING:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_WRITE;
          break;
        case MHD_REQUEST_FOOTERS_SENT:
          mhd_assert (0);
          break;
        case MHD_REQUEST_CLOSED:
	  request->event_loop_info = MHD_EVENT_LOOP_INFO_CLEANUP;
          return;       /* do nothing, not even reading */
#ifdef UPGRADE_SUPPORT
        case MHD_REQUEST_UPGRADE:
          mhd_assert (0);
          break;
#endif /* UPGRADE_SUPPORT */
        default:
          mhd_assert (0);
        }
      break;
    }
}


/**
 * This function was created to handle per-request processing that
 * has to happen even if the socket cannot be read or written to.
 * @remark To be called only from thread that process request's
 * recv(), send() and response.
 *
 * @param request the request to handle
 * @return true if we should continue to process the
 *         request (not dead yet), false if it died
 */
bool
MHD_request_handle_idle_ (struct MHD_Request *request)
{
  struct MHD_Daemon *daemon = request->daemon;
  struct MHD_Connection *connection = request->connection;
  char *line;
  size_t line_len;
  bool ret;

  request->in_idle = true;
  while (! connection->suspended)
    {
#ifdef HTTPS_SUPPORT
      struct MHD_TLS_Plugin *tls;

      if ( (NULL != (tls = daemon->tls_api)) &&
	   (! tls->idle_ready (tls->cls,
			       connection->tls_cs)) )
	break;
#endif /* HTTPS_SUPPORT */
#if DEBUG_STATES
      MHD_DLOG (daemon,
		MHD_SC_STATE_MACHINE_STATUS_REPORT,
                _("In function %s handling connection at state: %s\n"),
                __FUNCTION__,
                MHD_state_to_string (request->state));
#endif
      switch (request->state)
        {
        case MHD_REQUEST_INIT:
          line = get_next_header_line (request,
                                       &line_len);
          /* Check for empty string, as we might want
             to tolerate 'spurious' empty lines; also
             NULL means we didn't get a full line yet;
             line is not 0-terminated here. */
          if ( (NULL == line) ||
               (0 == line[0]) )
            {
              if (MHD_REQUEST_INIT != request->state)
                continue;
              if (connection->read_closed)
                {
		  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CONNECTION_READ_FAIL_CLOSED,
					  NULL);
                  continue;
                }
              break;
            }
          if (MHD_NO ==
	      parse_initial_message_line (request,
					  line,
					  line_len))
            CONNECTION_CLOSE_ERROR (connection,
				    MHD_SC_CONNECTION_CLOSED,
                                    NULL);
          else
            request->state = MHD_REQUEST_URL_RECEIVED;
          continue;
        case MHD_REQUEST_URL_RECEIVED:
          line = get_next_header_line (request,
                                       NULL);
          if (NULL == line)
            {
              if (MHD_REQUEST_URL_RECEIVED != request->state)
                continue;
              if (connection->read_closed)
                {
		  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CONNECTION_READ_FAIL_CLOSED,
					  NULL);
                  continue;
                }
              break;
            }
          if (0 == line[0])
            {
              request->state = MHD_REQUEST_HEADERS_RECEIVED;
              request->header_size = (size_t) (line - request->read_buffer);
              continue;
            }
          if (! process_header_line (request,
				     line))
            {
              transmit_error_response (request,
				       MHD_SC_CONNECTION_PARSE_FAIL_CLOSED,
                                       MHD_HTTP_BAD_REQUEST,
                                       REQUEST_MALFORMED);
              break;
            }
          request->state = MHD_REQUEST_HEADER_PART_RECEIVED;
          continue;
        case MHD_REQUEST_HEADER_PART_RECEIVED:
          line = get_next_header_line (request,
                                       NULL);
          if (NULL == line)
            {
              if (request->state != MHD_REQUEST_HEADER_PART_RECEIVED)
                continue;
              if (connection->read_closed)
                {
		  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CONNECTION_READ_FAIL_CLOSED,
					  NULL);
                  continue;
                }
              break;
            }
          if (MHD_NO ==
              process_broken_line (request,
                                   line,
                                   MHD_HEADER_KIND))
            continue;
          if (0 == line[0])
            {
              request->state = MHD_REQUEST_HEADERS_RECEIVED;
              request->header_size = (size_t) (line - request->read_buffer);
              continue;
            }
          continue;
        case MHD_REQUEST_HEADERS_RECEIVED:
          parse_request_headers (request);
          if (MHD_REQUEST_CLOSED == request->state)
            continue;
          request->state = MHD_REQUEST_HEADERS_PROCESSED;
          if (connection->suspended)
            break;
          continue;
        case MHD_REQUEST_HEADERS_PROCESSED:
          call_request_handler (request); /* first call */
          if (MHD_REQUEST_CLOSED == request->state)
            continue;
          if (need_100_continue (request))
            {
              request->state = MHD_REQUEST_CONTINUE_SENDING;
              if (socket_flush_possible (connection))
                socket_start_extra_buffering (connection);
              else
                socket_start_no_buffering (connection);
              break;
            }
          if ( (NULL != request->response) &&
	       ( (MHD_METHOD_POST == request->method) ||
		 (MHD_METHOD_PUT == request->method) ) )
            {
              /* we refused (no upload allowed!) */
              request->remaining_upload_size = 0;
              /* force close, in case client still tries to upload... */
              connection->read_closed = true;
            }
          request->state = (0 == request->remaining_upload_size)
            ? MHD_REQUEST_FOOTERS_RECEIVED
	    : MHD_REQUEST_CONTINUE_SENT;
          if (connection->suspended)
            break;
          continue;
        case MHD_REQUEST_CONTINUE_SENDING:
          if (request->continue_message_write_offset ==
              MHD_STATICSTR_LEN_ (HTTP_100_CONTINUE))
            {
              request->state = MHD_REQUEST_CONTINUE_SENT;
              if (! socket_flush_possible (connection))
                socket_start_no_buffering_flush (connection);
              else
                socket_start_normal_buffering (connection);
              continue;
            }
          break;
        case MHD_REQUEST_CONTINUE_SENT:
          if (0 != request->read_buffer_offset)
            {
              process_request_body (request);     /* loop call */
              if (MHD_REQUEST_CLOSED == request->state)
                continue;
            }
          if ( (0 == request->remaining_upload_size) ||
               ( (MHD_SIZE_UNKNOWN == request->remaining_upload_size) &&
                 (0 == request->read_buffer_offset) &&
                 (connection->read_closed) ) )
            {
              if ( (request->have_chunked_upload) &&
                   (! connection->read_closed) )
                request->state = MHD_REQUEST_BODY_RECEIVED;
              else
                request->state = MHD_REQUEST_FOOTERS_RECEIVED;
              if (connection->suspended)
                break;
              continue;
            }
          break;
        case MHD_REQUEST_BODY_RECEIVED:
          line = get_next_header_line (request,
                                       NULL);
          if (NULL == line)
            {
              if (request->state != MHD_REQUEST_BODY_RECEIVED)
                continue;
              if (connection->read_closed)
                {
		  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CONNECTION_CLOSED,
					  NULL);
                  continue;
                }
              break;
            }
          if (0 == line[0])
            {
              request->state = MHD_REQUEST_FOOTERS_RECEIVED;
              if (connection->suspended)
                break;
              continue;
            }
          if (MHD_NO == process_header_line (request,
                                             line))
            {
              transmit_error_response (request,
				       MHD_SC_CONNECTION_PARSE_FAIL_CLOSED,
                                       MHD_HTTP_BAD_REQUEST,
                                       REQUEST_MALFORMED);
              break;
            }
          request->state = MHD_REQUEST_FOOTER_PART_RECEIVED;
          continue;
        case MHD_REQUEST_FOOTER_PART_RECEIVED:
          line = get_next_header_line (request,
                                       NULL);
          if (NULL == line)
            {
              if (request->state != MHD_REQUEST_FOOTER_PART_RECEIVED)
                continue;
              if (connection->read_closed)
                {
		  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CONNECTION_CLOSED,
					  NULL);
                  continue;
                }
              break;
            }
          if (MHD_NO ==
              process_broken_line (request,
                                   line,
                                   MHD_FOOTER_KIND))
            continue;
          if (0 == line[0])
            {
              request->state = MHD_REQUEST_FOOTERS_RECEIVED;
              if (connection->suspended)
                break;
              continue;
            }
          continue;
        case MHD_REQUEST_FOOTERS_RECEIVED:
          call_request_handler (request); /* "final" call */
          if (request->state == MHD_REQUEST_CLOSED)
            continue;
          if (NULL == request->response)
            break;              /* try again next time */
          if (! build_header_response (request))
            {
              /* oops - close! */
	      CONNECTION_CLOSE_ERROR (connection,
				      MHD_SC_FAILED_RESPONSE_HEADER_GENERATION,
				      _("Closing connection (failed to create response header)\n"));
              continue;
            }
          request->state = MHD_REQUEST_HEADERS_SENDING;
          if (MHD_NO != socket_flush_possible (connection))
            socket_start_extra_buffering (connection);
          else
            socket_start_no_buffering (connection);

          break;
        case MHD_REQUEST_HEADERS_SENDING:
          /* no default action */
          break;
        case MHD_REQUEST_HEADERS_SENT:
          /* Some clients may take some actions right after header receive */
          if (MHD_NO != socket_flush_possible (connection))
            socket_start_no_buffering_flush (connection);

#ifdef UPGRADE_SUPPORT
          if (NULL != request->response->upgrade_handler)
            {
              socket_start_normal_buffering (connection);
              request->state = MHD_REQUEST_UPGRADE;
#if FIXME_LEGACY_STYLE
              /* This request is "upgraded".  Pass socket to application. */
              if (! MHD_response_execute_upgrade_ (request->response,
						   request))
                {
                  /* upgrade failed, fail hard */
                  CONNECTION_CLOSE_ERROR (connection,
					  MHD_SC_CONNECTION_CLOSED,
                                          NULL);
                  continue;
                }
#endif
              /* Response is not required anymore for this request. */
              {
                struct MHD_Response * const resp = request->response;

                request->response = NULL;
                MHD_response_queue_for_destroy (resp);
              }
              continue;
            }
#endif /* UPGRADE_SUPPORT */
          if (MHD_NO != socket_flush_possible (connection))
            socket_start_extra_buffering (connection);
          else
            socket_start_normal_buffering (connection);

          if (request->have_chunked_upload)
            request->state = MHD_REQUEST_CHUNKED_BODY_UNREADY;
          else
            request->state = MHD_REQUEST_NORMAL_BODY_UNREADY;
          continue;
        case MHD_REQUEST_NORMAL_BODY_READY:
          /* nothing to do here */
          break;
        case MHD_REQUEST_NORMAL_BODY_UNREADY:
          if (NULL != request->response->crc)
            MHD_mutex_lock_chk_ (&request->response->mutex);
          if (0 == request->response->total_size)
            {
              if (NULL != request->response->crc)
                MHD_mutex_unlock_chk_ (&request->response->mutex);
              request->state = MHD_REQUEST_BODY_SENT;
              continue;
            }
          if (try_ready_normal_body (request))
            {
	      if (NULL != request->response->crc)
	        MHD_mutex_unlock_chk_ (&request->response->mutex);
              request->state = MHD_REQUEST_NORMAL_BODY_READY;
              /* Buffering for flushable socket was already enabled*/
              if (MHD_NO == socket_flush_possible (connection))
                socket_start_no_buffering (connection);
              break;
            }
          /* mutex was already unlocked by "try_ready_normal_body */
          /* not ready, no socket action */
          break;
        case MHD_REQUEST_CHUNKED_BODY_READY:
          /* nothing to do here */
          break;
        case MHD_REQUEST_CHUNKED_BODY_UNREADY:
          if (NULL != request->response->crc)
            MHD_mutex_lock_chk_ (&request->response->mutex);
          if ( (0 == request->response->total_size) ||
               (request->response_write_position ==
                request->response->total_size) )
            {
              if (NULL != request->response->crc)
                MHD_mutex_unlock_chk_ (&request->response->mutex);
              request->state = MHD_REQUEST_BODY_SENT;
              continue;
            }
          if (try_ready_chunked_body (request))
            {
              if (NULL != request->response->crc)
                MHD_mutex_unlock_chk_ (&request->response->mutex);
              request->state = MHD_REQUEST_CHUNKED_BODY_READY;
              /* Buffering for flushable socket was already enabled */
              if (MHD_NO == socket_flush_possible (connection))
                socket_start_no_buffering (connection);
              continue;
            }
          /* mutex was already unlocked by try_ready_chunked_body */
          break;
        case MHD_REQUEST_BODY_SENT:
          if (! build_header_response (request))
            {
              /* oops - close! */
	      CONNECTION_CLOSE_ERROR (connection,
				      MHD_SC_FAILED_RESPONSE_HEADER_GENERATION,
				      _("Closing connection (failed to create response header)\n"));
              continue;
            }
          if ( (! request->have_chunked_upload) ||
               (request->write_buffer_send_offset ==
                request->write_buffer_append_offset) )
            request->state = MHD_REQUEST_FOOTERS_SENT;
          else
            request->state = MHD_REQUEST_FOOTERS_SENDING;
          continue;
        case MHD_REQUEST_FOOTERS_SENDING:
          /* no default action */
          break;
        case MHD_REQUEST_FOOTERS_SENT:
	  {
	    struct MHD_Response *response = request->response;

	    if (MHD_HTTP_PROCESSING == response->status_code)
	      {
		/* After this type of response, we allow sending another! */
		request->state = MHD_REQUEST_HEADERS_PROCESSED;
		MHD_response_queue_for_destroy (response);
		request->response = NULL;
		/* FIXME: maybe partially reset memory pool? */
		continue;
	      }
	    if (socket_flush_possible (connection))
	      socket_start_no_buffering_flush (connection);
	    else
	      socket_start_normal_buffering (connection);

	    if (NULL != response->termination_cb)
	      {
		response->termination_cb (response->termination_cb_cls,
					  MHD_REQUEST_TERMINATED_COMPLETED_OK,
					  request->client_context);
	      }
	    MHD_response_queue_for_destroy (response);
	    request->response = NULL;
	  }
	  if ( (MHD_CONN_USE_KEEPALIVE != request->keepalive) ||
               (connection->read_closed) )
            {
              /* have to close for some reason */
              MHD_connection_close_ (connection,
                                     MHD_REQUEST_TERMINATED_COMPLETED_OK);
              MHD_pool_destroy (connection->pool);
              connection->pool = NULL;
              request->read_buffer = NULL;
              request->read_buffer_size = 0;
              request->read_buffer_offset = 0;
            }
          else
            {
              /* can try to keep-alive */
              if (socket_flush_possible (connection))
                socket_start_normal_buffering (connection);
              request->version_s = NULL;
              request->state = MHD_REQUEST_INIT;
              request->last = NULL;
              request->colon = NULL;
              request->header_size = 0;
              request->keepalive = MHD_CONN_KEEPALIVE_UNKOWN;
              /* Reset the read buffer to the starting size,
                 preserving the bytes we have already read. */
              request->read_buffer
                = MHD_pool_reset (connection->pool,
                                  request->read_buffer,
                                  request->read_buffer_offset,
                                  daemon->connection_memory_limit_b / 2);
              request->read_buffer_size
                = daemon->connection_memory_limit_b / 2;
            }
	  // FIXME: this is too much, NULLs out some of the things
	  // initialized above...
	  memset (request,
		  0,
		  sizeof (struct MHD_Request));
	  request->daemon = daemon;
	  request->connection = connection;
          continue;
        case MHD_REQUEST_CLOSED:
	  cleanup_connection (connection);
          request->in_idle = false;
	  return false;
#ifdef UPGRADE_SUPPORT
	case MHD_REQUEST_UPGRADE:
          request->in_idle = false;
          return true; /* keep open */
#endif /* UPGRADE_SUPPORT */
       default:
          mhd_assert (0);
          break;
        }
      break;
    }
  if (! connection->suspended)
    {
      time_t timeout;
      timeout = connection->connection_timeout;
      if ( (0 != timeout) &&
           (timeout < (MHD_monotonic_sec_counter() - connection->last_activity)) )
        {
          MHD_connection_close_ (connection,
                                 MHD_REQUEST_TERMINATED_TIMEOUT_REACHED);
          request->in_idle = false;
          return true;
        }
    }
  connection_update_event_loop_info (connection);
  ret = true;
#ifdef EPOLL_SUPPORT
  if ( (! connection->suspended) &&
       (MHD_ELS_EPOLL == daemon->event_loop_syscall) )
    {
      ret = connection_epoll_update_ (connection);
    }
#endif /* EPOLL_SUPPORT */
  request->in_idle = false;
  return ret;
}


/**
 * Call the handlers for a connection in the appropriate order based
 * on the readiness as detected by the event loop.
 *
 * @param con connection to handle
 * @param read_ready set if the socket is ready for reading
 * @param write_ready set if the socket is ready for writing
 * @param force_close set if a hard error was detected on the socket;
 *        if this information is not available, simply pass #MHD_NO
 * @return #MHD_YES to continue normally,
 *         #MHD_NO if a serious error was encountered and the
 *         connection is to be closed.
 */
// FIXME: rename connection->request?
int
MHD_connection_call_handlers_ (struct MHD_Connection *con,
			       bool read_ready,
			       bool write_ready,
			       bool force_close)
{
  struct MHD_Daemon *daemon = con->daemon;
  int ret;
  bool states_info_processed = false;
  /* Fast track flag */
  bool on_fasttrack = (con->request.state == MHD_REQUEST_INIT);

#ifdef HTTPS_SUPPORT
  if (con->tls_read_ready)
    read_ready = true;
#endif /* HTTPS_SUPPORT */
  if (! force_close)
    {
      if ( (MHD_EVENT_LOOP_INFO_READ ==
	    con->request.event_loop_info) &&
	   read_ready)
        {
          MHD_request_handle_read_ (&con->request);
          ret = MHD_request_handle_idle_ (&con->request);
          states_info_processed = true;
        }
      /* No need to check value of 'ret' here as closed connection
       * cannot be in MHD_EVENT_LOOP_INFO_WRITE state. */
      if ( (MHD_EVENT_LOOP_INFO_WRITE ==
	    con->request.event_loop_info) &&
	   write_ready)
        {
          MHD_request_handle_write_ (&con->request);
          ret = MHD_request_handle_idle_ (&con->request);
          states_info_processed = true;
        }
    }
  else
    {
      MHD_connection_close_ (con,
                             MHD_REQUEST_TERMINATED_WITH_ERROR);
      return MHD_request_handle_idle_ (&con->request);
    }

  if (! states_info_processed)
    { /* Connection is not read or write ready, but external conditions
       * may be changed and need to be processed. */
      ret = MHD_request_handle_idle_ (&con->request);
    }
  /* Fast track for fast connections. */
  /* If full request was read by single read_handler() invocation
     and headers were completely prepared by single MHD_request_handle_idle_()
     then try not to wait for next sockets polling and send response
     immediately.
     As writeability of socket was not checked and it may have
     some data pending in system buffers, use this optimization
     only for non-blocking sockets. */
  /* No need to check 'ret' as connection is always in
   * MHD_CONNECTION_CLOSED state if 'ret' is equal 'MHD_NO'. */
  else if (on_fasttrack &&
	   con->sk_nonblck)
    {
      if (MHD_REQUEST_HEADERS_SENDING == con->request.state)
        {
          MHD_request_handle_write_ (&con->request);
          /* Always call 'MHD_request_handle_idle_()' after each read/write. */
          ret = MHD_request_handle_idle_ (&con->request);
        }
      /* If all headers were sent by single write_handler() and
       * response body is prepared by single MHD_request_handle_idle_()
       * call - continue. */
      if ((MHD_REQUEST_NORMAL_BODY_READY == con->request.state) ||
          (MHD_REQUEST_CHUNKED_BODY_READY == con->request.state))
        {
          MHD_request_handle_write_ (&con->request);
          ret = MHD_request_handle_idle_ (&con->request);
        }
    }

  /* All connection's data and states are processed for this turn.
   * If connection already has more data to be processed - use
   * zero timeout for next select()/poll(). */
  /* Thread-per-connection do not need global zero timeout as
   * connections are processed individually. */
  /* Note: no need to check for read buffer availability for
   * TLS read-ready connection in 'read info' state as connection
   * without space in read buffer will be market as 'info block'. */
  if ( (! daemon->data_already_pending) &&
       (MHD_TM_THREAD_PER_CONNECTION != daemon->threading_mode) )
    {
      if (MHD_EVENT_LOOP_INFO_BLOCK ==
	  con->request.event_loop_info)
        daemon->data_already_pending = true;
#ifdef HTTPS_SUPPORT
      else if ( (con->tls_read_ready) &&
                (MHD_EVENT_LOOP_INFO_READ ==
		 con->request.event_loop_info) )
        daemon->data_already_pending = true;
#endif /* HTTPS_SUPPORT */
    }
  return ret;
}

/* end of connection_call_handlers.c */
