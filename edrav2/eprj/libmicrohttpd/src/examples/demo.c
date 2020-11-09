/*
     This file is part of libmicrohttpd
     Copyright (C) 2013 Christian Grothoff (and other contributing authors)

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
 * @file demo.c
 * @brief complex demonstration site: create directory index, offer
 *        upload via form and HTTP POST, download with mime type detection
 *        and error reporting (403, etc.) --- and all of this with
 *        high-performance settings (large buffers, thread pool).
 *        If you want to benchmark MHD, this code should be used to
 *        run tests against.  Note that the number of threads may need
 *        to be adjusted depending on the number of available cores.
 * @author Christian Grothoff
 */
#include "platform.h"
#include <microhttpd.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef MHD_HAVE_LIBMAGIC
#include <magic.h>
#endif /* MHD_HAVE_LIBMAGIC */
#include <limits.h>
#include <ctype.h>

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

#ifndef PATH_MAX
/* Some platforms (namely: GNU Hurd) do no define PATH_MAX.
   As it is only example for MHD, just use reasonable value for PATH_MAX. */
#define PATH_MAX 16384
#endif

/**
 * Number of threads to run in the thread pool.  Should (roughly) match
 * the number of cores on your system.
 */
#define NUMBER_OF_THREADS CPU_COUNT

#ifdef MHD_HAVE_LIBMAGIC
/**
 * How many bytes of a file do we give to libmagic to determine the mime type?
 * 16k might be a bit excessive, but ought not hurt performance much anyway,
 * and should definitively be on the safe side.
 */
#define MAGIC_HEADER_SIZE (16 * 1024)
#endif /* MHD_HAVE_LIBMAGIC */


/**
 * Page returned for file-not-found.
 */
#define FILE_NOT_FOUND_PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"


/**
 * Page returned for internal errors.
 */
#define INTERNAL_ERROR_PAGE "<html><head><title>Internal error</title></head><body>Internal error</body></html>"


/**
 * Page returned for refused requests.
 */
#define REQUEST_REFUSED_PAGE "<html><head><title>Request refused</title></head><body>Request refused (file exists?)</body></html>"


/**
 * Head of index page.
 */
#define INDEX_PAGE_HEADER "<html>\n<head><title>Welcome</title></head>\n<body>\n"\
   "<h1>Upload</h1>\n"\
   "<form method=\"POST\" enctype=\"multipart/form-data\" action=\"/\">\n"\
   "<dl><dt>Content type:</dt><dd>"\
   "<input type=\"radio\" name=\"category\" value=\"books\">Book</input>"\
   "<input type=\"radio\" name=\"category\" value=\"images\">Image</input>"\
   "<input type=\"radio\" name=\"category\" value=\"music\">Music</input>"\
   "<input type=\"radio\" name=\"category\" value=\"software\">Software</input>"\
   "<input type=\"radio\" name=\"category\" value=\"videos\">Videos</input>\n"\
   "<input type=\"radio\" name=\"category\" value=\"other\" checked>Other</input></dd>"\
   "<dt>Language:</dt><dd>"\
   "<input type=\"radio\" name=\"language\" value=\"no-lang\" checked>none</input>"\
   "<input type=\"radio\" name=\"language\" value=\"en\">English</input>"\
   "<input type=\"radio\" name=\"language\" value=\"de\">German</input>"\
   "<input type=\"radio\" name=\"language\" value=\"fr\">French</input>"\
   "<input type=\"radio\" name=\"language\" value=\"es\">Spanish</input></dd>\n"\
   "<dt>File:</dt><dd>"\
   "<input type=\"file\" name=\"upload\"/></dd></dl>"\
   "<input type=\"submit\" value=\"Send!\"/>\n"\
   "</form>\n"\
   "<h1>Download</h1>\n"\
   "<ol>\n"

/**
 * Footer of index page.
 */
