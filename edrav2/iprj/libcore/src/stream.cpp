//
// EDRAv2::libcore
// Streams processing realization
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
#include "pch.h"
#include "stream.h"
#include "service.hpp"
#include "common.hpp"

namespace openEdr {
namespace io {

//
//
//
IoSize convertPosition(IoOffset nOffset, StreamPos eType, IoSize nCurPos, IoSize nMaxPos)
{
	switch (eType)
	{
		case StreamPos::Begin:
		{
			if (nOffset >= 0 && IoSize(nOffset) <= nMaxPos)
				return IoSize(nOffset);
			error::InvalidArgument(SL, "Incorrect stream offset").throwException();
			break;
		}
		case StreamPos::End:
		{
			if (nOffset <= 0 && IoSize(-nOffset) <= nMaxPos)
				return nMaxPos - IoSize(-nOffset);
			error::InvalidArgument(SL, "Incorrect stream offset").throwException();
			break;
		}
		case StreamPos::Current:
		{
			if (nOffset < 0)
			{
				if (IoSize(-nOffset) > nCurPos)
					error::InvalidArgument(SL, "Incorrect stream offset").throwException();
				return nCurPos - IoSize(-nOffset);
			}
			if (IoSize(nOffset) > nMaxPos - nCurPos)
				error::InvalidArgument(SL, "Incorrect stream offset").throwException();
			return nCurPos + IoSize(nOffset);
		}
	}
	error::InvalidArgument(SL, "Unknown stream offset type").throwException();
}

//
//
//
void ChildStream::finalConstruct(Variant vConfig)
{
	m_pSourceStream = vConfig["stream"];
	m_nSourceOffset = vConfig["offset"];
	m_nSize = vConfig["size"];
}

//
//
//
Size ChildStream::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid parameters").throwException();
	if (pProceedSize != nullptr)
		*pProceedSize = 0;
	
	CMD_IOBEGIN;
	m_pSourceStream->setPosition(m_nSourceOffset + m_nCurrPos);
	Size nReadSize = 0;
	CMD_IOSAFE(m_pSourceStream->read(pBuffer, nSize, &nReadSize));
	m_nCurrPos += nReadSize;
	if (pProceedSize != nullptr)
		*pProceedSize = nReadSize;
	CMD_IOEND;

	// Update m_nSize in case of continuous read
	if (nReadSize > 0 && nReadSize < nSize && m_nSize == c_nMaxIoSize)
		m_nSize = m_nCurrPos;
	return nReadSize;
}

//
//
//
IoSize ChildStream::getPosition()
{
	return m_nCurrPos;
}

//
//
//
IoSize ChildStream::setPosition(IoOffset nOffset, StreamPos eType)
{
	IoSize nOldPosition = m_nCurrPos;
	m_nCurrPos = convertPosition(nOffset, eType, m_nCurrPos, getSize());;
	return nOldPosition;
}

//
//
//
IoSize ChildStream::getSize()
{
	if (m_nSize != c_nMaxIoSize)
		return m_nSize;

	IoSize nSourceSize = m_pSourceStream->getSize();
	if (m_nSourceOffset > nSourceSize)
		error::LogicError(SL).throwException();
	m_nSize = nSourceSize - m_nSourceOffset;
	return m_nSize;
}

//
//
//
ObjPtr<IReadableStream> createChildStream(ObjPtr<IReadableStream> pStream, Size nOffset, Size nSize)
{
	auto pObj = createObject(CLSID_PartOfStream, 
		Dictionary({ {"stream", pStream}, {"offset", nOffset}, {"size", nSize} }));
	return queryInterface<IReadableStream>(pObj);
}

//
//
//
IoSize write(ObjPtr<IRawWritableStream> pDst, ObjPtr<IRawReadableStream> pStr, IoSize nSize, 
	IoSize* pProceedSize, Size nBufferSize)
{
	IoSize nTotalSize = 0;

	Byte pStaticBufer[c_nMaxStackBufferSize];
	Byte* pBuffer = pStaticBufer;
	MemPtr<Byte> pDynamicBuffer;
	if (nBufferSize > sizeof(pStaticBufer))
	{
		pDynamicBuffer = allocMem(nBufferSize);
		pBuffer = pDynamicBuffer.get();
		if (pBuffer == nullptr)
			throw std::bad_alloc();
	}

	while (nTotalSize < nSize)
	{
		Size nCopySize = static_cast<Size>(std::min<IoSize>(nSize - nTotalSize, nBufferSize));
		Size nReadSize = 0;
		CMD_IOBEGIN;
		CMD_IOSAFE(pStr->read(pBuffer, nCopySize, &nReadSize));
		CMD_IOSAFE(pDst->write(pBuffer, nReadSize));
		nTotalSize += nReadSize;
		if (pProceedSize)
			*pProceedSize = nTotalSize;
		CMD_IOEND;
		if (nReadSize < nCopySize) break; // EOS
	}

	if (nSize == c_nMaxIoSize)
		pDst->flush();
	if (nSize != c_nMaxIoSize && nTotalSize < nSize)
		error::NoData(SL, "Can't write stream").throwException();
	return nTotalSize;
}

//
//
//
void write(ObjPtr<IRawWritableStream> pDst, const std::string_view sString)
{
	TRACE_BEGIN;
	pDst->write(sString.data(), sString.size());
	TRACE_END("Can't write data");
}


//////////////////////////////////////////////////////////////////////////
//
// StringReadableStream 
//
//////////////////////////////////////////////////////////////////////////

//
//
//
void StringReadableStream::finalConstruct(Variant vConfig)
{
	m_sData = vConfig["data"];
}

//
//
//
Size StringReadableStream::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	Size nSizeToRead = std::min(m_sData.size() - m_nOffset, nSize);
	if (nSizeToRead != 0)
		memcpy(pBuffer, &m_sData[m_nOffset], nSizeToRead);
	if (pProceedSize != nullptr)
		*pProceedSize = nSizeToRead;
	m_nOffset += nSizeToRead;
	return nSizeToRead;
}

