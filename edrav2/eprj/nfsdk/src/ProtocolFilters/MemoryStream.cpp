//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdafx.h"
#include "MemoryStream.h"

using namespace ProtocolFilters;

MemoryStream::MemoryStream(void) : m_offset(0)
{
}

MemoryStream::~MemoryStream(void)
{
	close();
}

bool MemoryStream::open()
{
	return true;
}

void MemoryStream::close()
{
	m_offset = 0;
	m_iob.reset();
}

void MemoryStream::free()
{
	delete this;
}

tStreamPos MemoryStream::seek(tStreamPos offset, int origin)
{
	switch (origin)
	{
	case FILE_BEGIN:
		return m_offset = offset;
	case FILE_CURRENT:
		return m_offset = m_offset + offset;
	case FILE_END:
		return m_offset = m_iob.size();
	}

	return m_offset;
}

tStreamSize MemoryStream::read(void * buffer, tStreamSize size)
{
	if (size == 0)
		return 0;

	tStreamSize len;

	if (size < (m_iob.size() - m_offset))
		len = size;
	else
		len = (tStreamSize)(m_iob.size() - m_offset);

	if (len > 0)
	{
		memcpy(buffer, m_iob.buffer() + m_offset, len);
		m_offset += len;
	}

	return len;
}

tStreamSize MemoryStream::write(const void * buffer, tStreamSize size)
{
	if (size == 0)
		return 0;

	tStreamPos offset = m_offset + size;

	if (offset > m_iob.size())
	{
		if (!m_iob.append(NULL, (unsigned long)offset - m_iob.size(), false))
			return 0;
	}

	memcpy(m_iob.buffer() + m_offset, buffer, size);
		
	m_offset = offset;

	return size;
}

tStreamPos MemoryStream::size()
{
	return m_iob.size();
}

tStreamPos MemoryStream::copyTo(PFStream * pStream)
{
	if (!pStream)
		return 0;

	pStream->seek(0, FILE_BEGIN);
	pStream->setEndOfStream();

	tStreamPos pos = pStream->write(m_iob.buffer(), m_iob.size());
	pStream->seek(0, FILE_BEGIN);
	return pos;
}

tStreamPos MemoryStream::setEndOfStream()
{
	m_iob.resize((unsigned long)m_offset);

	return m_offset;
}

void MemoryStream::reset()
{
	m_iob.reset();
	m_offset = 0;
}
