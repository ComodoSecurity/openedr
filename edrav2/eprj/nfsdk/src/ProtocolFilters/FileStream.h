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
	class FileStream : public PFStream
	{
	public:
		FileStream(void);
		~FileStream(void);
		bool open();
#ifdef WIN32
		bool open(const wchar_t * filename, const wchar_t * mode = L"w+bTD");
#endif
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
		FILE * m_hFile;
	};
}