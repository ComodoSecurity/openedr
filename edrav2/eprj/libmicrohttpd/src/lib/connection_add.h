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
 * @file lib/connection_add.h
 * @brief functions to add connection to our active set
 * @author Christian Grothoff
 */

#ifndef CONNECTION_ADD_H
#define CONNECTION_ADD_H

/**
 * Accept an incoming connection and create the MHD_Connection object
 * for it.  This function also enforces policy by way of checking with
 * the accept policy callback.  @remark To be called only from thread
 * that process daemon's select()/poll()/etc.
 *
 * @param daemon handle with the listen socket
 * @return #MHD_SC_OK on success 
 */
enum MHD_StatusCode
MHD_accept_connection_ (struct MHD_Daemon *daemon)
  MHD_NONNULL (1);

#endif
