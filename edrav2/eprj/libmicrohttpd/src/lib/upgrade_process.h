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
 * @file lib/upgrade_process.h
 * @brief function to process upgrade activity (over TLS)
 * @author Christian Grothoff
 */
#ifndef UPGRADE_PROCESS_H
#define UPGRADE_PROCESS_H


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Performs bi-directional forwarding on upgraded HTTPS connections
 * based on the readyness state stored in the @a urh handle.
 * @remark To be called only from thread that process
 * connection's recv(), send() and response.
 *
 * @param urh handle to process
 */
void
MHD_upgrade_response_handle_process_ (struct MHD_UpgradeResponseHandle *urh)
  MHD_NONNULL(1);


#endif

#endif
