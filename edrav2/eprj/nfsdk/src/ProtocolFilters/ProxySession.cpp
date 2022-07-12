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
#include "ProxySession.h"
#include "Proxy.h"

#include "HTTPFilter.h"

#ifndef PF_NO_SSL_FILTER
#include "SSLFilter.h"
#endif

#include "POP3Filter.h"
#include "SMTPFilter.h"
#include "ProxyFilter.h"
#include "RawFilter.h"
#include "FTPFilter.h"
#include "FTPDataFilter.h"
#include "NNTPFilter.h"
#include "ICQFilter.h"
#include "XMPPFilter.h"

#include "auxutil.h"

using namespace ProtocolFilters;
#ifndef _C_API
using namespace nfapi;
#endif

ProxySession::ProxySession(Proxy & proxy) : m_proxy(proxy)
{
	m_disconnectLocal = m_disconnectRemote = false;
	m_disconnectRemotePending = false;
	m_sendPosted = m_receivePosted = 0;
	m_associatedFilterType = FT_NONE;
	m_flags = FF_DEFAULT;
	m_firstPacket = true;
	m_deletion = false;
	m_suspended = false;
	m_refCounter = 1;
}

ProxySession::~ProxySession(void)
{
	free();
}

bool ProxySession::init(NFAPI_NS ENDPOINT_ID id, int protocol, GEN_SESSION_INFO * pSessionInfo, int infoLen)
{
	m_id = id;
	m_protocol = protocol;

	if (!m_tcpSendsStream.open())
		return false;

	if (!m_tcpReceivesStream.open())
		return false;

	memcpy(&m_sessionInfo, pSessionInfo, infoLen);

	if (protocol == IPPROTO_TCP)
	{
		if (!(pSessionInfo->tcpConnInfo.filteringFlag & NF_FILTER))
			return false;
	}

	m_remoteEndpointStr = aux_getAddressString((sockaddr*)pSessionInfo->tcpConnInfo.remoteAddress);
	m_localEndpointStr = aux_getAddressString((sockaddr*)pSessionInfo->tcpConnInfo.localAddress);

	return true;
}

void ProxySession::free()
{
	AutoLock lock(m_cs);

	tFilters::iterator it;
	IFilterBase * pFilter;

	m_deletion = true;

	for (it = m_filters.begin(); it != m_filters.end(); it++)
	{
		pFilter = *it;
		
		// Send additional disconnect packets, to flush the buffers of aborted connections
		pFilter->tcp_packet(DD_INPUT, PD_SEND, NULL, 0);
		pFilter->tcp_packet(DD_INPUT, PD_RECEIVE, NULL, 0);
	}

	for (it = m_filters.begin(); it != m_filters.end(); it++)
	{
		pFilter = *it;
		delete pFilter;
	}

	m_filters.clear();
}

NFAPI_NS ENDPOINT_ID ProxySession::getId()
{
	return m_id;
}

int ProxySession::getProtocol()
{
	return m_protocol;
}

PFEvents * ProxySession::getParsersEventHandler()
{
	return m_proxy.getParsersEventHandler();
}

void ProxySession::getSessionInfo(GEN_SESSION_INFO * pSessionInfo)
{
	if (pSessionInfo)
	{
		*pSessionInfo = m_sessionInfo;
	}
}


IFilterBase * ProxySession::createFilter(PF_FilterType type, tPF_FilterFlags flags)
{
	IFilterBase * pFilter = NULL;

	switch (type)
	{
#ifndef PF_NO_SSL_FILTER
	case FT_SSL:
		pFilter = new SSLFilter(this, flags);
		break;
#endif
	case FT_HTTP:
		pFilter = new HTTPFilter(this, flags);
		break;

	case FT_POP3:
		pFilter = new POP3Filter(this, flags);
		break;

	case FT_SMTP:
		pFilter = new SMTPFilter(this, flags);
		break;

	case FT_PROXY:
		pFilter = new ProxyFilter(this, flags);
		break;

	case FT_RAW:
		pFilter = new RawFilter(this, flags);
		break;

	case FT_FTP:
		pFilter = new FTPFilter(this, flags);
		break;

	case FT_FTP_DATA:
		pFilter = new FTPDataFilter(this, flags);
		break;

	case FT_NNTP:
		pFilter = new NNTPFilter(this, flags);
		break;

	case FT_ICQ:
		pFilter = new ICQFilter(this, flags);
		break;

	case FT_XMPP:
		pFilter = new XMPPFilter(this, flags);
		break;

	default:
		return NULL;
	}

	return pFilter;
}

