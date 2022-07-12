//
// edrav2.libcore
// AES stream class declaration
// 
// Author: Denis Kroshin (05.01.2019)
// Reviewer: Denis Bogdanov (09.01.2019)
//
#pragma once
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace crypt {

namespace detail {

#define ROUNDS 14
#define AES_KEY_SIZE   32
#define AES_BLOCK_SIZE 16

typedef __declspec(align(16)) struct _aes256_key {
	__declspec(align(16)) unsigned long enc_key[4 *(ROUNDS + 1)];
	__declspec(align(16)) unsigned long dec_key[4 *(ROUNDS + 1)];
} aes256_key;

void aes256_set_key(const unsigned char *key, aes256_key *skey);
void aes256_encrypt(const unsigned char *in, unsigned char *out, aes256_key *key);
void aes256_decrypt(const unsigned char *in, unsigned char *out, aes256_key *key);
void aes256_gentab();

};

static const Size c_nDefaultCacheSize = 0x1000;

////////////////////////////////////////////////////////////////////////////////
//
// AES Encoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
class AesEncoder : public ObjectBase <CLSID_AesEncoder>, public io::IRawWritableStream
{
private:
	ObjPtr<io::IRawWritableStream> m_pDestStream;
	std::vector<Byte> m_pCacheBuffer;
	Size m_nCachePos = 0;
	AES_ctx m_Ctx = {};
	//detail::aes256_key m_pKey;

protected:
	AesEncoder()
	{};
	
	virtual ~AesEncoder() override
	{
		flush();
	};

public:

	/// @param[in] "stream" Pointer to destination stream
	/// @param[in] "key" String used as crypt key
	/// @param[in][opt] "cacheSize" Size cache
	void finalConstruct(Variant vConfig);

	// IRawWritableStream
	virtual void write(const void* pBuffer, Size nSize) override;
	virtual void flush() override;;
};

////////////////////////////////////////////////////////////////////////////////
//
// AES Decoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
class AesDecoder : public ObjectBase <CLSID_AesDecoder>, public io::IRawReadableStream
{
private:
	ObjPtr<io::IRawReadableStream> m_pSrcStream;
	bool m_fEof = false;
	Byte m_pCache[c_nDefaultCacheSize] = {};
	Size m_nCacheSize = 0;
	Size m_nCachePos = 0;
	AES_ctx m_Ctx = {};

	void fillCache(Size nSize);

protected:
	AesDecoder()
	{};

	virtual ~AesDecoder() override 
	{};

public:

	/// @param[in] "stream" Pointer to source stream
	/// @param[in][opt] "cacheSize" Size cache
	void finalConstruct(Variant vConfig);

	// IRawReadableStream
	virtual Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;
};

} // namespace crypt
} // namespace cmd 