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
#include "FTPDataFilter.h"
#include "ProxySession.h"
#include "auxutil.h"

using namespace ProtocolFilters;

FTPDataFilter::FTPDataFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags),
	m_object(OT_FTP_DATA_INCOMING, 2)
{
	m_pSession->getSessionInfo(&m_si);
}

FTPDataFilter::~FTPDataFilter(void)
{
}

eFilterResult FTPDataFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	eFilterResult res = FR_DELETE_ME;

	DbgPrint("FTPDataFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (!pHandler)
		return FR_DELETE_ME;

	if (len == 0)
	{
		if (m_object.getStream(0)->size() > 0)
		{
			PFStream * pStream = m_object.getStream(1);
			if (pStream->size() == 0)
			{
				std::string info = m_pSession->getAssociatedSessionInfo();
				if (!info.empty())
					pStream->write(info.c_str(), (tStreamSize)info.length());
			}

			// Filter
			try {
				pHandler->dataAvailable(m_pSession->getId(), &m_object);
			} catch (...)
			{
			}
	
			m_object.clear();
		}

		PFObjectImpl obj((pd == PD_SEND)? OT_TCP_DISCONNECT_LOCAL : OT_TCP_DISCONNECT_REMOTE, 0);
		try {
			pHandler->dataAvailable(m_pSession->getId(), &obj);
		} catch (...)
		{
		}

		return FR_DATA_EATEN;
	} else
	{
		if (pd == PD_SEND)
		{
			if (m_flags & FF_READ_ONLY_OUT)
			{
				m_object.setReadOnly(true);
				m_object.setType(OT_FTP_DATA_OUTGOING);
				res = FR_PASSTHROUGH;
			} else
			{
				m_object.setType(OT_FTP_DATA_PART_OUTGOING);
				res = FR_DATA_EATEN;
			}
		} else
		{
			if (m_flags & FF_READ_ONLY_IN)
			{
				m_object.setReadOnly(true);
				m_object.setType(OT_FTP_DATA_INCOMING);
				res = FR_PASSTHROUGH;
			} else
			{
				m_object.setType(OT_FTP_DATA_PART_INCOMING);
				res = FR_DATA_EATEN;
			}
		}

		if (res == FR_DATA_EATEN)
		{
			PFStream * pStream;

			pStream = m_object.getStream(1);
			if (pStream->size() == 0)
			{
				std::string info = m_pSession->getAssociatedSessionInfo();
				if (!info.empty())
					pStream->write(info.c_str(), (tStreamSize)info.length());
			}

			pStream = m_object.getStream(0);
			pStream->reset();
			pStream->write(buf, len);

			// Filter
			try {
				pHandler->dataAvailable(m_pSession->getId(), &m_object);
			} catch (...)
			{
			}

			pStream = m_object.getStream(0);
			pStream->reset();
		} else
		{
			PFStream * pStream = m_object.getStream(0);
			pStream->write(buf, len);

			m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);
			return FR_DATA_EATEN;
		}
	}

	return res;
}
		
eFilterResult FTPDataFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType FTPDataFilter::getType()
{
	return FT_FTP_DATA;
}

PF_FilterCategory FTPDataFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool FTPDataFilter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	switch (object.getType())
	{
	case OT_FTP_DATA_OUTGOING:
	case OT_FTP_DATA_PART_OUTGOING:
		pd = PD_SEND;
		break;
	case OT_FTP_DATA_INCOMING:
	case OT_FTP_DATA_PART_INCOMING:
		pd = PD_RECEIVE;
		break;
	default:
		return false;
	}

	m_pSession->tcpPostStream(this, pd, object.getStream(0));

	return true;
}

