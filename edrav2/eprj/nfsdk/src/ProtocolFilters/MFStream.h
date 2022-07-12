//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#pragma once

#include "PFStream.h"

namespace ProtocolFilters
{
	class MFStream : public PFStream
	{
	public:
		MFStream(void);
		~MFStream(void);

		bool open(tStreamSize maxMemSize = 8 * 1024 * 1024);
		void close();

		virtual void free();

		virtual tStreamPos seek(tStreamPos offset, int origin);
		virtual tStreamSize read(void * buffer, tStreamSize size);
		virtual tStreamSize write(const void * buffer, tStreamSize size);
		virtual tStreamPos size();
		virtual tStreamPos copyTo(PFStream * pStream);
		virtual tStreamPos setEndOfStream();
		virtual void reset();

	private:
		PFStream *	m_pStream;
		tStreamSize m_maxMemSize;
		bool		m_fileStream;
	};

	PFStream * createByteStream();
}