#include "pch.h"
#include "serializer.h"

namespace cmd {
namespace variant {

static const char g_sKey[] = "{57E31810-04AD-40E9-9E2A-8D511CAD8621}";

//
//
//
void StreamSerializer::finalConstruct(Variant vConfig)
{
	m_pDestStream = vConfig["stream"];
	m_pOutStream = m_pDestStream;

	std::string sFormat = vConfig.get("format", "json");
	if (sFormat == "json")
		m_sHdr.eFormat = Codec::Json;
	else if (sFormat == "jsonPretty")
		m_sHdr.eFormat = Codec::JsonPretty;
	else
		error::InvalidArgument(SL, FMT("Unknown output format <" << sFormat << ">")).throwException();

	std::string sCompression = vConfig.get("compression", "");
	if (!sCompression.empty())
	{
		if (m_pTempStream == nullptr)
		{
			m_pTempStream = queryInterface<io::IWritableStream>(createObject(CLSID_MemoryStream));
			m_pOutStream = queryInterface<io::IRawWritableStream>(m_pTempStream);
		}

		if (sCompression == "lzma")
		{
			error::InvalidArgument(SL, FMT("Unimplemented compression method <" << sCompression << ">")).throwException();
		}
		else if (sCompression == "deflate")
		{
			error::InvalidArgument(SL, FMT("Unimplemented compression method <" << sCompression << ">")).throwException();
		}
		else
			error::InvalidArgument(SL, FMT("Unknown compression method <" << sCompression << ">")).throwException();
	}

	std::string sEncryption = vConfig.get("encryption", "aes");
	if (!sEncryption.empty())
	{
		if (m_pTempStream == nullptr)
		{
			m_pTempStream = queryInterface<io::IWritableStream>(createObject(CLSID_MemoryStream));
			m_pOutStream = queryInterface<io::IRawWritableStream>(m_pTempStream);
		}
		if (sEncryption == "aes")
		{
			auto vCfg = Dictionary({
				{"stream", m_pOutStream},
				{"key", g_sKey}
			});
			m_pOutStream = queryInterface<io::IRawWritableStream>(createObject(CLSID_AesEncoder, vCfg));
			m_sHdr.eEncryption = Codec::Aes;
		}
		else if (sEncryption == "rc4")
		{
			error::InvalidArgument(SL, FMT("Unimplemented encryption method <" << sCompression << ">")).throwException();
		}
		else
			error::InvalidArgument(SL, FMT("Unknown encryption method <" << sEncryption << ">")).throwException();
	}
}

//
//
//
void StreamSerializer::write(const void* pBuffer, Size nSize)
{
	if (m_pOutStream == nullptr)
		error::InvalidUsage(SL, "Output stream is undefined").throwException();

	m_pOutStream->write(pBuffer, nSize);
	m_sHdr.nSize += nSize;
	if (m_sHdr.nVersion >= 2)
		m_ctxHash.update(pBuffer, nSize);
}

//
//
//
void StreamSerializer::flush()
{
	// Check if already flushed
	if (m_pOutStream == nullptr)
		return;

	// Flush 
	m_pOutStream->flush();
	m_pOutStream.reset();
	
	// Check if we have encryption/compressor
	if (m_pTempStream == nullptr)
		return;

	// Write header
	m_sHdr.nChecksum = m_ctxHash.finalize();
	m_pDestStream->write(&m_sHdr, sizeof(m_sHdr));

	// Write data
	auto pStream = queryInterface<io::IReadableStream>(m_pTempStream);
	pStream->setPosition(0);
	io::write(m_pDestStream, pStream);
	m_pTempStream.reset();
}

//
//
//
void StreamSerializer::put(const Variant& vData)
{
	if (m_pOutStream == nullptr)
		error::InvalidUsage(SL, "Output stream is undefined").throwException();

	auto pSelfStream = queryInterface<io::IRawWritableStream>(getPtrFromThis());
	if (m_sHdr.eFormat == Codec::Json)
		io::write(pSelfStream, serializeToJson(vData, JsonFormat::SingleLine));
	else if (m_sHdr.eFormat == Codec::JsonPretty)
		io::write(pSelfStream, serializeToJson(vData, JsonFormat::Pretty));
}

//
//
//
void StreamDeserializer::finalConstruct(Variant vConfig)
{
	m_pSrcStream = vConfig["stream"];
	m_nCacheSize = m_pSrcStream->read(&m_nCache, sizeof(m_nCache));
	if (m_nCacheSize == sizeof(m_nCache) && m_nCache == '!RDE')
	{
		Codec eCompession = None;
		Codec eEncryption = None;
		if (m_pSrcStream->read(&m_nVersion, sizeof(m_nVersion)) < sizeof(m_nVersion) ||
			m_pSrcStream->read(&m_eFormat, sizeof(m_eFormat)) < sizeof(m_eFormat) ||
			m_pSrcStream->read(&eCompession, sizeof(eCompession)) < sizeof(eCompession) ||
			m_pSrcStream->read(&eEncryption, sizeof(eEncryption)) < sizeof(eEncryption) ||
			m_pSrcStream->read(&m_nStreamSize, sizeof(m_nStreamSize)) < sizeof(m_nStreamSize))
			error::NoData(SL).throwException();
		if (m_nVersion >= 2)
		{
			if (m_pSrcStream->read(&m_nHash, sizeof(m_nHash)) < sizeof(m_nHash))
				error::NoData(SL).throwException();
		}
		if (m_nVersion > c_nHdrVersion)
			error::InvalidFormat(SL, "Unsupported version").throwException();

		if (eEncryption != Codec::None)
		{
			ObjPtr<IObject> pDecoder;
			if (eEncryption == Codec::Aes)
				pDecoder = createObject(CLSID_AesDecoder, Dictionary({
					{"stream", m_pSrcStream},
					{"key", g_sKey}
				}));
			else
				error::InvalidFormat(SL, "Unknown stream decoder").throwException();
			m_pSrcStream = queryInterface<io::IRawReadableStream>(pDecoder);
		}

		if (eCompession != Codec::None)
		{
		}

		m_nCacheSize = 0;
	}
	else
		m_nStreamSize = io::c_nMaxIoSize;
}

//
//
//
Size StreamDeserializer::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	// Read optimization
	if (m_nStreamPos >= m_nStreamSize)
	{
		if (pProceedSize)
			*pProceedSize = 0;
		return 0;
	}

