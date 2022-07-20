using System;
using System.Collections;
using System.Text;
using pfapinet;
using nfapinet;
using System.Runtime.InteropServices;
using System.Net;
using System.Net.Sockets;
using System.Collections.Specialized;
using System.IO;

namespace PFNetFilterCS
{
    enum ContentFilterParam
    {
        CFP_FILTER_SSL,
        CFP_FILTER_RAW,
        CFP_HTML_STOP_WORD,
        CFP_URL_STOP_WORD,
        CFP_BLOCK_PAGE,
        CFP_SKIP_DOMAIN,
        CFP_BLOCK_IMAGES,
        CFP_BLOCK_FLV,
        CFP_BLOCK_ADDRESS,
        CFP_MAIL_PREFIX,
        CFP_BLOCK_ICQ_UIN,
        CFP_BLOCK_ICQ_STRING,
        CFP_BLOCK_ICQ_FILE_TRANSFERS
    };

    class Filter : PFEventsDefault
    {
        private Form1 m_form = null;

        private Hashtable m_params = new Hashtable();

        public override void tcpConnected(ulong id, NF_TCP_CONN_INFO pConnInfo) 
        {
            if (pConnInfo.direction == (byte)NF_DIRECTION.NF_D_OUT)
            {
                bool filterSSL;

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_PROXY,
                    PF_FilterFlags.FF_DEFAULT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                lock (m_params)
                {
                    filterSSL = (bool)m_params[ContentFilterParam.CFP_FILTER_SSL];
                    if (filterSSL)
                    {
                        PFAPI.pf_addFilter(id,
                            PF_FilterType.FT_SSL,
                            PF_FilterFlags.FF_DEFAULT,
                            PF_OpTarget.OT_LAST,
                            PF_FilterType.FT_NONE);
                    }
                }

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_HTTP,
                    PF_FilterFlags.FF_DEFAULT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_SMTP,
                    filterSSL ? PF_FilterFlags.FF_SSL_TLS : PF_FilterFlags.FF_DEFAULT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_POP3,
                    filterSSL ? PF_FilterFlags.FF_SSL_TLS : PF_FilterFlags.FF_DEFAULT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_NNTP,
                    filterSSL ? PF_FilterFlags.FF_SSL_TLS : PF_FilterFlags.FF_DEFAULT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_FTP,
                    (filterSSL ? PF_FilterFlags.FF_SSL_TLS : PF_FilterFlags.FF_DEFAULT) | 
                        PF_FilterFlags.FF_READ_ONLY_IN | PF_FilterFlags.FF_READ_ONLY_OUT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_ICQ,
                    0,//PF_FilterFlags.FF_READ_ONLY_IN | PF_FilterFlags.FF_READ_ONLY_OUT,
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                PFAPI.pf_addFilter(id,
                    PF_FilterType.FT_XMPP,
                    (filterSSL ? PF_FilterFlags.FF_SSL_TLS : PF_FilterFlags.FF_DEFAULT) |
                        PF_FilterFlags.FF_READ_ONLY_IN | PF_FilterFlags.FF_READ_ONLY_OUT, 
                    PF_OpTarget.OT_LAST,
                    PF_FilterType.FT_NONE);

                lock (m_params)
                {
                    if ((bool)m_params[ContentFilterParam.CFP_FILTER_RAW])
                    {
                        PFAPI.pf_addFilter(id,
                            PF_FilterType.FT_RAW,
                            PF_FilterFlags.FF_DEFAULT,
                            PF_OpTarget.OT_LAST,
                            PF_FilterType.FT_NONE);
                    }
                }
            }

            m_form.updateSessionListSafe(id, pConnInfo, true);
        }

        public override void tcpClosed(ulong id, NF_TCP_CONN_INFO pConnInfo)
        {
            m_form.updateSessionListSafe(id, pConnInfo, false);
        }

