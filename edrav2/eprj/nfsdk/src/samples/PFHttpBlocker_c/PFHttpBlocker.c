// Blocks HTTP content by URL and text body.
//

#include "stdafx.h"
#include <crtdbg.h>
#define _C_API
#include "ProtocolFilters_c.h"

// Change this string after renaming and registering the driver under different name
#define NFDRIVER_NAME "netfilter2"

char g_blockString[256] = {0};

void threadStart()
{
}

void threadEnd()
{
}
	
void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
}

void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
}

void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	if (pConnInfo->direction == NF_D_OUT)
	{
		pfc_addFilter(id, FT_PROXY, FF_DEFAULT, OT_LAST, FT_NONE );
		pfc_addFilter(id, FT_SSL, FF_DEFAULT, OT_LAST, FT_NONE );
		pfc_addFilter(id, FT_HTTP, FF_HTTP_BLOCK_SPDY, OT_LAST, FT_NONE );
	} else
	{
		pfc_addFilter(id, FT_HTTP, FF_HTTP_BLOCK_SPDY, OT_LAST, FT_NONE );
	}
}

BOOL getHttpUrl(PFObject_c * object, char * pUrl, int urlLen)
{
	PPFHeader_c h;
	PPFStream_c pStream;
	char status[256];
	char * p;
	char * pEnd;
	tStreamSize len;

	if (PFObject_getType(object) != OT_HTTP_REQUEST)
		return FALSE;

	h = PFHeader_create();
	if (!h)
		return FALSE;
		
	if (pfc_readHeader(PFObject_getStream(object, HS_HEADER), h))
	{
		PPFHeaderField_c pField;
		
		pField = PFHeader_findFirstField(h, "Host");
		if (pField)
		{
			strcpy_s(pUrl, urlLen, "http://");
			strcat_s(pUrl, urlLen, PFHeaderField_getValue(pField));
		}
	}

	PFHeader_free(h);

	pStream = PFObject_getStream(object, HS_STATUS);
	if (!pStream)
		return FALSE;

	len = PFStream_read(pStream, status, (tStreamSize)sizeof(status)-1);
	status[len] = '\0';
	
	if (p = strchr(status, ' '))
	{
		p++;
		pEnd = strchr(p, ' ');
		if (pEnd)
		{
			if (strncmp(p, "http://", 7) == 0)
			{
				pUrl[0] = '0';
			}
			strncat_s(pUrl, urlLen, p, min(urlLen-strlen(pUrl)-1, pEnd-p+1));
		}
	}
	
	return TRUE;
}

void postBlockHttpResponse(ENDPOINT_ID id)
{
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
	PPFStream_c pStream;

	PPFObject_c newObj = PFObject_create(OT_HTTP_RESPONSE, 3);
	if (!newObj)
		return;

	pStream = PFObject_getStream(newObj, HS_STATUS);
	if (pStream)
	{
		PFStream_write(pStream, status, sizeof(status)-1);
	}

	pStream = PFObject_getStream(newObj, HS_HEADER);
	if (pStream)
	{
		char szLen[100];
		PPFHeader_c h = PFHeader_create();

		PFHeader_addField(h, "Content-Type", "text/html", TRUE);
		_snprintf(szLen, sizeof(szLen), "%d", sizeof(blockHtml)-1);
		PFHeader_addField(h,"Content-Length", szLen, TRUE);
		PFHeader_addField(h,"Connection", "close", TRUE);
		pfc_writeHeader(pStream, h);

		PFHeader_free(h);
	}

	pStream = PFObject_getStream(newObj, HS_CONTENT);
	if (pStream)
	{
		PFStream_write(pStream, blockHtml, sizeof(blockHtml)-1);
	}

	pfc_postObject(id, newObj);

	PFObject_free(newObj);
}

