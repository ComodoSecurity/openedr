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
#include "NNTPFilter.h"
#include "ProxySession.h"
#include "auxutil.h"

using namespace ProtocolFilters;

#define NNTP_WAIT_HEADER_PERIOD  15
#define MAX_CMD_BUF_LEN 2048

NNTPFilter::NNTPFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags),
	m_object(OT_NNTP_ARTICLE)
{
	m_pSession->getSessionInfo(&m_si);
	m_state = STATE_NONE;
	m_time = nf_time();
	setActive(false);
}

NNTPFilter::~NNTPFilter(void)
{
}

bool NNTPFilter::checkPacket(ePacketDirection pd, const char * buf, int len)
{
	for (int i = 0; i<len; i++)
	{
		if (((unsigned char)buf[i]) < 9) 
			return false;
	}
	return true;
}

int NNTPFilter::getEndOfMessage(const char * buf, int len)
{
	const char eof[] = "\r\n.\r\n";

	if (len < 5)
		return -1;

	for (int i=len-5; i>=0; i--)
	{
		if (!strchr("\r\n\t. ", buf[i]))
			break;

		if (memcmp(eof, buf + i, sizeof(eof)-1) == 0)
		{
			return i;
		}
	}

	return -1;
}

eFilterResult NNTPFilter::updateCmdBuffer(const char * buf, int len)
{
	if ((m_buffer.size() + len) > MAX_CMD_BUF_LEN)
		return FR_DELETE_ME;

	m_buffer.append(buf, len);	

	for (int i=m_buffer.size()-2; i>=0; i--)
	{
		if (!strchr("\r\n", m_buffer.buffer()[i]))
			break;

		if (memcmp("\r\n", m_buffer.buffer() + i, 2) == 0)
		{
			return FR_DATA_EATEN;
		}
	}

	return FR_PASSTHROUGH;
}