bool ProxySession::findAssociatedSession()
{
	const std::string & sAddr = (m_sessionInfo.tcpConnInfo.direction == NF_D_IN)?
		m_localEndpointStr : m_remoteEndpointStr;

	ProxySession * pAssocSession = 
		m_proxy.findAssociatedSession(
				sAddr, 
				m_sessionInfo.tcpConnInfo.direction == NF_D_OUT);

	if (pAssocSession != NULL && pAssocSession != this)
	{
		DbgPrint("ProxySession::tcp_packet() id=%llu found associated session - %llu", getId(), pAssocSession->getId());

		int flags = pAssocSession->getAssociatedFilterFlags();

		// Remove protocol filters
		tFilters::iterator it = m_filters.begin();
		while (it != m_filters.end())
		{
			if ((*it)->getCategory() == FC_PROTOCOL_FILTER)
			{
				delete *it;
				it = m_filters.erase(it);
			} else
			{
				// No need to add SSL filter if it is already in chain
				if ((*it)->getType() == FT_SSL)
				{
					flags = flags & ~FF_SSL_TLS;
				}

				it++;
			}
		}
		
		// Add needed filter (e.g. FT_FTP_DATA for FT_FTP)			
		addFilter(pAssocSession->getAssociatedFilterType(), 
			pAssocSession->getAssociatedFilterFlags());
		
		// Filter also SSL/TLS if needed
		if (flags & FF_SSL_TLS)
		{
			addFilter(FT_SSL, FF_SSL_TLS, OT_FIRST);
		}

		setAssociatedSessionInfo(pAssocSession->getAssociatedSessionInfo());

#ifndef PF_NO_SSL_FILTER
		SSLFilter * pAssocSSLFilter = (SSLFilter*)pAssocSession->getFilter(FT_SSL);
		if (pAssocSSLFilter)
		{
			tFilters::iterator it = m_filters.begin();
			while (it != m_filters.end())
			{
				if ((*it)->getType() == FT_SSL)
				{
					SSL_SESSION * pSession = pAssocSSLFilter->getRemoteSSLSession();
					if (pSession)
					{
						((SSLFilter*)*it)->setRemoteSSLSession(pSession);
					}
				}

				it++;
			}
		}
#endif
		pAssocSession->release();

		return true;
	}

	if (pAssocSession)
	{
		pAssocSession->release();
	}

	return false;
}

