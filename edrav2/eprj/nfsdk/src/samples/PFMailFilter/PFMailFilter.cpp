//  The sample adds a prefix to the subjects of incoming messages, and blocks outgoing 
//  messages having the specified prefix in subject.

#include "stdafx.h"
#include <crtdbg.h>
#include "ProtocolFilters.h"
#include "PFEventsDefault.h"

using namespace nfapi;
using namespace ProtocolFilters;

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

std::string g_mailPrefix;

class MailFilter : public PFEventsDefault
{
public:
	MailFilter()
	{
	}

	virtual void tcpConnected(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo)
	{
		if (pConnInfo->direction == NF_D_OUT)
		{
			pf_addFilter(id, FT_PROXY);
			pf_addFilter(id, FT_SSL, FF_SSL_SUPPORT_CLIENT_CERTIFICATES);
			pf_addFilter(id, FT_POP3, FF_SSL_TLS | FF_SSL_SUPPORT_CLIENT_CERTIFICATES);
			pf_addFilter(id, FT_SMTP, FF_SSL_TLS | FF_SSL_SUPPORT_CLIENT_CERTIFICATES);
		}
	}

	bool filterOutgoingMail(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		PFHeader h;
		bool res = false;

		if (!pf_readHeader(object->getStream(0), &h))
			return false;

		PFHeaderField * pField = h.findFirstField("Subject");
		if (!pField)
			return false;

		std::string s = strToLower(pField->unfoldedValue());

		if (s.find(g_mailPrefix) != std::string::npos)
		{
			PFObject * newObj = PFObject_create(OT_RAW_INCOMING, 1);

			if (newObj)
			{
				PFStream * pStream = newObj->getStream(0);
				if (pStream)
				{
					const char blockCmd[] = "554 Message blocked!\r\n";
					pStream->write(blockCmd, sizeof(blockCmd)-1);
					pf_postObject(id, newObj);
					res = true;
				}
				newObj->free();
			}
		}

		return res;
	}

	void filterIncomingMail(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		PFHeader h;
		tStreamSize len;

		PFStream * pStream = object->getStream(0);
		if (!pStream)
			return;

		len = (tStreamSize)pStream->size();

		char * pContent = (char*)malloc(len + 1);
		if (!pContent)
			return;

		pStream->seek(0, FILE_BEGIN);
		pStream->read(pContent, len);
		pContent[len] = '\0';
		
		char * pDiv = strstr(pContent, "\r\n\r\n");
		if (!pDiv)
		{
			free(pContent);
			return;
		}

		if (!h.parseString(pContent, len))
		{
			free(pContent);
			return;
		}

		PFHeaderField * pField = h.findFirstField("Subject");
		if (!pField)
		{
			free(pContent);
			return;
		}

		std::string s = g_mailPrefix + " " + pField->unfoldedValue();

		h.removeFields("Subject", true);
		h.addField("Subject", s);

		pStream->reset();
		pf_writeHeader(pStream, &h);
		pDiv += 4;
		pStream->write(pDiv, len - (pDiv - pContent));

		free(pContent);
	}

	virtual void dataAvailable(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		bool blocked = false;

		switch (object->getType())
		{
		case OT_SMTP_MAIL_OUTGOING:
			blocked = filterOutgoingMail(id, object);
			break;

		case OT_POP3_MAIL_INCOMING:
			filterIncomingMail(id, object);
			break;
		}
		
		if (!blocked)
			pf_postObject(id, object);
	}

};

int main(int argc, char* argv[])
{
	NF_RULE rule;

	if (argc < 2)
	{
		printf("Usage: PFMailFilter <string>\n" \
			"<string> : add this to the subjects of incoming messages\n" \
			"	and block outgoing messages with modified subjects\n");
		return -1;
	}
	
	g_mailPrefix = argv[1];
	
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

	printf("Press any key to stop...\n\n");

	MailFilter mf;

	if (!pf_init(&mf, L"c:\\netfilter2"))
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
	rule.direction = NF_D_OUT;
	rule.protocol = IPPROTO_TCP;
	rule.filteringFlag = NF_FILTER;
	nf_addRule(&rule, TRUE);

	// Wait for any key
	getchar();

	// Free the libraries
	nf_free();
	pf_free();

	return 0;
}

