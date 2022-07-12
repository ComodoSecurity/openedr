// Blocks HTTP content by URL and text body.
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

std::string g_blockString;

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
			pf_addFilter(id, FT_PROXY);
			pf_addFilter(id, FT_SSL, FF_SSL_VERIFY);
			pf_addFilter(id, FT_HTTP, FF_HTTP_BLOCK_SPDY);
		} else
		{
			pf_addFilter(id, FT_HTTP, FF_HTTP_BLOCK_SPDY);
		}
	}

	std::string getHttpUrl(PFObject * object)
	{
		PFHeader h;
		PFStream * pStream;
		std::string url;
		std::string status;
		char * p;

		if (object->getType() != OT_HTTP_REQUEST)
			return "";

		if (pf_readHeader(object->getStream(HS_HEADER), &h))
		{
			PFHeaderField * pField = h.findFirstField("Host");
			if (pField)
			{
				url = "http://" + pField->value();
			}
		}

		pStream = object->getStream(HS_STATUS);
		if (!pStream)
			return "";

		status.resize((unsigned int)pStream->size());
		pStream->read((char*)status.c_str(), (tStreamSize)status.length());
		
		if (p = strchr((char*)status.c_str(), ' '))
		{
			p++;
			char * pEnd = strchr(p, ' ');
			if (pEnd)
			{
				if (strncmp(p, "http://", 7) == 0)
				{
					url = "";
				}
				url.append(p, pEnd-p+1);
			}
		}
		
		return url;
	}

	void postBlockHttpResponse(nfapi::ENDPOINT_ID id)
	{
		PFObject * newObj = PFObject_create(OT_HTTP_RESPONSE, 3);
		if (!newObj)
			return;

		const char status[] = "HTTP/1.1 404 Not OK\r\n";
		const char blockHtml[] = "<html>" \
			"<body bgcolor=#f0f0f0><center><h1>Content blocked</h1></center></body></html>" \
            "<!-- - Unfortunately, Microsoft has added a clever new" \
            "   - 'feature' to Internet Explorer. If the text of" \
            "   - an error's message is 'too small', specifically" \
            "   - less than 512 bytes, Internet Explorer returns" \
            "   - its own error message. You can turn that off," \
            "   - but it's pretty tricky to find switch called" \
            "   - 'smart error messages'. That means, of course," \
            "   - that short error messages are censored by default." \
            "   - IIS always returns error messages that are long" \
            "   - enough to make Internet Explorer happy. The" \
            "   - workaround is pretty simple: pad the error" \
            "   - message with a big comment like this to push it" \
            "   - over the five hundred and twelve bytes minimum." \
            "   - Of course, that's exactly what you're reading" \
            "   - right now. -->";

		PFStream * pStream;
		
		pStream = newObj->getStream(HS_STATUS);
		if (pStream)
		{
			pStream->write(status, sizeof(status)-1);
		}

		pStream = newObj->getStream(HS_HEADER);
		if (pStream)
		{
			PFHeader h;

			h.addField("Content-Type", "text/html", true);
			char szLen[100];
			_snprintf(szLen, sizeof(szLen), "%d", sizeof(blockHtml)-1);
			h.addField("Content-Length", szLen, true);
			h.addField("Connection", "close", true);
			pf_writeHeader(pStream, &h);
		}

		pStream = newObj->getStream(HS_CONTENT);
		if (pStream)
		{
			pStream->write(blockHtml, sizeof(blockHtml)-1);
		}

		pf_postObject(id, newObj);

		newObj->free();
	}

	void dataAvailable(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		if (object->getType() == OT_HTTP_REQUEST)
		{
			std::string url = strToLower(getHttpUrl(object));

			if (url.find(g_blockString) != std::string::npos)
			{
				postBlockHttpResponse(id);
				return;
			}
		} 
		else
		if (object->getType() == OT_HTTP_RESPONSE)
		{
			PFHeader h;

			if (pf_readHeader(object->getStream(HS_HEADER), &h))
			{
				PFHeaderField * pField = h.findFirstField("Content-Type");
				if (pField)
				{
					if (pField->value().find("text/") == -1)
					{
						pf_postObject(id, object);
						return;
					}
				}
			}

			PFStream * pStream = object->getStream(HS_CONTENT);
			char * buf;

			if (pStream && pStream->size() > 0)
			{
				buf = (char*)malloc((size_t)pStream->size() + 1);
				if (buf)
				{
					pStream->read(buf, (tStreamSize)pStream->size());
					buf[pStream->size()] = '\0';
					
					strlwr(buf);

					if (strstr(buf, g_blockString.c_str()))
					{
						postBlockHttpResponse(id);
						free(buf);
						return;
					}
					
					free(buf);
				}
			}
		}

		pf_postObject(id, object);
	}

	PF_DATA_PART_CHECK_RESULT 
	dataPartAvailable(nfapi::ENDPOINT_ID id, PFObject * object)
	{
		if (object->getType() == OT_HTTP_RESPONSE)
		{
			PFHeader h;

			if (pf_readHeader(object->getStream(HS_HEADER), &h))
			{
				PFHeaderField * pField = h.findFirstField("Content-Type");
				if (pField)
				{
					if (pField->value().find("text/") != -1)
					{
						return DPCR_FILTER;
					}
				}
			}
		} else
		if (object->getType() == OT_HTTP_REQUEST)
		{
			std::string url = strToLower(getHttpUrl(object));

			if (url.find(g_blockString) != std::string::npos)
			{
				postBlockHttpResponse(id);
				return DPCR_BLOCK;
			}
		}

		return DPCR_BYPASS;
	}

};

int main(int argc, char* argv[])
{
	NF_RULE rule;

#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	if (argc < 2)
	{
		printf("Usage: PFHttpBlocker <string>\n" \
			"<string> : block HTTP requests with this string in Url, and HTTP responses with this string in text body\n");
		return -1;
	}
	
	g_blockString = strToLower(argv[1]);

	nf_adjustProcessPriviledges();

	printf("Press any key to stop...\n\n");

	HttpFilter f;

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
	rule.direction = NF_D_OUT | NF_D_IN;
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

