/*
     This file is part of libmicrohttpd
     Copyright (C) 2016 Christian Grothoff (and other contributing authors)

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
 * @file upgrade_example.c
 * @brief example for how to use libmicrohttpd upgrade
 * @author Christian Grothoff
 *
 * Telnet to the HTTP server, use this in the request:
 * GET / http/1.1
 * Connection: Upgrade
 *
 * After this, whatever you type will be echo'ed back to you.
 */

#include "platform.h"
#include <microhttpd.h>
#include <pthread.h>

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd demo</body></html>"


/**
 * Change socket to blocking.
 *
 * @param fd the socket to manipulate
 * @return non-zero if succeeded, zero otherwise
 */
static void
make_blocking (MHD_socket fd)
{
#if defined(MHD_POSIX_SOCKETS)
  int flags;

  flags = fcntl (fd, F_GETFL);
  if (-1 == flags)
    return;
  if ((flags & ~O_NONBLOCK) != flags)
    if (-1 == fcntl (fd, F_SETFL, flags & ~O_NONBLOCK))
      abort ();
#elif defined(MHD_WINSOCK_SOCKETS)
  unsigned long flags = 1;

  ioctlsocket (fd, FIONBIO, &flags);
#endif /* MHD_WINSOCK_SOCKETS */

}


static void
send_all (MHD_socket sock,
          const char *buf,
          size_t len)
{
  ssize_t ret;
  size_t off;

  make_blocking (sock);
  for (off = 0; off < len; off += ret)
    {
      ret = send (sock,
                  &buf[off],
                  len - off,
                  0);
      if (0 > ret)
        {
          if (EAGAIN == errno)
            {
              ret = 0;
              continue;
            }
          break;
        }
      if (0 == ret)
        break;
    }
}


struct MyData
{
  struct MHD_UpgradeResponseHandle *urh;
  char *extra_in;
  size_t extra_in_size;
  MHD_socket sock;
};


/**
 * Main function for the thread that runs the interaction with
 * the upgraded socket.  Writes what it reads.
 *
 * @param cls the `struct MyData`
 */
static void *
run_usock (void *cls)
{
  struct MyData *md = cls;
  struct MHD_UpgradeResponseHandle *urh = md->urh;
  char buf[128];
  ssize_t got;

  make_blocking (md->sock);
  /* start by sending extra data MHD may have already read, if any */
  if (0 != md->extra_in_size)
    {
      send_all (md->sock,
                md->extra_in,
                md->extra_in_size);
      free (md->extra_in);
    }
  /* now echo in a loop */
  while (1)
    {
      got = recv (md->sock,
                  buf,
                  sizeof (buf),
                  0);
      if (0 >= got)
        break;
      send_all (md->sock,
                buf,
                got);
    }
  free (md);
  MHD_upgrade_action (urh,
                      MHD_UPGRADE_ACTION_CLOSE);
  return NULL;
}


/**
 * Function called after a protocol "upgrade" response was sent
 * successfully and the socket should now be controlled by some
 * protocol other than HTTP.
 *
 * Any data already received on the socket will be made available in
 * @e extra_in.  This can happen if the application sent extra data
 * before MHD send the upgrade response.  The application should
 * treat data from @a extra_in as if it had read it from the socket.
 *
 * Note that the application must not close() @a sock directly,
 * but instead use #MHD_upgrade_action() for special operations
 * on @a sock.
 *
 * Data forwarding to "upgraded" @a sock will be started as soon
 * as this function return.
 *
 * Except when in 'thread-per-connection' mode, implementations
 * of this function should never block (as it will still be called
 * from within the main event loop).
 *
 * @param cls closure, whatever was given to #MHD_create_response_for_upgrade().
 * @param connection original HTTP connection handle,
 *                   giving the function a last chance
 *                   to inspect the original HTTP request
 * @param con_cls last value left in `con_cls` of the `MHD_AccessHandlerCallback`
 * @param extra_in if we happened to have read bytes after the
 *                 HTTP header already (because the client sent
 *                 more than the HTTP header of the request before
 *                 we sent the upgrade response),
 *                 these are the extra bytes already read from @a sock
 *                 by MHD.  The application should treat these as if
 *                 it had read them from @a sock.
 * @param extra_in_size number of bytes in @a extra_in
 * @param sock socket to use for bi-directional communication
 *        with the client.  For HTTPS, this may not be a socket
 *        that is directly connected to the client and thus certain
 *        operations (TCP-specific setsockopt(), getsockopt(), etc.)
 *        may not work as expected (as the socket could be from a
 *        socketpair() or a TCP-loopback).  The application is expected
 *        to perform read()/recv() and write()/send() calls on the socket.
 *        The application may also call shutdown(), but must not call
 *        close() directly.
 * @param urh argument for #MHD_upgrade_action()s on this @a connection.
 *        Applications must eventually use this callback to (indirectly)
 *        perform the close() action on the @a sock.
 */
static void
uh_cb (void *cls,
       struct MHD_Connection *connection,
       void *con_cls,
       const char *extra_in,
       size_t extra_in_size,
       MHD_socket sock,
       struct MHD_UpgradeResponseHandle *urh)
{
  struct MyData *md;
  pthread_t pt;
  (void)cls;         /* Unused. Silent compiler warning. */
  (void)connection;  /* Unused. Silent compiler warning. */
  (void)con_cls;     /* Unused. Silent compiler warning. */

  md = malloc (sizeof (struct MyData));
  if (NULL == md)
    abort ();
  memset (md, 0, sizeof (struct MyData));
  if (0 != extra_in_size)
    {
      md->extra_in = malloc (extra_in_size);
      if (NULL == md->extra_in)
        abort ();
      memcpy (md->extra_in,
              extra_in,
              extra_in_size);
    }
  md->extra_in_size = extra_in_size;
  md->sock = sock;
  md->urh = urh;
  if (0 != pthread_create (&pt,
                           NULL,
                           &run_usock,
                           md))
    abort ();
  /* Note that by detaching like this we make it impossible to ensure
     a clean shutdown, as the we stop the daemon even if a worker thread
     is still running. Alas, this is a simple example... */
  pthread_detach (pt);

  /* This callback must return as soon as possible. */

  /* Data forwarding to "upgraded" socket will be started
   * after return from this callback. */
}


static int
ahc_echo (void *cls,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char *version,
          const char *upload_data,
          size_t *upload_data_size,
          void **ptr)
{
  static int aptr;
  struct MHD_Response *response;
  int ret;
  (void)cls;               /* Unused. Silent compiler warning. */
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
  response = MHD_create_response_for_upgrade (&uh_cb,
                                              NULL);

  MHD_add_response_header (response,
                           MHD_HTTP_HEADER_UPGRADE,
                           "Echo Server");
  ret = MHD_queue_response (connection,
                            MHD_HTTP_SWITCHING_PROTOCOLS,
                            response);
  MHD_destroy_response (response);
  return ret;
}


int
main (int argc,
      char *const *argv)
{
  struct MHD_Daemon *d;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  d = MHD_start_daemon (MHD_ALLOW_UPGRADE | MHD_USE_AUTO | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                        atoi (argv[1]),
                        NULL, NULL,
                        &ahc_echo, NULL,
                        MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 120,
                        MHD_OPTION_END);
  if (d == NULL)
    return 1;
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}
