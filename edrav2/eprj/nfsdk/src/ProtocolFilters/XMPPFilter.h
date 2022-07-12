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
#include "MFStream.h"
#include "UnzipStream.h"

namespace ProtocolFilters
{
	class XMPPParser
	{
	public:
		XMPPParser(PF_ObjectType ot, bool readOnly);
		~XMPPParser();

		int inputPacket(const char * buf, int len);
	
		void reset();

		PFObject * getObject();

		bool isFinished();

		bool isCompressed()
		{
			return m_unzip;
		}

		void setCompressed(bool v)
		{
			m_unzip = v;
		}

	private:
		PFObject * m_pObject;
		int m_xmlDepth;

		enum State
		{
			S_NONE,
			S_TAG_START,
			S_TAG_NAME,
			S_TAG_ATTRIBUTES,
			S_TAG_ATTRIBUTES_VALUE,
			S_CLOSE_TAG_NAME,
			S_TAG_CLOSE,
			S_META_TAG,
			S_META_TAG_CLOSE,
		};

		State m_state;
		std::string m_tagName;
		char m_attrValueQuote;
		bool m_unzip;
	};

	class XMPPFilter : public IFilterBase
	{
	public:
		XMPPFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~XMPPFilter(void);

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
		bool isXMPPRequest(const char * buf, int len);

	private:
		XMPPParser m_requestParser;
		XMPPParser m_responseParser;
		bool m_bFirstPacket;
		ProxySession * m_pSession;
		tPF_FilterFlags m_flags;
		UnzipStream	m_uzStreamIn;
		UnzipStream	m_uzStreamOut;
		bool m_unzip;
	};
}