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

#include "IFilterBase.h"
#include "IOB.h"
#include <queue>

namespace ProtocolFilters
{
	class FTPDataFilter : public IFilterBase
	{
	public:
		FTPDataFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~FTPDataFilter(void);

		eFilterResult tcp_packet(eDataDirection dd,
								ePacketDirection pd,
								const char * buf, int len);
		
		eFilterResult udp_packet(eDataDirection dd,
								ePacketDirection pd,
								const unsigned char * remoteAddress, 
								const char * buf, int len);

		PF_FilterType getType();

		PF_FilterCategory getCategory();
	
		bool postObject(PFObjectImpl & object);

		PF_FilterFlags getFlags()
		{
			return (PF_FilterFlags)m_flags;
		}

	protected:

	private:
		ProxySession * m_pSession;
		tPF_FilterFlags m_flags;

		PFObjectImpl 	m_object;
		GEN_SESSION_INFO m_si;
	};
}