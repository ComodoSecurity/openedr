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
#include "MFStream.h"
#include "MemoryStream.h"
#include "FileStream.h"
#include "Settings.h"

using namespace ProtocolFilters;

#define MAX_MEM_STREAM_SIZE 100 * 1024 * 1024

PFStream * ProtocolFilters::createByteStream()
{
	MFStream * pStream = new MFStream();
	if (!pStream)
		return NULL;

	if (!pStream->open())
	{
		delete pStream;
		return NULL;
	}

	return pStream;
}

MFStream::MFStream(void) : m_pStream(NULL), m_fileStream(false)
{
}

MFStream::~MFStream(void)
{
	close();
}

bool MFStream::open(tStreamSize maxMemSize)
{
	close();

	if (maxMemSize > MAX_MEM_STREAM_SIZE)
	{
		maxMemSize = MAX_MEM_STREAM_SIZE;
	}

	MemoryStream * pStream = new MemoryStream();
	if (!pStream)
		return false;

	if (!pStream->open())
	{
		delete pStream;
		return false;
	}

	m_pStream = pStream;
	m_maxMemSize = maxMemSize;

	return true;
}

void MFStream::close()
{
	if (m_pStream)
	{
		m_pStream->free();
		m_pStream = NULL;
	}
}

void MFStream::free()
{
	delete this;
}

tStreamPos MFStream::seek(tStreamPos offset, int origin)
{
	if (!m_pStream)
		return 0;

	return m_pStream->seek(offset, origin);
}

tStreamSize MFStream::read(void * buffer, tStreamSize size)
{
	if (!m_pStream)
		return 0;

	return m_pStream->read(buffer, size);
}

tStreamSize MFStream::write(const void * buffer, tStreamSize size)
{
	if (!m_pStream)
		return 0;

	tStreamSize written = m_pStream->write(buffer, size);
	if (written == 0)
		return 0;

	if (!m_fileStream && (m_pStream->size() > m_maxMemSize))
	{
#ifdef WIN32
		wchar_t tempFileName[_MAX_PATH];

		std::wstring wBaseFolder = Settings::get().getBaseFolder();

		if (!GetTempFileNameW(wBaseFolder.c_str(), L"mfs", 0, tempFileName))
		{
			return written;
		}

		FileStream * pStream = new FileStream();
		if (!pStream)
			return written;

		if (!pStream->open(tempFileName))
		{
			delete pStream;
			return written;
		}
#else
		FileStream * pStream = new FileStream();
		if (!pStream)
			return written;

		if (!pStream->open())
		{
			delete pStream;
			return written;
		}

#endif

		tStreamPos offset = m_pStream->seek(0, FILE_CURRENT);
		tStreamPos copied = m_pStream->copyTo(pStream);
		if (copied != m_pStream->size())
		{
			delete pStream;
			return written;
		}
		
		m_pStream->free();

		m_pStream = pStream;
		
		m_pStream->seek(offset, FILE_BEGIN);

		m_fileStream = true;
	}

	return written;
}

tStreamPos MFStream::size()
{
	if (!m_pStream)
		return 0;

	return m_pStream->size();
}

tStreamPos MFStream::copyTo(PFStream * pStream)
{
	if (!m_pStream)
		return 0;

	return m_pStream->copyTo(pStream);
}

tStreamPos MFStream::setEndOfStream()
{
	if (!m_pStream)
		return 0;

	return m_pStream->setEndOfStream();
}

void MFStream::reset()
{
	if (!m_pStream)
		return;

	if (!m_fileStream)
	{
		m_pStream->reset();
		return;
	}

	m_pStream->free();
	m_pStream = NULL;

	MemoryStream * pStream = new MemoryStream();
	if (!pStream)
		return;

	if (!pStream->open())
	{
		delete pStream;
		return;
	}

	m_pStream = pStream;
	m_fileStream = false;
}
