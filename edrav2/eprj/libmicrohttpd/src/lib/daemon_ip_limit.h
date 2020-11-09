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
 * @file lib/daemon_ip_limit.h
 * @brief counting of connections per IP
 * @author Christian Grothoff
 */

#ifndef DAEMON_IP_LIMIT_H
#define DAEMON_IP_LIMIT_H

/**
 * Check if IP address is over its limit in terms of the number
 * of allowed concurrent connections.  If the IP is still allowed,
 * increments the connection counter.
 *
 * @param daemon handle to daemon where connection counts are tracked
 * @param addr address to add (or increment counter)
 * @param addrlen number of bytes in @a addr
 * @return Return #MHD_YES if IP below limit, #MHD_NO if IP has surpassed limit.
 *   Also returns #MHD_NO if fails to allocate memory.
 */
int
MHD_ip_limit_add (struct MHD_Daemon *daemon,
		  const struct sockaddr *addr,
		  socklen_t addrlen)
  MHD_NONNULL (1,2);


/**
 * Decrement connection count for IP address, removing from table
 * count reaches 0.
 *
 * @param daemon handle to daemon where connection counts are tracked
 * @param addr address to remove (or decrement counter)
 * @param addrlen number of bytes in @a addr
 */
void
MHD_ip_limit_del (struct MHD_Daemon *daemon,
		  const struct sockaddr *addr,
		  socklen_t addrlen)
  MHD_NONNULL (1,2);

#endif
