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
 * @file lib/init.c
 * @brief initialization routines
 * @author Christian Grothoff
 */
#include "internal.h"
#include "init.h"


#ifndef _AUTOINIT_FUNCS_ARE_SUPPORTED
/**
 * Track global initialisation
 */
volatile unsigned int global_init_count = 0;

#ifdef MHD_MUTEX_STATIC_DEFN_INIT_
/**
 * Global initialisation mutex
 */
MHD_MUTEX_STATIC_DEFN_INIT_(global_init_mutex_);
#endif /* MHD_MUTEX_STATIC_DEFN_INIT_ */

#endif

#if defined(_WIN32) && ! defined(__CYGWIN__)
/**
 * Track initialization of winsock
 */
static int mhd_winsock_inited_ = 0;
#endif

/**
 * Default implementation of the panic function,
 * prints an error message and aborts.
 *
 * @param cls unused
 * @param file name of the file with the problem
 * @param line line number with the problem
 * @param reason error message with details
 */
static void
mhd_panic_std (void *cls,
	       const char *file,
	       unsigned int line,
	       const char *reason)
{
  (void)cls; /* Mute compiler warning. */
#ifdef HAVE_MESSAGES
  fprintf (stderr,
           _("Fatal error in GNU libmicrohttpd %s:%u: %s\n"),
	   file,
           line,
           reason);
#else  /* ! HAVE_MESSAGES */
  (void)file;   /* Mute compiler warning. */
  (void)line;   /* Mute compiler warning. */
  (void)reason; /* Mute compiler warning. */
#endif
  abort ();
}


/**
 * Globally initialize library.
 */
void
MHD_init (void)
{
#if defined(_WIN32) && ! defined(__CYGWIN__)
  WSADATA wsd;
#endif /* _WIN32 && ! __CYGWIN__ */

  if (NULL == mhd_panic)
    mhd_panic = &mhd_panic_std;

#if defined(_WIN32) && ! defined(__CYGWIN__)
  if (0 != WSAStartup (MAKEWORD(2, 2),
		       &wsd))
    MHD_PANIC (_("Failed to initialize winsock\n"));
  mhd_winsock_inited_ = 1;
  if ( (2 != LOBYTE(wsd.wVersion)) &&
       (2 != HIBYTE(wsd.wVersion)) )
    MHD_PANIC (_("Winsock version 2.2 is not available\n"));
#endif
  MHD_monotonic_sec_counter_init ();
#ifdef HAVE_FREEBSD_SENDFILE
  MHD_conn_init_static_ ();
#endif /* HAVE_FREEBSD_SENDFILE */
}


/**
 * Global teardown work.
 */
void
MHD_fini (void)
{
#if defined(_WIN32) && ! defined(__CYGWIN__)
  if (mhd_winsock_inited_)
    WSACleanup();
#endif
  MHD_monotonic_sec_counter_finish ();
}

#ifdef _AUTOINIT_FUNCS_ARE_SUPPORTED

_SET_INIT_AND_DEINIT_FUNCS(MHD_init, MHD_fini);

#else

/**
 * Check whether global initialisation was performed
 * and call initialiser if necessary.
 */
void
MHD_check_global_init_ (void)
{
#ifdef MHD_MUTEX_STATIC_DEFN_INIT_
  MHD_mutex_lock_chk_(&global_init_mutex_);
#endif /* MHD_MUTEX_STATIC_DEFN_INIT_ */
  if (0 == global_init_count++)
    MHD_init ();
#ifdef MHD_MUTEX_STATIC_DEFN_INIT_
  MHD_mutex_unlock_chk_(&global_init_mutex_);
#endif /* MHD_MUTEX_STATIC_DEFN_INIT_ */
}
#endif
