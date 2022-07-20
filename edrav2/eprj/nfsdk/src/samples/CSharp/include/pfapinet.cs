using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Text;
using System.Runtime.InteropServices;

namespace pfapinet
{
	public enum PF_FilterFlags
	{
		FF_DEFAULT = 0,
		
		// Generic filtering flags
		FF_DONT_FILTER_IN = 1,	// Passthrough incoming objects without filtering
		FF_DONT_FILTER_OUT = 2,	// Passthrough outgoing objects without filtering
		FF_READ_ONLY_IN = 4,	// Filter incoming objects in read-only mode
		FF_READ_ONLY_OUT = 8,	// Filter outgoing objects in read-only mode

		// SSL flags

		// Decode TLS sessions
		FF_SSL_TLS = 16,		
		
		// Generate self-signed certificates instead of using root CA as parent
		FF_SSL_SELF_SIGNED_CERTIFICATE = 32, 
		
		// Indicate OT_SSL_HANDSHAKE_OUTGOING/OT_SSL_HANDSHAKE_INCOMING via dataPartAvailable
		FF_SSL_INDICATE_HANDSHAKE_REQUESTS = 64, 
		
		// Try to detect TLS handshake automatically in first 8 kilobytes of packets
		FF_SSL_TLS_AUTO = 128,

        // Use RC4 for SSL sessions with local and remote endpoints
        FF_SSL_COMPATIBILITY = 256,

        // Verify server certificates and don't filter SSL if the certificate is not valid
        FF_SSL_VERIFY = 512,

        // Filter SSL connections in case when server requests a client certificate.
        // This method requires appropriate client certificates to be in 
        // Windows certificate storage with exportable private key.
        FF_SSL_SUPPORT_CLIENT_CERTIFICATES = 1024,

        // Indicate OT_SSL_SERVER_CERTIFICATE via dataPartAvailable
        FF_SSL_INDICATE_SERVER_CERTIFICATES = 2048,

        // Indicate OT_SSL_EXCEPTION via dataPartAvailable
        FF_SSL_INDICATE_EXCEPTIONS = 4096,

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

        // By default the filter sends pipelined requests by one, after receiving 
		// a response from server for a previous request. This flag instructs the filter 
		// to send all pipelined requests as-is.
		FF_HTTP_KEEP_PIPELINING = 16,	

		// Indicate via dataAvailable the objects OT_HTTP_SKIPPED_REQUEST_COMPLETE and
		// OT_HTTP_SKIPPED_RESPONSE_COMPLETE 
		FF_HTTP_INDICATE_SKIPPED_OBJECTS = 32,	

		// Block SPDY protocol
		FF_HTTP_BLOCK_SPDY = 64,	
	};

    public enum PF_Step 
    {
	    FT_STEP	 = 100
    };

	public enum PF_FilterType
	{
		FT_NONE,			// Empty 
		FT_SSL		= PF_Step.FT_STEP,		// SSL decoder
        FT_HTTP = 2 * PF_Step.FT_STEP,	// HTTP
        FT_POP3 = 3 * PF_Step.FT_STEP,	// POP3
        FT_SMTP = 4 * PF_Step.FT_STEP,	// SMTP
        FT_PROXY = 5 * PF_Step.FT_STEP,	// HTTPS and SOCK4/5 proxy
        FT_RAW = 6 * PF_Step.FT_STEP,  // Raw packets
        FT_FTP = 7 * PF_Step.FT_STEP,	// FTP
        FT_FTP_DATA = 8 * PF_Step.FT_STEP,	// FTP data
        FT_NNTP = 9 * PF_Step.FT_STEP,	// NNTP
        FT_ICQ = 10 * PF_Step.FT_STEP,	// ICQ
        FT_XMPP = 11 * PF_Step.FT_STEP,	// XMPP
	};

