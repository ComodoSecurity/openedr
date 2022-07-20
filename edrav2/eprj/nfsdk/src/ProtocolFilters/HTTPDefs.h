//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _HTTPDEFS
#define _HTTPDEFS

namespace ProtocolFilters
{

#define HTTP_ACCEPT_RANGES_SZ           "Accept-Ranges"
#define HTTP_DATE_SZ                    "Date"
#define HTTP_EXPIRES_SZ                 "Expires"
#define HTTP_CONTENT_DISPOSITION_SZ     "Content-Disposition"
#define HTTP_CONTENT_LENGTH_SZ			"Content-Length"
#define HTTP_CONTENT_TYPE_SZ			"Content-Type"
#define HTTP_TRANSFER_ENCODING_SZ		"Transfer-Encoding"
#define HTTP_ACCEPT_ENCODING_SZ			"Accept-Encoding"
#define HTTP_CONTENT_ENCODING_SZ		"Content-Encoding"
#define GZIP_SZ							"gzip"
#define COMPRESS_SZ						"compress"
#define DEFLATE_SZ						"deflate"
#define X_GZIP_SZ						"x-gzip"
#define X_COMPRESS_SZ					"x-compress"
#define HTTP_LAST_MODIFIED_SZ           "Last-Modified"
#define HTTP_UNLESS_MODIFIED_SINCE_SZ   "Unless-Modified-Since"
#define HTTP_SERVER_SZ                  "Server"
#define HTTP_CONNECTION_SZ              "Connection"
#define HTTP_PROXY_CONNECTION_SZ        "Proxy-Connection"
#define HTTP_SET_COOKIE_SZ              "Set-Cookie"
#define CHUNKED_SZ                      "chunked"
#define KEEP_ALIVE_SZ                   "Keep-Alive"
#define CLOSE_SZ                        "Close"
#define BYTES_SZ                        "bytes"
#define HTTP_VIA_SZ                     "Via"
#define HTTP_CACHE_CONTROL_SZ           "Cache-Control"
#define HTTP_AGE_SZ                     "Age"
#define HTTP_VARY_SZ                    "Vary"
#define NO_CACHE_SZ                     "no-cache"
#define NO_STORE_SZ                     "no-store"
#define MUST_REVALIDATE_SZ              "must-revalidate"
#define MAX_AGE_SZ                      "max-age"
#define PRIVATE_SZ                      "private"
#define POSTCHECK_SZ                    "post-check"
#define PRECHECK_SZ                     "pre-check"
#define FILENAME_SZ                     "filename"
#define USER_AGENT_SZ                   "user-agent"
#define HTTP_HOST_SZ					"Host"
#define HTTP_DATE_SIZE  40
#define HTTP_GET_SZ						"GET"
#define HTTP_POST_SZ					"POST"


/**
*	Internal header, which is added to header when the filter is unable to decode
*	data and there is no need to encode the buffer before passing an object back to proxy.
*/
#define DECODE_FAILED_SZ	"decode_failed"

}

#endif //_HTTPDEFS