//
// EDRAv2.libcore project
// 
// Author: Denis Kroshin (28.01.2019)
// Reviewer: Denis Bogdanov (04.02.2019)
//
///
/// @file MD5 implementation
///
#pragma once

namespace openEdr {
namespace crypt {
namespace md5 {
namespace detail {

constexpr size_t c_nBlockLen = 64;
constexpr size_t c_nHashLen = 16;

typedef struct Hash {
	uint8_t& operator[](int i) { return byte[i]; }
	uint8_t byte[c_nHashLen];
} Hash;

struct Context
{
	uint_fast32_t lo, hi;
	uint_fast32_t a, b, c, d;
	uint8_t buffer[64];
	uint_fast32_t block[c_nHashLen];
};

// 
// The basic MD5 functions.
// 
// F is optimized compared to its RFC 1321 definition just like in Colin
// Plumb's implementation.
// 
#define MD5_F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define MD5_G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | ~(z)))

// 
// The MD5 transformation for all four rounds.
// 
#define MD5_STEP(f, a, b, c, d, x, t, s)						\
    (a) += f((b), (c), (d)) + (x) + (t);						\
    (a) = (((a) << (s)) | (((a)&0xffffffff) >> (32 - (s))));	\
    (a) += (b);


// SET reads 4 input bytes in little-endian byte order and stores them
// in a properly aligned word in host byte order.
// 
// The check for little-endian architectures which tolerate unaligned
// memory accesses is just an optimization.  Nothing will break if it
// doesn't work.
#if defined(__i386) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#define MD5_SET(ctx,ptr,n) (*(const uint32_t *)&ptr[(n)*4])
#define MD5_GET(ctx,ptr,n) MD5_SET(ctx,ptr,n)
#else
#define MD5_SET(ctx,ptr,n)																	\
    (ctx->block[(n)] = (uint_fast32_t)ptr[(n)*4] | ((uint_fast32_t)ptr[(n)*4 + 1] << 8) |	\
		((uint_fast32_t)ptr[(n)*4 + 2] << 16) |	((uint_fast32_t)ptr[(n)*4 + 3] << 24))
#define MD5_GET(ctx,ptr,n) (ctx->block[(n)])
#endif

//
// This processes one or more 64-byte data blocks, but does NOT update
// the bit counters.  There are no alignment requirements.
static const void* transform(Context* pCtx, const uint8_t* pData, size_t nSize)
{
	uint_fast32_t a = pCtx->a;
	uint_fast32_t b = pCtx->b;
	uint_fast32_t c = pCtx->c;
	uint_fast32_t d = pCtx->d;

	do {
		uint_fast32_t saved_a = a;
		uint_fast32_t saved_b = b;
		uint_fast32_t saved_c = c;
		uint_fast32_t saved_d = d;

		/* Round 1 */
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(pCtx, pData, 0), 0xd76aa478, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(pCtx, pData, 1), 0xe8c7b756, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(pCtx, pData, 2), 0x242070db, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(pCtx, pData, 3), 0xc1bdceee, 22);
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(pCtx, pData, 4), 0xf57c0faf, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(pCtx, pData, 5), 0x4787c62a, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(pCtx, pData, 6), 0xa8304613, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(pCtx, pData, 7), 0xfd469501, 22);
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(pCtx, pData, 8), 0x698098d8, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(pCtx, pData, 9), 0x8b44f7af, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(pCtx, pData, 10), 0xffff5bb1, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(pCtx, pData, 11), 0x895cd7be, 22);
		MD5_STEP(MD5_F, a, b, c, d, MD5_SET(pCtx, pData, 12), 0x6b901122, 7);
		MD5_STEP(MD5_F, d, a, b, c, MD5_SET(pCtx, pData, 13), 0xfd987193, 12);
		MD5_STEP(MD5_F, c, d, a, b, MD5_SET(pCtx, pData, 14), 0xa679438e, 17);
		MD5_STEP(MD5_F, b, c, d, a, MD5_SET(pCtx, pData, 15), 0x49b40821, 22);

