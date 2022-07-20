// Saves classified objects to *.bin files.
//

#include "stdafx.h"
#include <crtdbg.h>
#include "ProtocolFilters.h"
#include "PFEventsDefault.h"

#pragma comment(lib,"ws2_32.lib")

using namespace nfapi;
using namespace ProtocolFilters;

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

std::string typeName(tPF_ObjectType t)
{
	switch (t)
	{
	case OT_HTTPS_PROXY_REQUEST:
		return "https_proxy_request";
	case OT_SOCKS4_REQUEST:
		return "socks4_proxy_request";
	case OT_SOCKS5_AUTH_REQUEST:
		return "socks5_proxy_auth_request";
	case OT_SOCKS5_AUTH_UNPW:
		return "socks5_proxy_unpw_request";
	case OT_SOCKS5_REQUEST:
		return "socks5_proxy_request";
	case OT_HTTP_REQUEST:
		return "http_request";
	case OT_HTTP_RESPONSE:
		return "http_response";
	case OT_POP3_MAIL_INCOMING:
		return "incoming_mail";
	case OT_SMTP_MAIL_OUTGOING:
		return "outgoing_mail";
	case OT_RAW_INCOMING:
		return "raw_in";
	case OT_RAW_OUTGOING:
		return "raw_out";
	case OT_FTP_COMMAND:
		return "ftp_command";
	case OT_FTP_RESPONSE:
		return "ftp_response";
	case OT_FTP_DATA_OUTGOING:
		return "ftp_data_outgoing";
	case OT_FTP_DATA_INCOMING:
		return "ftp_data_incoming";
	case OT_FTP_DATA_PART_OUTGOING:
		return "ftp_data_part_outgoing";
	case OT_FTP_DATA_PART_INCOMING:
		return "ftp_data_part_incoming";
	case OT_NNTP_ARTICLE:
		return "nntp_article";
	case OT_NNTP_POST:
		return "nntp_post";
	case OT_ICQ_LOGIN:
		return "icq_login";
	case OT_ICQ_CHAT_MESSAGE_OUTGOING:
		return "icq_chat_outgoing";
	case OT_ICQ_CHAT_MESSAGE_INCOMING:
		return "icq_chat_incoming";
	case OT_ICQ_REQUEST:
		return "icq_request";
	case OT_ICQ_RESPONSE:
		return "icq_response";
	case OT_XMPP_REQUEST:
		return "xmpp_request";
	case OT_XMPP_RESPONSE:
		return "xmpp_response";
	}
	return "";
}

void saveObject(ENDPOINT_ID id, PFObject * object)
{
	char fileName[_MAX_PATH];
	static int c = 0;
	tPF_ObjectType ot = object->getType();
	std::string t = typeName(ot);
	char tempBuf[1000];
	int tempLen;
	PFStream * pStream;

	c++;

	_snprintf(fileName, sizeof(fileName), "%I64u_%.8d_%s.bin", id, c, t.c_str());

	FILE * f = fopen(fileName, "wb");
	if (f)
	{
		for (int i=0; i<object->getStreamCount(); i++)
		{
			pStream = object->getStream(i);
			if (pStream)
			{
				pStream->seek(0, FILE_BEGIN);
				for (;;)
				{
					tempLen = pStream->read(tempBuf, sizeof(tempBuf));
					if (tempLen <= 0)
						break;

					fwrite(tempBuf, tempLen, 1, f);
				}
				pStream->seek(0, FILE_BEGIN);
			}

			if (ot == OT_FTP_DATA_INCOMING ||
				ot == OT_FTP_DATA_OUTGOING ||
				ot == OT_FTP_DATA_PART_INCOMING ||
				ot == OT_FTP_DATA_PART_OUTGOING ||
				ot == OT_ICQ_LOGIN ||
				ot == OT_ICQ_CHAT_MESSAGE_OUTGOING ||
				ot == OT_ICQ_CHAT_MESSAGE_INCOMING)
			{
				break;
			}
		}
		fclose(f);
	}
	
	if (object->getType() == OT_FTP_DATA_INCOMING ||
		object->getType() == OT_FTP_DATA_OUTGOING ||
		object->getType() == OT_FTP_DATA_PART_INCOMING ||
		object->getType() == OT_FTP_DATA_PART_OUTGOING)
	{
		_snprintf(fileName, sizeof(fileName), "%I64u_%.8d_%s.info", id, c, t.c_str());

		f = fopen(fileName, "wb");
		if (f)
		{
			pStream = object->getStream(1);
			if (pStream)
			{
				pStream->seek(0, FILE_BEGIN);
				for (;;)
				{
					tempLen = pStream->read(tempBuf, sizeof(tempBuf));
					if (tempLen <= 0)
						break;

					fwrite(tempBuf, tempLen, 1, f);
				}
				pStream->seek(0, FILE_BEGIN);
			}
			fclose(f);
		}
	} else
	if (object->getType() == OT_ICQ_LOGIN)
	{
		_snprintf(fileName, sizeof(fileName), "%I64u_%.8d_%s.info", id, c, t.c_str());

		f = fopen(fileName, "wb");
		if (f)
		{
			fwrite("User: ", 6, 1, f);

			pStream = object->getStream(ICQS_USER_UIN);
			if (pStream)
			{
				pStream->seek(0, FILE_BEGIN);
				for (;;)
				{
					tempLen = pStream->read(tempBuf, sizeof(tempBuf));
					if (tempLen <= 0)
						break;

					fwrite(tempBuf, tempLen, 1, f);
				}
				pStream->seek(0, FILE_BEGIN);
			}
			fclose(f);
		}
	} else
	if (object->getType() == OT_ICQ_CHAT_MESSAGE_OUTGOING ||
		object->getType() == OT_ICQ_CHAT_MESSAGE_INCOMING)
	{
		_snprintf(fileName, sizeof(fileName), "%I64u_%.8d_%s.info", id, c, t.c_str());

		f = fopen(fileName, "wb");
		if (f)
		{
			fwrite("User: ", 6, 1, f);

			pStream = object->getStream(ICQS_USER_UIN);
			if (pStream)
			{
				pStream->seek(0, FILE_BEGIN);
				for (;;)
				{
					tempLen = pStream->read(tempBuf, sizeof(tempBuf));
					if (tempLen <= 0)
						break;

					fwrite(tempBuf, tempLen, 1, f);
				}
				pStream->seek(0, FILE_BEGIN);
			}

			fwrite("\r\n", 2, 1, f);

			fwrite("Contact: ", 9, 1, f);

			pStream = object->getStream(ICQS_CONTACT_UIN);
			if (pStream)
			{
				pStream->seek(0, FILE_BEGIN);
				for (;;)
				{
					tempLen = pStream->read(tempBuf, sizeof(tempBuf));
					if (tempLen <= 0)
						break;

					fwrite(tempBuf, tempLen, 1, f);
				}
				pStream->seek(0, FILE_BEGIN);
			}

			fwrite("\r\n", 2, 1, f);

			fclose(f);
		}
	
		int textFormat = ICQTF_ANSI;

		pStream = object->getStream(ICQS_TEXT_FORMAT);
		if (pStream && (pStream->size() > 0))
		{
			pStream->seek(0, FILE_BEGIN);
			pStream->read(&textFormat, sizeof(textFormat));
		}

		pStream = object->getStream(ICQS_TEXT);
		if (pStream && (pStream->size() > 0))
		{
			_snprintf(fileName, sizeof(fileName), "%I64u_%.8d_%s_fmt%d.txt", id, c, t.c_str(), textFormat);

			f = fopen(fileName, "wb");
			if (f)
			{
				pStream->seek(0, FILE_BEGIN);
				for (;;)
				{
					tempLen = pStream->read(tempBuf, sizeof(tempBuf));
					if (tempLen <= 0)
						break;

					fwrite(tempBuf, tempLen, 1, f);
				}
				pStream->seek(0, FILE_BEGIN);

				fclose(f);
			}
		}
	}

}


