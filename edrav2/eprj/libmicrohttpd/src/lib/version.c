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
 * @file lib/version.c
 * @brief versioning and optional feature tests
 * @author Christian Grothoff
 */
#include "internal.h"


/**
 * Obtain the version of this library
 *
 * @return static version string, e.g. "0.9.9"
 * @ingroup specialized
 */
const char *
MHD_get_version (void)
{
#ifdef PACKAGE_VERSION
  return PACKAGE_VERSION;
#else  /* !PACKAGE_VERSION */
  static char ver[12] = "\0\0\0\0\0\0\0\0\0\0\0";
  if (0 == ver[0])
  {
    int res = MHD_snprintf_(ver,
                            sizeof(ver),
                            "%x.%x.%x",
                            (((int)MHD_VERSION >> 24) & 0xFF),
                            (((int)MHD_VERSION >> 16) & 0xFF),
                            (((int)MHD_VERSION >> 8) & 0xFF));
    if (0 >= res || sizeof(ver) <= res)
      return "0.0.0"; /* Can't return real version*/
  }
  return ver;
#endif /* !PACKAGE_VERSION */
}


/**
 * Get information about supported MHD features.
 * Indicate that MHD was compiled with or without support for
 * particular feature. Some features require additional support
 * by kernel. Kernel support is not checked by this function.
 *
 * @param feature type of requested information
 * @return #MHD_YES if feature is supported by MHD, #MHD_NO if
 * feature is not supported or feature is unknown.
 * @ingroup specialized
 */
_MHD_EXTERN enum MHD_Bool
MHD_is_feature_supported(enum MHD_Feature feature)
{
  switch(feature)
    {
    case MHD_FEATURE_MESSAGES:
#ifdef HAVE_MESSAGES
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_TLS:
#ifdef HTTPS_SUPPORT
      return MHD_YES;
#else  /* ! HTTPS_SUPPORT */
      return MHD_NO;
#endif  /* ! HTTPS_SUPPORT */
    case MHD_FEATURE_HTTPS_CERT_CALLBACK:
#if defined(HTTPS_SUPPORT) && GNUTLS_VERSION_MAJOR >= 3
      return MHD_YES;
#else  /* !HTTPS_SUPPORT || GNUTLS_VERSION_MAJOR < 3 */
      return MHD_NO;
#endif /* !HTTPS_SUPPORT || GNUTLS_VERSION_MAJOR < 3 */
    case MHD_FEATURE_IPv6:
#ifdef HAVE_INET6
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_IPv6_ONLY:
#if defined(IPPROTO_IPV6) && defined(IPV6_V6ONLY)
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_POLL:
#ifdef HAVE_POLL
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_EPOLL:
#ifdef EPOLL_SUPPORT
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_SHUTDOWN_LISTEN_SOCKET:
#ifdef HAVE_LISTEN_SHUTDOWN
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_SOCKETPAIR:
#ifdef _MHD_ITC_SOCKETPAIR
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_TCP_FASTOPEN:
#ifdef TCP_FASTOPEN
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_BASIC_AUTH:
#ifdef BAUTH_SUPPORT
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_DIGEST_AUTH:
#ifdef DAUTH_SUPPORT
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_POSTPROCESSOR:
#ifdef HAVE_POSTPROCESSOR
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_HTTPS_KEY_PASSWORD:
#if defined(HTTPS_SUPPORT) && GNUTLS_VERSION_NUMBER >= 0x030111
      return MHD_YES;
#else  /* !HTTPS_SUPPORT || GNUTLS_VERSION_NUMBER < 0x030111 */
      return MHD_NO;
#endif /* !HTTPS_SUPPORT || GNUTLS_VERSION_NUMBER < 0x030111 */
    case MHD_FEATURE_LARGE_FILE:
#if defined(HAVE_PREAD64) || defined(_WIN32)
      return MHD_YES;
#elif defined(HAVE_PREAD)
      return (sizeof(uint64_t) > sizeof(off_t)) ? MHD_NO : MHD_YES;
#elif defined(HAVE_LSEEK64)
      return MHD_YES;
#else
      return (sizeof(uint64_t) > sizeof(off_t)) ? MHD_NO : MHD_YES;
#endif
    case MHD_FEATURE_THREAD_NAMES:
#if defined(MHD_USE_THREAD_NAME_)
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_UPGRADE:
#if defined(UPGRADE_SUPPORT)
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_RESPONSES_SHARED_FD:
#if defined(HAVE_PREAD64) || defined(HAVE_PREAD) || defined(_WIN32)
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_AUTODETECT_BIND_PORT:
#ifdef MHD_USE_GETSOCKNAME
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_AUTOSUPPRESS_SIGPIPE:
#if defined(MHD_WINSOCK_SOCKETS) || defined(MHD_socket_nosignal_) || defined (MSG_NOSIGNAL)
      return MHD_YES;
#else
      return MHD_NO;
#endif
    case MHD_FEATURE_SENDFILE:
#ifdef _MHD_HAVE_SENDFILE
      return MHD_YES;
#else
      return MHD_NO;
#endif

    }
  return MHD_NO;
}