        public unsafe string loadString(PFStream pStream, bool seekToBegin)
        {
            if (pStream == null || pStream.size() == 0)
                return "";

            byte[] buf = new byte[pStream.size() + 1];
            uint len = 0;

            if (seekToBegin)
            {
                pStream.seek(0, (int)SeekOrigin.Begin);
            }

            fixed (byte* p = buf)
            {
                len = pStream.read((IntPtr)p, (uint)pStream.size());

                char[] cbuf = new char[len];
                
                for (int i = 0; i < len; i++)
                {
                    cbuf[i] = (char)buf[i];
                }
                
                return new String(cbuf);
            }
        }

        public unsafe string loadUnicodeString(PFStream pStream, bool seekToBegin)
        {
            if (pStream == null || pStream.size() == 0)
                return "";

            byte[] buf = new byte[pStream.size() + 2];
            uint len = 0;

            if (seekToBegin)
            {
                pStream.seek(0, (int)SeekOrigin.Begin);
            }

            fixed (byte* p = buf)
            {
                len = pStream.read((IntPtr)p, (uint)pStream.size());
                buf[len] = 0;
                buf[len+1] = 0;

                char[] cbuf = new char[len + 1];

                for (int i = 0; i < len; i += 2)
                {
                    cbuf[i/2] = (char)(buf[i] + 256 * buf[i+1]);
                }

                return new String(cbuf);
            }
        }

        public unsafe string loadUTF8String(PFStream pStream, bool seekToBegin)
        {
            if (pStream == null || pStream.size() == 0)
                return "";

            byte[] buf = new byte[pStream.size() + 1];
            uint len = 0;

            if (seekToBegin)
            {
                pStream.seek(0, (int)SeekOrigin.Begin);
            }

            fixed (byte* p = buf)
            {
                len = pStream.read((IntPtr)p, (uint)pStream.size());
                buf[len] = 0;

            	Encoding e = new UTF8Encoding();
                string s = e.GetString(buf);
                return s;
            }
        }

        public unsafe int loadInt(PFStream pStream, bool seekToBegin)
        {
            if (pStream == null || pStream.size() == 0)
                return 0;

            Int32 res = 0;

            if (seekToBegin)
            {
                pStream.seek(0, (int)SeekOrigin.Begin);
            }

            pStream.read((IntPtr)(byte*)&res, (uint)sizeof(Int32));

            return res;
        }


        unsafe bool saveString(PFStream pStream, string s, bool clearStream)
        {
            if (pStream == null)
                return false;

            if (clearStream)
            {
                pStream.reset();
            }

            foreach (char c in s.ToCharArray())
            {
                byte b = (byte)c;
                if (pStream.write((IntPtr)(byte*)&b, (uint)1) < 1)
                    return false;
            }
            return true;
        }

        public string getHttpUrl(PFObject pObject)
        {
            string url = "", status, host, uri;

            if (pObject.getType() != PF_ObjectType.OT_HTTP_REQUEST &&
                pObject.getType() != PF_ObjectType.OT_HTTP_RESPONSE)
                return "";

            try
            {

                PFHeader h = PFAPI.pf_readHeader(pObject.getStream((int)PF_HttpStream.HS_HEADER));

                if (pObject.getType() == PF_ObjectType.OT_HTTP_REQUEST)
                {
                    host = h["Host"];
                    status = loadString(pObject.getStream((int)PF_HttpStream.HS_STATUS), true);
                }
                else
                {
                    host = h[CustomHTTPHeaders.HTTP_EXHDR_RESPONSE_HOST];
                    status = h[CustomHTTPHeaders.HTTP_EXHDR_RESPONSE_REQUEST];
                }

                int pos = status.IndexOf(' ');
                if (pos != -1)
                {
                    pos++;

                    int pEnd = status.IndexOf(' ', pos);

                    if (pEnd != -1)
                    {
                        uri = status.Substring(pos, pEnd - pos);
                        if (uri.StartsWith("http://"))
                        {
                            url = uri;
                        }
                        else
                        {
                            url = "http://" + host + uri;
                        }
                    }
                }
            }
            catch (Exception)
            {
                url = "";
            }
            return url;
        }

