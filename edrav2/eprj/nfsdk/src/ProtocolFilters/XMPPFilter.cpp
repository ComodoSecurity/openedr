//
// 	NetFilterSDK 
// 	Copyright (C) 2011 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdafx.h"
#include "XMPPFilter.h"
#include "ProxySession.h"

using namespace ProtocolFilters;

XMPPParser::XMPPParser(PF_ObjectType ot, bool readOnly) 
{
	m_pObject = new PFObjectImpl(ot, 1);
	if (m_pObject)
		m_pObject->setReadOnly(readOnly);
	m_xmlDepth = 0;
	m_state = S_NONE;
	m_attrValueQuote = '"';
	m_unzip = false;
}

XMPPParser::~XMPPParser()
{
	if (m_pObject)
		delete m_pObject;
}

int XMPPParser::inputPacket(const char * buf, int len)
{
	static const char szStreamTag[] = "stream:stream";
	static const int streamTagLen = (int)sizeof(szStreamTag)-1;
	static const char szFlashStreamTag[] = "flash:stream";
	static const int flashStreamTagLen = (int)sizeof(szFlashStreamTag)-1;
	static const char szCompressedTag[] = "compressed";
//	static const int compressedTagLen = sizeof(szCompressedTag)-1;
	int pos = 0;
	char c;

	for (int i=0; i<len; i++)
	{
		c = buf[i];

		switch (m_state)
		{
		case S_NONE:
			switch (c)
			{
			case '<':
				m_state = S_TAG_START;
				m_tagName = "";
				break;
			default:
				pos = i+1;
			}
			break;

		case S_TAG_START:
			switch (c)
			{
			case '/':
				m_state = S_CLOSE_TAG_NAME;
				break;
			case '?':
				m_state = S_META_TAG;
				break;
			default:
				m_tagName += c;
				m_state = S_TAG_NAME;
			}
			break;

		case S_META_TAG:
			switch (c)
			{
			case '?':
				m_state = S_META_TAG_CLOSE;
				break;
			}
			break;

		case S_META_TAG_CLOSE:
			switch (c)
			{
			case '>':
				m_state = S_NONE;
				pos = i+1;
				break;
			}
			break;

		case S_TAG_NAME:
			switch (c)
			{
			case '/':
				m_state = S_TAG_CLOSE;
				break;
			case ' ':
				m_state = S_TAG_ATTRIBUTES;
				break;
			case '>':
				m_state = S_NONE;
				if ((m_tagName.length() != streamTagLen ||
					stricmp(m_tagName.c_str(), szStreamTag) != 0) &&
					(m_tagName.length() != flashStreamTagLen ||
						stricmp(m_tagName.c_str(), szFlashStreamTag) != 0))
				{
					m_xmlDepth++;
				}
				
				pos = i+1;
				break;
			default:
				m_tagName += c;
			}
			break;

		case S_CLOSE_TAG_NAME:
			switch (c)
			{
			case '>':
				m_state = S_NONE;
				m_xmlDepth--;
				pos = i+1;
				break;
			}
			break;

		case S_TAG_ATTRIBUTES:
			switch (c)
			{
			case '>':
				m_state = S_NONE;
				if ((m_tagName.length() != streamTagLen ||
					stricmp(m_tagName.c_str(), szStreamTag) != 0) &&
					(m_tagName.length() != flashStreamTagLen ||
						stricmp(m_tagName.c_str(), szFlashStreamTag) != 0))
				{
					m_xmlDepth++;
				}
				pos = i+1;
				break;
			case '/':
				m_state = S_TAG_CLOSE;
				break;
			case '"':
			case '\'':
				m_attrValueQuote = c;
				m_state = S_TAG_ATTRIBUTES_VALUE;
				break;
			}
			break;

		case S_TAG_ATTRIBUTES_VALUE:
			if (c == m_attrValueQuote)
			{
				m_state = S_TAG_ATTRIBUTES;
			}
			break;

		case S_TAG_CLOSE:
			switch (c)
			{
			case '>':
				m_state = S_NONE;
				pos = i+1;

				if (!m_unzip)
				{
					if (stricmp(m_tagName.c_str(), szCompressedTag) == 0)
					{
						m_unzip = true;
					}
				}
				break;
			}
			break;
		}
	}

	if (m_xmlDepth > 0)
	{
		pos = len;
	}

	if (pos > 0)
	{
		m_pObject->getStream(0)->write(buf, pos);
	}
	
	return pos;
}

bool XMPPParser::isFinished()
{
	return (m_state == S_NONE) && (m_xmlDepth == 0);
}

void XMPPParser::reset()
{
	m_pObject->getStream(0)->reset();
}

