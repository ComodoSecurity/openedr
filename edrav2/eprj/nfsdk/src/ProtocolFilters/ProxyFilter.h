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
#include "HTTPParser.h"

namespace ProtocolFilters
{
	class ProxyFilter : public IFilterBase
	{
	public:
		ProxyFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~ProxyFilter(void);

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
		bool isHTTPConnectRequest(const char * buf, int len, std::string & connectTo);
		eFilterResult updateHttpsCmdBuffer(const char * buf, int len);
		eFilterResult updateSocksCmdBuffer(const char * buf, int len);

	private:
		ProxySession * m_pSession;
		tPF_FilterFlags m_flags;
		GEN_SESSION_INFO m_si;

		enum eState
		{
			STATE_NONE,
			STATE_RESP
		};

		enum eSocks5State
		{
			STATE_SOCKS5_AUTH,
			STATE_SOCKS5_AUTH_NEG,
			STATE_SOCKS5_REQUEST
		};

		enum eProxyType
		{
			PT_UNKNOWN,
			PT_HTTPS,
			PT_SOCKS4,
			PT_SOCKS5
		};

		eProxyType	m_proxyType;
		eSocks5State m_socks5State;
		char	m_socks5AuthMethod;

		bool	m_readOnly;
		eState	m_state;		
		IOB		m_buffer;
		HTTPParser m_httpParser;
	};
}