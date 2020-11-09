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
 * @file lib/daemon_options.c
 * @brief boring functions to manipulate daemon options
 * @author Christian Grothoff
 */
#include "internal.h"
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif

/**
 * Set logging method.  Specify NULL to disable logging entirely.  By
 * default (if this option is not given), we log error messages to
 * stderr.
 *
 * @param daemon which instance to setup logging for
 * @param logger function to invoke
 * @param logger_cls closure for @a logger
 */
void
MHD_daemon_set_logger (struct MHD_Daemon *daemon,
		       MHD_LoggingCallback logger,
		       void *logger_cls)
{
  daemon->logger = logger;
  daemon->logger_cls = logger_cls;
}


/**
 * Suppress use of "Date" header as this system has no RTC.
 *
 * @param daemon which instance to disable clock for.
 */
void
MHD_daemon_suppress_date_no_clock (struct MHD_Daemon *daemon)
{
  daemon->suppress_date = true;
}


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
void
MHD_daemon_disable_itc (struct MHD_Daemon *daemon)
{
  daemon->disable_itc = true;
}


/**
 * Enable `turbo`.  Disables certain calls to `shutdown()`,
 * enables aggressive non-blocking optimistic reads and
 * other potentially unsafe optimizations.
 * Most effects only happen with #MHD_ELS_EPOLL.
 *
 * @param daemon which instance to enable turbo for
 */
void
MHD_daemon_enable_turbo (struct MHD_Daemon *daemon)
{
  daemon->enable_turbo = true;
}


/**
 * Disable #MHD_action_suspend() functionality.
 *
 * You should only use this function if you are sure you do
 * satisfy all of its requirements and need a generally minor
 * boost in performance.
 *
 * @param daemon which instance to disable suspend for
 */
void
MHD_daemon_disallow_suspend_resume (struct MHD_Daemon *daemon)
{
  daemon->disallow_suspend_resume = true;
}


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
void
MHD_daemon_disallow_upgrade (struct MHD_Daemon *daemon)
{
  daemon->disallow_upgrade = true;
}


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
enum MHD_Bool
MHD_daemon_tcp_fastopen (struct MHD_Daemon *daemon,
			 enum MHD_FastOpenMethod fom,
			 unsigned int queue_length)
{
  daemon->fast_open_method = fom;
  daemon->fo_queue_length = queue_length;
  switch (fom)
  {
  case MHD_FOM_DISABLE:
    return MHD_YES;
  case MHD_FOM_AUTO:
    return MHD_YES;
  case MHD_FOM_REQUIRE:
#ifdef TCP_FASTOPEN
    return MHD_YES;
#else
    return MHD_NO;
#endif
  }
  return MHD_NO;
}


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
void
MHD_daemon_bind_port (struct MHD_Daemon *daemon,
		      enum MHD_AddressFamily af,
		      uint16_t port)
{
  daemon->listen_af = af;
  daemon->listen_port = port;
}


/**
 * Bind to the given socket address.
 * Ineffective in conjunction with #MHD_daemon_listen_socket().
 *
 * @param daemon which instance to configure the binding address for
 * @param sa address to bind to; can be IPv4 (AF_INET), IPv6 (AF_INET6)
 *        or even a UNIX domain socket (AF_UNIX)
 * @param sa_len number of bytes in @a sa
 */
void
MHD_daemon_bind_socket_address (struct MHD_Daemon *daemon,
				const struct sockaddr *sa,
				size_t sa_len)
{
  memcpy (&daemon->listen_sa,
	  sa,
	  sa_len);
  daemon->listen_sa_len = sa_len;
}


/**
 * Use the given backlog for the listen() call.
 * Ineffective in conjunction with #MHD_daemon_listen_socket().
 *
 * @param daemon which instance to configure the backlog for
 * @param listen_backlog backlog to use
 */
void
MHD_daemon_listen_backlog (struct MHD_Daemon *daemon,
			   int listen_backlog)
{
  daemon->listen_backlog = listen_backlog;
}


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
void
MHD_daemon_listen_allow_address_reuse (struct MHD_Daemon *daemon)
{
  daemon->allow_address_reuse = true;
}


/**
 * Use SHOUTcast.  This will cause the response to begin
 * with the SHOUTcast "ICY" line instad of "HTTP".
 *
 * @param daemon daemon to set SHOUTcast option for
 */
