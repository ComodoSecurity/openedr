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
 * @file lib/daemon_select.h
 * @brief  non-public functions provided by daemon_select.c
 * @author Christian Grothoff
 */

#ifndef DAEMON_SELECT_H
#define DAEMON_SELECT_H

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
  MHD_NONNULL(1);


#if defined(HTTPS_SUPPORT) && defined(UPGRADE_SUPPORT)
/**
 * Process upgraded connection with a select loop.
 * We are in our own thread, only processing @a con
 *
 * @param con connection to process
 */
void
MHD_daemon_upgrade_connection_with_select_ (struct MHD_Connection *con)
  MHD_NONNULL(1);
#endif

#endif
