//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _PF_FILTER_DEFS
#define _PF_FILTER_DEFS

#if defined(__cplusplus) && !defined(_C_API)
namespace ProtocolFilters
{
#endif

	typedef enum _PF_FilterFlags
	{
		FF_DEFAULT = 0,
		
		// Generic filtering flags
		FF_DONT_FILTER_IN = 1,	// Passthrough incoming objects without filtering
		FF_DONT_FILTER_OUT = 2,	// Passthrough outgoing objects without filtering
		FF_READ_ONLY_IN = 4,	// Filter incoming objects in read-only mode
		FF_READ_ONLY_OUT = 8,	// Filter outgoing objects in read-only mode
	
		// SSL flags
	
		// Decode TLS sessions
		FF_SSL_TLS = 0x10,		
		
		// Generate self-signed certificates instead of using root CA as parent
		FF_SSL_SELF_SIGNED_CERTIFICATE = 0x20, 
		
		// Indicate OT_SSL_HANDSHAKE_OUTGOING/OT_SSL_HANDSHAKE_INCOMING via dataPartAvailable
		FF_SSL_INDICATE_HANDSHAKE_REQUESTS = 0x40, 
		
		// Try to detect TLS handshake automatically in first 8 kilobytes of packets
		FF_SSL_TLS_AUTO = 0x80,

		// Use RC4 for SSL sessions with local and remote endpoints
		FF_SSL_COMPATIBILITY = 0x100,
		
		// Verify server certificates and don't filter SSL if the certificate is not valid
		FF_SSL_VERIFY = 0x200,

		// Filter SSL connections in case when server requests a client certificate.
		// This method requires appropriate client certificates to be in 
		// Windows certificate storage with exportable private key.
		FF_SSL_SUPPORT_CLIENT_CERTIFICATES = 0x400,

		// Indicate OT_SSL_SERVER_CERTIFICATE via dataPartAvailable
		FF_SSL_INDICATE_SERVER_CERTIFICATES = 0x800,

		// Indicate OT_SSL_EXCEPTION via dataPartAvailable
		FF_SSL_INDICATE_EXCEPTIONS = 0x1000,

		// Support ALPN TLS extension for negotiating next protocols (HTTP/2,SPDY)
		FF_SSL_ENABLE_ALPN = 0x2000,

		// Indicate OT_SSL_CLIENT_CERT_REQUEST via dataPartAvailable
		FF_SSL_INDICATE_CLIENT_CERT_REQUESTS = 0x4000,

		// Don't encode the traffic between proxy and server
		FF_SSL_DECODE_ONLY = 0x8000,

		// Copy serial numbers from original server certificates
		FF_SSL_KEEP_SERIAL_NUMBERS = 0x10000,

		// Don't import self signed certificates to trusted storages
		FF_SSL_DONT_IMPORT_SELF_SIGNED = 0x20000,

		// Disable TLS 1.0 support
		FF_SSL_DISABLE_TLS_1_0 = 0x40000,

		// Disable TLS 1.1 support
		FF_SSL_DISABLE_TLS_1_1 = 0x80000,

		// HTTP flags
	
		// By default the filter sends pipelined requests by one, after receiving 
		// a response from server for a previous request. This flag instructs the filter 
		// to send all pipelined requests as-is.
		FF_HTTP_KEEP_PIPELINING = 0x10,	

		// Indicate via dataAvailable the objects OT_HTTP_SKIPPED_REQUEST_COMPLETE and
		// OT_HTTP_SKIPPED_RESPONSE_COMPLETE 
		FF_HTTP_INDICATE_SKIPPED_OBJECTS = 0x20,	

		// Block SPDY protocol
		FF_HTTP_BLOCK_SPDY = 0x40,	

	} PF_FilterFlags;

	typedef unsigned long tPF_FilterFlags;

	#define FT_STEP	100

	typedef enum _PF_FilterType
	{
		FT_NONE,			// Empty 
		FT_SSL		= FT_STEP,		// SSL decoder
		FT_HTTP		= 2 * FT_STEP,	// HTTP
		FT_POP3		= 3 * FT_STEP,	// POP3
		FT_SMTP		= 4 * FT_STEP,	// SMTP
		FT_PROXY	= 5 * FT_STEP,	// HTTP(S) proxy
		FT_RAW		= 6 * FT_STEP,  // Raw packets
		FT_FTP		= 7 * FT_STEP,	// FTP
		FT_FTP_DATA	= 8 * FT_STEP,	// FTP data
		FT_NNTP		= 9 * FT_STEP,	// NNTP
		FT_ICQ		= 10 * FT_STEP,	// ICQ
		FT_XMPP		= 11 * FT_STEP,	// XMPP
	} PF_FilterType;

