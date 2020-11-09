/*
     This file is part of libmicrohttpd
     Copyright (C) 2019 Karlson2k (Evgeny Grin)
     Some ideas are based on Libgcrypt implementation.
     Copyright (C) 2003, 2006, 2008, 2009 Free Software Foundation, Inc.

     libmicrohttpd is free software; you can redistribute it and/or
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
 * @file microhttpd/sha256.c
 * @brief  Calculation of SHA-256 digest as defined in FIPS PUB 180-4 (2015)
 * @author Karlson2k (Evgeny Grin)
 */

/* Some tricks are based on Libgcrypt implementation. */

#include "sha256.h"

#include <string.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif /* HAVE_MEMORY_H */
#include "mhd_bithelpers.h"
#include "mhd_assert.h"

/**
 * Initialise structure for SHA256 calculation.
 *
 * @param ctx_ must be a `struct sha256_ctx *`
 */
void
MHD_SHA256_init (void *ctx_)
{
  struct sha256_ctx *const ctx = ctx_;
  /* Initial hash values, see FIPS PUB 180-4 paragraph 5.3.3 */
  /* First thirty-two bits of the fractional parts of the square
   * roots of the first eight prime numbers: 2, 3, 5, 7, 11, 13,
   * 17, 19." */
  ctx->H[0] = 0x6a09e667UL;
  ctx->H[1] = 0xbb67ae85UL;
  ctx->H[2] = 0x3c6ef372UL;
  ctx->H[3] = 0xa54ff53aUL;
  ctx->H[4] = 0x510e527fUL;
  ctx->H[5] = 0x9b05688cUL;
  ctx->H[6] = 0x1f83d9abUL;
  ctx->H[7] = 0x5be0cd19UL;

  /* Initialise number of bytes. */
  ctx->count = 0;
}

/**
 * Number of bytes in single SHA-256 word
 * used to process data
 */
#define SHA256_BYTES_IN_WORD 4

/**
 * Base of SHA-256 transformation.
 * Gets full 64 bytes block of data and updates hash values;
 * @param H     hash values
 * @param data  data, must be exactly 64 bytes long
 */
static void
sha256_transform (uint32_t H[_SHA256_DIGEST_LENGTH],
                  const uint8_t data[SHA256_BLOCK_SIZE])
{
  /* Working variables,
     see FIPS PUB 180-4 paragraph 6.2. */
  uint32_t a = H[0];
  uint32_t b = H[1];
  uint32_t c = H[2];
  uint32_t d = H[3];
  uint32_t e = H[4];
  uint32_t f = H[5];
  uint32_t g = H[6];
  uint32_t h = H[7];

  /* Data buffer, used as cyclic buffer.
     See FIPS PUB 180-4 paragraphs 5.2.1, 6.2. */
  uint32_t W[16];

  /* 'Ch' and 'Maj' macro functions are defined with
     widely-used optimization.
     See FIPS PUB 180-4 formulae 4.2, 4.3. */
#define Ch(x,y,z)     ( (z) ^ ((x) & ((y) ^ (z))) )
#define Maj(x,y,z)    ( ((x) & (y)) ^ ((z) & ((x) ^ (y))) )
  /* Unoptimized (original) versions: */
/* #define Ch(x,y,z)  ( ( (x) & (y) ) | ( ~(x) & (z) ) )          */
/* #define Maj(x,y,z) ( ((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)) ) */

  /* Four 'Sigma' macro functions.
     See FIPS PUB 180-4 formulae 4.4, 4.5, 4.6, 4.7. */
#define SIG0(x)  ( _MHD_ROTR32((x),2) ^ _MHD_ROTR32((x),13) ^ _MHD_ROTR32((x),22) )
#define SIG1(x)  ( _MHD_ROTR32((x),6) ^ _MHD_ROTR32((x),11) ^ _MHD_ROTR32((x),25) )
#define sig0(x)  ( _MHD_ROTR32((x),7)  ^ _MHD_ROTR32((x),18) ^ ((x) >> 3)  )
#define sig1(x)  ( _MHD_ROTR32((x),17) ^ _MHD_ROTR32((x),19) ^ ((x) >> 10) )

  /* Single step of SHA-256 computation,
     see FIPS PUB 180-4 paragraph 6.2.2 step 3.
   * Note: instead of reassigning all working variables on each step,
           variables are rotated for each step:
             SHA2STEP32(a, b, c, d, e, f, g, h, K[0], data[0]);
             SHA2STEP32(h, a, b, c, d, e, f, g, K[1], data[1]);
           so current 'vD' will be used as 'vE' on next step,
           current 'vH' will be used as 'vA' on next step.
   * Note: first (vH += SIG1(vE) + Ch(vE,vF,vG) + kt + wt) equals T1 in FIPS PUB 180-4 paragraph 6.2.2 step 3.
           second (vH += SIG0(vA) + Maj(vE,vF,vC) equals T1 + T2 in FIPS PUB 180-4 paragraph 6.2.2 step 3.
   * Note: 'wt' must be used exactly one time in this macro as it change other data as well
           every time when used. */
#define SHA2STEP32(vA,vB,vC,vD,vE,vF,vG,vH,kt,wt) do {                    \
	(vD) += ((vH) += SIG1((vE)) + Ch((vE),(vF),(vG)) + (kt) + (wt));  \
        (vH) += SIG0((vA)) + Maj((vA),(vB),(vC)); } while (0)

  /* Get value of W(t) from input data buffer,
     See FIPS PUB 180-4 paragraph 6.2.
     Input data must be read in big-endian bytes order,
     see FIPS PUB 180-4 paragraph 3.1.2. */
