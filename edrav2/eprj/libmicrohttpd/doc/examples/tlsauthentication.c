/* Feel free to use this example code in any way
   you see fit (Public Domain) */

#include <sys/types.h>
#ifndef _WIN32
#include <sys/select.h>
#include <sys/socket.h>
#else
#include <winsock2.h>
#endif
#include <microhttpd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT 8888

#define REALM     "\"Maintenance\""
#define USER      "a legitimate user"
#define PASSWORD  "and his password"

#define SERVERKEYFILE "server.key"
#define SERVERCERTFILE "server.pem"


static char *
string_to_base64 (const char *message)
{
  const char *lookup =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned long l;
  size_t i;
  char *tmp;
  size_t length = strlen (message);

  tmp = malloc (length * 2 + 1);
  if (NULL == tmp)
    return NULL;
  tmp[0] = 0;
  for (i = 0; i < length; i += 3)
    {
      l = (((unsigned long) message[i]) << 16)
        | (((i + 1) < length) ? (((unsigned long) message[i + 1]) << 8) : 0)
        | (((i + 2) < length) ? ((unsigned long) message[i + 2]) : 0);


      strncat (tmp, &lookup[(l >> 18) & 0x3F], 1);
      strncat (tmp, &lookup[(l >> 12) & 0x3F], 1);

      if (i + 1 < length)
        strncat (tmp, &lookup[(l >> 6) & 0x3F], 1);
      if (i + 2 < length)
        strncat (tmp, &lookup[l & 0x3F], 1);
    }

  if (length % 3)
    strncat (tmp, "===", 3 - length % 3);

  return tmp;
}


static long
get_file_size (const char *filename)
{
  FILE *fp;

  fp = fopen (filename, "rb");
  if (fp)
    {
      long size;

      if ((0 != fseek (fp, 0, SEEK_END)) || (-1 == (size = ftell (fp))))
        size = 0;

      fclose (fp);

      return size;
    }
  else
    return 0;
}


static char *
load_file (const char *filename)
{
  FILE *fp;
  char *buffer;
  long size;

  size = get_file_size (filename);
  if (0 == size)
    return NULL;

  fp = fopen (filename, "rb");
  if (! fp)
    return NULL;

  buffer = malloc (size + 1);
  if (! buffer)
    {
      fclose (fp);
      return NULL;
    }
  buffer[size] = '\0';

  if (size != (long)fread (buffer, 1, size, fp))
    {
      free (buffer);
      buffer = NULL;
    }

  fclose (fp);
  return buffer;
}


static int
ask_for_authentication (struct MHD_Connection *connection, const char *realm)
{
  int ret;
  struct MHD_Response *response;
  char *headervalue;
  size_t slen;
  const char *strbase = "Basic realm=";

  response = MHD_create_response_from_buffer (0, NULL,
					      MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;

  slen = strlen (strbase) + strlen (realm) + 1;
  if (NULL == (headervalue = malloc (slen)))
    return MHD_NO;
  snprintf (headervalue,
	    slen,
	    "%s%s",
	    strbase,
	    realm);
  ret = MHD_add_response_header (response,
				 "WWW-Authenticate",
				 headervalue);
  free (headervalue);
  if (! ret)
    {
      MHD_destroy_response (response);
      return MHD_NO;
    }

  ret = MHD_queue_response (connection,
			    MHD_HTTP_UNAUTHORIZED,
			    response);
  MHD_destroy_response (response);
  return ret;
}


static int
is_authenticated (struct MHD_Connection *connection,
                  const char *username,
		  const char *password)
{
  const char *headervalue;
  char *expected_b64;
  char *expected;
  const char *strbase = "Basic ";
  int authenticated;
  size_t slen;

  headervalue =
    MHD_lookup_connection_value (connection, MHD_HEADER_KIND,
                                 "Authorization");
  if (NULL == headervalue)
    return 0;
  if (0 != strncmp (headervalue, strbase, strlen (strbase)))
    return 0;

  slen = strlen (username) + 1 + strlen (password) + 1;
  if (NULL == (expected = malloc (slen)))
    return 0;
  snprintf (expected,
	    slen,
	    "%s:%s",
	    username,
	    password);
  expected_b64 = string_to_base64 (expected);
  free (expected);
  if (NULL == expected_b64)
    return 0;

  authenticated =
    (strcmp (headervalue + strlen (strbase), expected_b64) == 0);
  free (expected_b64);
  return authenticated;
}


static int
secret_page (struct MHD_Connection *connection)
{
  int ret;
  struct MHD_Response *response;
  const char *page = "<html><body>A secret.</body></html>";

  response =
    MHD_create_response_from_buffer (strlen (page), (void *) page,
				     MHD_RESPMEM_PERSISTENT);
  if (!response)
    return MHD_NO;

  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)url;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */

  if (0 != strcmp (method, "GET"))
    return MHD_NO;
  if (NULL == *con_cls)
    {
      *con_cls = connection;
      return MHD_YES;
    }

  if (!is_authenticated (connection, USER, PASSWORD))
    return ask_for_authentication (connection, REALM);

  return secret_page (connection);
}


int
main ()
{
  struct MHD_Daemon *daemon;
  char *key_pem;
  char *cert_pem;

  key_pem = load_file (SERVERKEYFILE);
  cert_pem = load_file (SERVERCERTFILE);

  if ((key_pem == NULL) || (cert_pem == NULL))
    {
      printf ("The key/certificate files could not be read.\n");
      if (NULL != key_pem)
        free (key_pem);
      if (NULL != cert_pem)
        free (cert_pem);
      return 1;
    }

  daemon =
    MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_TLS, PORT, NULL,
                      NULL, &answer_to_connection, NULL,
                      MHD_OPTION_HTTPS_MEM_KEY, key_pem,
                      MHD_OPTION_HTTPS_MEM_CERT, cert_pem, MHD_OPTION_END);
  if (NULL == daemon)
    {
      printf ("%s\n", cert_pem);

      free (key_pem);
      free (cert_pem);

      return 1;
    }

  (void) getchar ();

  MHD_stop_daemon (daemon);
  free (key_pem);
  free (cert_pem);

  return 0;
}