void dataAvailable(ENDPOINT_ID id, PFObject_c * object)
{
	if (PFObject_getType(object) == OT_HTTP_REQUEST)
	{
		char url[256];
		
		if (getHttpUrl(object, url, sizeof(url)))
		{
			strlwr(url);

			if (strstr(url, g_blockString))
			{
				postBlockHttpResponse(id);
				return;
			}
		} 
	}
	else
	if (PFObject_getType(object) == OT_HTTP_RESPONSE)
	{
		PPFStream_c pStream;
		PPFHeader_c h = PFHeader_create();
		char * buf;

		if (!h)
		{
			pfc_postObject(id, object);
			return;
		}

		pStream = PFObject_getStream(object, HS_HEADER);

		if (pfc_readHeader(pStream, h))
		{
			PFHeaderField_c * pField = PFHeader_findFirstField(h, "Content-Type");
			if (pField)
			{
				if (strstr(PFHeaderField_getValue(pField), "text/") == NULL)
				{
					pfc_postObject(id, object);
					PFHeader_free(h);
					return;
				}
			}
		}

		PFHeader_free(h);

		pStream = PFObject_getStream(object, HS_CONTENT);
		if (pStream && PFStream_size(pStream) > 0)
		{
			buf = (char*)malloc((size_t)PFStream_size(pStream) + 1);
			if (buf)
			{
				PFStream_read(pStream, buf, (tStreamSize)PFStream_size(pStream));
				buf[PFStream_size(pStream)] = '\0';
				
				strlwr(buf);

				if (strstr(buf, g_blockString))
				{
					postBlockHttpResponse(id);
					free(buf);
					return;
				}
				
				free(buf);
			}
		}
	}

	pfc_postObject(id, object);
}

PF_DATA_PART_CHECK_RESULT 
dataPartAvailable(ENDPOINT_ID id, PFObject_c * object)
{
	if (PFObject_getType(object) == OT_HTTP_RESPONSE)
	{
		PPFHeader_c h = PFHeader_create();
		if (!h)
		{
			return DPCR_BYPASS;
		}

		if (pfc_readHeader(PFObject_getStream(object, HS_HEADER), h))
		{
			PFHeaderField_c * pField = PFHeader_findFirstField(h, "Content-Type");
			if (pField)
			{
				if (strstr(PFHeaderField_getValue(pField), "text/"))
				{
					PFHeader_free(h);
					return DPCR_FILTER;
				}
			}
		}

		PFHeader_free(h);
	} else
	if (PFObject_getType(object) == OT_HTTP_REQUEST)
	{
		char url[256];
		
		if (getHttpUrl(object, url, sizeof(url)))
		{
			strlwr(url);

			if (strstr(url, g_blockString))
			{
				postBlockHttpResponse(id);
				return DPCR_BLOCK;
			}
		} 
	}

	return DPCR_BYPASS;
}

void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
}

void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
{
}

void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
}

NF_STATUS tcpPostSend(ENDPOINT_ID id, const char * buf, int len)
{
	NF_STATUS status = nf_tcpPostSend(id, buf, len);
	if (pfc_canDisableFiltering(id))
	{
		nf_tcpDisableFiltering(id);
	}
	return status;
}

NF_STATUS tcpPostReceive(ENDPOINT_ID id, const char * buf, int len)
{
	NF_STATUS status = nf_tcpPostReceive(id, buf, len);
	if (pfc_canDisableFiltering(id))
	{
		nf_tcpDisableFiltering(id);
	}
	return status;
}

NF_STATUS tcpSetConnectionState(ENDPOINT_ID id, int suspended)
{
	return nf_tcpSetConnectionState(id, suspended);
}

NF_STATUS udpPostSend(ENDPOINT_ID id, const unsigned char * remoteAddress, 
								const char * buf, int len, 
								PNF_UDP_OPTIONS options)
{
	return nf_udpPostSend(id, remoteAddress, buf, len, options);
}

NF_STATUS udpPostReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, 
		const char * buf, int len, PNF_UDP_OPTIONS options)
{
	return nf_udpPostReceive(id, remoteAddress, buf, len, options);
}

NF_STATUS udpSetConnectionState(ENDPOINT_ID id, int suspended)
{
	return nf_udpSetConnectionState(id, suspended);
}


int main(int argc, char* argv[])
{
	NF_RULE rule;
	PFEvents_c events = {
		threadStart,
		threadEnd,
		tcpConnectRequest,
		tcpConnected,
		tcpClosed,
		udpCreated,
		udpConnectRequest,
		udpClosed,
        dataAvailable,
		dataPartAvailable,
		tcpPostSend,
		tcpPostReceive,
		tcpSetConnectionState,
		udpPostSend,
		udpPostReceive,
		udpSetConnectionState
	};

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
	
	strcpy_s(g_blockString, sizeof(g_blockString), argv[1]);

	nf_adjustProcessPriviledges();

	printf("Press any key to stop...\n\n");

	if (!pfc_init(&events, L"c:\\netfilter2"))
	{
		printf("Failed to initialize protocol filter");
		return -1;
	}

	pfc_setRootSSLCertSubject("Sample CA");

	// Initialize the library and start filtering thread
	if (nf_init(NFDRIVER_NAME, pfc_getNFEventHandler()) != NF_STATUS_SUCCESS)
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
	pfc_free();

	return 0;
}