eFilterResult ProxySession::tcp_packet(IFilterBase * pSourceFilter,
							  eDataDirection dd,
							ePacketDirection pd,
							const char * buf, int len)
{
	AutoLock lock(m_cs);

	IFilterBase * pFilter;
	int nextFilter = 0;
	eFilterResult res = FR_PASSTHROUGH;
	int sourceFilter = findFilter(pSourceFilter);

	DbgPrint("ProxySession::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				getId(), dd, pd, len);

	switch (dd)
	{
	case DD_INPUT:
		{
			if (sourceFilter != FILTER_UNKNOWN)
			{
				nextFilter = sourceFilter + 1;
			}

			if (m_firstPacket)
			{
				const char szKAVCONN[] = "KAVCONN_ID";
				if (pd == PD_SEND && len >= (int)sizeof(szKAVCONN))
				{
					if (memcmp(buf, szKAVCONN, sizeof(szKAVCONN)-1) == 0)
					{
						if (len > 500)
						{
							tcp_postData(pd, buf, 500);
							buf += 500;
							len -= 500;
						} else
						{
							tcp_postData(pd, buf, len);
							return res;
						}
					}
				}
			}

			if (pd == PD_RECEIVE && len == 0)
			{
				m_disconnectRemotePending = true;
			}

			for (;;)
			{
				if (nextFilter >= (int)m_filters.size())
				{
					// Try to find an associated session and make necessary updates.
					// E.g. link FTP data channel to FTP control session.
					if (m_firstPacket)
					{
						m_firstPacket = false;

						if (findAssociatedSession())
						{
							if (nextFilter >= (int)m_filters.size())
								break;
							pFilter = m_filters[nextFilter];
						} else
							break;
					} else
						break;
				}

				pFilter = m_filters[nextFilter];

				// Try to find an associated session and make necessary updates.
				// E.g. link FTP data channel to FTP control session.
				if (m_firstPacket)// && (pFilter->getCategory() != FC_PREPROCESSOR))
				{
					m_firstPacket = false;

					if (findAssociatedSession())
					{
						if (nextFilter >= (int)m_filters.size())
							break;

						pFilter = m_filters[nextFilter];
					}
				}

				if (pFilter->isDeleted())
				{
					res = FR_DELETE_ME;
				} else
				{
					res = pFilter->tcp_packet(dd, pd, buf, len);
				}

				if (res == FR_DELETE_ME)
				{
					delete m_filters[nextFilter];
					m_filters.erase(m_filters.begin() + nextFilter);
					continue;
				} else
				if (res == FR_DELETE_OTHERS)
				{
					deleteProtocolFilters(pFilter);
				} else
				if (res == FR_ERROR)
				{
					delete m_filters[nextFilter];
					m_filters.erase(m_filters.begin() + nextFilter);

					if (len > 0)
					{
						PFEvents * pHandler = m_proxy.getParsersEventHandler();
						if (pHandler)
						{
							pHandler->tcpPostSend(m_id, NULL, 0);
						}
						return res;
					}
				} else
				if (res == FR_DATA_EATEN)
				{
					break;
				}

				nextFilter++;
			}

			if (nextFilter < (int)m_filters.size())
			{
				break;
			} else
			{
				sourceFilter = nextFilter + 1;
			}
		}

	case DD_OUTPUT:
		{
			if (sourceFilter == FILTER_UNKNOWN)
			{
				nextFilter = (int)m_filters.size() - 1;
			} else
			{
				nextFilter = sourceFilter - 1;
		
				if (nextFilter >= (int)m_filters.size())
				{
					nextFilter = (int)m_filters.size() - 1;
				}
			}

			while (nextFilter >= 0)
			{
				if (m_filters[nextFilter]->getCategory() == FC_PREPROCESSOR)
				{
					res = m_filters[nextFilter]->tcp_packet(DD_OUTPUT, pd, buf, len);
					if (res != FR_PASSTHROUGH)
						return res;
				}
				
				nextFilter--;
			}

			return tcp_postData(pd, buf, len)? FR_DATA_EATEN : FR_PASSTHROUGH;
		}
		break;
	
	default:
		return FR_ERROR;
	}

	return res;
}
		
eFilterResult ProxySession::udp_packet(IFilterBase * pSourceFilter,
							  eDataDirection dd,
							ePacketDirection pd,
							const unsigned char * remoteAddress, 
							const char * buf, int len)
{
	return FR_PASSTHROUGH;
}

void ProxySession::tcp_canPost(ePacketDirection pd)
{
	AutoLock lock(m_cs);
	tcp_postData(pd, NULL, -1);
}

void ProxySession::udp_canPost(ePacketDirection pd)
{
	AutoLock lock(m_cs);
//	udp_postData(pd, NULL, 0);
}

bool ProxySession::addFilter(PF_FilterType type, 
				tPF_FilterFlags flags,
				PF_OpTarget target, 
				PF_FilterType typeBase)
{
	AutoLock lock(m_cs);

	int insertIndex = -1;

	if (target == OT_FIRST)
	{
		insertIndex = -1;
	} else
	if (target == OT_LAST)
	{
		insertIndex = (int)m_filters.size();
	} else
	if (typeBase != FT_NONE)
	{
		int targetFilter = findFilter(typeBase);
		
		if (targetFilter != FILTER_UNKNOWN)
		{
			if (target == OT_PREV)
			{
				insertIndex = targetFilter - 1;
			} else
			{
				insertIndex = targetFilter + 1;
			}
		}
	}

	IFilterBase * pFilter = createFilter(type, flags);

	if (!pFilter)
		return false;

	if (insertIndex < 0)
	{
		m_filters.insert(m_filters.begin(), pFilter);
	} else
	if (insertIndex >= (int)m_filters.size())
	{
		m_filters.insert(m_filters.end(), pFilter);
	} else
	{
		m_filters.insert(m_filters.begin() + insertIndex, pFilter);
	}

	return true;
}

		
bool ProxySession::deleteFilter(PF_FilterType type)
{
	AutoLock lock(m_cs);

	int filterIndex = findFilter(type);
	
	if (filterIndex == FILTER_UNKNOWN)
		return false;

	tFilters::iterator it = m_filters.begin() + filterIndex;

	(*it)->setDeleted();
//	delete *it;
//	m_filters.erase(it);

	return true;
}

int ProxySession::getFilterCount()
{
	AutoLock lock(m_cs);
	return (int)m_filters.size();
}

bool ProxySession::isFilterActive(PF_FilterType type)
{
	AutoLock lock(m_cs);
	for (int i=0; i<(int)m_filters.size(); i++)
	{
		if (m_filters[i]->getType() == type &&
			m_filters[i]->isActive())
			return true;
	}
	return false;
}

bool ProxySession::deleteProtocolFilters(IFilterBase * exceptThisOne)
{
	tFilters::iterator it = m_filters.begin();

	while (it != m_filters.end())
	{
		if ((*it != exceptThisOne) && ((*it)->getCategory() == FC_PROTOCOL_FILTER))
		{
			delete *it;
			it = m_filters.erase(it);
		} else
		{
			it++;
		}
	}

	return true;
}

IFilterBase * ProxySession::getFilter(PF_FilterType type)
{
	int index;
	
	index = findFilter(type);

	if (index != FILTER_UNKNOWN)
		return m_filters[index];

	return NULL;
}


int ProxySession::findFilter(PF_FilterType type)
{
	for (int i=0; i<(int)m_filters.size(); i++)
	{
		if (m_filters[i]->getType() == type)
			return i;
	}
	return FILTER_UNKNOWN;
}

int ProxySession::findFilter(IFilterBase * pFilter)
{
	if (pFilter == NULL)
		return FILTER_UNKNOWN;

	for (int i=0; i<(int)m_filters.size(); i++)
	{
		if (m_filters[i] == pFilter)
			return i;
	}
	return FILTER_UNKNOWN;
}

bool ProxySession::postObject(PFObjectImpl & object)
{
	AutoLock lock(m_cs);

	tPF_ObjectType ot = object.getType();

	DbgPrint("ProxySession::postObject() id=%llu ot=%d", getId(), ot);

	if (ot == OT_TCP_DISCONNECT_LOCAL || ot == OT_TCP_DISCONNECT_REMOTE)
	{
		tcpDisconnect((PF_ObjectType)ot);
		return true;
	}

	if (object.isReadOnly())
		return false;

	int filterType = ot - (ot % FT_STEP);

	IFilterBase * pFilter = getFilter((PF_FilterType)filterType);

	if (!pFilter)
	{
		DbgPrint("ProxySession::postObject() id=%llu ot=%d filter not found", getId(), ot);
	
		ePacketDirection pd;

		// Send the raw buffers as-is, even if there is no FT_RAW filter in a chain

		switch (object.getType())
		{
		case OT_RAW_INCOMING:
			pd = PD_RECEIVE;
			break;
		case OT_RAW_OUTGOING:
			pd = PD_SEND;
			break;
		default:
			return false;
		}

		// Send the buffer to a first preprocessor if it exists
		
		for (int i=(int)m_filters.size() - 2; i >= 0; i--)
		{
			if (m_filters[i]->getCategory() == FC_PREPROCESSOR)
			{
				pFilter = m_filters[i+1];
				break;
			}				
		}

		return tcpPostObjectStreams(pFilter, pd, object);
	}

	return pFilter->postObject(object);
}

bool ProxySession::tcp_postData(ePacketDirection pd, const char * buf, int len)
{
	DbgPrint("ProxySession::postData() id=%llu pd=%d len=%d", getId(), pd, len);
	
	IOB vTempBuf;
	vTempBuf.append(NULL, NF_TCP_PACKET_BUF_SIZE, false);
	char * tempBuf = vTempBuf.buffer();
	tStreamSize tempBufSize = (tStreamSize)vTempBuf.size();
	int nBytes = 0;
	bool result = true;

	PFEvents * pHandler = m_proxy.getParsersEventHandler();
	if (!pHandler)
		return false;

	switch (pd)
	{
	
	case PD_RECEIVE:
		{
			tStreamPos pos = m_tcpReceivesStream.seek(0, FILE_CURRENT);

			if (len == -1)
			{
				m_receivePosted = 0;
//				if (m_tcpReceivesStream.size() < NF_TCP_PACKET_BUF_SIZE * 32)
				{
					DbgPrint("ProxySession::postData() resume connection");
					pHandler->tcpSetConnectionState(m_id, FALSE);
				}
			}

			if (len == 0)
			{
				m_disconnectRemote = true;
			}

			if (buf && (len > 0))
			{
				m_tcpReceivesStream.seek(0, FILE_END);

				if (!m_tcpReceivesStream.write(buf, len))
				{
					DbgPrint("ProxySession::postData() m_tcpReceivesStream.write failed");
					result = false;
					break;
				}

				m_tcpReceivesStream.seek(pos, FILE_BEGIN);

				if (m_tcpReceivesStream.size() > NF_TCP_PACKET_BUF_SIZE * 32)
				{
					DbgPrint("ProxySession::postData() suspend connection");
					pHandler->tcpSetConnectionState(m_id, TRUE);
				}

				if (m_receivePosted > NF_TCP_PACKET_BUF_SIZE * 1024)
				{
					DbgPrint("ProxySession::postData() m_receivePosted too large (%d)", m_receivePosted);
					break;
				}
			} 

			if (m_suspended)
			{
				DbgPrint("ProxySession::postData() suspended");
				break;
			}
			
			for (int i=0; i<8; i++)
			{
				nBytes = m_tcpReceivesStream.read(tempBuf, tempBufSize);
				if (nBytes == 0)
					break;

				m_receivePosted += nBytes;

				DbgPrint("ProxySession::postData() nf_tcpPostReceive id=%llu len=%d", m_id, nBytes);
				pHandler->tcpPostReceive(m_id, tempBuf, nBytes);
			}

			if (nBytes == 0)
			{
				DbgPrint("ProxySession::postData() nBytes == 0");

				m_tcpReceivesStream.reset();

				if (m_disconnectRemote)
				{
					m_disconnectRemote = false;
	
					DbgPrint("ProxySession::postData() nf_tcpPostReceive id=%llu len=%d", getId(), 0);
					
					if (m_disconnectRemotePending)
					{
						result = (pHandler->tcpPostReceive(m_id, NULL, 0) == NF_STATUS_SUCCESS);
					} else
					{
//						result = (pHandler->tcpPostReceive(m_id, NULL, 0) == NF_STATUS_SUCCESS);
						tcp_packet(NULL, DD_OUTPUT, PD_SEND, NULL, 0);
						result = true;
					}
					break;
				}

				result = false;
			}

			break;
		}
	
	case PD_SEND:
		{
			tStreamPos pos = m_tcpSendsStream.seek(0, FILE_CURRENT);

			if (len == -1)
			{
				m_sendPosted = 0;

/*				if ((m_tcpSendsStream.size() - pos) < NF_TCP_PACKET_BUF_SIZE * 32)
				{
					DbgPrint("ProxySession::postData() resume connection (%llu bytes in buffer)", m_tcpSendsStream.size());
					pHandler->tcpSetConnectionState(m_id, FALSE);
				}
*/
			}

			if (len == 0)
			{
				m_disconnectLocal = true;
			}

			if (buf && (len > 0))
			{
				m_tcpSendsStream.seek(0, FILE_END);

				if (!m_tcpSendsStream.write(buf, len))
				{
					DbgPrint("ProxySession::postData() m_tcpSendsStream.write failed");
					result = false;
					break;
				}
	
				m_tcpSendsStream.seek(pos, FILE_BEGIN);

				if (m_tcpSendsStream.size() > NF_TCP_PACKET_BUF_SIZE * 32)
				{
					DbgPrint("ProxySession::postData() suspend connection (%llu bytes in buffer)", m_tcpSendsStream.size()-pos);
					pHandler->tcpSetConnectionState(m_id, TRUE);
				}

				if (m_sendPosted > NF_TCP_PACKET_BUF_SIZE * 32)
				{
					DbgPrint("ProxySession::postData() m_sendPosted too large (%llu bytes in buffer)", m_tcpSendsStream.size()-pos);
					break;
				}
			}

			if (m_suspended)
			{
				DbgPrint("ProxySession::postData() suspended");
				break;
			}

			for (int i=0; i<32; i++)
			{
				nBytes = m_tcpSendsStream.read(tempBuf, tempBufSize);
				if (nBytes == 0)
					break;

				m_sendPosted += nBytes;

				DbgPrint("ProxySession::postData() nf_tcpPostSend id=%llu len=%d", m_id, nBytes);
				pHandler->tcpPostSend(m_id, tempBuf, nBytes);
			}

			if (nBytes == 0)
			{
				DbgPrint("ProxySession::postData() nBytes == 0");

				m_tcpSendsStream.reset();

				if (len == -1)
				{
					DbgPrint("ProxySession::postData() resume connection");
					pHandler->tcpSetConnectionState(m_id, FALSE);
				}

				if (m_disconnectLocal)
				{
					m_disconnectLocal = false;

					DbgPrint("ProxySession::postData() nf_tcpPostSend id=%llu len=%d", getId(), 0);

					result = (pHandler->tcpPostSend(m_id, NULL, 0) == NF_STATUS_SUCCESS);
					break;
				}

				result = false;
			}

			break;
		}
	}

	return result;
}

void ProxySession::tcpDisconnect(PF_ObjectType dt)
{
	switch (dt)
	{
	case OT_TCP_DISCONNECT_LOCAL:
		{
			tcp_packet(NULL, DD_OUTPUT, PD_SEND, NULL, 0);
			break;
		}
	
	case OT_TCP_DISCONNECT_REMOTE:
		{
			tcp_packet(NULL, DD_OUTPUT, PD_RECEIVE, NULL, 0);
			break;
		}
	default:
		break;
	}
}

bool ProxySession::tcpPostObjectStreams(IFilterBase * pSourceFilter, ePacketDirection pd, PFObjectImpl & object)
{
	PFStream * pStream;

	DbgPrint("ProxySession::tcpPostObjectStreams() pd=%d", pd);

	for (int i=0; i < object.getStreamCount(); i++)
	{
		pStream = object.getStream(i);
		if (!pStream)
			continue;

		if (!tcpPostStream(pSourceFilter, pd, pStream))
			return false;
	}

	return true;
}

bool ProxySession::tcpPostStream(IFilterBase * pSourceFilter, ePacketDirection pd, PFStream * pStream)
{
	IOB vTempBuf;
	vTempBuf.append(NULL, NF_TCP_PACKET_BUF_SIZE+1, false);
	char * tempBuf = vTempBuf.buffer();
	tStreamSize tempBufSize = (tStreamSize)vTempBuf.size();
	int n;

	DbgPrint("ProxySession::tcpPostStream() pd=%d", pd);

	if (pStream->size() == 0)
		return true;

	pStream->seek(0, FILE_BEGIN);

	for (;;) 
	{
		n = pStream->read(tempBuf, tempBufSize);
		if (n == 0)
			break;
			
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		tempBuf[n] = 0;
		DbgPrint("ProxySession::tcpPostStream() tcp_packet pd=%d, len=%d:\n%s", 
			pd, n, tempBuf);
#endif

		tcp_packet(pSourceFilter, DD_OUTPUT, pd, tempBuf, n);
	}

	return true;
}

std::string ProxySession::getRemoteEndpointStr()
{
	return m_remoteEndpointStr;
}

void ProxySession::setRemoteEndpointStr(const std::string & s)
{
	DbgPrint("ProxySession::setRemoteEndpointStr() id=%llu %s", getId(), s.c_str());
	m_remoteEndpointStr = s;
	setFirstPacket(true);
}

std::string ProxySession::getLocalEndpointStr()
{
	return m_localEndpointStr;
}

void ProxySession::setLocalEndpointStr(const std::string & s)
{
	DbgPrint("ProxySession::setLocalEndpointStr() id=%llu %s", getId(), s.c_str());
	m_localEndpointStr = s;
	setFirstPacket(true);
}

std::string ProxySession::getAssociatedRemoteEndpointStr()
{
	return m_associatedRemoteEndpointStr;
}

void ProxySession::setAssociatedRemoteEndpointStr(const std::string & s)
{
	DbgPrint("setAssociatedRemoteEndpointStr id=%llu, %s", m_id, s.c_str());
	m_associatedRemoteEndpointStr = s;
}

std::string ProxySession::getAssociatedLocalEndpointStr()
{
	return m_associatedLocalEndpointStr;
}

void ProxySession::setAssociatedLocalEndpointStr(const std::string & s)
{
	DbgPrint("setAssociatedLocalEndpointStr id=%llu, %s", m_id, s.c_str());
	m_associatedLocalEndpointStr = s;
}

PF_FilterType ProxySession::getAssociatedFilterType()
{
	return m_associatedFilterType;
}

void ProxySession::setAssociatedFilterType(PF_FilterType t)
{
	m_associatedFilterType = t;
}

std::string ProxySession::getAssociatedSessionInfo()
{
	return m_associatedSessionInfo;
}

void ProxySession::setAssociatedSessionInfo(const std::string & s)
{
	m_associatedSessionInfo = s;
}

PF_FilterFlags ProxySession::getAssociatedFilterFlags()
{
	return m_flags;
}

void ProxySession::setAssociatedFilterFlags(PF_FilterFlags f)
{
	m_flags = f;
}

bool ProxySession::isFirstPacket()
{
	return m_firstPacket;
}

void ProxySession::setFirstPacket(bool v)
{
	m_firstPacket = v;
}

bool ProxySession::canDisableFiltering()
{
	// Disable filtering for the connection when there are no filters 
	// and IO buffers are empty
	return ((getFilterCount() == 0) &&
		(m_tcpSendsStream.size() == 0 || (m_tcpSendsStream.seek(0, FILE_CURRENT) == m_tcpSendsStream.size())) &&
		(m_tcpReceivesStream.size() == 0 || (m_tcpReceivesStream.seek(0, FILE_CURRENT) == m_tcpReceivesStream.size())));
}

bool ProxySession::isDeletion()
{
	return m_deletion;
}

bool ProxySession::isSuspended()
{
	AutoLock lock(m_cs);
	return m_suspended;
}

void ProxySession::setSessionState(bool suspend)
{
	AutoLock lock(m_cs);

	m_suspended = suspend;

	if (!suspend)
	{
		tcp_postData(PD_SEND, NULL, -1);
		tcp_postData(PD_RECEIVE, NULL, -1);
	}
}
