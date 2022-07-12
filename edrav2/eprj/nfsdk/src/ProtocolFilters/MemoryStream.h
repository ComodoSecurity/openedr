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
#include "IOB.h"

namespace ProtocolFilters
{
	class MemoryStream : public PFStream
	{
	public:
		MemoryStream(void);
		~MemoryStream(void);

		bool open();
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
		tStreamPos m_offset;
		IOB m_iob;
	};
}