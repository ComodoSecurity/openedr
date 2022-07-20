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
#include "UnzipStream.h"

using namespace ProtocolFilters;

UnzipStream::UnzipStream(void)
{
	m_begin = true;
	memset(&m_zStream, 0, sizeof(m_zStream));
}

UnzipStream::~UnzipStream(void)
{
	reset();
}

bool UnzipStream::uncompress(const char * buf, int len, IOB & outBuf)
{
    int		err = Z_OK;

	outBuf.reset();

	if (!outBuf.append(NULL, len * 3, false))
		return false;

    m_zStream.next_in = (Bytef*)buf;
    m_zStream.avail_in = (uInt)len;
    
	m_zStream.next_out = (Bytef*)outBuf.buffer();
	m_zStream.avail_out = (uInt)outBuf.size();

	if (m_begin)
	{
		m_begin = false;

		m_zStream.zalloc = (alloc_func)0;
		m_zStream.zfree = (free_func)0;

		for (int skipHeader = 0; skipHeader <= 1; skipHeader++)
		{
			m_zStream.next_in = (Bytef*)buf;
			m_zStream.next_out = (Bytef*)outBuf.buffer();
			m_zStream.avail_in = 0;
			m_zStream.avail_out = 0;
			m_zStream.opaque = (voidpf)0;

			if (skipHeader == 0)
			{
				err = inflateInit2(&m_zStream, 47);//15 + 16);//-MAX_WBITS);
				if (err != Z_OK) 
					return false;
			} else
			{
				err = inflateInit(&m_zStream);
				if (err != Z_OK) 
					return false;
			}
			
			m_zStream.avail_in = (uInt)len;
			m_zStream.avail_out = (uInt)outBuf.size();

			for (;;)
			{
				err = inflate(&m_zStream, Z_SYNC_FLUSH);

				if ((err == Z_OK || err == Z_BUF_ERROR) && m_zStream.avail_in > 0)
				{
					if (!outBuf.append(NULL, len, false))
						return false;

					m_zStream.next_out = (Bytef*)(outBuf.buffer() + outBuf.size() - len);
					m_zStream.avail_out = len;
					continue;
				}

				if (err == Z_OK || err == Z_STREAM_END)
				{
					outBuf.resize(m_zStream.total_out);			
					return true;
				} else
				{
					inflateEnd(&m_zStream);
					break;
				}				
			}
		}

		if (err != Z_OK)
			return false;
	} else
	{
		int oldTotalOut = m_zStream.total_out;

		for (;;)
		{
			err = inflate(&m_zStream, Z_SYNC_FLUSH);
			
			if ((err == Z_OK || err == Z_BUF_ERROR) && m_zStream.avail_in > 0)
			{
				len *= 2;

				if (!outBuf.append(NULL, len, false))
					return false;

				m_zStream.next_out = (Bytef*)(outBuf.buffer() + outBuf.size() - len);
				m_zStream.avail_out = len;
				continue;
			}

			if (err == Z_OK || err == Z_STREAM_END)
			{
				outBuf.resize(m_zStream.total_out - oldTotalOut);			
				return true;
			} else
			{
				return false;
			}		
		}
	}

	return true;
}

void UnzipStream::reset()
{
	inflateEnd(&m_zStream);
	m_begin = true;
	memset(&m_zStream, 0, sizeof(m_zStream));
}