eFilterResult NNTPFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	const char cmdMODE[] = "MODE ";
	const char cmdARTICLE[] = "ARTICLE ";
	const char cmdPOST[] = "POST";
	const char waitHeader[] = "X-WAIT: 0\r\n";
	const char retSTARTPOST[] = "340 Continue posting; Period on a line by itself to end\r\n"; 
	eFilterResult res = FR_DELETE_ME;

	DbgPrint("NNTPFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);

	if (m_si.tcpConnInfo.direction != NFAPI_NS NF_D_OUT)
		return FR_DELETE_ME;

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (!pHandler)
		return FR_DELETE_ME;

	if (len > 0)
	{
		switch (m_state)
		{
		case STATE_NONE:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				m_buffer.reset();

				if (pd == PD_RECEIVE)
				{
					res = updateCmdBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;

					// Check for response digits
					if (m_buffer.size() > 3)
					{
						for (int i=0; i<3; i++)
						{
							if (!isdigit(m_buffer.buffer()[i]))
								return FR_DELETE_ME;
						}
					}

					m_buffer.reset();

					res = FR_PASSTHROUGH;
					m_state = STATE_MODE;

					setActive(true);
				} else
				{
					return FR_DELETE_ME;
				}
				
				break;
			}

		case STATE_MODE:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_SEND)
				{
					res = updateCmdBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;

					buf = m_buffer.buffer();
					len = m_buffer.size();
					
					res = FR_DELETE_ME;

					if (len >= (int)sizeof(cmdMODE))
					{
						if (strnicmp(buf, cmdMODE, sizeof(cmdMODE)-1) == 0)
						{
							m_state = STATE_CMD;
							res = FR_PASSTHROUGH;
						} 
					}

					m_buffer.reset();
				} else
				{
					res = FR_PASSTHROUGH;
				}
			}
			break;

		case STATE_CMD:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_SEND)
				{
					res = updateCmdBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;

					res = FR_PASSTHROUGH;

					buf = m_buffer.buffer();
					len = m_buffer.size();
					
					if (len >= (int)sizeof(cmdARTICLE))
					{
						if (strnicmp(buf, cmdARTICLE, sizeof(cmdARTICLE)-1) == 0)
						{
							m_state = STATE_ARTICLE_RESP;
						} 
					}

					if (len >= (int)sizeof(cmdPOST))
					{
						if (strnicmp(buf, cmdPOST, sizeof(cmdPOST)-1) == 0)
						{
							m_object.clear();
							
							if (!(m_flags & FF_READ_ONLY_OUT))
							{
								res = FR_DATA_EATEN;
								m_state = STATE_POST;

								m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, retSTARTPOST, sizeof(retSTARTPOST)-1);
							} else
							{
								m_state = STATE_POST_RESP_1;
							}
						} 
					}

					m_buffer.reset();
				} else
				{
					res = FR_PASSTHROUGH;
				}
			}
			break;

		case STATE_ARTICLE_RESP:
			{
				if (pd == PD_RECEIVE)
				{
					m_buffer.append(buf, len);

					int offset = aux_find('\n', m_buffer.buffer(), m_buffer.size());
						
					if (offset == -1)
						return FR_DATA_EATEN;

					if (strchr("23", m_buffer.buffer()[0]))
					{
						m_state = STATE_ARTICLE_DATA;
						m_object.clear();

						// Passthrough the data immediately for read-only session
						if (m_flags & FF_READ_ONLY_IN)
						{
							res = FR_PASSTHROUGH;
						} else
						{
							res = FR_DATA_EATEN;
							m_pSession->tcp_packet(this, DD_OUTPUT, pd, 
								m_buffer.buffer(), offset + 1);
						}

						int newLen = m_buffer.size() - (offset + 1);
						memmove(m_buffer.buffer(), m_buffer.buffer()+offset+1, newLen);
						m_buffer.resize(newLen);
						len = 0;
					} else
					{
						m_state = STATE_CMD;
						m_buffer.reset();
						return FR_PASSTHROUGH;
					}
				}
			}

		case STATE_ARTICLE_DATA:
			{
				if (pd == PD_RECEIVE)
				{
					PFStream * pStream = m_object.getStream(0);

					m_buffer.append(buf, len);

					int eof = getEndOfMessage(m_buffer.buffer(), m_buffer.size());
					
					if (eof > 0)
					{
						pStream->write(m_buffer.buffer(), eof);

						m_object.setType(OT_NNTP_ARTICLE);

						if (m_flags & FF_READ_ONLY_IN)
						{
							m_object.setReadOnly(true);
						} else
						{
							// Send a fake header to email client to avoid breaking the connection 
							// due to server timeout
							m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, waitHeader, sizeof(waitHeader)-1);
						}

						// Filter
						try {
							pHandler->dataAvailable(m_pSession->getId(), &m_object);
						} catch (...)
						{
						}

						m_state = STATE_CMD;
						m_buffer.reset();
					} else
					{
						unsigned int newLen = 10;
						if (m_buffer.size() > newLen)
						{
							pStream->write(m_buffer.buffer(), m_buffer.size()-newLen);
							memmove(m_buffer.buffer(), m_buffer.buffer()+m_buffer.size()-newLen, newLen);
							m_buffer.resize(newLen);
						}

						if (!(m_flags & FF_READ_ONLY_IN))
						{
							nf_time_t curTime = nf_time();
							
							if ((curTime - m_time) > NNTP_WAIT_HEADER_PERIOD)
							{
								m_time = curTime;

								// Send a fake header to email client periodically
								// to avoid breaking the connection due to server timeout
								m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, waitHeader, sizeof(waitHeader)-1);
							}
						}
					}
				
					return (m_flags & FF_READ_ONLY_IN)? FR_PASSTHROUGH : FR_DATA_EATEN;
				}
				res = FR_PASSTHROUGH;
			}
			break;

		case STATE_POST_RESP_1:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				res = FR_PASSTHROUGH;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						int code = atoi(buf);

						if (code == 340)
						{
							m_state = STATE_POST;
						} 
					}
				}
			}
			break;
		
		case STATE_POST:
			{
				if (pd == PD_SEND)
				{
					PFStream * pStream = m_object.getStream(0);

					m_buffer.append(buf, len);

					int eof = getEndOfMessage(m_buffer.buffer(), m_buffer.size());
					
					if (eof > 0)
					{
						m_object.setType(OT_NNTP_POST);

						pStream->write(m_buffer.buffer(), eof);
						m_buffer.reset();

						if (m_flags & FF_READ_ONLY_OUT)
						{
//							m_state = STATE_CMD;
							m_object.setReadOnly(true);
						} else
						{
							// Send NOOP to server, to avoid breaking the connection 
							//m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, waitCmd, sizeof(waitCmd)-1);
						}

						m_state = STATE_CMD;

						// Filter
						try {
							pHandler->dataAvailable(m_pSession->getId(), &m_object);
						} catch (...)
						{
						}

					} else
					{
						int newLen = 10;
						if ((int)m_buffer.size() > newLen)
						{
							pStream->write(m_buffer.buffer(), m_buffer.size()-newLen);
							memmove(m_buffer.buffer(), m_buffer.buffer()+m_buffer.size()-newLen, newLen);
							m_buffer.resize(newLen);
						}
					}
				
					return (m_flags & FF_READ_ONLY_OUT)? FR_PASSTHROUGH : FR_DATA_EATEN;
				}

				res = FR_DATA_EATEN;
			}
			break;

		case STATE_POST_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				res = FR_DATA_EATEN;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						int code = atoi(buf);

						if (code == 340)
						{
							m_state = STATE_CMD;
							res = FR_DATA_EATEN;

							const char dot[] = "\r\n.\r\n";
							
							m_pSession->tcpPostObjectStreams(this, PD_SEND, m_object);

							m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, dot, sizeof(dot)-1);
						} 
					}
				}
			}
			break;

		}

		return res;
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
		
eFilterResult NNTPFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType NNTPFilter::getType()
{
	return FT_NNTP;
}

PF_FilterCategory NNTPFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool NNTPFilter::postObject(PFObjectImpl & object)
{
	switch (object.getType())
	{
	case OT_NNTP_ARTICLE:
		{
			ePacketDirection pd = PD_RECEIVE;
			const char dot[] = "\r\n.\r\n";
		
			m_pSession->tcpPostObjectStreams(this, pd, object);
			m_pSession->tcp_packet(this, DD_OUTPUT, pd, dot, sizeof(dot)-1);
			return true;
		}
		break;
	
	case OT_NNTP_POST:
		{
			const char cmdPOST[] = "POST\r\n";
			m_object = object;
			m_state = STATE_POST_RESP;
			m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, cmdPOST, sizeof(cmdPOST)-1);
			return true;
		}
		break;
	
	default:
		return false;
	}
}

