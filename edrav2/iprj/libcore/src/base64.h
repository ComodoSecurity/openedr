//
// edrav2.libcore
// Base64 stream class declaration
// 
// Author: Denis Kroshin (05.01.2019)
// Reviewer: Denis Bogdanov (09.01.2019)
//
#pragma once
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace crypt {

////////////////////////////////////////////////////////////////////////////////
//
// Base64 Encoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
class Base64Encoder : public ObjectBase <CLSID_Base64Encoder>, 
	public io::IRawWritableStream
{
private:
	static const Size c_nDefaultCacheSize = 0x1000;
	static const Size c_nOutBlockSize = 4;

	ObjPtr<io::IRawWritableStream> m_pDestStream;
	MemPtr<Byte> m_pCacheBuffer;
	Size m_nCacheSize = c_nDefaultCacheSize;
	Size m_nCachePos = 0;

	Byte m_pInBlock[3] = {};
	Size m_nInLength = 0;

	void fillCache(bool fFlush = false);

protected:
	Base64Encoder()
	{};
	
	virtual ~Base64Encoder() override
	{};

public:

	/// @param[in] "stream" Pointer to destination stream
	/// @param[in][opt] "cacheSize" Size cache
	void finalConstruct(Variant vConfig);

	// IRawWritableStream
	virtual void write(const void* pBuffer, Size nSize) override;
	virtual void flush() override;;
};

////////////////////////////////////////////////////////////////////////////////
//
// Base64 Decoder
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
class Base64Decoder : public ObjectBase <CLSID_Base64Decoder>, public io::IRawReadableStream
{
private:
	ObjPtr<io::IRawReadableStream> m_pSrcStream;
	bool m_fEof = false;
	Byte m_OutBlock[3] = {};
	Size m_nOutIndex = 3;
	Size m_nOutSize = 0;

	void fillCache();

protected:
	Base64Decoder()
	{};

	virtual ~Base64Decoder() override 
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