#define INDEX_PAGE_FOOTER "</ol>\n</body>\n</html>"


/**
 * NULL-terminated array of supported upload categories.  Should match HTML
 * in the form.
 */
static const char * const categories[] =
  {
    "books",
    "images",
    "music",
    "software",
    "videos",
    "other",
    NULL,
  };


/**
 * Specification of a supported language.
 */
struct Language
{
  /**
   * Directory name for the language.
   */
  const char *dirname;

  /**
   * Long name for humans.
   */
  const char *longname;

};

/**
 * NULL-terminated array of supported upload categories.  Should match HTML
 * in the form.
 */
static const struct Language languages[] =
  {
    { "no-lang", "No language specified" },
    { "en", "English" },
    { "de", "German" },
    { "fr", "French" },
    { "es", "Spanish" },
    { NULL, NULL },
  };


/**
 * Response returned if the requested file does not exist (or is not accessible).
 */
static struct MHD_Response *file_not_found_response;

/**
 * Response returned for internal errors.
 */
static struct MHD_Response *internal_error_response;

/**
 * Response returned for '/' (GET) to list the contents of the directory and allow upload.
 */
static struct MHD_Response *cached_directory_response;

/**
 * Response returned for refused uploads.
 */
static struct MHD_Response *request_refused_response;

/**
 * Mutex used when we update the cached directory response object.
 */
static pthread_mutex_t mutex;

#ifdef MHD_HAVE_LIBMAGIC
/**
 * Global handle to MAGIC data.
 */
static magic_t magic;
#endif /* MHD_HAVE_LIBMAGIC */


/**
 * Mark the given response as HTML for the brower.
 *
 * @param response response to mark
 */
static void
mark_as_html (struct MHD_Response *response)
{
  (void) MHD_add_response_header (response,
				  MHD_HTTP_HEADER_CONTENT_TYPE,
				  "text/html");
}


/**
 * Replace the existing 'cached_directory_response' with the
 * given response.
 *
 * @param response new directory response
 */
static void
update_cached_response (struct MHD_Response *response)
{
  (void) pthread_mutex_lock (&mutex);
  if (NULL != cached_directory_response)
    MHD_destroy_response (cached_directory_response);
  cached_directory_response = response;
  (void) pthread_mutex_unlock (&mutex);
}


/**
 * Context keeping the data for the response we're building.
 */
struct ResponseDataContext
{
  /**
   * Response data string.
   */
  char *buf;

  /**
   * Number of bytes allocated for 'buf'.
   */
  size_t buf_len;

  /**
   * Current position where we append to 'buf'. Must be smaller or equal to 'buf_len'.
   */
  size_t off;

};


/**
 * Create a listing of the files in 'dirname' in HTML.
 *
 * @param rdc where to store the list of files
 * @param dirname name of the directory to list
 * @return #MHD_YES on success, #MHD_NO on error
 */
static int
list_directory (struct ResponseDataContext *rdc,
		const char *dirname)
{
  char fullname[PATH_MAX];
  struct stat sbuf;
  DIR *dir;
  struct dirent *de;

  if (NULL == (dir = opendir (dirname)))
    return MHD_NO;
  while (NULL != (de = readdir (dir)))
    {
      if ('.' == de->d_name[0])
	continue;
      if (sizeof (fullname) <= (unsigned int)
	  snprintf (fullname, sizeof (fullname),
		    "%s/%s",
		    dirname, de->d_name))
	continue; /* ugh, file too long? how can this be!? */
      if (0 != stat (fullname, &sbuf))
	continue; /* ugh, failed to 'stat' */
      if (! S_ISREG (sbuf.st_mode))
	continue; /* not a regular file, skip */
      if (rdc->off + 1024 > rdc->buf_len)
	{
	  void *r;

	  if ( (2 * rdc->buf_len + 1024) < rdc->buf_len)
	    break; /* more than SIZE_T _index_ size? Too big for us */
	  rdc->buf_len = 2 * rdc->buf_len + 1024;
	  if (NULL == (r = realloc (rdc->buf, rdc->buf_len)))
	    break; /* out of memory */
	  rdc->buf = r;
	}
      rdc->off += snprintf (&rdc->buf[rdc->off],
			    rdc->buf_len - rdc->off,
			    "<li><a href=\"/%s\">%s</a></li>\n",
			    fullname,
			    de->d_name);
    }
  (void) closedir (dir);
  return MHD_YES;
}


