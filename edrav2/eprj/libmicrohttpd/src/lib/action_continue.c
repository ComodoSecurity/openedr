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
 * @file lib/action_continue.c
 * @brief implementation of MHD_action_continue()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * The continue action is being run.  Continue
 * handling the upload.
 *
 * @param cls NULL
 * @param request the request to apply the action to
 * @return #MHD_SC_OK on success
 */
static enum MHD_StatusCode
cont_action (void *cls,
	     struct MHD_Request *request)
{
  (void) cls;
  (void) request;
  /* not sure yet, but this function body may 
     just legitimately stay empty... */
  return MHD_SC_OK;
}


/**
 * Action telling MHD to continue processing the upload.
 *
 * @return action operation, never NULL
 */
const struct MHD_Action *
MHD_action_continue (void)
{
  static struct MHD_Action acont = {
    .action = &cont_action,
    .action_cls = NULL
  };

  return &acont;
}

/* end of action_continue.c */