_MHD_EXTERN void
MHD_daemon_enable_shoutcast (struct MHD_Daemon *daemon)
{
  daemon->enable_shoutcast = true;
}


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
void
MHD_daemon_listen_socket (struct MHD_Daemon *daemon,
			  MHD_socket listen_socket)
{
  daemon->listen_socket = listen_socket;
}


/**
 * Force use of a particular event loop system call.
 *
 * @param daemon daemon to set event loop style for
 * @param els event loop syscall to use
 * @return #MHD_NO on failure, #MHD_YES on success
 */
enum MHD_Bool
MHD_daemon_event_loop (struct MHD_Daemon *daemon,
		       enum MHD_EventLoopSyscall els)
{
  switch (els)
  {
  case MHD_ELS_AUTO:
    break; /* should always be OK */
  case MHD_ELS_SELECT:
    break; /* should always be OK */
  case MHD_ELS_POLL:
#ifdef HAVE_POLL
    break;
#else
    return MHD_NO; /* not supported */
#endif
  case MHD_ELS_EPOLL:
#ifdef EPOLL_SUPPORT
    break;
#else
    return MHD_NO; /* not supported */
#endif
  default:
    return MHD_NO; /* not supported (presumably future ABI extension) */
  }
  daemon->event_loop_syscall = els;
  return MHD_YES;
}


/**
 * Set how strictly MHD will enforce the HTTP protocol.
 *
 * @param daemon daemon to configure strictness for
 * @param sl how strict should we be
 */
void
MHD_daemon_protocol_strict_level (struct MHD_Daemon *daemon,
				  enum MHD_ProtocolStrictLevel sl)
{
  daemon->protocol_strict_level = sl;
}


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
enum MHD_StatusCode
MHD_daemon_set_tls_backend (struct MHD_Daemon *daemon,
			    const char *tls_backend,
			    const char *ciphers)
{
#if ! (defined(HTTPS_SUPPORT) && defined (HAVE_DLFCN_H))
  return MHD_SC_TLS_DISABLED;
#else
  char filename[1024];
  int res;
  MHD_TLS_PluginInit init;

  /* todo: .dll on W32? */
  res = MHD_snprintf_ (filename,
		       sizeof (filename),
		       "%s/libmicrohttpd_tls_%s.so",
		       MHD_PLUGIN_INSTALL_PREFIX,
		       tls_backend);
  if (0 >= res)
    return MHD_SC_TLS_BACKEND_UNSUPPORTED; /* string too long? */
  if (NULL ==
      (daemon->tls_backend_lib = dlopen (filename,
					 RTLD_NOW | RTLD_LOCAL)))
    return MHD_SC_TLS_BACKEND_UNSUPPORTED; /* plugin not found */
  if (NULL == (init = dlsym (daemon->tls_backend_lib,
			     "MHD_TLS_init_" MHD_TLS_ABI_VERSION_STR)))

  {
    dlclose (daemon->tls_backend_lib);
    daemon->tls_backend_lib = NULL;
    return MHD_SC_TLS_BACKEND_UNSUPPORTED; /* possibly wrong version installed */
  }
  if (NULL == (daemon->tls_api = init (ciphers)))
  {
    dlclose (daemon->tls_backend_lib);
    daemon->tls_backend_lib = NULL;
    return MHD_SC_TLS_CIPHERS_INVALID; /* possibly wrong version installed */
  }
  return MHD_SC_OK;
#endif
}


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
 * @return #MHD_SC_OK upon success; MHD_BACKEND_UNINITIALIZED
 *     if the TLS backend is not yet setup.
 */
enum MHD_StatusCode
MHD_daemon_tls_key_and_cert_from_memory (struct MHD_Daemon *daemon,
					 const char *mem_key,
					 const char *mem_cert,
					 const char *pass)
{
#ifndef HTTPS_SUPPORT
  return MHD_SC_TLS_DISABLED;
#else
  struct MHD_TLS_Plugin *plugin;

  if (NULL == (plugin = daemon->tls_api))
    return MHD_SC_TLS_BACKEND_UNINITIALIZED;
  return plugin->init_kcp (plugin->cls,
			   mem_key,
			   mem_cert,
			   pass);
#endif
}


/**
 * Configure DH parameters (dh.pem) to use for the TLS key
 * exchange.
 *
 * @param daemon daemon to configure tls for
 * @param dh parameters to use
 * @return #MHD_SC_OK upon success; MHD_BACKEND_UNINITIALIZED
 *     if the TLS backend is not yet setup.
 */