	/**
	 *	The types of objects for filtering	
	 */
	public enum PF_ObjectType
	{
		OT_NULL,
		/** Disconnect request from local peer */
		OT_TCP_DISCONNECT_LOCAL = 1,
		/** Disconnect request from remote peer */
		OT_TCP_DISCONNECT_REMOTE = 2,
		/** HTTP request object */
		OT_HTTP_REQUEST = PF_FilterType.FT_HTTP,
		/** HTTP response object */
		OT_HTTP_RESPONSE = PF_FilterType.FT_HTTP + 1,
		/** HTTP skipped request complete */
		OT_HTTP_SKIPPED_REQUEST_COMPLETE = PF_FilterType.FT_HTTP + 2,
		/** HTTP skipped response complete */
		OT_HTTP_SKIPPED_RESPONSE_COMPLETE = PF_FilterType.FT_HTTP + 3,
		/** POP3 messages */
		OT_POP3_MAIL_INCOMING = PF_FilterType.FT_POP3,
		/** SMTP messages */
		OT_SMTP_MAIL_OUTGOING = PF_FilterType.FT_SMTP,
		/** Request to HTTPS proxy */
		OT_HTTPS_PROXY_REQUEST = PF_FilterType.FT_PROXY,
		/** Request to SOCKS4 proxy */
		OT_SOCKS4_REQUEST = PF_FilterType.FT_PROXY + 1,
		/** Query authentication method from SOCKS5 proxy */
		OT_SOCKS5_AUTH_REQUEST = PF_FilterType.FT_PROXY + 2,
		/** Send user name and password to SOCKS5 proxy */
		OT_SOCKS5_AUTH_UNPW_NEGOTIATE = PF_FilterType.FT_PROXY + 3,
		/** Request to SOCKS5 proxy */
		OT_SOCKS5_REQUEST = PF_FilterType.FT_PROXY + 4,
		/** Outgoing raw buffer */
		OT_RAW_OUTGOING = PF_FilterType.FT_RAW,
		/** Incoming raw buffer */
		OT_RAW_INCOMING = PF_FilterType.FT_RAW + 1,
		/** FTP command */
		OT_FTP_COMMAND = PF_FilterType.FT_FTP,
		/** FTP response */
		OT_FTP_RESPONSE = PF_FilterType.FT_FTP + 1,
		/** FTP outgoing data */
		OT_FTP_DATA_OUTGOING = PF_FilterType.FT_FTP_DATA,
		/** FTP incoming data */
		OT_FTP_DATA_INCOMING = PF_FilterType.FT_FTP_DATA + 1,
		/** FTP outgoing data part */
		OT_FTP_DATA_PART_OUTGOING = PF_FilterType.FT_FTP_DATA + 2,
		/** FTP incoming data part */
		OT_FTP_DATA_PART_INCOMING = PF_FilterType.FT_FTP_DATA + 3,
		/** News group article */
		OT_NNTP_ARTICLE = PF_FilterType.FT_NNTP,
		/** News group post */
		OT_NNTP_POST = PF_FilterType.FT_NNTP + 1,
		/** ICQ login */
		OT_ICQ_LOGIN = PF_FilterType.FT_ICQ,
		/** Outgoing ICQ data */
		OT_ICQ_REQUEST = PF_FilterType.FT_ICQ + 1,
		/** Incoming ICQ data */
		OT_ICQ_RESPONSE = PF_FilterType.FT_ICQ + 2,
		/** Outgoing ICQ chat message */
		OT_ICQ_CHAT_MESSAGE_OUTGOING = PF_FilterType.FT_ICQ + 3,
		/** Incoming ICQ chat message */
		OT_ICQ_CHAT_MESSAGE_INCOMING = PF_FilterType.FT_ICQ + 4,
		/** Outgoing SSL handshake request */
		OT_SSL_HANDSHAKE_OUTGOING = PF_FilterType.FT_SSL + 1,
		/** Incoming SSL handshake request */
		OT_SSL_HANDSHAKE_INCOMING = PF_FilterType.FT_SSL + 2,
        /** Invalid server SSL certificate */
        OT_SSL_INVALID_SERVER_CERTIFICATE = PF_FilterType.FT_SSL + 3,
        /** Server SSL certificate */
        OT_SSL_SERVER_CERTIFICATE = PF_FilterType.FT_SSL + 4,
        /** SSL exception */
        OT_SSL_EXCEPTION = PF_FilterType.FT_SSL + 5,
		/** SSL client certificate request */
		OT_SSL_CLIENT_CERT_REQUEST = PF_FilterType.FT_SSL + 6,
        /** XMPP request */
		OT_XMPP_REQUEST = PF_FilterType.FT_XMPP,
		/** XMPP response */
		OT_XMPP_RESPONSE = PF_FilterType.FT_XMPP + 1,
	};

