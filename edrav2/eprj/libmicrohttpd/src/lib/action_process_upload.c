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
 * @file lib/action_process_upload.c
 * @brief implementation of MHD_action_process_upload()
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Internal details about an upload action.
 */
struct UploadAction
{
  /**
   * An upload action is an action. This field
   * must come first!
   */
  struct MHD_Action action;

  MHD_UploadCallback uc;

  void *uc_cls;

};


/**
 * The application wants to process uploaded data for
 * the given request. Do it!
 *
 * @param cls the `struct UploadAction` with the
 *    function we are to call for upload data
 * @param request the request for which we are to process 
 *    upload data
 * @return #MHD_SC_OK on success
 */
static enum MHD_StatusCode
upload_action (void *cls,
	       struct MHD_Request *request)
{
  struct UploadAction *ua = cls;

  (void) ua;
  // FIXME: implement!
  return -1;
}


/**
 * Create an action that handles an upload.
 *
 * @param uc function to call with uploaded data
 * @param uc_cls closure for @a uc
 * @return NULL on error (out of memory)
 * @ingroup action
 */
const struct MHD_Action *
MHD_action_process_upload (MHD_UploadCallback uc,
			   void *uc_cls)
{
  struct UploadAction *ua;

  if (NULL == (ua = malloc (sizeof (struct UploadAction))))
    return NULL;
  ua->action.action = &upload_action;
  ua->action.action_cls = ua;
  ua->uc = uc;
  ua->uc_cls = uc_cls;
  return &ua->action;
}


/* end of action_process_upload.c */