	/**
	 *	The types of objects for filtering	
	 */
	typedef enum _PF_ObjectType
	{
		OT_NULL,
		/** Disconnect request from local peer */
		OT_TCP_DISCONNECT_LOCAL = 1,
		/** Disconnect request from remote peer */
		OT_TCP_DISCONNECT_REMOTE = 2,
		/** HTTP request object */
		OT_HTTP_REQUEST = FT_HTTP,
		/** HTTP response object */
		OT_HTTP_RESPONSE = FT_HTTP + 1,
		/** HTTP skipped request complete */
		OT_HTTP_SKIPPED_REQUEST_COMPLETE = FT_HTTP + 2,
		/** HTTP skipped response complete */
		OT_HTTP_SKIPPED_RESPONSE_COMPLETE = FT_HTTP + 3,
		/** POP3 messages */
		OT_POP3_MAIL_INCOMING = FT_POP3,
		/** SMTP messages */
		OT_SMTP_MAIL_OUTGOING = FT_SMTP,
		/** Request to HTTPS proxy */
		OT_HTTPS_PROXY_REQUEST = FT_PROXY,
		/** Request to SOCKS4 proxy */
		OT_SOCKS4_REQUEST = FT_PROXY + 1,
		/** Query authentication method from SOCKS5 proxy */
		OT_SOCKS5_AUTH_REQUEST = FT_PROXY + 2,
		/** Send user name and password to SOCKS5 proxy */
		OT_SOCKS5_AUTH_UNPW = FT_PROXY + 3,
		/** Request to SOCKS5 proxy */
		OT_SOCKS5_REQUEST = FT_PROXY + 4,
		/** Outgoing raw buffer */
		OT_RAW_OUTGOING = FT_RAW,
		/** Incoming raw buffer */
		OT_RAW_INCOMING = FT_RAW + 1,
		/** FTP command */
		OT_FTP_COMMAND = FT_FTP,
		/** FTP response */
		OT_FTP_RESPONSE = FT_FTP + 1,
		/** FTP outgoing data */
		OT_FTP_DATA_OUTGOING = FT_FTP_DATA,
		/** FTP incoming data */
		OT_FTP_DATA_INCOMING = FT_FTP_DATA + 1,
		/** FTP outgoing data part */
		OT_FTP_DATA_PART_OUTGOING = FT_FTP_DATA + 2,
		/** FTP incoming data part */
		OT_FTP_DATA_PART_INCOMING = FT_FTP_DATA + 3,
		/** News group article */
		OT_NNTP_ARTICLE = FT_NNTP,
		/** News group post */
		OT_NNTP_POST = FT_NNTP + 1,
		/** ICQ login */
		OT_ICQ_LOGIN = FT_ICQ,
		/** Outgoing ICQ data */
		OT_ICQ_REQUEST = FT_ICQ + 1,
		/** Incoming ICQ data */
		OT_ICQ_RESPONSE = FT_ICQ + 2,
		/** Outgoing ICQ chat message */
		OT_ICQ_CHAT_MESSAGE_OUTGOING = FT_ICQ + 3,
		/** Incoming ICQ chat message */
		OT_ICQ_CHAT_MESSAGE_INCOMING = FT_ICQ + 4,
		/** Outgoing SSL handshake request */
		OT_SSL_HANDSHAKE_OUTGOING = FT_SSL + 1,
		/** Incoming SSL handshake request */
		OT_SSL_HANDSHAKE_INCOMING = FT_SSL + 2,
		/** Invalid server SSL certificate */
		OT_SSL_INVALID_SERVER_CERTIFICATE = FT_SSL + 3,
		/** Server SSL certificate */
		OT_SSL_SERVER_CERTIFICATE = FT_SSL + 4,
		/** SSL exception */
		OT_SSL_EXCEPTION = FT_SSL + 5,
		/** SSL client certificate request */
		OT_SSL_CLIENT_CERT_REQUEST = FT_SSL + 6,
		/** XMPP request */
		OT_XMPP_REQUEST = FT_XMPP,
		/** XMPP response */
		OT_XMPP_RESPONSE = FT_XMPP + 1,
	} PF_ObjectType;

	/**
	 *	The possible values are listed in <tt>PF_ObjectType</tt>
	 */
	typedef int tPF_ObjectType;

