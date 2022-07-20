// Adds a prefix to the titles of HTML pages.
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

std::string g_titlePrefix;

class HttpFilter : public PFEventsDefault
{
public:
	HttpFilter()
	{
	}

	virtual void tcpConnected(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo)
	{
		if (pConnInfo->direction == NF_D_OUT)
		{
			pf_addFilter(id, FT_PROXY, FF_READ_ONLY_IN | FF_READ_ONLY_OUT);
			pf_addFilter(id, FT_SSL, FF_SSL_INDICATE_HANDSHAKE_REQUESTS | FF_SSL_VERIFY | FF_SSL_TLS_AUTO); 
			pf_addFilter(id, FT_HTTP, FF_HTTP_BLOCK_SPDY);
		}
	}

	bool updateContent(PFObject * object)
	{
		PFStream * pStream = object->getStream(HS_CONTENT);
		char * buf;
		bool contentUpdated = false;

		if (pStream && pStream->size() > 0)
		{
			buf = (char*)malloc((size_t)pStream->size() + 1);
			if (buf)
			{
				pStream->read(buf, (tStreamSize)pStream->size());
				buf[pStream->size()] = '\0';

				for (char * p = buf; *p; p++)
				{
					if (strnicmp(p, "<title", 6) == 0)
					{
						p = p + 6;
						if (p = strchr(p, '>'))
						{
							tStreamSize len = (tStreamSize)pStream->size();
							pStream->reset();
							pStream->write(buf, (tStreamSize)(p - buf + 1));
							pStream->write(g_titlePrefix.c_str(), (tStreamSize)g_titlePrefix.length());
							pStream->write(p + 1, len - (p + 1 - buf));
							
							contentUpdated = true;
						}
						break;
					}
				}
				free(buf);
			}
		}

		return contentUpdated;
	}

	void dataAvailable(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		if (object->isReadOnly())
			return;

		if ((object->getType() == OT_HTTP_RESPONSE) && 
			(object->getStreamCount() == 3))
		{
			PFHeader h;

			if (pf_readHeader(object->getStream(HS_HEADER), &h))
			{
				PFHeaderField * pField = h.findFirstField("Content-Type");
				if (pField)
				{
					if (pField->value().find("text/html") == -1)
					{
						pf_postObject(id, object);
						return;
					}
				}
			}

			updateContent(object);
		}

		pf_postObject(id, object);
	}

	PF_DATA_PART_CHECK_RESULT 
	dataPartAvailable(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		if (object->getType() == OT_SSL_HANDSHAKE_OUTGOING)
		{
			PFStream * pStream = object->getStream(0);
			char * buf;
			PF_DATA_PART_CHECK_RESULT res = DPCR_FILTER;

			if (pStream && pStream->size() > 0)
			{
				buf = (char*)malloc((size_t)pStream->size() + 1);
				if (buf)
				{
					pStream->read(buf, (tStreamSize)pStream->size());
					buf[pStream->size()] = '\0';

					if (strcmp(buf, "get.adobe.com") == 0)
					{
						res = DPCR_BYPASS;
					}

					free(buf);
				}
			}
			return res;
		}

		if (object->getType() == OT_HTTP_RESPONSE)
		{
			PFHeader h;

			if (pf_readHeader(object->getStream(HS_HEADER), &h))
			{
				PFHeaderField * pField = h.findFirstField("Content-Type");
				if (pField)
				{
					if (pField->value().find("text/html") != -1)
					{
						if (updateContent(object))
						{
							return DPCR_UPDATE_AND_BYPASS;
						} else
						{
							return DPCR_MORE_DATA_REQUIRED;
						}
					}
				}
			}
		}

		return DPCR_BYPASS;
	}
};

int main(int argc, char* argv[])
{
	NF_RULE rule;

	if (argc < 2)
	{
		printf("Usage: PFHttpContentFilter <string>\n" \
			"<string> : add this to titles of HTML pages\n");
		return -1;
	}
	
	g_titlePrefix = argv[1];
	g_titlePrefix += " ";
	
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	nf_adjustProcessPriviledges();

//	nf_setOptions(0, 0);

	printf("Press any key to stop...\n\n");

	HttpFilter f;

	if (!pf_init(&f, L"c:\\netfilter2"))
	{
		printf("Failed to initialize protocol filter");
		return -1;
	}

	pf_setExceptionsTimeout(EXC_GENERIC, 30);
	pf_setExceptionsTimeout(EXC_TLS, 30);
	pf_setExceptionsTimeout(EXC_CERT_REVOKED, 30);

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

	// Block QUIC
	rule.direction = NF_D_BOTH;

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