/**
 * Re-scan our local directory and re-build the index.
 */
static void
update_directory ()
{
  static size_t initial_allocation = 32 * 1024; /* initial size for response buffer */
  struct MHD_Response *response;
  struct ResponseDataContext rdc;
  unsigned int language_idx;
  unsigned int category_idx;
  const struct Language *language;
  const char *category;
  char dir_name[128];
  struct stat sbuf;

  rdc.buf_len = initial_allocation;
  if (NULL == (rdc.buf = malloc (rdc.buf_len)))
    {
      update_cached_response (NULL);
      return;
    }
  rdc.off = snprintf (rdc.buf, rdc.buf_len,
		      "%s",
		      INDEX_PAGE_HEADER);
  for (language_idx = 0; NULL != languages[language_idx].dirname; language_idx++)
    {
      language = &languages[language_idx];

      if (0 != stat (language->dirname, &sbuf))
	continue; /* empty */
      /* we ensured always +1k room, filenames are ~256 bytes,
	 so there is always still enough space for the header
	 without need for an additional reallocation check. */
      rdc.off += snprintf (&rdc.buf[rdc.off], rdc.buf_len - rdc.off,
			   "<h2>%s</h2>\n",
			   language->longname);
      for (category_idx = 0; NULL != categories[category_idx]; category_idx++)
	{
	  category = categories[category_idx];
	  snprintf (dir_name, sizeof (dir_name),
		    "%s/%s",
		    language->dirname,
		    category);
	  if (0 != stat (dir_name, &sbuf))
	    continue; /* empty */

	  /* we ensured always +1k room, filenames are ~256 bytes,
	     so there is always still enough space for the header
	     without need for an additional reallocation check. */
	  rdc.off += snprintf (&rdc.buf[rdc.off], rdc.buf_len - rdc.off,
			       "<h3>%s</h3>\n",
			       category);

	  if (MHD_NO == list_directory (&rdc, dir_name))
	    {
	      free (rdc.buf);
	      update_cached_response (NULL);
	      return;
	    }
	}
    }
  /* we ensured always +1k room, filenames are ~256 bytes,
     so there is always still enough space for the footer
     without need for a final reallocation check. */
  rdc.off += snprintf (&rdc.buf[rdc.off], rdc.buf_len - rdc.off,
		       "%s",
		       INDEX_PAGE_FOOTER);
  initial_allocation = rdc.buf_len; /* remember for next time */
  response = MHD_create_response_from_buffer (rdc.off,
					      rdc.buf,
					      MHD_RESPMEM_MUST_FREE);
  mark_as_html (response);
#if FORCE_CLOSE
  (void) MHD_add_response_header (response,
				  MHD_HTTP_HEADER_CONNECTION,
				  "close");
#endif
  update_cached_response (response);
}


/**
 * Context we keep for an upload.
 */
struct UploadContext
{
  /**
   * Handle where we write the uploaded file to.
   */
  int fd;

  /**
   * Name of the file on disk (used to remove on errors).
   */
  char *filename;

  /**
   * Language for the upload.
   */
  char *language;

  /**
   * Category for the upload.
   */
  char *category;

  /**
   * Post processor we're using to process the upload.
   */
  struct MHD_PostProcessor *pp;

  /**
   * Handle to connection that we're processing the upload for.
   */
  struct MHD_Connection *connection;

  /**
   * Response to generate, NULL to use directory.
   */
  struct MHD_Response *response;
};


