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
#include "HTTPFilter.h"
#include "HTTPDefs.h"
#include "ProxySession.h"
#include "PFHeader.h"

using namespace ProtocolFilters;
using namespace std;

HTTPFilter::HTTPFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_bFirstPacket(true),
	m_requestParser(OT_HTTP_REQUEST),
	m_responseParser(OT_HTTP_RESPONSE),
	m_flags(flags)
{
	m_pSession->getSessionInfo(&m_si);
	m_requestState = STATE_NONE; 
	m_responseState = STATE_NONE;
	m_requestPart.open();
	m_responsePart.open();
	m_bypassNextRequestInfo = false;
	setActive(false);
}

HTTPFilter::~HTTPFilter(void)
{
}

static const char RPC_IN_DATA[] = "RPC_IN_DATA ";
static const char RPC_OUT_DATA[] = "RPC_OUT_DATA ";
static const char CONNECT[] = "CONNECT ";

bool HTTPFilter::isHTTPRequest(const char * buf, int len)
{
	const char httpVersion[] = "HTTP/1.";
	const char httpVersionL[] = "http/1.";
	int i, j;
	int state;

	if (len < 12)
		return false;

	if (len >= (int)sizeof(RPC_IN_DATA)-1)
	{
		if (memcmp(buf, RPC_IN_DATA, sizeof(RPC_IN_DATA) - 1) == 0)
		{
			return false;
		}
	}

	if (len >= (int)sizeof(RPC_OUT_DATA)-1)
	{
		if (memcmp(buf, RPC_OUT_DATA, sizeof(RPC_OUT_DATA) - 1) == 0)
		{
			return false;
		}
	}

	if (len >= (int)sizeof(CONNECT)-1)
	{
		if (memcmp(buf, CONNECT, sizeof(CONNECT) - 1) == 0)
		{
			return false;
		}
	}

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

bool isSPDY(const char * buf, int len)
{
	if (( len > 4) &&
       ( (buf[0] & (1<<7)) > 0 ) &&
       ( buf[1] == 2 || buf[1] == 3 ) &&
       ( buf[2] == 0 ) &&
       ( (buf[3] == 4) || (buf[3] == 1) )
		)
	{
		return true;
	}

	return false;
}


eFilterResult HTTPFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	// Bypass filtered packets
	if (dd != DD_INPUT)
		return FR_PASSTHROUGH;

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	DbgPrint("HTTPFilter::tcp_packet() id=%llu dd=%d pd=%d len=%d",
				m_pSession->getId(), dd, pd, len);
	if (len > 0)
	{
		std::string s(buf, len);
		DbgPrint("Contents:\n%s", s.c_str());
	}
#endif

	// Check the protocol
	if (m_bFirstPacket)
	{
		if (((m_si.tcpConnInfo.direction == NFAPI_NS NF_D_OUT) && (pd != PD_SEND)) ||
			((m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN) && (pd != PD_RECEIVE)))
			return FR_DELETE_ME;

		if (m_flags & FF_HTTP_BLOCK_SPDY)
		{
			if (isSPDY(buf, len))
			{
				DbgPrint("HTTPFilter::tcp_packet() id=%llu blocked SPDY protocol", m_pSession->getId());
				return FR_ERROR;
			}
		}

		if (len > 1)
		{
			if (!isHTTPRequest(buf, len))
				return FR_DELETE_ME;
		} else
		if (len == 1)
		{
			if (buf[0] != 'G')
				return FR_DELETE_ME;
		} else
		{
			return FR_DELETE_ME;
		}

		m_bFirstPacket = false;
	}

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (!pHandler)
		return FR_PASSTHROUGH;

	ePacketDirection pdSimulated = pd;

	if (m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN)
	{
		pdSimulated = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
	}

	std::auto_ptr<IOB> pTempRequestBuffer;

	// Process pipelined requests one by one, unless FF_HTTP_KEEP_PIPELINING is specified for the filter
	if (!(m_flags & FF_HTTP_KEEP_PIPELINING))
	{
		if (pdSimulated == PD_SEND)
		{
			if (!m_requestInfoQueue.empty())
			{
				if (len != -1)
				{
					if (len == 0)
					{
						if (!m_requestBuffer.isEmpty() && !m_requestBuffer.getRemoteDisconnect())
						{
							m_requestBuffer.setDisconnect(true);
							return FR_DATA_EATEN;
						}
					} else
					{
						if (!m_requestBuffer.append(buf, len))
							return FR_ERROR;
						return FR_DATA_EATEN;
					}
				} else
				{
					return FR_DATA_EATEN;
				}
			} else
			{
				if (!m_requestBuffer.isEmpty())
				{
					if (len != -1)
					{
						if (len == 0)
						{
							m_requestBuffer.setDisconnect(true);
						} else
						{
							if (!m_requestBuffer.append(buf, len))
								return FR_ERROR;
						}
					}

					if (!pTempRequestBuffer.get())
					{
						pTempRequestBuffer = std::auto_ptr<IOB>(new IOB());
						if (!pTempRequestBuffer->resize(DEFAULT_BUFFER_SIZE))
							return FR_ERROR;
					}

					len = m_requestBuffer.read(pTempRequestBuffer->buffer(), pTempRequestBuffer->size());
					if (len == 0)
					{
						return FR_ERROR;
					}

					buf = pTempRequestBuffer->buffer();
				} else
				{
					if (len == -1)
					{
						if (m_requestBuffer.getDisconnect())
						{
							m_requestBuffer.setDisconnect(false);
							buf = NULL;
							len = 0;
						}
					} else
					{
						if (len == 0)
						{
							m_requestBuffer.setDisconnect(true);
						} else
						{
							if (!m_requestBuffer.append(buf, len))
								return FR_ERROR;
						}
					}
				}
			}
		} else
		{
			if (len == 0)
			{
				m_requestBuffer.setRemoteDisconnect(true);
		
				if (m_requestBuffer.getDisconnect())
				{
					tcp_packet(DD_OUTPUT, PD_SEND, NULL, 0);
				}
			}
		}
	}

	HTTPParser & parser = (pdSimulated == PD_SEND)? m_requestParser : m_responseParser;
	MFStream & contentPart = (pdSimulated == PD_SEND)? m_requestPart : m_responsePart;
	eParserState & state = (pdSimulated == PD_SEND)? m_requestState : m_responseState;
	int remaining = len;
	int offset = 0;
	int oldOffset = 0;
	int canPost = 0;

	for (;;)
	{
		if ((len > 0) || (offset < len))
		{
			if ((pdSimulated == PD_RECEIVE) && (state == STATE_NONE) && !m_requestInfoQueue.empty())
			{
				std::string & ri = m_requestInfoQueue.front();
				parser.setExtraHeaders(ri);
			}

			// Parse the buffer, taking in account the case when it contains multiple requests or responses
			remaining = parser.inputPacket(buf, remaining);
			if (remaining == -1)
				return FR_DELETE_ME;

			setActive(true);

			// The length of parsed object
			canPost = (len - remaining) - offset;
			// Store last offset
			oldOffset = offset;
			// Move offset to the start of next object
			offset = (len - remaining);

			if (pdSimulated == PD_SEND)
			{
				if (!m_requestBuffer.isEmpty())
				{
					m_requestBuffer.incOffset(canPost);
				}
			}

			if (state == STATE_NONE)
			{
				contentPart.reset();

				if (((m_flags & FF_DONT_FILTER_IN) && (pdSimulated == PD_RECEIVE)) ||
					((m_flags & FF_DONT_FILTER_OUT) && (pdSimulated == PD_SEND)))
				{
					state = STATE_BYPASS;
					parser.skipObject();
				} else
				if (((pdSimulated == PD_RECEIVE) && (m_flags & FF_READ_ONLY_IN)) ||
					((pdSimulated == PD_SEND) && (m_flags & FF_READ_ONLY_OUT)))
				{
					state = STATE_READ_ONLY;
				} else
				{
					state = STATE_CONTENT_PART;
				}
			} 

			if (((state == STATE_READ_ONLY) || (state == STATE_BYPASS)))
			{
				DbgPrint("HTTPFilter::tcp_packet() post read only stream offset=%d, len=%d", 
					oldOffset, canPost);

				m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf + oldOffset, canPost);
			} 

			if (state == STATE_UPDATE_AND_BYPASS ||
				state == STATE_UPDATE_AND_FILTER_READ_ONLY)
			{
				parser.sendContent(pd, m_pSession, this);
			}

			if (state == STATE_CONTENT_PART) 
			{ 
				if (!contentPart.write(buf + oldOffset, canPost))
					return FR_ERROR;

				if (parser.getState() == HTTPParser::STATE_CONTENT)
				{
					PFObjectImpl * pObj = parser.getObject(false);
					PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
					tStreamPos oldContentLength;
					
					oldContentLength = pObj->getStream(HS_CONTENT)->size();
			
					try {
						DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable, type=%d", pObj->getType());
						cr = pHandler->dataPartAvailable(m_pSession->getId(), pObj);
					} catch (...)
					{
					}

					switch (cr)
					{
					case DPCR_MORE_DATA_REQUIRED:
						DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned DPCR_MORE_DATA_REQUIRED");
						break;
					
					case DPCR_FILTER:
						{
							DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER");
							state = STATE_CONTENT;
							contentPart.reset();
						}
						break;
					
					case DPCR_FILTER_READ_ONLY:
						{
							DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER_READ_ONLY");
							state = STATE_READ_ONLY;
							m_pSession->tcpPostStream(this, pd, &contentPart);
							contentPart.reset();
						}
						break;
					
					case DPCR_BYPASS:
						{
							DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned STATE_BYPASS");
							state = STATE_BYPASS;
							m_pSession->tcpPostStream(this, pd, &contentPart);
							contentPart.reset();
							parser.skipObject();
						}
						break;

					case DPCR_BLOCK:
						{
							DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned STATE_BLOCK");
							state = STATE_BLOCK;
							contentPart.reset();
							parser.skipObject();
						}
						break;

					case DPCR_UPDATE_AND_BYPASS:
						{
							DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned DPCR_UPDATE_AND_BYPASS");
							state = STATE_UPDATE_AND_BYPASS;
							contentPart.reset();
							parser.sendUpdatedObject(oldContentLength, pd, m_pSession, this);
						}
						break;

					case DPCR_UPDATE_AND_FILTER_READ_ONLY:
						{
							DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned DPCR_UPDATE_AND_FILTER_READ_ONLY");
							state = STATE_UPDATE_AND_FILTER_READ_ONLY;
							contentPart.reset();
							parser.sendUpdatedObject(oldContentLength, pd, m_pSession, this);
						}
						break;

					default:
						DbgPrint("HTTPFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
						break;
					}
				}
			}
		} else
		{
			if (len == 0)
			{
				parser.inputPacket(buf, len);
			}
		}
		
		if (((len == 0) && (parser.getState() != HTTPParser::STATE_NONE)) || 
			(parser.getState() == HTTPParser::STATE_FINISHED) ||
			(parser.getState() == HTTPParser::STATE_FINISHED_CONTINUE))
		{
			contentPart.reset();

			if ((state != STATE_BYPASS) && (state != STATE_BLOCK) && (state != STATE_UPDATE_AND_BYPASS))
			{
				PFObjectImpl * pObj = parser.getObject();
	
				if (state == STATE_READ_ONLY ||
					state == STATE_UPDATE_AND_FILTER_READ_ONLY)
				{
					DbgPrint("HTTPFilter::tcp_packet() indicating read-only object");

					pObj->setReadOnly(true);
				}

				if (pdSimulated == PD_SEND)
				{
					pushRequestInfo();
				}

				try {
					DbgPrint("HTTPFilter::tcp_packet() dataAvailable, type=%d", pObj->getType());
					pHandler->dataAvailable(m_pSession->getId(), pObj);
				} catch (...)
				{
				}
	
				if (state == STATE_READ_ONLY ||
					state == STATE_UPDATE_AND_FILTER_READ_ONLY)
				{
					if (pdSimulated == PD_RECEIVE)
					{
						popRequestInfo();
					}
				}
			} else
			{
				if (pdSimulated == PD_SEND)
				{
					pushRequestInfo();
				} else
				{
					popRequestInfo();
				}

				if (m_flags & FF_HTTP_INDICATE_SKIPPED_OBJECTS)
				{
					PFObjectImpl * pObj = parser.getObject(false);
		
					pObj->setReadOnly(true);

					tPF_ObjectType type = pObj->getType();
					
					pObj->setType((type == OT_HTTP_REQUEST)? OT_HTTP_SKIPPED_REQUEST_COMPLETE : OT_HTTP_SKIPPED_RESPONSE_COMPLETE);

					try {
						DbgPrint("HTTPFilter::tcp_packet() dataAvailable, type=%d", pObj->getType());
						pHandler->dataAvailable(m_pSession->getId(), pObj);
					} catch (...)
					{
					}

					pObj->setType(type);
				}
			}

			if (parser.getState() == HTTPParser::STATE_FINISHED_CONTINUE)
			{
				// Continue parsing the objects in the current buffer
				bool ignoreResponse = parser.isIgnoreResponse();
				state = STATE_NONE;
				remaining = 0;
				
				if (!(m_flags & FF_HTTP_KEEP_PIPELINING) && !ignoreResponse)
				{
					parser.reset(false);
					break;
				} else
				{
					parser.reset(true);
					continue;
				}
			}

			parser.reset(false);
			state = STATE_NONE;
		}

		break;
	}

	if (len == 0)
	{
		if (pHandler)
		{
			PFObjectImpl obj((pd == PD_SEND)? OT_TCP_DISCONNECT_LOCAL : OT_TCP_DISCONNECT_REMOTE, 0);
			try {
				DbgPrint("HTTPFilter::tcp_packet() dataAvailable, type=%d", obj.getType());
				pHandler->dataAvailable(m_pSession->getId(), &obj);
			} catch (...)
			{
			}
		}
	} 

	return FR_DATA_EATEN;
}
		
