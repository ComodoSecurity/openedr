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
#include "SMTPFilter.h"
#include "ProxySession.h"
#include "auxutil.h"

using namespace ProtocolFilters;

#define SMTP_WAIT_HEADER_PERIOD  15
#define MAX_CMD_BUF_LEN 4096

SMTPFilter::SMTPFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags),
	m_object(OT_SMTP_MAIL_OUTGOING)
{
	m_pSession->getSessionInfo(&m_si);
	m_state = STATE_NONE;
	m_time = nf_time();

	setActive(false);
}

SMTPFilter::~SMTPFilter(void)
{
}

bool SMTPFilter::checkPacket(ePacketDirection pd, const char * buf, int len)
{
	for (int i = 0; i<len; i++)
	{
		if (((unsigned char)buf[i]) < 9) 
			return false;
	}
	return true;
}

int SMTPFilter::getEndOfMessage(const char * buf, int len)
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

eFilterResult SMTPFilter::updateCmdBuffer(const char * buf, int len)
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

inline bool _splitString(const std::string & s, 
				 std::vector<std::string> & parts, 
				 const char * dividers)
{
	std::string part;
	
	parts.clear();

	for (size_t i=0; i<s.length(); i++)
	{
		if (strchr(dividers, s[i]) != 0)
		{
			if (!part.empty())
			{
				parts.push_back(part);
				part.erase();
			}
		} else
		{
			part += s[i];
		}
	}
	if (!part.empty())
	{
		parts.push_back(part);
		part.erase();
	}
	return true;
}


eFilterResult SMTPFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	const char cmdEHLO[] = "EHLO";
	const char cmdHELO[] = "HELO";
	const char cmdDATA[] = "DATA";
	const char cmdSTLS[] = "STLS";
	const char cmdSTARTTLS[] = "STARTTLS";
	const char retSTARTMAIL[] = "354 Start mail input; end with <CRLF>.<CRLF>\r\n"; 
	const char waitCmd[] = "NOOP\r\n";
	
	eFilterResult res = FR_DELETE_ME; // Stop filtering in case of any filtering problems

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	DbgPrint("SMTPFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);
	if (len > 0)
	{
		std::string s(buf, len);
		DbgPrint("Contents:\n%s", s.c_str());
	}
#endif

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

				m_state = STATE_HELO;
				res = FR_PASSTHROUGH;
				m_buffer.reset();

				if (pd == PD_RECEIVE)
					break;
			}

		case STATE_HELO:
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

					if (len >= (int)sizeof(cmdHELO))
					{
						if (strnicmp(buf, cmdHELO, sizeof(cmdHELO)-1) == 0)
						{
							m_state = STATE_HELO_RESP;
							res = FR_PASSTHROUGH;
						} else
						if (strnicmp(buf, cmdEHLO, sizeof(cmdEHLO)-1) == 0)
						{
							m_state = STATE_HELO_RESP;
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

		case STATE_HELO_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						int code = atoi(buf);

						if (code == 250)
						{
							std::string heloResp(buf, len);
							std::vector<std::string> parts;
							
							_splitString(heloResp, parts, "\r\n");
							
							// Delete QQ service MAILCOMPRESS option to avoid filtering issues
							for (size_t i=0; i<parts.size(); i++)
							{
								if (parts[i].find("MAILCOMPRESS") != std::string::npos)
								{
									if ((i == parts.size()-1) && (i > 0))
									{
										parts[i-1].replace(3, 1, " ");
									}

									parts.erase(parts.begin()+i);

									std::string resp;
									for (size_t j=0; j<parts.size(); j++)
									{
										resp += parts[j] + "\r\n";
									}

									m_state = STATE_CMD;

									DbgPrint("Contents:\n%s", resp.c_str());

									m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, resp.c_str(), (int)resp.length());

									return FR_DATA_EATEN;
								}
							}

							m_state = STATE_CMD;
							res = FR_PASSTHROUGH;
						} 
					}
				}
			}
			break;

		case STATE_CMD:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				setActive(true);

				if (pd == PD_SEND)
				{
					res = updateCmdBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;

					buf = m_buffer.buffer();
					len = m_buffer.size();

					res = FR_PASSTHROUGH;

					if (len >= (int)sizeof(cmdSTLS))
					{
						if (strnicmp(buf, cmdSTLS, sizeof(cmdSTLS)-1) == 0)
						{
							m_state = STATE_STLS_RESP;
						} 
					}

					if (m_state == STATE_CMD && len >= (int)sizeof(cmdSTARTTLS))
					{
						if (strnicmp(buf, cmdSTARTTLS, sizeof(cmdSTARTTLS)-1) == 0)
						{
							m_state = STATE_STLS_RESP;
						} 
					}

					if (m_state == STATE_CMD && len >= (int)sizeof(cmdDATA))
					{
						if (strnicmp(buf, cmdDATA, sizeof(cmdDATA)-1) == 0)
						{				
							m_object.clear();
							
							if (!(m_flags & FF_READ_ONLY_OUT))
							{
								res = FR_DATA_EATEN;
								m_state = STATE_DATA;

								m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, retSTARTMAIL, sizeof(retSTARTMAIL)-1);
							} else
							{
								m_state = STATE_DATA_RESP_1;
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
						int code = atoi(buf);

						if (code == 220)
						{
							m_state = STATE_NONE;
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

		case STATE_DATA_RESP_1:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				res = FR_PASSTHROUGH;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						int code = atoi(buf);

						if (code == 354)
						{
							m_state = STATE_DATA;
						} 
					}
				}
			}
			break;

		case STATE_DATA:
			{
				if (pd == PD_SEND)
				{
					PFStream * pStream = m_object.getStream(0);

					m_buffer.append(buf, len);

					int eof = getEndOfMessage(m_buffer.buffer(), m_buffer.size());
					
					if (eof > 0)
					{
						pStream->write(m_buffer.buffer(), eof);
						m_buffer.reset();

						if (m_flags & FF_READ_ONLY_OUT)
						{
//							m_state = STATE_CMD;
							m_object.setReadOnly(true);
						} else
						{
							// Send NOOP to server, to avoid breaking the connection 
//							m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, waitCmd, sizeof(waitCmd)-1);
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

						if (!(m_flags & FF_READ_ONLY_OUT))
						{
							nf_time_t curTime = nf_time();
							
							if ((curTime - m_time) > SMTP_WAIT_HEADER_PERIOD)
							{
								m_time = curTime;

								// Send a fake header to email client periodically
								// to avoid breaking the connection due to server timeout
								m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, waitCmd, sizeof(waitCmd)-1);
							}
						}
					}
				
					return (m_flags & FF_READ_ONLY_OUT)? FR_PASSTHROUGH : FR_DATA_EATEN;
				}

				res = FR_DATA_EATEN;
			}
			break;

		case STATE_DATA_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				res = FR_DATA_EATEN;

				if (pd == PD_RECEIVE)
				{
					if (len > 1)
					{
						int code = atoi(buf);

						if (code == 354)
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
		if (m_state != STATE_CMD)
		{
			return FR_PASSTHROUGH;
		}

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
		
eFilterResult SMTPFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType SMTPFilter::getType()
{
	return FT_SMTP;
}

PF_FilterCategory SMTPFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool SMTPFilter::postObject(PFObjectImpl & object)
{
	if (object.getType() != OT_SMTP_MAIL_OUTGOING)
	{
		return false;
	}

	const char cmdDATA[] = "DATA\r\n";

	m_object = object;
	m_state = STATE_DATA_RESP;
	
	m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, cmdDATA, sizeof(cmdDATA)-1);

	return true;
}