#define GET_W_FROM_DATA(buf,t) \
        _MHD_GET_32BIT_BE(((const uint8_t*)(buf)) + (t) * SHA256_BYTES_IN_WORD)

  /* During first 16 steps, before making any calculations on each step,
     the W element is read from input data buffer as big-endian value and
     stored in array of W elements. */
  /* Note: instead of using K constants as array, all K values are specified
     individually for each step, see FIPS PUB 180-4 paragraph 4.2.2 for K values. */
  SHA2STEP32(a, b, c, d, e, f, g, h, 0x428a2f98UL, W[0]  = GET_W_FROM_DATA(data,0));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0x71374491UL, W[1]  = GET_W_FROM_DATA(data,1));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0xb5c0fbcfUL, W[2]  = GET_W_FROM_DATA(data,2));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0xe9b5dba5UL, W[3]  = GET_W_FROM_DATA(data,3));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0x3956c25bUL, W[4]  = GET_W_FROM_DATA(data,4));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0x59f111f1UL, W[5]  = GET_W_FROM_DATA(data,5));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0x923f82a4UL, W[6]  = GET_W_FROM_DATA(data,6));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0xab1c5ed5UL, W[7]  = GET_W_FROM_DATA(data,7));
  SHA2STEP32(a, b, c, d, e, f, g, h, 0xd807aa98UL, W[8]  = GET_W_FROM_DATA(data,8));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0x12835b01UL, W[9]  = GET_W_FROM_DATA(data,9));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0x243185beUL, W[10] = GET_W_FROM_DATA(data,10));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0x550c7dc3UL, W[11] = GET_W_FROM_DATA(data,11));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0x72be5d74UL, W[12] = GET_W_FROM_DATA(data,12));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0x80deb1feUL, W[13] = GET_W_FROM_DATA(data,13));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0x9bdc06a7UL, W[14] = GET_W_FROM_DATA(data,14));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0xc19bf174UL, W[15] = GET_W_FROM_DATA(data,15));

  /* 'W' generation and assignment for 16 <= t <= 63.
     See FIPS PUB 180-4 paragraph 6.2.2.
     As only last 16 'W' are used in calculations, it is possible to
     use 16 elements array of W as cyclic buffer.
   * Note: ((t-16)&0xf) have same value as (t&0xf) */
