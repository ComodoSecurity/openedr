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
 * @file lib/connection_info.c
 * @brief implementation of MHD_connection_get_information_sz()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Obtain information about the given connection.
 * Use wrapper macro #MHD_connection_get_information() instead of direct use
 * of this function.
 *
 * @param connection what connection to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @param return_value_size size of union MHD_ConnectionInformation at compile
 *                          time
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
enum MHD_Bool
MHD_connection_get_information_sz (struct MHD_Connection *connection,
				   enum MHD_ConnectionInformationType info_type,
				   union MHD_ConnectionInformation *return_value,
				   size_t return_value_size)
{
#define CHECK_SIZE(type) if (sizeof(type) < return_value_size) \
    return MHD_NO
  
  switch (info_type)
  {
#ifdef HTTPS_SUPPORT
  case MHD_CONNECTION_INFORMATION_CIPHER_ALGO:
    CHECK_SIZE (int);
    if (NULL == connection->tls_cs)
      return MHD_NO;
    // return_value->cipher_algorithm
    //  = gnutls_cipher_get (connection->tls_session);
    return MHD_NO; // FIXME: to be implemented
  case MHD_CONNECTION_INFORMATION_PROTOCOL:
    CHECK_SIZE (int);
    if (NULL == connection->tls_cs)
      return MHD_NO;
    //return_value->protocol
    //  = gnutls_protocol_get_version (connection->tls_session);
    return MHD_NO; // FIXME: to be implemented
  case MHD_CONNECTION_INFORMATION_GNUTLS_SESSION:
    CHECK_SIZE (void *);
    if (NULL == connection->tls_cs)
      return MHD_NO;
    // return_value->tls_session = connection->tls_session;
    return MHD_NO; // FIXME: to be implemented
#endif /* HTTPS_SUPPORT */
  case MHD_CONNECTION_INFORMATION_CLIENT_ADDRESS:
    CHECK_SIZE (struct sockaddr *);
    return_value->client_addr
      = (const struct sockaddr *) &connection->addr;
    return MHD_YES;
  case MHD_CONNECTION_INFORMATION_DAEMON:
    CHECK_SIZE (struct MHD_Daemon *);
    return_value->daemon = connection->daemon;
    return MHD_YES;
  case MHD_CONNECTION_INFORMATION_CONNECTION_FD:
    CHECK_SIZE (MHD_socket);
    return_value->connect_fd = connection->socket_fd;
    return MHD_YES;
  case MHD_CONNECTION_INFORMATION_SOCKET_CONTEXT:
    CHECK_SIZE (void **);
    return_value->socket_context = &connection->socket_context;
    return MHD_YES;
  case MHD_CONNECTION_INFORMATION_CONNECTION_SUSPENDED:
    CHECK_SIZE (enum MHD_Bool);
    return_value->suspended
      = connection->suspended ? MHD_YES : MHD_NO;
    return MHD_YES;
  case MHD_CONNECTION_INFORMATION_CONNECTION_TIMEOUT:
    CHECK_SIZE (unsigned int);
    return_value->connection_timeout
      = (unsigned int) connection->connection_timeout;
    return MHD_YES;
  default:
    return MHD_NO;
  }
  
#undef CHECK_SIZE
}

/* end of connection_info.c */