        void postBlockHttpResponse(ulong id)
        {
            string blockPage;

            lock (m_params)
            {
                blockPage = (string)m_params[ContentFilterParam.CFP_BLOCK_PAGE];
            }

            PFObject obj = PFObject.create(PF_ObjectType.OT_HTTP_RESPONSE, 3);

            saveString(obj.getStream((int)PF_HttpStream.HS_STATUS), "HTTP/1.1 404 Not OK\r\n", true);

            PFHeader h = new PFHeader();
            h.Add("Content-Type", "text/html");
            h.Add("Content-Length", Convert.ToString(blockPage.Length));
            h.Add("Connection", "close");

            PFAPI.pf_writeHeader(obj.getStream((int)PF_HttpStream.HS_HEADER), h);

            saveString(obj.getStream((int)PF_HttpStream.HS_CONTENT), blockPage, true);

            PFAPI.pf_postObject(id, ref obj);

            obj.free();
        }

        unsafe bool filterHTTPResponse(ulong id, PFObject pObject)
        {
            bool block = false;
            string skipDomain;

            lock (m_params)
            {
                skipDomain = (string)m_params[ContentFilterParam.CFP_SKIP_DOMAIN];
            }

            if (skipDomain != null && skipDomain.Length > 0)
            {
                string url = getHttpUrl(pObject).ToLower();
                skipDomain = skipDomain.ToLower();
                if (url.IndexOf(skipDomain) != -1)
                {
                    // Allowed domain is found in URL.
                    return false;
                }
            }

            PFHeader h = PFAPI.pf_readHeader(pObject.getStream((int)PF_HttpStream.HS_HEADER));
            
            string contentType = h["Content-Type"];
            if (contentType != null && contentType.Contains("text/html"))
            {
                string htmlStopWord;

                lock (m_params)
                {
                    htmlStopWord = (string)m_params[ContentFilterParam.CFP_HTML_STOP_WORD];
                }

                if (htmlStopWord == null || htmlStopWord.Length == 0)
                    return false;

                htmlStopWord = htmlStopWord.ToLower();

                string html = loadString(pObject.getStream((int)PF_HttpStream.HS_CONTENT), true).ToLower();

                if (html.Contains(htmlStopWord))
                {
                    block = true;
                }
            }
            else
            {
                byte[] buf = new byte[5];

                PFStream pStream = pObject.getStream((int)PF_HttpStream.HS_CONTENT);
                
                fixed (byte* p = buf)
                {
                    pStream.seek(0, (int)SeekOrigin.Begin);
                    if (pStream.read((IntPtr)p, (uint)buf.Length) < buf.Length)
                        return false;
                }

                // Naive methods are used here, must be replaced with something more 
                // precise in real application.

                lock (m_params)
                {
                    if ((bool)m_params[ContentFilterParam.CFP_BLOCK_FLV])
                    {
                        // Check for flash movie (flv)
                        if (buf[0] == 'F' &&
                            buf[1] == 'L' &&
                            buf[2] == 'V')
                        {
                            block = true;
                        }
                    }

                    if (!block &&
                        (bool)m_params[ContentFilterParam.CFP_BLOCK_IMAGES])
                    {
                        // Check for GIF image
                        if (buf[0] == 'G' &&
                            buf[1] == 'I' &&
                            buf[2] == 'F')
                        {
                            block = true;
                        }
                        else
                            // Check for JPEG image
                            if (buf[0] == 0xff &&
                                buf[1] == 0xd8 &&
                                buf[2] == 0xff)
                            {
                                block = true;
                            }
                    }
                }
            }

            if (block)
            {
                postBlockHttpResponse(id);
            }

            return block;
        }