enum MHD_StatusCode
MHD_daemon_tls_mem_dhparams (struct MHD_Daemon *daemon,
			     const char *dh)
{
#ifndef HTTPS_SUPPORT
  return MHD_SC_TLS_DISABLED;
#else
  struct MHD_TLS_Plugin *plugin;

  if (NULL == (plugin = daemon->tls_api))
    return MHD_SC_TLS_BACKEND_UNINITIALIZED;
  return plugin->init_dhparams (plugin->cls,
				dh);
#endif
}


/**
 * Memory pointer for the certificate (ca.pem) to be used by the
 * HTTPS daemon for client authentification.
 *
 * @param daemon daemon to configure tls for
 * @param mem_trust memory pointer to the certificate
 * @return #MHD_SC_OK upon success; MHD_BACKEND_UNINITIALIZED
 *     if the TLS backend is not yet setup.
 */
enum MHD_StatusCode
MHD_daemon_tls_mem_trust (struct MHD_Daemon *daemon,
			  const char *mem_trust)
{
#ifndef HTTPS_SUPPORT
  return MHD_SC_TLS_DISABLED;
#else
  struct MHD_TLS_Plugin *plugin;

  if (NULL == (plugin = daemon->tls_api))
    return MHD_SC_TLS_BACKEND_UNINITIALIZED;
  return plugin->init_mem_trust (plugin->cls,
				 mem_trust);
#endif
}


/**
 * Configure daemon credentials type for GnuTLS.
 *
 * @param gnutls_credentials must be a value of
 *   type `gnutls_credentials_type_t`
 * @return #MHD_SC_OK upon success; TODO: define failure modes
 */
enum MHD_StatusCode
MHD_daemon_gnutls_credentials (struct MHD_Daemon *daemon,
			       int gnutls_credentials)
{
#ifndef HTTPS_SUPPORT
  return MHD_SC_TLS_DISABLED;
#else
  struct MHD_TLS_Plugin *plugin;

  if (NULL == (plugin = daemon->tls_api))
    return MHD_SC_TLS_BACKEND_UNINITIALIZED;
  return MHD_SC_TLS_BACKEND_OPERATION_UNSUPPORTED;
#endif
}


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
enum MHD_StatusCode
MHD_daemon_gnutls_key_and_cert_from_callback (struct MHD_Daemon *daemon,
					      void *cb)
{
#ifndef HTTPS_SUPPORT
  return MHD_SC_TLS_DISABLED;
#else
  struct MHD_TLS_Plugin *plugin;

  if (NULL == (plugin = daemon->tls_api))
    return MHD_SC_TLS_BACKEND_UNINITIALIZED;
  return MHD_SC_TLS_BACKEND_OPERATION_UNSUPPORTED;
#endif
}


/**
 * Specify threading mode to use.
 *
 * @param daemon daemon to configure
 * @param tm mode to use (positive values indicate the
 *        number of worker threads to be used)
 */
void
MHD_daemon_threading_mode (struct MHD_Daemon *daemon,
			    enum MHD_ThreadingMode tm)
{
  daemon->threading_mode = tm;
}


/**
 * Set  a policy callback that accepts/rejects connections
 * based on the client's IP address.  This function will be called
 * before a connection object is created.
 *
 * @param daemon daemon to set policy for
 * @param apc function to call to check the policy
 * @param apc_cls closure for @a apc
 */
void
MHD_daemon_accept_policy (struct MHD_Daemon *daemon,
			  MHD_AcceptPolicyCallback apc,
			  void *apc_cls)
{
  daemon->accept_policy_cb = apc;
  daemon->accept_policy_cb_cls = apc_cls;
}


/**
 * Register a callback to be called first for every request
 * (before any parsing of the header).  Makes it easy to
 * log the full URL.
 *
 * @param daemon daemon for which to set the logger
 * @param cb function to call
 * @param cb_cls closure for @a cb
 */
void
MHD_daemon_set_early_uri_logger (struct MHD_Daemon *daemon,
				 MHD_EarlyUriLogCallback cb,
				 void *cb_cls)
{
  daemon->early_uri_logger_cb = cb;
  daemon->early_uri_logger_cb_cls = cb_cls;
}


/**
 * Register a function that should be called whenever a connection is
 * started or closed.
 *
 * @param daemon daemon to set callback for
 * @param ncc function to call to check the policy
 * @param ncc_cls closure for @a apc
 */
