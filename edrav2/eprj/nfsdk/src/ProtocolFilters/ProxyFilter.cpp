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
#include "ProxyFilter.h"
#include "ProxySession.h"
#include "auxutil.h"
#include "socksdefs.h"

using namespace ProtocolFilters;

#define MAX_CMD_BUF_LEN 2048

ProxyFilter::ProxyFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_flags(flags),
	m_httpParser(OT_HTTP_RESPONSE)
{
	m_state = STATE_NONE;
	m_proxyType = PT_UNKNOWN;
	m_readOnly = false;
	m_pSession->getSessionInfo(&m_si);
	setActive(false);
}

ProxyFilter::~ProxyFilter(void)
{
}

bool ProxyFilter::isHTTPConnectRequest(const char * buf, int len, std::string & connectTo)
{
	const char httpVersion[] = "HTTP/1.";
	const char httpVersionL[] = "http/1.";
	const char httpConnect[] = "CONNECT ";
	int i, j;
	int state;

	connectTo = "";

	if (len < 19)
		return false;

	if (strnicmp(buf, httpConnect, sizeof(httpConnect)-1) != 0)
		return false;

	j = 0;
	state = 0;

	for (i = 0; i<len; i++)
	{
		if (((unsigned char)buf[i]) < 9) 
			return false;

		if (buf[i] == ' ')
		{
			if ((i > 0) && (buf[i-1] != ' '))
			{
				state++;
			}
			continue;
		}

		if (state == 1)
		{
			connectTo += buf[i];
		}

		if (state == 2)
		{
			if (httpVersion[j] == '\0')
			{
				state++;
				continue;
			}
			
			if (buf[i] == httpVersion[j] ||
				buf[i] == httpVersionL[j])
			{
				j++;
				continue;
			} else
			{
				return false;
			}
		} else
		if (state == 3)
		{
			if (buf[i] == '\n' ||
				buf[i] == '\r')
			{
				return true;
			}
		}

	}

	return false;
}

eFilterResult ProxyFilter::updateHttpsCmdBuffer(const char * buf, int len)
{
	if ((m_buffer.size() + len) > (MAX_CMD_BUF_LEN * 10))
		return FR_DELETE_ME;

	m_buffer.append(buf, len);	

	const char httpConnect[] = "CONNECT ";

	if (m_buffer.size() >= sizeof(httpConnect)-1)
	{
		if (strnicmp(m_buffer.buffer(), httpConnect, sizeof(httpConnect)-1) != 0)
		{
			return FR_DELETE_ME;
		}
		m_proxyType = PT_HTTPS;
	} else
	{
		if (strnicmp(m_buffer.buffer(), httpConnect, m_buffer.size()) != 0)
		{
			return FR_DELETE_ME;
		}
		m_proxyType = PT_HTTPS;
		return FR_PASSTHROUGH;
	}


	int i, n = 0;
	for (i=0; i<(int)m_buffer.size(); i++)
	{
		if (m_buffer.buffer()[i] == '\r')
			continue;
		if (m_buffer.buffer()[i] == '\n')
		{
			n++;
			if (n == 2)
			{
				return FR_DATA_EATEN;
			}
			continue;
		}
		n = 0;
	}

	return FR_PASSTHROUGH;
}

