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
#include "icqdefs.h"

namespace ProtocolFilters
{
	class ICQFilter : public IFilterBase
	{
	public:
		ICQFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~ICQFilter(void);

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
		eFilterResult updateCmdBuffer(const char * buf, int len);
		eFilterResult updateResBuffer(const char * buf, int len);
		eFilterResult filterSNAC(OSCAR_FLAP * pFlap, ePacketDirection pd, PFEvents * pHandler);
		eFilterResult filterOutgoingMessage(char * p, char * pFlap, char * pEnd, PFEvents * pHandler);
		eFilterResult filterIncomingMessage(char * p, char * pFlap, char * pEnd, PFEvents * pHandler);
		eFilterResult filterIncomingOfflineMessage(char * p, char * pFlap, char * pEnd, PFEvents * pHandler);

	private:
		ProxySession * m_pSession;
		tPF_FilterFlags m_flags;

		enum tICQState
		{
			STATE_NONE,
			STATE_SESSION
		};

		tICQState	m_state;
		IOB			m_cmdBuffer;
		IOB			m_resBuffer;
		std::string	m_user;
		GEN_SESSION_INFO m_si;
	};
}