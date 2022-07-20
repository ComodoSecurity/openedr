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
#include "FileStream.h"
#include "IOB.h"

using namespace ProtocolFilters;

FileStream::FileStream(void) :
	m_hFile(NULL)
{
}

FileStream::~FileStream(void)
{
	close();
}

#ifdef WIN32
bool FileStream::open(const wchar_t * filename, const wchar_t * mode)
{
	if (m_hFile != NULL)
	{
		close();
	}

	m_hFile = _wfopen(filename, mode);
	if (m_hFile == NULL)
		return false;

	return true;
}
#endif

bool FileStream::open()
{
	if (m_hFile != NULL)
	{
		close();
	}

	m_hFile = tmpfile();
	if (m_hFile == NULL)
		return false;

	return true;
}

void FileStream::close()
{
	if (m_hFile != NULL)
	{
		fclose(m_hFile);
		m_hFile = NULL;
	}
}

void FileStream::free()
{
	delete this;
}

tStreamPos FileStream::seek(tStreamPos offset, int origin)
{
	if (m_hFile == NULL)
		return false;

	if (_fseeki64(m_hFile, offset, origin) == 0)
		return _ftelli64(m_hFile);

	return 0;
}

tStreamSize FileStream::read(void * buffer, tStreamSize size)
{
	if (m_hFile == NULL)
		return 0;

	return (tStreamSize)fread(buffer, 1, size, m_hFile);
}

tStreamSize FileStream::write(const void * buffer, tStreamSize size)
{
	if (m_hFile == NULL)
		return 0;

	return (tStreamSize)fwrite(buffer, 1, size, m_hFile);
}

tStreamPos FileStream::size()
{
	if (m_hFile == NULL)
		return 0;

#ifdef WIN32
	return _filelengthi64(_fileno(m_hFile));
#else
	long pos, len;

	pos = ftell(m_hFile);
	fseek(m_hFile, 0, FILE_END);
	len = ftell(m_hFile);
	fseek(m_hFile, pos, FILE_BEGIN);
	return (tStreamPos)len;
#endif
}

tStreamPos FileStream::copyTo(PFStream * pStream)
{
	IOB tempBuffer;
	tStreamSize r, w;
	tStreamPos copied = 0;

	if (!pStream)
		return 0;

	if (m_hFile == NULL)
		return 0;

	if (!tempBuffer.append(NULL, 16384, false))
		return 0;

	pStream->seek(0, SEEK_SET);
	pStream->setEndOfStream();
		
	tStreamPos oldPos = seek(0, SEEK_CUR);
	seek(0, SEEK_SET);

	do {
		r = read(tempBuffer.buffer(), tempBuffer.size());
		if (r == 0)
			break;

		w = pStream->write(tempBuffer.buffer(), r);
		if (w != r)
			break;

		copied += w;
	} while (r > 0);

	seek(oldPos, SEEK_SET);

	pStream->seek(0, SEEK_SET);

	return copied;
}

tStreamPos FileStream::setEndOfStream()
{
	if (m_hFile == NULL)
		return 0; 

	tStreamPos pos = seek(0, SEEK_CUR);

	_chsize(_fileno(m_hFile), (long)pos);

	return pos;
}

void FileStream::reset()
{
	if (m_hFile == NULL)
		return;

	seek(0, SEEK_SET);

	setEndOfStream();
}
