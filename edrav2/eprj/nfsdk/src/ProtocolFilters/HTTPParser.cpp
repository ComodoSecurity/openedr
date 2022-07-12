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
#include "HTTPParser.h"
#include "HTTPDefs.h"
#include "ProxySession.h"
#include "HTTPFilter.h"

using namespace ProtocolFilters;
using namespace std;

inline char * findChar(char c, const char * s, int offset, int len)
{
	for (int i = offset; i < len; i++)
	{
		if (s[i] == c)
			return (char*)(s + i);
	}
	return NULL;
}


HTTPParser::HTTPParser(PF_ObjectType ot)
{
	m_pObject = new PFObjectImpl(ot, 3);
	reset(false);
	m_httpVersion = 10;
}

HTTPParser::~HTTPParser(void)
{
	if (m_pObject)
	{
		delete m_pObject;
		m_pObject = NULL;
	}
}

HTTPParser::tState HTTPParser::getState()
{
	return m_state;
}

int HTTPParser::getHttpVersion()
{
	return m_httpVersion;
}

PFHeader & HTTPParser::getHeader()
{
	return m_header;
}

void HTTPParser::setExtraHeaders(const std::string & extraHeaders)
{
	m_extraHeaders = extraHeaders;
}

void HTTPParser::reset(bool keepBuffer)
{
//	if (m_contentEncoding == CE_GZIP)
	{
		m_uzStream.reset();
	}

	if (!keepBuffer)
	{
		m_buffer.reset();
	}

	m_header.clear();
	m_status.clear();
	m_state = STATE_NONE;
	m_sContentEncoding.clear();
	m_contentEncoding = CE_NOT_ENCODED;
	m_transferEncoding = TE_NORMAL;
	m_contentLength = 0;
	m_readContentLength = 0;
	m_waitingForClose = false;
	m_skipObject = false;
	m_isHtml = false;
	m_ignoreResponse = false;
	m_sentContentLength = 0;

	m_pObject->setReadOnly(false);

	PFStream * pStream;

	pStream = m_pObject->getStream(HS_STATUS);
	if (pStream)
	{
		pStream->reset();
	}
	pStream = m_pObject->getStream(HS_HEADER);
	if (pStream)
	{
		pStream->reset();
	}
	pStream = m_pObject->getStream(HS_CONTENT);
	if (pStream)
	{
		pStream->reset();
	}
}

