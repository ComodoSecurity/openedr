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
#include "POP3Filter.h"
#include "ProxySession.h"
#include "auxutil.h"

using namespace ProtocolFilters;

#define POP3_WAIT_HEADER_PERIOD  15
#define MAX_CMD_BUF_LEN 2048

POP3Filter::POP3Filter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags),
	m_object(OT_POP3_MAIL_INCOMING)
{
	m_pSession->getSessionInfo(&m_si);
	m_state = STATE_NONE;
	m_time = nf_time();
	setActive(false);
}

POP3Filter::~POP3Filter(void)
{
}

bool POP3Filter::checkPacket(ePacketDirection pd, const char * buf, int len)
{
	for (int i = 0; i<len; i++)
	{
		if (((unsigned char)buf[i]) < 9) 
			return false;
	}
	return true;
}

int POP3Filter::getEndOfMessage(const char * buf, int len)
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

eFilterResult POP3Filter::updateCmdBuffer(const char * buf, int len)
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

eFilterResult POP3Filter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	const char cmdUSER[] = "USER";
	const char cmdPASS[] = "PASS";
	const char cmdRETR[] = "RETR";
	const char cmdSTLS[] = "STLS";
	const char cmdSTARTTLS[] = "STARTTLS";
	const char cmdAUTH[] = "AUTH";
	const char cmdAUTH_1[] = "AUTH ";
	const char cmdAUTH_2[] = "APOP ";
	const char cmdCAPA[] = "CAPA";
	const char waitHeader[] = "X-WAIT: 0\r\n";
	const char cmdPASV[] = "PASV";
	const char cmdPORT[] = "PORT";
	const char cmdEPSV[] = "EPSV";
	const char cmdEPRT[] = "EPRT";
	eFilterResult res = FR_DELETE_ME;

	DbgPrint("POP3Filter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
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

				m_state = STATE_USER;
				m_buffer.reset();

				if (pd == PD_RECEIVE)
				{
					if (strchr("+-", buf[0]))
						res = FR_PASSTHROUGH;
				}
				
				break;
			}

		case STATE_USER:
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

					if (len >= (int)sizeof(cmdUSER))
					{
						if (strnicmp(buf, cmdUSER, sizeof(cmdUSER)-1) == 0)
						{
							m_state = STATE_USER_RESP;
							res = FR_PASSTHROUGH;
						} 
					}

					if (m_state == STATE_USER)
					{
						if (len >= (int)sizeof(cmdSTLS))
						{
							if (strnicmp(buf, cmdSTLS, sizeof(cmdSTLS)-1) == 0)
							{
								m_state = STATE_STLS_RESP;
								res = FR_PASSTHROUGH;
							} 
						}
					}

					if (m_state == STATE_USER)
					{
						if (len >= (int)sizeof(cmdSTARTTLS))
						{
							if (strnicmp(buf, cmdSTARTTLS, sizeof(cmdSTARTTLS)-1) == 0)
							{
								m_state = STATE_STLS_RESP;
								res = FR_PASSTHROUGH;
							} 
						}
					}

					if (m_state == STATE_USER)
					{
						if (len >= (int)sizeof(cmdAUTH))
						{
							if (strnicmp(buf, cmdAUTH_2, sizeof(cmdAUTH_2)-1) == 0)
							{
								m_state = STATE_RETR;
								res = FR_PASSTHROUGH;
							} else
							if (strnicmp(buf, cmdAUTH_1, sizeof(cmdAUTH_1)-1) == 0)
							{
								m_state = STATE_RETR;
								res = FR_PASSTHROUGH;
							} else
							if (strnicmp(buf, cmdAUTH, sizeof(cmdAUTH)-1) == 0)
							{
								res = FR_PASSTHROUGH;
							} else
							if (strnicmp(buf, cmdCAPA, sizeof(cmdCAPA)-1) == 0)
							{
								res = FR_PASSTHROUGH;
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

		case STATE_STLS_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						if (buf[0] == '-')
						{
							m_state = STATE_USER;
							// Bypass the response to email client
							res = FR_PASSTHROUGH;
						} else
						if (buf[0] == '+')
						{
							m_state = STATE_USER;
							res = FR_DATA_EATEN;

							// Send the response to email client
							m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);

							// Prepare for SSL handshake if TLS filtering flag is turned on
							if (m_flags & FF_SSL_TLS)
							{
								m_pSession->addFilter(FT_SSL, m_flags, OT_FIRST);
							}
						} 
					}
				}
			}
			break;


		case STATE_USER_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						if (buf[0] == '-')
						{
							m_state = STATE_USER;
							res = FR_PASSTHROUGH;
						} else
						if (buf[0] == '+')
						{
							m_state = STATE_PASS;
							res = FR_PASSTHROUGH;
						}
					}
		
					break;
				}
			}

		case STATE_PASS:
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
					
					if (len >= (int)sizeof(cmdPASS))
					{
						if (strnicmp(buf, cmdPASS, sizeof(cmdPASS)-1) == 0)
						{
							m_state = STATE_PASS_RESP;
							res = FR_PASSTHROUGH;
						} 
					}

					m_buffer.reset();
				}
			}
			break;

		case STATE_PASS_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						if (buf[0] == '-')
						{
							m_state = STATE_USER;
							res = FR_PASSTHROUGH;
						} else
						if (buf[0] == '+')
						{
							m_state = STATE_RETR;
							res = FR_PASSTHROUGH;
						} 
					}
				}
			}
			break;

		case STATE_RETR:
			{
				setActive(true);

				if (pd == PD_SEND)
				{
					if (!checkPacket(pd, buf, len))
						return FR_DELETE_ME;

					res = updateCmdBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;

					buf = m_buffer.buffer();
					len = m_buffer.size();
					
					if (len >= (int)sizeof(cmdRETR))
					{
						if (strnicmp(buf, cmdPASV, sizeof(cmdPASV)-1) == 0 ||
							strnicmp(buf, cmdPORT, sizeof(cmdPORT)-1) == 0 ||
							strnicmp(buf, cmdEPRT, sizeof(cmdEPRT)-1) == 0 ||
							strnicmp(buf, cmdEPSV, sizeof(cmdEPSV)-1) == 0)
						{
							return FR_DELETE_ME;
						}

						if (strnicmp(buf, cmdRETR, sizeof(cmdRETR)-1) == 0)
						{
							for (int i=5; i<len; i++)
							{
								if (strchr("\r\n", buf[i]))
									break;
								if (!isdigit(buf[i]))
									return FR_DELETE_ME;
							}
							m_state = STATE_RETR_RESP;
						} 
					}

					m_buffer.reset();
				}

				res = FR_PASSTHROUGH;
			}
			break;

		case STATE_RETR_RESP:
			{
				if (pd == PD_RECEIVE)
				{
					m_buffer.append(buf, len);

					int offset = aux_find('\n', m_buffer.buffer(), m_buffer.size());
						
					if (offset == -1)
						return FR_DATA_EATEN;

					if (m_buffer.buffer()[0] == '-')
					{
						m_state = STATE_RETR;
						m_buffer.reset();
						return FR_PASSTHROUGH;
					} else
					if (m_buffer.buffer()[0] == '+')
					{
						m_state = STATE_RETR_MAIL;
						m_object.clear();

						// Passthrough the data immediately for read-only session
						if (m_flags & FF_READ_ONLY_IN)
						{
							res = FR_PASSTHROUGH;
						} else
						{
							res = FR_DATA_EATEN;
							const char sOK[] = "+OK\r\n";
							m_pSession->tcp_packet(this, DD_OUTPUT, pd, sOK, sizeof(sOK)-1);
						}

						int newLen = m_buffer.size() - (offset + 1);
						memmove(m_buffer.buffer(), m_buffer.buffer()+offset+1, newLen);
						m_buffer.resize(newLen);
						len = 0;
					} 
				}
			}

		case STATE_RETR_MAIL:
			{
				if (pd == PD_RECEIVE)
				{
					PFStream * pStream = m_object.getStream(0);

					m_buffer.append(buf, len);

					int eof = getEndOfMessage(m_buffer.buffer(), m_buffer.size());
					
					if (eof > 0)
					{
						pStream->write(m_buffer.buffer(), eof);

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

						m_state = STATE_RETR;
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
							
							if ((curTime - m_time) > POP3_WAIT_HEADER_PERIOD)
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

		}

		return res;
	}

	if (len == 0)
	{
		if (m_state != STATE_RETR)
			return FR_PASSTHROUGH;

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
		
eFilterResult POP3Filter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType POP3Filter::getType()
{
	return FT_POP3;
}

PF_FilterCategory POP3Filter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool POP3Filter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	switch (object.getType())
	{
	case OT_POP3_MAIL_INCOMING:
		pd = PD_RECEIVE;
		break;

	default:
		return false;
	}

	const char dot[] = "\r\n.\r\n";
	
	m_pSession->tcpPostObjectStreams(this, pd, object);

	m_pSession->tcp_packet(this, DD_OUTPUT, pd, dot, sizeof(dot)-1);

	return true;
}

