//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdafx.h"
#include "RawFilter.h"
#include "ProxySession.h"

using namespace ProtocolFilters;

RawFilter::RawFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags)
{
}

RawFilter::~RawFilter(void)
{
}


eFilterResult RawFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	DbgPrint("RawFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
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
		PFObjectImpl obj((pd == PD_SEND)? OT_RAW_OUTGOING : OT_RAW_INCOMING, 1);
		bool readOnly = false;
		PFStream * pStream = obj.getStream(0);

		if (pStream)
		{
			pStream->write(buf, len);
			pStream->seek(0, FILE_BEGIN);
		}

		if (((pd == PD_RECEIVE) && (m_flags & FF_READ_ONLY_IN)) ||
			((pd == PD_SEND) && (m_flags & FF_READ_ONLY_OUT)))
		{
			obj.setReadOnly(true);
			readOnly = true;
		} 

		try {
			pHandler->dataAvailable(m_pSession->getId(), &obj);
		} catch (...)
		{
		}

		if (readOnly)
		{
			return FR_PASSTHROUGH;
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
		
eFilterResult RawFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType RawFilter::getType()
{
	return FT_RAW;
}

PF_FilterCategory RawFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool RawFilter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	switch (object.getType())
	{
	case OT_RAW_OUTGOING:
		pd = PD_SEND;
		break;

	case OT_RAW_INCOMING:
		pd = PD_RECEIVE;
		break;

	default:
		return false;
	}

	return m_pSession->tcpPostObjectStreams(this, pd, object);
}

