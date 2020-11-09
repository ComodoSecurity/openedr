/*
     This file is part of libmicrohttpd
     Copyright (C) 2008 Christian Grothoff (and other contributing authors)

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
 * @file authorization_example.c
 * @brief example for how to use libmicrohttpd with HTTP authentication
 * @author Christian Grothoff
 */

#include "platform.h"
#include <microhttpd.h>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif /* !WIN32_LEAN_AND_MEAN */
#include <windows.h>
#endif

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd demo</body></html>"

#define DENIED "<html><head><title>Access denied</title></head><body>Access denied</body></html>"



static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data, size_t *upload_data_size, void **ptr)
{
  static int aptr;
  const char *me = cls;
  struct MHD_Response *response;
  int ret;
  char *user;
  char *pass;
  int fail;
  (void)url;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, "GET"))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  *ptr = NULL;                  /* reset when done */

  /* require: "Aladdin" with password "open sesame" */
  pass = NULL;
  user = MHD_basic_auth_get_username_password (connection,
                                               &pass);
  fail = ( (NULL == user) ||
           (0 != strcmp (user, "Aladdin")) ||
           (0 != strcmp (pass, "open sesame") ) );
  if (fail)
  {
      response = MHD_create_response_from_buffer (strlen (DENIED),
						  (void *) DENIED,
						  MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_basic_auth_fail_response (connection,"TestRealm",response);
    }
  else
    {
      response = MHD_create_response_from_buffer (strlen (me),
						  (void *) me,
						  MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    }
  if (NULL != user)
    MHD_free (user);
  if (NULL != pass)
    MHD_free (pass);
  MHD_destroy_response (response);
  return ret;
}


int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;
  unsigned int port;

  if ( (argc != 2) ||
       (1 != sscanf (argv[1], "%u", &port)) ||
       (UINT16_MAX < port) )
    {
      fprintf (stderr,
	       "%s PORT\n", argv[0]);
      return 1;
    }

  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        atoi (argv[1]),
                        NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
  if (d == NULL)
    return 1;
  fprintf (stderr, "HTTP server running. Press ENTER to stop the server\n");
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