#define Wgen(w,t) ( (w)[(t-16)&0xf] + sig1((w)[((t)-2)&0xf]) + \
                    (w)[((t)-7)&0xf] + sig0((w)[((t)-15)&0xf]) )

  /* During last 48 steps, before making any calculations on each step,
     W element is generated from W elements of cyclic buffer and generated value
     stored back in cyclic buffer. */
  /* Note: instead of using K constants as array, all K values are specified
     individually for each step, see FIPS PUB 180-4 paragraph 4.2.2 for K values. */
  SHA2STEP32(a, b, c, d, e, f, g, h, 0xe49b69c1UL, W[16&0xf] = Wgen(W,16));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0xefbe4786UL, W[17&0xf] = Wgen(W,17));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0x0fc19dc6UL, W[18&0xf] = Wgen(W,18));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0x240ca1ccUL, W[19&0xf] = Wgen(W,19));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0x2de92c6fUL, W[20&0xf] = Wgen(W,20));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0x4a7484aaUL, W[21&0xf] = Wgen(W,21));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0x5cb0a9dcUL, W[22&0xf] = Wgen(W,22));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0x76f988daUL, W[23&0xf] = Wgen(W,23));
  SHA2STEP32(a, b, c, d, e, f, g, h, 0x983e5152UL, W[24&0xf] = Wgen(W,24));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0xa831c66dUL, W[25&0xf] = Wgen(W,25));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0xb00327c8UL, W[26&0xf] = Wgen(W,26));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0xbf597fc7UL, W[27&0xf] = Wgen(W,27));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0xc6e00bf3UL, W[28&0xf] = Wgen(W,28));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0xd5a79147UL, W[29&0xf] = Wgen(W,29));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0x06ca6351UL, W[30&0xf] = Wgen(W,30));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0x14292967UL, W[31&0xf] = Wgen(W,31));
  SHA2STEP32(a, b, c, d, e, f, g, h, 0x27b70a85UL, W[32&0xf] = Wgen(W,32));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0x2e1b2138UL, W[33&0xf] = Wgen(W,33));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0x4d2c6dfcUL, W[34&0xf] = Wgen(W,34));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0x53380d13UL, W[35&0xf] = Wgen(W,35));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0x650a7354UL, W[36&0xf] = Wgen(W,36));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0x766a0abbUL, W[37&0xf] = Wgen(W,37));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0x81c2c92eUL, W[38&0xf] = Wgen(W,38));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0x92722c85UL, W[39&0xf] = Wgen(W,39));
  SHA2STEP32(a, b, c, d, e, f, g, h, 0xa2bfe8a1UL, W[40&0xf] = Wgen(W,40));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0xa81a664bUL, W[41&0xf] = Wgen(W,41));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0xc24b8b70UL, W[42&0xf] = Wgen(W,42));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0xc76c51a3UL, W[43&0xf] = Wgen(W,43));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0xd192e819UL, W[44&0xf] = Wgen(W,44));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0xd6990624UL, W[45&0xf] = Wgen(W,45));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0xf40e3585UL, W[46&0xf] = Wgen(W,46));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0x106aa070UL, W[47&0xf] = Wgen(W,47));
  SHA2STEP32(a, b, c, d, e, f, g, h, 0x19a4c116UL, W[48&0xf] = Wgen(W,48));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0x1e376c08UL, W[49&0xf] = Wgen(W,49));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0x2748774cUL, W[50&0xf] = Wgen(W,50));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0x34b0bcb5UL, W[51&0xf] = Wgen(W,51));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0x391c0cb3UL, W[52&0xf] = Wgen(W,52));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0x4ed8aa4aUL, W[53&0xf] = Wgen(W,53));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0x5b9cca4fUL, W[54&0xf] = Wgen(W,54));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0x682e6ff3UL, W[55&0xf] = Wgen(W,55));
  SHA2STEP32(a, b, c, d, e, f, g, h, 0x748f82eeUL, W[56&0xf] = Wgen(W,56));
  SHA2STEP32(h, a, b, c, d, e, f, g, 0x78a5636fUL, W[57&0xf] = Wgen(W,57));
  SHA2STEP32(g, h, a, b, c, d, e, f, 0x84c87814UL, W[58&0xf] = Wgen(W,58));
  SHA2STEP32(f, g, h, a, b, c, d, e, 0x8cc70208UL, W[59&0xf] = Wgen(W,59));
  SHA2STEP32(e, f, g, h, a, b, c, d, 0x90befffaUL, W[60&0xf] = Wgen(W,60));
  SHA2STEP32(d, e, f, g, h, a, b, c, 0xa4506cebUL, W[61&0xf] = Wgen(W,61));
  SHA2STEP32(c, d, e, f, g, h, a, b, 0xbef9a3f7UL, W[62&0xf] = Wgen(W,62));
  SHA2STEP32(b, c, d, e, f, g, h, a, 0xc67178f2UL, W[63&0xf] = Wgen(W,63));

  /* Compute intermediate hash.
     See FIPS PUB 180-4 paragraph 4.2.2 step 4. */
  H[0] += a;
  H[1] += b;
  H[2] += c;
  H[3] += d;
  H[4] += e;
  H[5] += f;
  H[6] += g;
  H[7] += h;
}

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
               size_t length)
{
  struct sha256_ctx *const ctx = ctx_;
  unsigned bytes_have; /**< Number of bytes in buffer */

  mhd_assert((data != NULL) || (length == 0));

  if (0 == length)
    return; /* Do nothing */

  /* Note: (count & (SHA256_BLOCK_SIZE-1))
           equal (count % SHA256_BLOCK_SIZE) for this block size. */
  bytes_have = (unsigned)(ctx->count & (SHA256_BLOCK_SIZE-1));
  ctx->count += length;

  if (0 != bytes_have)
    {
      unsigned bytes_left = SHA256_BLOCK_SIZE - bytes_have;
      if (length >= bytes_left)
        { /* Combine new data with data in buffer and
             process full block. */
          memcpy (ctx->buffer + bytes_have,
                  data,
                  bytes_left);
          data += bytes_left;
          length -= bytes_left;
          sha256_transform (ctx->H, ctx->buffer);
          bytes_have = 0;
        }
    }

  while (SHA256_BLOCK_SIZE <= length)
    { /* Process any full blocks of new data directly,
         without copying to buffer. */
      sha256_transform (ctx->H, data);
      data += SHA256_BLOCK_SIZE;
      length -= SHA256_BLOCK_SIZE;
    }

  if (0 != length)
    { /* Copy incomplete block of new data (if any)
         to buffer. */
      memcpy (ctx->buffer + bytes_have, data, length);
    }
}


