//
// EDRAv2::libcore
// Memory stream class realization
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
#include "pch.h"
#include "memstream.h"
#include "stream.h"

namespace openEdr {
namespace io {

//
// 
//
void MemoryStream::reserveMemory(Size nSize)
{
	if (nSize <= m_nBufferSize) return;
	if (!m_fCanRealloc)
		error::LogicError(SL, "Can't resize external buffer").throwException();
	if (nSize > m_nMaxStreamSize)
		error::LimitExceeded(SL).throwException();

	Size nGrow = std::min(c_nStreamGranulation * m_nResizeCount * m_nResizeCount, c_nMaxGrowSize);
	if (nGrow < c_nMaxGrowSize) ++m_nResizeCount;
	nSize = growToLog2(nSize, nGrow);

	m_pBuffer = reallocMem(m_pBuffer, nSize);
	if (m_pBuffer.get() == nullptr)
		error::BadAlloc(SL).throwException();

	m_pData = m_pBuffer.get();
	m_nBufferSize = nSize;
	return;
}

//
//
//
void MemoryStream::finalConstruct(Variant vConfig)
{
	if (vConfig.isNull())
		return;
	m_nMaxStreamSize = vConfig.get("size", c_nMaxStreamSize);
	if (m_nMaxStreamSize > c_nMaxStreamSize)
		error::InvalidArgument(SL).throwException();
	if (vConfig.get("allocMem", false))
	{
		// FIXME: Change to write() from zeroStream
		reserveMemory(m_nMaxStreamSize);
		m_nDataSize = m_nMaxStreamSize;
		memset(m_pData, 0, m_nDataSize);
	}
}

//
//
//
Size MemoryStream::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();

	Size nCopySize = std::min(nSize, m_nDataSize - m_nOffset);
	if (nCopySize != 0)
	{
		memcpy(pBuffer, m_pData + m_nOffset, nCopySize);
		m_nOffset += nCopySize;
	}

	return (pProceedSize != nullptr) ? *pProceedSize = nCopySize : nCopySize;
}

//
//
//
void MemoryStream::write(const void* pBuffer, Size nSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();
	if (m_fReadOnly)
		error::LogicError("Can't write to read-only stream");

	reserveMemory(m_nOffset + nSize);
	memcpy(m_pData + m_nOffset, pBuffer, nSize);	
	
	if (m_nOffset + nSize > m_nDataSize)
		m_nDataSize = m_nOffset + nSize;
	
	m_nOffset += nSize;
	return;
}

//
//
//
void MemoryStream::setSize(IoSize nSize)
{
	// Need to check size before type cast
	if (nSize > m_nMaxStreamSize)
		error::InvalidArgument(SL).throwException();

	reserveMemory(static_cast<Size>(nSize));
	m_nOffset = m_nDataSize = static_cast<Size>(nSize);
}

//
//
//
IoSize MemoryStream::getPosition()
{
	return m_nOffset;
}

//
//
//
IoSize MemoryStream::setPosition(IoOffset nOffset, StreamPos eType)
{
	IoSize nOldPosition = m_nOffset;
	m_nOffset = static_cast<Size>(convertPosition(nOffset, eType, m_nOffset, m_nDataSize));
	return nOldPosition;
}

//
//
//
IoSize MemoryStream::getSize()
{
	return m_nDataSize;
}

//
//
//
std::pair<void*, Size> MemoryStream::getData()
{
	return std::pair<void*, Size>(m_pData, m_nDataSize);
}

//
//
//
ObjPtr<IReadableStream> createMemoryStream(Size nStreamSize)
{
	Dictionary vSettings;
	if (nStreamSize != c_nMaxSize)
		vSettings = Dictionary({ {"size", nStreamSize}, {"allocMem", true} });

	auto pObj = createObject(CLSID_MemoryStream, vSettings);
	return queryInterface<IReadableStream>(pObj);
}

//
//
//
ObjPtr<IReadableStream> createMemoryStream(void* pBuffer, Size nBufferSize)
{
	return MemoryStream::create(pBuffer, nBufferSize);
}

ObjPtr<IReadableStream> createMemoryStream(const void* pBuffer, Size nBufferSize)
{
	return MemoryStream::create(pBuffer, nBufferSize);
}

} // namespace io
} // namespace openEdr 