void
MHD_daemon_set_notify_connection (struct MHD_Daemon *daemon,
				  MHD_NotifyConnectionCallback ncc,
				  void *ncc_cls)
{
  daemon->notify_connection_cb = ncc;
  daemon->notify_connection_cb_cls = ncc_cls;
}


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
void
MHD_daemon_connection_memory_limit (struct MHD_Daemon *daemon,
				    size_t memory_limit_b,
				    size_t memory_increment_b)
{
  if (memory_increment_b >= memory_limit_b)
    MHD_PANIC ("sane memory increment must be below memory limit");
  daemon->connection_memory_limit_b = memory_limit_b;
  daemon->connection_memory_increment_b = memory_increment_b;
}


/**
 * Desired size of the stack for threads created by MHD.  Use 0 for
 * system default.  Only useful if the selected threading mode
 * is not #MHD_TM_EXTERNAL_EVENT_LOOP.
 *
 * @param daemon daemon to configure
 * @param stack_limit_b stack size to use in bytes
 */
void
MHD_daemon_thread_stack_size (struct MHD_Daemon *daemon,
			      size_t stack_limit_b)
{
  daemon->thread_stack_limit_b = stack_limit_b;
}


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
void
MHD_daemon_connection_limits (struct MHD_Daemon *daemon,
			      unsigned int global_connection_limit,
			      unsigned int ip_connection_limit)
{
  daemon->global_connection_limit = global_connection_limit;
  daemon->ip_connection_limit = ip_connection_limit;
}


/**
 * After how many seconds of inactivity should a
 * connection automatically be timed out?
 * Use zero for no timeout, which is also the (unsafe!) default.
 *
 * @param daemon daemon to configure
 * @param timeout_s number of seconds of timeout to use
 */
void
MHD_daemon_connection_default_timeout (struct MHD_Daemon *daemon,
				       unsigned int timeout_s)
{
  daemon->connection_default_timeout = (time_t) timeout_s;
}


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
void
MHD_daemon_unescape_cb (struct MHD_Daemon *daemon,
			MHD_UnescapeCallback unescape_cb,
			void *unescape_cb_cls)
{
  daemon->unescape_cb = unescape_cb;
  daemon->unescape_cb_cls = unescape_cb_cls;
}


/**
 * Set random values to be used by the Digest Auth module.  Note that
 * the application must ensure that @a buf remains allocated and
 * unmodified while the deamon is running.
 *
 * @param daemon daemon to configure
 * @param buf_size number of bytes in @a buf
 * @param buf entropy buffer
 */
void
MHD_daemon_digest_auth_random (struct MHD_Daemon *daemon,
			       size_t buf_size,
			       const void *buf)
{
#if ENABLE_DAUTH
  daemon->digest_auth_random_buf = buf;
  daemon->digest_auth_random_buf_size = buf_size;
#else
  (void) daemon;
  (void) buf_size;
  (void) buf;
  MHD_PANIC ("digest authentication not supported by this build");
#endif
}


/**
 * Length of the internal array holding the map of the nonce and
 * the nonce counter.
 *
 * @param daemon daemon to configure
 * @param nc_length desired array length
 */
enum MHD_StatusCode
MHD_daemon_digest_auth_nc_length (struct MHD_Daemon *daemon,
				  size_t nc_length)
{
#if ENABLE_DAUTH
  if ( ( (size_t) (nc_length * sizeof (struct MHD_NonceNc))) /
       sizeof (struct MHD_NonceNc) != nc_length)
    {
#ifdef HAVE_MESSAGES
      MHD_DLOG (daemon,
		_("Specified value for NC_SIZE too large\n"));
#endif
      return MHD_SC_DIGEST_AUTH_NC_LENGTH_TOO_BIG;
    }
  if (0 < nc_length)
    {
      if (NULL != daemon->nnc)
	free (daemon->nnc);
      daemon->nnc = malloc (daemon->nonce_nc_size *
			    sizeof (struct MHD_NonceNc));
      if (NULL == daemon->nnc)
	{
#ifdef HAVE_MESSAGES
	  MHD_DLOG (daemon,
		    _("Failed to allocate memory for nonce-nc map: %s\n"),
		    MHD_strerror_ (errno));
#endif
	  return MHD_SC_DIGEST_AUTH_NC_ALLOCATION_FAILURE;
	}
    }
  daemon->digest_nc_length = nc_length;
  return MHD_SC_OK;
#else
  (void) daemon;
  (void) nc_length;
  return MHD_SC_DIGEST_AUTH_NOT_SUPPORTED_BY_BUILD;
#endif
}


/* end of daemon_options.c */