/**
 * Size of "length" padding addition in bytes.
 * See FIPS PUB 180-4 paragraph 5.1.1.
 */
#define SHA256_SIZE_OF_LEN_ADD (64 / 8)

/**
 * Finalise SHA256 calculation, return digest.
 *
 * @param ctx_ must be a `struct sha256_ctx *`
 * @param[out] digest set to the hash, must be #SHA256_DIGEST_SIZE bytes
 */
void
sha256_finish (void *ctx_,
               uint8_t digest[SHA256_DIGEST_SIZE])
{
  struct sha256_ctx *const ctx = ctx_;
  uint64_t num_bits;   /**< Number of processed bits */
  unsigned bytes_have; /**< Number of bytes in buffer */

  num_bits = ctx->count << 3;
  /* Note: (count & (SHA256_BLOCK_SIZE-1))
           equal (count % SHA256_BLOCK_SIZE) for this block size. */
  bytes_have = (unsigned)(ctx->count & (SHA256_BLOCK_SIZE-1));

  /* Input data must be padded with bit "1" and with length of data in bits.
     See FIPS PUB 180-4 paragraph 5.1.1. */
  /* Data is always processed in form of bytes (not by individual bits),
     therefore position of first padding bit in byte is always predefined (0x80). */
  /* Buffer always have space at least for one byte (as full buffers are
     processed immediately). */
  ctx->buffer[bytes_have++] = 0x80;

  if (SHA256_BLOCK_SIZE - bytes_have < SHA256_SIZE_OF_LEN_ADD)
    { /* No space in current block to put total length of message.
         Pad current block with zeros and process it. */
      while (bytes_have < SHA256_BLOCK_SIZE) ctx->buffer[bytes_have++] = 0;
      /* Process full block. */
      sha256_transform (ctx->H, ctx->buffer);
      /* Start new block. */
      bytes_have = 0;
    }

  /* Pad the rest of the buffer with zeros. */
  memset(ctx->buffer + bytes_have, 0,
         SHA256_BLOCK_SIZE - SHA256_SIZE_OF_LEN_ADD - bytes_have);
  /* Put number of bits in processed message as big-endian value. */
  _MHD_PUT_64BIT_BE(ctx->buffer + SHA256_BLOCK_SIZE - SHA256_SIZE_OF_LEN_ADD, num_bits);
  /* Process full final block. */
  sha256_transform (ctx->H, ctx->buffer);

  /* Put final hash/digest in BE mode */
  _MHD_PUT_32BIT_BE(digest + 0 * SHA256_BYTES_IN_WORD, ctx->H[0]);
  _MHD_PUT_32BIT_BE(digest + 1 * SHA256_BYTES_IN_WORD, ctx->H[1]);
  _MHD_PUT_32BIT_BE(digest + 2 * SHA256_BYTES_IN_WORD, ctx->H[2]);
  _MHD_PUT_32BIT_BE(digest + 3 * SHA256_BYTES_IN_WORD, ctx->H[3]);
  _MHD_PUT_32BIT_BE(digest + 4 * SHA256_BYTES_IN_WORD, ctx->H[4]);
  _MHD_PUT_32BIT_BE(digest + 5 * SHA256_BYTES_IN_WORD, ctx->H[5]);
  _MHD_PUT_32BIT_BE(digest + 6 * SHA256_BYTES_IN_WORD, ctx->H[6]);
  _MHD_PUT_32BIT_BE(digest + 7 * SHA256_BYTES_IN_WORD, ctx->H[7]);

  /* Erase potentially sensitive data. */
  memset(ctx, 0, sizeof(struct sha256_ctx));
}
