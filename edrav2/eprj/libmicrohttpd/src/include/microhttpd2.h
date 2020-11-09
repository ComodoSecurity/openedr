/*
     This file is part of libmicrohttpd
     Copyright (C) 2006-2018 Christian Grothoff, Karlson2k (Evgeny Grin)
     (and other contributing authors)

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
 * Just includes the NEW definitions for the NG-API.
 * Note that we do not indicate which of the OLD APIs
 * simply need to be kept vs. deprecated.
 *
 *
 * The goal is to provide a basis for discussion!
 * Little of this is implemented yet.
 *
 * Main goals:
 * - simplify application callbacks by splitting header/upload/post
 *   functionality currently provided by calling the same
 *   MHD_AccessHandlerCallback 3+ times into separate callbacks.
 * - keep the API very simple for simple requests, but allow
 *   more complex logic to be incrementally introduced
 *   (via new struct MHD_Action construction)
 * - avoid repeated scans for URL matches via the new
 *   struct MHD_Action construction
 * - provide default logarithmic implementation of URL scan
 *   => reduce strcmp(url) from >= 3n operations to "log n"
 *      per request.
 * - better types, in particular avoid varargs for options
 * - make it harder to pass inconsistent options
 * - combine options and flags into more uniform API (at least
 *   exterally!)
 * - simplify API use by using sane defaults (benefiting from
 *   breaking backwards compatibility) and making all options
 *   really optional, and where applicable avoid having options
 *   where the default works if nothing is specified
 * - simplify API by moving rarely used http_version into
 *   MHD_request_get_information()
 * - avoid 'int' for MHD_YES/MHD_NO by introducing `enum MHD_Bool`
 * - improve terminology by eliminating confusion between
 *   'request' and 'connection'
 * - prepare API for having multiple TLS backends
 * - use more consistent prefixes for related functions
 *   by using MHD_subject_verb_object naming convention, also
 *   at the same time avoid symbol conflict with legacy names
 *   (so we can have one binary implementing old and new
 *   library API at the same time via compatibility layer).
 * - make it impossible to queue a response at the wrong time
 * - make it impossible to suspend a connection/request at the
 *   wrong time (improves thread-safety)
 * - make it clear which response status codes are "properly"
 *   supported (include the descriptive string) by using an enum;
 * - simplify API for common-case of one-shot responses by
 *   eliminating need for destroy response in most cases;
 *
 * TODO:
 * - varargs in upgrade is still there and ugly (and not even used!)
 * - migrate event loop apis (get fdset, timeout, MHD_run(), etc.)
 */
#ifndef MICROHTTPD2_H
#define MICROHTTPD2_H


#ifdef __cplusplus
extern "C"
{
#if 0                           /* keep Emacsens' auto-indent happy */
}
#endif
#endif

/* While we generally would like users to use a configure-driven
   build process which detects which headers are present and
   hence works on any platform, we use "standard" includes here
   to build out-of-the-box for beginning users on common systems.

   If generic headers don't work on your platform, include headers
   which define 'va_list', 'size_t', 'ssize_t', 'intptr_t',
   'uint16_t', 'uint32_t', 'uint64_t', 'off_t', 'struct sockaddr',
   'socklen_t', 'fd_set' and "#define MHD_PLATFORM_H" before
   including "microhttpd.h". Then the following "standard"
   includes won't be used (which might be a good idea, especially
   on platforms where they do not exist).
   */
#ifndef MHD_PLATFORM_H
#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <ws2tcpip.h>
#if defined(_MSC_FULL_VER) && !defined (_SSIZE_T_DEFINED)
#define _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#endif /* !_SSIZE_T_DEFINED */
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif
#endif

#if defined(__CYGWIN__) && !defined(_SYS_TYPES_FD_SET)
/* Do not define __USE_W32_SOCKETS under Cygwin! */
#error Cygwin with winsock fd_set is not supported
#endif

/**
 * Current version of the library.
 * 0x01093001 = 1.9.30-1.
 */
#define MHD_VERSION 0x01000000



/**
 * Representation of 'bool' in the public API as stdbool.h may not
 * always be available.
 */
enum MHD_Bool
{

  /**
   * MHD-internal return code for "NO".
   */
  MHD_NO = 0,

  /**
   * MHD-internal return code for "YES".  All non-zero values
   * will be interpreted as "YES", but MHD will only ever
   * return #MHD_YES or #MHD_NO.
   */
  MHD_YES = 1
};


/**
 * Constant used to indicate unknown size (use when
 * creating a response).
 */
#ifdef UINT64_MAX
#define MHD_SIZE_UNKNOWN UINT64_MAX
#else
#define MHD_SIZE_UNKNOWN  ((uint64_t) -1LL)
#endif

#ifdef SIZE_MAX
#define MHD_CONTENT_READER_END_OF_STREAM SIZE_MAX
#define MHD_CONTENT_READER_END_WITH_ERROR (SIZE_MAX - 1)
#else
#define MHD_CONTENT_READER_END_OF_STREAM ((size_t) -1LL)
#define MHD_CONTENT_READER_END_WITH_ERROR (((size_t) -1LL) - 1)
#endif

#ifndef _MHD_EXTERN
#if defined(_WIN32) && defined(MHD_W32LIB)
#define _MHD_EXTERN extern
#elif defined (_WIN32) && defined(MHD_W32DLL)
/* Define MHD_W32DLL when using MHD as W32 .DLL to speed up linker a little */
#define _MHD_EXTERN __declspec(dllimport)
#else
#define _MHD_EXTERN extern
#endif
#endif

#ifndef MHD_SOCKET_DEFINED
/**
 * MHD_socket is type for socket FDs
 */
#if !defined(_WIN32) || defined(_SYS_TYPES_FD_SET)
#define MHD_POSIX_SOCKETS 1
typedef int MHD_socket;
#define MHD_INVALID_SOCKET (-1)
#else /* !defined(_WIN32) || defined(_SYS_TYPES_FD_SET) */
#define MHD_WINSOCK_SOCKETS 1
#include <winsock2.h>
typedef SOCKET MHD_socket;
#define MHD_INVALID_SOCKET (INVALID_SOCKET)
#endif /* !defined(_WIN32) || defined(_SYS_TYPES_FD_SET) */
#define MHD_SOCKET_DEFINED 1
#endif /* MHD_SOCKET_DEFINED */

/**
 * Define MHD_NO_DEPRECATION before including "microhttpd.h" to disable deprecation messages
 */
#ifdef MHD_NO_DEPRECATION
#define _MHD_DEPR_MACRO(msg)
#define _MHD_NO_DEPR_IN_MACRO 1
#define _MHD_DEPR_IN_MACRO(msg)
#define _MHD_NO_DEPR_FUNC 1
#define _MHD_DEPR_FUNC(msg)
#endif /* MHD_NO_DEPRECATION */

#ifndef _MHD_DEPR_MACRO
#if defined(_MSC_FULL_VER) && _MSC_VER+0 >= 1500
/* VS 2008 or later */
/* Stringify macros */
#define _MHD_INSTRMACRO(a) #a
#define _MHD_STRMACRO(a) _MHD_INSTRMACRO(a)
/* deprecation message */
#define _MHD_DEPR_MACRO(msg) __pragma(message(__FILE__ "(" _MHD_STRMACRO(__LINE__)"): warning: " msg))
#define _MHD_DEPR_IN_MACRO(msg) _MHD_DEPR_MACRO(msg)
#elif defined(__clang__) || defined (__GNUC_PATCHLEVEL__)
/* clang or GCC since 3.0 */
#define _MHD_GCC_PRAG(x) _Pragma (#x)
#if (defined(__clang__) && (__clang_major__+0 >= 5 ||			\
			    (!defined(__apple_build_version__) && (__clang_major__+0  > 3 || (__clang_major__+0 == 3 && __clang_minor__ >= 3))))) || \
  __GNUC__+0 > 4 || (__GNUC__+0 == 4 && __GNUC_MINOR__+0 >= 8)
/* clang >= 3.3 (or XCode's clang >= 5.0) or
   GCC >= 4.8 */
#define _MHD_DEPR_MACRO(msg) _MHD_GCC_PRAG(GCC warning msg)
#define _MHD_DEPR_IN_MACRO(msg) _MHD_DEPR_MACRO(msg)
#else /* older clang or GCC */
/* clang < 3.3, XCode's clang < 5.0, 3.0 <= GCC < 4.8 */
#define _MHD_DEPR_MACRO(msg) _MHD_GCC_PRAG(message msg)
#if (defined(__clang__) && (__clang_major__+0  > 2 || (__clang_major__+0 == 2 && __clang_minor__ >= 9))) /* FIXME: clang >= 2.9, earlier versions not tested */
/* clang handles inline pragmas better than GCC */
#define _MHD_DEPR_IN_MACRO(msg) _MHD_DEPR_MACRO(msg)
#endif /* clang >= 2.9 */
#endif  /* older clang or GCC */
/* #elif defined(SOMEMACRO) */ /* add compiler-specific macros here if required */
#endif /* clang || GCC >= 3.0 */
#endif /* !_MHD_DEPR_MACRO */

#ifndef _MHD_DEPR_MACRO
#define _MHD_DEPR_MACRO(msg)
#endif /* !_MHD_DEPR_MACRO */

#ifndef _MHD_DEPR_IN_MACRO
#define _MHD_NO_DEPR_IN_MACRO 1
#define _MHD_DEPR_IN_MACRO(msg)
#endif /* !_MHD_DEPR_IN_MACRO */

#ifndef _MHD_DEPR_FUNC
#if defined(_MSC_FULL_VER) && _MSC_VER+0 >= 1400
/* VS 2005 or later */
#define _MHD_DEPR_FUNC(msg) __declspec(deprecated(msg))
#elif defined(_MSC_FULL_VER) && _MSC_VER+0 >= 1310
/* VS .NET 2003 deprecation do not support custom messages */
#define _MHD_DEPR_FUNC(msg) __declspec(deprecated)
#elif (__GNUC__+0 >= 5) || (defined (__clang__) && \
  (__clang_major__+0 > 2 || (__clang_major__+0 == 2 && __clang_minor__ >= 9)))  /* FIXME: earlier versions not tested */
/* GCC >= 5.0 or clang >= 2.9 */
#define _MHD_DEPR_FUNC(msg) __attribute__((deprecated(msg)))
#elif defined (__clang__) || __GNUC__+0 > 3 || (__GNUC__+0 == 3 && __GNUC_MINOR__+0 >= 1)
/* 3.1 <= GCC < 5.0 or clang < 2.9 */
/* old GCC-style deprecation do not support custom messages */
#define _MHD_DEPR_FUNC(msg) __attribute__((__deprecated__))
/* #elif defined(SOMEMACRO) */ /* add compiler-specific macros here if required */
#endif /* clang < 2.9 || GCC >= 3.1 */
#endif /* !_MHD_DEPR_FUNC */

#ifndef _MHD_DEPR_FUNC
#define _MHD_NO_DEPR_FUNC 1
#define _MHD_DEPR_FUNC(msg)
#endif /* !_MHD_DEPR_FUNC */


/* Define MHD_NONNULL attribute */

/**
 * Macro to indicate that certain parameters must be
 * non-null.  Todo: port to non-gcc platforms.
 */
#if defined(__CYGWIN__) || defined(_WIN32) || defined(MHD_W32LIB)
#define MHD_NONNULL(...) /* empty */
#else
#define MHD_NONNULL(...) __THROW __nonnull((__VA_ARGS__))
#endif

/**
 * Not all architectures and `printf()`'s support the `long long` type.
 * This gives the ability to replace `long long` with just a `long`,
 * standard `int` or a `short`.
 */
#ifndef MHD_UNSIGNED_LONG_LONG
#define MHD_UNSIGNED_LONG_LONG unsigned long long
#endif
/**
 * Format string for printing a variable of type #MHD_LONG_LONG.
 * You should only redefine this if you also define #MHD_LONG_LONG.
 */
#ifndef MHD_UNSIGNED_LONG_LONG_PRINTF
#define MHD_UNSIGNED_LONG_LONG_PRINTF "%llu"
#endif


/**
 * @brief Handle for a connection / HTTP request.
 *
 * With HTTP/1.1, multiple requests can be run over the same
 * connection.  However, MHD will only show one request per TCP
 * connection to the client at any given time.
 *
 * Replaces `struct MHD_Connection`, renamed to better reflect
 * what this object truly represents to the application using
 * MHD.
 *
 * @ingroup request
 */
struct MHD_Request;


/**
 * A connection corresponds to the network/stream abstraction.
 * A single network (i.e. TCP) stream may be used for multiple
 * requests, which in HTTP/1.1 must be processed sequentially.
 */
struct MHD_Connection;


/**
 * Return values for reporting errors, also used
 * for logging.
 *
 * A value of 0 indicates success (as a return value).
 * Values between 0 and 10000 must be handled explicitly by the app.
 * Values from 10000-19999 are informational.
 * Values from 20000-29999 indicate successful operations.
 * Values from 30000-39999 indicate unsuccessful (normal) operations.
 * Values from 40000-49999 indicate client errors.
 * Values from 50000-59999 indicate MHD server errors.
 * Values from 60000-69999 indicate application errors.
 */
enum MHD_StatusCode
{

  /* 00000-level status codes indicate return values
     the application must act on. */

  /**
   * Successful operation (not used for logging).
   */
  MHD_SC_OK = 0,

  /**
   * We were asked to return a timeout, but, there is no timeout.
   */
  MHD_SC_NO_TIMEOUT = 1,


  /* 10000-level status codes indicate intermediate
     results of some kind. */

  /**
   * Informational event, MHD started.
   */
  MHD_SC_DAEMON_STARTED = 10000,

  /**
   * Informational event, we accepted a connection.
   */
  MHD_SC_CONNECTION_ACCEPTED = 10001,

  /**
   * Informational event, thread processing connection termiantes.
   */
  MHD_SC_THREAD_TERMINATING = 10002,

  /**
   * Informational event, state machine status for a connection.
   */
  MHD_SC_STATE_MACHINE_STATUS_REPORT = 10003,

  /**
   * accept() returned transient error.
   */
  MHD_SC_ACCEPT_FAILED_EAGAIN = 10004,


  /* 20000-level status codes indicate success of some kind. */

  /**
   * MHD is closing a connection after the client closed it
   * (perfectly normal end).
   */
  MHD_SC_CONNECTION_CLOSED = 20000,

  /**
   * MHD is closing a connection because the application
   * logic to generate the response data completed.
   */
  MHD_SC_APPLICATION_DATA_GENERATION_FINISHED = 20001,


  /* 30000-level status codes indicate transient failures
     that might go away if the client tries again. */


  /**
   * Resource limit in terms of number of parallel connections
   * hit.
   */
  MHD_SC_LIMIT_CONNECTIONS_REACHED = 30000,

  /**
   * We failed to allocate memory for poll() syscall.
   * (May be transient.)
   */
  MHD_SC_POLL_MALLOC_FAILURE = 30001,

  /**
   * The operation failed because the respective
   * daemon is already too deep inside of the shutdown
   * activity.
   */
  MHD_SC_DAEMON_ALREADY_SHUTDOWN = 30002,

  /**
   * We failed to start a thread.
   */
  MHD_SC_THREAD_LAUNCH_FAILURE = 30003,

  /**
   * The operation failed because we either have no
   * listen socket or were already quiesced.
   */
  MHD_SC_DAEMON_ALREADY_QUIESCED = 30004,

  /**
   * The operation failed because client disconnected
   * faster than we could accept().
   */
  MHD_SC_ACCEPT_FAST_DISCONNECT = 30005,

  /**
   * Operating resource limits hit on accept().
   */
  MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED = 30006,

  /**
   * Connection was refused by accept policy callback.
   */
  MHD_SC_ACCEPT_POLICY_REJECTED = 30007,

  /**
   * We failed to allocate memory for the connection.
   * (May be transient.)
   */
  MHD_SC_CONNECTION_MALLOC_FAILURE = 30008,

  /**
   * We failed to allocate memory for the connection's memory pool.
   * (May be transient.)
   */
  MHD_SC_POOL_MALLOC_FAILURE = 30009,

  /**
   * We failed to forward data from a Web socket to the
   * application to the remote side due to the socket
   * being closed prematurely. (May be transient.)
   */
  MHD_SC_UPGRADE_FORWARD_INCOMPLETE = 30010,

  /**
   * We failed to allocate memory for generatig the response from our
   * memory pool.  Likely the request header was too large to leave
   * enough room.
   */
  MHD_SC_CONNECTION_POOL_MALLOC_FAILURE = 30011,