eFilterResult ProxyFilter::updateSocksCmdBuffer(const char * buf, int len)
{
	if ((m_buffer.size() + len) > MAX_CMD_BUF_LEN)
		return FR_DELETE_ME;

	m_buffer.append(buf, len);	
	len = m_buffer.size();

	if (len < (int)sizeof(SOCKS5_AUTH_REQUEST))
	{
		return FR_PASSTHROUGH;
	}

	SOCKS45_REQUEST * req = (SOCKS45_REQUEST *)m_buffer.buffer();
			
	char socksVersion = req->socks5_auth_req.version;

	if (m_proxyType == PT_SOCKS5)
	{
		socksVersion = SOCKS_5;
	}
			
	switch (socksVersion)
	{
	case SOCKS_4:
		{
			m_proxyType = PT_SOCKS4;

			switch (req->socks4_req.command)
			{
			case S4C_CONNECT:
			case S4C_BIND:
				{
					if (len < (int)sizeof(SOCKS4_REQUEST))
						return FR_PASSTHROUGH;

					// Check User Id
					int i = (int)sizeof(SOCKS4_REQUEST)-1;
					for(;;)
					{
						if (i >= len)
							return FR_PASSTHROUGH;
						if (m_buffer.buffer()[i] == 0)
							break;
						i++;
					}

					// Detect SOCKS4A
					if ((req->socks4_req.ip & 0xffffff) == 0)
					{
						i++;

						for(;;)
						{
							if (i >= len)
								return FR_PASSTHROUGH;
							if (m_buffer.buffer()[i] == 0)
								break;
							i++;
						}
					}

					return FR_DATA_EATEN;
				}
			default:
				return FR_DELETE_ME;
			}
		}
		break;
	
	case SOCKS_5:
		{
			if (m_proxyType == PT_UNKNOWN)
			{
				m_socks5State = STATE_SOCKS5_AUTH;
				m_proxyType = PT_SOCKS5;
			}

			switch (m_socks5State)
			{
			case STATE_SOCKS5_AUTH:
				{
					if (len < ((int)sizeof(SOCKS5_AUTH_REQUEST) + req->socks5_auth_req.nmethods - 1))
					{
						return FR_PASSTHROUGH;
					} else
					{
						if (len > ((int)sizeof(SOCKS5_AUTH_REQUEST) + req->socks5_auth_req.nmethods - 1))
						{
							return FR_DELETE_ME;
						} else
						{
							return FR_DATA_EATEN;
						}
					}
				}
				break;
			case STATE_SOCKS5_AUTH_NEG:
				{
					if (m_socks5AuthMethod == S5AM_UNPW)
					{
						if (len < 5)
							return FR_PASSTHROUGH;

						int i = 2 + m_buffer.buffer()[1];
						
						if (i >= len)
							return FR_PASSTHROUGH;
						
						i += m_buffer.buffer()[i];
						
						if (i >= len)
							return FR_PASSTHROUGH;
						
						return FR_DATA_EATEN;						
					} else
					{
						return FR_DELETE_ME;
					}
				}
				break;
			case STATE_SOCKS5_REQUEST:
				break;
			}

			int cmd = req->socks5_req.command;

			switch (cmd)
			{
			case S5C_CONNECT:
			case S5C_BIND:
			case S5C_UDP_ASSOCIATE:
				{
					if (req->socks5_req.reserved != 0)
						return FR_DELETE_ME;
					if (len < (int)sizeof(SOCKS5_REQUEST))
						return FR_PASSTHROUGH;

					switch (req->socks5_req.address_type)
					{
					case SOCKS5_ADDR_IPV4:
						{
							if (len < (int)sizeof(SOCKS5_REQUEST) + 5)
								return FR_PASSTHROUGH;
						}
						break;
					case SOCKS5_ADDR_DOMAIN:
						{
							if (len < (int)sizeof(SOCKS5_REQUEST) + *req->socks5_req.address + 2)
								return FR_PASSTHROUGH;
						}
						break;
					case SOCKS5_ADDR_IPV6:
						{
							if (len < (int)sizeof(SOCKS5_REQUEST) + 17)
								return FR_PASSTHROUGH;
						}
						break;
					default:
						return FR_DELETE_ME;
					}
					return FR_DATA_EATEN;
				}
				break;

			default:
				return FR_DELETE_ME;
			}
		}
		break;
	
	default:
		return FR_DELETE_ME;
	};

	return FR_PASSTHROUGH;
}

