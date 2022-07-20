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
#include "FTPFilter.h"
#include "ProxySession.h"
#include "auxutil.h"

using namespace ProtocolFilters;

#define MAX_CMD_BUF_LEN 4096
#define MAX_RES_BUF_LEN 4096

FTPFilter::FTPFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags),
	m_cmdObject(OT_FTP_COMMAND),
	m_resObject(OT_FTP_RESPONSE)
{
	m_pSession->getSessionInfo(&m_si);
	m_state = STATE_NONE;
	m_mustUpdateAssociatedSessionData = false;

	char szId[100];
	_snprintf(szId, sizeof(szId), "%llu", pSession->getId());
	m_info[II_SESSIONID] = szId;

	setActive(false);
}

FTPFilter::~FTPFilter(void)
{
}

bool FTPFilter::checkPacket(ePacketDirection pd, const char * buf, int len)
{
	for (int i = 0; i<len; i++)
	{
		if (((unsigned char)buf[i]) < 9) 
			return false;
	}
	return true;
}

eFilterResult FTPFilter::updateCmdBuffer(const char * buf, int len)
{
	if ((m_cmdBuffer.size() + len) > MAX_CMD_BUF_LEN)
		return FR_DELETE_ME;

	m_cmdBuffer.append(buf, len);	

	for (int i=m_cmdBuffer.size()-2; i>=0; i--)
	{
		if (!strchr("\r\n", m_cmdBuffer.buffer()[i]))
			break;

		if (memcmp("\r\n", m_cmdBuffer.buffer() + i, 2) == 0)
		{
			return FR_DATA_EATEN;
		}
	}

	return FR_PASSTHROUGH;
}