  /* 40000-level errors are caused by the HTTP client
     (or the network) */

  /**
   * MHD is closing a connection because parsing the
   * request failed.
   */
  MHD_SC_CONNECTION_PARSE_FAIL_CLOSED = 40000,

  /**
   * MHD is closing a connection because it was reset.
   */
  MHD_SC_CONNECTION_RESET_CLOSED = 40001,

  /**
   * MHD is closing a connection because reading the
   * request failed.
   */
  MHD_SC_CONNECTION_READ_FAIL_CLOSED = 40002,

  /**
   * MHD is closing a connection because writing the response failed.
   */
  MHD_SC_CONNECTION_WRITE_FAIL_CLOSED = 40003,

  /**
   * MHD is returning an error because the header provided
   * by the client is too big.
   */
  MHD_SC_CLIENT_HEADER_TOO_BIG = 40004,

  /**
   * An HTTP/1.1 request was sent without the "Host:" header.
   */
  MHD_SC_HOST_HEADER_MISSING = 40005,

  /**
   * The given content length was not a number.
   */
  MHD_SC_CONTENT_LENGTH_MALFORMED = 40006,

  /**
   * The given uploaded, chunked-encoded body was malformed.
   */
  MHD_SC_CHUNKED_ENCODING_MALFORMED = 40007,



  /* 50000-level errors are because of an error internal
     to the MHD logic, possibly including our interaction
     with the operating system (but not the application) */

  /**
   * This build of MHD does not support TLS, but the application
   * requested TLS.
   */
  MHD_SC_TLS_DISABLED = 50000,

  /**
   * The application attempted to setup TLS paramters before
   * enabling TLS.
   */
  MHD_SC_TLS_BACKEND_UNINITIALIZED = 50003,

  /**
   * The selected TLS backend does not yet support this operation.
   */
  MHD_SC_TLS_BACKEND_OPERATION_UNSUPPORTED = 50004,

  /**
   * Failed to setup ITC channel.
   */
  MHD_SC_ITC_INITIALIZATION_FAILED = 50005,

  /**
   * File descriptor for ITC channel too large.
   */
  MHD_SC_ITC_DESCRIPTOR_TOO_LARGE = 50006,

  /**
   * The specified value for the NC length is way too large
   * for this platform (integer overflow on `size_t`).
   */
  MHD_SC_DIGEST_AUTH_NC_LENGTH_TOO_BIG = 50007,

  /**
   * We failed to allocate memory for the specified nonce
   * counter array.  The option was not set.
   */
  MHD_SC_DIGEST_AUTH_NC_ALLOCATION_FAILURE = 50008,

  /**
   * This build of the library does not support
   * digest authentication.
   */
  MHD_SC_DIGEST_AUTH_NOT_SUPPORTED_BY_BUILD = 50009,

  /**
   * IPv6 requested but not supported by this build.
   */
  MHD_SC_IPV6_NOT_SUPPORTED_BY_BUILD = 50010,

  /**
   * We failed to open the listen socket. Maybe the build
   * supports IPv6, but your kernel does not?
   */
  MHD_SC_FAILED_TO_OPEN_LISTEN_SOCKET = 50011,

  /**
   * Specified address family is not supported by this build.
   */
  MHD_SC_AF_NOT_SUPPORTED_BY_BUILD = 50012,

  /**
   * Failed to enable listen address reuse.
   */
  MHD_SC_LISTEN_ADDRESS_REUSE_ENABLE_FAILED = 50013,

  /**
   * Enabling listen address reuse is not supported by this platform.
   */
  MHD_SC_LISTEN_ADDRESS_REUSE_ENABLE_NOT_SUPPORTED = 50014,

  /**
   * Failed to disable listen address reuse.
   */
  MHD_SC_LISTEN_ADDRESS_REUSE_DISABLE_FAILED = 50015,

  /**
   * Disabling listen address reuse is not supported by this platform.
   */
  MHD_SC_LISTEN_ADDRESS_REUSE_DISABLE_NOT_SUPPORTED = 50016,

  /**
   * We failed to explicitly enable or disable dual stack for
   * the IPv6 listen socket.  The socket will be used in whatever
   * the default is the OS gives us.
   */
  MHD_SC_LISTEN_DUAL_STACK_CONFIGURATION_FAILED = 50017,

  /**
   * On this platform, MHD does not support explicitly configuring
   * dual stack behavior.
   */
  MHD_SC_LISTEN_DUAL_STACK_CONFIGURATION_NOT_SUPPORTED = 50018,

  /**
   * Failed to enable TCP FAST OPEN option.
   */
  MHD_SC_FAST_OPEN_FAILURE = 50020,

  /**
   * Failed to start listening on listen socket.
   */
  MHD_SC_LISTEN_FAILURE = 50021,

  /**
   * Failed to obtain our listen port via introspection.
   */
  MHD_SC_LISTEN_PORT_INTROSPECTION_FAILURE = 50022,

  /**
   * Failed to obtain our listen port via introspection
   * due to unsupported address family being used.
   */
  MHD_SC_LISTEN_PORT_INTROSPECTION_UNKNOWN_AF = 50023,

  /**
   * We failed to set the listen socket to non-blocking.
   */
  MHD_SC_LISTEN_SOCKET_NONBLOCKING_FAILURE = 50024,

  /**
   * Listen socket value is too large (for use with select()).
   */
  MHD_SC_LISTEN_SOCKET_TOO_LARGE = 50025,

  /**
   * We failed to allocate memory for the thread pool.
   */
  MHD_SC_THREAD_POOL_MALLOC_FAILURE = 50026,

  /**
   * We failed to allocate mutex for thread pool worker.
   */
  MHD_SC_THREAD_POOL_CREATE_MUTEX_FAILURE = 50027,

  /**
   * There was an attempt to upgrade a connection on
   * a daemon where upgrades are disallowed.
   */
  MHD_SC_UPGRADE_ON_DAEMON_WITH_UPGRADE_DISALLOWED = 50028,

  /**
   * Failed to signal via ITC channel.
   */
  MHD_SC_ITC_USE_FAILED = 50029,

  /**
   * We failed to initialize the main thread for listening.
   */
  MHD_SC_THREAD_MAIN_LAUNCH_FAILURE = 50030,

  /**
   * We failed to initialize the threads for the worker pool.
   */
  MHD_SC_THREAD_POOL_LAUNCH_FAILURE = 50031,

  /**
   * We failed to add a socket to the epoll() set.
   */
  MHD_SC_EPOLL_CTL_ADD_FAILED = 50032,

  /**
   * We failed to create control socket for the epoll().
   */
  MHD_SC_EPOLL_CTL_CREATE_FAILED = 50034,

  /**
   * We failed to configure control socket for the epoll()
   * to be non-inheritable.
   */
  MHD_SC_EPOLL_CTL_CONFIGURE_NOINHERIT_FAILED = 50035,

  /**
   * We failed to build the FD set because a socket was
   * outside of the permitted range.
   */
  MHD_SC_SOCKET_OUTSIDE_OF_FDSET_RANGE = 50036,

  /**
   * This daemon was not configured with options that
   * would allow us to build an FD set for select().
   */
  MHD_SC_CONFIGURATION_MISMATCH_FOR_GET_FDSET = 50037,

  /**
   * This daemon was not configured with options that
   * would allow us to obtain a meaningful timeout.
   */
  MHD_SC_CONFIGURATION_MISMATCH_FOR_GET_TIMEOUT = 50038,

  /**
   * This daemon was not configured with options that
   * would allow us to run with select() data.
   */
  MHD_SC_CONFIGURATION_MISMATCH_FOR_RUN_SELECT = 50039,

  /**
   * This daemon was not configured to run with an
   * external event loop.
   */
  MHD_SC_CONFIGURATION_MISMATCH_FOR_RUN_EXTERNAL = 50040,

  /**
   * Encountered an unexpected event loop style
   * (should never happen).
   */
  MHD_SC_CONFIGURATION_UNEXPECTED_ELS = 50041,

  /**
   * Encountered an unexpected error from select()
   * (should never happen).
   */
  MHD_SC_UNEXPECTED_SELECT_ERROR = 50042,

  /**
   * poll() is not supported.
   */
  MHD_SC_POLL_NOT_SUPPORTED = 50043,

  /**
   * Encountered an unexpected error from poll()
   * (should never happen).
   */
  MHD_SC_UNEXPECTED_POLL_ERROR = 50044,

  /**
   * We failed to configure accepted socket
   * to not use a signal pipe.
   */
  MHD_SC_ACCEPT_CONFIGURE_NOSIGPIPE_FAILED = 50045,

  /**
   * Encountered an unexpected error from epoll_wait()
   * (should never happen).
   */
  MHD_SC_UNEXPECTED_EPOLL_WAIT_ERROR = 50046,

  /**
   * epoll file descriptor is invalid (strange)
   */
  MHD_SC_EPOLL_FD_INVALID = 50047,

  /**
   * We failed to configure accepted socket
   * to be non-inheritable.
   */
  MHD_SC_ACCEPT_CONFIGURE_NOINHERIT_FAILED = 50048,

  /**
   * We failed to configure accepted socket
   * to be non-blocking.
   */
  MHD_SC_ACCEPT_CONFIGURE_NONBLOCKING_FAILED = 50049,

  /**
   * accept() returned non-transient error.
   */
  MHD_SC_ACCEPT_FAILED_UNEXPECTEDLY = 50050,

  /**
   * Operating resource limits hit on accept() while
   * zero connections are active. Oopsie.
   */
  MHD_SC_ACCEPT_SYSTEM_LIMIT_REACHED_INSTANTLY = 50051,

  /**
   * Failed to add IP address to per-IP counter for
   * some reason.
   */
  MHD_SC_IP_COUNTER_FAILURE = 50052,

  /**
   * Application violated our API by calling shutdown
   * while having an upgrade connection still open.
   */
  MHD_SC_SHUTDOWN_WITH_OPEN_UPGRADED_CONNECTION = 50053,

  /**
   * Due to an unexpected internal error with the
   * state machine, we closed the connection.
   */
  MHD_SC_STATEMACHINE_FAILURE_CONNECTION_CLOSED = 50054,

  /**
   * Failed to allocate memory in connection's pool
   * to parse the cookie header.
   */
  MHD_SC_COOKIE_POOL_ALLOCATION_FAILURE = 50055,

  /**
   * MHD failed to build the response header.
   */
  MHD_SC_FAILED_RESPONSE_HEADER_GENERATION = 50056,



  /* 60000-level errors are because the application
     logic did something wrong or generated an error. */

  /**
   * MHD does not support the requested combination of
   * EPOLL with thread-per-connection mode.
   */
  MHD_SC_SYSCALL_THREAD_COMBINATION_INVALID = 60000,

  /**
   * MHD does not support quiescing if ITC was disabled
   * and threads are used.
   */
  MHD_SC_SYSCALL_QUIESCE_REQUIRES_ITC = 60001,

  /**
   * We failed to bind the listen socket.
   */
  MHD_SC_LISTEN_SOCKET_BIND_FAILED = 60002,

  /**
   * The application requested an unsupported TLS backend to be used.
   */
  MHD_SC_TLS_BACKEND_UNSUPPORTED = 60003,

  /**
   * The application requested a TLS cipher suite which is not
   * supported by the selected backend.
   */
  MHD_SC_TLS_CIPHERS_INVALID = 60004,

  /**
   * MHD is closing a connection because the application
   * logic to generate the response data failed.
   */
  MHD_SC_APPLICATION_DATA_GENERATION_FAILURE_CLOSED = 60005,

  /**
   * MHD is closing a connection because the application
   * callback told it to do so.
   */
  MHD_SC_APPLICATION_CALLBACK_FAILURE_CLOSED = 60006,

  /**
   * Application only partially processed upload and did
   * not suspend connection. This may result in a hung
   * connection.
   */
  MHD_SC_APPLICATION_HUNG_CONNECTION = 60007,

  /**
   * Application only partially processed upload and did
   * not suspend connection and the read buffer was maxxed
   * out, so MHD closed the connection.
   */
  MHD_SC_APPLICATION_HUNG_CONNECTION_CLOSED = 60008,


};


/**
 * Actions are returned by the application to drive the request
 * handling of MHD.
 */
struct MHD_Action;


/**
 * HTTP methods explicitly supported by MHD.  Note that for
 * non-canonical methods, MHD will return #MHD_METHOD_UNKNOWN
 * and you can use #MHD_REQUEST_INFORMATION_HTTP_METHOD to get
 * the original string.
 *
 * However, applications must check for "#MHD_METHOD_UNKNOWN" *or* any
 * enum-value above those in this list, as future versions of MHD may
 * add additional methods (as per IANA registry), thus even if the API
 * returns "unknown" today, it may return a method-specific header in
 * the future!
 *
 * @defgroup methods HTTP methods
 * HTTP methods (as strings).
 * See: http://www.iana.org/assignments/http-methods/http-methods.xml
 * Registry Version 2015-05-19
 * @{
 */
enum MHD_Method
{

  /**
   * Method did not match any of the methods given below.
   */
  MHD_METHOD_UNKNOWN = 0,

  /**
   * "OPTIONS" method.
   * Safe.     Idempotent.     RFC7231, Section 4.3.7.
   */
  MHD_METHOD_OPTIONS = 1,

  /**
   * "GET" method.
   * Safe.     Idempotent.     RFC7231, Section 4.3.1.
   */
  MHD_METHOD_GET = 2,

  /**
   * "HEAD" method.
   * Safe.     Idempotent.     RFC7231, Section 4.3.2.
   */
  MHD_METHOD_HEAD = 3,

  /**
   * "POST" method.
   * Not safe. Not idempotent. RFC7231, Section 4.3.3.
   */
  MHD_METHOD_POST = 4,

  /**
   * "PUT" method.
   * Not safe. Idempotent.     RFC7231, Section 4.3.4.
   */
  MHD_METHOD_PUT = 5,

  /**
   * "DELETE" method.
   * Not safe. Idempotent.     RFC7231, Section 4.3.5.
   */
  MHD_METHOD_DELETE = 6,

  /**
   * "TRACE" method.
   */
  MHD_METHOD_TRACE = 7,

  /**
   * "CONNECT" method.
   */
  MHD_METHOD_CONNECT = 8,

  /**
   * "ACL" method.
   */
  MHD_METHOD_ACL = 9,

  /**
   * "BASELINE-CONTROL" method.
   */
  MHD_METHOD_BASELINE_CONTROL = 10,

  /**
   * "BIND" method.
   */
  MHD_METHOD_BIND = 11,

  /**
   * "CHECKIN" method.
   */
  MHD_METHOD_CHECKIN = 12,

  /**
   * "CHECKOUT" method.
   */
  MHD_METHOD_CHECKOUT = 13,

  /**
   * "COPY" method.
   */
  MHD_METHOD_COPY = 14,

  /**
   * "LABEL" method.
   */
  MHD_METHOD_LABEL = 15,

  /**
   * "LINK" method.
   */
  MHD_METHOD_LINK = 16,

  /**
   * "LOCK" method.
   */
  MHD_METHOD_LOCK = 17,

  /**
   * "MERGE" method.
   */
  MHD_METHOD_MERGE = 18,

  /**
   * "MKACTIVITY" method.
   */
  MHD_METHOD_MKACTIVITY = 19,

  /**
   * "MKCOL" method.
   */
  MHD_METHOD_MKCOL = 20,

  /**
   * "MKREDIRECTREF" method.
   */
  MHD_METHOD_MKREDIRECTREF = 21,

  /**
   * "MKWORKSPACE" method.
   */
  MHD_METHOD_MKWORKSPACE = 22,

  /**
   * "MOVE" method.
   */
  MHD_METHOD_MOVE = 23,

  /**
   * "ORDERPATCH" method.
   */
  MHD_METHOD_ORDERPATCH = 24,

  /**
   * "PATCH" method.
   */
  MHD_METHOD_PATH = 25,

  /**
   * "PRI" method.
   */
  MHD_METHOD_PRI = 26,

  /**
   * "PROPFIND" method.
   */
  MHD_METHOD_PROPFIND = 27,

  /**
   * "PROPPATCH" method.
   */
  MHD_METHOD_PROPPATCH = 28,

  /**
   * "REBIND" method.
   */
  MHD_METHOD_REBIND = 29,

  /**
   * "REPORT" method.
   */
  MHD_METHOD_REPORT = 30,

  /**
   * "SEARCH" method.
   */
  MHD_METHOD_SEARCH = 31,

  /**
   * "UNBIND" method.
   */
  MHD_METHOD_UNBIND = 32,

  /**
   * "UNCHECKOUT" method.
   */
  MHD_METHOD_UNCHECKOUT = 33,