/**
 * Append the 'size' bytes from 'data' to '*ret', adding
 * 0-termination.  If '*ret' is NULL, allocate an empty string first.
 *
 * @param ret string to update, NULL or 0-terminated
 * @param data data to append
 * @param size number of bytes in 'data'
 * @return #MHD_NO on allocation failure, #MHD_YES on success
 */
static int
do_append (char **ret,
	   const char *data,
	   size_t size)
{
  char *buf;
  size_t old_len;

  if (NULL == *ret)
    old_len = 0;
  else
    old_len = strlen (*ret);
  if (NULL == (buf = malloc (old_len + size + 1)))
    return MHD_NO;
  if (NULL != *ret)
    {
      memcpy (buf,
	      *ret,
	      old_len);
      free (*ret);
    }
  memcpy (&buf[old_len],
	  data,
	  size);
  buf[old_len + size] = '\0';
  *ret = buf;
  return MHD_YES;
}


/**
 * Iterator over key-value pairs where the value
 * maybe made available in increments and/or may
 * not be zero-terminated.  Used for processing
 * POST data.
 *
 * @param cls user-specified closure
 * @param kind type of the value, always MHD_POSTDATA_KIND when called from MHD
 * @param key 0-terminated key for the value
 * @param filename name of the uploaded file, NULL if not known
 * @param content_type mime-type of the data, NULL if not known
 * @param transfer_encoding encoding of the data, NULL if not known
 * @param data pointer to size bytes of data at the
 *              specified offset
 * @param off offset of data in the overall value
 * @param size number of bytes in data available
 * @return #MHD_YES to continue iterating,
 *         #MHD_NO to abort the iteration
 */
static int
process_upload_data (void *cls,
		     enum MHD_ValueKind kind,
		     const char *key,
		     const char *filename,
		     const char *content_type,
		     const char *transfer_encoding,
		     const char *data,
		     uint64_t off,
		     size_t size)
{
  struct UploadContext *uc = cls;
  int i;
  (void)kind;              /* Unused. Silent compiler warning. */
  (void)content_type;      /* Unused. Silent compiler warning. */
  (void)transfer_encoding; /* Unused. Silent compiler warning. */
  (void)off;               /* Unused. Silent compiler warning. */

  if (0 == strcmp (key, "category"))
    return do_append (&uc->category, data, size);
  if (0 == strcmp (key, "language"))
    return do_append (&uc->language, data, size);
  if (0 != strcmp (key, "upload"))
    {
      fprintf (stderr,
	       "Ignoring unexpected form value `%s'\n",
	       key);
      return MHD_YES; /* ignore */
    }
  if (NULL == filename)
    {
      fprintf (stderr, "No filename, aborting upload\n");
      return MHD_NO; /* no filename, error */
    }
  if ( (NULL == uc->category) ||
       (NULL == uc->language) )
    {
      fprintf (stderr,
	       "Missing form data for upload `%s'\n",
	       filename);
      uc->response = request_refused_response;
      return MHD_NO;
    }
  if (-1 == uc->fd)
    {
      char fn[PATH_MAX];

      if ( (NULL != strstr (filename, "..")) ||
	   (NULL != strchr (filename, '/')) ||
	   (NULL != strchr (filename, '\\')) )
	{
	  uc->response = request_refused_response;
	  return MHD_NO;
	}
      /* create directories -- if they don't exist already */
#ifdef WINDOWS
      (void) mkdir (uc->language);
#else
      (void) mkdir (uc->language, S_IRWXU);
#endif
      snprintf (fn, sizeof (fn),
		"%s/%s",
		uc->language,
		uc->category);
#ifdef WINDOWS
      (void) mkdir (fn);
#else
      (void) mkdir (fn, S_IRWXU);
#endif
      /* open file */
      snprintf (fn, sizeof (fn),
		"%s/%s/%s",
		uc->language,
		uc->category,
		filename);
      for (i=strlen (fn)-1;i>=0;i--)
	if (! isprint ((unsigned char) fn[i]))
	  fn[i] = '_';
      uc->fd = open (fn,
		     O_CREAT | O_EXCL
#if O_LARGEFILE
		     | O_LARGEFILE
#endif
		     | O_WRONLY,
		     S_IRUSR | S_IWUSR);
      if (-1 == uc->fd)
	{
	  fprintf (stderr,
		   "Error opening file `%s' for upload: %s\n",
		   fn,
		   strerror (errno));
	  uc->response = request_refused_response;
	  return MHD_NO;
	}
      uc->filename = strdup (fn);
    }
  if ( (0 != size) &&
       (size != (size_t) write (uc->fd, data, size)) )
    {
      /* write failed; likely: disk full */
      fprintf (stderr,
	       "Error writing to file `%s': %s\n",
	       uc->filename,
	       strerror (errno));
      uc->response = internal_error_response;
      (void) close (uc->fd);
      uc->fd = -1;
      if (NULL != uc->filename)
	{
	  unlink (uc->filename);
	  free (uc->filename);
	  uc->filename = NULL;
	}
      return MHD_NO;
    }
  return MHD_YES;
}