			/* Round 2 */
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(pCtx, pData, 1), 0xf61e2562, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(pCtx, pData, 6), 0xc040b340, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(pCtx, pData, 11), 0x265e5a51, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(pCtx, pData, 0), 0xe9b6c7aa, 20);
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(pCtx, pData, 5), 0xd62f105d, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(pCtx, pData, 10), 0x02441453, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(pCtx, pData, 15), 0xd8a1e681, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(pCtx, pData, 4), 0xe7d3fbc8, 20);
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(pCtx, pData, 9), 0x21e1cde6, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(pCtx, pData, 14), 0xc33707d6, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(pCtx, pData, 3), 0xf4d50d87, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(pCtx, pData, 8), 0x455a14ed, 20);
		MD5_STEP(MD5_G, a, b, c, d, MD5_GET(pCtx, pData, 13), 0xa9e3e905, 5);
		MD5_STEP(MD5_G, d, a, b, c, MD5_GET(pCtx, pData, 2), 0xfcefa3f8, 9);
		MD5_STEP(MD5_G, c, d, a, b, MD5_GET(pCtx, pData, 7), 0x676f02d9, 14);
		MD5_STEP(MD5_G, b, c, d, a, MD5_GET(pCtx, pData, 12), 0x8d2a4c8a, 20);

			/* Round 3 */
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(pCtx, pData, 5), 0xfffa3942, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(pCtx, pData, 8), 0x8771f681, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(pCtx, pData, 11), 0x6d9d6122, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(pCtx, pData, 14), 0xfde5380c, 23);
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(pCtx, pData, 1), 0xa4beea44, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(pCtx, pData, 4), 0x4bdecfa9, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(pCtx, pData, 7), 0xf6bb4b60, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(pCtx, pData, 10), 0xbebfbc70, 23);
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(pCtx, pData, 13), 0x289b7ec6, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(pCtx, pData, 0), 0xeaa127fa, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(pCtx, pData, 3), 0xd4ef3085, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(pCtx, pData, 6), 0x04881d05, 23);
		MD5_STEP(MD5_H, a, b, c, d, MD5_GET(pCtx, pData, 9), 0xd9d4d039, 4);
		MD5_STEP(MD5_H, d, a, b, c, MD5_GET(pCtx, pData, 12), 0xe6db99e5, 11);
		MD5_STEP(MD5_H, c, d, a, b, MD5_GET(pCtx, pData, 15), 0x1fa27cf8, 16);
		MD5_STEP(MD5_H, b, c, d, a, MD5_GET(pCtx, pData, 2), 0xc4ac5665, 23);

			/* Round 4 */
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(pCtx, pData, 0), 0xf4292244, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(pCtx, pData, 7), 0x432aff97, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(pCtx, pData, 14), 0xab9423a7, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(pCtx, pData, 5), 0xfc93a039, 21);
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(pCtx, pData, 12), 0x655b59c3, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(pCtx, pData, 3), 0x8f0ccc92, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(pCtx, pData, 10), 0xffeff47d, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(pCtx, pData, 1), 0x85845dd1, 21);
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(pCtx, pData, 8), 0x6fa87e4f, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(pCtx, pData, 15), 0xfe2ce6e0, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(pCtx, pData, 6), 0xa3014314, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(pCtx, pData, 13), 0x4e0811a1, 21);
		MD5_STEP(MD5_I, a, b, c, d, MD5_GET(pCtx, pData, 4), 0xf7537e82, 6);
		MD5_STEP(MD5_I, d, a, b, c, MD5_GET(pCtx, pData, 11), 0xbd3af235, 10);
		MD5_STEP(MD5_I, c, d, a, b, MD5_GET(pCtx, pData, 2), 0x2ad7d2bb, 15);
		MD5_STEP(MD5_I, b, c, d, a, MD5_GET(pCtx, pData, 9), 0xeb86d391, 21);

		a += saved_a;
		b += saved_b;
		c += saved_c;
		d += saved_d;

		pData += 64;
	} while (nSize -= 64);

	pCtx->a = a;
	pCtx->b = b;
	pCtx->c = c;
	pCtx->d = d;

	return pData;
}