  /**
   * "UNLINK" method.
   */
  MHD_METHOD_UNLINK = 34,

  /**
   * "UNLOCK" method.
   */
  MHD_METHOD_UNLOCK = 35,

  /**
   * "UPDATE" method.
   */
  MHD_METHOD_UPDATE = 36,

  /**
   * "UPDATEDIRECTREF" method.
   */
  MHD_METHOD_UPDATEDIRECTREF = 37,

  /**
   * "VERSION-CONTROL" method.
   */
  MHD_METHOD_VERSION_CONTROL = 38

  /* For more, check:
     https://www.iana.org/assignments/http-methods/http-methods.xhtml */

};

/** @} */ /* end of group methods */


/**
 * @defgroup postenc HTTP POST encodings
 * See also: http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4
 * @{
 */
#define MHD_HTTP_POST_ENCODING_FORM_URLENCODED "application/x-www-form-urlencoded"
#define MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA "multipart/form-data"

/** @} */ /* end of group postenc */



/**
 * @defgroup headers HTTP headers
 * These are the standard headers found in HTTP requests and responses.
 * See: http://www.iana.org/assignments/message-headers/message-headers.xml
 * Registry Version 2017-01-27
 * @{
 */

/* Main HTTP headers. */
/* Standard.      RFC7231, Section 5.3.2 */
#define MHD_HTTP_HEADER_ACCEPT "Accept"
/* Standard.      RFC7231, Section 5.3.3 */
#define MHD_HTTP_HEADER_ACCEPT_CHARSET "Accept-Charset"
/* Standard.      RFC7231, Section 5.3.4; RFC7694, Section 3 */
#define MHD_HTTP_HEADER_ACCEPT_ENCODING "Accept-Encoding"
/* Standard.      RFC7231, Section 5.3.5 */
#define MHD_HTTP_HEADER_ACCEPT_LANGUAGE "Accept-Language"
/* Standard.      RFC7233, Section 2.3 */
#define MHD_HTTP_HEADER_ACCEPT_RANGES "Accept-Ranges"
/* Standard.      RFC7234, Section 5.1 */
#define MHD_HTTP_HEADER_AGE "Age"
/* Standard.      RFC7231, Section 7.4.1 */
#define MHD_HTTP_HEADER_ALLOW "Allow"
/* Standard.      RFC7235, Section 4.2 */
#define MHD_HTTP_HEADER_AUTHORIZATION "Authorization"
/* Standard.      RFC7234, Section 5.2 */
#define MHD_HTTP_HEADER_CACHE_CONTROL "Cache-Control"
/* Reserved.      RFC7230, Section 8.1 */
#define MHD_HTTP_HEADER_CLOSE "Close"
/* Standard.      RFC7230, Section 6.1 */
#define MHD_HTTP_HEADER_CONNECTION "Connection"
/* Standard.      RFC7231, Section 3.1.2.2 */
#define MHD_HTTP_HEADER_CONTENT_ENCODING "Content-Encoding"
/* Standard.      RFC7231, Section 3.1.3.2 */
#define MHD_HTTP_HEADER_CONTENT_LANGUAGE "Content-Language"
/* Standard.      RFC7230, Section 3.3.2 */
#define MHD_HTTP_HEADER_CONTENT_LENGTH "Content-Length"
/* Standard.      RFC7231, Section 3.1.4.2 */
#define MHD_HTTP_HEADER_CONTENT_LOCATION "Content-Location"
/* Standard.      RFC7233, Section 4.2 */
#define MHD_HTTP_HEADER_CONTENT_RANGE "Content-Range"
/* Standard.      RFC7231, Section 3.1.1.5 */
#define MHD_HTTP_HEADER_CONTENT_TYPE "Content-Type"
/* Standard.      RFC7231, Section 7.1.1.2 */
#define MHD_HTTP_HEADER_DATE "Date"
/* Standard.      RFC7232, Section 2.3 */
#define MHD_HTTP_HEADER_ETAG "ETag"
/* Standard.      RFC7231, Section 5.1.1 */
#define MHD_HTTP_HEADER_EXPECT "Expect"
/* Standard.      RFC7234, Section 5.3 */
#define MHD_HTTP_HEADER_EXPIRES "Expires"
/* Standard.      RFC7231, Section 5.5.1 */
#define MHD_HTTP_HEADER_FROM "From"
/* Standard.      RFC7230, Section 5.4 */
#define MHD_HTTP_HEADER_HOST "Host"
/* Standard.      RFC7232, Section 3.1 */
#define MHD_HTTP_HEADER_IF_MATCH "If-Match"
/* Standard.      RFC7232, Section 3.3 */
#define MHD_HTTP_HEADER_IF_MODIFIED_SINCE "If-Modified-Since"
/* Standard.      RFC7232, Section 3.2 */
#define MHD_HTTP_HEADER_IF_NONE_MATCH "If-None-Match"
/* Standard.      RFC7233, Section 3.2 */
#define MHD_HTTP_HEADER_IF_RANGE "If-Range"
/* Standard.      RFC7232, Section 3.4 */
#define MHD_HTTP_HEADER_IF_UNMODIFIED_SINCE "If-Unmodified-Since"
/* Standard.      RFC7232, Section 2.2 */
#define MHD_HTTP_HEADER_LAST_MODIFIED "Last-Modified"
/* Standard.      RFC7231, Section 7.1.2 */
#define MHD_HTTP_HEADER_LOCATION "Location"
/* Standard.      RFC7231, Section 5.1.2 */
#define MHD_HTTP_HEADER_MAX_FORWARDS "Max-Forwards"
/* Standard.      RFC7231, Appendix A.1 */
#define MHD_HTTP_HEADER_MIME_VERSION "MIME-Version"
/* Standard.      RFC7234, Section 5.4 */
#define MHD_HTTP_HEADER_PRAGMA "Pragma"
/* Standard.      RFC7235, Section 4.3 */
#define MHD_HTTP_HEADER_PROXY_AUTHENTICATE "Proxy-Authenticate"
/* Standard.      RFC7235, Section 4.4 */
#define MHD_HTTP_HEADER_PROXY_AUTHORIZATION "Proxy-Authorization"
/* Standard.      RFC7233, Section 3.1 */
#define MHD_HTTP_HEADER_RANGE "Range"
/* Standard.      RFC7231, Section 5.5.2 */
#define MHD_HTTP_HEADER_REFERER "Referer"
/* Standard.      RFC7231, Section 7.1.3 */
#define MHD_HTTP_HEADER_RETRY_AFTER "Retry-After"
/* Standard.      RFC7231, Section 7.4.2 */
#define MHD_HTTP_HEADER_SERVER "Server"
/* Standard.      RFC7230, Section 4.3 */
#define MHD_HTTP_HEADER_TE "TE"
/* Standard.      RFC7230, Section 4.4 */
#define MHD_HTTP_HEADER_TRAILER "Trailer"
/* Standard.      RFC7230, Section 3.3.1 */
#define MHD_HTTP_HEADER_TRANSFER_ENCODING "Transfer-Encoding"
/* Standard.      RFC7230, Section 6.7 */
#define MHD_HTTP_HEADER_UPGRADE "Upgrade"
/* Standard.      RFC7231, Section 5.5.3 */
#define MHD_HTTP_HEADER_USER_AGENT "User-Agent"
/* Standard.      RFC7231, Section 7.1.4 */
#define MHD_HTTP_HEADER_VARY "Vary"
/* Standard.      RFC7230, Section 5.7.1 */
#define MHD_HTTP_HEADER_VIA "Via"
/* Standard.      RFC7235, Section 4.1 */
#define MHD_HTTP_HEADER_WWW_AUTHENTICATE "WWW-Authenticate"
/* Standard.      RFC7234, Section 5.5 */
#define MHD_HTTP_HEADER_WARNING "Warning"

/* Additional HTTP headers. */
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_A_IM "A-IM"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_ACCEPT_ADDITIONS "Accept-Additions"
/* Informational. RFC7089 */
#define MHD_HTTP_HEADER_ACCEPT_DATETIME "Accept-Datetime"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_ACCEPT_FEATURES "Accept-Features"
/* No category.   RFC5789 */
#define MHD_HTTP_HEADER_ACCEPT_PATCH "Accept-Patch"
/* Standard.      RFC7639, Section 2 */
#define MHD_HTTP_HEADER_ALPN "ALPN"
/* Standard.      RFC7838 */
#define MHD_HTTP_HEADER_ALT_SVC "Alt-Svc"
/* Standard.      RFC7838 */
#define MHD_HTTP_HEADER_ALT_USED "Alt-Used"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_ALTERNATES "Alternates"
/* No category.   RFC4437 */
#define MHD_HTTP_HEADER_APPLY_TO_REDIRECT_REF "Apply-To-Redirect-Ref"
/* Experimental.  RFC8053, Section 4 */
#define MHD_HTTP_HEADER_AUTHENTICATION_CONTROL "Authentication-Control"
/* Standard.      RFC7615, Section 3 */
#define MHD_HTTP_HEADER_AUTHENTICATION_INFO "Authentication-Info"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_C_EXT "C-Ext"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_C_MAN "C-Man"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_C_OPT "C-Opt"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_C_PEP "C-PEP"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_C_PEP_INFO "C-PEP-Info"
/* Standard.      RFC7809, Section 7.1 */
#define MHD_HTTP_HEADER_CALDAV_TIMEZONES "CalDAV-Timezones"
/* Obsoleted.     RFC2068; RFC2616 */
#define MHD_HTTP_HEADER_CONTENT_BASE "Content-Base"
/* Standard.      RFC6266 */
#define MHD_HTTP_HEADER_CONTENT_DISPOSITION "Content-Disposition"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_CONTENT_ID "Content-ID"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_CONTENT_MD5 "Content-MD5"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_CONTENT_SCRIPT_TYPE "Content-Script-Type"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_CONTENT_STYLE_TYPE "Content-Style-Type"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_CONTENT_VERSION "Content-Version"
/* Standard.      RFC6265 */
#define MHD_HTTP_HEADER_COOKIE "Cookie"
/* Obsoleted.     RFC2965; RFC6265 */
#define MHD_HTTP_HEADER_COOKIE2 "Cookie2"
/* Standard.      RFC5323 */
#define MHD_HTTP_HEADER_DASL "DASL"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_DAV "DAV"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_DEFAULT_STYLE "Default-Style"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_DELTA_BASE "Delta-Base"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_DEPTH "Depth"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_DERIVED_FROM "Derived-From"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_DESTINATION "Destination"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_DIFFERENTIAL_ID "Differential-ID"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_DIGEST "Digest"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_EXT "Ext"
/* Standard.      RFC7239 */
#define MHD_HTTP_HEADER_FORWARDED "Forwarded"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_GETPROFILE "GetProfile"
/* Experimental.  RFC7486, Section 6.1.1 */
#define MHD_HTTP_HEADER_HOBAREG "Hobareg"
/* Standard.      RFC7540, Section 3.2.1 */
#define MHD_HTTP_HEADER_HTTP2_SETTINGS "HTTP2-Settings"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_IM "IM"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_IF "If"
/* Standard.      RFC6638 */
#define MHD_HTTP_HEADER_IF_SCHEDULE_TAG_MATCH "If-Schedule-Tag-Match"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_KEEP_ALIVE "Keep-Alive"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_LABEL "Label"
/* No category.   RFC5988 */
#define MHD_HTTP_HEADER_LINK "Link"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_LOCK_TOKEN "Lock-Token"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_MAN "Man"
/* Informational. RFC7089 */
#define MHD_HTTP_HEADER_MEMENTO_DATETIME "Memento-Datetime"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_METER "Meter"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_NEGOTIATE "Negotiate"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_OPT "Opt"
/* Experimental.  RFC8053, Section 3 */
#define MHD_HTTP_HEADER_OPTIONAL_WWW_AUTHENTICATE "Optional-WWW-Authenticate"
/* Standard.      RFC4229 */
#define MHD_HTTP_HEADER_ORDERING_TYPE "Ordering-Type"
/* Standard.      RFC6454 */
#define MHD_HTTP_HEADER_ORIGIN "Origin"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_OVERWRITE "Overwrite"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_P3P "P3P"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PEP "PEP"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PICS_LABEL "PICS-Label"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PEP_INFO "Pep-Info"
/* Standard.      RFC4229 */
#define MHD_HTTP_HEADER_POSITION "Position"
/* Standard.      RFC7240 */
#define MHD_HTTP_HEADER_PREFER "Prefer"
/* Standard.      RFC7240 */
#define MHD_HTTP_HEADER_PREFERENCE_APPLIED "Preference-Applied"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROFILEOBJECT "ProfileObject"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROTOCOL "Protocol"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROTOCOL_INFO "Protocol-Info"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROTOCOL_QUERY "Protocol-Query"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROTOCOL_REQUEST "Protocol-Request"
/* Standard.      RFC7615, Section 4 */
#define MHD_HTTP_HEADER_PROXY_AUTHENTICATION_INFO "Proxy-Authentication-Info"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROXY_FEATURES "Proxy-Features"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PROXY_INSTRUCTION "Proxy-Instruction"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_PUBLIC "Public"
/* Standard.      RFC7469 */
#define MHD_HTTP_HEADER_PUBLIC_KEY_PINS "Public-Key-Pins"
/* Standard.      RFC7469 */
#define MHD_HTTP_HEADER_PUBLIC_KEY_PINS_REPORT_ONLY "Public-Key-Pins-Report-Only"
/* No category.   RFC4437 */
#define MHD_HTTP_HEADER_REDIRECT_REF "Redirect-Ref"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_SAFE "Safe"
/* Standard.      RFC6638 */
#define MHD_HTTP_HEADER_SCHEDULE_REPLY "Schedule-Reply"
/* Standard.      RFC6638 */
#define MHD_HTTP_HEADER_SCHEDULE_TAG "Schedule-Tag"
/* Standard.      RFC6455 */
#define MHD_HTTP_HEADER_SEC_WEBSOCKET_ACCEPT "Sec-WebSocket-Accept"
/* Standard.      RFC6455 */
#define MHD_HTTP_HEADER_SEC_WEBSOCKET_EXTENSIONS "Sec-WebSocket-Extensions"
/* Standard.      RFC6455 */
#define MHD_HTTP_HEADER_SEC_WEBSOCKET_KEY "Sec-WebSocket-Key"
/* Standard.      RFC6455 */
#define MHD_HTTP_HEADER_SEC_WEBSOCKET_PROTOCOL "Sec-WebSocket-Protocol"
/* Standard.      RFC6455 */
#define MHD_HTTP_HEADER_SEC_WEBSOCKET_VERSION "Sec-WebSocket-Version"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_SECURITY_SCHEME "Security-Scheme"
/* Standard.      RFC6265 */
#define MHD_HTTP_HEADER_SET_COOKIE "Set-Cookie"
/* Obsoleted.     RFC2965; RFC6265 */
#define MHD_HTTP_HEADER_SET_COOKIE2 "Set-Cookie2"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_SETPROFILE "SetProfile"
/* Standard.      RFC5023 */
#define MHD_HTTP_HEADER_SLUG "SLUG"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_SOAPACTION "SoapAction"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_STATUS_URI "Status-URI"
/* Standard.      RFC6797 */
#define MHD_HTTP_HEADER_STRICT_TRANSPORT_SECURITY "Strict-Transport-Security"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_SURROGATE_CAPABILITY "Surrogate-Capability"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_SURROGATE_CONTROL "Surrogate-Control"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_TCN "TCN"
/* Standard.      RFC4918 */
#define MHD_HTTP_HEADER_TIMEOUT "Timeout"
/* Standard.      RFC8030, Section 5.4 */
#define MHD_HTTP_HEADER_TOPIC "Topic"
/* Standard.      RFC8030, Section 5.2 */
#define MHD_HTTP_HEADER_TTL "TTL"
/* Standard.      RFC8030, Section 5.3 */
#define MHD_HTTP_HEADER_URGENCY "Urgency"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_URI "URI"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_VARIANT_VARY "Variant-Vary"
/* No category.   RFC4229 */
#define MHD_HTTP_HEADER_WANT_DIGEST "Want-Digest"
/* Informational. RFC7034 */
#define MHD_HTTP_HEADER_X_FRAME_OPTIONS "X-Frame-Options"

/* Some provisional headers. */
#define MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN "Access-Control-Allow-Origin"
/** @} */ /* end of group headers */


