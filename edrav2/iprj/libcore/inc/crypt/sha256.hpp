//
// edrav2.libcore project
// 
// Author: Denis Kroshin (28.01.2019)
// Reviewer: Denis Bogdanov (04.02.2019)
//
///
/// @file SHA256 implementation
///
#pragma once

namespace openEdr {
namespace crypt {
namespace sha256 {
namespace detail {

constexpr size_t c_nBlockLen = 64;
constexpr size_t c_nHashLen = 32;

typedef struct Hash {
	uint8_t& operator[](int i) { return byte[i]; }
	uint8_t byte[c_nHashLen];
} Hash;

struct Context
{
    uint64_t length;
    uint32_t state[c_nHashLen / 4];
    size_t curlen;
    uint8_t buf[c_nBlockLen];
};


#define sha256_ch(x, y, z) (z ^ (x & (y ^ z)))
#define sha256_maj(x, y, z) (((x | y) & z) | (x & y))
#define sha256_s(x, y)	\
    (((((uint32_t)(x)&0xFFFFFFFFUL) >> (uint32_t)((y)&31)) | ((uint32_t)(x) << (uint32_t)(32 - ((y)&31)))) & 0xFFFFFFFFUL)
#define sha256_r(x, n) (((x)&0xFFFFFFFFUL) >> (n))
#define sha256_sigma0(x) (sha256_s(x, 2) ^ sha256_s(x, 13) ^ sha256_s(x, 22))
#define sha256_sigma1(x) (sha256_s(x, 6) ^ sha256_s(x, 11) ^ sha256_s(x, 25))
#define sha256_gamma0(x) (sha256_s(x, 7) ^ sha256_s(x, 18) ^ sha256_r(x, 3))
#define sha256_gamma1(x) (sha256_s(x, 17) ^ sha256_s(x, 19) ^ sha256_r(x, 10))
#define sha256_rnd(a, b, c, d, e, f, g, h, i)									\
    uint32_t t0 = h + sha256_sigma1(e) + sha256_ch(e, f, g) + K[i] + W[i];		\
    uint32_t t1 = sha256_sigma0(a) + sha256_maj(a, b, c);						\
    d += t0;																	\
    h = t0 + t1;

//
//
//
static inline void transform(Context *ctx, const uint8_t* buf)
{
	static const uint32_t K[64] =
	{
		0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL, 0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
		0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL, 0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
		0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL, 0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
		0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL, 0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
		0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL, 0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
		0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL, 0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
		0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL, 0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
		0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL, 0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
	};

	/* copy state into S */
	uint32_t S[8];
	for (int i = 0; i < 8; i++)
		S[i] = ctx->state[i];

	/* copy the state into 512-bits into W[0..15] */
	uint32_t W[64];
	for (int i = 0; i < 16; i++)
		W[i] = (uint32_t)buf[4 * i] << 24 | (uint32_t)buf[4 * i + 1] << 16 | 
		(uint32_t)buf[4 * i + 2] << 8 | (uint32_t)buf[4 * i + 3];

	/* fill W[16..63] */
	for (int i = 16; i < 64; i++)
		W[i] = sha256_gamma1(W[i - 2]) + W[i - 7] + sha256_gamma0(W[i - 15]) + W[i - 16];

	/* Compress */
	for (int i = 0; i < 64; ++i)
	{
		sha256_rnd(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], i);
		uint32_t t = S[7];
		S[7] = S[6];
		S[6] = S[5];
		S[5] = S[4];
		S[4] = S[3];
		S[3] = S[2];
		S[2] = S[1];
		S[1] = S[0];
		S[0] = t;
	}

