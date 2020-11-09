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
 * @file lib/action_from_response.c
 * @brief implementation of #MHD_action_from_response()
 * @author Christian Grothoff
 */
#include "internal.h"
#include "connection_call_handlers.h"


/**
 * A response was given as the desired action for a @a request.
 * Queue the response for the request.
 *
 * @param cls the `struct MHD_Response`
 * @param request the request we are processing
 * @return #MHD_SC_OK on success
 */
static enum MHD_StatusCode
response_action (void *cls,
		 struct MHD_Request *request)
{
  struct MHD_Response *response = cls;
  struct MHD_Daemon *daemon = request->daemon;

  /* If daemon was shut down in parallel,
   * response will be aborted now or on later stage. */
  if (daemon->shutdown)
    return MHD_SC_DAEMON_ALREADY_SHUTDOWN;

#ifdef UPGRADE_SUPPORT
  if ( (NULL != response->upgrade_handler) &&
       daemon->disallow_upgrade )
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_UPGRADE_ON_DAEMON_WITH_UPGRADE_DISALLOWED,
                _("Attempted 'upgrade' connection on daemon without MHD_ALLOW_UPGRADE option!\n"));
#endif
      return MHD_SC_UPGRADE_ON_DAEMON_WITH_UPGRADE_DISALLOWED;
    }
#endif /* UPGRADE_SUPPORT */
  request->response = response;
#if defined(_MHD_HAVE_SENDFILE)
  if ( (-1 == response->fd)
#if HTTPS_SUPPORT
       || (NULL != daemon->tls_api)
#endif
       )
    request->resp_sender = MHD_resp_sender_std;
  else
    request->resp_sender = MHD_resp_sender_sendfile;
#endif /* _MHD_HAVE_SENDFILE */

  if ( (MHD_METHOD_HEAD == request->method) ||
       (MHD_HTTP_OK > response->status_code) ||
       (MHD_HTTP_NO_CONTENT == response->status_code) ||
       (MHD_HTTP_NOT_MODIFIED == response->status_code) )
    {
      /* if this is a "HEAD" request, or a status code for
         which a body is not allowed, pretend that we
         have already sent the full message body. */
      request->response_write_position = response->total_size;
    }
  if ( (MHD_REQUEST_HEADERS_PROCESSED == request->state) &&
       ( (MHD_METHOD_POST == request->method) ||
	 (MHD_METHOD_PUT == request->method) ) )
    {
      /* response was queued "early", refuse to read body / footers or
         further requests! */
      request->connection->read_closed = true;
      request->state = MHD_REQUEST_FOOTERS_RECEIVED;
    }
  if (! request->in_idle)
    (void) MHD_request_handle_idle_ (request);
  return MHD_SC_OK;
}


/**
 * Converts a @a response to an action.  If @a consume
 * is set, the reference to the @a response is consumed
 * by the conversion. If @a consume is #MHD_NO, then
 * the response can be converted to actions in the future.
 * However, the @a response is frozen by this step and
 * must no longer be modified (i.e. by setting headers).
 *
 * @param response response to convert, not NULL
 * @param destroy_after_use should the response object be consumed?
 * @return corresponding action, never returns NULL
 *
 * Implementation note: internally, this is largely just
 * a cast (and possibly an RC increment operation),
 * as a response *is* an action.  As no memory is
 * allocated, this operation cannot fail.
 */
_MHD_EXTERN const struct MHD_Action *
MHD_action_from_response (struct MHD_Response *response,
			  enum MHD_Bool destroy_after_use)
{
  response->action.action = &response_action;
  response->action.action_cls = response;
  if (! destroy_after_use)
    {
      MHD_mutex_lock_chk_ (&response->mutex);
      response->reference_count++;
      MHD_mutex_unlock_chk_ (&response->mutex);
    }
  return &response->action;
}

/* end of action_from_response */
