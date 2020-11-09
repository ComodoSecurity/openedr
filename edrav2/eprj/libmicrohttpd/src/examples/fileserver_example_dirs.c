/*
     This file is part of libmicrohttpd
     Copyright (C) 2007 Christian Grothoff (and other contributing authors)

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
 * @file fileserver_example_dirs.c
 * @brief example for how to use libmicrohttpd to serve files (with directory support)
 * @author Christian Grothoff
 */

#include "platform.h"
#include <dirent.h>
#include <microhttpd.h>
#include <unistd.h>

#define PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"


static ssize_t
file_reader (void *cls, uint64_t pos, char *buf, size_t max)
{
  FILE *file = cls;

  (void) fseek (file, pos, SEEK_SET);
  return fread (buf, 1, max, file);
}


static void
file_free_callback (void *cls)
{
  FILE *file = cls;
  fclose (file);
}


static void
dir_free_callback (void *cls)
{
  DIR *dir = cls;
  if (dir != NULL)
    closedir (dir);
}


static ssize_t
dir_reader (void *cls, uint64_t pos, char *buf, size_t max)
{
  DIR *dir = cls;
  struct dirent *e;

  if (max < 512)
    return 0;
  (void)pos; /* 'pos' is ignored as function return next one single entry per call. */
  do
    {
      e = readdir (dir);
      if (e == NULL)
        return MHD_CONTENT_READER_END_OF_STREAM;
  } while (e->d_name[0] == '.');
  return snprintf (buf, max,
		   "<a href=\"/%s\">%s</a><br>",
		   e->d_name,
		   e->d_name);
}


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data,
	  size_t *upload_data_size, void **ptr)
{
  static int aptr;
  struct MHD_Response *response;
  int ret;
  FILE *file;
  int fd;
  DIR *dir;
  struct stat buf;
  char emsg[1024];
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, MHD_HTTP_METHOD_GET))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  *ptr = NULL;                  /* reset when done */

  file = fopen (&url[1], "rb");
  if (NULL != file)
    {
      fd = fileno (file);
      if (-1 == fd)
        {
          (void) fclose (file);
          return MHD_NO; /* internal error */
        }
      if ( (0 != fstat (fd, &buf)) ||
           (! S_ISREG (buf.st_mode)) )
        {
          /* not a regular file, refuse to serve */
          fclose (file);
          file = NULL;
        }
    }

  if (NULL == file)
    {
      dir = opendir (".");
      if (NULL == dir)
	{
	  /* most likely cause: more concurrent requests than
	     available file descriptors / 2 */
	  snprintf (emsg,
		    sizeof (emsg),
		    "Failed to open directory `.': %s\n",
		    strerror (errno));
	  response = MHD_create_response_from_buffer (strlen (emsg),
						      emsg,
						      MHD_RESPMEM_MUST_COPY);
	  if (NULL == response)
	    return MHD_NO;
	  ret = MHD_queue_response (connection,
                                    MHD_HTTP_SERVICE_UNAVAILABLE,
                                    response);
	  MHD_destroy_response (response);
	}
      else
	{
	  response = MHD_create_response_from_callback (MHD_SIZE_UNKNOWN,
							32 * 1024,
							&dir_reader,
							dir,
							&dir_free_callback);
	  if (NULL == response)
	    {
	      closedir (dir);
	      return MHD_NO;
	    }
	  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	  MHD_destroy_response (response);
	}
    }
  else
    {
      response = MHD_create_response_from_callback (buf.st_size, 32 * 1024,     /* 32k page size */
                                                    &file_reader,
                                                    file,
                                                    &file_free_callback);
      if (NULL == response)
	{
	  fclose (file);
	  return MHD_NO;
	}
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }
  return ret;
}


int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        atoi (argv[1]),
                        NULL, NULL, &ahc_echo, PAGE, MHD_OPTION_END);
  if (NULL == d)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