eFilterResult HTTPFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType HTTPFilter::getType()
{
	return FT_HTTP;
}

PF_FilterCategory HTTPFilter::getCategory()
{
	return FC_PROTOCOL_FILTER;
}

bool HTTPFilter::postObject(PFObjectImpl & object)
{
	ePacketDirection pd;

	DbgPrint("HTTPFilter::postObject() type=%d", object.getType());

	switch (object.getType())
	{
	case OT_HTTP_REQUEST:
		pd = PD_SEND;
		if (m_requestInfoQueue.empty())
		{
			pushRequestInfo(&object);
		}
		break;

	case OT_HTTP_RESPONSE:
		pd = PD_RECEIVE;
		popRequestInfo();
		break;

	default:
		return false;
	}

	if (m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN)
	{
		pd = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
	}

	if (object.getStreamCount() == 3)
	{
		IOB tempBuf, hdrBuf;
		PFStream * pStream;
		tStreamPos contentLength = 0;
		tTransferEncoding transferEncoding = TE_NORMAL;

		pStream = object.getStream(HS_CONTENT);
		if (pStream)
		{
			contentLength = pStream->size();
		}

		pStream = object.getStream(HS_STATUS);
		if (pStream)
		{
			tempBuf.reset();
			tempBuf.append(NULL, (unsigned long)pStream->size(), false);

			if (tempBuf.buffer())
			{
				// response

				pStream->seek(0, FILE_BEGIN);
				pStream->read(tempBuf.buffer(), tempBuf.size());

				hdrBuf.append(tempBuf.buffer(), tempBuf.size());
			}
		}

		pStream = object.getStream(HS_HEADER);
		if (pStream)
		{
			tempBuf.reset();
			tempBuf.append(NULL, (unsigned long)pStream->size(), false);

			if (tempBuf.buffer())
			{
				PFHeader header;
		
				pStream->seek(0, FILE_BEGIN);
				pStream->read(tempBuf.buffer(), tempBuf.size());

				if (header.parseString(tempBuf.buffer(), tempBuf.size()))
				{
					bool updateContentLength = true;

//					if (pd == PD_RECEIVE)
					{
						PFHeaderField * pField = header.findFirstField(HTTP_EXHDR_RESPONSE_REQUEST);
						if (pField)
						{
							// Handle HEAD responses properly
							if (strToLower(pField->value()).find("head ") == 0)
							{
								updateContentLength = false;
							}
						} 

						header.removeFields(HTTP_EXHDR_RESPONSE_REQUEST, true);
						header.removeFields(HTTP_EXHDR_RESPONSE_HOST, true);
					}

					if (updateContentLength && object.getType() == OT_HTTP_RESPONSE)
					{
						transferEncoding = m_responseParser.getTransferEncoding();
						
						PFHeaderField * pField = header.findFirstField(HTTP_TRANSFER_ENCODING_SZ);
						if (pField)
						{
							if (stricmp(pField->value().c_str(), CHUNKED_SZ) == 0)
							{
								transferEncoding = TE_NORMAL;
							}
						}

						if (transferEncoding == TE_CHUNKED)
						{
							header.removeFields(HTTP_CONTENT_LENGTH_SZ, true);
							header.addField(HTTP_TRANSFER_ENCODING_SZ, CHUNKED_SZ);
						}
					}

					if (updateContentLength && header.findFirstField(HTTP_CONTENT_LENGTH_SZ))
					{
						char clBuf[100];

						_snprintf(clBuf, sizeof(clBuf), "%llu", contentLength);

						header.removeFields(HTTP_CONTENT_LENGTH_SZ, true);
						header.addField(HTTP_CONTENT_LENGTH_SZ, clBuf);
					}
				
					string headerStr = header.toString() + "\r\n";
					
					hdrBuf.append(headerStr.c_str(), (unsigned long)headerStr.length());
				} else
				{
					hdrBuf.append(tempBuf.buffer(), tempBuf.size());
				}
			}
		}

		pStream = object.getStream(HS_CONTENT);
		if (!pStream)
		{
			m_pSession->tcp_packet(this, DD_OUTPUT, pd, hdrBuf.buffer(), hdrBuf.size());
		} else
		{
			tStreamPos contentSize = pStream->size();
			
//			if ((contentSize > 0) && (contentSize < 2 * DEFAULT_BUFFER_SIZE))
			if (contentSize > 0)
			{
				char tempBuf[2 * DEFAULT_BUFFER_SIZE];
				int n;

				pStream->seek(0, FILE_BEGIN);

				for (;;) 
				{
					n = pStream->read(tempBuf, sizeof(tempBuf));
					if (n == 0)
						break;

					if (transferEncoding == TE_CHUNKED)
					{
						char clBuf[100];
						_snprintf(clBuf, sizeof(clBuf), "%x\r\n", n);
						hdrBuf.append(clBuf, (unsigned long)strlen(clBuf));
					}
						
					hdrBuf.append(tempBuf, n);

					if (transferEncoding == TE_CHUNKED)
					{
						hdrBuf.append("\r\n", 2);
					}

					break;
				}
				m_pSession->tcp_packet(this, DD_OUTPUT, pd, hdrBuf.buffer(), hdrBuf.size());

				for (;;) 
				{
					n = pStream->read(tempBuf, sizeof(tempBuf));
					if (n == 0)
						break;
						
					if (transferEncoding == TE_CHUNKED)
					{
						char clBuf[100];
						_snprintf(clBuf, sizeof(clBuf), "%x\r\n", n);
						m_pSession->tcp_packet(this, DD_OUTPUT, pd, clBuf, (unsigned long)strlen(clBuf));
					}

					m_pSession->tcp_packet(this, DD_OUTPUT, pd, tempBuf, n);

					if (transferEncoding == TE_CHUNKED)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pd, "\r\n", 2);
					}
				}

				if (transferEncoding == TE_CHUNKED)
				{
					m_pSession->tcp_packet(this, DD_OUTPUT, pd, "0\r\n\r\n", 5);
				}
			
			} else
			{
				m_pSession->tcp_packet(this, DD_OUTPUT, pd, hdrBuf.buffer(), hdrBuf.size());

				if (pStream->size() > 0)
				{
					m_pSession->tcpPostStream(this, pd, pStream);
				}

				if (transferEncoding == TE_CHUNKED)
				{
					m_pSession->tcp_packet(this, DD_OUTPUT, pd, "0\r\n\r\n", 5);
				}
			}
		}

		return true;
	} else
	{
		return m_pSession->tcpPostObjectStreams(this, pd, object);
	}
}

