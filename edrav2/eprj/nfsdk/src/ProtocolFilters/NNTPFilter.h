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

namespace ProtocolFilters
{
	class NNTPFilter : public IFilterBase
	{
	public:
		NNTPFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~NNTPFilter(void);

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
		bool checkPacket(ePacketDirection pd, const char * buf, int len);
		int getEndOfMessage(const char * buf, int len);
		eFilterResult updateCmdBuffer(const char * buf, int len);

	private:
		ProxySession * m_pSession;
		tPF_FilterFlags m_flags;

		enum tNNTPState
		{
			STATE_NONE,
			STATE_MODE,
			STATE_CMD,
			STATE_ARTICLE_RESP,
			STATE_ARTICLE_DATA,
			STATE_POST,
			STATE_POST_RESP,
			STATE_POST_RESP_1
		};

		tNNTPState	m_state;
		IOB			m_buffer;
		nf_time_t		m_time;
		PFObjectImpl 	m_object;
		GEN_SESSION_INFO m_si;
	};
}