eFilterResult ProxyFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	eFilterResult res = FR_DELETE_ME;

	if (dd != DD_INPUT)
		return FR_PASSTHROUGH;

	DbgPrint("ProxyFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);
	
	ePacketDirection pdReal = pd;

	if (m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN)
	{
		pd = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
	}

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (!pHandler)
		return FR_PASSTHROUGH;

	if (len == 0)
	{
		return FR_DELETE_ME;
	}

	if (pd == PD_SEND)
	{
		if (m_state == STATE_RESP)
		{
			m_buffer.reset();
			m_readOnly = false;
			m_state = STATE_NONE;
		}
	}

	switch (m_state)
	{
	case STATE_NONE:
		{
			if ((m_si.tcpConnInfo.direction == NFAPI_NS NF_D_OUT && pd == PD_RECEIVE) ||
				(m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN && pd == PD_RECEIVE))
			{
				return FR_DELETE_ME;
			}

			if (m_proxyType == PT_HTTPS)
				m_proxyType = PT_UNKNOWN;

			if (m_proxyType == PT_UNKNOWN ||
				m_proxyType == PT_SOCKS4 ||
				m_proxyType == PT_SOCKS5)
			{
				res = updateSocksCmdBuffer(buf, len);
				if (res == FR_DELETE_ME)
				{
					if (m_proxyType == PT_UNKNOWN)
					{
						m_readOnly = false;
						m_buffer.reset();
					} else
					{
						return res;
					}
				} else
				if (res == FR_PASSTHROUGH)
				{
					if (m_readOnly)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, buf, len);
					} else
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, m_buffer.buffer(), m_buffer.size());
						m_readOnly = true;
					}
					return FR_DATA_EATEN;	
				} else
				{
					setActive(true);

					if (!m_readOnly)
						m_readOnly = (m_flags & FF_READ_ONLY_OUT) != 0;

					tPF_ObjectType objectType = OT_NULL;

					if (m_proxyType == PT_SOCKS4)
					{
						objectType = OT_SOCKS4_REQUEST;
					} else
					{
						switch (m_socks5State)
						{
						case STATE_SOCKS5_AUTH:
							objectType = OT_SOCKS5_AUTH_REQUEST;
							break;
						case STATE_SOCKS5_AUTH_NEG:
							objectType = OT_SOCKS5_AUTH_UNPW;
							break;
						case STATE_SOCKS5_REQUEST:
							objectType = OT_SOCKS5_REQUEST;
							break;
						default:
							objectType = OT_SOCKS5_REQUEST;
							break;
						}
					}

					PFObjectImpl obj(objectType);

					if (m_readOnly)
					{
						obj.setReadOnly(true);
					}

					PFStream * pStream = obj.getStream(0);
					if (pStream)
					{
						pStream->reset();
						pStream->write(m_buffer.buffer(), m_buffer.size());

						if (m_readOnly)
						{
							postObject(obj);
							m_state = STATE_RESP;
						}

						try {
							pHandler->dataAvailable(m_pSession->getId(), &obj);
						} catch (...)
						{
						}
					}

					m_buffer.reset();

					return FR_DATA_EATEN;
				}
			}

			if (m_proxyType == PT_UNKNOWN ||
				m_proxyType == PT_HTTPS)
			{
				res = updateHttpsCmdBuffer(buf, len);
				if (res == FR_DELETE_ME)
				{
					if (m_proxyType == PT_UNKNOWN)
					{
						m_readOnly = false;
						m_buffer.reset();
					}
					return res;
				} else
				if (res == FR_PASSTHROUGH)
				{
					if (m_readOnly)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, buf, len);
					} else
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, m_buffer.buffer(), m_buffer.size());
						m_readOnly = true;
					}
					return FR_DATA_EATEN;	
				} else
				{
					if (!m_readOnly)
						m_readOnly = (m_flags & FF_READ_ONLY_OUT) != 0;

					std::string connectTo;

					if (!isHTTPConnectRequest(m_buffer.buffer(), m_buffer.size(), connectTo))
					{
						m_buffer.reset();
						return FR_DELETE_ME;
					}

					PFObjectImpl obj(OT_HTTPS_PROXY_REQUEST);

					if (m_readOnly)
					{
						obj.setReadOnly(true);
					}

					PFStream * pStream = obj.getStream(0);
					if (pStream)
					{
						pStream->reset();
						pStream->write(m_buffer.buffer(), m_buffer.size());

						if (m_readOnly)
						{
							postObject(obj);
							//m_pSession->tcp_packet(this, DD_OUTPUT, pd, m_buffer.buffer(), m_buffer.size());
							m_state = STATE_RESP;
						}

						try {
							pHandler->dataAvailable(m_pSession->getId(), &obj);
						} catch (...)
						{
						}
					}
					
					m_buffer.reset();
					
					return FR_DATA_EATEN;
				}
			}

			return FR_PASSTHROUGH;
		} 
		break;

	case STATE_RESP:
		{
			if (pd == PD_RECEIVE)
			{
				if (m_proxyType == PT_SOCKS5)
				{
					switch (m_socks5State)
					{
					case STATE_SOCKS5_AUTH:
						{
							if (len == 2)
							{
								SOCK5_AUTH_RESPONSE * r = (SOCK5_AUTH_RESPONSE*)buf;

								if (r->method == S5AM_NONE)
								{
									m_socks5State = STATE_SOCKS5_REQUEST;
									break;
								} else
								if (r->method == S5AM_UNPW)
								{
									m_socks5AuthMethod = r->method;
									m_socks5State = STATE_SOCKS5_AUTH_NEG;
								} else
								{
									return FR_DELETE_ME;
								}
							} else
							{
								return FR_DELETE_ME;
							}
						}
						break;

					case STATE_SOCKS5_AUTH_NEG:
						{
							if (len == 2)
							{
								m_socks5State = STATE_SOCKS5_REQUEST;
								break;
							} else
							{
								return FR_DELETE_ME;
							}
						}
						break;

					case STATE_SOCKS5_REQUEST:
						m_socks5State = STATE_SOCKS5_AUTH;
						m_proxyType = PT_UNKNOWN;
						m_state = STATE_NONE;
						break;
					}
				} else
				{
					m_httpParser.inputPacket(buf, len);

					if (m_httpParser.getState() == HTTPParser::STATE_FINISHED || 
						m_httpParser.getState() == HTTPParser::STATE_FINISHED_CONTINUE)
					{
						m_httpParser.reset(false);
						m_proxyType = PT_UNKNOWN;
						m_state = STATE_NONE;
					}
				}

				m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, buf, len);
				return FR_DATA_EATEN;
			}
		}
		break;
	}

	return FR_PASSTHROUGH;
}
		