PFObject * XMPPParser::getObject()
{
	m_pObject->getStream(0)->seek(0, FILE_BEGIN);
	return m_pObject;
}



XMPPFilter::XMPPFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_requestParser(OT_XMPP_REQUEST, (flags & FF_READ_ONLY_OUT)!=0),
	m_responseParser(OT_XMPP_RESPONSE, (flags & FF_READ_ONLY_IN)!=0),
	m_bFirstPacket(true),
	m_pSession(pSession),
	m_flags(flags)
{
	setActive(false);
	m_unzip = false;
}

XMPPFilter::~XMPPFilter(void)
{
}

bool XMPPFilter::isXMPPRequest(const char * buf, int len)
{
	const char szStream[] = "<stream:stream";
	const char szFlashStream[] = "<flash:stream";

	if (len < (int)sizeof(szStream)-1)
		return false;

	std::string s(buf, len);

	if (s.find(szStream) == std::string::npos &&
		s.find(szFlashStream) == std::string::npos)
		return false;

	return true;
}

eFilterResult XMPPFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	DbgPrint("XMPPFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);

	if (((m_flags & FF_DONT_FILTER_IN) && (pd == PD_RECEIVE)) ||
		((m_flags & FF_DONT_FILTER_OUT) && (pd == PD_SEND)))
	{
		return FR_PASSTHROUGH;
	}

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (!pHandler)
		return FR_PASSTHROUGH;

	if (len > 0)
	{
		if (m_bFirstPacket)
		{
			GEN_SESSION_INFO si;

			m_pSession->getSessionInfo(&si);
		
			if (((si.tcpConnInfo.direction == NFAPI_NS NF_D_OUT) && (pd != PD_SEND)) ||
				((si.tcpConnInfo.direction == NFAPI_NS NF_D_IN) && (pd != PD_RECEIVE)))
				return FR_DELETE_ME;

			if (!isXMPPRequest(buf, len))
			{
				const char szXml[] = "<?xml";
				std::string s(buf, len);
				if (s.find(szXml) == std::string::npos)
					return FR_DELETE_ME;
				else
					return FR_PASSTHROUGH;
			}

			setActive(true);

			if (m_flags & FF_SSL_TLS) 
			{ 
				m_pSession->addFilter(FT_SSL, FF_SSL_TLS_AUTO, OT_FIRST);
			}

			m_bFirstPacket = false;
		}

		if (((pd == PD_RECEIVE) && (m_flags & FF_READ_ONLY_IN)) ||
			((pd == PD_SEND) && (m_flags & FF_READ_ONLY_OUT)))
		{
			m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);
		} 

		int offset = 0;
		int taken = 0;
		XMPPParser & parser = (pd == PD_SEND)? m_requestParser : m_responseParser;

		IOB outBuf;

		if (m_unzip)
		{
			UnzipStream & uzStream = (pd == PD_SEND)? m_uzStreamOut : m_uzStreamIn;
			if (uzStream.uncompress(buf, len, outBuf))
			{
				buf = outBuf.buffer();
				len = outBuf.size();
			} 
		}

		for (;;)
		{
			taken = parser.inputPacket(buf+offset, len-offset);
			if (taken == 0)
				break;

			if (parser.isFinished())
			{
				PFObject * pObj = parser.getObject();

				try {
					pHandler->dataAvailable(m_pSession->getId(), pObj);
				} catch (...)
				{
				}
				
				parser.reset();

				if ((offset + taken) < len)
				{
					offset += taken;
					continue;
				}
			}

			break;
		}
	
		if (m_responseParser.isCompressed())
		{
			if ((m_flags & (FF_READ_ONLY_IN | FF_READ_ONLY_OUT)) == (FF_READ_ONLY_IN | FF_READ_ONLY_OUT))
			{
				m_unzip = true;
			} else
			{
				setDeleted();
			}
		}
	}

	if (len == 0)
	{
		if (pHandler)
		{
			PFObjectImpl obj((pd == PD_SEND)? OT_TCP_DISCONNECT_LOCAL : OT_TCP_DISCONNECT_REMOTE, 0);
			try {
				pHandler->dataAvailable(m_pSession->getId(), &obj);
			} catch (...)
			{
			}
		}
	} 

	return FR_DATA_EATEN;
}
		
eFilterResult XMPPFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType XMPPFilter::getType()
{
	return FT_XMPP;
}

PF_FilterCategory XMPPFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool XMPPFilter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	switch (object.getType())
	{
	case OT_XMPP_REQUEST:
		pd = PD_SEND;
		break;

	case OT_XMPP_RESPONSE:
		pd = PD_RECEIVE;
		break;

	default:
		return false;
	}

	return m_pSession->tcpPostObjectStreams(this, pd, object);
}