        bool filterHTTPRequest(ulong id, PFObject pObject)
        {
            bool block = false;
            string urlStopWord, skipDomain;

            lock (m_params)
            {
                urlStopWord = (string)m_params[ContentFilterParam.CFP_URL_STOP_WORD];
                skipDomain = (string)m_params[ContentFilterParam.CFP_SKIP_DOMAIN];
            }

            string url = getHttpUrl(pObject).ToLower();

            if (skipDomain != null && skipDomain.Length > 0)
            {
                skipDomain = skipDomain.ToLower();
                if (url.IndexOf(skipDomain) != -1)
                {
                    // Allowed domain is found in URL.
                    return false;
                }
            }

            if (urlStopWord != null && urlStopWord.Length > 0)
            {
                urlStopWord = urlStopWord.ToLower();

                if (url.Contains(urlStopWord))
                {
                    postBlockHttpResponse(id);
                    block = true;
                }
            }

            return block;
        }

        bool filterOutgoingMail(ulong id, PFObject pObject)
        {
            bool block = false;
            string blockAddress;

            lock (m_params)
            {
                blockAddress = (string)m_params[ContentFilterParam.CFP_BLOCK_ADDRESS];
            }

            if (blockAddress == null || blockAddress.Length == 0)
                return false;

            PFHeader h = PFAPI.pf_readHeader(pObject.getStream(0));
            string toAddress = h["To"];
            if (toAddress == null)
            {
                toAddress = h["Newsgroups"];
            }

            if (toAddress != null)
            {
                if (toAddress.ToLower().Contains(blockAddress))
                {
                    PFObject obj = PFObject.create(PF_ObjectType.OT_RAW_INCOMING, 1);
                    saveString(obj.getStream(0), "554 Message blocked!\r\n", true);
                    PFAPI.pf_postObject(id, ref obj);
                    block = true;
                }
            }

            return block;
        }

        void filterIncomingMail(ulong id, PFObject pObject)
        {
            string mailPrefix;

            lock (m_params)
            {
                mailPrefix = (string)m_params[ContentFilterParam.CFP_MAIL_PREFIX];
            }

            if (mailPrefix == null || mailPrefix.Length == 0)
                return;

            PFStream pStream = pObject.getStream(0);
            PFHeader h = PFAPI.pf_readHeader(pStream);
            string subject = h["Subject"];
            if (subject != null)
            {
                string content = loadString(pStream, true);
                int pos = content.IndexOf("\r\n\r\n");
                if (pos != -1)
                {
                    h.Remove("Subject");
                    h["Subject"] = mailPrefix + " " + subject;

                    pStream.reset();

                    PFAPI.pf_writeHeader(pStream, h);
                    saveString(pStream, content.Substring(pos + 4), false);
                }
            }
        }

        unsafe void postBlockICQResponse(ulong id, PFObject obj)
        {
            PFStream pStream;
            PFObject blockObj;

            // Copy and post the modified content to destination

            if (obj.getType() == PF_ObjectType.OT_ICQ_CHAT_MESSAGE_INCOMING)
            {
                blockObj = PFObject.create(PF_ObjectType.OT_ICQ_RESPONSE, 1);
            }
            else
            if (obj.getType() == PF_ObjectType.OT_ICQ_CHAT_MESSAGE_OUTGOING)
            {
                blockObj = PFObject.create(PF_ObjectType.OT_ICQ_REQUEST, 1);
            }
            else
                return;
            
            pStream = obj.getStream(0);

            byte[] buf = new byte[pStream.size()];

            pStream.seek(0, (int)SeekOrigin.Begin);

            fixed (byte* p = buf)
            {
                pStream.read((IntPtr)p, (uint)pStream.size());
            }

            if (buf.Length < 27)
                return;

            buf[26] = 0;

            pStream = blockObj.getStream(0);

            fixed (byte* p = buf)
            {
                pStream.write((IntPtr)p, (uint)buf.Length);
            }

            PFAPI.pf_postObject(id, ref blockObj);

            blockObj.free();
        }
        