/**
 * Function called whenever a request was completed.
 * Used to clean up 'struct UploadContext' objects.
 *
 * @param cls client-defined closure, NULL
 * @param connection connection handle
 * @param con_cls value as set by the last call to
 *        the MHD_AccessHandlerCallback, points to NULL if this was
 *            not an upload
 * @param toe reason for request termination
 */
static void
response_completed_callback (void *cls,
			     struct MHD_Connection *connection,
			     void **con_cls,
			     enum MHD_RequestTerminationCode toe)
{
  struct UploadContext *uc = *con_cls;
  (void)cls;         /* Unused. Silent compiler warning. */
  (void)connection;  /* Unused. Silent compiler warning. */
  (void)toe;         /* Unused. Silent compiler warning. */

  if (NULL == uc)
    return; /* this request wasn't an upload request */
  if (NULL != uc->pp)
    {
      MHD_destroy_post_processor (uc->pp);
      uc->pp = NULL;
    }
  if (-1 != uc->fd)
  {
    (void) close (uc->fd);
    if (NULL != uc->filename)
      {
	fprintf (stderr,
		 "Upload of file `%s' failed (incomplete or aborted), removing file.\n",
		 uc->filename);
	(void) unlink (uc->filename);
      }
  }
  if (NULL != uc->filename)
    free (uc->filename);
  free (uc);
}


/**
 * Return the current directory listing.
 *
 * @param connection connection to return the directory for
 * @return MHD_YES on success, MHD_NO on error
 */
static int
return_directory_response (struct MHD_Connection *connection)
{
  int ret;

  (void) pthread_mutex_lock (&mutex);
  if (NULL == cached_directory_response)
    ret = MHD_queue_response (connection,
			      MHD_HTTP_INTERNAL_SERVER_ERROR,
			      internal_error_response);
  else
    ret = MHD_queue_response (connection,
			      MHD_HTTP_OK,
			      cached_directory_response);
  (void) pthread_mutex_unlock (&mutex);
  return ret;
}


/**
 * Main callback from MHD, used to generate the page.
 *
 * @param cls NULL
 * @param connection connection handle
 * @param url requested URL
 * @param method GET, PUT, POST, etc.
 * @param version HTTP version
 * @param upload_data data from upload (PUT/POST)
 * @param upload_data_size number of bytes in @a upload_data
 * @param ptr our context
 * @return #MHD_YES on success, #MHD_NO to drop connection
 */
