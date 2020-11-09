/*
     This file is part of libmicrohttpd
     Copyright (C) 2018 Christian Grothoff (and other contributing authors)

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
 * @file microhttpd_tls.h
 * @brief interface for TLS plugins of libmicrohttpd
 * @author Christian Grothoff
 */

#ifndef MICROHTTPD_TLS_H
#define MICROHTTPD_TLS_H

#include <microhttpd2.h>

/**
 * Version of the TLS ABI.
 */
#define MHD_TLS_ABI_VERSION 0

/**
 * Version of the TLS ABI as a string.
 * Must match #MHD_TLS_ABI_VERSION!
 */
#define MHD_TLS_ABI_VERSION_STR "0"


/**
 * Data structure kept per TLS client by the plugin.
 */
struct MHD_TLS_ConnectionState;



/**
 * Callback functions to use for TLS operations.
 */
struct MHD_TLS_Plugin
{
  /**
   * Closure with plugin's internal state, opaque to MHD.
   */ 
  void *cls;
  
  /**
   * Destroy the plugin, we are done with it.
   */
  void
  (*done)(struct MHD_TLS_Plugin *plugin);

  /**
   * Initialize key and certificate data from memory.
   *
   * @param cls the @e cls of this struct
   * @param mem_key private key (key.pem) to be used by the
   *     HTTPS daemon.  Must be the actual data in-memory, not a filename.
   * @param mem_cert certificate (cert.pem) to be used by the
   *     HTTPS daemon.  Must be the actual data in-memory, not a filename.
   * @param pass passphrase phrase to decrypt 'key.pem', NULL
   *     if @param mem_key is in cleartext already
   * @return #MHD_SC_OK upon success; TODO: define failure modes
   */
  enum MHD_StatusCode
  (*init_kcp)(void *cls,
	      const char *mem_key,
	      const char *mem_cert,
	      const char *pass);


  /**
   * Initialize DH parameters.
   *
   * @param cls the @e cls of this struct
   * @param dh parameters to use
   * @return #MHD_SC_OK upon success; TODO: define failure modes
   */
  enum MHD_StatusCode
  (*init_dhparams)(void *cls,
		   const char *dh);


  /**
   * Initialize certificate to use for client authentication.
   *
   * @param cls the @e cls of this struct
   * @param mem_trust client certificate
   * @return #MHD_SC_OK upon success; TODO: define failure modes
   */
  enum MHD_StatusCode
  (*init_mem_trust)(void *cls,
		    const char *mem_trust);


  /**
   * Function called when we receive a connection and need
   * to initialize our TLS state for it.
   *
   * @param cls the @e cls of this struct
   * @param ... TBD
   * @return NULL on error
   */
  struct MHD_TLS_ConnectionState *
  (*setup_connection)(void *cls,
		      ...);


  enum MHD_Bool
  (*handshake)(void *cls,
	       struct MHD_TLS_ConnectionState *cs);


  enum MHD_Bool
  (*idle_ready)(void *cls,
		struct MHD_TLS_ConnectionState *cs);


  enum MHD_Bool
  (*update_event_loop_info)(void *cls,
			    struct MHD_TLS_ConnectionState *cs,
			    enum MHD_RequestEventLoopInfo *eli);
  
  ssize_t
  (*send)(void *cls,
	  struct MHD_TLS_ConnectionState *cs,
	  const void *buf,
	  size_t buf_size);


  ssize_t
  (*recv)(void *cls,
	  struct MHD_TLS_ConnectionState *cs,
	  void *buf,
	  size_t buf_size);


  const char *
  (*strerror)(void *cls,
	      int ec);

  enum MHD_Bool
  (*check_record_pending)(void *cls,
			  struct MHD_TLS_ConnectionState *cs);

  enum MHD_Bool
  (*shutdown_connection) (void *cls,
			  struct MHD_TLS_ConnectionState *cs);

  
  void
  (*teardown_connection)(void *cls,
			 struct MHD_TLS_ConnectionState *cs);
  
  /**
   * TODO: More functions here....
   */
  
};


/**
 * Signature of the initialization function each TLS plugin must
 * export.
 *
 * @param ciphers desired cipher suite
 * @return NULL on errors (in particular, invalid cipher suite)
 */
typedef struct MHD_TLS_Plugin *
(*MHD_TLS_PluginInit) (const char *ciphers);


/**
 * Define function to be exported from the TLS plugin.
 *
 * @a body function body that receives `ciphers` argument
 *    and must return the plugin API, or NULL on error.
 */
#define MHD_TLS_INIT(body) \
  struct MHD_TLS_Plugin * \
  MHD_TLS_init_ ## MHD_TLS_ABI_VERSION (const char *ciphers) \\
  {  body  }

#endif