        bool filterICQMessage(ulong id, PFObject obj)
        {
            string blockUIN;
            string blockString;
            bool blockFileTransfers;

            lock (m_params)
            {
                blockUIN = (string)m_params[ContentFilterParam.CFP_BLOCK_ICQ_UIN];
                blockString = (string)m_params[ContentFilterParam.CFP_BLOCK_ICQ_STRING];
                blockFileTransfers = (bool)m_params[ContentFilterParam.CFP_BLOCK_ICQ_FILE_TRANSFERS];
            }

            if (blockUIN != null && blockUIN.Length > 0)
            {
                string contactUIN = loadString(obj.getStream((int)PF_ICQStream.ICQS_CONTACT_UIN), true);
                if (contactUIN == blockUIN)
                {
                    postBlockICQResponse(id, obj);
                    return true;
                }
            }

            int textFormat = loadInt(obj.getStream((int)PF_ICQStream.ICQS_TEXT_FORMAT), true);

            if ((blockString != null && blockString.Length > 0))
            {
                string msgText = "";

                if (textFormat == (int)PF_ICQTextFormat.ICQTF_UNICODE)
                {
                    msgText = loadUnicodeString(obj.getStream((int)PF_ICQStream.ICQS_TEXT), true);
                }
                else
                if (textFormat == (int)PF_ICQTextFormat.ICQTF_UTF8)
                {
                    msgText = loadUTF8String(obj.getStream((int)PF_ICQStream.ICQS_TEXT), true);
                }
                else 
                if (textFormat == (int)PF_ICQTextFormat.ICQTF_ANSI)
                {
                    msgText = loadString(obj.getStream((int)PF_ICQStream.ICQS_TEXT), true);
                }

                if (msgText.ToLower().Contains(blockString.ToLower()))
                {
                    postBlockICQResponse(id, obj);
                    return true;
                }
            }

            if (blockFileTransfers)
            {
                if (textFormat == (int)PF_ICQTextFormat.ICQTF_FILE_TRANSFER)
                {
                    postBlockICQResponse(id, obj);
                    return true;
                }
            }

            return false;
        }

        public override void dataAvailable(ulong id, ref PFObject pObject)
        {
            bool blocked = false;
            PFObject clone = pObject.detach();

            clone.setReadOnly(pObject.isReadOnly());

            if (!pObject.isReadOnly())
            {
                try
                {
                    switch (pObject.getType())
                    {
                        case PF_ObjectType.OT_HTTP_RESPONSE:
                            blocked = filterHTTPResponse(id, clone);
                            break;
                        case PF_ObjectType.OT_HTTP_REQUEST:
                            blocked = filterHTTPRequest(id, clone);
                            break;
                        case PF_ObjectType.OT_SMTP_MAIL_OUTGOING:
                        case PF_ObjectType.OT_NNTP_POST:
                            blocked = filterOutgoingMail(id, clone);
                            break;
                        case PF_ObjectType.OT_POP3_MAIL_INCOMING:
                        case PF_ObjectType.OT_NNTP_ARTICLE:
                            filterIncomingMail(id, clone);
                            break;
                        case PF_ObjectType.OT_ICQ_CHAT_MESSAGE_OUTGOING:
                        case PF_ObjectType.OT_ICQ_CHAT_MESSAGE_INCOMING:
                            blocked = filterICQMessage(id, clone);
                            break;
                    }
                } catch (Exception)
                {
                }
            }
             
            if (!blocked)
                PFAPI.pf_postObject(id, ref clone);

            m_form.addObjectSafe(id, clone, blocked);
        }

