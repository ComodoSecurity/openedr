#pragma once
#include <objects.h>
#include <io.hpp>
#include <crypt.hpp>

namespace openEdr {
namespace variant {

enum Codec : uint8_t
{
	None = 0,
	Json = 1,
	Aes = 2,
	JsonPretty = 3,
};

static const uint8_t c_nHdrVersion = 2;

#pragma pack(push, 1)
struct StreamHeader
{
	uint32_t nMagic = '!RDE';
	uint8_t nVersion = c_nHdrVersion;
	// Version 1 fields
	Codec eFormat = Codec::None;
	Codec eCompession = Codec::None;
	Codec eEncryption = Codec::None;
	io::IoSize nSize = 0;
	// Version 2 fields
	crypt::crc32::Hash nChecksum = 0;
};
#pragma pack(pop)

//
//
//
class StreamSerializer : 
	public ObjectBase <CLSID_StreamSerializer>, 
	public io::IRawWritableStream,
	public IDataReceiver
{
private:
	ObjPtr<io::IRawWritableStream> m_pDestStream;
	ObjPtr<io::IRawWritableStream> m_pOutStream;
	ObjPtr<io::IWritableStream> m_pTempStream;
	StreamHeader m_sHdr;
	crypt::crc32::Hasher m_ctxHash;

protected:
	StreamSerializer()
	{};
	
	virtual ~StreamSerializer() override
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
	virtual void flush() override;

	// IDataReceiver
	virtual void put(const Variant& vData) override;
};

//
//
//
class StreamDeserializer : 
	public ObjectBase <CLSID_StreamDeserializer>, 
	public io::IRawReadableStream,
	public IDataProvider
{
private:
	ObjPtr<io::IRawReadableStream> m_pSrcStream;
	io::IoSize m_nStreamSize = 0;
	io::IoSize m_nStreamPos = 0;
	Size m_nCacheSize = 0;
	uint32_t m_nCache = 0;
	uint8_t m_nVersion = 0;
	Codec m_eFormat = Codec::None;
	crypt::crc32::Hash m_nHash = 0;
	crypt::crc32::Hasher m_ctxHash;

protected:
	StreamDeserializer()
	{};

	virtual ~StreamDeserializer() override
	{};

public:

	/// @param[in] "stream" Pointer to source stream
	/// @param[in][opt] "cacheSize" Size cache
	void finalConstruct(Variant vConfig);

	// IRawReadableStream
	virtual Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;

	// IDataProvider
	virtual std::optional<Variant> get() override;
};

} // namespace variant
} // namespace openEdr 