//
//
//
IoSize StringReadableStream::getPosition()
{
	return m_nOffset;
}

//
//
//
IoSize StringReadableStream::setPosition(IoOffset nOffset, StreamPos eType) 
{
	IoSize nOldPosition = m_nOffset;
	m_nOffset = static_cast<Size>(convertPosition(nOffset, eType, m_nOffset, m_sData.size()));
	return nOldPosition;
}

//
//
//
IoSize StringReadableStream::getSize()
{
	return m_sData.size();
}

//////////////////////////////////////////////////////////////////////////
//
// NullStream 
//
//////////////////////////////////////////////////////////////////////////

//
//
//
void NullStream::finalConstruct(Variant vConfig)
{
	m_nSize = vConfig.get("size", m_nSize);
	m_sQueueName = vConfig.get("queueName", m_sQueueName);
}

//
//
//
Size NullStream::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	Size nRealSize = std::min(nSize, Size(m_nSize - m_nCurrPos));
	memset(pBuffer, 0, nRealSize);
	if (pProceedSize != nullptr)
		*pProceedSize = nRealSize;
	m_nCurrPos += nRealSize;
	return nRealSize;
}

//
//
//
void NullStream::write(const void*, Size nSize)
{
	Size nRealSize = std::min(nSize, Size(m_nSize - m_nCurrPos));
	m_nCurrPos += nRealSize;
	return;
}

//
//
//
IoSize NullStream::getPosition()
{
	return m_nCurrPos;
}

//
//
//
IoSize NullStream::setPosition(IoOffset nOffset, StreamPos eType)
{
	auto nOldPos = m_nCurrPos;
	if (eType == StreamPos::Begin)
		m_nCurrPos = nOffset < 0 ? 0 : ((IoSize)nOffset > m_nSize? m_nSize : nOffset);
	else if (eType == StreamPos::Current)
		m_nCurrPos = (nOffset >= 0) ? 
		(m_nCurrPos + (IoSize)nOffset > m_nSize? m_nSize : m_nCurrPos + nOffset) :
		((IoSize)(-nOffset) > m_nCurrPos ? 0 : m_nCurrPos + nOffset);
	else
		m_nCurrPos = nOffset < 0 ? m_nSize : ((IoSize)nOffset <= m_nSize ? m_nSize - nOffset : 0);
	return nOldPos;
}

//
//
//
IoSize NullStream::getSize()
{
	return m_nSize;
}

//
//
//
void NullStream::setSize(IoSize nSize)
{
	m_nSize = nSize;
}

//
//
//
void NullStream::flush()
{
	return;
}

//
//
//
void NullStream::put(const Variant&)
{
	return;
}

//
//
//
void NullStream::notifyAddQueueData(Variant vTag)
{
	if (m_sQueueName.empty()) return;
	auto pQm = queryInterface<IQueueManager>(queryService("queueManager"));
	auto pProvider = queryInterface<IDataProvider>(pQm->getQueue(m_sQueueName));
	(void)pProvider->get();
}

//
//
//
void NullStream::notifyQueueOverflowWarning(Variant vTag)
{
}

//
//
//
ObjPtr<IReadableStream> createNullStream(Size nSize)
{
	auto pObj = createObject(CLSID_NullStream, Dictionary({ {"size", nSize} }));
	return queryInterface<IReadableStream>(pObj);
}

} // namespace io
} // namespace openEdr 