        public override PF_DATA_PART_CHECK_RESULT dataPartAvailable(ulong id, ref PFObject pObject)
        {
            try
            {
                if (pObject.getType() == PF_ObjectType.OT_HTTP_RESPONSE)
                {
                    if (pObject.getStream((int)PF_HttpStream.HS_CONTENT).size() < 5)
                        return PF_DATA_PART_CHECK_RESULT.DPCR_MORE_DATA_REQUIRED;

                    if (filterHTTPResponse(id, pObject))
                    {
                        // Response blocked
                        return PF_DATA_PART_CHECK_RESULT.DPCR_BLOCK;
                    }

                    PFHeader h = PFAPI.pf_readHeader(pObject.getStream((int)PF_HttpStream.HS_HEADER));

                    string contentType = h["Content-Type"];
                    if (contentType == null)
                        return PF_DATA_PART_CHECK_RESULT.DPCR_FILTER_READ_ONLY;

                    if (contentType.Contains("text/html"))
                    {
                        // Switch to DPCR_FILTER mode if we must filter HTML
                        lock (m_params)
                        {
                            string htmlStopWord = (string)m_params[ContentFilterParam.CFP_HTML_STOP_WORD];
                            if (htmlStopWord != null && htmlStopWord.Length > 0)
                                return PF_DATA_PART_CHECK_RESULT.DPCR_FILTER;
                        }
                    }
                }
                else
                    if (pObject.getType() == PF_ObjectType.OT_HTTP_REQUEST)
                    {
                        if (filterHTTPRequest(id, pObject))
                        {
                            // Request blocked
                            return PF_DATA_PART_CHECK_RESULT.DPCR_BLOCK;
                        }
                    }
            }
            catch (Exception)
            {
            }

            return PF_DATA_PART_CHECK_RESULT.DPCR_FILTER_READ_ONLY;
        }

        public void setParam(ContentFilterParam type, object value)
        {
            lock (m_params)
            {
                m_params[type] = value;
            }
        }

        public bool start(Form1 form)
        {
            m_form = form;

            if (!PFAPI.pf_init(this, "c:\\netfilter2"))
                return false;

            PFAPI.pf_setExceptionsTimeout(eEXCEPTION_CLASS.EXC_GENERIC, 60 * 60);
            PFAPI.pf_setExceptionsTimeout(eEXCEPTION_CLASS.EXC_TLS, 60 * 60);
            PFAPI.pf_setExceptionsTimeout(eEXCEPTION_CLASS.EXC_CERT_REVOKED, 60 * 60);

            PFAPI.pf_setRootSSLCertSubject("Sample CA");

            if (NFAPI.nf_init("netfilter2", PFAPI.pf_getNFEventHandler()) != 0)
            {
                PFAPI.pf_free();
                return false;
            }

            NFAPI.nf_setTCPTimeout(0);

            NF_RULE rule = new NF_RULE();

            // Do not filter local traffic
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_ALLOW;
            rule.ip_family = (ushort)AddressFamily.InterNetwork;
            rule.remoteIpAddress = IPAddress.Parse("127.0.0.1").GetAddressBytes();
            rule.remoteIpAddressMask = IPAddress.Parse("255.0.0.0").GetAddressBytes();
            NFAPI.nf_addRule(rule, 0);

            rule = new NF_RULE();
            // Filter outgoing TCP connections 
            rule.protocol = (int)ProtocolType.Tcp;
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_FILTER;
            NFAPI.nf_addRule(rule, 0);

            // Disable QUIC protocol to make the browsers switch to generic HTTP

            rule = new NF_RULE();
            rule.protocol = (int)ProtocolType.Udp;
            rule.remotePort = (ushort)IPAddress.HostToNetworkOrder((Int16)80);
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_BLOCK;
            NFAPI.nf_addRule(rule, 0);

            rule = new NF_RULE();
            rule.protocol = (int)ProtocolType.Udp;
            rule.remotePort = (ushort)IPAddress.HostToNetworkOrder((Int16)443);
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_BLOCK;
            NFAPI.nf_addRule(rule, 0);

            return true;
        }

        public void stop()
        {
            NFAPI.nf_free();
            PFAPI.pf_free();
        }
    }
}
