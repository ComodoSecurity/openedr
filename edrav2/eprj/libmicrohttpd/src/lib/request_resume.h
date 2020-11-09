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
 * @file lib/request_resume.h
 * @brief implementation of MHD_request_resume()
 * @author Christian Grothoff
 */


#ifndef REQUEST_RESUME_H
#define REQUEST_RESUME_H

/**
 * Run through the suspended connections and move any that are no
 * longer suspended back to the active state.
 * @remark To be called only from thread that process
 * daemon's select()/poll()/etc.
 *
 * @param daemon daemon context
 * @return true if a connection was actually resumed
 */
bool
MHD_resume_suspended_connections_ (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);

#endif
