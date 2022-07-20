unit ProtocolFiltersAPI;

interface

uses NetFilter2API;

const

  protocolfilters_module = 'ProtocolFilters.dll';

  { PF_FilterFlags }
  FF_DEFAULT = 0;
  // Generic filtering flags
  FF_DONT_FILTER_IN = 1;        // Passthrough incoming objects without filtering
  FF_DONT_FILTER_OUT = 2;       // Passthrough outgoing objects without filtering
  FF_READ_ONLY_IN = 4;          // Filter incoming objects in read-only mode
  FF_READ_ONLY_OUT = 8;         // Filter outgoing objects in read-only mode
  // SSL flags

  // Decode TLS sessions
  FF_SSL_TLS = 16;
  // Generate self-signed certificate instead of using root CA
  FF_SSL_SELF_SIGNED_CERTIFICATE = 32;
  // Indicate OT_SSL_HANDSHAKE_OUTGOING/OT_SSL_HANDSHAKE_INCOMING via dataPartAvailable
  FF_SSL_INDICATE_HANDSHAKE_REQUESTS = 64;
  // Try to detect TLS handshake automatically in first 8 kilobytes of packets
  FF_SSL_TLS_AUTO = 128;
  // Use RC4 for SSL sessions with local and remote endpoints
  FF_SSL_COMPATIBILITY = 256;
  // Verify server certificates and don't filter SSL if the certificate is not valid
  FF_SSL_VERIFY = 512;
  // Filter SSL connections in case when server requests a client certificate.
  // This method requires appropriate client certificates to be in
  // Windows certificate storage with exportable private key.
  FF_SSL_SUPPORT_CLIENT_CERTIFICATES = 1024;
  // Indicate OT_SSL_SERVER_CERTIFICATE via dataPartAvailable
  FF_SSL_INDICATE_SERVER_CERTIFICATES = 2048;
  // Indicate OT_SSL_EXCEPTION via dataPartAvailable
  FF_SSL_INDICATE_EXCEPTIONS = 4096;
  // Support ALPN TLS extension for negotiating next protocols (HTTP/2,SPDY)
  FF_SSL_ENABLE_ALPN = 8192;
  // Indicate OT_SSL_CLIENT_CERT_REQUEST via dataPartAvailable
  FF_SSL_INDICATE_CLIENT_CERT_REQUESTS = 16384;
  // Don't encode the traffic between proxy and server
  FF_SSL_DECODE_ONLY = 32768;
  // Copy serial numbers from original server certificates
  FF_SSL_KEEP_SERIAL_NUMBERS = 65536;
  // Don't import self signed certificates to trusted storages
  FF_SSL_DONT_IMPORT_SELF_SIGNED = $20000;
  // Disable TLS 1.0 support
  FF_SSL_DISABLE_TLS_1_0 = $40000;
  // Disable TLS 1.1 support
  FF_SSL_DISABLE_TLS_1_1 = $80000;
                                    	
  // HTTP flags

  // By default the filter sends pipelined requests by one, after receiving 
  // a response from server for a previous request. This flag instructs the filter 
  // to send all pipelined requests as-is.
  FF_HTTP_KEEP_PIPELINING = 16;
  // Indicate via dataAvailable the objects OT_HTTP_SKIPPED_REQUEST_COMPLETE and
  // OT_HTTP_SKIPPED_RESPONSE_COMPLETE 
  FF_HTTP_INDICATE_SKIPPED_OBJECTS = 32;

  // Block SPDY protocol
  FF_HTTP_BLOCK_SPDY = 64;
  
  { PF_FilterType }
  FT_STEP = 100;

  FT_NONE = 0;		        // Empty
  FT_SSL  = FT_STEP;	        // SSL decoder/encoder
  FT_HTTP = 2 * FT_STEP;        // HTTP
  FT_POP3 = 3 * FT_STEP;        // POP3
  FT_SMTP = 4 * FT_STEP;	// SMTP
  FT_PROXY = 5 * FT_STEP;	// HTTPS and SOCK4/5 proxy
  FT_RAW = 6 * FT_STEP;         // Raw buffers
  FT_FTP = 7 * FT_STEP;	        // FTP
  FT_FTP_DATA = 8 * FT_STEP;	// FTP data
  FT_NNTP = 9 * FT_STEP;	// NNTP
  FT_ICQ = 10 * FT_STEP;	// ICQ
  FT_XMPP = 11 * FT_STEP;	// XMPP

  { PF_ObjectType }

  OT_NULL = 0;

  // Disconnect request from local peer
  OT_TCP_DISCONNECT_LOCAL = 1;
  // Disconnect request from remote peer
  OT_TCP_DISCONNECT_REMOTE = 2;
  // HTTP request object
  OT_HTTP_REQUEST = FT_HTTP;
  // HTTP response object
  OT_HTTP_RESPONSE = FT_HTTP + 1;
  // POP3 messages
  OT_POP3_MAIL_INCOMING = FT_POP3;
  // SMTP messages
  OT_SMTP_MAIL_OUTGOING = FT_SMTP;
  // Request to HTTPS proxy
  OT_HTTPS_PROXY_REQUEST = FT_PROXY;
  // Request to SOCKS4 proxy
  OT_SOCKS4_REQUEST = FT_PROXY + 1;
  // Query authentication method from SOCKS5 proxy
  OT_SOCKS5_AUTH_REQUEST = FT_PROXY + 2;
  // Send user name and password to SOCKS5 proxy
  OT_SOCKS5_AUTH_UNPW_NEGOTIATE = FT_PROXY + 3;
  // Request to SOCKS5 proxy
  OT_SOCKS5_REQUEST = FT_PROXY + 4;
  // Outgoing raw buffer
  OT_RAW_OUTGOING = FT_RAW;
  // Incoming raw buffer
  OT_RAW_INCOMING = FT_RAW + 1;
  // FTP command
  OT_FTP_COMMAND = FT_FTP;
  // FTP response
  OT_FTP_RESPONSE = FT_FTP + 1;
  // FTP outgoing data
  OT_FTP_DATA_OUTGOING = FT_FTP_DATA;
  // FTP incoming data
  OT_FTP_DATA_INCOMING = FT_FTP_DATA + 1;
  // FTP outgoing data part
  OT_FTP_DATA_PART_OUTGOING = FT_FTP_DATA + 2;
  // FTP incoming data part
  OT_FTP_DATA_PART_INCOMING = FT_FTP_DATA + 3;
  // News group article
  OT_NNTP_ARTICLE = FT_NNTP;
  // News group post
  OT_NNTP_POST = FT_NNTP + 1;
  // ICQ login
  OT_ICQ_LOGIN = FT_ICQ;
  // Outgoing ICQ data
  OT_ICQ_REQUEST = FT_ICQ + 1;
  // Incoming ICQ data
  OT_ICQ_RESPONSE = FT_ICQ + 2;
  // Outgoing ICQ chat message *
  OT_ICQ_CHAT_MESSAGE_OUTGOING = FT_ICQ + 3;
  // Incoming ICQ chat message
  OT_ICQ_CHAT_MESSAGE_INCOMING = FT_ICQ + 4;
  // Outgoing SSL handshake request 
  OT_SSL_HANDSHAKE_OUTGOING = FT_SSL + 1;
  // Incoming SSL handshake request
  OT_SSL_HANDSHAKE_INCOMING = FT_SSL + 2;
  // Invalid server SSL certificate
  OT_SSL_INVALID_SERVER_CERTIFICATE = FT_SSL + 3;
  // Server SSL certificate
  OT_SSL_SERVER_CERTIFICATE = FT_SSL + 4;
  // SSL exception
  OT_SSL_EXCEPTION = FT_SSL + 5;
  // SSL client certificate request 
  OT_SSL_CLIENT_CERT_REQUEST = FT_SSL + 6;
  // XMPP request
  OT_XMPP_REQUEST = FT_XMPP;
  // XMPP response
  OT_XMPP_RESPONSE = FT_XMPP + 1;

  { PF_OpTarget }
  OT_FIRST = 0;
  OT_PREV = 1;
  OT_NEXT = 2;
  OT_LAST = 3;

  { PF_DATA_PART_CHECK_RESULT }
  // Continue indicating the same object with more content via dataPartAvailable.
  DPCR_MORE_DATA_REQUIRED = 0;
  // Stop calling dataPartAvailable, wait until receiving the full content
  // and indicate it via dataAvailable.
  DPCR_FILTER = 1;
  // Same as DPCR_FILTER, but the content goes to destination immediately,
  // and the object in dataAvailable will have read-only flag.
  DPCR_FILTER_READ_ONLY = 2;
  // Do not call dataPartAvailable or dataAvailable for the current object,
  // just passthrough it to destination.
  DPCR_BYPASS = 3;
  // Block the transmittion of the current object.
  DPCR_BLOCK = 4;
  // Post the updated content in PFObject to session and bypass the rest of data as-is.
  DPCR_UPDATE_AND_BYPASS = 5;
  // Post the updated content in PFObject to session and indicate the full object via dataAvailable in read-only mode.
  DPCR_UPDATE_AND_FILTER_READ_ONLY = 6;

  { PF_StreamOrigin }
  FILE_BEGIN = 0;       // Stream begin
  FILE_CURRENT = 1;     // Current position
  FILE_END = 2;         // Stream end

  // HTTP constants

  { PF_HttpStream - stream indexes in HTTP object }
  HS_STATUS = 0;  // First string of HTTP request or response
  HS_HEADER = 1;  // HTTP header
  HS_CONTENT = 2; // HTTP content

  { Custom HTTP headers for responses }
  // First string from HTTP request
  HTTP_EXHDR_RESPONSE_REQUEST = 'X-EXHDR-REQUEST';
  // Host field from HTTP request
  HTTP_EXHDR_RESPONSE_HOST = 'X-EXHDR-REQUEST-HOST';

  { ICQ constants }

  // ICQ stream indices
  ICQS_RAW = 0;
  ICQS_USER_UIN = 1;
  ICQS_CONTACT_UIN = 2;
  ICQS_TEXT_FORMAT = 3;
  ICQS_TEXT = 4;

  // ICQ text format
  ICQTF_ANSI = 0;
  ICQTF_UTF8 = 1;
  ICQTF_UNICODE = 2;
  ICQTF_FILE_TRANSFER = 3;

  ICQ_FILE_COUNT = 'File-Count';
  ICQ_TOTAL_BYTES = 'Total-Bytes';
  ICQ_FILE_NAME = 'File-Name';

  // ePF_RootSSLImportFlag
  RSIF_DONT_IMPORT = 0;
  RSIF_IMPORT_TO_MOZILLA_AND_OPERA = 1;
  RSIF_IMPORT_TO_PIDGIN = 2;
  RSIF_IMPORT_EVERYWHERE = 3;
  RSIF_GENERATE_ROOT_PRIVATE_KEY = 4;

  // Stream indices in OT_SSL_SERVER_CERTIFICATE object

  // Server certificate bytes
  SSL_SCS_CERTIFICATE = 0;
  // Certificate subject name
  SSL_SCS_SUBJECT = 1;
  // Certificate issuer name
  SSL_SCS_ISSUER = 2;

	// Class of SSL exceptions

  // Generic exceptions generated because of unexpected disconnect during handshake
	EXC_GENERIC = 0;
	// TLS exceptions, switching version of TLS protocol
	EXC_TLS = 1;
	// Certificate revokation exceptions
	EXC_CERT_REVOKED = 2;