	public enum PF_DATA_PART_CHECK_RESULT
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
	};

	public enum PF_OpTarget
	{
		OT_FIRST,
		OT_PREV,
		OT_NEXT,
		OT_LAST
	};

	// Filter category
	public enum PF_FilterCategory
	{
		FC_PROTOCOL_FILTER,
		FC_PREPROCESSOR
	};

	// HTTP definitions

	/** Stream indexes in HTTP object */
	public enum PF_HttpStream
	{
		/** First string of HTTP request or response */
		HS_STATUS = 0,
		/** HTTP header */
		HS_HEADER = 1,
		/** HTTP content */
		HS_CONTENT = 2
	};

	/** Custom HTTP headers for responses */
	public class CustomHTTPHeaders
	{
		// First string from appropriate HTTP request
		public const String HTTP_EXHDR_RESPONSE_REQUEST = "X-EXHDR-REQUEST";
		// Host field from appropriate HTTP request header
		public const String HTTP_EXHDR_RESPONSE_HOST = "X-EXHDR-REQUEST-HOST";
	};

	// ICQ definitions

	public enum PF_ICQStream
	{
		ICQS_RAW = 0,
		ICQS_USER_UIN = 1,
		ICQS_CONTACT_UIN = 2,
		ICQS_TEXT_FORMAT = 3,
		ICQS_TEXT = 4,
		ICQS_MAX = 5
	};

	public enum PF_ICQTextFormat
	{
		ICQTF_ANSI = 0,
		ICQTF_UTF8 = 1,
		ICQTF_UNICODE = 2,
		ICQTF_FILE_TRANSFER = 3
	};

	public class ICQFileTransferHeaders
	{
		public const String ICQ_FILE_COUNT	= "File-Count";
		public const String ICQ_TOTAL_BYTES = "Total-Bytes";
		public const String ICQ_FILE_NAME = "File-Name";
	};

    public enum PF_RootSSLImportFlag
	{
		RSIF_DONT_IMPORT = 0,
		RSIF_IMPORT_TO_MOZILLA_AND_OPERA = 1,
		RSIF_IMPORT_TO_PIDGIN = 2,
		RSIF_IMPORT_EVERYWHERE = 3,
		RSIF_GENERATE_ROOT_PRIVATE_KEY = 4,
	};

	/** Stream indices in OT_SSL_SERVER_CERTIFICATE object */
	public enum PF_SSL_ServerCertStream
	{
		/** Server certificate bytes */
		SSL_SCS_CERTIFICATE = 0,
		/** Certificate subject name */
		SSL_SCS_SUBJECT = 1,
		/** Certificate issuer name */
		SSL_SCS_ISSUER = 2
	};

    /** Class of SSL exceptions */
    public enum eEXCEPTION_CLASS
    {
        // Generic exceptions generated because of unexpected disconnect during handshake
        EXC_GENERIC = 0,
        // TLS exceptions, switching version of TLS protocol
        EXC_TLS = 1,
        // Certificate revokation exceptions
        EXC_CERT_REVOKED = 2,
        EXC_MAX
    };

    public class PFHeader : List<KeyValuePair<string,string> >
    {
        public void Add(string key, string value)
        {
            this[key] = value;
        }
        public void Remove(string key)
        {
            string lcKey = key.ToLowerInvariant();
            for (int i = 0; i < this.Count; i++)
            {
                if (this[i].Key.ToLowerInvariant() == lcKey)
                {
                    this.RemoveAt(i);
                    break;
                }
            }
        }

        public string this[string key]
        {
            get
            {
                string lcKey = key.ToLowerInvariant();
                foreach (KeyValuePair<string, string> v in this)
                {
                    if (v.Key.ToLowerInvariant() == lcKey)
                        return v.Value;
                }
                return null;
            }
            set
            {
                this.Add(new KeyValuePair<string, string>(key, value));
            }
        }
    };

	/////////////////////////////////////////////////////////////////////////////
	// Managed wrappers over ProtocolFilters classes

	public class PFStream
	{
        private IntPtr m_nativeObject;
        
        public PFStream(IntPtr nativeObject)
        {
            m_nativeObject = nativeObject;
        }
        
        public IntPtr getNativeObject()
        {
            return m_nativeObject;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern UInt64 PFStream_seek(IntPtr nativeObject, UInt64 offset, int origin);

        public UInt64 seek(UInt64 offset, int origin)
        {
            return PFStream_seek(m_nativeObject, offset, origin);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern UInt32 PFStream_read(IntPtr nativeObject, IntPtr buffer, UInt32 size);

        public UInt32 read(IntPtr buffer, UInt32 size)
        {
            return PFStream_read(m_nativeObject, buffer, size);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern UInt32 PFStream_write(IntPtr nativeObject, IntPtr buffer, UInt32 size);

        public UInt32 write(IntPtr buffer, UInt32 size)
        {
            return PFStream_write(m_nativeObject, buffer, size);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern UInt64 PFStream_size(IntPtr nativeObject);

        public UInt64 size()
        {
            return PFStream_size(m_nativeObject);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern UInt64 PFStream_copyTo(IntPtr nativeObject, IntPtr dstStream);

        public UInt64 copyTo(ref PFStream dstStream)
        {
            return PFStream_copyTo(m_nativeObject, dstStream.getNativeObject());
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern UInt64 PFStream_setEndOfStream(IntPtr nativeObject);

        public UInt64 setEndOfStream()
        {
            return PFStream_setEndOfStream(m_nativeObject);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void PFStream_reset(IntPtr nativeObject);

        public void reset()
        {
            PFStream_reset(m_nativeObject);
        }
    };

    public class PFObject : IDisposable
    {
        private IntPtr  m_nativeObject;
        private bool    m_disposed = false;
        private bool    m_mustDelete;

        public PFObject(IntPtr nativeObject, bool mustDelete)
        {
            m_nativeObject = nativeObject;
            m_mustDelete = mustDelete;
        }
        
        ~PFObject()
        {
            Dispose(false);
        }
        
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this.m_disposed)
            {
                if (m_mustDelete)
                    free();
                m_disposed = true;
            }
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr PFObject_create(PF_ObjectType type, int nStreams);

        /**
		 *	Creates a new object with specified type and number of streams
		 */
        public static PFObject create(PF_ObjectType type, int nStreams)
        {
            return new PFObject(PFObject_create(type, nStreams), true);
        }

        public IntPtr getNativeObject()
        {
            return m_nativeObject;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int PFObject_getType(IntPtr nativeObject);

        public PF_ObjectType getType()
        {
            return (PF_ObjectType)PFObject_getType(m_nativeObject);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void PFObject_setType(IntPtr nativeObject, PF_ObjectType type);

        public void setType(PF_ObjectType type)
        {
            PFObject_setType(m_nativeObject, type);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int PFObject_isReadOnly(IntPtr nativeObject);

        public bool isReadOnly()
        {
            return PFObject_isReadOnly(m_nativeObject) != 0;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int PFObject_setReadOnly(IntPtr nativeObject, int value);

        public void setReadOnly(bool value)
        {
            PFObject_setReadOnly(m_nativeObject, value? 1 : 0);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int PFObject_getStreamCount(IntPtr nativeObject);

        public int getStreamCount()
        {
            return PFObject_getStreamCount(m_nativeObject);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr PFObject_getStream(IntPtr nativeObject, int index);

        public PFStream getStream(int index)
        {
            IntPtr pStream = PFObject_getStream(m_nativeObject, index);
            return (pStream != IntPtr.Zero) ? new PFStream(pStream) : null;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void PFObject_clear(IntPtr nativeObject);

        public void clear()
        {
            PFObject_clear(m_nativeObject);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr PFObject_clone(IntPtr nativeObject);

        public PFObject clone()
        {
            IntPtr pObject = PFObject_clone(m_nativeObject);
            return (pObject != IntPtr.Zero) ? new PFObject(pObject, true) : null;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr PFObject_detach(IntPtr nativeObject);

        public PFObject detach()
        {
            IntPtr pObject = PFObject_detach(m_nativeObject);
            return (pObject != IntPtr.Zero) ? new PFObject(pObject, true) : null;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void PFObject_free(IntPtr nativeObject);

        public void free()
        {
            if (m_nativeObject != IntPtr.Zero)
            {
                PFObject_free(m_nativeObject);
                m_nativeObject = (IntPtr)null;
            }
        }

    };

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_threadStart();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_threadEnd();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_tcpConnectRequest(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_tcpConnected(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_tcpClosed(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_udpCreated(ulong id, ref nfapinet.NF_UDP_CONN_INFO pConnInfo);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_udpConnectRequest(ulong id, ref nfapinet.NF_UDP_CONN_REQUEST pConnReq);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_udpClosed(ulong id, ref nfapinet.NF_UDP_CONN_INFO pConnInfo);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void cbd_dataAvailable(ulong id, IntPtr pObject);	
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate PF_DATA_PART_CHECK_RESULT
            cbd_dataPartAvailable(ulong id, IntPtr pObject);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate nfapinet.NF_STATUS cbd_tcpPostSend(ulong id, IntPtr buf, int len);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate nfapinet.NF_STATUS cbd_tcpPostReceive(ulong id, IntPtr buf, int len);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate nfapinet.NF_STATUS cbd_tcpSetConnectionState(ulong id, int suspended);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate nfapinet.NF_STATUS cbd_udpPostSend(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate nfapinet.NF_STATUS cbd_udpPostReceive(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options);
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate nfapinet.NF_STATUS cbd_udpSetConnectionState(ulong id, int suspended);

    public interface PFEvents
    {
        void threadStart();
        void threadEnd();
        void tcpConnectRequest(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo);
        void tcpConnected(ulong id, nfapinet.NF_TCP_CONN_INFO pConnInfo);
        void tcpClosed(ulong id, nfapinet.NF_TCP_CONN_INFO pConnInfo);
        void udpCreated(ulong id, nfapinet.NF_UDP_CONN_INFO pConnInfo);
        void udpConnectRequest(ulong id, ref nfapinet.NF_UDP_CONN_REQUEST pConnReq);
        void udpClosed(ulong id, nfapinet.NF_UDP_CONN_INFO pConnInfo);
		// New object is ready for filtering
		void dataAvailable(ulong id, ref PFObject pObject);	
		// A part of content is available for examining.
		PF_DATA_PART_CHECK_RESULT 
			dataPartAvailable(ulong id, ref PFObject pObject);
        nfapinet.NF_STATUS tcpPostSend(ulong id, IntPtr buf, int len);
        nfapinet.NF_STATUS tcpPostReceive(ulong id, IntPtr buf, int len);
        nfapinet.NF_STATUS tcpSetConnectionState(ulong id, int suspended);
        nfapinet.NF_STATUS udpPostSend(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen);
        nfapinet.NF_STATUS udpPostReceive(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen);
        nfapinet.NF_STATUS udpSetConnectionState(ulong id, int suspended);
    };

    public class PFEventsFwd
    {
        public static PFEvents m_pEventHandler = null;

        public static void threadStart()
        {
            m_pEventHandler.threadStart();
        }
        public static void threadEnd()
        {
            m_pEventHandler.threadEnd();
        }
        public static void tcpConnectRequest(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo)
        {
            m_pEventHandler.tcpConnectRequest(id, ref pConnInfo);
        }
        public static void tcpConnected(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo)
        {
            m_pEventHandler.tcpConnected(id, pConnInfo);
        }
        public static void tcpClosed(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo)
        {
            m_pEventHandler.tcpClosed(id, pConnInfo);
        }
        public static void udpCreated(ulong id, ref nfapinet.NF_UDP_CONN_INFO pConnInfo)
        {
            m_pEventHandler.udpCreated(id, pConnInfo);
        }
        public static void udpConnectRequest(ulong id, ref nfapinet.NF_UDP_CONN_REQUEST pConnReq)
        {
            m_pEventHandler.udpConnectRequest(id, ref pConnReq);
        }
        public static void udpClosed(ulong id, ref nfapinet.NF_UDP_CONN_INFO pConnInfo)
        {
            m_pEventHandler.udpClosed(id, pConnInfo);
        }
        public static void dataAvailable(ulong id, IntPtr pObject)
        {
            PFObject obj = new PFObject(pObject, false);
            m_pEventHandler.dataAvailable(id, ref obj);
        }
        public static PF_DATA_PART_CHECK_RESULT
            dataPartAvailable(ulong id, IntPtr pObject)
        {
            PFObject obj = new PFObject(pObject, false);
            return m_pEventHandler.dataPartAvailable(id, ref obj);
        }
        public static nfapinet.NF_STATUS tcpPostSend(ulong id, IntPtr buf, int len)
        {
            return m_pEventHandler.tcpPostSend(id, buf, len);
        }
        public static nfapinet.NF_STATUS tcpPostReceive(ulong id, IntPtr buf, int len)
        {
            return m_pEventHandler.tcpPostReceive(id, buf, len);
        }
        public static nfapinet.NF_STATUS tcpSetConnectionState(ulong id, int suspended)
        {
            return m_pEventHandler.tcpSetConnectionState(id, suspended);
        }
        public static nfapinet.NF_STATUS udpPostSend(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options)
        {
            if (options.ToInt64() != 0)
            {
                nfapinet.NF_UDP_OPTIONS optionsCopy =
                    (nfapinet.NF_UDP_OPTIONS)
                    Marshal.PtrToStructure((IntPtr)options, typeof(nfapinet.NF_UDP_OPTIONS));
                int optionsLen = 8 + optionsCopy.optionsLength;
                return m_pEventHandler.udpPostSend(id, remoteAddress, buf, len, options, optionsLen);
            }
            else
            {
                return m_pEventHandler.udpPostSend(id, remoteAddress, buf, len, (IntPtr)null, 0);
            }
        }
        public static nfapinet.NF_STATUS udpPostReceive(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options)
        {
            if (options.ToInt64() != 0)
            {
                nfapinet.NF_UDP_OPTIONS optionsCopy =
                    (nfapinet.NF_UDP_OPTIONS)
                    Marshal.PtrToStructure((IntPtr)options, typeof(nfapinet.NF_UDP_OPTIONS));
                int optionsLen = 8 + optionsCopy.optionsLength;
                return m_pEventHandler.udpPostReceive(id, remoteAddress, buf, len, options, optionsLen);
            }
            else
            {
                return m_pEventHandler.udpPostReceive(id, remoteAddress, buf, len, (IntPtr)null, 0);
            }
        }
        public static nfapinet.NF_STATUS udpSetConnectionState(ulong id, int suspended)
        {
            return m_pEventHandler.udpSetConnectionState(id, suspended);
        }
    };

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct PFEventsInternal
    {
        public cbd_threadStart threadStart;
        public cbd_threadEnd threadEnd;
        public cbd_tcpConnectRequest tcpConnectRequest;
        public cbd_tcpConnected tcpConnected;
        public cbd_tcpClosed tcpClosed;
        public cbd_udpCreated udpCreated;
        public cbd_udpConnectRequest udpConnectRequest;
        public cbd_udpClosed udpClosed;
        public cbd_dataAvailable dataAvailable;
        public cbd_dataPartAvailable dataPartAvailable;
        public cbd_tcpPostSend tcpPostSend;
        public cbd_tcpPostReceive tcpPostReceive;
        public cbd_tcpSetConnectionState tcpSetConnectionState;
        public cbd_udpPostSend udpPostSend;
        public cbd_udpPostReceive udpPostReceive;
        public cbd_udpSetConnectionState udpSetConnectionState;
    };

    // Managed wrapper over API 
    public class PFAPI
    {
        private static IntPtr m_pEventHandlerRaw;
        private static PFEventsInternal m_pEventHandler;

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl, CharSet=CharSet.Unicode)]
		private static extern int pfc_init(IntPtr pHandler, String dataFolder);

        /**
         * Initialize the library
         * @param pHandler Event handler
         * @param dataFolder A path to configuration folder, where the library
                  stores SSL certificates and temporary files.
         */
        public static bool pf_init(PFEvents pHandler, String dataFolder)
        {
            PFEventsFwd.m_pEventHandler = pHandler;

            m_pEventHandler = new PFEventsInternal();

            m_pEventHandler.threadStart = new cbd_threadStart(PFEventsFwd.threadStart);
            m_pEventHandler.threadEnd = new cbd_threadEnd(PFEventsFwd.threadEnd);
            m_pEventHandler.tcpConnectRequest = new cbd_tcpConnectRequest(PFEventsFwd.tcpConnectRequest);
            m_pEventHandler.tcpConnected = new cbd_tcpConnected(PFEventsFwd.tcpConnected);
            m_pEventHandler.tcpClosed = new cbd_tcpClosed(PFEventsFwd.tcpClosed);
            m_pEventHandler.udpCreated = new cbd_udpCreated(PFEventsFwd.udpCreated);
            m_pEventHandler.udpConnectRequest = new cbd_udpConnectRequest(PFEventsFwd.udpConnectRequest);
            m_pEventHandler.udpClosed = new cbd_udpClosed(PFEventsFwd.udpClosed);
            m_pEventHandler.dataAvailable = new cbd_dataAvailable(PFEventsFwd.dataAvailable);
            m_pEventHandler.dataPartAvailable = new cbd_dataPartAvailable(PFEventsFwd.dataPartAvailable);
            m_pEventHandler.tcpPostSend = new cbd_tcpPostSend(PFEventsFwd.tcpPostSend);
            m_pEventHandler.tcpPostReceive = new cbd_tcpPostReceive(PFEventsFwd.tcpPostReceive);
            m_pEventHandler.tcpSetConnectionState = new cbd_tcpSetConnectionState(PFEventsFwd.tcpSetConnectionState);
            m_pEventHandler.udpPostSend = new cbd_udpPostSend(PFEventsFwd.udpPostSend);
            m_pEventHandler.udpPostReceive = new cbd_udpPostReceive(PFEventsFwd.udpPostReceive);
            m_pEventHandler.udpSetConnectionState = new cbd_udpSetConnectionState(PFEventsFwd.udpSetConnectionState);

            m_pEventHandlerRaw = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(PFEventsInternal)));
            Marshal.StructureToPtr(m_pEventHandler, m_pEventHandlerRaw, true);

            return pfc_init(m_pEventHandlerRaw, dataFolder) != 0;
        }

        /** 
         * Free the library 
         */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void pfc_free();

        public static void pf_free()
        {
            pfc_free();
        }

		/** 
		 * Returns a pointer to event handler class for passing to nf_init() 
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr pfc_getNFEventHandler();

        public static IntPtr pf_getNFEventHandler()
        {
            return pfc_getNFEventHandler();
        }

		/**
		 *	Post an object to specified endpoint
		 *	@param id Endpoint id
		 *	@param pObject Object with some content
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_postObject(ulong id, IntPtr pObject);

		public static bool pf_postObject(ulong id, ref PFObject pObject)
        {
            return pfc_postObject(id, pObject.getNativeObject()) != 0;
        }

        /**
		 *	Adds a new filter to session filtering chain.
		 *	@param id Endpoint id
		 *	@param type Type of the filter to add
		 *	@param flags Filter specific flags
		 *	@param target Position where to add new filter (OT_NEXT, OT_NEXT - relative to typeBase)
		 *	@param typeBase Type of origin filter
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_addFilter(ulong id,
							PF_FilterType type, 
							PF_FilterFlags flags,
							PF_OpTarget target, 
							PF_FilterType typeBase);

		public static bool pf_addFilter(ulong id,
							PF_FilterType type, 
							PF_FilterFlags flags,
							PF_OpTarget target, 
							PF_FilterType typeBase)
        {
            return pfc_addFilter(id, type, flags, target, typeBase) != 0;
        }

				
		/**
		 *	Removes the specified filter from chain.
		 *	@param id Endpoint id
		 *	@param type Type of the filter to remove
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_deleteFilter(ulong id, PF_FilterType type);

		public static bool pf_deleteFilter(ulong id, PF_FilterType type)
        {
            return pfc_deleteFilter(id, type) != 0;
        }

		/**
		 *	Returns the number of active filters for the specified connection.
		 *	@param id Endpoint id
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_getFilterCount(ulong id);

		public static int pf_getFilterCount(ulong id)
        {
            return pfc_getFilterCount(id);
        }

		/**
		 *	Returns true if there is a filter of the specified type in filtering chain
		 *	@param id Endpoint id
		 *	@param type Filter type
		 */
		[DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_isFilterActive(ulong id, PF_FilterType type);

		public static bool pf_isFilterActive(ulong id, PF_FilterType type)
        {
            return pfc_isFilterActive(id, type) != 0;
        }

		/**
		 *	Returns true when it is safe to disable filtering for the connection 
		 *  with specified id (there are no filters in chain and internal buffers are empty).
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_canDisableFiltering(ulong id);
		
        public static bool pf_canDisableFiltering(ulong id)
        {
            return pfc_canDisableFiltering(id) != 0;
        }

		/**
		 *	Specifies a subject of root certificate, used for generating other SSL certificates.
		 *	This name appears in "Issued by" field of certificates assigned to filtered SSL
		 *	connections. Default value - "NetFilterSDK".
		 *	@param id Endpoint id
		 *	@param type Type of the filter to remove
		 */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void pfc_setRootSSLCertSubject(String rootSubject);
		
        public static void pf_setRootSSLCertSubject(String rootSubject)
        {
            pfc_setRootSSLCertSubject(rootSubject);
        }

	    [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void pfc_setRootSSLCertSubjectEx(String rootSubject, IntPtr x509, int x509Len, IntPtr pkey, int pkeyLen);

        public static void pf_setRootSSLCertSubjectEx(String rootSubject, IntPtr x509, int x509Len, IntPtr pkey, int pkeyLen)
        {
            pfc_setRootSSLCertSubjectEx(rootSubject, x509, x509Len, pkey, pkeyLen);
        }

        /**
         *	Specifies import flags from PF_RootSSLImportFlag enumeration, allowing to control
         *	Importing root SSL certificate in pf_setRootSSLCertSubject to supported storages.
         *	Default value - RSIF_IMPORT_EVERYWHERE.
         */

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void pfc_setRootSSLCertImportFlags(ulong flags);

        public static void pf_setRootSSLCertImportFlags(ulong flags)
        {
            pfc_setRootSSLCertImportFlags(flags);
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int pfc_getRootSSLCertFileName(IntPtr buf, int len);

        public unsafe static string getRootSSLCertFileName()
        {
            byte[] buf = new byte[512];
            fixed (byte* p = buf)
            {
                if (pfc_getRootSSLCertFileName((IntPtr)p, buf.Length) == 0)
                    return "";
                return Marshal.PtrToStringUni((IntPtr)p);
            }
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr PFHeaderField_getName(IntPtr hf);
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr PFHeaderField_getValue(IntPtr hf);

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr PFHeader_create();
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void PFHeader_free(IntPtr ph);
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void PFHeader_addField(IntPtr ph, IntPtr name, IntPtr value, int toStart);
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void PFHeader_removeFields(IntPtr ph, IntPtr name, int exact);
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void PFHeader_clear(IntPtr ph);
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int PFHeader_size(IntPtr ph);
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern IntPtr PFHeader_getField(IntPtr ph, int index);

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_readHeader(IntPtr pStream, IntPtr pHeader);

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern int pfc_writeHeader(IntPtr pStream, IntPtr pHeader);

		/**
		 *	Load header from stream
		 */
        public static PFHeader pf_readHeader(PFStream pStream)
        {
            PFHeader pColl = new PFHeader();	
	        IntPtr pHeader = PFHeader_create();

            if (pHeader == IntPtr.Zero)
                return pColl;
            
            try {
	            if (pfc_readHeader(pStream.getNativeObject(), pHeader) == 0)
		            return pColl;

	            for (int i=0; i<PFHeader_size(pHeader); i++)
	            {
                    IntPtr pField = PFHeader_getField(pHeader, i);
                    if (pField == IntPtr.Zero)
                        continue;

		            pColl.Add(new KeyValuePair<string,string>(
                        Marshal.PtrToStringAnsi(PFHeaderField_getName(pField)), 
                        Marshal.PtrToStringAnsi(PFHeaderField_getValue(pField))));
	            }
            } finally 
            {
                PFHeader_free(pHeader);
            }

	        return pColl;

        }

		/**
		 *	Save header from stream
		 */
        public static bool pf_writeHeader(PFStream pStream, PFHeader pValues)
        {
	        IntPtr pHeader = PFHeader_create();

            if (pHeader == IntPtr.Zero)
                return false;
            
            try {
                foreach (KeyValuePair<string, string> v in pValues)
                {
                    IntPtr key = Marshal.StringToHGlobalAnsi(v.Key);
                    IntPtr value = Marshal.StringToHGlobalAnsi(v.Value);

                    if (key != null && value != null)
                    {
                        PFHeader_addField(pHeader, key, value, 0);
                    }

                    Marshal.FreeHGlobal(key);
                    Marshal.FreeHGlobal(value);
                }

                if (pfc_writeHeader(pStream.getNativeObject(), pHeader) != 0)
		            return true;

            } finally 
            {
                PFHeader_free(pHeader);
            }

            return false;
        }
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int pfc_getProcessOwnerW(UInt32 processId, IntPtr buf, int len);

        public unsafe static string pf_getProcessOwnerW(UInt32 processId)
        {
            byte[] buf = new byte[512];
            fixed (byte* p = buf)
            {
                if (pfc_getProcessOwnerW(processId, (IntPtr)p, buf.Length) == 0)
                    return "";
                return Marshal.PtrToStringUni((IntPtr)p);
            }
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int pfc_unzipStream(IntPtr pStream);

        /**
         *	Decompress stream contents inplace
         */
        public static bool pf_unzipStream(PFStream pStream)
        {
            return pfc_unzipStream(pStream.getNativeObject()) != 0;
        }

		[DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void pfc_waitForImportCompletion();

        public static void pf_waitForImportCompletion()
        {
            pfc_waitForImportCompletion();
        }

	    [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern int pfc_startLog(String logFileName);

        public static bool pf_startLog(String logFileName)
        {
            return pfc_startLog(logFileName) != 0;
        }

        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
		private static extern void pfc_stopLog();

        public static void pf_stopLog()
        {
            pfc_stopLog();
        }

	    /**
	     *	Set timeout in seconds for SSL exceptions of the specified class
	     */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void pfc_setExceptionsTimeout(eEXCEPTION_CLASS ec, ulong timeout);

        public static void pf_setExceptionsTimeout(eEXCEPTION_CLASS ec, ulong timeout)
        {
            pfc_setExceptionsTimeout(ec, timeout);
        }

        /*
         * Delete SSL exceptions of the specified class
         */
        [DllImport("ProtocolFilters", CallingConvention = CallingConvention.Cdecl)]
        private static extern void pfc_deleteExceptions(eEXCEPTION_CLASS ec);

        public static void pf_deleteExceptions(eEXCEPTION_CLASS ec)
        {
            pfc_deleteExceptions(ec);
        }
    };


    public abstract class PFEventsDefault : PFEvents
	{
		public virtual void threadStart()
		{
		}
        public virtual void threadEnd()
		{
		}
        public virtual void tcpConnectRequest(ulong id, ref nfapinet.NF_TCP_CONN_INFO pConnInfo)
		{
		}
		public abstract void tcpConnected(ulong id, nfapinet.NF_TCP_CONN_INFO pConnInfo);
        public virtual void tcpClosed(ulong id, nfapinet.NF_TCP_CONN_INFO pConnInfo)
		{
		}
        public virtual void udpCreated(ulong id, nfapinet.NF_UDP_CONN_INFO pConnInfo)
		{
		}
        public virtual void udpConnectRequest(ulong id, ref nfapinet.NF_UDP_CONN_REQUEST pConnReq)
		{
		}
        public virtual void udpClosed(ulong id, nfapinet.NF_UDP_CONN_INFO pConnInfo)
		{
		}      
		public abstract void dataAvailable(ulong id, ref PFObject pObject);
        public virtual PF_DATA_PART_CHECK_RESULT 
			dataPartAvailable(ulong id, ref PFObject pObject)
		{
			return PF_DATA_PART_CHECK_RESULT.DPCR_FILTER;
		}
        public virtual nfapinet.NF_STATUS tcpPostSend(ulong id, IntPtr buf, int len)
		{
			nfapinet.NF_STATUS status;

			status = nfapinet.NFAPI.nf_tcpPostSend(id, buf, len);

			return status;
		}
        public virtual nfapinet.NF_STATUS tcpPostReceive(ulong id, IntPtr buf, int len)
		{
			nfapinet.NF_STATUS status;

			status = nfapinet.NFAPI.nf_tcpPostReceive(id, buf, len);

			return status;
		}
        public virtual nfapinet.NF_STATUS tcpSetConnectionState(ulong id, int suspended)
		{
			return nfapinet.NFAPI.nf_tcpSetConnectionState(id, suspended);
		}
        public virtual nfapinet.NF_STATUS udpPostSend(ulong id, 
			IntPtr remoteAddress, 
			IntPtr buf, int len,
            IntPtr options, int optionsLen)
		{
			return nfapinet.NFAPI.nf_udpPostSend(id, remoteAddress, buf, len, options);
		}
        public virtual nfapinet.NF_STATUS udpPostReceive(ulong id, 
			IntPtr remoteAddress, 
			IntPtr buf, int len,
            IntPtr options, int optionsLen)
		{
			return nfapinet.NFAPI.nf_udpPostReceive(id, remoteAddress, buf, len, options);
		}
        public virtual nfapinet.NF_STATUS udpSetConnectionState(ulong id, int suspended)
		{
			return nfapinet.NFAPI.nf_udpSetConnectionState(id, suspended);
		}
	};

}
