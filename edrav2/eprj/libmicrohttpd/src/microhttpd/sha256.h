/*
     This file is part of libmicrohttpd
     Copyright (C) 2019 Karlson2k (Evgeny Grin)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library.
     If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file microhttpd/sha256.h
 * @brief  Calculation of SHA-256 digest
 * @author Karlson2k (Evgeny Grin)
 */

#ifndef MHD_SHA256_H
#define MHD_SHA256_H 1

#include "mhd_options.h"
#include <stdint.h>
#include <stddef.h>

/**
 *  Digest is kept internally as 8 32-bit words.
 */
#define _SHA256_DIGEST_LENGTH 8

/**
 * Size of SHA-256 digest in bytes
 */
#define SHA256_DIGEST_SIZE (_SHA256_DIGEST_LENGTH * 4)

/**
 * Size of SHA-256 digest string in chars
 */
#define SHA256_DIGEST_STRING_SIZE ((SHA256_DIGEST_SIZE) * 2 + 1)

/**
 * Size of single processing block in bytes
 */
#define SHA256_BLOCK_SIZE 64


struct sha256_ctx
{
  uint32_t H[_SHA256_DIGEST_LENGTH];    /**< Intermediate hash value / digest at end of calculation */
  uint64_t count;                       /**< number of bytes, mod 2^64 */
  uint8_t buffer[SHA256_BLOCK_SIZE];     /**< SHA256 input data buffer */
};

/**
 * Initialise structure for SHA256 calculation.
 *
 * @param ctx_ must be a `struct sha256_ctx *`
 */
void
MHD_SHA256_init (void *ctx_);


/**
 * Process portion of bytes.
 *
 * @param ctx_ must be a `struct sha256_ctx *`
 * @param data bytes to add to hash
 * @param length number of bytes in @a data
 */
void
MHD_SHA256_update (void *ctx_,
               const uint8_t *data,
               size_t length);


/**
 * Finalise SHA256 calculation, return digest.
 *
 * @param ctx_ must be a `struct sha256_ctx *`
 * @param[out] digest set to the hash, must be #SHA256_DIGEST_SIZE bytes
 */
void
sha256_finish (void *ctx_,
               uint8_t digest[SHA256_DIGEST_SIZE]);

#endif /* MHD_SHA256_H */
