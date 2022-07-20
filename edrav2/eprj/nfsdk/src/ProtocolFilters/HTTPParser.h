//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _HTTPPARSER
#define _HTTPPARSER

#include "IFilterBase.h"
#include "IOB.h"
#include "PFHeader.h"
#include "UnzipStream.h"
#include <string>

namespace ProtocolFilters
{
	class ProxySession;
	class HTTPFilter;

	enum tTransferEncoding
	{
		TE_NORMAL,
		TE_CHUNKED
	};

	enum tContentEncoding
	{
		CE_NOT_ENCODED,
		CE_GZIP
	};

	class HTTPParser
	{
	public:
		HTTPParser(PF_ObjectType ot);
		~HTTPParser(void);

		int inputPacket(const char * buffer, int len);
	
		void reset(bool keepBuffer);

		enum tState
		{
			STATE_NONE,
			STATE_HEADER,
			STATE_CONTENT,
			STATE_FINISHED,
			STATE_FINISHED_CONTINUE
		};

		tState getState();

		PFObjectImpl * getObject(bool checkContentLength = true);

		std::string getStatus();

		PFHeader & getHeader();

		void setExtraHeaders(const std::string & extraHeaders);

		int getHttpVersion();

		void skipObject();

		bool isIgnoreResponse()
		{
			return m_ignoreResponse;
		}

		void sendUpdatedObject(tStreamPos oldContentLength, ePacketDirection pd, ProxySession * pSession, HTTPFilter * pFilter);
		void sendContent(ePacketDirection pd, ProxySession * pSession, HTTPFilter * pFilter);

		tTransferEncoding getTransferEncoding()
		{
			return m_transferEncoding;
		}

	protected:
		bool parseHeader(const char * buffer, int len, bool hasContent);
		bool parseContent();
		bool saveContent(const char * buffer, int len);
		bool isHTTP(const char * buf, int len);

	private:
		tState	m_state;
		IOB		m_buffer;
		PFObjectImpl *	m_pObject;
		UnzipStream	m_uzStream;

		std::string m_status;
		PFHeader	m_header;
		std::string m_extraHeaders;
		tTransferEncoding m_transferEncoding;
		std::string m_sContentEncoding;
		int m_contentEncoding;
		__int64 m_contentLength;
		__int64 m_readContentLength;
		int m_httpVersion;
		bool m_waitingForClose;
		bool m_skipObject;
		bool m_isHtml;
		bool m_ignoreResponse;
		int m_sentContentLength;
	};
}

#endif