static int
generate_page (void *cls,
	       struct MHD_Connection *connection,
	       const char *url,
	       const char *method,
	       const char *version,
	       const char *upload_data,
	       size_t *upload_data_size, void **ptr)
{
  struct MHD_Response *response;
  int ret;
  int fd;
  struct stat buf;
  (void) cls;               /* Unused. Silent compiler warning. */
  (void) version;           /* Unused. Silent compiler warning. */

  if (0 != strcmp (url, "/"))
    {
      /* should be file download */
#ifdef MHD_HAVE_LIBMAGIC
      char file_data[MAGIC_HEADER_SIZE];
      ssize_t got;
#endif /* MHD_HAVE_LIBMAGIC */
      const char *mime;

      if ( (0 != strcmp (method, MHD_HTTP_METHOD_GET)) &&
           (0 != strcmp (method, MHD_HTTP_METHOD_HEAD)) )
        return MHD_NO;  /* unexpected method (we're not polite...) */
      fd = -1;
      if ( (NULL == strstr (&url[1], "..")) &&
	   ('/' != url[1]) )
        {
          fd = open (&url[1], O_RDONLY);
          if ( (-1 != fd) &&
               ( (0 != fstat (fd, &buf)) ||
                 (! S_ISREG (buf.st_mode)) ) )
            {
              (void) close (fd);
              fd = -1;
            }
        }
      if (-1 == fd)
	return MHD_queue_response (connection,
				   MHD_HTTP_NOT_FOUND,
				   file_not_found_response);
#ifdef MHD_HAVE_LIBMAGIC
      /* read beginning of the file to determine mime type  */
      got = read (fd, file_data, sizeof (file_data));
      (void) lseek (fd, 0, SEEK_SET);
      if (-1 != got)
	mime = magic_buffer (magic, file_data, got);
      else
#endif /* MHD_HAVE_LIBMAGIC */
	mime = NULL;

      if (NULL == (response = MHD_create_response_from_fd (buf.st_size,
							   fd)))
	{
	  /* internal error (i.e. out of memory) */
	  (void) close (fd);
	  return MHD_NO;
	}

      /* add mime type if we had one */
      if (NULL != mime)
	(void) MHD_add_response_header (response,
					MHD_HTTP_HEADER_CONTENT_TYPE,
					mime);
      ret = MHD_queue_response (connection,
				MHD_HTTP_OK,
				response);
      MHD_destroy_response (response);
      return ret;
    }

  if (0 == strcmp (method, MHD_HTTP_METHOD_POST))
    {
      /* upload! */
      struct UploadContext *uc = *ptr;

      if (NULL == uc)
	{
	  if (NULL == (uc = malloc (sizeof (struct UploadContext))))
	    return MHD_NO; /* out of memory, close connection */
	  memset (uc, 0, sizeof (struct UploadContext));
          uc->fd = -1;
	  uc->connection = connection;
	  uc->pp = MHD_create_post_processor (connection,
					      64 * 1024 /* buffer size */,
					      &process_upload_data, uc);
	  if (NULL == uc->pp)
	    {
	      /* out of memory, close connection */
	      free (uc);
	      return MHD_NO;
	    }
	  *ptr = uc;
	  return MHD_YES;
	}
      if (0 != *upload_data_size)
	{
	  if (NULL == uc->response)
	    (void) MHD_post_process (uc->pp,
				     upload_data,
				     *upload_data_size);
	  *upload_data_size = 0;
	  return MHD_YES;
	}
      /* end of upload, finish it! */
      MHD_destroy_post_processor (uc->pp);
      uc->pp = NULL;
      if (-1 != uc->fd)
	{
	  close (uc->fd);
	  uc->fd = -1;
	}
      if (NULL != uc->response)
	{
	  return MHD_queue_response (connection,
				     MHD_HTTP_FORBIDDEN,
				     uc->response);
	}
      else
	{
	  update_directory ();
	  return return_directory_response (connection);
	}
    }
  if ( (0 == strcmp (method, MHD_HTTP_METHOD_GET)) ||
       (0 == strcmp (method, MHD_HTTP_METHOD_HEAD)) )
  {
    return return_directory_response (connection);
  }

  /* unexpected request, refuse */
  return MHD_queue_response (connection,
			     MHD_HTTP_FORBIDDEN,
			     request_refused_response);
}


