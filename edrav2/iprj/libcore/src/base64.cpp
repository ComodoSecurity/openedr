//
// edrav2.libcore
// Base64 stream class realization
// 
// Author: Denis Kroshin (05.01.2019)
// Reviewer: Denis Bogdanov (09.01.2019)
//
//
#include "pch.h"
#include "base64.h"

namespace openEdr {
namespace crypt {

////////////////////////////////////////////////////////////////////////////////
//
// Base64 Encoder
//
////////////////////////////////////////////////////////////////////////////////

static const char c_pCoderTableB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//
//
//
void Base64Encoder::finalConstruct(Variant vConfig)
{
	m_pDestStream = vConfig["stream"];
	m_nCacheSize = vConfig.get("cacheSize", c_nDefaultCacheSize);
	m_nCacheSize = growToLog2<Size>(m_nCacheSize, c_nOutBlockSize);
	m_pCacheBuffer = allocMem(m_nCacheSize);
	if (m_pCacheBuffer.get() == nullptr)
		error::BadAlloc(SL).throwException();
}

//
//
//
void Base64Encoder::fillCache(bool fFlush)
{
	if (m_nInLength > 0)
	{
		// encode 3 8-bit binary bytes as 4 '6-bit' characters
		Byte pOutBlock[c_nOutBlockSize];
		pOutBlock[0] = c_pCoderTableB64[m_pInBlock[0] >> 2];
		pOutBlock[1] = c_pCoderTableB64[((m_pInBlock[0] & 0x03) << 4) | ((m_pInBlock[1] & 0xF0) >> 4)];
		pOutBlock[2] = (m_nInLength > 1 ?
			c_pCoderTableB64[((m_pInBlock[1] & 0x0F) << 2) | ((m_pInBlock[2] & 0xC0) >> 6)] : '=');
		pOutBlock[3] = (Byte)(m_nInLength > 2 ? c_pCoderTableB64[m_pInBlock[2] & 0x3F] : '=');

		memcpy(m_pCacheBuffer.get() + m_nCachePos, pOutBlock, c_nOutBlockSize);
		m_nCachePos += c_nOutBlockSize;
	}

	if (m_nCachePos == m_nCacheSize || fFlush)
	{
		m_pDestStream->write(m_pCacheBuffer.get(), m_nCachePos);
		m_nCachePos = 0;
	}

	// Zero after successful write
	m_pInBlock[0] = m_pInBlock[1] = m_pInBlock[2] = 0;
	m_nInLength = 0;
}


//
//
//
void Base64Encoder::write(const void* pBuffer, Size nSize)
{
	// Validate incoming parameter
	if (pBuffer == NULL)
		error::InvalidArgument(SL).throwException();

	Size nTotalSize = 0;
	while (nTotalSize < nSize)
	{
		Size nCopySize = std::min(sizeof(m_pInBlock) - m_nInLength, nSize - nTotalSize);
		memcpy(m_pInBlock + m_nInLength, (Byte*)pBuffer + nTotalSize, nCopySize);
		m_nInLength += nCopySize;
		nTotalSize += nCopySize;

		if (m_nInLength == sizeof(m_pInBlock))
			fillCache();
	}
}

//
//
//
void Base64Encoder::flush()
{
	fillCache(true);
}

////////////////////////////////////////////////////////////////////////////////
//
// Base64 Decoder
//
////////////////////////////////////////////////////////////////////////////////


static const Byte c_pDecoderTableB64[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

//
//
//
void Base64Decoder::finalConstruct(Variant vConfig)
{
	m_pSrcStream = vConfig["stream"];
	m_nOutIndex = 3;
}

//
//
//
void Base64Decoder::fillCache()
{
	if (m_nOutIndex < m_nOutSize || m_fEof)
		return;

	CMD_IOBEGIN;

	Size nInIndex = 0;
	Byte m_InBlock[4];
	m_nOutSize = 3;
	while (nInIndex < _countof(m_InBlock))
	{
		Byte bSymbol = 0;
		while (bSymbol == 0 && !m_fEof)
		{
			Size nBytesRead = 0;
			CMD_IOSAFE(m_pSrcStream->read(&bSymbol, sizeof(bSymbol), &nBytesRead));
			if (nBytesRead != sizeof(bSymbol))
			{
				m_fEof = true;
				if (m_nOutSize == 3)
					m_nOutSize = nInIndex > 0 ? nInIndex - 1 : 0;
				break;
			}

			if (bSymbol == '=' && m_nOutSize == 3)
				m_nOutSize = (nInIndex == 0) ? 0 : nInIndex - 1;
			bSymbol = (bSymbol < 43 || bSymbol > 122) ? 0 : c_pDecoderTableB64[bSymbol - 43];
			if (bSymbol != 0)
				bSymbol = (bSymbol == '$') ? 0 : bSymbol - 61;
		}

		m_InBlock[nInIndex++] = (bSymbol > 0) ? bSymbol - 1 : 0;
	}

	// decode 4 '6-bit' characters into 3 8-bit binary bytes
	m_OutBlock[0] = m_InBlock[0] << 2 | m_InBlock[1] >> 4;
	m_OutBlock[1] = m_InBlock[1] << 4 | m_InBlock[2] >> 2;
	m_OutBlock[2] = ((m_InBlock[2] << 6) & 0xC0) | m_InBlock[3];
	m_nOutIndex = 0;

	CMD_IOEND;
}

//
//
//
Size Base64Decoder::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL).throwException();

	Size nTotalSize = 0;
	while (nTotalSize < nSize && !m_fEof)
	{
		CMD_IOBEGIN;
		CMD_IOSAFE(fillCache());
		Size nCopySize = std::min(m_nOutSize - m_nOutIndex, nSize - nTotalSize);
		memcpy((Byte*)pBuffer + nTotalSize, m_OutBlock + m_nOutIndex, nCopySize);
		m_nOutIndex += nCopySize;
		nTotalSize += nCopySize;
		if (pProceedSize != nullptr)
			*pProceedSize = nTotalSize;
		CMD_IOEND;
	}
	return nTotalSize;
}

} // namespace crypt
} // namespace openEdr 