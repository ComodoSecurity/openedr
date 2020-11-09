//
// EDRAv2::libcore
// Streams processing realization
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (08.01.2019)
//
#include "pch.h"
#include "stream.h"
#include "streamcache.h"

namespace openEdr {
namespace io {

////////////////////////////////////////////////////////////////////////////////
//
// RawReadableStreamCache object
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
void RawReadableStreamCache::finalConstruct(Variant vConfig)
{
	m_pSourceStream = vConfig["stream"];
	m_nCacheSize = vConfig.get("cacheSize", c_nDefaultCacheSize);
	m_pCacheBuffer.reset(new Byte[m_nCacheSize]);
}

//
//
//
Size RawReadableStreamCache::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();

	bool fEos = false;
	Size nTotalRead = 0;
	while (nTotalRead < nSize && !fEos)
	{
		CMD_IOBEGIN;
		if (m_nCachePos == m_nCurrCacheSize)
		{
			// Transit read
			if (nSize - nTotalRead >= m_nCacheSize)
				return m_pSourceStream->read((Byte*)pBuffer + nTotalRead, nSize - nTotalRead, pProceedSize);

			m_nCachePos = 0;
			m_nCurrCacheSize = 0;
			CMD_IOSAFE(m_pSourceStream->read(m_pCacheBuffer.get(), m_nCacheSize, &m_nCurrCacheSize));
			fEos = (m_nCurrCacheSize < m_nCacheSize);
		}

		Size nCopySize = std::min<Size>(m_nCurrCacheSize - m_nCachePos, nSize - nTotalRead);
		memcpy((Byte*)pBuffer + nTotalRead, m_pCacheBuffer.get() + m_nCachePos, nCopySize);
		m_nCachePos += nCopySize;
		nTotalRead += nCopySize;
		if (pProceedSize)
			*pProceedSize = nTotalRead;
		CMD_IOEND;
	}
	return nTotalRead;
}

//
//
//
ObjPtr<IRawReadableStream> createStreamCache(ObjPtr<IRawReadableStream> pStream, Size nCacheSize)
{
	auto pObj = createObject(CLSID_RawReadableStreamCache, 
		Dictionary({ {"stream", pStream}, {"cacheSize", nCacheSize} }));
	return queryInterface<IRawReadableStream>(pObj);
}

////////////////////////////////////////////////////////////////////////////////
//
// ReadableStreamCache object
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
void ReadableStreamCache::finalConstruct(Variant vConfig)
{
	m_pSourceStream = vConfig["stream"];
	m_nCacheSize = vConfig.get("cacheSize", c_nDefaultCacheSize);
	m_pCacheBuffer.reset(new Byte[m_nCacheSize]);
}

//
//
//
Size ReadableStreamCache::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();

	bool fEos = false;
	Size nTotalSize = 0;
	while (nTotalSize < nSize && !fEos)
	{
		CMD_IOBEGIN;

		// Update cache
		if (m_nStreamPos < m_nCachePos || m_nStreamPos >= m_nCachePos + m_nCurrCacheSize)
		{
			// Reserve 5% of data to seek backward
			m_nCachePos = m_nStreamPos - std::min<IoSize>(m_nStreamPos, m_nCacheSize / c_nReadBackDevider);
			m_pSourceStream->setPosition(m_nCachePos);
			CMD_IOSAFE(m_pSourceStream->read(m_pCacheBuffer.get(), m_nCacheSize, &m_nCurrCacheSize));
			fEos = (m_nCurrCacheSize < m_nCacheSize);
		}

		// Copy data from cache
		// FIXME: Why do you use second condition?
		if (m_nStreamPos >= m_nCachePos && m_nStreamPos - m_nCachePos < m_nCurrCacheSize)
		{
			Size nCopyOffset = static_cast<Size>(m_nStreamPos - m_nCachePos);
			Size nCopySize = std::min(m_nCurrCacheSize - nCopyOffset, nSize - nTotalSize);
			memcpy((Byte*)pBuffer + nTotalSize, m_pCacheBuffer.get() + nCopyOffset, nCopySize);
			m_nStreamPos += nCopySize;
			nTotalSize += nCopySize;
		}

		if (pProceedSize)
			*pProceedSize = nTotalSize;
		CMD_IOEND(nTotalSize);
	}
	return nTotalSize;
}

//
//
//
IoSize ReadableStreamCache::getPosition()
{
	return m_nStreamPos;
}

//
//
//
IoSize ReadableStreamCache::setPosition(IoOffset nOffset, StreamPos eType)
{	
	IoSize nOldPosition = m_nStreamPos;
	m_nStreamPos = convertPosition(nOffset, eType, m_nStreamPos, getSize());
	return nOldPosition;
}

