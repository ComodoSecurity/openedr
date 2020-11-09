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
 * @file lib/response_for_upgrade.c
 * @brief implementation of MHD_response_for_upgrade()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Create a response object that can be used for 101 UPGRADE
 * responses, for example to implement WebSockets.  After sending the
 * response, control over the data stream is given to the callback (which
 * can then, for example, start some bi-directional communication).
 * If the response is queued for multiple connections, the callback
 * will be called for each connection.  The callback
 * will ONLY be called after the response header was successfully passed
 * to the OS; if there are communication errors before, the usual MHD
 * connection error handling code will be performed.
 *
 * MHD will automatically set the correct HTTP status
 * code (#MHD_HTTP_SWITCHING_PROTOCOLS).
 * Setting correct HTTP headers for the upgrade must be done
 * manually (this way, it is possible to implement most existing
 * WebSocket versions using this API; in fact, this API might be useful
 * for any protocol switch, not just WebSockets).  Note that
 * draft-ietf-hybi-thewebsocketprotocol-00 cannot be implemented this
 * way as the header "HTTP/1.1 101 WebSocket Protocol Handshake"
 * cannot be generated; instead, MHD will always produce "HTTP/1.1 101
 * Switching Protocols" (if the response code 101 is used).
 *
 * As usual, the response object can be extended with header
 * information and then be used any number of times (as long as the
 * header information is not connection-specific).
 *
 * @param upgrade_handler function to call with the "upgraded" socket
 * @param upgrade_handler_cls closure for @a upgrade_handler
 * @return NULL on error (i.e. invalid arguments, out of memory)
 */
struct MHD_Response *
MHD_response_for_upgrade (MHD_UpgradeHandler upgrade_handler,
			  void *upgrade_handler_cls)
{
#ifdef UPGRADE_SUPPORT
  struct MHD_Response *response;

  mhd_assert (NULL != upgrade_handler);
  response = MHD_calloc_ (1,
			  sizeof (struct MHD_Response));
  if (NULL == response)
    return NULL;
  if (! MHD_mutex_init_ (&response->mutex))
    {
      free (response);
      return NULL;
    }
  response->upgrade_handler = upgrade_handler;
  response->upgrade_handler_cls = upgrade_handler_cls;
  response->status_code = MHD_HTTP_SWITCHING_PROTOCOLS;
  response->total_size = MHD_SIZE_UNKNOWN;
  response->reference_count = 1;
  if (MHD_NO ==
      MHD_response_add_header (response,
                               MHD_HTTP_HEADER_CONNECTION,
                               "Upgrade"))
    {
      MHD_response_queue_for_destroy (response);
      return NULL;
    }
  return response;
#else
  return NULL;
#endif
}

/* end of response_for_upgrade.c */