bool HTTPParser::isHTTP(const char * buf, int len)
{
	const char httpVersion[] = "HTTP/1.";
	const char httpVersionL[] = "http/1.";
	int i, j;
	int state;

	if (m_pObject->getType() == OT_HTTP_RESPONSE)
	{
		if (len < (int)(sizeof(httpVersion)-1))
			return false;

		for (i = 0; i<(int)sizeof(httpVersion)-1; i++)
		{
			if (buf[i] != httpVersion[i] &&
				buf[i] != httpVersionL[i])
			{
				return false;
			}
		}

		return true;
	}

	if (len < 12)
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


int HTTPParser::inputPacket(const char * buffer, int len)
{
	if (len > 0)
	{
		if (!m_buffer.append(buffer, len))
			return -1;
	}

	if (m_state == STATE_NONE && m_buffer.size() > 0)
	{
		m_state = STATE_HEADER;
	}

	switch (m_state)
	{
	case STATE_HEADER:
		{
			char * pDivider, * p;			
			char * pBuffer = m_buffer.buffer();
			int i, n;

			if (m_buffer.size() > 1)
			{
				if (!isHTTP(m_buffer.buffer(), m_buffer.size()))
					return -1;
			} else
			{
				return 0;
			}

			n = 0;
			pDivider = 0;
			for (i=0; i<(int)m_buffer.size(); i++)
			{
				if (pBuffer[i] == '\r')
					continue;
				if (pBuffer[i] == '\n')
				{
					n++;
					if (n == 2)
					{
						pDivider = pBuffer + i + 1;
						break;
					}
					continue;
				}
				n = 0;
			}

			if (len == 0 &&
				!pDivider)
			{
				pDivider = pBuffer + m_buffer.size();
			}

			if (pDivider)
			{
				PFStream * pStream;

				// Process header
				
				p = findChar('\n', pBuffer, 0, m_buffer.size());
				if (p)
				{
					if (p > pBuffer)
					{
						char * ep = (*(p-1) != '\r')? p : p - 1;
						m_status = string(pBuffer, ep);

						if (m_pObject->getType() == OT_HTTP_REQUEST)
						{
							m_httpVersion = (*(ep-1) == '1')? 11 : 10;
						} else
						{
							if ((ep - pBuffer) > 7)
							{
								m_httpVersion = (*(pBuffer+7) == '1')? 11 : 10;
							}
						}
					}					
					
					p += 1;

					pStream = m_pObject->getStream(HS_STATUS);
					if (pStream)
					{
						pStream->reset();
						pStream->write(pBuffer, (tStreamSize)(p - pBuffer));
					}

					std::string strHeader = m_extraHeaders + std::string(p, pDivider);
					parseHeader(strHeader.c_str(), (int)strHeader.length(), ((int)m_buffer.size() > (pDivider - pBuffer)));

					m_state = STATE_CONTENT;

					if ((int)m_buffer.size() > (pDivider - pBuffer))
					{
						len = m_buffer.size() - (int)(pDivider - pBuffer);
						memmove(pBuffer, pDivider, len);
						m_buffer.resize(len);

						if (m_status.find(" 101") != std::string::npos)
						{
							m_contentLength = (__int64)len;
						}
						
						if ((m_contentLength > 0) ||
							(m_transferEncoding == TE_CHUNKED) ||
							(m_pObject->getType() == OT_HTTP_RESPONSE && 
								(m_contentLength > 0 || m_waitingForClose)))
						{
							parseContent();
						} else
						if (m_pObject->getType() == OT_HTTP_REQUEST)
						{
							if (m_buffer.buffer()[0] != 'G')
								return -1;
						}
					} else
					{
						m_buffer.reset();
					}

					if ((m_transferEncoding != TE_CHUNKED) && (m_contentLength == 0))
					{
						if ((m_pObject->getType() == OT_HTTP_REQUEST) || !m_waitingForClose)
						{
							if (m_buffer.size() > 0)
							{
								m_state = STATE_FINISHED_CONTINUE;
							} else
							{
								m_state = STATE_FINISHED;
							}
						}
					}
				}
			} 
#ifdef _DEBUG
/*			else
			{
				char fileName[_MAX_PATH];
				static int c = 0;

				_snprintf(fileName, sizeof(fileName), "http_header_part%d.bin", c++);

				FILE * f = fopen(fileName, "wb");
				if (f)
				{
					fwrite(pBuffer, m_buffer.size(), 1, f);
					fclose(f);
				}
			}
*/
#endif

		}
		break;

	case STATE_CONTENT:
		{
			if (!parseContent())
				return -1;
		}
		break;

	default:
		break;
	}

	return (m_state == STATE_FINISHED_CONTINUE)? m_buffer.size() : 0;
}

bool HTTPParser::parseHeader(const char * buffer, int len, bool hasContent)
{
	m_header.parseString(buffer, len);

#ifdef _DEBUG
/*			{
				char fileName[_MAX_PATH];
				static int c = 0;

				_snprintf(fileName, sizeof(fileName), "http_header%d.bin", c++);

				FILE * f = fopen(fileName, "wb");
				if (f)
				{
					fwrite(buffer, len, 1, f);
					fclose(f);
				}
			}
*/
#endif

	PFHeaderField * pField;

	// Accept only gzip/deflate or identity compression
	pField = m_header.findFirstField(HTTP_ACCEPT_ENCODING_SZ);
	if (pField)
	{
		std::string encoding = strToLower(pField->value());

		if (encoding.find("identity") != std::string::npos)
		{
			m_header.removeFields(HTTP_ACCEPT_ENCODING_SZ, true);
			m_header.addField(HTTP_ACCEPT_ENCODING_SZ, "identity");
		} else
		{
			m_header.removeFields(HTTP_ACCEPT_ENCODING_SZ, true);
			m_header.addField(HTTP_ACCEPT_ENCODING_SZ, "gzip, deflate");
		}
	}

	m_transferEncoding = TE_NORMAL;

	pField = m_header.findFirstField(HTTP_TRANSFER_ENCODING_SZ);
	if (pField)
	{
		if (stricmp(pField->value().c_str(), CHUNKED_SZ) == 0)
		{
			m_transferEncoding = TE_CHUNKED;
			m_header.removeFields(HTTP_TRANSFER_ENCODING_SZ, true);
		}
	}

	m_contentEncoding = CE_NOT_ENCODED;

	pField = m_header.findFirstField(HTTP_CONTENT_TYPE_SZ);
	if (pField)
	{
		const char szApp[] = "application/";
/*
		const char szAppGzip[] = "application/x-gzip";
		const char szAppGtar[] = "application/x-gtar";
		const char szAppTar[] = "application/x-tar";
		const char szAppZip[] = "application/zip";
		const char szAppCompress[] = "application/x-compress";
*/
		const char szHtml[] = "text/html";
		bool uncompress = true;

		if (strnicmp(pField->value().c_str(), szHtml, sizeof(szHtml)-1) == 0)
		{
			m_isHtml = true;
		}

		// Do not unzip the archives
		if (strnicmp(pField->value().c_str(), szApp, sizeof(szApp)-1) == 0)
		{
/*			if (strnicmp(pField->value().c_str(), szAppGzip, sizeof(szAppGzip)-1) == 0 ||
				strnicmp(pField->value().c_str(), szAppGtar, sizeof(szAppGtar)-1) == 0 ||
				strnicmp(pField->value().c_str(), szAppTar, sizeof(szAppTar)-1) == 0 ||
				strnicmp(pField->value().c_str(), szAppZip, sizeof(szAppZip)-1) == 0 ||
				strnicmp(pField->value().c_str(), szAppCompress, sizeof(szAppCompress)-1) == 0)
*/			{
				uncompress = false;
			}
		}

		if (uncompress)
		{
			pField = m_header.findFirstField(HTTP_CONTENT_ENCODING_SZ);
			if (pField)
			{
				if (stricmp(pField->value().c_str(), GZIP_SZ) == 0 ||
					stricmp(pField->value().c_str(), X_GZIP_SZ) == 0 ||
					stricmp(pField->value().c_str(), DEFLATE_SZ) == 0 ||
					stricmp(pField->value().c_str(), COMPRESS_SZ) == 0 ||
					stricmp(pField->value().c_str(), X_COMPRESS_SZ) == 0)
				{
					m_contentEncoding = CE_GZIP;
					m_sContentEncoding = pField->value();
					m_header.removeFields(HTTP_CONTENT_ENCODING_SZ, true);
				}
			}
		}
	}
	
	bool isHeadRequest = false;

	pField = m_header.findFirstField(HTTP_EXHDR_RESPONSE_REQUEST);
	if (pField)
	{
		// Handle HEAD responses properly
		if (strToLower(pField->value()).find("head ") == 0)
		{
			m_contentLength = 0;
			m_waitingForClose = false;
			isHeadRequest = true;
		}
	} 

	if (m_pObject->getType() == OT_HTTP_RESPONSE) 
	{
		pField = m_header.findFirstField(HTTP_CONTENT_LENGTH_SZ);
		if (!pField)
		{
			int httpCode = 0;
			size_t pos = m_status.find(" ");
			if (pos != std::string::npos)
			{
				httpCode = atoi(&m_status[pos]);
			}

			switch (httpCode)
			{
			case 100:
			case 101:
			case 204:
			case 301:
			case 302:
			case 304:
				{
					m_contentLength = 0;
					m_waitingForClose = false;
					isHeadRequest = true;
				}
				break;
			}
		}
	}

	if (!isHeadRequest)
	{
		pField = m_header.findFirstField(HTTP_CONTENT_LENGTH_SZ);
		if (pField)
		{
			m_contentLength = _atoi64(pField->value().c_str());
		} else
		{
			if ((m_pObject->getType() == OT_HTTP_RESPONSE) &&
				(m_transferEncoding != TE_CHUNKED))
			{
				pField = m_header.findFirstField(HTTP_CONNECTION_SZ);
				if (pField && 
					(stricmp(pField->value().c_str(), CLOSE_SZ) == 0))
				{
					m_waitingForClose = true;
				} else
				{
					int httpCode = 0;
					size_t pos = m_status.find(" ");
					if (pos != std::string::npos)
					{
						httpCode = atoi(&m_status[pos]);
					}

					switch (httpCode)
					{
					case 100:
					case 101:
					case 204:
					case 301:
					case 302:
					case 304:
						{
							m_ignoreResponse = true;
						}
						break;
					default:
						{
							if (hasContent || m_header.findFirstField(HTTP_CONTENT_TYPE_SZ))
							{
								m_waitingForClose = true;
							}
						}
					}
				}
			}	
		}
	}
	return true;
}
		
bool HTTPParser::parseContent()
{
	if (m_transferEncoding == TE_CHUNKED)
	{
		char *	p;
		char *	pOff;
		int		offset = 0;
		int		lastChunkOffset = 0;
		int		chunkSize;
		char *	pBuffer = m_buffer.buffer();
		int		len = m_buffer.size();

		while (offset < len)
		{
			while (offset < len)
			{
				if (strchr("\r\n ", pBuffer[offset]) != 0)
					offset++;
				else
					break;
			}

			p = findChar('\n', pBuffer, offset, len);
			if (!p)
				break;

			chunkSize = strtol(pBuffer + offset, &pOff, 16);
			
			if (chunkSize < 0)
			{
				break;
			}

			if (chunkSize == 0)
			{
				if (offset < len)
				{
					p = findChar('\n', pBuffer, (int)(p - pBuffer), len);
					if (!p)
						break;
				}

				if (offset <= len)
				{
					m_state = STATE_FINISHED;
				} else
				{
					m_state = STATE_FINISHED_CONTINUE;
				}

				m_buffer.reset();
				return true;
			}

			lastChunkOffset = offset;
			offset += (int)(p - (pBuffer + offset) + 1);

			if ((offset + chunkSize) > len)
			{
				break;
			} else
			{
				saveContent(pBuffer + offset, chunkSize);
				
				offset += chunkSize + 2;
				lastChunkOffset = offset;
			}
		}

		if (offset <= len)
		{
			memmove(pBuffer, pBuffer + lastChunkOffset, len - lastChunkOffset);
			m_buffer.resize(len - lastChunkOffset);
		} else
		{
			m_buffer.reset();
		}
	} else
	{
		int		len = m_buffer.size();

		m_readContentLength += m_buffer.size();

		DbgPrint("HTTPParser::parseContent() m_readContentLength %d", m_readContentLength);

		if (m_contentLength > 0)
		{
			if (m_readContentLength >= m_contentLength)
			{
				if (m_readContentLength == m_contentLength)
				{
					m_state = STATE_FINISHED;
				} else
				{
					if ((m_readContentLength - m_contentLength) <= 4)
					{
						m_state = STATE_FINISHED;
					} else
					{
						PFHeaderField * pField = m_header.findFirstField(HTTP_CONNECTION_SZ);
						if (pField && 
							(stricmp(pField->value().c_str(), CLOSE_SZ) == 0))
						{
							DbgPrint("HTTPParser::parseContent() content length overrun (%d > %d), waiting for close", m_readContentLength, m_contentLength);
						} else
						{
							len -= (int)(m_readContentLength - m_contentLength);
						}
						m_state = STATE_FINISHED_CONTINUE;
					}
				}
			} else
			{
				if (m_isHtml)
				{
					if (m_buffer.size() > 0 && 
						(m_readContentLength < m_contentLength) && 
						((m_contentLength - m_readContentLength) < 10))
					{
						if (strstr(m_buffer.buffer(), "</html>") != NULL ||
							strstr(m_buffer.buffer(), "</HTML>") != NULL)
						{
							m_state = STATE_FINISHED;
						}
					}
				}
			}
		}

		if (!saveContent(m_buffer.buffer(), len))
			return false;
		
		if (len == (int)m_buffer.size())
		{
			m_buffer.reset();
		} else
		{
			memmove(m_buffer.buffer(), m_buffer.buffer() + len, m_buffer.size() - len);
			m_buffer.resize(m_buffer.size() - len);
		}
	}

	return true;
}

bool HTTPParser::saveContent(const char * buffer, int len)
{
	if (m_skipObject)
		return true;
	
	PFStream * pStream = m_pObject->getStream(HS_CONTENT);
	bool result = false;
	
	if (!pStream)
	{
		return false;
	}

	pStream->seek(0, FILE_END);

	if (m_contentEncoding == CE_GZIP)
	{
		IOB outBuf;
		
		if (m_uzStream.uncompress(buffer, len, outBuf))
		{
			result = pStream->write(outBuf.buffer(), outBuf.size()) == outBuf.size();
		} else
		{
			if (pStream->size() == 0)
			{
				m_contentEncoding = CE_NOT_ENCODED;
				m_header.addField(HTTP_CONTENT_ENCODING_SZ, m_sContentEncoding);
				result = (int)pStream->write(buffer, len) == len;
			} else
			{
				return false;
			}
		}
	} else
	{
		result = pStream->write(buffer, len) == len;
	}

	return result;
}

PFObjectImpl * HTTPParser::getObject(bool checkContentLength)
{
	PFStream * pStream;

	if (checkContentLength)
	{
		pStream = m_pObject->getStream(HS_CONTENT);

		if (pStream)
		{
			bool isHeadRequest = false;

			PFHeaderField * pField = m_header.findFirstField(HTTP_EXHDR_RESPONSE_REQUEST);
			if (pField)
			{
				// Handle HEAD responses properly
				if (strToLower(pField->value()).compare(0, 5, "head ") == 0)
				{
					isHeadRequest = true;
				}
			} 

			if (!isHeadRequest)
			{
				if (m_header.findFirstField(HTTP_CONTENT_LENGTH_SZ) ||
					(m_transferEncoding == TE_CHUNKED) ||
					(pStream->size() > 0))
				{
					char clBuf[100];
			
					_snprintf(clBuf, sizeof(clBuf), "%llu", pStream->size());

					m_header.removeFields(HTTP_CONTENT_LENGTH_SZ, true);
					m_header.addField(HTTP_CONTENT_LENGTH_SZ, clBuf);
				}
			}
		}
	}

	pStream = m_pObject->getStream(HS_HEADER);
	if (pStream)
	{
		pStream->reset();

		m_header.removeFields(HTTP_TRANSFER_ENCODING_SZ, true);

		string headerStr = m_header.toString() + "\r\n";

		pStream->write(headerStr.c_str(), (unsigned long)headerStr.length());
	}
	
	// Rewind streams
	
	pStream = m_pObject->getStream(HS_STATUS);
	if (pStream)
	{
		pStream->seek(0, FILE_BEGIN);
	}

	pStream = m_pObject->getStream(HS_HEADER);
	if (pStream)
	{
		pStream->seek(0, FILE_BEGIN);
	}

	pStream = m_pObject->getStream(HS_CONTENT);
	if (pStream)
	{
		pStream->seek(0, FILE_BEGIN);
	}

	return m_pObject;
}

std::string HTTPParser::getStatus()
{
	return m_status;
}

void HTTPParser::skipObject()
{
	m_skipObject = true;

	PFStream * pStream = m_pObject->getStream(HS_CONTENT);
	if (pStream)
	{
		pStream->reset();
	}
}

void HTTPParser::sendUpdatedObject(tStreamPos oldContentLength, ePacketDirection pd, ProxySession * pSession, HTTPFilter * pFilter)
{
	IOB tempBuf, hdrBuf;
	PFStream * pStream;
	tStreamPos contentLength = m_pObject->getStream(HS_CONTENT)->size();

	if (m_contentEncoding == CE_GZIP && m_contentLength > 0 &&
		(m_state != STATE_FINISHED && m_state != STATE_FINISHED_CONTINUE))
	{
		return;
	}

	pStream = m_pObject->getStream(HS_STATUS);
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

	pStream = m_pObject->getStream(HS_HEADER);
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

				if (m_transferEncoding == TE_CHUNKED)
				{
					header.addField(HTTP_TRANSFER_ENCODING_SZ, "chunked");
				}

				if (updateContentLength && header.findFirstField(HTTP_CONTENT_LENGTH_SZ) && m_contentLength > 0)
				{
					char clBuf[100];

					_snprintf(clBuf, sizeof(clBuf), "%llu", m_contentLength + ((__int64)contentLength - (__int64)oldContentLength));

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

	pStream = m_pObject->getStream(HS_CONTENT);
	if (!pStream)
	{
		pSession->tcp_packet(pFilter, DD_OUTPUT, pd, hdrBuf.buffer(), hdrBuf.size());
	} else
	{
		tStreamPos contentSize = pStream->size();
		
		if (contentSize > 0)
		{
			char chunkBuf[DEFAULT_BUFFER_SIZE];
			int n;

			pStream->seek(0, FILE_BEGIN);

			for (;;) 
			{
				n = pStream->read(chunkBuf, sizeof(chunkBuf));
				if (n == 0)
					break;

				if (m_transferEncoding == TE_CHUNKED)
				{
					char clBuf[100];
					_snprintf(clBuf, sizeof(clBuf), "%x\r\n", n);
					hdrBuf.append(clBuf, (unsigned long)strlen(clBuf));
				}
					
				hdrBuf.append(chunkBuf, n);

				if (m_transferEncoding == TE_CHUNKED)
				{
					hdrBuf.append("\r\n", 2);
				}
				break;
			}

			pSession->tcp_packet(pFilter, DD_OUTPUT, pd, hdrBuf.buffer(), hdrBuf.size());

			hdrBuf.reset();

			for (;;) 
			{
				tempBuf.reset();
					
				n = pStream->read(chunkBuf, sizeof(chunkBuf));
				if (n == 0)
					break;
					
				if (m_transferEncoding == TE_CHUNKED)
				{
					char clBuf[100];
					_snprintf(clBuf, sizeof(clBuf), "%x\r\n", n);
					tempBuf.append(clBuf, (unsigned long)strlen(clBuf));
				}

				tempBuf.append(chunkBuf, n);

				if (m_transferEncoding == TE_CHUNKED)
				{
					tempBuf.append("\r\n", 2);
				}

				pSession->tcp_packet(pFilter, DD_OUTPUT, pd, tempBuf.buffer(), tempBuf.size());
			}
		
			if (m_transferEncoding == TE_CHUNKED && 
				(m_state == STATE_FINISHED || m_state == STATE_FINISHED_CONTINUE))
			{
				pSession->tcp_packet(pFilter, DD_OUTPUT, pd, "0\r\n\r\n", 5);
			}

//			pStream->reset();
		} else
		{
			pSession->tcp_packet(pFilter, DD_OUTPUT, pd, hdrBuf.buffer(), hdrBuf.size());

			if (pStream->size() > 0)
			{
				pSession->tcpPostStream(pFilter, pd, pStream);
			}
		}

		m_sentContentLength = (int)contentSize;
	}
}

void HTTPParser::sendContent(ePacketDirection pd, ProxySession * pSession, HTTPFilter * pFilter)
{
	IOB tempBuf;
	PFStream * pStream;

	if (m_contentEncoding == CE_GZIP && m_contentLength > 0)
	{
		if (m_state == STATE_FINISHED || m_state == STATE_FINISHED_CONTINUE)
		{
			m_contentLength = (int)m_pObject->getStream(HS_CONTENT)->size();
			sendUpdatedObject(m_contentLength, pd, pSession, pFilter);
		}

		return;
	}

	pStream = m_pObject->getStream(HS_CONTENT);
	if (pStream)
	{
		tStreamPos contentSize = pStream->size();
		
		if (contentSize > 0)
		{
			char chunkBuf[DEFAULT_BUFFER_SIZE];
			int n;

			pStream->seek(m_sentContentLength, FILE_BEGIN);

			for (;;) 
			{
				tempBuf.reset();
					
				n = pStream->read(chunkBuf, sizeof(chunkBuf));
				if (n == 0)
					break;
					
				if (m_transferEncoding == TE_CHUNKED)
				{
					char clBuf[100];
					_snprintf(clBuf, sizeof(clBuf), "%x\r\n", n);
					tempBuf.append(clBuf, (unsigned long)strlen(clBuf));
				}

				tempBuf.append(chunkBuf, n);

				if (m_transferEncoding == TE_CHUNKED)
				{
					tempBuf.append("\r\n", 2);
				}

				pSession->tcp_packet(pFilter, DD_OUTPUT, pd, tempBuf.buffer(), tempBuf.size());
			}
		
//			pStream->reset();
			m_sentContentLength = (int)contentSize;
		}

		if (m_transferEncoding == TE_CHUNKED && 
			(m_state == STATE_FINISHED || m_state == STATE_FINISHED_CONTINUE))
		{
			pSession->tcp_packet(pFilter, DD_OUTPUT, pd, "0\r\n\r\n", 5);
		}
	}
}