/**
 * A client has requested the given url using the given method
 * (#MHD_HTTP_METHOD_GET, #MHD_HTTP_METHOD_PUT,
 * #MHD_HTTP_METHOD_DELETE, #MHD_HTTP_METHOD_POST, etc).  The callback
 * must initialize @a rhp to provide further callbacks which will
 * process the request further and ultimately to provide the response
 * to give back to the client, or return #MHD_NO.
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param url the requested url (without arguments after "?")
 * @param method the HTTP method used (#MHD_HTTP_METHOD_GET,
 *        #MHD_HTTP_METHOD_PUT, etc.)
 * @return action how to proceed, NULL
 *         if the socket must be closed due to a serios
 *         error while handling the request
 */
typedef const struct MHD_Action *
(*MHD_RequestCallback) (void *cls,
			struct MHD_Request *request,
			const char *url,
			enum MHD_Method method);


/**
 * Create (but do not yet start) an MHD daemon.
 * Usually, you will want to set various options before
 * starting the daemon with #MHD_daemon_start().
 *
 * @param cb function to be called for incoming requests
 * @param cb_cls closure for @a cb
 * @return NULL on error
 */
_MHD_EXTERN struct MHD_Daemon *
MHD_daemon_create (MHD_RequestCallback cb,
		   void *cb_cls)
  MHD_NONNULL(1);


/**
 * Start a webserver.
 *
 * @param daemon daemon to start; you can no longer set
 *        options on this daemon after this call!
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_start (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Stop accepting connections from the listening socket.  Allows
 * clients to continue processing, but stops accepting new
 * connections.  Note that the caller is responsible for closing the
 * returned socket; however, if MHD is run using threads (anything but
 * external select mode), it must not be closed until AFTER
 * #MHD_stop_daemon has been called (as it is theoretically possible
 * that an existing thread is still using it).
 *
 * Note that some thread modes require the caller to have passed
 * #MHD_USE_ITC when using this API.  If this daemon is
 * in one of those modes and this option was not given to
 * #MHD_start_daemon, this function will return #MHD_INVALID_SOCKET.
 *
 * @param daemon daemon to stop accepting new connections for
 * @return old listen socket on success, #MHD_INVALID_SOCKET if
 *         the daemon was already not listening anymore, or
 *         was never started
 * @ingroup specialized
 */