//
//
//
IoSize ReadableStreamCache::getSize()
{
	if (m_nStreamSize == c_nMaxIoSize)
		m_nStreamSize = m_pSourceStream->getSize(); // Getting full size
	return m_nStreamSize;
}

//
//
//
ObjPtr<IReadableStream> createStreamCache(ObjPtr<IReadableStream> pStream, openEdr::Size nCacheSize)
{
	auto pObj = createObject(CLSID_ReadableStreamCache, 
		Dictionary({ {"stream", pStream}, {"cacheSize", nCacheSize} }));
	return queryInterface<IReadableStream>(pObj);
}

////////////////////////////////////////////////////////////////////////////////
//
// RawReadableStreamConverter object
//
////////////////////////////////////////////////////////////////////////////////

//
//
//
void RawReadableStreamConverter::fillCache(IoSize nNewSize)
{
	// Check cached data size
	IoSize nCurrSize = m_pInCacheStream->getSize();
	if (nCurrSize >= nNewSize || nCurrSize >= m_nStreamSize) 
		return;

	if (m_eLastStatus == ReadStatus::eos)
		return;
	if (m_eLastStatus == ReadStatus::error)
		// FIXME: Why logic error?
		error::LogicError(SL).throwException();
	
	m_pOutCacheStream->setPosition(nCurrSize);
	IoSize nCopySize = std::min(nNewSize, m_nStreamSize) - nCurrSize;
	IoSize nWriteSize = 0;

	CMD_IOBEGIN;
	CMD_IOSAFE(write(m_pOutCacheStream, m_pSourceStream, nCopySize, &nWriteSize));
	if (CMD_IOEXCEPTION)
		// FIXME: Save exception here and throw it next time
		m_eLastStatus = ReadStatus::error;

	if (m_eLastStatus == ReadStatus::ok && nWriteSize != nCopySize)
	{
		if (m_nStreamSize == c_nMaxIoSize)
		{
			m_nStreamSize = m_pInCacheStream->getSize();
			m_eLastStatus = ReadStatus::eos;
		}
		else
		{
			m_eLastStatus = ReadStatus::error;
			error::NoData(SL).throwException();
		}
	}
	CMD_IOEND;
}

//
//
//
void RawReadableStreamConverter::finalConstruct(Variant vConfig)
{
	m_pSourceStream = vConfig["stream"];
	m_nStreamSize = vConfig.get("size", c_nMaxIoSize);

	// TODO: Move to HybridStream
	auto pObjCache = createObject(CLSID_MemoryStream);
	m_pInCacheStream = queryInterface<IReadableStream>(pObjCache);
	m_pOutCacheStream = queryInterface<IWritableStream>(pObjCache);
}

//
//
//
Size RawReadableStreamConverter::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pProceedSize)
		*pProceedSize = 0;
	if (pBuffer == nullptr)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();
	if (m_nOffset >= m_nStreamSize)
		return 0;

	// Read new portion of data
	CMD_IOBEGIN;
	Size nReadSize = static_cast<Size>(std::min<IoSize>(nSize, m_nStreamSize - m_nOffset));
	CMD_IOSAFE(fillCache(m_nOffset + nReadSize));

	m_pInCacheStream->setPosition(m_nOffset);
	Size nTotalRead = 0;
	CMD_IOSAFE(m_pInCacheStream->read(pBuffer, nReadSize, &nTotalRead));
	m_nOffset += nTotalRead;
	
	if (pProceedSize)
		*pProceedSize = nTotalRead;
	CMD_IOEND;
	return nTotalRead;
}

//
//
//
IoSize RawReadableStreamConverter::getPosition()
{
	return m_nOffset;
}

//
//
//
IoSize RawReadableStreamConverter::setPosition(IoOffset nOffset, StreamPos eType)
{
	IoSize nOldPosition = m_nOffset;
	m_nOffset = convertPosition(nOffset, eType, m_nOffset, m_nStreamSize);
	return nOldPosition;
}

//
//
//
IoSize RawReadableStreamConverter::getSize()
{
	if (m_nStreamSize != c_nMaxIoSize)
		return m_nStreamSize;

	fillCache(m_nStreamSize);
	return m_pInCacheStream->getSize();
}

//
//
//
ObjPtr<IReadableStream> convertStream(ObjPtr<IRawReadableStream> pStream, IoSize nStreamSize)
{
	auto pRStream = queryInterfaceSafe<IReadableStream>(pStream);
	if (pRStream != nullptr)
		return pRStream;
	auto pObj = createObject(CLSID_RawReadableStreamConverter, 
		Dictionary({ {"stream", pStream}, {"size", nStreamSize} }));
	return queryInterface<IReadableStream>(pObj);
}

} // namespace io
} // namespace openEdr 