eFilterResult ProxyFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType ProxyFilter::getType()
{
	return FT_PROXY;
}

PF_FilterCategory ProxyFilter::getCategory()
{
	return FC_PREPROCESSOR;
}

bool ProxyFilter::postObject(PFObjectImpl & object)
{
	DbgPrint("ProxyFilter::postObject() id=%llu ot=%d", m_pSession->getId(), object.getType());

	if ((object.getType() - FT_PROXY) < 0 || (object.getType() - FT_PROXY) > FT_STEP)
	{
		return false;
	}
			
	IOB buffer;
	std::string connectTo;
	std::string bindTo;
	PFStream * pStream;

	pStream = object.getStream(0);
	if (!pStream)
	{
		DbgPrint("ProxyFilter::postObject() id=%llu ot=%d no data stream", 
			m_pSession->getId(), object.getType());
		return false;
	}
	
	pStream->seek(0, FILE_BEGIN);

	if (!buffer.append(NULL, (unsigned long)pStream->size(), false))
	{
		DbgPrint("ProxyFilter::postObject() id=%llu ot=%d len=%d append failed", 
			m_pSession->getId(), object.getType(), pStream->size());
		return false;
	}

	if (pStream->read(buffer.buffer(), buffer.size()) != buffer.size())
	{
		DbgPrint("ProxyFilter::postObject() id=%llu ot=%d unable to read stream", m_pSession->getId(), object.getType());
		return false;
	}

	int len = buffer.size();

	if (m_proxyType == PT_HTTPS)
	{
		if (!isHTTPConnectRequest(buffer.buffer(), buffer.size(), connectTo))
		{
			DbgPrint("ProxyFilter::postObject() id=%llu ot=%d invalid request", m_pSession->getId(), object.getType());
		}
	} else
	if (m_proxyType == PT_SOCKS4 ||
		m_proxyType == PT_SOCKS5)
	{
		if (len < (int)sizeof(SOCKS5_AUTH_REQUEST))
		{
			DbgPrint("ProxyFilter::postObject() id=%llu ot=%d invalid request", m_pSession->getId(), object.getType());
			return false;
		}

		SOCKS45_REQUEST * req = (SOCKS45_REQUEST *)buffer.buffer();

		char socksVersion = req->socks5_auth_req.version;

		if (m_proxyType == PT_SOCKS5)
		{
			socksVersion = SOCKS_5;
		}

		switch (socksVersion)
		{
		case SOCKS_4:
			{
				switch (req->socks4_req.command)
				{
				case S4C_CONNECT:
					{
						std::string domain;

						if (len < (int)sizeof(SOCKS4_REQUEST))
							return false;

						// Check User Id
						int i = (int)sizeof(SOCKS4_REQUEST)-1;
						for(;;)
						{
							if (i >= len)
								break;
							if (buffer.buffer()[i] == 0)
							{
								buffer.resize(i+1);
								break;
							}
							i++;
						}

						// Detect SOCKS4A
						if ((req->socks4_req.ip & 0xffffff) == 0)
						{
							i++;

							for(;;)
							{
								if (i >= len)
									break;
								if (buffer.buffer()[i] == 0)
									break;
								domain += buffer.buffer()[i];
								i++;
							}
						}

						if (domain.empty())
						{
							sockaddr_in sa;
							memset(&sa, 0, sizeof(sa));
							sa.sin_family = AF_INET;
#ifdef WIN32
							sa.sin_addr.S_un.S_addr = req->socks4_req.ip;
#else
							sa.sin_addr.s_addr = req->socks4_req.ip;
#endif
							sa.sin_port = req->socks4_req.port;
							connectTo = aux_getAddressString((sockaddr*)&sa);
						} else
						{
							connectTo = domain;
							char sPort[10];
							_snprintf(sPort, sizeof(sPort), ":%d", htons(req->socks4_req.port));
							connectTo += sPort;
						}

						DbgPrint("SOCKS4 proxy connect to %s", connectTo.c_str());
					}
					break;
				case S4C_BIND:
					{
						std::string domain;

						if (len < (int)sizeof(SOCKS4_REQUEST))
							return false;

						// Check User Id
						int i = (int)sizeof(SOCKS4_REQUEST)-1;
						for(;;)
						{
							if (i >= len)
								break;
							if (buffer.buffer()[i] == 0)
							{
								buffer.resize(i+1);
								break;
							}
							i++;
						}

						// Detect SOCKS4A
						if ((req->socks4_req.ip & 0xffffff) == 0)
						{
							i++;

							for(;;)
							{
								if (i >= len)
									break;
								if (buffer.buffer()[i] == 0)
									break;
								domain += buffer.buffer()[i];
								i++;
							}
						}

						if (domain.empty())
						{
							sockaddr_in sa;
							memset(&sa, 0, sizeof(sa));
							sa.sin_family = AF_INET;
#ifdef WIN32
							sa.sin_addr.S_un.S_addr = req->socks4_req.ip;
#else
							sa.sin_addr.s_addr = req->socks4_req.ip;
#endif
							sa.sin_port = req->socks4_req.port;
							bindTo = aux_getAddressString((sockaddr*)&sa);
						} else
						{
							connectTo = domain;
							char sPort[10];
							_snprintf(sPort, sizeof(sPort), ":%d", htons(req->socks4_req.port));
							bindTo += sPort;
						}

						DbgPrint("SOCKS4 proxy bind to %s", bindTo.c_str());
					}
					break;
				}
			}
			break;
		case SOCKS_5:
			{
				if (m_socks5State == STATE_SOCKS5_REQUEST)
				{
					switch (req->socks5_req.command)
					{
					case S5C_CONNECT:
						{
							if (req->socks5_req.reserved == 0 &&
								len >= (int)sizeof(SOCKS5_REQUEST))
							{
								switch (req->socks5_req.address_type)
								{
								case SOCKS5_ADDR_IPV4:
									{
										if (len < (int)sizeof(SOCKS5_REQUEST) + 5)
											break;

										sockaddr_in sa;
										memset(&sa, 0, sizeof(sa));
										sa.sin_family = AF_INET;
#ifdef WIN32
										sa.sin_addr.S_un.S_addr = *(unsigned int*)req->socks5_req.address;
#else
										sa.sin_addr.s_addr = *(unsigned int*)req->socks5_req.address;
#endif
										sa.sin_port = *(unsigned short*)(req->socks5_req.address + 4);
										connectTo = aux_getAddressString((sockaddr*)&sa);
										DbgPrint("SOCKS5 proxy connect to %s", connectTo.c_str());
									}
									break;
								case SOCKS5_ADDR_DOMAIN:
									{
										if (len < (int)sizeof(SOCKS5_REQUEST) + *req->socks5_req.address + 2)
											break;
										connectTo = std::string(req->socks5_req.address + 1, 
											req->socks5_req.address + *req->socks5_req.address + 1);
										unsigned short port = *(unsigned short*)(req->socks5_req.address + *req->socks5_req.address + 1);
										char sPort[10];
										_snprintf(sPort, sizeof(sPort), ":%d", htons(port));
										connectTo += sPort;
										DbgPrint("SOCKS5 proxy connect to %s", connectTo.c_str());
									}
									break;
								case SOCKS5_ADDR_IPV6:
									{
										if (len < (int)sizeof(SOCKS5_REQUEST) + 17)
											break;

										sockaddr_in6 sa;
										memset(&sa, 0, sizeof(sa));
										sa.sin6_family = AF_INET6;
										memcpy(&sa.sin6_addr, req->socks5_req.address, 16);
										sa.sin6_port = *(unsigned short*)(req->socks5_req.address + 16);
										connectTo = aux_getAddressString((sockaddr*)&sa);
										DbgPrint("SOCKS5 proxy connect to %s", connectTo.c_str());
									}
									break;
								default:
									break;
								}
							}
						}
						break;
					case S5C_BIND:
						{
							if (req->socks5_req.reserved == 0 &&
								len >= (int)sizeof(SOCKS5_REQUEST))
							{
								switch (req->socks5_req.address_type)
								{
								case SOCKS5_ADDR_IPV4:
									{
										if (len < (int)sizeof(SOCKS5_REQUEST) + 5)
											break;

										sockaddr_in sa;
										memset(&sa, 0, sizeof(sa));
										sa.sin_family = AF_INET;
#ifdef WIN32
										sa.sin_addr.S_un.S_addr = *(unsigned int*)req->socks5_req.address;
#else
										sa.sin_addr.s_addr = *(unsigned int*)req->socks5_req.address;
#endif
										sa.sin_port = *(unsigned short*)(req->socks5_req.address + 4);
										bindTo = aux_getAddressString((sockaddr*)&sa);
										DbgPrint("SOCKS5 proxy bind to %s", bindTo.c_str());
									}
									break;
								case SOCKS5_ADDR_DOMAIN:
									{
										if (len < (int)sizeof(SOCKS5_REQUEST) + *req->socks5_req.address + 2)
											break;
										connectTo = std::string(req->socks5_req.address + 1, 
											req->socks5_req.address + *req->socks5_req.address + 1);
										unsigned short port = *(unsigned short*)(req->socks5_req.address + *req->socks5_req.address + 1);
										char sPort[10];
										_snprintf(sPort, sizeof(sPort), ":%d", htons(port));
										bindTo += sPort;
										DbgPrint("SOCKS5 proxy bind to %s", bindTo.c_str());
									}
									break;
								case SOCKS5_ADDR_IPV6:
									{
										if (len < (int)sizeof(SOCKS5_REQUEST) + 17)
											break;

										sockaddr_in6 sa;
										memset(&sa, 0, sizeof(sa));
										sa.sin6_family = AF_INET6;
										memcpy(&sa.sin6_addr, req->socks5_req.address, 16);
										sa.sin6_port = *(unsigned short*)(req->socks5_req.address + 16);
										bindTo = aux_getAddressString((sockaddr*)&sa);
										DbgPrint("SOCKS5 proxy bind to %s", bindTo.c_str());
									}
									break;
								default:
									break;
								}
							}
						}
						break;
					case S5C_UDP_ASSOCIATE:
						break;
					default:
						break;
					}
				}
			}
			break;
		default:
			DbgPrint("ProxyFilter::postObject() id=%llu ot=%d invalid request", m_pSession->getId(), object.getType());
			return false;
		};
	}

	if (m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN)
	{
		if (!connectTo.empty())
			m_pSession->setLocalEndpointStr(connectTo);
		
		if (!bindTo.empty())
			m_pSession->setRemoteEndpointStr(bindTo);

		m_state = STATE_RESP;

		return m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, buffer.buffer(), buffer.size()) != FR_ERROR;
	} else
	{
		if (!connectTo.empty())
			m_pSession->setRemoteEndpointStr(connectTo);
		
		if (!bindTo.empty())
			m_pSession->setLocalEndpointStr(bindTo);

		m_state = STATE_RESP;

		return m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, buffer.buffer(), buffer.size()) != FR_ERROR;
	}
}