//
//
//
inline void init(Context* pCtx)
{
	pCtx->a = 0x67452301;
	pCtx->b = 0xefcdab89;
	pCtx->c = 0x98badcfe;
	pCtx->d = 0x10325476;

	pCtx->lo = 0;
	pCtx->hi = 0;
}

//
//
//
inline void update(Context* pCtx, const void* pData, size_t nSize)
{
	if (pData == nullptr) return;

	uint_fast32_t saved_lo = pCtx->lo;
	if ((pCtx->lo = (saved_lo + nSize) & 0x1fffffff) < saved_lo)
		pCtx->hi++;
	pCtx->hi += uint_fast32_t(nSize >> 29);

	unsigned long used = saved_lo & 0x3f;
	if (used)
	{
		unsigned long free = 64 - used;

		if (nSize < free)
		{
			memcpy(&pCtx->buffer[used], pData, nSize);
			return;
		}

		memcpy(&pCtx->buffer[used], pData, free);
		pData = (const uint8_t *)pData + free;
		nSize -= free;
		transform(pCtx, pCtx->buffer, 64);
	}

	if (nSize >= 64)
	{
		pData = transform(pCtx, (uint8_t *)pData, nSize & ~(unsigned long)0x3f);
		nSize &= 0x3f;
	}

	memcpy(pCtx->buffer, pData, nSize);
}

//
//
//
inline Hash finalize(Context* pCtx)
{
	unsigned long used = pCtx->lo & 0x3f;
	pCtx->buffer[used++] = 0x80;

	unsigned long free = 64 - used;
	if (free < 8) {
		memset(&pCtx->buffer[used], 0, free);
		transform(pCtx, pCtx->buffer, 64);
		used = 0;
		free = 64;
	}

	memset(&pCtx->buffer[used], 0, free - 8);

	pCtx->lo <<= 3;
	pCtx->buffer[56] = (uint8_t)(pCtx->lo);
	pCtx->buffer[57] = (uint8_t)(pCtx->lo >> 8);
	pCtx->buffer[58] = (uint8_t)(pCtx->lo >> 16);
	pCtx->buffer[59] = (uint8_t)(pCtx->lo >> 24);
	pCtx->buffer[60] = (uint8_t)(pCtx->hi);
	pCtx->buffer[61] = (uint8_t)(pCtx->hi >> 8);
	pCtx->buffer[62] = (uint8_t)(pCtx->hi >> 16);
	pCtx->buffer[63] = (uint8_t)(pCtx->hi >> 24);

	transform(pCtx, pCtx->buffer, 64);

	Hash pDigest;
	pDigest[0] = (unsigned char)(pCtx->a);
	pDigest[1] = (unsigned char)(pCtx->a >> 8);
	pDigest[2] = (unsigned char)(pCtx->a >> 16);
	pDigest[3] = (unsigned char)(pCtx->a >> 24);
	pDigest[4] = (unsigned char)(pCtx->b);
	pDigest[5] = (unsigned char)(pCtx->b >> 8);
	pDigest[6] = (unsigned char)(pCtx->b >> 16);
	pDigest[7] = (unsigned char)(pCtx->b >> 24);
	pDigest[8] = (unsigned char)(pCtx->c);
	pDigest[9] = (unsigned char)(pCtx->c >> 8);
	pDigest[10] = (unsigned char)(pCtx->c >> 16);
	pDigest[11] = (unsigned char)(pCtx->c >> 24);
	pDigest[12] = (unsigned char)(pCtx->d);
	pDigest[13] = (unsigned char)(pCtx->d >> 8);
	pDigest[14] = (unsigned char)(pCtx->d >> 16);
	pDigest[15] = (unsigned char)(pCtx->d >> 24);

	memset(pCtx, 0, sizeof(*pCtx));
	return pDigest;
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

constexpr size_t c_nHashLen = detail::c_nHashLen;
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

} // namespace md5
} // namespace crypt
} // namespace openEdr