_MHD_EXTERN MHD_socket
MHD_daemon_quiesce (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Shutdown and destroy an HTTP daemon.
 *
 * @param daemon daemon to stop
 * @ingroup event
 */
_MHD_EXTERN void
MHD_daemon_destroy (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Add another client connection to the set of connections managed by
 * MHD.  This API is usually not needed (since MHD will accept inbound
 * connections on the server socket).  Use this API in special cases,
 * for example if your HTTP server is behind NAT and needs to connect
 * out to the HTTP client, or if you are building a proxy.
 *
 * If you use this API in conjunction with a internal select or a
 * thread pool, you must set the option #MHD_USE_ITC to ensure that
 * the freshly added connection is immediately processed by MHD.
 *
 * The given client socket will be managed (and closed!) by MHD after
 * this call and must no longer be used directly by the application
 * afterwards.
 *
 * @param daemon daemon that manages the connection
 * @param client_socket socket to manage (MHD will expect
 *        to receive an HTTP request from this socket next).
 * @param addr IP address of the client
 * @param addrlen number of bytes in @a addr
 * @return #MHD_SC_OK on success
 *        The socket will be closed in any case; `errno` is
 *        set to indicate further details about the error.
 * @ingroup specialized
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_add_connection (struct MHD_Daemon *daemon,
			   MHD_socket client_socket,
			   const struct sockaddr *addr,
			   socklen_t addrlen)
  MHD_NONNULL(1);


/**
 * Obtain the `select()` sets for this daemon.  Daemon's FDs will be
 * added to fd_sets. To get only daemon FDs in fd_sets, call FD_ZERO
 * for each fd_set before calling this function. FD_SETSIZE is assumed
 * to be platform's default.
 *
 * This function should only be called in when MHD is configured to
 * use external select with 'select()' or with 'epoll'.  In the latter
 * case, it will only add the single 'epoll()' file descriptor used by
 * MHD to the sets.  It's necessary to use #MHD_get_timeout() in
 * combination with this function.
 *
 * This function must be called only for daemon started without
 * #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value); can be NULL
 * @return #MHD_SC_OK on success, otherwise error code
 * @ingroup event
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_get_fdset (struct MHD_Daemon *daemon,
		      fd_set *read_fd_set,
		      fd_set *write_fd_set,
		      fd_set *except_fd_set,
		      MHD_socket *max_fd)
  MHD_NONNULL(1,2,3,4);


/**
 * Obtain the `select()` sets for this daemon.  Daemon's FDs will be
 * added to fd_sets. To get only daemon FDs in fd_sets, call FD_ZERO
 * for each fd_set before calling this function.
 *
 * Passing custom FD_SETSIZE as @a fd_setsize allow usage of
 * larger/smaller than platform's default fd_sets.
 *
 * This function should only be called in when MHD is configured to
 * use external select with 'select()' or with 'epoll'.  In the latter
 * case, it will only add the single 'epoll' file descriptor used by
 * MHD to the sets.  It's necessary to use #MHD_get_timeout() in
 * combination with this function.
 *
 * This function must be called only for daemon started
 * without #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value); can be NULL
 * @param fd_setsize value of FD_SETSIZE
 * @return #MHD_SC_OK on success, otherwise error code
 * @ingroup event
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_get_fdset2 (struct MHD_Daemon *daemon,
		       fd_set *read_fd_set,
		       fd_set *write_fd_set,
		       fd_set *except_fd_set,
		       MHD_socket *max_fd,
		       unsigned int fd_setsize)
  MHD_NONNULL(1,2,3,4);


/**
 * Obtain the `select()` sets for this daemon.  Daemon's FDs will be
 * added to fd_sets. To get only daemon FDs in fd_sets, call FD_ZERO
 * for each fd_set before calling this function. Size of fd_set is
 * determined by current value of FD_SETSIZE.  It's necessary to use
 * #MHD_get_timeout() in combination with this function.
 *
 * This function could be called only for daemon started
 * without #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to get sets from
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @param max_fd increased to largest FD added (if larger
 *               than existing value); can be NULL
 * @return #MHD_YES on success, #MHD_NO if this
 *         daemon was not started with the right
 *         options for this call or any FD didn't
 *         fit fd_set.
 * @ingroup event
 */
#define MHD_daemon_get_fdset(daemon,read_fd_set,write_fd_set,except_fd_set,max_fd) \
  MHD_get_fdset2((daemon),(read_fd_set),(write_fd_set),(except_fd_set),(max_fd),FD_SETSIZE)


/**
 * Obtain timeout value for polling function for this daemon.
 * This function set value to amount of milliseconds for which polling
 * function (`select()` or `poll()`) should at most block, not the
 * timeout value set for connections.
 * It is important to always use this function, even if connection
 * timeout is not set, as in some cases MHD may already have more
 * data to process on next turn (data pending in TLS buffers,
 * connections are already ready with epoll etc.) and returned timeout
 * will be zero.
 *
 * @param daemon daemon to query for timeout
 * @param timeout set to the timeout (in milliseconds)
 * @return #MHD_SC_OK on success, #MHD_SC_NO_TIMEOUT if timeouts are
 *        not used (or no connections exist that would
 *        necessitate the use of a timeout right now), otherwise
 *        an error code
 * @ingroup event
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_get_timeout (struct MHD_Daemon *daemon,
			MHD_UNSIGNED_LONG_LONG *timeout)
  MHD_NONNULL(1,2);


/**
 * Run webserver operations (without blocking unless in client
 * callbacks).  This method should be called by clients in combination
 * with #MHD_get_fdset if the client-controlled select method is used
 * and #MHD_get_timeout().
 *
 * This function is a convenience method, which is useful if the
 * fd_sets from #MHD_get_fdset were not directly passed to `select()`;
 * with this function, MHD will internally do the appropriate `select()`
 * call itself again.  While it is always safe to call #MHD_run (if
 * #MHD_USE_INTERNAL_POLLING_THREAD is not set), you should call
 * #MHD_run_from_select if performance is important (as it saves an
 * expensive call to `select()`).
 *
 * @param daemon daemon to run
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_run (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Run webserver operations. This method should be called by clients
 * in combination with #MHD_get_fdset and #MHD_get_timeout() if the
 * client-controlled select method is used.
 *
 * You can use this function instead of #MHD_run if you called
 * `select()` on the result from #MHD_get_fdset.  File descriptors in
 * the sets that are not controlled by MHD will be ignored.  Calling
 * this function instead of #MHD_run is more efficient as MHD will not
 * have to call `select()` again to determine which operations are
 * ready.
 *
 * This function cannot be used with daemon started with
 * #MHD_USE_INTERNAL_POLLING_THREAD flag.
 *
 * @param daemon daemon to run select loop for
 * @param read_fd_set read set
 * @param write_fd_set write set
 * @param except_fd_set except set
 * @return #MHD_SC_OK on success
 * @ingroup event
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_run_from_select (struct MHD_Daemon *daemon,
			    const fd_set *read_fd_set,
			    const fd_set *write_fd_set,
			    const fd_set *except_fd_set)
  MHD_NONNULL(1,2,3,4);


/* ********************* daemon options ************** */


/**
 * Type of a callback function used for logging by MHD.
 *
 * @param cls closure
 * @param sc status code of the event
 * @param fm format string (`printf()`-style)
 * @param ap arguments to @a fm
 * @ingroup logging
 */
typedef void
(*MHD_LoggingCallback)(void *cls,
		       enum MHD_StatusCode sc,
		       const char *fm,
		       va_list ap);


/**
 * Set logging method.  Specify NULL to disable logging entirely.  By
 * default (if this option is not given), we log error messages to
 * stderr.
 *
 * @param daemon which instance to setup logging for
 * @param logger function to invoke
 * @param logger_cls closure for @a logger
 */
_MHD_EXTERN void
MHD_daemon_set_logger (struct MHD_Daemon *daemon,
		       MHD_LoggingCallback logger,
		       void *logger_cls)
  MHD_NONNULL(1);


/**
 * Convenience macro used to disable logging.
 *
 * @param daemon which instance to disable logging for
 */
#define MHD_daemon_disable_logging(daemon) MHD_daemon_set_logger (daemon, NULL, NULL)


/**
 * Suppress use of "Date" header as this system has no RTC.
 *
 * @param daemon which instance to disable clock for.
 */
_MHD_EXTERN void
MHD_daemon_suppress_date_no_clock (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Disable use of inter-thread communication channel.
 * #MHD_daemon_disable_itc() can be used with
 * #MHD_daemon_thread_internal() to perform some additional
 * optimizations (in particular, not creating a pipe for IPC
 * signalling).  If it is used, certain functions like
 * #MHD_daemon_quiesce() or #MHD_connection_add() or
 * #MHD_action_suspend() cannot be used anymore.
 * #MHD_daemon_disable_itc() is not beneficial on platforms where
 * select()/poll()/other signal shutdown() of a listen socket.
 *
 * You should only use this function if you are sure you do
 * satisfy all of its requirements and need a generally minor
 * boost in performance.
 *
 * @param daemon which instance to disable itc for
 */
_MHD_EXTERN void
MHD_daemon_disable_itc (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Enable `turbo`.  Disables certain calls to `shutdown()`,
 * enables aggressive non-blocking optimistic reads and
 * other potentially unsafe optimizations.
 * Most effects only happen with #MHD_ELS_EPOLL.
 *
 * @param daemon which instance to enable turbo for
 */
_MHD_EXTERN void
MHD_daemon_enable_turbo (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Disable #MHD_action_suspend() functionality.
 *
 * You should only use this function if you are sure you do
 * satisfy all of its requirements and need a generally minor
 * boost in performance.
 *
 * @param daemon which instance to disable suspend for
 */
_MHD_EXTERN void
MHD_daemon_disallow_suspend_resume (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * You need to set this option if you want to disable use of HTTP "Upgrade".
 * "Upgrade" may require usage of additional internal resources,
 * which we can avoid providing if they will not be used.
 *
 * You should only use this function if you are sure you do
 * satisfy all of its requirements and need a generally minor
 * boost in performance.
 *
 * @param daemon which instance to enable suspend/resume for
 */
_MHD_EXTERN void
MHD_daemon_disallow_upgrade (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Possible levels of enforcement for TCP_FASTOPEN.
 */
enum MHD_FastOpenMethod
{
  /**
   * Disable use of TCP_FASTOPEN.
   */
  MHD_FOM_DISABLE = -1,

  /**
   * Enable TCP_FASTOPEN where supported (Linux with a kernel >= 3.6).
   * This is the default.
   */
  MHD_FOM_AUTO = 0,

  /**
   * If TCP_FASTOPEN is not available, return #MHD_NO.
   * Also causes #MHD_daemon_start() to fail if setting
   * the option fails later.
   */
  MHD_FOM_REQUIRE = 1
};


/**
 * Configure TCP_FASTOPEN option, including setting a
 * custom @a queue_length.
 *
 * Note that having a larger queue size can cause resource exhaustion
 * attack as the TCP stack has to now allocate resources for the SYN
 * packet along with its DATA.
 *
 * @param daemon which instance to configure TCP_FASTOPEN for
 * @param fom under which conditions should we use TCP_FASTOPEN?
 * @param queue_length queue length to use, default is 50 if this
 *        option is never given.
 * @return #MHD_YES upon success, #MHD_NO if #MHD_FOM_REQUIRE was
 *         given, but TCP_FASTOPEN is not available on the platform
 */
_MHD_EXTERN enum MHD_Bool
MHD_daemon_tcp_fastopen (struct MHD_Daemon *daemon,
			 enum MHD_FastOpenMethod fom,
			 unsigned int queue_length)
  MHD_NONNULL(1);


/**
 * Address family to be used by MHD.
 */
enum MHD_AddressFamily
{
  /**
   * Option not given, do not listen at all
   * (unless listen socket or address specified by
   * other means).
   */
  MHD_AF_NONE = 0,

  /**
   * Pick "best" available method automatically.
   */
  MHD_AF_AUTO,

  /**
   * Use IPv4.
   */
  MHD_AF_INET4,

  /**
   * Use IPv6.
   */
  MHD_AF_INET6,

  /**
   * Use dual stack.
   */
  MHD_AF_DUAL
};


/**
 * Bind to the given TCP port and address family.
 *
 * Ineffective in conjunction with #MHD_daemon_listen_socket().
 * Ineffective in conjunction with #MHD_daemon_bind_sa().
 *
 * If neither this option nor the other two mentioned above
 * is specified, MHD will simply not listen on any socket!
 *
 * @param daemon which instance to configure the TCP port for
 * @param af address family to use
 * @param port port to use, 0 to bind to a random (free) port
 */
_MHD_EXTERN void
MHD_daemon_bind_port (struct MHD_Daemon *daemon,
		      enum MHD_AddressFamily af,
		      uint16_t port)
  MHD_NONNULL(1);


/**
 * Bind to the given socket address.
 * Ineffective in conjunction with #MHD_daemon_listen_socket().
 *
 * @param daemon which instance to configure the binding address for
 * @param sa address to bind to; can be IPv4 (AF_INET), IPv6 (AF_INET6)
 *        or even a UNIX domain socket (AF_UNIX)
 * @param sa_len number of bytes in @a sa
 */
_MHD_EXTERN void
MHD_daemon_bind_socket_address (struct MHD_Daemon *daemon,
				const struct sockaddr *sa,
				size_t sa_len)
  MHD_NONNULL(1);


/**
 * Use the given backlog for the listen() call.
 * Ineffective in conjunction with #MHD_daemon_listen_socket().
 *
 * @param daemon which instance to configure the backlog for
 * @param listen_backlog backlog to use
 */
_MHD_EXTERN void
MHD_daemon_listen_backlog (struct MHD_Daemon *daemon,
			   int listen_backlog)
  MHD_NONNULL(1);


/**
 * If present true, allow reusing address:port socket (by using
 * SO_REUSEPORT on most platform, or platform-specific ways).  If
 * present and set to false, disallow reusing address:port socket
 * (does nothing on most plaform, but uses SO_EXCLUSIVEADDRUSE on
 * Windows).
 * Ineffective in conjunction with #MHD_daemon_listen_socket().
 *
 * @param daemon daemon to configure address reuse for
 */
_MHD_EXTERN void
MHD_daemon_listen_allow_address_reuse (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Accept connections from the given socket.  Socket
 * must be a TCP or UNIX domain (stream) socket.
 *
 * Unless -1 is given, this disables other listen options, including
 * #MHD_daemon_bind_sa(), #MHD_daemon_bind_port(),
 * #MHD_daemon_listen_queue() and
 * #MHD_daemon_listen_allow_address_reuse().
 *
 * @param daemon daemon to set listen socket for
 * @param listen_socket listen socket to use,
 *        MHD_INVALID_SOCKET value will cause this call to be
 *        ignored (other binding options may still be effective)
 */
_MHD_EXTERN void
MHD_daemon_listen_socket (struct MHD_Daemon *daemon,
			  MHD_socket listen_socket)
  MHD_NONNULL(1);


/**
 * Event loop syscalls supported by MHD.
 */
enum MHD_EventLoopSyscall
{
  /**
   * Automatic selection of best-available method. This is also the
   * default.
   */
  MHD_ELS_AUTO = 0,

  /**
   * Use select().
   */
  MHD_ELS_SELECT = 1,

  /**
   * Use poll().
   */
  MHD_ELS_POLL = 2,

  /**
   * Use epoll().
   */
  MHD_ELS_EPOLL = 3
};


/**
 * Force use of a particular event loop system call.
 *
 * @param daemon daemon to set event loop style for
 * @param els event loop syscall to use
 * @return #MHD_NO on failure, #MHD_YES on success
 */
_MHD_EXTERN enum MHD_Bool
MHD_daemon_event_loop (struct MHD_Daemon *daemon,
		       enum MHD_EventLoopSyscall els)
  MHD_NONNULL(1);


/**
 * Protocol strictness enforced by MHD on clients.
 */
enum MHD_ProtocolStrictLevel
{
  /**
   * Be particularly permissive about the protocol, allowing slight
   * deviations that are technically not allowed by the
   * RFC. Specifically, at the moment, this flag causes MHD to allow
   * spaces in header field names. This is disallowed by the standard.
   * It is not recommended to set this value on publicly available
   * servers as it may potentially lower level of protection.
   */
  MHD_PSL_PERMISSIVE = -1,

  /**
   * Sane level of protocol enforcement for production use.
   */
  MHD_PSL_DEFAULT = 0,

  /**
   * Be strict about the protocol (as opposed to as tolerant as
   * possible).  Specifically, at the moment, this flag causes MHD to
   * reject HTTP 1.1 connections without a "Host" header.  This is
   * required by the standard, but of course in violation of the "be
   * as liberal as possible in what you accept" norm.  It is
   * recommended to set this if you are testing clients against
   * MHD, and to use default in production.
   */
  MHD_PSL_STRICT = 1
};


/**
 * Set how strictly MHD will enforce the HTTP protocol.
 *
 * @param daemon daemon to configure strictness for
 * @param sl how strict should we be
 */
_MHD_EXTERN void
MHD_daemon_protocol_strict_level (struct MHD_Daemon *daemon,
				  enum MHD_ProtocolStrictLevel sl)
  MHD_NONNULL(1);


/**
 * Use SHOUTcast.  This will cause the response to begin
 * with the SHOUTcast "ICY" line instad of "HTTP".
 *
 * @param daemon daemon to set SHOUTcast option for
 */
_MHD_EXTERN void
MHD_daemon_enable_shoutcast (struct MHD_Daemon *daemon)
  MHD_NONNULL(1);


/**
 * Enable and configure TLS.
 *
 * @param daemon which instance should be configured
 * @param tls_backend which TLS backend should be used,
 *    currently only "gnutls" is supported.  You can
 *    also specify NULL for best-available (which is the default).
 * @param ciphers which ciphers should be used by TLS, default is
 *     "NORMAL"
 * @return status code, #MHD_SC_OK upon success
 *     #MHD_TLS_BACKEND_UNSUPPORTED if the @a backend is unknown
 *     #MHD_TLS_DISABLED if this build of MHD does not support TLS
 *     #MHD_TLS_CIPHERS_INVALID if the given @a ciphers are not supported
 *     by this backend
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_set_tls_backend (struct MHD_Daemon *daemon,
			    const char *tls_backend,
			    const char *ciphers)
  MHD_NONNULL(1);


/**
 * Provide TLS key and certificate data in-memory.
 *
 * @param daemon which instance should be configured
 * @param mem_key private key (key.pem) to be used by the
 *     HTTPS daemon.  Must be the actual data in-memory, not a filename.
 * @param mem_cert certificate (cert.pem) to be used by the
 *     HTTPS daemon.  Must be the actual data in-memory, not a filename.
 * @param pass passphrase phrase to decrypt 'key.pem', NULL
 *     if @param mem_key is in cleartext already
 * @return #MHD_SC_OK upon success; TODO: define failure modes
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_tls_key_and_cert_from_memory (struct MHD_Daemon *daemon,
					 const char *mem_key,
					 const char *mem_cert,
					 const char *pass)
  MHD_NONNULL(1,2,3);


/**
 * Configure DH parameters (dh.pem) to use for the TLS key
 * exchange.
 *
 * @param daemon daemon to configure tls for
 * @param dh parameters to use
 * @return #MHD_SC_OK upon success; TODO: define failure modes
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_tls_mem_dhparams (struct MHD_Daemon *daemon,
			     const char *dh)
  MHD_NONNULL(1);


/**
 * Function called to lookup the pre shared key (@a psk) for a given
 * HTTP connection based on the @a username.
 *
 * @param cls closure
 * @param connection the HTTPS connection
 * @param username the user name claimed by the other side
 * @param[out] psk to be set to the pre-shared-key; should be allocated with malloc(),
 *                 will be freed by MHD
 * @param[out] psk_size to be set to the number of bytes in @a psk
 * @return 0 on success, -1 on errors 
 */
typedef int
(*MHD_PskServerCredentialsCallback)(void *cls,
				    const struct MHD_Connection *connection,
				    const char *username,
				    void **psk,
				    size_t *psk_size);


/**
 * Configure PSK to use for the TLS key exchange.
 *
 * @param daemon daemon to configure tls for
 * @param psk_cb function to call to obtain pre-shared key
 * @param psk_cb_cls closure for @a psk_cb
 * @return #MHD_SC_OK upon success; TODO: define failure modes
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_set_tls_psk_callback (struct MHD_Daemon *daemon,
				 MHD_PskServerCredentialsCallback psk_cb,
				 void *psk_cb_cls)
  MHD_NONNULL(1);


/**
 * Memory pointer for the certificate (ca.pem) to be used by the
 * HTTPS daemon for client authentication.
 *
 * @param daemon daemon to configure tls for
 * @param mem_trust memory pointer to the certificate
 * @return #MHD_SC_OK upon success; TODO: define failure modes
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_tls_mem_trust (struct MHD_Daemon *daemon,
			  const char *mem_trust)
  MHD_NONNULL(1);


/**
 * Configure daemon credentials type for GnuTLS.
 *
 * @param gnutls_credentials must be a value of
 *   type `gnutls_credentials_type_t`
 * @return #MHD_SC_OK upon success; TODO: define failure modes
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_gnutls_credentials (struct MHD_Daemon *daemon,
			       int gnutls_credentials)
  MHD_NONNULL(1);


/**
 * Provide TLS key and certificate data via callback.
 *
 * Use a callback to determine which X.509 certificate should be used
 * for a given HTTPS connection.  This option provides an alternative
 * to #MHD_daemon_tls_key_and_cert_from_memory().  You must use this
 * version if multiple domains are to be hosted at the same IP address
 * using TLS's Server Name Indication (SNI) extension.  In this case,
 * the callback is expected to select the correct certificate based on
 * the SNI information provided.  The callback is expected to access
 * the SNI data using `gnutls_server_name_get()`.  Using this option
 * requires GnuTLS 3.0 or higher.
 *
 * @param daemon daemon to configure callback for
 * @param cb must be of type `gnutls_certificate_retrieve_function2 *`.
 * @return #MHD_SC_OK on success
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_gnutls_key_and_cert_from_callback (struct MHD_Daemon *daemon,
					      void *cb)
  MHD_NONNULL(1);


/**
 * Which threading mode should be used by MHD?
 */
enum MHD_ThreadingMode
{

  /**
   * MHD should create its own thread for listening and furthermore
   * create another thread per connection to handle requests.  Use
   * this if handling requests is CPU-intensive or blocking, your
   * application is thread-safe and you have plenty of memory (per
   * request).
   */
  MHD_TM_THREAD_PER_CONNECTION = -1,

  /**
   * Use an external event loop. This is the default.
   */
  MHD_TM_EXTERNAL_EVENT_LOOP = 0,

  /**
   * Run with one or more worker threads.  Any positive value
   * means that MHD should start that number of worker threads
   * (so > 1 is a thread pool) and distributed processing of
   * requests among the workers.
   *
   * A good way to express the use of a thread pool
   * in your code would be to write "MHD_TM_THREAD_POOL(4)"
   * to indicate four threads.
   *
   * If a positive value is set, * #MHD_daemon_run() and
   * #MHD_daemon_run_from_select() cannot be used.
   */
  MHD_TM_WORKER_THREADS = 1

};


/**
 * Use a thread pool of size @a n.
 *
 * @return an `enum MHD_ThreadingMode` for a thread pool of size @a n
 */
#define MHD_TM_THREAD_POOL(n) ((enum MHD_ThreadingMode)(n))


/**
 * Specify threading mode to use.
 *
 * @param daemon daemon to configure
 * @param tm mode to use (positive values indicate the
 *        number of worker threads to be used)
 */
_MHD_EXTERN void
MHD_daemon_threading_mode (struct MHD_Daemon *daemon,
			    enum MHD_ThreadingMode tm)
  MHD_NONNULL(1);


/**
 * Allow or deny a client to connect.
 *
 * @param cls closure
 * @param addr address information from the client
 * @param addrlen length of @a addr
 * @see #MHD_daemon_accept_policy()
 * @return #MHD_YES if connection is allowed, #MHD_NO if not
 */
typedef enum MHD_Bool
(*MHD_AcceptPolicyCallback) (void *cls,
                             const struct sockaddr *addr,
                             size_t addrlen);


/**
 * Set  a policy callback that accepts/rejects connections
 * based on the client's IP address.  This function will be called
 * before a connection object is created.
 *
 * @param daemon daemon to set policy for
 * @param apc function to call to check the policy
 * @param apc_cls closure for @a apc
 */
_MHD_EXTERN void
MHD_daemon_accept_policy (struct MHD_Daemon *daemon,
			  MHD_AcceptPolicyCallback apc,
			  void *apc_cls)
  MHD_NONNULL(1);


/**
 * Function called by MHD to allow the application to log
 * the full @a uri of a @a request.
 *
 * @param cls client-defined closure
 * @param uri the full URI from the HTTP request
 * @param request the HTTP request handle (headers are
 *         not yet available)
 * @return value to set for the "request_context" of @a request
 */
typedef void *
(*MHD_EarlyUriLogCallback)(void *cls,
			   const char *uri,
			   struct MHD_Request *request);


/**
 * Register a callback to be called first for every request
 * (before any parsing of the header).  Makes it easy to
 * log the full URL.
 *
 * @param daemon daemon for which to set the logger
 * @param cb function to call
 * @param cb_cls closure for @a cb
 */
_MHD_EXTERN void
MHD_daemon_set_early_uri_logger (struct MHD_Daemon *daemon,
				 MHD_EarlyUriLogCallback cb,
				 void *cb_cls)
  MHD_NONNULL(1);


/**
 * The `enum MHD_ConnectionNotificationCode` specifies types
 * of connection notifications.
 * @ingroup request
 */
enum MHD_ConnectionNotificationCode
{

  /**
   * A new connection has been started.
   * @ingroup request
   */
  MHD_CONNECTION_NOTIFY_STARTED = 0,

  /**
   * A connection is closed.
   * @ingroup request
   */
  MHD_CONNECTION_NOTIFY_CLOSED = 1

};


/**
 * Signature of the callback used by MHD to notify the
 * application about started/stopped connections
 *
 * @param cls client-defined closure
 * @param connection connection handle
 * @param socket_context socket-specific pointer where the
 *                       client can associate some state specific
 *                       to the TCP connection; note that this is
 *                       different from the "con_cls" which is per
 *                       HTTP request.  The client can initialize
 *                       during #MHD_CONNECTION_NOTIFY_STARTED and
 *                       cleanup during #MHD_CONNECTION_NOTIFY_CLOSED
 *                       and access in the meantime using
 *                       #MHD_CONNECTION_INFO_SOCKET_CONTEXT.
 * @param toe reason for connection notification
 * @see #MHD_OPTION_NOTIFY_CONNECTION
 * @ingroup request
 */
typedef void
(*MHD_NotifyConnectionCallback) (void *cls,
				 struct MHD_Connection *connection,
				 enum MHD_ConnectionNotificationCode toe);


/**
 * Register a function that should be called whenever a connection is
 * started or closed.
 *
 * @param daemon daemon to set callback for
 * @param ncc function to call to check the policy
 * @param ncc_cls closure for @a apc
 */
_MHD_EXTERN void
MHD_daemon_set_notify_connection (struct MHD_Daemon *daemon,
				  MHD_NotifyConnectionCallback ncc,
				  void *ncc_cls)
  MHD_NONNULL(1);


/**
 * Maximum memory size per connection.
 * Default is 32 kb (#MHD_POOL_SIZE_DEFAULT).
 * Values above 128k are unlikely to result in much benefit, as half
 * of the memory will be typically used for IO, and TCP buffers are
 * unlikely to support window sizes above 64k on most systems.
 *
 * @param daemon daemon to configure
 * @param memory_limit_b connection memory limit to use in bytes
 * @param memory_increment_b increment to use when growing the read buffer, must be smaller than @a memory_limit_b
 */
_MHD_EXTERN void
MHD_daemon_connection_memory_limit (struct MHD_Daemon *daemon,
				    size_t memory_limit_b,
				    size_t memory_increment_b)
  MHD_NONNULL(1);


/**
 * Desired size of the stack for threads created by MHD.  Use 0 for
 * system default.  Only useful if the selected threading mode
 * is not #MHD_TM_EXTERNAL_EVENT_LOOP.
 *
 * @param daemon daemon to configure
 * @param stack_limit_b stack size to use in bytes
 */
_MHD_EXTERN void
MHD_daemon_thread_stack_size (struct MHD_Daemon *daemon,
			      size_t stack_limit_b)
  MHD_NONNULL(1);


/**
 * Set maximum number of concurrent connections to accept.  If not
 * given, MHD will not enforce any limits (modulo running into
 * OS limits).  Values of 0 mean no limit.
 *
 * @param daemon daemon to configure
 * @param global_connection_limit maximum number of (concurrent)
          connections
 * @param ip_connection_limit limit on the number of (concurrent)
 *        connections made to the server from the same IP address.
 *        Can be used to prevent one IP from taking over all of
 *        the allowed connections.  If the same IP tries to
 *        establish more than the specified number of
 *        connections, they will be immediately rejected.
 */
_MHD_EXTERN void
MHD_daemon_connection_limits (struct MHD_Daemon *daemon,
			      unsigned int global_connection_limit,
			      unsigned int ip_connection_limit)
  MHD_NONNULL(1);


/**
 * After how many seconds of inactivity should a
 * connection automatically be timed out?
 * Use zero for no timeout, which is also the (unsafe!) default.
 *
 * @param daemon daemon to configure
 * @param timeout_s number of seconds of timeout to use
 */
_MHD_EXTERN void
MHD_daemon_connection_default_timeout (struct MHD_Daemon *daemon,
				       unsigned int timeout_s)
  MHD_NONNULL(1);


/**
 * Signature of functions performing unescaping of strings.
 * The return value must be "strlen(s)" and @a s  should be
 * updated.  Note that the unescape function must not lengthen @a s
 * (the result must be shorter than the input and still be
 * 0-terminated).
 *
 * @param cls closure
 * @param req the request for which unescaping is performed
 * @param[in,out] s string to unescape
 * @return number of characters in @a s (excluding 0-terminator)
 */
typedef size_t
(*MHD_UnescapeCallback) (void *cls,
			 struct MHD_Request *req,
			 char *s);


/**
 * Specify a function that should be called for unescaping escape
 * sequences in URIs and URI arguments.  Note that this function
 * will NOT be used by the `struct MHD_PostProcessor`.  If this
 * option is not specified, the default method will be used which
 * decodes escape sequences of the form "%HH".
 *
 * @param daemon daemon to configure
 * @param unescape_cb function to use, NULL for default
 * @param unescape_cb_cls closure for @a unescape_cb
 */
_MHD_EXTERN void
MHD_daemon_unescape_cb (struct MHD_Daemon *daemon,
			MHD_UnescapeCallback unescape_cb,
			void *unescape_cb_cls)
  MHD_NONNULL(1);


/**
 * Set random values to be used by the Digest Auth module.  Note that
 * the application must ensure that @a buf remains allocated and
 * unmodified while the deamon is running.
 *
 * @param daemon daemon to configure
 * @param buf_size number of bytes in @a buf
 * @param buf entropy buffer
 */
_MHD_EXTERN void
MHD_daemon_digest_auth_random (struct MHD_Daemon *daemon,
			       size_t buf_size,
			       const void *buf)
  MHD_NONNULL(1,3);


/**
 * Length of the internal array holding the map of the nonce and
 * the nonce counter.
 *
 * @param daemon daemon to configure
 * @param nc_length desired array length
 */
_MHD_EXTERN enum MHD_StatusCode
MHD_daemon_digest_auth_nc_length (struct MHD_Daemon *daemon,
				  size_t nc_length)
  MHD_NONNULL(1);


/* ********************* connection options ************** */


/**
 * Set custom timeout for the given connection.
 * Specified as the number of seconds.  Use zero for no timeout.
 * Calling this function will reset timeout timer.
 *
 * @param connection connection to configure timeout for
 * @param timeout_s new timeout in seconds
 */
_MHD_EXTERN void
MHD_connection_set_timeout (struct MHD_Connection *connection,
			    unsigned int timeout_s)
  MHD_NONNULL(1);


/* **************** Request handling functions ***************** */


/**
 * The `enum MHD_ValueKind` specifies the source of
 * the key-value pairs in the HTTP protocol.
 */
enum MHD_ValueKind
{

  /**
   * HTTP header (request/response).
   */
  MHD_HEADER_KIND = 1,

  /**
   * Cookies.  Note that the original HTTP header containing
   * the cookie(s) will still be available and intact.
   */
  MHD_COOKIE_KIND = 2,

  /**
   * POST data.  This is available only if a content encoding
   * supported by MHD is used (currently only URL encoding),
   * and only if the posted content fits within the available
   * memory pool.  Note that in that case, the upload data
   * given to the #MHD_AccessHandlerCallback will be
   * empty (since it has already been processed).
   */
  MHD_POSTDATA_KIND = 4,

  /**
   * GET (URI) arguments.
   */
  MHD_GET_ARGUMENT_KIND = 8,

  /**
   * HTTP footer (only for HTTP 1.1 chunked encodings).
   */
  MHD_FOOTER_KIND = 16
};



/**
 * Iterator over key-value pairs.  This iterator can be used to
 * iterate over all of the cookies, headers, or POST-data fields of a
 * request, and also to iterate over the headers that have been added
 * to a response.
 *
 * @param cls closure
 * @param kind kind of the header we are looking at
 * @param key key for the value, can be an empty string
 * @param value corresponding value, can be NULL
 * @return #MHD_YES to continue iterating,
 *         #MHD_NO to abort the iteration
 * @ingroup request
 */
typedef int
(*MHD_KeyValueIterator) (void *cls,
                         enum MHD_ValueKind kind,
                         const char *key,
                         const char *value);


/**
 * Get all of the headers from the request.
 *
 * @param request request to get values from
 * @param kind types of values to iterate over, can be a bitmask
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to @a iterator
 * @return number of entries iterated over
 * @ingroup request
 */
_MHD_EXTERN unsigned int
MHD_request_get_values (struct MHD_Request *request,
			enum MHD_ValueKind kind,
			MHD_KeyValueIterator iterator,
			void *iterator_cls)
  MHD_NONNULL(1);


/**
 * This function can be used to add an entry to the HTTP headers of a
 * request (so that the #MHD_request_get_values function will
 * return them -- and the `struct MHD_PostProcessor` will also see
 * them).  This maybe required in certain situations (see Mantis
 * #1399) where (broken) HTTP implementations fail to supply values
 * needed by the post processor (or other parts of the application).
 *
 * This function MUST only be called from within the
 * request callbacks (otherwise, access maybe improperly
 * synchronized).  Furthermore, the client must guarantee that the key
 * and value arguments are 0-terminated strings that are NOT freed
 * until the connection is closed.  (The easiest way to do this is by
 * passing only arguments to permanently allocated strings.).
 *
 * @param request the request for which a
 *  value should be set
 * @param kind kind of the value
 * @param key key for the value
 * @param value the value itself
 * @return #MHD_NO if the operation could not be
 *         performed due to insufficient memory;
 *         #MHD_YES on success
 * @ingroup request
 */
_MHD_EXTERN enum MHD_Bool
MHD_request_set_value (struct MHD_Request *request,
		       enum MHD_ValueKind kind,
		       const char *key,
		       const char *value)
  MHD_NONNULL(1,3,4);


/**
 * Get a particular header value.  If multiple
 * values match the kind, return any one of them.
 *
 * @param request request to get values from
 * @param kind what kind of value are we looking for
 * @param key the header to look for, NULL to lookup 'trailing' value without a key
 * @return NULL if no such item was found
 * @ingroup request
 */
_MHD_EXTERN const char *
MHD_request_lookup_value (struct MHD_Request *request,
			  enum MHD_ValueKind kind,
			  const char *key)
  MHD_NONNULL(1);



/**
 * @defgroup httpcode HTTP response codes.
 * These are the status codes defined for HTTP responses.
 * @{
 */
/* See http://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml */
enum MHD_HTTP_StatusCode {
  MHD_HTTP_CONTINUE = 100,
  MHD_HTTP_SWITCHING_PROTOCOLS = 101,
  MHD_HTTP_PROCESSING = 102,

  MHD_HTTP_OK = 200,
  MHD_HTTP_CREATED = 201,
  MHD_HTTP_ACCEPTED = 202,
  MHD_HTTP_NON_AUTHORITATIVE_INFORMATION = 203,
  MHD_HTTP_NO_CONTENT = 204,
  MHD_HTTP_RESET_CONTENT = 205,
  MHD_HTTP_PARTIAL_CONTENT = 206,
  MHD_HTTP_MULTI_STATUS = 207,
  MHD_HTTP_ALREADY_REPORTED = 208,

  MHD_HTTP_IM_USED = 226,

  MHD_HTTP_MULTIPLE_CHOICES = 300,
  MHD_HTTP_MOVED_PERMANENTLY = 301,
  MHD_HTTP_FOUND = 302,
  MHD_HTTP_SEE_OTHER = 303,
  MHD_HTTP_NOT_MODIFIED = 304,
  MHD_HTTP_USE_PROXY = 305,
  MHD_HTTP_SWITCH_PROXY = 306, /* IANA: unused */
  MHD_HTTP_TEMPORARY_REDIRECT = 307,
  MHD_HTTP_PERMANENT_REDIRECT = 308,

  MHD_HTTP_BAD_REQUEST = 400,
  MHD_HTTP_UNAUTHORIZED = 401,
  MHD_HTTP_PAYMENT_REQUIRED = 402,
  MHD_HTTP_FORBIDDEN = 403,
  MHD_HTTP_NOT_FOUND = 404,
  MHD_HTTP_METHOD_NOT_ALLOWED = 405,
  MHD_HTTP_NOT_ACCEPTABLE = 406,
/** @deprecated */
#define MHD_HTTP_METHOD_NOT_ACCEPTABLE \
  _MHD_DEPR_IN_MACRO("Value MHD_HTTP_METHOD_NOT_ACCEPTABLE is deprecated, use MHD_HTTP_NOT_ACCEPTABLE") MHD_HTTP_NOT_ACCEPTABLE
  MHD_HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
  MHD_HTTP_REQUEST_TIMEOUT = 408,
  MHD_HTTP_CONFLICT = 409,
  MHD_HTTP_GONE = 410,
  MHD_HTTP_LENGTH_REQUIRED = 411,
  MHD_HTTP_PRECONDITION_FAILED = 412,
  MHD_HTTP_PAYLOAD_TOO_LARGE = 413,
/** @deprecated */
#define MHD_HTTP_REQUEST_ENTITY_TOO_LARGE \
  _MHD_DEPR_IN_MACRO("Value MHD_HTTP_REQUEST_ENTITY_TOO_LARGE is deprecated, use MHD_HTTP_PAYLOAD_TOO_LARGE") MHD_HTTP_PAYLOAD_TOO_LARGE
  MHD_HTTP_URI_TOO_LONG = 414,
/** @deprecated */
#define MHD_HTTP_REQUEST_URI_TOO_LONG \
  _MHD_DEPR_IN_MACRO("Value MHD_HTTP_REQUEST_URI_TOO_LONG is deprecated, use MHD_HTTP_URI_TOO_LONG") MHD_HTTP_URI_TOO_LONG
  MHD_HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
  MHD_HTTP_RANGE_NOT_SATISFIABLE = 416,
/** @deprecated */
#define MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE \
  _MHD_DEPR_IN_MACRO("Value MHD_HTTP_REQUESTED_RANGE_NOT_SATISFIABLE is deprecated, use MHD_HTTP_RANGE_NOT_SATISFIABLE") MHD_HTTP_RANGE_NOT_SATISFIABLE
  MHD_HTTP_EXPECTATION_FAILED = 417,

  MHD_HTTP_MISDIRECTED_REQUEST = 421,
  MHD_HTTP_UNPROCESSABLE_ENTITY = 422,
  MHD_HTTP_LOCKED = 423,
  MHD_HTTP_FAILED_DEPENDENCY = 424,
  MHD_HTTP_UNORDERED_COLLECTION = 425, /* IANA: unused */
  MHD_HTTP_UPGRADE_REQUIRED = 426,

  MHD_HTTP_PRECONDITION_REQUIRED = 428,
  MHD_HTTP_TOO_MANY_REQUESTS = 429,
  MHD_HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,

  MHD_HTTP_NO_RESPONSE = 444, /* IANA: unused */

  MHD_HTTP_RETRY_WITH = 449, /* IANA: unused */
  MHD_HTTP_BLOCKED_BY_WINDOWS_PARENTAL_CONTROLS = 450,  /* IANA: unused */
  MHD_HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451,

  MHD_HTTP_INTERNAL_SERVER_ERROR = 500,
  MHD_HTTP_NOT_IMPLEMENTED = 501,
  MHD_HTTP_BAD_GATEWAY = 502,
  MHD_HTTP_SERVICE_UNAVAILABLE = 503,
  MHD_HTTP_GATEWAY_TIMEOUT = 504,
  MHD_HTTP_HTTP_VERSION_NOT_SUPPORTED = 505,
  MHD_HTTP_VARIANT_ALSO_NEGOTIATES = 506,
  MHD_HTTP_INSUFFICIENT_STORAGE = 507,
  MHD_HTTP_LOOP_DETECTED = 508,
  MHD_HTTP_BANDWIDTH_LIMIT_EXCEEDED = 509,  /* IANA: unused */
  MHD_HTTP_NOT_EXTENDED = 510,
  MHD_HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511

};


/**
 * Returns the string reason phrase for a response code.
 *
 * If we don't have a string for a status code, we give the first
 * message in that status code class.
 */
_MHD_EXTERN const char *
MHD_get_reason_phrase_for (enum MHD_HTTP_StatusCode code);

/** @} */ /* end of group httpcode */


/**
 * @defgroup versions HTTP versions
 * These strings should be used to match against the first line of the
 * HTTP header.
 * @{
 */
#define MHD_HTTP_VERSION_1_0 "HTTP/1.0"
#define MHD_HTTP_VERSION_1_1 "HTTP/1.1"

/** @} */ /* end of group versions */


/**
 * Suspend handling of network data for a given request.  This can
 * be used to dequeue a request from MHD's event loop for a while.
 *
 * If you use this API in conjunction with a internal select or a
 * thread pool, you must set the option #MHD_USE_ITC to
 * ensure that a resumed request is immediately processed by MHD.
 *
 * Suspended requests continue to count against the total number of
 * requests allowed (per daemon, as well as per IP, if such limits
 * are set).  Suspended requests will NOT time out; timeouts will
 * restart when the request handling is resumed.  While a
 * request is suspended, MHD will not detect disconnects by the
 * client.
 *
 * The only safe time to suspend a request is from either a
 * #MHD_RequestHeaderCallback, #MHD_UploadCallback, or a
 * #MHD_RequestfetchResponseCallback.  Suspending a request
 * at any other time will cause an assertion failure.
 *
 * Finally, it is an API violation to call #MHD_daemon_stop() while
 * having suspended requests (this will at least create memory and
 * socket leaks or lead to undefined behavior).  You must explicitly
 * resume all requests before stopping the daemon.
 *
 * @return action to cause a request to be suspended.
 */
_MHD_EXTERN const struct MHD_Action *
MHD_action_suspend (void);


/**
 * Resume handling of network data for suspended request.  It is
 * safe to resume a suspended request at any time.  Calling this
 * function on a request that was not previously suspended will
 * result in undefined behavior.
 *
 * If you are using this function in ``external'' select mode, you must
 * make sure to run #MHD_run() afterwards (before again calling
 * #MHD_get_fdset(), as otherwise the change may not be reflected in
 * the set returned by #MHD_get_fdset() and you may end up with a
 * request that is stuck until the next network activity.
 *
 * @param request the request to resume
 */
_MHD_EXTERN void
MHD_request_resume (struct MHD_Request *request)
  MHD_NONNULL(1);


/* **************** Response manipulation functions ***************** */


/**
 * Data transmitted in response to an HTTP request.
 * Usually the final action taken in response to
 * receiving a request.
 */
struct MHD_Response;


/**
 * Converts a @a response to an action.  If @a destroy_after_use
 * is set, the reference to the @a response is consumed
 * by the conversion. If @a consume is #MHD_NO, then
 * the @a response can be converted to actions in the future.
 * However, the @a response is frozen by this step and
 * must no longer be modified (i.e. by setting headers).
 *
 * @param response response to convert, not NULL
 * @param destroy_after_use should the response object be consumed?
 * @return corresponding action, never returns NULL
 *
 * Implementation note: internally, this is largely just
 * a cast (and possibly an RC increment operation),
 * as a response *is* an action.  As no memory is
 * allocated, this operation cannot fail.
 */
_MHD_EXTERN const struct MHD_Action *
MHD_action_from_response (struct MHD_Response *response,
			  enum MHD_Bool destroy_after_use)
  MHD_NONNULL(1);


/**
 * Only respond in conservative HTTP 1.0-mode.  In
 * particular, do not (automatically) sent "Connection" headers and
 * always close the connection after generating the response.
 *
 * @param request the request for which we force HTTP 1.0 to be used
 */
_MHD_EXTERN void
MHD_response_option_v10_only (struct MHD_Response *response)
  MHD_NONNULL(1);


/**
 * The `enum MHD_RequestTerminationCode` specifies reasons
 * why a request has been terminated (or completed).
 * @ingroup request
 */
enum MHD_RequestTerminationCode
{

  /**
   * We finished sending the response.
   * @ingroup request
   */
  MHD_REQUEST_TERMINATED_COMPLETED_OK = 0,

  /**
   * Error handling the connection (resources
   * exhausted, other side closed connection,
   * application error accepting request, etc.)
   * @ingroup request
   */
  MHD_REQUEST_TERMINATED_WITH_ERROR = 1,

  /**
   * No activity on the connection for the number
   * of seconds specified using
   * #MHD_OPTION_CONNECTION_TIMEOUT.
   * @ingroup request
   */
  MHD_REQUEST_TERMINATED_TIMEOUT_REACHED = 2,

  /**
   * We had to close the session since MHD was being
   * shut down.
   * @ingroup request
   */
  MHD_REQUEST_TERMINATED_DAEMON_SHUTDOWN = 3,

  /**
   * We tried to read additional data, but the other side closed the
   * connection.  This error is similar to
   * #MHD_REQUEST_TERMINATED_WITH_ERROR, but specific to the case where
   * the connection died because the other side did not send expected
   * data.
   * @ingroup request
   */
  MHD_REQUEST_TERMINATED_READ_ERROR = 4,

  /**
   * The client terminated the connection by closing the socket
   * for writing (TCP half-closed); MHD aborted sending the
   * response according to RFC 2616, section 8.1.4.
   * @ingroup request
   */
  MHD_REQUEST_TERMINATED_CLIENT_ABORT = 5

};


/**
 * Signature of the callback used by MHD to notify the application
 * about completed requests.
 *
 * @param cls client-defined closure
 * @param toe reason for request termination
 * @param request_context request context value, as originally
 *         returned by the #MHD_EarlyUriLogCallback
 * @see #MHD_option_request_completion()
 * @ingroup request
 */
typedef void
(*MHD_RequestTerminationCallback) (void *cls,
				   enum MHD_RequestTerminationCode toe,
				   void *request_context);


/**
 * Set a function to be called once MHD is finished with the
 * request.
 *
 * @param response which response to set the callback for
 * @param termination_cb function to call
 * @param termination_cb_cls closure for @e termination_cb
 */
_MHD_EXTERN void
MHD_response_option_termination_callback (struct MHD_Response *response,
					  MHD_RequestTerminationCallback termination_cb,
					  void *termination_cb_cls)
  MHD_NONNULL(1);


/**
 * Callback used by libmicrohttpd in order to obtain content.  The
 * callback is to copy at most @a max bytes of content into @a buf.  The
 * total number of bytes that has been placed into @a buf should be
 * returned.
 *
 * Note that returning zero will cause libmicrohttpd to try again.
 * Thus, returning zero should only be used in conjunction
 * with MHD_suspend_connection() to avoid busy waiting.
 *
 * @param cls extra argument to the callback
 * @param pos position in the datastream to access;
 *        note that if a `struct MHD_Response` object is re-used,
 *        it is possible for the same content reader to
 *        be queried multiple times for the same data;
 *        however, if a `struct MHD_Response` is not re-used,
 *        libmicrohttpd guarantees that "pos" will be
 *        the sum of all non-negative return values
 *        obtained from the content reader so far.
 * @param buf where to copy the data
 * @param max maximum number of bytes to copy to @a buf (size of @a buf)
 * @return number of bytes written to @a buf;
 *  0 is legal unless we are running in internal select mode (since
 *    this would cause busy-waiting); 0 in external select mode
 *    will cause this function to be called again once the external
 *    select calls MHD again;
 *  #MHD_CONTENT_READER_END_OF_STREAM (-1) for the regular
 *    end of transmission (with chunked encoding, MHD will then
 *    terminate the chunk and send any HTTP footers that might be
 *    present; without chunked encoding and given an unknown
 *    response size, MHD will simply close the connection; note
 *    that while returning #MHD_CONTENT_READER_END_OF_STREAM is not technically
 *    legal if a response size was specified, MHD accepts this
 *    and treats it just as #MHD_CONTENT_READER_END_WITH_ERROR;
 *  #MHD_CONTENT_READER_END_WITH_ERROR (-2) to indicate a server
 *    error generating the response; this will cause MHD to simply
 *    close the connection immediately.  If a response size was
 *    given or if chunked encoding is in use, this will indicate
 *    an error to the client.  Note, however, that if the client
 *    does not know a response size and chunked encoding is not in
 *    use, then clients will not be able to tell the difference between
 *    #MHD_CONTENT_READER_END_WITH_ERROR and #MHD_CONTENT_READER_END_OF_STREAM.
 *    This is not a limitation of MHD but rather of the HTTP protocol.
 */
typedef ssize_t
(*MHD_ContentReaderCallback) (void *cls,
                              uint64_t pos,
                              char *buf,
                              size_t max);


/**
 * This method is called by libmicrohttpd if we are done with a
 * content reader.  It should be used to free resources associated
 * with the content reader.
 *
 * @param cls closure
 * @ingroup response
 */
typedef void
(*MHD_ContentReaderFreeCallback) (void *cls);


/**
 * Create a response action.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param sc status code to return
 * @param size size of the data portion of the response, #MHD_SIZE_UNKNOWN for unknown
 * @param block_size preferred block size for querying crc (advisory only,
 *                   MHD may still call @a crc using smaller chunks); this
 *                   is essentially the buffer size used for IO, clients
 *                   should pick a value that is appropriate for IO and
 *                   memory performance requirements
 * @param crc callback to use to obtain response data
 * @param crc_cls extra argument to @a crc
 * @param crfc callback to call to free @a crc_cls resources
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
_MHD_EXTERN struct MHD_Response *
MHD_response_from_callback (enum MHD_HTTP_StatusCode sc,
			    uint64_t size,
			    size_t block_size,
			    MHD_ContentReaderCallback crc,
			    void *crc_cls,
			    MHD_ContentReaderFreeCallback crfc);


/**
 * Specification for how MHD should treat the memory buffer
 * given for the response.
 * @ingroup response
 */
enum MHD_ResponseMemoryMode
{

  /**
   * Buffer is a persistent (static/global) buffer that won't change
   * for at least the lifetime of the response, MHD should just use
   * it, not free it, not copy it, just keep an alias to it.
   * @ingroup response
   */
  MHD_RESPMEM_PERSISTENT,

  /**
   * Buffer is heap-allocated with `malloc()` (or equivalent) and
   * should be freed by MHD after processing the response has
   * concluded (response reference counter reaches zero).
   * @ingroup response
   */
  MHD_RESPMEM_MUST_FREE,

  /**
   * Buffer is in transient memory, but not on the heap (for example,
   * on the stack or non-`malloc()` allocated) and only valid during the
   * call to #MHD_create_response_from_buffer.  MHD must make its
   * own private copy of the data for processing.
   * @ingroup response
   */
  MHD_RESPMEM_MUST_COPY

};


/**
 * Create a response object.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param sc status code to use for the response;
 *           #MHD_HTTP_NO_CONTENT is only valid if @a size is 0;
 * @param size size of the data portion of the response
 * @param buffer size bytes containing the response's data portion
 * @param mode flags for buffer management
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
_MHD_EXTERN struct MHD_Response *
MHD_response_from_buffer (enum MHD_HTTP_StatusCode sc,
			  size_t size,
			  void *buffer,
			  enum MHD_ResponseMemoryMode mode);


/**
 * Create a response object based on an @a fd from which
 * data is read.  The response object can be extended with
 * header information and then be used any number of times.
 *
 * @param sc status code to return
 * @param fd file descriptor referring to a file on disk with the
 *        data; will be closed when response is destroyed;
 *        fd should be in 'blocking' mode
 * @param offset offset to start reading from in the file;
 *        reading file beyond 2 GiB may be not supported by OS or
 *        MHD build; see ::MHD_FEATURE_LARGE_FILE
 * @param size size of the data portion of the response;
 *        sizes larger than 2 GiB may be not supported by OS or
 *        MHD build; see ::MHD_FEATURE_LARGE_FILE
 * @return NULL on error (i.e. invalid arguments, out of memory)
 * @ingroup response
 */
_MHD_EXTERN struct MHD_Response *
MHD_response_from_fd (enum MHD_HTTP_StatusCode sc,
		      int fd,
		      uint64_t offset,
		      uint64_t size);


/**
 * Enumeration for operations MHD should perform on the underlying socket
 * of the upgrade.  This API is not finalized, and in particular
 * the final set of actions is yet to be decided. This is just an
 * idea for what we might want.
 */
enum MHD_UpgradeOperation
{

  /**
   * Close the socket, the application is done with it.
   *
   * Takes no extra arguments.
   */
  MHD_UPGRADE_OPERATION_CLOSE = 0

};


/**
 * Handle given to the application to manage special
 * actions relating to MHD responses that "upgrade"
 * the HTTP protocol (i.e. to WebSockets).
 */
struct MHD_UpgradeResponseHandle;


/**
 * This connection-specific callback is provided by MHD to
 * applications (unusual) during the #MHD_UpgradeHandler.
 * It allows applications to perform 'special' actions on
 * the underlying socket from the upgrade.
 *
 * FIXME: this API still uses the untyped, ugly varargs.
 * Should we not modernize this one as well?
 *
 * @param urh the handle identifying the connection to perform
 *            the upgrade @a action on.
 * @param operation which operation should be performed
 * @param ... arguments to the action (depends on the action)
 * @return #MHD_NO on error, #MHD_YES on success
 */
_MHD_EXTERN enum MHD_Bool
MHD_upgrade_operation (struct MHD_UpgradeResponseHandle *urh,
		       enum MHD_UpgradeOperation operation,
		       ...)
  MHD_NONNULL(1);


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
 * @param cls closure, whatever was given to #MHD_response_create_for_upgrade().
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
typedef void
(*MHD_UpgradeHandler)(void *cls,
                      struct MHD_Connection *connection,
                      void *con_cls,
                      const char *extra_in,
                      size_t extra_in_size,
                      MHD_socket sock,
                      struct MHD_UpgradeResponseHandle *urh);


/**
 * Create a response object that can be used for 101 UPGRADE
 * responses, for example to implement WebSockets.  After sending the
 * response, control over the data stream is given to the callback (which
 * can then, for example, start some bi-directional communication).
 * If the response is queued for multiple connections, the callback
 * will be called for each connection.  The callback
 * will ONLY be called after the response header was successfully passed
 * to the OS; if there are communication errors before, the usual MHD
 * connection error handling code will be performed.
 *
 * MHD will automatically set the correct HTTP status
 * code (#MHD_HTTP_SWITCHING_PROTOCOLS).
 * Setting correct HTTP headers for the upgrade must be done
 * manually (this way, it is possible to implement most existing
 * WebSocket versions using this API; in fact, this API might be useful
 * for any protocol switch, not just WebSockets).  Note that
 * draft-ietf-hybi-thewebsocketprotocol-00 cannot be implemented this
 * way as the header "HTTP/1.1 101 WebSocket Protocol Handshake"
 * cannot be generated; instead, MHD will always produce "HTTP/1.1 101
 * Switching Protocols" (if the response code 101 is used).
 *
 * As usual, the response object can be extended with header
 * information and then be used any number of times (as long as the
 * header information is not connection-specific).
 *
 * @param upgrade_handler function to call with the "upgraded" socket
 * @param upgrade_handler_cls closure for @a upgrade_handler
 * @return NULL on error (i.e. invalid arguments, out of memory)
 */
_MHD_EXTERN struct MHD_Response *
MHD_response_for_upgrade (MHD_UpgradeHandler upgrade_handler,
			  void *upgrade_handler_cls)
  MHD_NONNULL(1);


/**
 * Explicitly decrease reference counter of a response object.  If the
 * counter hits zero, destroys a response object and associated
 * resources.  Usually, this is implicitly done by converting a
 * response to an action and returning the action to MHD.
 *
 * @param response response to decrement RC of
 * @ingroup response
 */
_MHD_EXTERN void
MHD_response_queue_for_destroy (struct MHD_Response *response)
  MHD_NONNULL(1);


/**
 * Add a header line to the response.
 *
 * @param response response to add a header to
 * @param header the header to add
 * @param content value to add
 * @return #MHD_NO on error (i.e. invalid header or content format),
 *         or out of memory
 * @ingroup response
 */
_MHD_EXTERN enum MHD_Bool
MHD_response_add_header (struct MHD_Response *response,
                         const char *header,
			 const char *content)
  MHD_NONNULL(1,2,3);


/**
 * Add a tailer line to the response.
 *
 * @param response response to add a footer to
 * @param footer the footer to add
 * @param content value to add
 * @return #MHD_NO on error (i.e. invalid footer or content format),
 *         or out of memory
 * @ingroup response
 */
_MHD_EXTERN enum MHD_Bool
MHD_response_add_trailer (struct MHD_Response *response,
                          const char *footer,
                          const char *content)
  MHD_NONNULL(1,2,3);


/**
 * Delete a header (or footer) line from the response.
 *
 * @param response response to remove a header from
 * @param header the header to delete
 * @param content value to delete
 * @return #MHD_NO on error (no such header known)
 * @ingroup response
 */
_MHD_EXTERN enum MHD_Bool
MHD_response_del_header (struct MHD_Response *response,
                         const char *header,
			 const char *content)
  MHD_NONNULL(1,2,3);


/**
 * Get all of the headers (and footers) added to a response.
 *
 * @param response response to query
 * @param iterator callback to call on each header;
 *        maybe NULL (then just count headers)
 * @param iterator_cls extra argument to @a iterator
 * @return number of entries iterated over
 * @ingroup response
 */
_MHD_EXTERN unsigned int
MHD_response_get_headers (struct MHD_Response *response,
                          MHD_KeyValueIterator iterator,
			  void *iterator_cls)
  MHD_NONNULL(1);


/**
 * Get a particular header (or footer) from the response.
 *
 * @param response response to query
 * @param key which header to get
 * @return NULL if header does not exist
 * @ingroup response
 */
_MHD_EXTERN const char *
MHD_response_get_header (struct MHD_Response *response,
			 const char *key)
  MHD_NONNULL(1,2);


/* ************Upload and PostProcessor functions ********************** */


/**
 * Action telling MHD to continue processing the upload.
 *
 * @return action operation, never NULL
 */
_MHD_EXTERN const struct MHD_Action *
MHD_action_continue (void);


/**
 * Function to process data uploaded by a client.
 *
 * @param cls argument given together with the function
 *        pointer when the handler was registered with MHD
 * @param upload_data the data being uploaded (excluding headers)
 *        POST data will typically be made available incrementally via
 *        multiple callbacks
 * @param[in,out] upload_data_size set initially to the size of the
 *        @a upload_data provided; the method must update this
 *        value to the number of bytes NOT processed;
 * @return action specifying how to proceed, often
 *         #MHD_action_continue() if all is well,
 *         #MHD_action_suspend() to stop reading the upload until
 *              the request is resumed,
 *         NULL to close the socket, or a response
 *         to discard the rest of the upload and return the data given
 */
typedef const struct MHD_Action *
(*MHD_UploadCallback) (void *cls,
		       const char *upload_data,
		       size_t *upload_data_size);


/**
 * Create an action that handles an upload.
 *
 * @param uc function to call with uploaded data
 * @param uc_cls closure for @a uc
 * @return NULL on error (out of memory)
 * @ingroup action
 */
_MHD_EXTERN const struct MHD_Action *
MHD_action_process_upload (MHD_UploadCallback uc,
			   void *uc_cls)
  MHD_NONNULL(1);


/**
 * Iterator over key-value pairs where the value maybe made available
 * in increments and/or may not be zero-terminated.  Used for
 * MHD parsing POST data.  To access "raw" data from POST or PUT
 * requests, use #MHD_action_process_upload() instead.
 *
 * @param cls user-specified closure
 * @param kind type of the value, always #MHD_POSTDATA_KIND when called from MHD
 * @param key 0-terminated key for the value
 * @param filename name of the uploaded file, NULL if not known
 * @param content_type mime-type of the data, NULL if not known
 * @param transfer_encoding encoding of the data, NULL if not known
 * @param data pointer to @a size bytes of data at the
 *              specified offset
 * @param off offset of data in the overall value
 * @param size number of bytes in @a data available
 * @return action specifying how to proceed, often
 *         #MHD_action_continue() if all is well,
 *         #MHD_action_suspend() to stop reading the upload until
 *              the request is resumed,
 *         NULL to close the socket, or a response
 *         to discard the rest of the upload and return the data given
 */
typedef const struct MHD_Action *
(*MHD_PostDataIterator) (void *cls,
                         enum MHD_ValueKind kind,
                         const char *key,
                         const char *filename,
                         const char *content_type,
                         const char *transfer_encoding,
                         const char *data,
                         uint64_t off,
                         size_t size);


/**
 * Create an action that parses a POST request.
 *
 * This action can be used to (incrementally) parse the data portion
 * of a POST request.  Note that some buggy browsers fail to set the
 * encoding type.  If you want to support those, you may have to call
 * #MHD_set_connection_value with the proper encoding type before
 * returning this action (if no supported encoding type is detected,
 * returning this action will cause a bad request to be returned to
 * the client).
 *
 * @param buffer_size maximum number of bytes to use for
 *        internal buffering (used only for the parsing,
 *        specifically the parsing of the keys).  A
 *        tiny value (256-1024) should be sufficient.
 *        Do NOT use a value smaller than 256.  For good
 *        performance, use 32 or 64k (i.e. 65536).
 * @param iter iterator to be called with the parsed data,
 *        Must NOT be NULL.
 * @param iter_cls first argument to @a iter
 * @return NULL on error (out of memory, unsupported encoding),
 *         otherwise a PP handle
 * @ingroup request
 */
_MHD_EXTERN const struct MHD_Action *
MHD_action_parse_post (size_t buffer_size,
		       MHD_PostDataIterator iter,
		       void *iter_cls)
  MHD_NONNULL(2);



/* ********************** generic query functions ********************** */


/**
 * Select which member of the `struct ConnectionInformation`
 * union is desired to be returned by #MHD_connection_get_info().
 */
enum MHD_ConnectionInformationType
{
  /**
   * What cipher algorithm is being used.
   * Takes no extra arguments.
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_CIPHER_ALGO,

  /**
   *
   * Takes no extra arguments.
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_PROTOCOL,

  /**
   * Obtain IP address of the client.  Takes no extra arguments.
   * Returns essentially a `struct sockaddr **` (since the API returns
   * a `union MHD_ConnectionInfo *` and that union contains a `struct
   * sockaddr *`).
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_CLIENT_ADDRESS,

  /**
   * Get the gnuTLS session handle.
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_GNUTLS_SESSION,

  /**
   * Get the gnuTLS client certificate handle.  Dysfunctional (never
   * implemented, deprecated).  Use #MHD_CONNECTION_INFORMATION_GNUTLS_SESSION
   * to get the `gnutls_session_t` and then call
   * gnutls_certificate_get_peers().
   */
  MHD_CONNECTION_INFORMATION_GNUTLS_CLIENT_CERT,

  /**
   * Get the `struct MHD_Daemon *` responsible for managing this connection.
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_DAEMON,

  /**
   * Request the file descriptor for the connection socket.
   * No extra arguments should be passed.
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_CONNECTION_FD,

  /**
   * Returns the client-specific pointer to a `void *` that was (possibly)
   * set during a #MHD_NotifyConnectionCallback when the socket was
   * first accepted.  Note that this is NOT the same as the "con_cls"
   * argument of the #MHD_AccessHandlerCallback.  The "con_cls" is
   * fresh for each HTTP request, while the "socket_context" is fresh
   * for each socket.
   */
  MHD_CONNECTION_INFORMATION_SOCKET_CONTEXT,

  /**
   * Get connection timeout
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_CONNECTION_TIMEOUT,

  /**
   * Check whether the connection is suspended.
   * @ingroup request
   */
  MHD_CONNECTION_INFORMATION_CONNECTION_SUSPENDED


};


/**
 * Information about a connection.
 */
union MHD_ConnectionInformation
{

  /**
   * Cipher algorithm used, of type "enum gnutls_cipher_algorithm".
   */
  int /* enum gnutls_cipher_algorithm */ cipher_algorithm;

  /**
   * Protocol used, of type "enum gnutls_protocol".
   */
  int /* enum gnutls_protocol */ protocol;

  /**
   * Amount of second that connection could spend in idle state
   * before automatically disconnected.
   * Zero for no timeout (unlimited idle time).
   */
  unsigned int connection_timeout;

  /**
   * Connect socket
   */
  MHD_socket connect_fd;

  /**
   * GNUtls session handle, of type "gnutls_session_t".
   */
  void * /* gnutls_session_t */ tls_session;

  /**
   * GNUtls client certificate handle, of type "gnutls_x509_crt_t".
   */
  void * /* gnutls_x509_crt_t */ client_cert;

  /**
   * Address information for the client.
   */
  const struct sockaddr *client_addr;

  /**
   * Which daemon manages this connection (useful in case there are many
   * daemons running).
   */
  struct MHD_Daemon *daemon;

  /**
   * Pointer to connection-specific client context.  Points to the
   * same address as the "socket_context" of the
   * #MHD_NotifyConnectionCallback.
   */
  void **socket_context;

  /**
   * Is this connection right now suspended?
   */
  enum MHD_Bool suspended;
};


/**
 * Obtain information about the given connection.
 * Use wrapper macro #MHD_connection_get_information() instead of direct use
 * of this function.
 *
 * @param connection what connection to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @param return_value_size size of union MHD_ConnectionInformation at compile
 *                          time
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
_MHD_EXTERN enum MHD_Bool
MHD_connection_get_information_sz (struct MHD_Connection *connection,
				   enum MHD_ConnectionInformationType info_type,
				   union MHD_ConnectionInformation *return_value,
				   size_t return_value_size)
  MHD_NONNULL(1,3);


/**
 * Obtain information about the given connection.
 *
 * @param connection what connection to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
#define MHD_connection_get_information(connection,   \
                                       info_type,    \
                                       return_value) \
        MHD_connection_get_information_sz ((connection),(info_type),(return_value),sizeof(union MHD_ConnectionInformation))


/**
 * Information we return about a request.
 */
union MHD_RequestInformation
{

  /**
   * Connection via which we received the request.
   */
  struct MHD_Connection *connection;

  /**
   * Pointer to client context.  Will also be given to
   * the application in a #MHD_RequestTerminationCallback.
   */
  void **request_context;

  /**
   * HTTP version requested by the client.
   */
  const char *http_version;

  /**
   * HTTP method of the request, as a string.  Particularly useful if
   * #MHD_HTTP_METHOD_UNKNOWN was given.
   */
  const char *http_method;

  /**
   * Size of the client's HTTP header.
   */
  size_t header_size;

};


/**
 * Select which member of the `struct RequestInformation`
 * union is desired to be returned by #MHD_request_get_info().
 */
enum MHD_RequestInformationType
{
  /**
   * Return which connection the request is associated with.
   */
  MHD_REQUEST_INFORMATION_CONNECTION,

  /**
   * Returns the client-specific pointer to a `void *` that
   * is specific to this request.
   */
  MHD_REQUEST_INFORMATION_CLIENT_CONTEXT,

  /**
   * Return the HTTP version string given by the client.
   * @ingroup request
   */
  MHD_REQUEST_INFORMATION_HTTP_VERSION,

  /**
   * Return the HTTP method used by the request.
   * @ingroup request
   */
  MHD_REQUEST_INFORMATION_HTTP_METHOD,

  /**
   * Return length of the client's HTTP request header.
   * @ingroup request
   */
  MHD_REQUEST_INFORMATION_HEADER_SIZE
};


/**
 * Obtain information about the given request.
 * Use wrapper macro #MHD_request_get_information() instead of direct use
 * of this function.
 *
 * @param request what request to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @param return_value_size size of union MHD_RequestInformation at compile
 *                          time
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
_MHD_EXTERN enum MHD_Bool
MHD_request_get_information_sz (struct MHD_Request *request,
			        enum MHD_RequestInformationType info_type,
			        union MHD_RequestInformation *return_value,
			        size_t return_value_size)
  MHD_NONNULL(1,3);


/**
 * Obtain information about the given request.
 *
 * @param request what request to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
#define MHD_request_get_information (request,      \
                                     info_type,    \
                                     return_value) \
        MHD_request_get_information_sz ((request), (info_type), (return_value), sizeof(union MHD_RequestInformation))


/**
 * Values of this enum are used to specify what
 * information about a deamon is desired.
 */
enum MHD_DaemonInformationType
{

  /**
   * Request the file descriptor for the listening socket.
   * No extra arguments should be passed.
   */
  MHD_DAEMON_INFORMATION_LISTEN_SOCKET,

  /**
   * Request the file descriptor for the external epoll.
   * No extra arguments should be passed.
   */
  MHD_DAEMON_INFORMATION_EPOLL_FD,

  /**
   * Request the number of current connections handled by the daemon.
   * No extra arguments should be passed.
   * Note: when using MHD in external polling mode, this type of request
   * could be used only when #MHD_run()/#MHD_run_from_select is not
   * working in other thread at the same time.
   */
  MHD_DAEMON_INFORMATION_CURRENT_CONNECTIONS,

  /**
   * Request the port number of daemon's listen socket.
   * No extra arguments should be passed.
   * Note: if port '0' was specified for #MHD_option_port(), returned
   * value will be real port number.
   */
  MHD_DAEMON_INFORMATION_BIND_PORT
};


/**
 * Information about an MHD daemon.
 */
union MHD_DaemonInformation
{

  /**
   * Socket, returned for #MHD_DAEMON_INFORMATION_LISTEN_SOCKET.
   */
  MHD_socket listen_socket;

  /**
   * Bind port number, returned for #MHD_DAEMON_INFORMATION_BIND_PORT.
   */
  uint16_t port;

  /**
   * epoll FD, returned for #MHD_DAEMON_INFORMATION_EPOLL_FD.
   */
  int epoll_fd;

  /**
   * Number of active connections, for #MHD_DAEMON_INFORMATION_CURRENT_CONNECTIONS.
   */
  unsigned int num_connections;

};


/**
 * Obtain information about the given daemon.
 * Use wrapper macro #MHD_daemon_get_information() instead of direct use
 * of this function.
 *
 * @param daemon what daemon to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @param return_value_size size of union MHD_DaemonInformation at compile
 *                          time
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
_MHD_EXTERN enum MHD_Bool
MHD_daemon_get_information_sz (struct MHD_Daemon *daemon,
			       enum MHD_DaemonInformationType info_type,
			       union MHD_DaemonInformation *return_value,
			       size_t return_value_size)
  MHD_NONNULL(1,3);

/**
 * Obtain information about the given daemon.
 *
 * @param daemon what daemon to get information about
 * @param info_type what information is desired?
 * @param[out] return_value pointer to union where requested information will
 *                          be stored
 * @return #MHD_YES on success, #MHD_NO on error
 *         (@a info_type is unknown, NULL pointer etc.)
 * @ingroup specialized
 */
#define MHD_daemon_get_information(daemon,       \
                                   info_type,    \
                                   return_value) \
	MHD_daemon_get_information_sz((daemon), (info_type), (return_value), sizeof(union MHD_DaemonInformation));


/**
 * Callback for serious error condition. The default action is to print
 * an error message and `abort()`.
 *
 * @param cls user specified value
 * @param file where the error occured
 * @param line where the error occured
 * @param reason error detail, may be NULL
 * @ingroup logging
 */
typedef void
(*MHD_PanicCallback) (void *cls,
                      const char *file,
                      unsigned int line,
                      const char *reason);


/**
 * Sets the global error handler to a different implementation.  @a cb
 * will only be called in the case of typically fatal, serious
 * internal consistency issues.  These issues should only arise in the
 * case of serious memory corruption or similar problems with the
 * architecture.  While @a cb is allowed to return and MHD will then
 * try to continue, this is never safe.
 *
 * The default implementation that is used if no panic function is set
 * simply prints an error message and calls `abort()`.  Alternative
 * implementations might call `exit()` or other similar functions.
 *
 * @param cb new error handler
 * @param cls passed to @a cb
 * @ingroup logging
 */
_MHD_EXTERN void
MHD_set_panic_func (MHD_PanicCallback cb,
		    void *cls);


/**
 * Process escape sequences ('%HH') Updates val in place; the
 * result should be UTF-8 encoded and cannot be larger than the input.
 * The result must also still be 0-terminated.
 *
 * @param val value to unescape (modified in the process)
 * @return length of the resulting val (`strlen(val)` may be
 *  shorter afterwards due to elimination of escape sequences)
 */
_MHD_EXTERN size_t
MHD_http_unescape (char *val)
  MHD_NONNULL(1);


/**
 * Types of information about MHD features,
 * used by #MHD_is_feature_supported().
 */
enum MHD_Feature
{
  /**
   * Get whether messages are supported. If supported then in debug
   * mode messages can be printed to stderr or to external logger.
   */
  MHD_FEATURE_MESSAGES = 1,

  /**
   * Get whether HTTPS is supported.  If supported then flag
   * #MHD_USE_TLS and options #MHD_OPTION_HTTPS_MEM_KEY,
   * #MHD_OPTION_HTTPS_MEM_CERT, #MHD_OPTION_HTTPS_MEM_TRUST,
   * #MHD_OPTION_HTTPS_MEM_DHPARAMS, #MHD_OPTION_HTTPS_CRED_TYPE,
   * #MHD_OPTION_HTTPS_PRIORITIES can be used.
   */
  MHD_FEATURE_TLS = 2,

  /**
   * Get whether option #MHD_OPTION_HTTPS_CERT_CALLBACK is
   * supported.
   */
  MHD_FEATURE_HTTPS_CERT_CALLBACK = 3,

  /**
   * Get whether IPv6 is supported. If supported then flag
   * #MHD_USE_IPv6 can be used.
   */
  MHD_FEATURE_IPv6 = 4,

  /**
   * Get whether IPv6 without IPv4 is supported. If not supported
   * then IPv4 is always enabled in IPv6 sockets and
   * flag #MHD_USE_DUAL_STACK if always used when #MHD_USE_IPv6 is
   * specified.
   */
  MHD_FEATURE_IPv6_ONLY = 5,

  /**
   * Get whether `poll()` is supported. If supported then flag
   * #MHD_USE_POLL can be used.
   */
  MHD_FEATURE_POLL = 6,

  /**
   * Get whether `epoll()` is supported. If supported then Flags
   * #MHD_USE_EPOLL and
   * #MHD_USE_EPOLL_INTERNAL_THREAD can be used.
   */
  MHD_FEATURE_EPOLL = 7,

  /**
   * Get whether shutdown on listen socket to signal other
   * threads is supported. If not supported flag
   * #MHD_USE_ITC is automatically forced.
   */
  MHD_FEATURE_SHUTDOWN_LISTEN_SOCKET = 8,

  /**
   * Get whether socketpair is used internally instead of pipe to
   * signal other threads.
   */
  MHD_FEATURE_SOCKETPAIR = 9,

  /**
   * Get whether TCP Fast Open is supported. If supported then
   * flag #MHD_USE_TCP_FASTOPEN and option
   * #MHD_OPTION_TCP_FASTOPEN_QUEUE_SIZE can be used.
   */
  MHD_FEATURE_TCP_FASTOPEN = 10,

  /**
   * Get whether HTTP Basic authorization is supported. If supported
   * then functions #MHD_basic_auth_get_username_password and
   * #MHD_queue_basic_auth_fail_response can be used.
   */
  MHD_FEATURE_BASIC_AUTH = 11,

  /**
   * Get whether HTTP Digest authorization is supported. If
   * supported then options #MHD_OPTION_DIGEST_AUTH_RANDOM,
   * #MHD_OPTION_NONCE_NC_SIZE and
   * #MHD_digest_auth_check() can be used.
   */
  MHD_FEATURE_DIGEST_AUTH = 12,

  /**
   * Get whether postprocessor is supported. If supported then
   * functions #MHD_create_post_processor(), #MHD_post_process() and
   * #MHD_destroy_post_processor() can
   * be used.
   */
  MHD_FEATURE_POSTPROCESSOR = 13,

  /**
  * Get whether password encrypted private key for HTTPS daemon is
  * supported. If supported then option
  * ::MHD_OPTION_HTTPS_KEY_PASSWORD can be used.
  */
  MHD_FEATURE_HTTPS_KEY_PASSWORD = 14,

  /**
   * Get whether reading files beyond 2 GiB boundary is supported.
   * If supported then #MHD_create_response_from_fd(),
   * #MHD_create_response_from_fd64 #MHD_create_response_from_fd_at_offset()
   * and #MHD_create_response_from_fd_at_offset64() can be used with sizes and
   * offsets larger than 2 GiB. If not supported value of size+offset is
   * limited to 2 GiB.
   */
  MHD_FEATURE_LARGE_FILE = 15,

  /**
   * Get whether MHD set names on generated threads.
   */
  MHD_FEATURE_THREAD_NAMES = 16,

  /**
   * Get whether HTTP "Upgrade" is supported.
   * If supported then #MHD_ALLOW_UPGRADE, #MHD_upgrade_action() and
   * #MHD_create_response_for_upgrade() can be used.
   */
  MHD_FEATURE_UPGRADE = 17,

  /**
   * Get whether it's safe to use same FD for multiple calls of
   * #MHD_create_response_from_fd() and whether it's safe to use single
   * response generated by #MHD_create_response_from_fd() with multiple
   * connections at same time.
   * If #MHD_is_feature_supported() return #MHD_NO for this feature then
   * usage of responses with same file FD in multiple parallel threads may
   * results in incorrect data sent to remote client.
   * It's always safe to use same file FD in multiple responses if MHD
   * is run in any single thread mode.
   */
  MHD_FEATURE_RESPONSES_SHARED_FD = 18,

  /**
   * Get whether MHD support automatic detection of bind port number.
   * @sa #MHD_DAEMON_INFO_BIND_PORT
   */
  MHD_FEATURE_AUTODETECT_BIND_PORT = 19,

  /**
   * Get whether MHD support SIGPIPE suppression.
   * If SIGPIPE suppression is not supported, application must handle
   * SIGPIPE signal by itself.
   */
  MHD_FEATURE_AUTOSUPPRESS_SIGPIPE = 20,

  /**
   * Get whether MHD use system's sendfile() function to send
   * file-FD based responses over non-TLS connections.
   * @note Since v0.9.56
   */
  MHD_FEATURE_SENDFILE = 21
};


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
MHD_is_feature_supported (enum MHD_Feature feature);


/**
 * What is this request waiting for?
 */
enum MHD_RequestEventLoopInfo
{
  /**
   * We are waiting to be able to read.
   */
  MHD_EVENT_LOOP_INFO_READ = 0,

  /**
   * We are waiting to be able to write.
   */
  MHD_EVENT_LOOP_INFO_WRITE = 1,

  /**
   * We are waiting for the application to provide data.
   */
  MHD_EVENT_LOOP_INFO_BLOCK = 2,

  /**
   * We are finished and are awaiting cleanup.
   */
  MHD_EVENT_LOOP_INFO_CLEANUP = 3
};


#endif
