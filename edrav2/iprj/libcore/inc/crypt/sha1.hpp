//
// edrav2.libcore project
// 
// Author: Denis Kroshin (28.01.2019)
// Reviewer: Denis Bogdanov (04.02.2019)
//
///
/// @file SHA1 implementation
///

#pragma once

namespace openEdr {
namespace crypt {
namespace sha1 {
namespace detail {

constexpr size_t c_nBlockLen = 64;
constexpr size_t c_nHashLen = 20;

typedef struct Hash {
	uint8_t& operator[](int i) { return byte[i]; }
	uint8_t byte[c_nHashLen];
} Hash;

struct Context {
	uint32_t buffer[c_nBlockLen / 4];
	uint32_t state[c_nHashLen / 4];
	uint64_t byteCount;
	uint8_t bufferOffset;
};

#define SHA1_K0 0x5a827999
#define SHA1_K20 0x6ed9eba1
#define SHA1_K40 0x8f1bbcdc
#define SHA1_K60 0xca62c1d6

//
//
//
static inline uint32_t rol32(uint32_t number, uint8_t bits)
{
	return ((number << bits) | (number >> (32 - bits)));
}

//
//
//
static inline void hashBlock(Context *s)
{
	uint32_t a = s->state[0];
	uint32_t b = s->state[1];
	uint32_t c = s->state[2];
	uint32_t d = s->state[3];
	uint32_t e = s->state[4];
	for (uint8_t i = 0; i < 80; i++)
	{
		uint32_t t = 0;
		if (i >= 16)
		{
			t = s->buffer[(i + 13) & 15] ^ s->buffer[(i + 8) & 15] ^ s->buffer[(i + 2) & 15] ^ s->buffer[i & 15];
			s->buffer[i & 15] = rol32(t, 1);
		}
		if (i < 20) 
			t = (d ^ (b & (c ^ d))) + SHA1_K0;
		else if (i < 40) 
			t = (b ^ c ^ d) + SHA1_K20;
		else if (i < 60) 
			t = ((b & c) | (d & (b | c))) + SHA1_K40;
		else 
			t = (b ^ c ^ d) + SHA1_K60;
		t += rol32(a, 5) + e + s->buffer[i & 15];
		e = d;
		d = c;
		c = rol32(b, 30);
		b = a;
		a = t;
	}
	s->state[0] += a;
	s->state[1] += b;
	s->state[2] += c;
	s->state[3] += d;
	s->state[4] += e;
}

//
//
//
static inline void addUncounted(Context *s, uint8_t data)
{
	uint8_t *const b = (uint8_t *)s->buffer;
#ifdef BIG_ENDIAN
	b[s->bufferOffset] = data;
#else
	b[s->bufferOffset ^ 3] = data;
#endif
	s->bufferOffset++;
	if (s->bufferOffset == c_nBlockLen)
	{
		hashBlock(s);
		s->bufferOffset = 0;
	}
}

//
//
//
inline void init(Context *s)
{
	s->state[0] = 0x67452301;
	s->state[1] = 0xefcdab89;
	s->state[2] = 0x98badcfe;
	s->state[3] = 0x10325476;
	s->state[4] = 0xc3d2e1f0;
	s->byteCount = 0;
	s->bufferOffset = 0;
}

//
//
//
inline void update(Context* pCtx, const void *pData, size_t nSize)
{
	if (pData == nullptr) return;

	const uint8_t *data = static_cast<const uint8_t*>(pData);
	for (; nSize != 0; --nSize)
	{
		++pCtx->byteCount;
		addUncounted(pCtx, *data++);
	}
}

//
//
//
inline Hash finalize(Context* pCtx)
{
	// Pad with 0x80 followed by 0x00 until the end of the block
	addUncounted(pCtx, 0x80);
	while (pCtx->bufferOffset != 56)
		addUncounted(pCtx, 0x00);

	// Append length in the last 8 bytes
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 53)); // Shifting to multiply by 8
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 45)); // as SHA-1 supports bitstreams as well as
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 37)); // byte.
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 29));
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 21));
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 13));
	addUncounted(pCtx, uint8_t(pCtx->byteCount >> 5));
	addUncounted(pCtx, uint8_t(pCtx->byteCount << 3));

#ifndef BIG_ENDIAN
	{ // Swap byte order back
		int i;
		for (i = 0; i < 5; i++) {
			pCtx->state[i] = (((pCtx->state[i]) << 24) & 0xff000000) | (((pCtx->state[i]) << 8) & 0x00ff0000) |
				(((pCtx->state[i]) >> 8) & 0x0000ff00) | (((pCtx->state[i]) >> 24) & 0x000000ff);
		}
	}
#endif

	Hash hash;
	memcpy(&hash, pCtx->state, sizeof(pCtx->state));
	memset(pCtx, 0, sizeof(*pCtx));
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


} // namespace sha1
} // namespace crypt
} // namespace openEdr
