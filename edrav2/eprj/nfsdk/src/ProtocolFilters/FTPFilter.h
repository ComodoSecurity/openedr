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
	class FTPFilter : public IFilterBase
	{
	public:
		FTPFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~FTPFilter(void);

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
		eFilterResult updateCmdBuffer(const char * buf, int len);
		eFilterResult updateResBuffer(const char * buf, int len);
		void updateAssociatedSessionData(PFObjectImpl & object);

	private:
		ProxySession * m_pSession;
		tPF_FilterFlags m_flags;

		enum tFTPState
		{
			STATE_NONE,
			STATE_USER,
			STATE_USER_RESP,
			STATE_AUTH_RESP,
			STATE_CMD,
			STATE_CMD_RESP
		};

		tFTPState	m_state;
		IOB			m_cmdBuffer;
		IOB			m_resBuffer;
		PFObjectImpl 	m_cmdObject;
		PFObjectImpl 	m_resObject;
		bool m_mustUpdateAssociatedSessionData;
		
		enum INFO_INDEX
		{
			II_COMMAND = 0,
			II_TYPE = 1,
			II_MODE = 2,
			II_STRU = 3,
			II_REST = 4,
			II_SESSIONID = 5,
			II_MAX = 6
		};
		
		std::string		m_info[II_MAX];

		GEN_SESSION_INFO m_si;
	};
}