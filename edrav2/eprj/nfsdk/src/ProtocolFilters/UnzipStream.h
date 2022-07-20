//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _UNZIPSTREAM
#define _UNZIPSTREAM

#include "IOB.h"
// EDR_FIX: Fix path to zlib.h
#include <zlib.h>

namespace ProtocolFilters
{
	class UnzipStream
	{
	public:
		UnzipStream(void);
		~UnzipStream(void);

		bool uncompress(const char * buf, int len, IOB & outBuf);
		void reset();

	private:
		z_stream	m_zStream;
		bool		m_begin;
	};
}

#endif