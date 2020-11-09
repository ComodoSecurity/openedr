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
 * @file lib/daemon_ip_limit.c
 * @brief counting of connections per IP
 * @author Christian Grothoff
 */
#include "internal.h"
#include "daemon_ip_limit.h"
#if HAVE_SEARCH_H
#include <search.h>
#else
#include "tsearch.h"
#endif


/**
 * Maintain connection count for single address.
 */
struct MHD_IPCount
{
  /**
   * Address family. AF_INET or AF_INET6 for now.
   */
  int family;

  /**
   * Actual address.
   */
  union
  {
    /**
     * IPv4 address.
     */
    struct in_addr ipv4;
#if HAVE_INET6
    /**
     * IPv6 address.
     */
    struct in6_addr ipv6;
#endif
  } addr;

  /**
   * Counter.
   */
  unsigned int count;
};


/**
 * Trace up to and return master daemon. If the supplied daemon
 * is a master, then return the daemon itself.
 *
 * @param daemon handle to a daemon
 * @return master daemon handle
 */
static struct MHD_Daemon*
get_master (struct MHD_Daemon *daemon)
{
  while (NULL != daemon->master)
    daemon = daemon->master;
  return daemon;
}


/**
 * Lock shared structure for IP connection counts and connection DLLs.
 *
 * @param daemon handle to daemon where lock is
 */
static void
MHD_ip_count_lock (struct MHD_Daemon *daemon)
{
  MHD_mutex_lock_chk_(&daemon->per_ip_connection_mutex);
}


/**
 * Unlock shared structure for IP connection counts and connection DLLs.
 *
 * @param daemon handle to daemon where lock is
 */
static void
MHD_ip_count_unlock (struct MHD_Daemon *daemon)
{
  MHD_mutex_unlock_chk_(&daemon->per_ip_connection_mutex);
}


/**
 * Tree comparison function for IP addresses (supplied to tsearch() family).
 * We compare everything in the struct up through the beginning of the
 * 'count' field.
 *
 * @param a1 first address to compare
 * @param a2 second address to compare
 * @return -1, 0 or 1 depending on result of compare
 */
static int
MHD_ip_addr_compare (const void *a1,
                     const void *a2)
{
  return memcmp (a1,
                 a2,
                 offsetof (struct MHD_IPCount,
                           count));
}


/**
 * Parse address and initialize @a key using the address.
 *
 * @param addr address to parse
 * @param addrlen number of bytes in @a addr
 * @param key where to store the parsed address
 * @return #MHD_YES on success and #MHD_NO otherwise (e.g., invalid address type)
 */
static int
MHD_ip_addr_to_key (const struct sockaddr *addr,
		    socklen_t addrlen,
		    struct MHD_IPCount *key)
{
  memset(key,
         0,
         sizeof(*key));

  /* IPv4 addresses */
  if (sizeof (struct sockaddr_in) == addrlen)
    {
      const struct sockaddr_in *addr4 = (const struct sockaddr_in*) addr;

      key->family = AF_INET;
      memcpy (&key->addr.ipv4,
              &addr4->sin_addr,
              sizeof(addr4->sin_addr));
      return MHD_YES;
    }

#if HAVE_INET6
  /* IPv6 addresses */
  if (sizeof (struct sockaddr_in6) == addrlen)
    {
      const struct sockaddr_in6 *addr6 = (const struct sockaddr_in6*) addr;

      key->family = AF_INET6;
      memcpy (&key->addr.ipv6,
              &addr6->sin6_addr,
              sizeof(addr6->sin6_addr));
      return MHD_YES;
    }
#endif

  /* Some other address */
  return MHD_NO;
}


/**
 * Check if IP address is over its limit in terms of the number
 * of allowed concurrent connections.  If the IP is still allowed,
 * increments the connection counter.
 *
 * @param daemon handle to daemon where connection counts are tracked
 * @param addr address to add (or increment counter)
 * @param addrlen number of bytes in @a addr
 * @return Return #MHD_YES if IP below limit, #MHD_NO if IP has surpassed limit.
 *   Also returns #MHD_NO if fails to allocate memory.
 */
int
MHD_ip_limit_add (struct MHD_Daemon *daemon,
		  const struct sockaddr *addr,
		  socklen_t addrlen)
{
  struct MHD_IPCount *key;
  void **nodep;
  void *node;
  int result;

  daemon = get_master (daemon);
  /* Ignore if no connection limit assigned */
  if (0 == daemon->ip_connection_limit)
    return MHD_YES;

  if (NULL == (key = malloc (sizeof(*key))))
    return MHD_NO;

  /* Initialize key */
  if (MHD_NO == MHD_ip_addr_to_key (addr,
                                    addrlen,
                                    key))
    {
      /* Allow unhandled address types through */
      free (key);
      return MHD_YES;
    }
  MHD_ip_count_lock (daemon);

  /* Search for the IP address */
  if (NULL == (nodep = tsearch (key,
				&daemon->per_ip_connection_count,
				&MHD_ip_addr_compare)))
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		MHD_SC_IP_COUNTER_FAILURE,
		_("Failed to add IP connection count node\n"));
#endif
      MHD_ip_count_unlock (daemon);
      free (key);
      return MHD_NO;
    }
  node = *nodep;
  /* If we got an existing node back, free the one we created */
  if (node != key)
    free(key);
  key = (struct MHD_IPCount *) node;
  /* Test if there is room for another connection; if so,
   * increment count */
  result = (key->count < daemon->ip_connection_limit) ? MHD_YES : MHD_NO;
  if (MHD_YES == result)
    ++key->count;

  MHD_ip_count_unlock (daemon);
  return result;
}


/**
 * Decrement connection count for IP address, removing from table
 * count reaches 0.
 *
 * @param daemon handle to daemon where connection counts are tracked
 * @param addr address to remove (or decrement counter)
 * @param addrlen number of bytes in @a addr
 */
void
MHD_ip_limit_del (struct MHD_Daemon *daemon,
		  const struct sockaddr *addr,
		  socklen_t addrlen)
{
  struct MHD_IPCount search_key;
  struct MHD_IPCount *found_key;
  void **nodep;

  daemon = get_master (daemon);
  /* Ignore if no connection limit assigned */
  if (0 == daemon->ip_connection_limit)
    return;
  /* Initialize search key */
  if (MHD_NO == MHD_ip_addr_to_key (addr,
                                    addrlen,
                                    &search_key))
    return;

  MHD_ip_count_lock (daemon);

  /* Search for the IP address */
  if (NULL == (nodep = tfind (&search_key,
			      &daemon->per_ip_connection_count,
			      &MHD_ip_addr_compare)))
    {
      /* Something's wrong if we couldn't find an IP address
       * that was previously added */
      MHD_PANIC (_("Failed to find previously-added IP address\n"));
    }
  found_key = (struct MHD_IPCount *) *nodep;
  /* Validate existing count for IP address */
  if (0 == found_key->count)
    {
      MHD_PANIC (_("Previously-added IP address had counter of zero\n"));
    }
  /* Remove the node entirely if count reduces to 0 */
  if (0 == --found_key->count)
    {
      tdelete (found_key,
	       &daemon->per_ip_connection_count,
	       &MHD_ip_addr_compare);
      free (found_key);
    }

  MHD_ip_count_unlock (daemon);
}

/* end of daemon_ip_limit.c */
