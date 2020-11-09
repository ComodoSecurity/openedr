/*
 * This code implements the MD5 message-digest algorithm.
 * The algorithm is due to Ron Rivest.	This code was
 * written by Colin Plumb in 1993, no copyright is claimed.
 * This code is in the public domain; do with it what you wish.
 *
 * Equivalent code is available from RSA Data Security, Inc.
 * This code has been tested against that, and is equivalent,
 * except that you don't need to include two pages of legalese
 * with every copy.
 *
 * To compute the message digest of a chunk of bytes, declare an
 * MD5Context structure, pass it to MHD_MD5Init, call MHD_MD5Update as
 * needed on buffers full of bytes, and then call MHD_MD5Final, which
 * will fill a supplied 16-byte array with the digest.
 */

#ifndef MHD_MD5_H
#define MHD_MD5_H

#include "mhd_options.h"
#include <stdint.h>
#include <stddef.h>

#define	MD5_BLOCK_SIZE              64
#define	MD5_DIGEST_SIZE             16
#define	MD5_DIGEST_STRING_LENGTH    (MD5_DIGEST_SIZE * 2 + 1)

struct MD5Context
{
  uint32_t state[4];			/* state */
  uint64_t count;			/* number of bytes, mod 2^64 */
  uint8_t buffer[MD5_BLOCK_SIZE];	/* input buffer */
};


/**
 * Start MD5 accumulation.  Set bit count to 0 and buffer to mysterious
 * initialization constants.
 *
 * @param ctx_ must be a `struct MD5Context *`
 */
void
MHD_MD5Init (void *ctx_);


/**
 * Update context to reflect the concatenation of another buffer full
 * of bytes.
 *
 * @param ctx_ must be a `struct MD5Context *`
 */
void
MHD_MD5Update (void *ctx_,
           const uint8_t *input,
           size_t len);


/**
 * Final wrapup--call MD5Pad, fill in digest and zero out ctx.
 *
 * @param ctx_ must be a `struct MD5Context *`
 */
void
MHD_MD5Final (void *ctx_,
          uint8_t digest[MD5_DIGEST_SIZE]);


#endif /* !MHD_MD5_H */