	/* feedback */
	for (int i = 0; i < 8; i++)
		ctx->state[i] = ctx->state[i] + S[i];
}

//
//
//
inline void init(Context* ctx)
{
	ctx->curlen = 0;
	ctx->length = 0;
	ctx->state[0] = 0x6A09E667UL;
	ctx->state[1] = 0xBB67AE85UL;
	ctx->state[2] = 0x3C6EF372UL;
	ctx->state[3] = 0xA54FF53AUL;
	ctx->state[4] = 0x510E527FUL;
	ctx->state[5] = 0x9B05688CUL;
	ctx->state[6] = 0x1F83D9ABUL;
	ctx->state[7] = 0x5BE0CD19UL;
}

//
//
//
inline void update(Context* ctx, const void* data, size_t len)
{
	const uint8_t *in = static_cast<const uint8_t *>(data);
	while (len > 0)
	{
		if (ctx->curlen == 0 && len >= c_nBlockLen)
		{
			transform(ctx, in);
			ctx->length += c_nBlockLen * 8;
			in += c_nBlockLen;
			len -= c_nBlockLen;
		}
		else
		{
			size_t n = c_nBlockLen - ctx->curlen;
			if (n > len)
				n = len;
			memcpy(ctx->buf + ctx->curlen, in, (size_t)n);
			ctx->curlen += n;
			in += n;
			len -= n;
			if (ctx->curlen == 64)
			{
				transform(ctx, ctx->buf);
				ctx->length += 8 * c_nBlockLen;
				ctx->curlen = 0;
			}
		}
	}
}

//
//
//
inline 	Hash finalize(Context *ctx)
{
	/* increase the length of the message */
	ctx->length += ctx->curlen * 8;

	/* append the '1' bit */
	ctx->buf[ctx->curlen++] = (uint8_t)0x80;

	/* if the length is currently above 56 bytes we append zeros
	 * then compress.  Then we can fall back to padding zeros and length
	 * encoding like normal.
	 */
	if (ctx->curlen > 56)
	{
		while (ctx->curlen < 64)
			ctx->buf[ctx->curlen++] = (uint8_t)0;
		transform(ctx, ctx->buf);
		ctx->curlen = 0;
	}

	/* pad up to 56 bytes of zeros */
	while (ctx->curlen < 56)
		ctx->buf[ctx->curlen++] = (uint8_t)0;

	/* store length */
	for (int i = 0; i != 8; ++i)
		ctx->buf[56 + i] = uint8_t(ctx->length >> (56 - 8 * i));
	transform(ctx, ctx->buf);

	/* copy output */
	Hash hash = {};
	for (int i = 0; i != c_nHashLen / 4; ++i)
	{
		hash[i * 4 + 0] = uint8_t(ctx->state[i] >> 24);
		hash[i * 4 + 1] = uint8_t(ctx->state[i] >> 16);
		hash[i * 4 + 2] = uint8_t(ctx->state[i] >> 8);
		hash[i * 4 + 3] = uint8_t(ctx->state[i]);
	}
	return hash;
}

} // namespace detail 

using detail::Hash;

///
/// Hash calculator class
/// It is used with crypt::updateHash() and crypt::getHash() 
///
class Hasher
{
private:
	detail::Context m_ctx;
public:

	Hasher()
	{
		detail::init(&m_ctx);
	}

	typedef detail::Hash Hash;
	static constexpr size_t c_nBlockLen = detail::c_nBlockLen;
	static constexpr size_t c_nHashLen = detail::c_nHashLen;

	void update(const void* pBuffer, size_t nSize)
	{
		detail::update(&m_ctx, pBuffer, nSize);
	}

	detail::Hash finalize()
	{
		return detail::finalize(&m_ctx);
	}
};

constexpr size_t c_nHashLen = Hasher::c_nHashLen;
typedef Hasher::Hash Hash;

///
/// Common function for fast hash calculating 
/// Pass all params to crypt::updateHash()
/// Use crypt::updateHash() directly for more complex calculation 
///
template<class... Args>
auto getHash(Args&&... args)
{
	Hasher hasher;
	crypt::updateHash(hasher, std::forward<Args>(args)...);
	return hasher.finalize();
}


} // namespace sha256
} // namespace crypt
} // namespace openEdr