static std::string loadAnsiString(PFStream * pStream, bool seekToBegin)
{
    if (pStream == NULL || pStream->size() == 0)
        return "";

	IOB buf;

	buf.append(NULL, (unsigned long)pStream->size()+1, false);

    if (seekToBegin)
    {
        pStream->seek(0, FILE_BEGIN);
    }

	tStreamSize len = pStream->read(buf.buffer(), buf.size());
	buf.buffer()[len] = 0;

	std::string res = buf.buffer();

	return res;
}

void HTTPFilter::pushRequestInfo(PFObject * pObject)
{
	std::string ri;

	if (m_bypassNextRequestInfo)
	{
		m_bypassNextRequestInfo = false;
		return;
	}

	if (pObject)
	{
		std::string status = loadAnsiString(pObject->getStream(HS_STATUS), true);
		std::string sHeader = loadAnsiString(pObject->getStream(HS_HEADER), true);
		
		PFHeader hdr;
		hdr.parseString(sHeader.c_str(), (int)sHeader.length());

		ri += std::string(HTTP_EXHDR_RESPONSE_REQUEST) + ": " + status;

		PFHeaderField * pField = hdr.findFirstField(HTTP_HOST_SZ);
		if (pField)
		{
			ri += std::string(HTTP_EXHDR_RESPONSE_HOST) + ": " + pField->value() + "\r\n";
		}
	} else
	{
		ri += std::string(HTTP_EXHDR_RESPONSE_REQUEST) + ": " + m_requestParser.getStatus() + "\r\n";

		PFHeaderField * pField = m_requestParser.getHeader().findFirstField(HTTP_HOST_SZ);
		if (pField)
		{
			ri += std::string(HTTP_EXHDR_RESPONSE_HOST) + ": " + pField->value() + "\r\n";
		}
	}

	m_requestInfoQueue.push(ri);
}

