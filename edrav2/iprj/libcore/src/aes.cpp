//
// edrav2.libcore
// AES stream class realization
// 
// Author: Denis Kroshin (05.01.2019)
// Reviewer: Denis Bogdanov (09.01.2019)
//
//
#include "pch.h"
#include "aes.h"
#include "crypt.hpp"

namespace cmd {
namespace crypt {

namespace detail {

static unsigned long Te0[256];
static unsigned long Td0[256];
static unsigned char Te4[256];
static unsigned char Td4[256];

#define Td0(x) (Td0[x])
#define Td1(x) _rotr(Td0(x), 24)
#define Td2(x) _rotr(Td0(x), 16)
#define Td3(x) _rotr(Td0(x), 8) 
#define Td4(x) Td4[x]

#define Te0(x) (Te0[x])
#define Te1(x) _rotr(Te0(x), 24)
#define Te2(x) _rotr(Te0(x), 16)
#define Te3(x) _rotr(Te0(x), 8) 
#define Te4(x) Te4[x]
#define WPOLY  0x11b

#define lfsr2(x) ((x & 0x80) ? x<<1 ^ WPOLY: x<<1)
 
static unsigned long key_mix(unsigned long temp)
{
	return (Te4[(unsigned char)(temp >> 8 )]) << 0  ^ (Te4[(unsigned char)(temp >> 16)]) << 8  ^
		   (Te4[(unsigned char)(temp >> 24)]) << 16 ^ (Te4[(unsigned char)(temp >> 0 )]) << 24;
}

void aes256_set_key(const unsigned char *key, aes256_key *skey)
{
	unsigned long *ek, *dk;
   	int  j, i, k;
	unsigned long  t, rcon, d;

	__movsb((unsigned char*)(ek = skey->enc_key), key, AES_KEY_SIZE);
	i = 7; rcon = 1;
	do 
	{
		for (t = key_mix(ek[7]) ^ rcon, j = 0; j < 4; j++) {
			t ^= ek[j]; ek[8 + j] = t;
		}        
		if (--i == 0) break;		
		
		for (t = key_mix(_rotr(ek[11], 24)), j = 4; j < 8; j++) {
			t ^= ek[j]; ek[8 + j] = t;
		}
        ek += 8; rcon <<= 1;
	} while (1);

	ek = skey->enc_key;
	dk = skey->dec_key;

	for (i = 0, j = 4*ROUNDS; i <= j; i += 4, j -= 4) 
	{
		for (k = 0; k < 4; k++) {
			dk[i + k] = ek[j + k]; dk[j + k] = ek[i + k];
		}
	}
	for (i = 0; i < (ROUNDS-1) * 4; i++) 
	{
		t = dk[i + 4], d = 0;

		for (j = 32; j; j -= 8) {
			d ^= _rotr(Td0(Te4[(unsigned char)t]), j); t >>= 8;
		}
		dk[i + 4] = d;
	}
}

void aes256_encrypt(const unsigned char *in, unsigned char *out, aes256_key *key)
{
	unsigned long  s[4];
	unsigned long  t[4];
	unsigned long *rk, x;
	unsigned long  r, n;

	rk   = key->enc_key;
	s[0] = ((unsigned long*)in)[0] ^ *rk++; s[1] = ((unsigned long*)in)[1] ^ *rk++;
    s[2] = ((unsigned long*)in)[2] ^ *rk++; s[3] = ((unsigned long*)in)[3] ^ *rk++;
	r    = ROUNDS-1;

	do
	{
		for (n = 0; n < 4; n++)
		{
			t[n] = (Te0((unsigned char)(s[0] >> 0 ))) ^ (Te1((unsigned char)(s[1] >> 8 ))) ^
				   (Te2((unsigned char)(s[2] >> 16))) ^ (Te3((unsigned char)(s[3] >> 24))) ^ *rk++;

			x = s[0]; s[0] = s[1]; s[1] = s[2]; s[2] = s[3]; s[3] = x;
		}
		s[0] = t[0]; s[1] = t[1]; s[2] = t[2]; s[3] = t[3];
	} while (--r);
  
	for (n = 0; n < 4; n++)
	{
		s[n] = (Te4((unsigned char)(t[0] >> 0 )) << 0 ) ^ (Te4((unsigned char)(t[1] >> 8 )) << 8 ) ^
			   (Te4((unsigned char)(t[2] >> 16)) << 16) ^ (Te4((unsigned char)(t[3] >> 24)) << 24) ^ *rk++;
		
		x = t[0]; t[0] = t[1]; t[1] = t[2]; t[2] = t[3]; t[3] = x;
	}	
	__movsb(out, (const unsigned char*)&s, AES_BLOCK_SIZE);
}

void aes256_decrypt(const unsigned char *in, unsigned char *out, aes256_key *key)
{
	unsigned long  s[4];
	unsigned long  t[4];
	unsigned long *rk, x;
	unsigned long  r, n;
	
	rk   = key->dec_key;
    s[0] = ((unsigned long*)in)[0] ^ *rk++; s[1] = ((unsigned long*)in)[1] ^ *rk++;
    s[2] = ((unsigned long*)in)[2] ^ *rk++; s[3] = ((unsigned long*)in)[3] ^ *rk++;
	r    = ROUNDS-1;

	do
	{
		for (n = 0; n < 4; n++)
		{
			t[n] = (Td0((unsigned char)(s[0] >> 0 ))) ^ (Td1((unsigned char)(s[3] >> 8 ))) ^
				   (Td2((unsigned char)(s[2] >> 16))) ^ (Td3((unsigned char)(s[1] >> 24))) ^ *rk++;

			x = s[0]; s[0] = s[1]; s[1] = s[2]; s[2] = s[3]; s[3] = x;
		}		
		s[0] = t[0]; s[1] = t[1]; s[2] = t[2]; s[3] = t[3];
	} while (--r);

	for (n = 0; n < 4; n++)
	{
		s[n] = (Td4((unsigned char)(t[0] >> 0 )) << 0 ) ^ (Td4((unsigned char)(t[3] >> 8 )) << 8 ) ^
			   (Td4((unsigned char)(t[2] >> 16)) << 16) ^ (Td4((unsigned char)(t[1] >> 24)) << 24) ^ *rk++;
		
		x = t[0]; t[0] = t[1]; t[1] = t[2]; t[2] = t[3]; t[3] = x;
	}
	__movsb(out, (const unsigned char*)&s, AES_BLOCK_SIZE);
}

void aes256_gentab()
{
	unsigned char pow[256], log[256];
	unsigned char i, w;
		
	i = 0; w = 1; 
	do
    {
        pow[i] = w;
		log[w] = i;          
		w ^= lfsr2(w);
    } while(++i);

	log[0] = 0; pow[255] = 0; i = 0;
    do
    {
        w = pow[255 - log[i]];		
		w ^= w << 1 ^ w << 2 ^ w << 3 ^ w << 4 ^
			 w >> 4 ^ w >> 5 ^ w >> 6 ^ w >> 7 ^ (1<<6 ^ 1<<5 ^ 1<<1 ^ 1<<0);        
        Te4[i] = w;
        Td4[w] = i;
    } while(++i);

	i = 0;
    do
    {
		unsigned char f = Te4[i]; 
		unsigned char r = Td4[i];
		unsigned char x = unsigned char(lfsr2(f));

        Te0[i] = (f ^ x) << 24 | f << 16 | f << 8 | x;
		Td0[i] = ! r ? r :
                 pow[(0x68 + log[r]) % 255] << 24 ^
                 pow[(0xEE + log[r]) % 255] << 16 ^
                 pow[(0xC7 + log[r]) % 255] <<  8 ^
                 pow[(0xDF + log[r]) % 255] <<  0;     
    } while (++i);
}

};

////////////////////////////////////////////////////////////////////////////////
//
// AES Encoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
void AesEncoder::finalConstruct(Variant vConfig)
{
	m_pDestStream = vConfig["stream"];
	std::string sKey = vConfig["key"];
	AES_init_ctx(&m_Ctx, crypt::md5::getHash(sKey).byte);

	Size nCacheSize = vConfig.get("cacheSize", c_nDefaultCacheSize);
	nCacheSize = growToLog2<Size>(nCacheSize, AES_BLOCKLEN);
	m_pCacheBuffer.resize(nCacheSize);
}

//
//
//
void AesEncoder::write(const void* pBuffer, Size nSize)
{
	// Validate incoming parameter
	if (pBuffer == NULL)
		error::InvalidArgument(SL).throwException();

	Size nTotalSize = 0;
	while (nTotalSize < nSize)
	{
		Size nCopySize = std::min(nSize - nTotalSize, m_pCacheBuffer.size() - m_nCachePos);
		memcpy(m_pCacheBuffer.data() + m_nCachePos, (Byte*)pBuffer + nTotalSize, nCopySize);
		m_nCachePos += nCopySize;
		nTotalSize += nCopySize;

		if (m_nCachePos == m_pCacheBuffer.size())
			flush();
	}
}

//
//
//
void AesEncoder::flush()
{
	if (m_nCachePos == 0)
		return;

	Size nTail = m_nCachePos % AES_BLOCKLEN;
	if (nTail)
	{
		nTail = AES_BLOCKLEN - nTail;
		memcpy(m_pCacheBuffer.data() + m_nCachePos, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", nTail);
		m_nCachePos += nTail;
	}
	AES_CBC_encrypt_buffer(&m_Ctx, m_pCacheBuffer.data(), m_nCachePos);
	m_pDestStream->write(m_pCacheBuffer.data(), m_nCachePos);
	m_nCachePos = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// AES Decoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
void AesDecoder::finalConstruct(Variant vConfig)
{
	m_pSrcStream = vConfig["stream"];
	std::string sKey = vConfig["key"];
	AES_init_ctx(&m_Ctx, crypt::md5::getHash(sKey).byte);
	m_nCachePos = 0;
}

//
//
//
void AesDecoder::fillCache(Size nSize)
{
	if (m_nCachePos < m_nCacheSize || m_fEof)
		return;

	m_nCachePos = 0;
	Size nReadSize = std::min<Size>(growToLog2<Size>(nSize, AES_BLOCKLEN), std::size(m_pCache));
	m_pSrcStream->read(m_pCache, nReadSize, &m_nCacheSize);
	if (m_nCacheSize == 0)
	{
		m_fEof = true;
		return;
	}

	AES_CBC_decrypt_buffer(&m_Ctx, m_pCache, m_nCacheSize);

	if (m_nCacheSize % AES_BLOCKLEN != 0)
		error::NoData(SL, "Invalid AES stream size (must be bound to block size)").throwException();
}

//
//
//
Size AesDecoder::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL).throwException();

	Size nTotalSize = 0;
	while (nTotalSize < nSize && !m_fEof)
	{
		CMD_IOBEGIN;
		CMD_IOSAFE(fillCache(nSize - nTotalSize));
		Size nCopySize = std::min(m_nCacheSize - m_nCachePos, nSize - nTotalSize);
		memcpy((Byte*)pBuffer + nTotalSize, m_pCache + m_nCachePos, nCopySize);
		m_nCachePos += nCopySize;
		nTotalSize += nCopySize;
		if (pProceedSize != nullptr)
			*pProceedSize = nTotalSize;
		CMD_IOEND;
	}
	return nTotalSize;
}

} // namespace crypt
} // namespace cmd 