	Size nTotalRead = 0;
	
	// Read cached data
	if (m_nStreamPos < m_nCacheSize)
	{
		Size nCopySize = std::min(nSize, m_nCacheSize - Size(m_nStreamPos));
		memcpy(pBuffer, (uint8_t*)(&m_nCache) + m_nStreamPos, nCopySize);
		m_nStreamPos += nCopySize;
		nTotalRead = nCopySize;
	}

	CMD_IOBEGIN;
	Size nReadSize = std::min(nSize - nTotalRead, Size(m_nStreamSize - m_nStreamPos));
	CMD_IOSAFE(m_pSrcStream->read((uint8_t*)pBuffer + nTotalRead, nReadSize, &nReadSize));
	m_nStreamPos += nReadSize;
	nTotalRead += nReadSize;
	if (pProceedSize)
		*pProceedSize = nTotalRead;
	CMD_IOEND;

	// Calculate and check CRC
	if (m_nVersion >= 2 && m_nStreamSize != io::c_nMaxIoSize)
	{
		m_ctxHash.update(pBuffer, nTotalRead);
		if (m_nStreamPos == m_nStreamSize)
		{
			auto nHash = m_ctxHash.finalize();
			if(nHash != m_nHash)
				error::InvalidFormat(SL, FMT("Stream checksum mismatch, original <" << m_nHash <<
					">, calculated <" << nHash << ">")).throwException();
		}
	}

	return nTotalRead;
}

//
//
//
std::optional<Variant> StreamDeserializer::get()
{
	TRACE_BEGIN;
	if (m_eFormat == Codec::None)
		error::InvalidFormat(SL).throwException();

	// Read data from file
	auto pReadableStream = io::convertStream(queryInterface<io::IRawReadableStream>(getPtrFromThis()));
	auto nSize = pReadableStream->getSize();
	if (nSize == 0)
		error::NoData(SL, "Source stream is empty").throwException();

	std::vector<char> sBuffer;
	sBuffer.resize((size_t)nSize);

	if (pReadableStream->read(sBuffer.data(), sBuffer.size()) != sBuffer.size())
		error::InvalidFormat(SL).throwException();

	// data -> string_view
	std::string_view sData(sBuffer.data(), sBuffer.size());

	// Check and delete BOM
	if (sData.size() > 3 &&
		sData[0] == '\xEF' && sData[1] == '\xBB' && sData[2] == '\xBF')
		sBuffer[0] = sBuffer[1] = sBuffer[2] = ' ';

	if (m_eFormat == Codec::Json || m_eFormat == Codec::JsonPretty)
		return deserializeFromJson(sData);

	error::InvalidFormat(SL).throwException();
	TRACE_END("Can't deserialize JSON stream");
}

} // namespace variant
} // namespace cmd 