void HTTPFilter::popRequestInfo()
{
	if (!m_requestInfoQueue.empty())
	{
		{
			PFHeader hdr;
			PFHeaderField * pField;
			std::string ri;

			ri = m_requestInfoQueue.front();

			hdr.parseString(ri.c_str(), (int)ri.length());

			pField = hdr.findFirstField(HTTP_EXHDR_RESPONSE_REQUEST);
			if (pField)
			{
				if (strnicmp(pField->value().c_str(), "CONNECT", 7) == 0)
				{
					m_bFirstPacket = true;
				}
			}
		}

		if (m_responseParser.getStatus().find(" 101 ") != std::string::npos)
		{
			m_bFirstPacket = true;
		}

		if (m_responseParser.getStatus().find(" 100 ") != std::string::npos)
			return;

		m_requestInfoQueue.pop();
	
		ePacketDirection pdSimulated = (m_si.tcpConnInfo.direction == NFAPI_NS NF_D_IN)? PD_RECEIVE : PD_SEND;

		m_requestParser.reset((m_flags & FF_HTTP_KEEP_PIPELINING)? true : false);
		m_requestState = STATE_NONE;

		eFilterResult res;
		
		while (m_requestInfoQueue.empty() &&
			(!m_requestBuffer.isEmpty() ||
				m_requestBuffer.getDisconnect()))
		{
			res = tcp_packet(DD_INPUT, pdSimulated, NULL, -1);
			if (res == FR_ERROR || res == FR_DELETE_ME)
				return;
		}
	} else
	{
		m_bypassNextRequestInfo = true;
	}
}

