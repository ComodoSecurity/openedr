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
#include <stdio.h>

#define PORT 8888


static int
print_out_key (void *cls, enum MHD_ValueKind kind, const char *key,
               const char *value)
{
  (void)cls;    /* Unused. Silent compiler warning. */
  (void)kind;   /* Unused. Silent compiler warning. */
  printf ("%s: %s\n", key, value);
  return MHD_YES;
}


static int
answer_to_connection (void *cls, struct MHD_Connection *connection,
                      const char *url, const char *method,
                      const char *version, const char *upload_data,
                      size_t *upload_data_size, void **con_cls)
{
  (void)cls;               /* Unused. Silent compiler warning. */
  (void)version;           /* Unused. Silent compiler warning. */
  (void)upload_data;       /* Unused. Silent compiler warning. */
  (void)upload_data_size;  /* Unused. Silent compiler warning. */
  (void)con_cls;           /* Unused. Silent compiler warning. */
  printf ("New %s request for %s using version %s\n", method, url, version);

  MHD_get_connection_values (connection, MHD_HEADER_KIND, print_out_key,
                             NULL);

  return MHD_NO;
}


int
main ()
{
  struct MHD_Daemon *daemon;

  daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
  if (NULL == daemon)
    return 1;

  (void) getchar ();

  MHD_stop_daemon (daemon);
  return 0;
}