	typedef enum _PF_DATA_PART_CHECK_RESULT
	{
		/** 
			Continue indicating the same object with more content via dataPartAvailable. 
		*/
		DPCR_MORE_DATA_REQUIRED,
	    /** 
			Stop calling dataPartAvailable, wait until receiving the full content
	        and indicate it via dataAvailable. 
		*/
		DPCR_FILTER,
		/** 
			Same as DPCR_FILTER, but the content goes to destination immediately,
			and the object in dataAvailable will have read-only flag. 
		*/
		DPCR_FILTER_READ_ONLY,
		/** 
			Do not call dataPartAvailable or dataAvailable for the current object,
			just passthrough it to destination.
		*/
		DPCR_BYPASS,
		/** 
			Block the transmittion of the current object.
		*/
		DPCR_BLOCK,
	    /** 
			Post the updated content in PFObject to session and bypass the rest of data as-is.
		*/
		DPCR_UPDATE_AND_BYPASS,
	    /** 
			Post the updated content in PFObject to session and indicate the full object via dataAvailable in read-only mode.
		*/
		DPCR_UPDATE_AND_FILTER_READ_ONLY,
	} PF_DATA_PART_CHECK_RESULT;

	typedef enum _PF_OpTarget
	{
		OT_FIRST,
		OT_PREV,
		OT_NEXT,
		OT_LAST
	} PF_OpTarget;

	// Filter category
	typedef enum _PF_FilterCategory
	{
		FC_PROTOCOL_FILTER,
		FC_PREPROCESSOR
	} PF_FilterCategory;

	// HTTP definitions

	/** Stream indices in HTTP object */
	typedef enum _ePF_HttpStream
	{
		/** First string of HTTP request or response */
		HS_STATUS = 0,
		/** HTTP header */
		HS_HEADER = 1,
		/** HTTP content */
		HS_CONTENT = 2
	} ePF_HttpStream;

	/** Custom HTTP headers for responses */

	// First string from appropriate HTTP request
	#define HTTP_EXHDR_RESPONSE_REQUEST	"X-EXHDR-REQUEST"
	// Host field from appropriate HTTP request header
	#define HTTP_EXHDR_RESPONSE_HOST	"X-EXHDR-REQUEST-HOST"

	// ICQ definitions

	typedef enum _ePF_ICQStream
	{
		ICQS_RAW = 0,
		ICQS_USER_UIN = 1,
		ICQS_CONTACT_UIN = 2,
		ICQS_TEXT_FORMAT = 3,
		ICQS_TEXT = 4,
		ICQS_MAX = 5
	} ePF_ICQStream;

	typedef enum _ePF_ICQTextFormat
	{
		ICQTF_ANSI = 0,
		ICQTF_UTF8 = 1,
		ICQTF_UNICODE = 2,
		ICQTF_FILE_TRANSFER = 3
	} ePF_ICQTextFormat;

	#define ICQ_FILE_COUNT	"File-Count"
	#define ICQ_TOTAL_BYTES	"Total-Bytes"
	#define ICQ_FILE_NAME	"File-Name"

	typedef enum _ePF_RootSSLImportFlag
	{
		RSIF_DONT_IMPORT = 0,
		RSIF_IMPORT_TO_MOZILLA_AND_OPERA = 1,
		RSIF_IMPORT_TO_PIDGIN = 2,
		RSIF_IMPORT_EVERYWHERE = 3,
		RSIF_GENERATE_ROOT_PRIVATE_KEY = 4,
	} ePF_RootSSLImportFlag;
	
	/** Stream indices in OT_SSL_SERVER_CERTIFICATE object */
	typedef enum _ePF_SSL_ServerCertStream
	{
		/** Server certificate bytes */
		SSL_SCS_CERTIFICATE = 0,
		/** Certificate subject name */
		SSL_SCS_SUBJECT = 1,
		/** Certificate issuer name */
		SSL_SCS_ISSUER = 2
	} ePF_SSL_ServerCertStream;

	/** Class of SSL exceptions */
	typedef enum _eEXCEPTION_CLASS
	{
		// Generic exceptions generated because of unexpected disconnect during handshake
		EXC_GENERIC = 0,		
		// TLS exceptions, switching version of TLS protocol
		EXC_TLS = 1,			
		// Certificate revokation exceptions
		EXC_CERT_REVOKED = 2,	
		EXC_MAX
	} eEXCEPTION_CLASS;

#if defined(__cplusplus) && !defined(_C_API)
}
#endif

#endif