eFilterResult FTPFilter::updateResBuffer(const char * buf, int len)
{
	if ((m_resBuffer.size() + len) > MAX_RES_BUF_LEN)
		return FR_DELETE_ME;

	m_resBuffer.append(buf, len);	

	int i;
	char szCode[5] = "    ";

	if (m_resBuffer.size() < 4)
		return FR_PASSTHROUGH;

	if (m_resBuffer.buffer()[3] != ' ' &&
		m_resBuffer.buffer()[3] != '-')
		return FR_DELETE_ME;

	for (i=0; i<3; i++)
	{
		if (!isdigit(m_resBuffer.buffer()[i]))
			return FR_DELETE_ME;
		szCode[i] = m_resBuffer.buffer()[i];
	}

	if (m_resBuffer.buffer()[3] == ' ')
	{
		for (i=4; i<len; i++)
		{
			if (memcmp("\r\n", m_resBuffer.buffer() + i, 2) == 0)
			{
				return FR_DATA_EATEN;
			}
		}
	} else
	if (m_resBuffer.buffer()[3] == '-')
	{
		if (strstr(m_resBuffer.buffer(), szCode) != 0)
		{
			for (int i=m_resBuffer.size()-2; i>=0; i--)
			{
				if (!strchr("\r\n", m_resBuffer.buffer()[i]))
					break;

				if (memcmp("\r\n", m_resBuffer.buffer() + i, 2) == 0)
				{
					return FR_DATA_EATEN;
				}
			}
		}
	} else
		return FR_DELETE_ME;

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



eFilterResult FTPFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	const char cmdUSER[] = "USER";
	const char cmdPASS[] = "PASS";
	const char cmdAUTH_TLS[] = "AUTH TLS";
	const char cmdAUTH_SSL[] = "AUTH SSL";
	const char cmdPORT[] = "PORT";
	const char cmdPASV[] = "PASV";
	const char cmdTYPE[] = "TYPE";
	const char cmdMODE[] = "MODE";
	const char cmdSTRU[] = "STRU";
	const char cmdREST[] = "REST";
	const char cmdEPSV[] = "EPSV";
	const char cmdEPRT[] = "EPRT";
	const char cmdOPEN[] = "OPEN";
	const char cmdOPTS[] = "OPTS";
	eFilterResult res = FR_DELETE_ME;

	DbgPrint("FTPFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
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

				if (pd == PD_RECEIVE)
				{
					res = updateResBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;
					m_resBuffer.reset();
					res = FR_PASSTHROUGH;
					m_state = STATE_USER;
					m_mustUpdateAssociatedSessionData = false;
				} else
				{
					return FR_DELETE_ME;
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

					buf = m_cmdBuffer.buffer();
					len = m_cmdBuffer.size();

					if (len >= (int)sizeof(cmdOPEN))
					{
						if (strnicmp(buf, cmdOPEN, sizeof(cmdOPEN)-1) == 0)
						{
							m_cmdBuffer.reset();
							return FR_PASSTHROUGH;
						}
					}
					if (len >= (int)sizeof(cmdOPTS))
					{
						if (strnicmp(buf, cmdOPTS, sizeof(cmdOPTS)-1) == 0)
						{
							m_cmdBuffer.reset();
							return FR_PASSTHROUGH;
						}
					}

					res = FR_DELETE_ME;

					if (len >= (int)sizeof(cmdUSER))
					{
						if (strnicmp(buf, cmdUSER, sizeof(cmdUSER)-1) == 0)
						{
							m_state = STATE_USER_RESP;
							res = FR_PASSTHROUGH;

							m_cmdObject.clear();

							PFStream * pStream = m_cmdObject.getStream(0);

							pStream->write(m_cmdBuffer.buffer(), m_cmdBuffer.size());

							if (m_flags & FF_READ_ONLY_OUT)
							{
								m_cmdObject.setReadOnly(true);
								res = FR_PASSTHROUGH;
							} else
							{
								res = FR_DATA_EATEN;
							}

							// Filter
							try {
								pHandler->dataAvailable(m_pSession->getId(), &m_cmdObject);
							} catch (...)
							{
							}

						} else
						if (strnicmp(buf, cmdAUTH_TLS, sizeof(cmdAUTH_TLS)-1) == 0 ||
							strnicmp(buf, cmdAUTH_SSL, sizeof(cmdAUTH_SSL)-1) == 0) 
						{
							m_state = STATE_AUTH_RESP;
							res = FR_PASSTHROUGH;
						} 
					}

					m_cmdBuffer.reset();
				} else
				{
					res = FR_PASSTHROUGH;
				}
			}
			break;

		case STATE_USER_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_RECEIVE)
				{
					res = updateResBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;
					m_resBuffer.reset();
					res = FR_PASSTHROUGH;
					m_state = STATE_CMD;
				}
			}
			break;

		case STATE_AUTH_RESP:
			{
				if (!checkPacket(pd, buf, len))
					return FR_DELETE_ME;

				if (pd == PD_RECEIVE)
				{
					res = updateResBuffer(buf, len);
					if (res != FR_DATA_EATEN)
						return res;

					res = FR_PASSTHROUGH;
					m_state = STATE_USER;

					if (strchr("23", m_resBuffer.buffer()[0]))
					{
						res = FR_DATA_EATEN;
						m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);

						// Prepare for SSL handshake if TLS filtering flag is turned on
						if (m_flags & FF_SSL_TLS)
						{
							m_pSession->addFilter(FT_SSL, m_flags, OT_FIRST);
							m_state = STATE_CMD;
						}
					}
					m_resBuffer.reset();
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
					if (res == FR_DELETE_ME)
						return res;
					else
					if (res == FR_PASSTHROUGH)
						return (m_flags & FF_READ_ONLY_OUT)? FR_PASSTHROUGH : FR_DATA_EATEN;

					std::vector<std::string> parts;
					{
						std::string requestStr(m_cmdBuffer.buffer(), m_cmdBuffer.size());
						_splitString(requestStr, parts, "\r\n");
					}

					for (size_t i=0; i<parts.size(); i++)
					{
						std::string cmdPart = parts[i] + "\r\n";

						buf = cmdPart.c_str();
						len = (int)cmdPart.length();

						DbgPrint("FTPFilter::tcp_packet() id=%llu command=%s",
							m_pSession->getId(), buf);

						if (strnicmp(buf, cmdPASV, sizeof(cmdPASV)-1) == 0 ||
							strnicmp(buf, cmdEPSV, sizeof(cmdEPSV)-1) == 0)
						{
							m_mustUpdateAssociatedSessionData = true;
						} else
						if (strnicmp(buf, cmdTYPE, sizeof(cmdTYPE)-1) == 0)
						{
							m_info[II_TYPE] = buf[5];
						} else
						if (strnicmp(buf, cmdMODE, sizeof(cmdMODE)-1) == 0)
						{
							if (buf[5] != 'S')
							{
								const char sERR[] = "504 Unsupported transfer mode\r\n";
								m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, sERR, sizeof(sERR)-1);
								m_cmdBuffer.reset();
								return FR_DATA_EATEN;
							}
						} else
						if (strnicmp(buf, cmdSTRU, sizeof(cmdSTRU)-1) == 0)
						{
							const char sERR[] = "504 Unsupported command\r\n";
							m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, sERR, sizeof(sERR)-1);
							m_cmdBuffer.reset();
							return FR_DATA_EATEN;
						} else
						if (strnicmp(buf, cmdREST, sizeof(cmdREST)-1) == 0)
						{
							std::string cmd = buf + 5;

							if (cmd.length() > 2)
								cmd.erase(cmd.length() - 2);

							m_info[II_REST] = cmd;
						} else
						{
							std::string cmd = buf;

							if (cmd.length() > 2)
								cmd.erase(cmd.length() - 2);

							m_info[II_COMMAND] = cmd;

							std::string info = "COMMAND: " + m_info[II_COMMAND] + "\r\n";

							if (!m_info[II_TYPE].empty())
								info += "TYPE: " + m_info[II_TYPE] + "\r\n";

							if (!m_info[II_REST].empty())
							{
								info += "REST: " + m_info[II_REST] + "\r\n";
								m_info[II_REST] = ""; // Cleanup REST immediately after transfer command
							}

							info += "SESSIONID: " + m_info[II_SESSIONID] + "\r\n";

							info += "\r\n";

							m_pSession->setAssociatedSessionInfo(info);
						}

						m_cmdObject.clear();

						PFStream * pStream = m_cmdObject.getStream(0);

						pStream->write(buf, len);

						if (m_flags & FF_READ_ONLY_OUT)
						{
							m_cmdObject.setReadOnly(true);
							res = FR_PASSTHROUGH;
						} else
						{
							res = FR_DATA_EATEN;
						}

						if (m_flags & FF_READ_ONLY_OUT)
						{
							updateAssociatedSessionData(m_cmdObject);
						}

						// Filter
						try {
							pHandler->dataAvailable(m_pSession->getId(), &m_cmdObject);
						} catch (...)
						{
						}
					}

					m_cmdBuffer.reset();
				} else
				{
					res = updateResBuffer(buf, len);
					if (res == FR_DELETE_ME)
						return res;
					else
					if (res == FR_PASSTHROUGH)
						return (m_flags & FF_READ_ONLY_IN)? FR_PASSTHROUGH : FR_DATA_EATEN;

					DbgPrint("FTPFilter::tcp_packet() id=%llu response=%s",
						m_pSession->getId(), m_resBuffer.buffer());

					m_resObject.clear();

					PFStream * pStream = m_resObject.getStream(0);

					pStream->write(m_resBuffer.buffer(), m_resBuffer.size());

					if (m_flags & FF_READ_ONLY_IN)
					{
						m_resObject.setReadOnly(true);

						if (m_mustUpdateAssociatedSessionData)
						{
							updateAssociatedSessionData(m_resObject);
							m_mustUpdateAssociatedSessionData = false;
						}

						res = FR_PASSTHROUGH;
					} else
					{
						res = FR_DATA_EATEN;
					}

					// Filter
					try {
						pHandler->dataAvailable(m_pSession->getId(), &m_resObject);
					} catch (...)
					{
					}

					m_resBuffer.reset();
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
		
eFilterResult FTPFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType FTPFilter::getType()
{
	return FT_FTP;
}

PF_FilterCategory FTPFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool FTPFilter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	switch (object.getType())
	{
	case OT_FTP_COMMAND:
		pd = PD_SEND;
		updateAssociatedSessionData(object);
		break;
	case OT_FTP_RESPONSE:
		pd = PD_RECEIVE;
		if (m_mustUpdateAssociatedSessionData)
		{
			updateAssociatedSessionData(object);
			m_mustUpdateAssociatedSessionData = false;
		}
		break;
	default:
		return false;
	}

	m_pSession->tcpPostObjectStreams(this, pd, object);

	return true;
}