class DumpFilter : public PFEventsDefault
{
public:
	DumpFilter()
	{
	}

	virtual void tcpConnected(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo)
	{
		if (pConnInfo->direction == NF_D_OUT)
		{
			pf_addFilter(id, FT_PROXY, FF_READ_ONLY_OUT | FF_READ_ONLY_IN);
			pf_addFilter(id, FT_SSL);
			pf_addFilter(id, FT_FTP, FF_SSL_TLS | FF_READ_ONLY_IN | FF_READ_ONLY_OUT);
			pf_addFilter(id, FT_HTTP, FF_READ_ONLY_OUT | FF_READ_ONLY_IN | FF_HTTP_BLOCK_SPDY);
			pf_addFilter(id, FT_POP3, FF_SSL_TLS | FF_READ_ONLY_IN);
			pf_addFilter(id, FT_SMTP, FF_SSL_TLS | FF_READ_ONLY_OUT);
			pf_addFilter(id, FT_NNTP, FF_READ_ONLY_OUT | FF_READ_ONLY_IN);
			pf_addFilter(id, FT_ICQ, FF_READ_ONLY_OUT | FF_READ_ONLY_IN);
			pf_addFilter(id, FT_XMPP, FF_SSL_TLS | FF_READ_ONLY_OUT | FF_READ_ONLY_IN);

			pf_addFilter(id, FT_RAW, FF_READ_ONLY_OUT | FF_READ_ONLY_IN);
		}
	}

	void dataAvailable(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		if (object->getStreamCount() > 0)
		{
			saveObject(id, object);
		}

		pf_postObject(id, object);
	}

};

int main(int argc, char* argv[])
{
	NF_RULE rule;

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

	printf("Press any key to stop...\n\n");

	DumpFilter f;

	if (!pf_init(&f, L"c:\\netfilter2"))
	{
		printf("Failed to initialize protocol filter");
		return -1;
	}

	pf_setRootSSLCertSubject("Sample CA");

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, pf_getNFEventHandler()) != NF_STATUS_SUCCESS)
	{
		printf("Failed to connect to driver");
		return -1;
	}

	// Filter all TCP connections
	memset(&rule, 0, sizeof(rule));
	rule.protocol = IPPROTO_TCP;
	rule.filteringFlag = NF_FILTER;
	nf_addRule(&rule, TRUE);

	// Block QUIC
	rule.protocol = IPPROTO_UDP;
	rule.remotePort = ntohs(80);
	rule.filteringFlag = NF_BLOCK;
	nf_addRule(&rule, TRUE);

	rule.protocol = IPPROTO_UDP;
	rule.remotePort = ntohs(443);
	rule.filteringFlag = NF_BLOCK;
	nf_addRule(&rule, TRUE);

	// Wait for any key
	getchar();

	// Free the libraries
	nf_free();
	pf_free();

	return 0;
}