type

  ENDPOINT_ID = Int64;

  PF_BOOL = integer;

  PF_FilterFlags = integer;
  PF_FilterType = integer;
  PF_ObjectType = integer;
  PF_DATA_PART_CHECK_RESULT = integer;
  PF_OpTarget = integer;
  tStreamOrigin = integer;

  PNF_EventHandler = ^NF_EventHandler;

  // PFStream
  tStreamPos = Int64;
  tStreamSize = Longword;
  PPFStream_c = pointer;

  // PFObject
  PPFObject_c = pointer;

  // PFHeader
  PPFHeaderField_c = pointer;
  PPFHeader_c = pointer;

  ///////////////////////////////////////////////////////////////////////////////////
  // ProtocolFilters events
  PFEvents_c = packed record
        // The events from NF_EventHandler
	threadStart : procedure(); cdecl;
	threadEnd : procedure(); cdecl;
	tcpConnectRequest : procedure(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
	tcpConnected : procedure(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
	tcpClosed : procedure(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
	udpCreated : procedure(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
	udpConnectRequest : procedure(id : ENDPOINT_ID; var pConnReq : NF_UDP_CONN_REQUEST); cdecl;
	udpClosed : procedure(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;

        // New object is ready for filtering
        dataAvailable : procedure (id : ENDPOINT_ID; pObject : PPFObject_c); cdecl;

        // A part of content is available for examining.
        // The function must return one of PF_DATA_PART_CHECK_RESULT
        // values to instruct the filter what to do with the rest of content.
        // The object passed to this function is read-only. It means that
        // passing it back to filter using pf_postObject has no effect.
        dataPartAvailable : function (id : ENDPOINT_ID; pObject : PPFObject_c) : PF_DATA_PART_CHECK_RESULT; cdecl;

        // The library calls these functions to post the filtered data buffers
        // to destination, and to control the state of filtered connections.
        tcpPostSend : function (id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl;
        tcpPostReceive : function (id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl;
        tcpSetConnectionState : function (id : ENDPOINT_ID; suspended : integer): integer; cdecl;

        udpPostSend : function (id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl;
        udpPostReceive : function (id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl;
        udpSetConnectionState : function (id : ENDPOINT_ID; suspended : integer): integer; cdecl;
  end;

  ///////////////////////////////////////////////////////////////
  // Delphi wrappers over C API 

  TPFStream = class;

  TPFHeader = class
  private
        // C API PFHeader pointer
        m_pHeader : PPFHeader_c;

  protected
        // Returns number of fields in a header
        function getSize() : integer;

  public
        constructor Create(); overload;
        constructor Create(pHeader : PPFHeader_c); overload;
        constructor Create(buf : PAnsiChar; len : integer); overload;
        destructor Destroy(); override;

        function readFromStream(pStream : TPFStream) : Longbool;
        function writeToStream(pStream : TPFStream) : Longbool;

        // Adds a new field to header start or beginning
        procedure addField(const name : AnsiString; const value : AnsiString; toStart : Longbool);
        // Removes fields with specified name. If exact is false, the function
        // compares name with the first symbols of each field name and removes matches.
        procedure removeFields(const name : AnsiString; exact : Longbool);
        // Removes all fields
        procedure clear();
        function getFieldName(index : integer) : AnsiString;
        function getFieldValue(const name : AnsiString) : AnsiString; overload;
        function getFieldValue(index : integer) : AnsiString; overload;
        // Returns number of fields in a header
        property size : integer read getSize;
        // Returns C API object pointer
        property baseObject : PPFHeader_c read m_pHeader;
  end;

  TPFStream = class
  private
        m_pStream : PPFStream_c;
  public
        constructor Create(pStream : PPFStream_c); overload;
        destructor Destroy(); override;

        // Moves the stream pointer to a specified location.
        function seek(offset : tStreamPos; origin : tStreamOrigin) : tStreamPos;
        // Reads data from a stream and returns the number of bytes actually read.
        function read(buffer : PAnsiChar; size : tStreamSize) : tStreamSize;
        // Writes data to a stream and returns the number of bytes actually written.
        function write(buffer : PAnsiChar; size : tStreamSize) : tStreamSize;
        // Returns the number of bytes in stream.
        function size() : tStreamPos;
        // Set the stream end at the current position.
        function setEndOfStream() : tStreamPos;
        // Clear the stream contents.
        procedure reset();
        // Returns C API object pointer
        property baseObject : PPFStream_c read m_pStream;
  end;

  TPFObject = class
  private
        m_pObject : PPFObject_c;
        m_streams : array of TPFStream;

  protected
        function getType() : PF_ObjectType;
        procedure setType(value : PF_ObjectType);
        function isReadOnly() : Longbool;
        procedure setReadOnly(value : Longbool);
        function getStreamCount() : integer;

        procedure createStreamWrappers;
        procedure freeStreamWrappers;
  public
        constructor Create(pObject : PPFObject_c); overload;
        constructor Create(objectType : PF_ObjectType; nStreams : integer); overload;
        destructor Destroy(); override;

        function getStream(index : integer) : TPFStream;
        procedure clear();
        function clone() : TPFObject;
        function detach() : TPFObject;

        // Post object to the specified endpoint
        function post(id : ENDPOINT_ID) : PF_BOOL;

        property objectType : PF_ObjectType read getType write setType;
        property readOnly : Longbool read isReadOnly write setReadOnly;
        property streamCount : integer read getStreamCount;
        property streams[index : integer] : TPFStream read getStream; default;
        // Returns C API object pointer
        property baseObject : PPFObject_c read m_pObject;
  end;

  ///////////////////////////////////////////////////////////////////////////////////
  // PFStream

  // Moves the stream pointer to a specified location.
  function PFStream_seek(s : PPFStream_c; offset : tStreamPos; origin : tStreamOrigin) : tStreamPos; cdecl; external protocolfilters_module;
  // Reads data from a stream and returns the number of bytes actually read.
  function PFStream_read(s : PPFStream_c; buffer : PAnsiChar; size : tStreamSize) : tStreamSize; cdecl; external protocolfilters_module;
  // Writes data to a stream and returns the number of bytes actually written.
  function PFStream_write(s : PPFStream_c; buffer : PAnsiChar; size : tStreamSize) : tStreamSize; cdecl; external protocolfilters_module;
  // Returns the number of bytes in stream.
  function PFStream_size(s : PPFStream_c) : tStreamPos; cdecl; external protocolfilters_module;
  // Set the stream end at the current position.
  function PFStream_setEndOfStream(s : PPFStream_c) : tStreamPos; cdecl; external protocolfilters_module;
  // Clear the stream contents.
  procedure PFStream_reset(s : PPFStream_c); cdecl; external protocolfilters_module;

  ///////////////////////////////////////////////////////////////////////////////////
  // PFObject

  // Creates a new object of the specified type, with specified number of streams.
  function PFObject_create(objectType : PF_ObjectType; nStreams : integer) : PPFObject_c; cdecl; external protocolfilters_module;
  // Free the object created with PFObject_create.
  procedure PFObject_free(pObject : PPFObject_c); cdecl; external protocolfilters_module;
  // Returns object type.
  function PFObject_getType(pObject : PPFObject_c) : PF_ObjectType; cdecl; external protocolfilters_module;
  // Assigns a type
  procedure PFObject_setType(pObject : PPFObject_c; value : PF_ObjectType); cdecl; external protocolfilters_module;
  // Returns true if the object has read-only flag
  function PFObject_isReadOnly(pObject : PPFObject_c) : Longbool; cdecl; external protocolfilters_module;
  // Assigns read-only flag
  procedure PFObject_setReadOnly(pObject : PPFObject_c; value : Longbool); external protocolfilters_module;
  // Returns the number of streams in object.
  function PFObject_getStreamCount(pObject : PPFObject_c) : integer; cdecl; external protocolfilters_module;
  // Returns a stream at the specified position.
  function PFObject_getStream(pObject : PPFObject_c; index : integer) : PPFStream_c; cdecl; external protocolfilters_module;
  // Clears all object streams.
  procedure PFObject_clear(pObject : PPFObject_c); cdecl; external protocolfilters_module;
  // Creates a copy of the specified object
  function PFObject_clone(pObject : PPFObject_c) : PPFObject_c; cdecl; external protocolfilters_module;
  // Creates a copy of the specified object by moving all streams of old object to new one.
  function PFObject_detach(pObject : PPFObject_c) : PPFObject_c; cdecl; external protocolfilters_module;


  ///////////////////////////////////////////////////////////////////////////////////
  // PFHeaderField

  // Returns header field name
  function PFHeaderField_getName(hf : PPFHeaderField_c) : PAnsiChar; cdecl; external protocolfilters_module;
  // Returns header field value
  function PFHeaderField_getValue(hf : PPFHeaderField_c) : PAnsiChar; cdecl; external protocolfilters_module;
  // Returns header field value with stripped tabs and \r\n sequences
  function PFHeaderField_getUnfoldedValue(hf : PPFHeaderField_c) : PAnsiChar; cdecl; external protocolfilters_module;

  ///////////////////////////////////////////////////////////////////////////////////
  // PFHeader

  // Creates a new header
  function PFHeader_create() : PPFHeader_c; cdecl; external protocolfilters_module;
  // Creates a new header and adds the fields from the specified buffer.
  function PFHeader_parse(buf : PAnsiChar; len : integer) : PPFHeader_c; cdecl; external protocolfilters_module;
  // Deletes the header
  procedure PFHeader_free(ph : PPFHeader_c); cdecl; external protocolfilters_module;
  // Adds a new field to header start or beginning
  procedure PFHeader_addField(ph : PPFHeader_c; name : PAnsiChar; value : PAnsiChar; toStart : Longbool); cdecl; external protocolfilters_module;
  // Removes fields with specified name. If exact is false, the function
  // compares name with the first symbols of each field name and removes matches.
  procedure PFHeader_removeFields(ph : PPFHeader_c; name : PAnsiChar; exact : Longbool); cdecl; external protocolfilters_module;
  // Removes all fields
  procedure PFHeader_clear(ph : PPFHeader_c); cdecl; external protocolfilters_module;
  // Returns a first field having specified name
  function PFHeader_findFirstField(ph : PPFHeader_c; name : PAnsiChar) : PPFHeaderField_c; cdecl; external protocolfilters_module;
  // Returns number of fields in a header
  function PFHeader_size(ph : PPFHeader_c) : integer; cdecl; external protocolfilters_module;
  // Returns a fields at the specified position
  function PFHeader_getField(ph : PPFHeader_c; index : integer) : PPFHeaderField_c; cdecl; external protocolfilters_module;

  ///////////////////////////////////////////////////////////////////////////////////
  // Main API

  // Initialize the library
  // pHandler - event handler
  // dataFolder - a path to configuration folder, where the library
  // stores SSL certificates and temporary files.
  function pfc_init(var pHandler : PFEvents_c; dataFolder : PWideChar) : PF_BOOL; cdecl; external protocolfilters_module;

  // Free the library
  procedure pfc_free(); cdecl; external protocolfilters_module;

  // Returns NF_EventHandler for passing to nf_init
  function pfc_getNFEventHandler() : PNF_EventHandler; cdecl; external protocolfilters_module;

  // Post an object to the specified endpoint
  function pfc_postObject(id : ENDPOINT_ID; pObject : PPFObject_c) : PF_BOOL; cdecl; external protocolfilters_module;

  // Adds a new filter with the specified properties to filtering chain.
  function pfc_addFilter(id : ENDPOINT_ID;
                        ft : PF_FilterType;
			flags : PF_FilterFlags;
			target : PF_OpTarget;
			typeBase : PF_FilterType) : PF_BOOL;
                        cdecl; external protocolfilters_module;

  // Removes a filter of the specified type.
  function pfc_deleteFilter(id : ENDPOINT_ID; ft : PF_FilterType) : PF_BOOL; cdecl; external protocolfilters_module;

  // Returns the number of active filters for the specified connection.
  function pfc_getFilterCount(id : ENDPOINT_ID) : integer; cdecl; external protocolfilters_module;

  // Returns TRUE if there is a filter of the specified type in filtering chain.
  function pfc_isFilterActive(id : ENDPOINT_ID; ft : PF_FilterType) : PF_BOOL; cdecl; external protocolfilters_module;

  // Returns true when it is safe to disable filtering for the connection
  // with specified id (there are no filters in chain and internal buffers are empty).
  function pfc_canDisableFiltering(id : ENDPOINT_ID) : PF_BOOL; cdecl; external protocolfilters_module;

  // Set the issuer name of a root CA certificate for generating SSL certificates.
  procedure pfc_setRootSSLCertSubject(rootSubject : PAnsiChar); cdecl; external protocolfilters_module;
  procedure pfc_setRootSSLCertSubjectEx(rootSubject : PAnsiChar; x509 : PAnsiChar; x509Len : integer; pkey : PAnsiChar; pkeyLen : integer); cdecl; external protocolfilters_module;

  // Specifies import flags from ePF_RootSSLImportFlag enumeration, allowing to control
  // Importing root SSL certificate in pfc_setRootSSLCertSubject to supported storages.
  // Default value - RSIF_IMPORT_EVERYWHERE.
  procedure pfc_setRootSSLCertImportFlags(flags : integer); cdecl; external protocolfilters_module;

  function pfc_getRootSSLCertFileName(fileName : PWideChar; fileNameLen : integer) : PF_BOOL; cdecl; external protocolfilters_module;

  // Support functions

  // Returns a process owner name by id, ansi version
  function pfc_getProcessOwnerA(processId : Longword; buf : PAnsiChar; len : integer) : LongBool; cdecl; external protocolfilters_module;
  // Returns a process owner name by id, unicode version
  function pfc_getProcessOwnerW(processId : Longword; buf : PWideChar; len : integer) : LongBool; cdecl; external protocolfilters_module;

  // Reads header from the specified stream
  function pfc_readHeader(pStream : PPFStream_c; ph : PPFHeader_c) : Longbool; cdecl; external protocolfilters_module;
  // Writes header to the specified stream
  function pfc_writeHeader(pStream : PPFStream_c; ph : PPFHeader_c) : Longbool; cdecl; external protocolfilters_module;
  // Decompress the data stream inplace
  function pfc_unzipStream(pStream : PPFStream_c) : Longbool; cdecl; external protocolfilters_module;

  // Return after root certificate import thread is completed
  procedure pfc_waitForImportCompletion(); cdecl; external protocolfilters_module;

  // Start writing debug log (only for debug builds and release builds with log support)
  function pfc_startLog(logFileName : PAnsiChar) : Longbool; cdecl; external protocolfilters_module;

  // Stop writing debug log (only for debug builds and release builds with log support)
  procedure pfc_stopLog(); cdecl; external protocolfilters_module;

  // Set timeout in seconds for SSL exceptions of the specified class
  procedure pfc_setExceptionsTimeout(ec : integer; timeout : Int64); cdecl; external protocolfilters_module;

  // Delete SSL exceptions of the specified class
  procedure pfc_deleteExceptions(ec : integer); cdecl; external protocolfilters_module;

implementation

//////////////////////////////////////////////////////////////////////////////
// TPFHeader

constructor TPFHeader.Create();
begin
inherited Create;
m_pHeader := PFHeader_create();
end;

constructor TPFHeader.Create(pHeader : PPFHeader_c);
begin
inherited Create;
m_pHeader := pHeader;
end;

constructor TPFHeader.Create(buf : PAnsiChar; len : integer);
begin
inherited Create;
m_pHeader := PFHeader_parse(buf, len);
end;

destructor TPFHeader.Destroy();
begin
if (assigned(m_pHeader)) then
  begin
  PFHeader_free(m_pHeader);
  m_pHeader := nil;
  end;
inherited Destroy;
end;

function TPFHeader.readFromStream(pStream : TPFStream) : Longbool;
begin
Result := pfc_readHeader(pStream.baseObject, m_pHeader);
end;

function TPFHeader.writeToStream(pStream : TPFStream) : Longbool;
begin
Result := pfc_writeHeader(pStream.baseObject, m_pHeader);
end;

procedure TPFHeader.addField(const name : AnsiString; const value : AnsiString; toStart : Longbool);
begin
PFHeader_addField(m_pHeader, PAnsiChar(name), PAnsiChar(value), toStart);
end;

procedure TPFHeader.removeFields(const name : AnsiString; exact : Longbool);
begin
PFHeader_removeFields(m_pHeader, PAnsiChar(name), exact);
end;

procedure TPFHeader.clear();
begin
PFHeader_clear(m_pHeader);
end;

function TPFHeader.getFieldValue(const name : AnsiString) : AnsiString;
var pField : PPFHeaderField_c;
begin
pField := PFHeader_findFirstField(m_pHeader, PAnsiChar(name));
if (pField = nil) then
  begin
  Result := '';
  exit;
  end;
Result := PFHeaderField_getValue(pField);
end;

function TPFHeader.getSize() : integer;
begin
Result := PFHeader_size(m_pHeader);
end;

function TPFHeader.getFieldName(index : integer) : AnsiString;
var pField : PPFHeaderField_c;
begin
pField := PFHeader_getField(m_pHeader, index);
if (pField = nil) then
  begin
  Result := '';
  exit;
  end;
Result := PFHeaderField_getName(pField);
end;

function TPFHeader.getFieldValue(index : integer) : AnsiString;
var pField : PPFHeaderField_c;
begin
pField := PFHeader_getField(m_pHeader, index);
if (pField = nil) then
  begin
  Result := '';
  exit;
  end;
Result := PFHeaderField_getValue(pField);
end;

//////////////////////////////////////////////////////////////////////////////
// TPFStream

constructor TPFStream.Create(pStream : PPFStream_c);
begin
inherited Create;
m_pStream := pStream;
end;

destructor TPFStream.Destroy();
begin
inherited Destroy;
end;

function TPFStream.seek(offset : tStreamPos; origin : tStreamOrigin) : tStreamPos;
begin
Result := PFStream_seek(m_pStream, offset, origin);
end;

function TPFStream.read(buffer : PAnsiChar; size : tStreamSize) : tStreamSize;
begin
Result := PFStream_read(m_pStream, buffer, size);
end;

function TPFStream.write(buffer : PAnsiChar; size : tStreamSize) : tStreamSize;
begin
Result := PFStream_write(m_pStream, buffer, size);
end;

function TPFStream.size() : tStreamPos;
begin
Result := PFStream_size(m_pStream);
end;

function TPFStream.setEndOfStream() : tStreamPos;
begin
Result := PFStream_setEndOfStream(m_pStream);
end;

procedure TPFStream.reset();
begin
PFStream_reset(m_pStream);
end;

//////////////////////////////////////////////////////////////////////////////
// TPFObject

constructor TPFObject.Create(pObject : PPFObject_c);
begin
inherited Create;
m_pObject := pObject;
createStreamWrappers();
end;

constructor TPFObject.Create(objectType : PF_ObjectType; nStreams : integer);
begin
inherited Create;
m_pObject := PFObject_create(objectType, nStreams);
createStreamWrappers();
end;

destructor TPFObject.Destroy();
begin
if (assigned(m_pObject)) then
  begin
  PFObject_free(m_pObject);
  m_pObject := nil;
  end;
freeStreamWrappers();
inherited Destroy;
end;

function TPFObject.getType() : PF_ObjectType;
begin
Result := PFObject_getType(m_pObject);
end;

procedure TPFObject.setType(value : PF_ObjectType);
begin
PFObject_setType(m_pObject, value);
end;

function TPFObject.isReadOnly() : Longbool;
begin
Result := PFObject_isReadOnly(m_pObject);
end;

procedure TPFObject.setReadOnly(value : Longbool);
begin
PFObject_setReadOnly(m_pObject, value);
end;

function TPFObject.getStreamCount() : integer;
begin
Result := PFObject_getStreamCount(m_pObject);
end;

function TPFObject.getStream(index : integer) : TPFStream;
begin
if ((index < 0) or (index >= Length(m_streams))) then
  begin
  Result := m_streams[Length(m_streams)-1];
  exit;
  end;
Result := m_streams[index];
end;

procedure TPFObject.clear();
begin
PFObject_clear(m_pObject);
end;

function TPFObject.clone() : TPFObject;
begin
Result := TPFObject.Create(PFObject_clone(m_pObject));
end;

function TPFObject.detach() : TPFObject;
begin
Result := TPFObject.Create(PFObject_detach(m_pObject));
end;

function TPFObject.post(id : ENDPOINT_ID) : PF_BOOL;
begin
Result := pfc_postObject(id, m_pObject);
end;

procedure TPFObject.createStreamWrappers;
var i, n : integer;
begin
n := getStreamCount();
SetLength(m_streams, n+1);
for i:=0 to n do
  begin
  m_streams[i] := TPFStream.Create(PFObject_getStream(m_pObject, i));
  end;
end;

procedure TPFObject.freeStreamWrappers;
var i, n : integer;
begin
n := Length(m_streams);
for i:=0 to n-1 do
  begin
  m_streams[i].Free;
  end;
end;



end.