void FTPFilter::updateAssociatedSessionData(PFObjectImpl & object)
{
	IOB buffer;
	std::string sAddr;
	PFStream * pStream;

	pStream = object.getStream(0);
	if (!pStream)
	{
		DbgPrint("FTPFilter::updateAssociatedSessionData() id=%llu ot=%d no data stream", 
			m_pSession->getId(), object.getType());
		return;
	}
	
	pStream->seek(0, FILE_BEGIN);

	if (!buffer.append(NULL, (unsigned long)pStream->size(), false))
	{
		DbgPrint("FTPFilter::updateAssociatedSessionData() id=%llu ot=%d len=%d append failed", 
			m_pSession->getId(), object.getType(), pStream->size());
		return;
	}

	if (pStream->read(buffer.buffer(), buffer.size()) != buffer.size())
	{
		DbgPrint("FTPFilter::updateAssociatedSessionData() id=%llu ot=%d unable to read stream", m_pSession->getId(), object.getType());
		return;
	}

	std::vector<std::string> parts;
	{
		std::string requestStr(buffer.buffer(), buffer.size());
		_splitString(requestStr, parts, "\r\n");
	}

	for (size_t i=0; i<parts.size(); i++)
	{
		std::string cmdPart = parts[i] + "\r\n";

		char * buf = (char*)cmdPart.c_str();
		int len = (int)cmdPart.length();

		if (object.getType() == OT_FTP_COMMAND)
		{
			const char cmdPORT[] = "PORT ";
			const char cmdEPRT[] = "EPRT ";
			if (len >= (int)(sizeof(cmdPORT) + 11))
			{
				if (strnicmp(buf, cmdPORT, sizeof(cmdPORT)-1) == 0)
				{
					std::vector<std::string> vAddr;
					std::string sAddrPart;

					for (int i=5; i<len; i++)
					{
						if (strchr("\r\n", buf[i]))
							break;
						if ((buf[i] == ',') && !sAddrPart.empty())
						{
							vAddr.push_back(sAddrPart);
							sAddrPart = "";
						} else
						{
							sAddrPart += buf[i];
						}
					}

					if (!sAddrPart.empty())
					{
						vAddr.push_back(sAddrPart);
					}

					if (vAddr.size() < 6)
					{
						DbgPrint("FTPFilter::updateAssociatedSessionData() id=%llu invalid PORT argument", m_pSession->getId());
						continue;
					}
					
					char szAddress[100];

					_snprintf(szAddress, sizeof(szAddress),
						"%s.%s.%s.%s:%d",
						vAddr[0].c_str(),
						vAddr[1].c_str(),
						vAddr[2].c_str(),
						vAddr[3].c_str(),
						256 * atoi(vAddr[4].c_str()) + atoi(vAddr[5].c_str()));

					m_pSession->setAssociatedLocalEndpointStr(szAddress);
					m_pSession->setAssociatedFilterFlags((PF_FilterFlags)m_flags);
					m_pSession->setAssociatedFilterType(FT_FTP_DATA);
				} else
				if (strnicmp(buf, cmdEPRT, sizeof(cmdEPRT)-1) == 0)
				{
					std::vector<std::string> vAddr;
					std::string sAddrPart;

					for (int i=5; i<len; i++)
					{
						if (strchr("\r\n", buf[i]))
							break;
						if (buf[i] == '|')
						{
							if (!sAddrPart.empty())
							{
								vAddr.push_back(sAddrPart);
								sAddrPart = "";
							}
						} else
						{
							sAddrPart += buf[i];
						}
					}

					if (!sAddrPart.empty())
					{
						vAddr.push_back(sAddrPart);
					}

					if (vAddr.size() < 3)
					{
						DbgPrint("FTPFilter::updateAssociatedSessionData() id=%llu invalid EPRT argument", m_pSession->getId());
						continue;
					}
					
					char szAddress[100];

					if (vAddr[0] == "1")
					{
						_snprintf(szAddress, sizeof(szAddress),
							"%s:%s",
							vAddr[1].c_str(),
							vAddr[2].c_str());
					} else
					{
						_snprintf(szAddress, sizeof(szAddress),
							"[%s]:%s",
							vAddr[1].c_str(),
							vAddr[2].c_str());
					}
					m_pSession->setAssociatedLocalEndpointStr(szAddress);
					m_pSession->setAssociatedFilterFlags((PF_FilterFlags)m_flags);
					m_pSession->setAssociatedFilterType(FT_FTP_DATA);
				} 
			}
		} else
		if (object.getType() == OT_FTP_RESPONSE)
		{
			std::vector<std::string> vAddr;
			std::string sAddrPart;
			bool bracesOpen = false;
			const char resp150[] = "150 ";
			const char acceptedFrom[] = "accepted from ";

			if (len > (int)sizeof(resp150) &&
				strncmp(buf, resp150, sizeof(resp150)-1) == 0)
			{
				DbgPrint("Response 150");

				char * p = strstr(buf, acceptedFrom);
				if (p)
				{
					p += sizeof(acceptedFrom)-1;

					DbgPrint("Found accepted from: %s", p);

					while (*p && *p == ' ')
						p++;

					for (; *p && *p != ';'; p++)
					{
						sAddrPart += *p;
					}
					
					m_pSession->setAssociatedLocalEndpointStr(sAddrPart);
					m_pSession->setAssociatedFilterFlags((PF_FilterFlags)m_flags);
					m_pSession->setAssociatedFilterType(FT_FTP_DATA);
					
					return;
				}
			}

			for (int i=4; i<len; i++)
			{
				if (!bracesOpen)
				{
					if (buf[i] != '(')
						continue;
					i++;
					bracesOpen = true;
				}

				if (strchr(")\r\n", buf[i]))
				{
					break;
				}
				
				if (strchr(",|", buf[i]))
				{
					if (!sAddrPart.empty())
					{
						vAddr.push_back(sAddrPart);
						sAddrPart = "";
					}
				} else
				{
					sAddrPart += buf[i];
				}
			}

			if (!sAddrPart.empty())
			{
				vAddr.push_back(sAddrPart);
			}

			char szAddress[100];

			if (vAddr.size() == 6)
			{
				_snprintf(szAddress, sizeof(szAddress),
					"%s.%s.%s.%s:%d",
					vAddr[0].c_str(),
					vAddr[1].c_str(),
					vAddr[2].c_str(),
					vAddr[3].c_str(),
					256 * atoi(vAddr[4].c_str()) + atoi(vAddr[5].c_str()));
			} else
			if (vAddr.size() == 1)
			{
				_snprintf(szAddress, sizeof(szAddress),
					":%s",
					vAddr[0].c_str());
			} else
			{
				DbgPrint("FTPFilter::updateAssociatedSessionData() id=%llu invalid PASV address", m_pSession->getId());
				continue;
			}

			m_pSession->setAssociatedRemoteEndpointStr(szAddress);
			m_pSession->setAssociatedFilterFlags((PF_FilterFlags)m_flags);
			m_pSession->setAssociatedFilterType(FT_FTP_DATA);
		}
	}
}
