//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#pragma once

#include <queue>
#include "IFilterBase.h"
#include "HTTPParser.h"
#include "MFStream.h"

namespace ProtocolFilters
{
	// Helper class implementing a temporary buffer for pipelined HTTP requests
	class HTTPPipelineBuffer
	{
	public:
		HTTPPipelineBuffer()
		{
			memset(&m_streams[0], 0, sizeof(m_streams));
			m_offset = 0;
			m_disconnect = false;
			m_remoteDisconnect = false;
		}
		~HTTPPipelineBuffer()
		{
			if (m_streams[0])
			{
				delete m_streams[0];
				m_streams[0] = NULL;
			}
			if (m_streams[1])
			{
				delete m_streams[1];
				m_streams[1] = NULL;
			}
		}

		void setDisconnect(bool value)
		{
			m_disconnect = value;
		}

		bool getDisconnect()
		{
			return m_disconnect;
		}

		void setRemoteDisconnect(bool value)
		{
			m_remoteDisconnect = value;
		}

		bool getRemoteDisconnect()
		{
			return m_remoteDisconnect;
		}

		bool append(const char * buf, tStreamSize len)
		{
			MFStream * pStream;

			if (m_streams[1] != NULL)
			{
				// If stream 1 exists, append there
				pStream = m_streams[1];
			} else
			if (m_streams[0] != NULL)
			{
				// If stream 0 exists, but some content already parsed, create stream 1 for new data.
				// Use stream 0 in other case.
				if (m_offset > 0)
				{
					m_streams[1] = new MFStream();
					if (!m_streams[1]->open())
					{
						delete m_streams[1];
						return false;
					}
					pStream = m_streams[1];
				} else
				{
					pStream = m_streams[0];
				}
			} else
			{
				// Create stream 0 
				m_streams[0] = new MFStream();
				if (!m_streams[0]->open())
				{
					delete m_streams[0];
					return false;
				}
				pStream = m_streams[0];
			}

			if (!pStream)
				return false;

			pStream->seek(pStream->size(), FILE_BEGIN);

			return pStream->write(buf, len) != 0;
		}

		tStreamSize read(char * buf, tStreamSize len)
		{
			if (m_streams[0] == NULL)
				return 0;

			m_streams[0]->seek(m_offset, FILE_BEGIN);

			tStreamSize ret = m_streams[0]->read(buf, len);

			if ((ret < len) && m_streams[1])
			{
				m_streams[1]->seek(0, FILE_BEGIN);

				tStreamSize ret1 = m_streams[1]->read(buf+ret, len-ret);
				
				return ret + ret1;
			}

			return ret;
		}

		void incOffset(tStreamPos offset)
		{
			m_offset += offset;

			if (m_streams[0] &&
				m_offset >= m_streams[0]->size())
			{
				m_offset -= m_streams[0]->size();

				delete m_streams[0];
				
				if (m_streams[1])
				{
					m_streams[0] = m_streams[1];
					m_streams[1] = NULL;
				} else
				{
					m_streams[0] = NULL;
					m_offset = 0;
				}
			}
		}

		bool isEmpty()
		{
			return m_streams[0] == NULL && m_streams[1] == NULL;
		}

	private:
		bool m_disconnect;
		bool m_remoteDisconnect;
		tStreamPos m_offset;
		MFStream * m_streams[2];
	};


	class HTTPFilter : public IFilterBase
	{
	public:
		HTTPFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~HTTPFilter(void);

		eFilterResult tcp_packet(eDataDirection dd,
								ePacketDirection pd,
								const char * buf, int len);
		
		eFilterResult udp_packet(eDataDirection dd,
								ePacketDirection pd,
								const unsigned char * remoteAddress, 
								const char * buf, int len);

		PF_FilterType getType();

		PF_FilterCategory getCategory();
	
		bool postObject(PFObjectImpl & object);

		PF_FilterFlags getFlags()
		{
			return (PF_FilterFlags)m_flags;
		}

	protected:
		bool isHTTPRequest(const char * buf, int len);
		void pushRequestInfo(PFObject * pObject = NULL);
		void popRequestInfo();

	private:
		typedef std::queue<std::string> tRequestInfoQueue;
		tRequestInfoQueue m_requestInfoQueue;

		enum eParserState
		{
			STATE_NONE,
			STATE_CONTENT_PART,
			STATE_CONTENT,
			STATE_READ_ONLY,
			STATE_BYPASS,
			STATE_BLOCK,
			STATE_UPDATE_AND_BYPASS,
			STATE_UPDATE_AND_FILTER_READ_ONLY
		};

		ProxySession * m_pSession;
		bool m_bFirstPacket;
		GEN_SESSION_INFO m_si;

		HTTPPipelineBuffer m_requestBuffer;

		HTTPParser	m_requestParser;
		MFStream	m_requestPart;
		eParserState	m_requestState;

		HTTPParser	m_responseParser;
		MFStream	m_responsePart;
		eParserState	m_responseState;

		bool m_bypassNextRequestInfo;

		tPF_FilterFlags m_flags;
	};
}