#ifndef MINGW
/**
 * Function called if we get a SIGPIPE. Does nothing.
 *
 * @param sig will be SIGPIPE (ignored)
 */
static void
catcher (int sig)
{
  (void)sig;	/* Unused. Silent compiler warning. */
  /* do nothing */
}


/**
 * setup handlers to ignore SIGPIPE.
 */
static void
ignore_sigpipe ()
{
  struct sigaction oldsig;
  struct sigaction sig;

  sig.sa_handler = &catcher;
  sigemptyset (&sig.sa_mask);
#ifdef SA_INTERRUPT
  sig.sa_flags = SA_INTERRUPT;  /* SunOS */
#else
  sig.sa_flags = SA_RESTART;
#endif
  if (0 != sigaction (SIGPIPE, &sig, &oldsig))
    fprintf (stderr,
             "Failed to install SIGPIPE handler: %s\n", strerror (errno));
}
#endif


/**
 * Entry point to demo.  Note: this HTTP server will make all
 * files in the current directory and its subdirectories available
 * to anyone.  Press ENTER to stop the server once it has started.
 *
 * @param argc number of arguments in argv
 * @param argv first and only argument should be the port number
 * @return 0 on success
 */
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
#ifndef MINGW
  ignore_sigpipe ();
#endif
#ifdef MHD_HAVE_LIBMAGIC
  magic = magic_open (MAGIC_MIME_TYPE);
  (void) magic_load (magic, NULL);
#endif /* MHD_HAVE_LIBMAGIC */

  (void) pthread_mutex_init (&mutex, NULL);
  file_not_found_response = MHD_create_response_from_buffer (strlen (FILE_NOT_FOUND_PAGE),
							     (void *) FILE_NOT_FOUND_PAGE,
							     MHD_RESPMEM_PERSISTENT);
  mark_as_html (file_not_found_response);
  request_refused_response = MHD_create_response_from_buffer (strlen (REQUEST_REFUSED_PAGE),
							     (void *) REQUEST_REFUSED_PAGE,
							     MHD_RESPMEM_PERSISTENT);
  mark_as_html (request_refused_response);
  internal_error_response = MHD_create_response_from_buffer (strlen (INTERNAL_ERROR_PAGE),
							     (void *) INTERNAL_ERROR_PAGE,
							     MHD_RESPMEM_PERSISTENT);
  mark_as_html (internal_error_response);
  update_directory ();
  d = MHD_start_daemon (MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        port,
                        NULL, NULL,
			&generate_page, NULL,
			MHD_OPTION_CONNECTION_MEMORY_LIMIT, (size_t) (256 * 1024),
#if PRODUCTION
			MHD_OPTION_PER_IP_CONNECTION_LIMIT, (unsigned int) (64),
#endif
			MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) (120 /* seconds */),
			MHD_OPTION_THREAD_POOL_SIZE, (unsigned int) NUMBER_OF_THREADS,
			MHD_OPTION_NOTIFY_COMPLETED, &response_completed_callback, NULL,
			MHD_OPTION_END);
  if (NULL == d)
    return 1;
  fprintf (stderr, "HTTP server running. Press ENTER to stop the server\n");
  (void) getc (stdin);
  MHD_stop_daemon (d);
  MHD_destroy_response (file_not_found_response);
  MHD_destroy_response (request_refused_response);
  MHD_destroy_response (internal_error_response);
  update_cached_response (NULL);
  (void) pthread_mutex_destroy (&mutex);
#ifdef MHD_HAVE_LIBMAGIC
  magic_close (magic);
#endif /* MHD_HAVE_LIBMAGIC */
  return 0;
}

/* end of demo.c */
