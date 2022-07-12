using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using pfapinet;
using nfapinet;
using System.Net;
using System.Net.Sockets;
using System.Diagnostics;
using System.IO;
using System.Collections.Specialized;

namespace PFNetFilterCS
{

    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        Filter m_filter = new Filter();
        PFObject m_storageObject = PFObject.create(PF_ObjectType.OT_NULL, 1);

        private void Start_Click(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_FILTER_SSL, FilterSSL.Checked);
            m_filter.setParam(ContentFilterParam.CFP_FILTER_RAW, FilterRaw.Checked);

            // HTTP parameters
            m_filter.setParam(ContentFilterParam.CFP_URL_STOP_WORD, UrlStopWord.Text);
            m_filter.setParam(ContentFilterParam.CFP_HTML_STOP_WORD, HtmlStopWord.Text);
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_PAGE, BlockPage.Text);
            m_filter.setParam(ContentFilterParam.CFP_SKIP_DOMAIN, SkipDomain.Text);
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_IMAGES, BlockImages.Checked);
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_FLV, BlockFLV.Checked);

            // Mail parameters
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ADDRESS, SMTPBlackAddress.Text);
            m_filter.setParam(ContentFilterParam.CFP_MAIL_PREFIX, POP3Prefix.Text);

            // ICQ parameters
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ICQ_UIN, blockICQUIN.Text);
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ICQ_STRING, blockICQString.Text);
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ICQ_FILE_TRANSFERS, ICQBlockFileTransfers.Checked);

            if (m_filter.start(this))
            {
                Start.Enabled = false;
                Stop.Enabled = true;
            }
        }

        private void Stop_Click(object sender, EventArgs e)
        {
            m_filter.stop();
            Start.Enabled = true;
            Stop.Enabled = false;

            Sessions.Items.Clear();
        }

        private void FilterSSL_CheckedChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_FILTER_SSL, FilterSSL.Checked);
        }

        private void FilterRaw_CheckedChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_FILTER_RAW, FilterRaw.Checked);
        }

        private void UrlStopWord_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_URL_STOP_WORD, UrlStopWord.Text);
        }

        private void HtmlStopWord_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_HTML_STOP_WORD, HtmlStopWord.Text);
        }

        private void BlockPage_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_PAGE, BlockPage.Text);
        }

        private void SkipDomain_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_SKIP_DOMAIN, SkipDomain.Text);
        }

        private void BlockImages_CheckedChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_IMAGES, BlockImages.Checked);
        }

        private void BlockFLV_CheckedChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_FLV, BlockFLV.Checked);
        }

        public unsafe class NFUtil
        {
            public static SocketAddress convertAddress(byte[] buf)
            {
                if (buf == null)
                {
                    return new SocketAddress(AddressFamily.InterNetwork);
                }

                SocketAddress addr = new SocketAddress((AddressFamily)(buf[0]), (int)NF_CONSTS.NF_MAX_ADDRESS_LENGTH);

                for (int i = 0; i < (int)NF_CONSTS.NF_MAX_ADDRESS_LENGTH; i++)
                {
                    addr[i] = buf[i];
                }

                return addr;
            }

            public static string addressToString(SocketAddress addr)
            {
                IPEndPoint ipep;

                if (addr.Family == AddressFamily.InterNetworkV6)
                {
                    ipep = new IPEndPoint(IPAddress.IPv6None, 0);
                }
                else
                {
                    ipep = new IPEndPoint(0, 0);
                }
                ipep = (IPEndPoint)ipep.Create(addr);
                return ipep.ToString();
            }
        }

        string getSessionProperties(NF_TCP_CONN_INFO connInfo)
        {
            string s = "";

            try
            {
                SocketAddress localAddr = NFUtil.convertAddress(connInfo.localAddress);
                s += NFUtil.addressToString(localAddr);

                s += "<->";

                SocketAddress remoteAddr = NFUtil.convertAddress(connInfo.remoteAddress);
                s += NFUtil.addressToString(remoteAddr);
            }
            catch (Exception)
            {
            }

            s += " [pid=" + connInfo.processId + " owner=" + PFAPI.pf_getProcessOwnerW(connInfo.processId) + "] ";

            s += NFAPI.nf_getProcessName(connInfo.processId);

            return s;
        }


        void updateSessionListInternal(ulong id, NF_TCP_CONN_INFO pConnInfo, bool newItem)
        {
            if (newItem)
            {
                string sid = Convert.ToString(id);
                ListViewItem lvi = Sessions.Items.Add(sid, sid, -1);
                lvi.SubItems.Add(getSessionProperties(pConnInfo));
            }
            else
            {
                Sessions.Items.RemoveByKey(Convert.ToString(id));
            }
        }

        delegate void dgUpdateSessionList(ulong id, NF_TCP_CONN_INFO pConnInfo, bool newItem);

        public void updateSessionListSafe(ulong id, NF_TCP_CONN_INFO pConnInfo, bool newItem)
        {
            BeginInvoke(new dgUpdateSessionList(updateSessionListInternal),
                new Object[] { id, pConnInfo, newItem });
        }

        unsafe ulong saveObject(PFObject obj)
        {
            int ot, nStreams;
            uint streamLen, rwLen;
            ulong pos = 0;
            PFStream pStg = m_storageObject.getStream(0);
            byte[] tempBuf = new byte[1000];

            fixed (byte* pTempBuf = tempBuf)
            {
                pos = pStg.seek(0, (int)SeekOrigin.End);

                ot = (int)obj.getType();
                pStg.write((IntPtr)(byte*)&ot, (uint)sizeof(int));
                nStreams = (int)obj.getStreamCount();
                pStg.write((IntPtr)(byte*)&nStreams, (uint)sizeof(int));
                for (int i = 0; i < nStreams; i++)
                {
                    PFStream pStream = obj.getStream(i);
                    pStream.seek(0, (int)SeekOrigin.Begin);
                    streamLen = (uint)pStream.size();
                    pStg.write((IntPtr)(byte*)&streamLen, (uint)sizeof(uint));
                    while (streamLen > 0)
                    {
                        rwLen = (uint)Math.Min(tempBuf.Length, streamLen);
                        rwLen = pStream.read((IntPtr)pTempBuf, rwLen);
                        if (rwLen <= 0)
                            break;
                        pStg.write((IntPtr)pTempBuf, rwLen);
                        streamLen -= rwLen;
                    }
                    pStream.seek(0, (int)SeekOrigin.Begin);
                }
            }

            return pos;
        }

        unsafe PFObject loadObject(ulong offset)
        {
            int ot, nStreams;
            uint streamLen, rwLen;
            PFStream pStg = m_storageObject.getStream(0);
            byte[] tempBuf = new byte[1000];
            PFObject obj = null;

            fixed (byte* pTempBuf = tempBuf)
            {
                pStg.seek(offset, (int)SeekOrigin.Begin);

                pStg.read((IntPtr)(byte*)&ot, (uint)sizeof(int));
                pStg.read((IntPtr)(byte*)&nStreams, (uint)sizeof(int));

                obj = PFObject.create((PF_ObjectType)ot, nStreams);

                for (int i = 0; i < nStreams; i++)
                {
                    PFStream pStream = obj.getStream(i);
                    pStg.read((IntPtr)(byte*)&streamLen, (uint)sizeof(uint));
                    while (streamLen > 0)
                    {
                        rwLen = (uint)Math.Min(tempBuf.Length, streamLen);
                        rwLen = pStg.read((IntPtr)pTempBuf, rwLen);
                        if (rwLen <= 0)
                            break;
                        pStream.write((IntPtr)pTempBuf, rwLen);
                        streamLen -= rwLen;
                    }
                    pStream.seek(0, (int)SeekOrigin.Begin);
                }
            }

            return obj;
        }

        private int c = 0;
        unsafe void saveObjectToFile(ulong id, PFObject obj)
        {
            string fileName = String.Format("{0}_{1}_{2}.bin", id, c++, obj.getType());

            FileStream fs = new FileStream(fileName, FileMode.CreateNew);
            BinaryWriter w = new BinaryWriter(fs);
            for (int i = 0; i < obj.getStreamCount(); i++)
            {
                PFStream pStream = obj.getStream(i);
                byte[] buf = new byte[pStream.size()];
                int len = 0;

                pStream.seek(0, (int)SeekOrigin.Begin);

                fixed (byte* p = buf)
                {
                    len = (int)pStream.read((IntPtr)p, (uint)pStream.size());
                    w.Write(buf, 0, len);
                }
            }

            w.Close();
            fs.Close();
        }

        void addHTTPObject(ulong id, PFObject obj, bool blocked)
        {
            string ids = Convert.ToString(id);
            ListViewItem lvi = HttpList.Items.Add(ids);
            if (obj.getType() == PF_ObjectType.OT_HTTP_REQUEST)
            {
                lvi.SubItems.Add("Request");
            }
            else
            {
                lvi.SubItems.Add("Response");
            }

            lvi.SubItems.Add(m_filter.loadString(obj.getStream((int)PF_HttpStream.HS_STATUS), true));
            lvi.SubItems.Add(m_filter.getHttpUrl(obj));
            lvi.SubItems.Add(Convert.ToString(obj.getStream((int)PF_HttpStream.HS_CONTENT).size()));
            lvi.SubItems.Add(blocked ? "BLOCKED" : "");

            ListViewItem[] items = Sessions.Items.Find(ids, false);
            if (items.Length > 0)
            {
                lvi.SubItems.Add(items[0].SubItems[1]);
            }

            lvi.Tag = saveObject(obj);
        }

        void addMailObject(ulong id, PFObject obj, bool blocked)
        {
            string ids = Convert.ToString(id);
            ListViewItem lvi = MailList.Items.Add(ids);

            switch (obj.getType())
            {
                case PF_ObjectType.OT_SMTP_MAIL_OUTGOING:
                    lvi.SubItems.Add("Outgoing mail");
                    break;
                case PF_ObjectType.OT_POP3_MAIL_INCOMING:
                    lvi.SubItems.Add("Incoming mail");
                    break;
                case PF_ObjectType.OT_NNTP_POST:
                    lvi.SubItems.Add("Outgoing news");
                    break;
                case PF_ObjectType.OT_NNTP_ARTICLE:
                    lvi.SubItems.Add("Incoming news");
                    break;
            }

            lvi.SubItems.Add(Convert.ToString(obj.getStream(0).size()));
            lvi.SubItems.Add(blocked ? "BLOCKED" : "");

            ListViewItem[] items = Sessions.Items.Find(ids, false);
            if (items.Length > 0)
            {
                lvi.SubItems.Add(items[0].SubItems[1]);
            }

            lvi.Tag = saveObject(obj);
        }

        void addRawObject(ulong id, PFObject obj)
        {
            string ids = Convert.ToString(id);
            ListViewItem lvi = RawList.Items.Add(ids);
            if (obj.getType() == PF_ObjectType.OT_RAW_OUTGOING)
            {
                lvi.SubItems.Add("Outgoing");
            }
            else
            {
                lvi.SubItems.Add("Incoming");
            }

            lvi.SubItems.Add(Convert.ToString(obj.getStream(0).size()));

            ListViewItem[] items = Sessions.Items.Find(ids, false);
            if (items.Length > 0)
            {
                lvi.SubItems.Add(items[0].SubItems[1]);
            }

            lvi.Tag = saveObject(obj);
        }

        void addXmppObject(ulong id, PFObject obj)
        {
            string ids = Convert.ToString(id);
            ListViewItem lvi = XMPPList.Items.Add(ids);
            if (obj.getType() == PF_ObjectType.OT_XMPP_REQUEST)
            {
                lvi.SubItems.Add("Request");
            }
            else
            {
                lvi.SubItems.Add("Response");
            }

            lvi.SubItems.Add(Convert.ToString(obj.getStream(0).size()));

            ListViewItem[] items = Sessions.Items.Find(ids, false);
            if (items.Length > 0)
            {
                lvi.SubItems.Add(items[0].SubItems[1]);
            }

            lvi.Tag = saveObject(obj);
        }

        void addFTPObject(ulong id, PFObject obj)
        {
            string ids = Convert.ToString(id);
            ListViewItem lvi = FTPList.Items.Add(ids);

            switch (obj.getType())
            {
                case PF_ObjectType.OT_FTP_COMMAND:
                    lvi.SubItems.Add("Command");
                    break;
                case PF_ObjectType.OT_FTP_RESPONSE:
                    lvi.SubItems.Add("Response");
                    break;
                case PF_ObjectType.OT_FTP_DATA_OUTGOING:
                    lvi.SubItems.Add("Outgoing data");
                    break;
                case PF_ObjectType.OT_FTP_DATA_INCOMING:
                    lvi.SubItems.Add("Incoming data");
                    break;
                case PF_ObjectType.OT_FTP_DATA_PART_OUTGOING:
                    lvi.SubItems.Add("Outgoing data part");
                    break;
                case PF_ObjectType.OT_FTP_DATA_PART_INCOMING:
                    lvi.SubItems.Add("Incoming data part");
                    break;
            }

            lvi.SubItems.Add(Convert.ToString(obj.getStream(0).size()));

            ListViewItem[] items = Sessions.Items.Find(ids, false);
            if (items.Length > 0)
            {
                lvi.SubItems.Add(items[0].SubItems[1]);
            }

            lvi.Tag = saveObject(obj);
        }

        void addICQObject(ulong id, PFObject obj, bool blocked)
        {
            string ids = Convert.ToString(id);
            ListViewItem lvi = ICQList.Items.Add(ids);

            switch (obj.getType())
            {
                case PF_ObjectType.OT_ICQ_CHAT_MESSAGE_OUTGOING:
                    lvi.SubItems.Add("Outgoing message");
                    break;
                case PF_ObjectType.OT_ICQ_CHAT_MESSAGE_INCOMING:
                    lvi.SubItems.Add("Incoming message");
                    break;
            }

            lvi.SubItems.Add(Convert.ToString(obj.getStream(0).size()));
            lvi.SubItems.Add(blocked ? "BLOCKED" : "");

            ListViewItem[] items = Sessions.Items.Find(ids, false);
            if (items.Length > 0)
            {
                lvi.SubItems.Add(items[0].SubItems[1]);
            }

            lvi.Tag = saveObject(obj);
        }

        void addObjectInternal(ulong id, PFObject obj, bool blocked)
        {
            string sid = Convert.ToString(id);

            switch (obj.getType())
            {
                case PF_ObjectType.OT_HTTP_REQUEST:
                case PF_ObjectType.OT_HTTP_RESPONSE:
                    addHTTPObject(id, obj, blocked);
                    break;
                case PF_ObjectType.OT_SMTP_MAIL_OUTGOING:
                case PF_ObjectType.OT_POP3_MAIL_INCOMING:
                case PF_ObjectType.OT_NNTP_POST:
                case PF_ObjectType.OT_NNTP_ARTICLE:
                    addMailObject(id, obj, blocked);
                    break;
                case PF_ObjectType.OT_FTP_COMMAND:
                case PF_ObjectType.OT_FTP_RESPONSE:
                case PF_ObjectType.OT_FTP_DATA_OUTGOING:
                case PF_ObjectType.OT_FTP_DATA_INCOMING:
                case PF_ObjectType.OT_FTP_DATA_PART_OUTGOING:
                case PF_ObjectType.OT_FTP_DATA_PART_INCOMING:
                    addFTPObject(id, obj);
                    break;
                case PF_ObjectType.OT_ICQ_CHAT_MESSAGE_OUTGOING:
                case PF_ObjectType.OT_ICQ_CHAT_MESSAGE_INCOMING:
                    addICQObject(id, obj, blocked);
                    break;
                case PF_ObjectType.OT_RAW_OUTGOING:
                case PF_ObjectType.OT_RAW_INCOMING:
                    addRawObject(id, obj);
                    break;
                case PF_ObjectType.OT_XMPP_REQUEST:
                case PF_ObjectType.OT_XMPP_RESPONSE:
                    addXmppObject(id, obj);
                    break;
            }

            if (SaveToBin.Checked)
            {
                if (obj.getStreamCount() > 0)
                {
                    saveObjectToFile(id, obj);
                }
            }
        }

        delegate void dgAddObject(ulong id, PFObject obj, bool blocked);

        public void addObjectSafe(ulong id, PFObject obj, bool blocked)
        {
            BeginInvoke(new dgAddObject(addObjectInternal),
                new Object[] { id, obj, blocked });
        }

        private void HttpList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (HttpList.SelectedItems.Count == 0)
                return;

            try
            {
                PFObject obj = loadObject((ulong)HttpList.SelectedItems[0].Tag);
                if (obj != null)
                {
                    HttpHeader.Items.Clear();

                    PFHeader h = PFAPI.pf_readHeader(obj.getStream((int)PF_HttpStream.HS_HEADER));
                    foreach (KeyValuePair<string, string> v in h)
                    {
                        ListViewItem lvi = HttpHeader.Items.Add(v.Key);
                        lvi.SubItems.Add(v.Value);
                    }

                    string contentType = h["Content-Type"];
                    if (contentType == null)
                    {
                        contentType = "";
                    }
                    if (contentType.Contains("text/"))
                    {
                        HttpContent.Text = m_filter.loadString(obj.getStream((int)PF_HttpStream.HS_CONTENT), true);
                    }
                    else
                    {
                        HttpContent.Text = contentType;
                    }
                }
            }
            catch (Exception)
            {
            }
        }

        private void DeleteSavedContent_Click(object sender, EventArgs e)
        {
            HttpList.Items.Clear();
            HttpHeader.Items.Clear();
            HttpContent.Clear();
            
            MailList.Items.Clear();
            MailHeader.Items.Clear();
            MailContent.Clear();
            
            RawList.Items.Clear();
            RawContent.Clear();
            RawContentHexBox.ByteProvider = null;
            
            FTPList.Items.Clear();
            FTPText.Clear();
            FTPHexBox.ByteProvider = null;
            FTPMetaData.Clear();
            
            ICQList.Items.Clear();
            ICQHeader.Items.Clear();
            ICQContent.Clear();

            XMPPList.Items.Clear();
            XMPPContent.Clear();

            m_storageObject.getStream(0).reset();
        }

        private void SMTPBlackAddress_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ADDRESS, SMTPBlackAddress.Text);
        }

        private void POP3Prefix_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_MAIL_PREFIX, POP3Prefix.Text);
        }

        private void MailList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (MailList.SelectedItems.Count == 0)
                return;

            PFObject obj = loadObject((ulong)MailList.SelectedItems[0].Tag);
            if (obj != null)
            {
                MailHeader.Items.Clear();
                PFHeader h = PFAPI.pf_readHeader(obj.getStream(0));

                foreach (KeyValuePair<string, string> v in h)
                {
                    ListViewItem lvi = MailHeader.Items.Add(v.Key);
                    lvi.SubItems.Add(v.Value);
                }

                MailContent.Text = m_filter.loadString(obj.getStream(0), true);
            }
        }

        private unsafe void RawList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (RawList.SelectedItems.Count == 0)
                return;

            PFObject obj = loadObject((ulong)RawList.SelectedItems[0].Tag);
            if (obj != null)
            {
                PFStream s = obj.getStream(0);

                s.seek(0, (int)SeekOrigin.Begin);
                RawContent.Text = m_filter.loadString(s, true);

                if (s.size() > 0)
                {
                    byte[] buf = new byte[s.size()];
                    fixed (byte* p = buf)
                    {
                        s.seek(0, (int)SeekOrigin.Begin);
                        s.read((IntPtr)p, (uint)s.size());
                    }
                    RawContentHexBox.ByteProvider =
                        new Be.Windows.Forms.DynamicByteProvider(buf);
                }
                else
                {
                    RawContentHexBox.ByteProvider = null;
                }
            }
        }

        private unsafe void FTPList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (FTPList.SelectedItems.Count == 0)
                return;

            PFObject obj = loadObject((ulong)FTPList.SelectedItems[0].Tag);
            if (obj != null)
            {
                PFStream s = obj.getStream(0);

                s.seek(0, (int)SeekOrigin.Begin);
                FTPText.Text = m_filter.loadString(s, true);

                if (s.size() > 0)
                {
                    byte[] buf = new byte[s.size()];
                    fixed (byte* p = buf)
                    {
                        s.seek(0, (int)SeekOrigin.Begin);
                        s.read((IntPtr)p, (uint)s.size());
                    }
                    FTPHexBox.ByteProvider = new Be.Windows.Forms.DynamicByteProvider(buf);
                }
                else
                {
                    FTPHexBox.ByteProvider = null;
                }

                s = null;

                try
                {
                    s = obj.getStream(1);
                }
                catch (Exception )
                {
                }

                // Update metadata tab
                if (s != null && s.size() > 0)
                {
                    FTPMetaData.Text = m_filter.loadString(s, true);
                    if (FTPTabControl.TabPages.Count < 3)
                    {
                        FTPTabControl.TabPages.Add(FTPMetaDataTab);
                    }
                }
                else
                {
                    FTPMetaData.Text = "";
                    if (FTPTabControl.TabPages.Count > 2)
                    {
                        FTPTabControl.TabPages.Remove(FTPMetaDataTab);
                    }
                }
            }
        }

        private void blockICQUIN_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ICQ_UIN, blockICQUIN.Text);
        }

        private void blockICQString_TextChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ICQ_STRING, blockICQString.Text);
        }

        private void ICQBlockFileTransfers_CheckedChanged(object sender, EventArgs e)
        {
            m_filter.setParam(ContentFilterParam.CFP_BLOCK_ICQ_FILE_TRANSFERS, ICQBlockFileTransfers.Checked);
        }

        private void ICQList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (ICQList.SelectedItems.Count == 0)
                return;

            PFObject obj = loadObject((ulong)ICQList.SelectedItems[0].Tag);
            if (obj != null)
            {
                ICQHeader.Items.Clear();
                ICQContent.Text = "";

                ListViewItem lvi = null;

                try
                {
                    lvi = ICQHeader.Items.Add("User:");
                    lvi.SubItems.Add(m_filter.loadString(obj.getStream((int)PF_ICQStream.ICQS_USER_UIN), true));
                    lvi = ICQHeader.Items.Add("Contact:");
                    lvi.SubItems.Add(m_filter.loadString(obj.getStream((int)PF_ICQStream.ICQS_CONTACT_UIN), true));

                    int textFormat = m_filter.loadInt(obj.getStream((int)PF_ICQStream.ICQS_TEXT_FORMAT), true);
                                       
                    if (textFormat == (int)PF_ICQTextFormat.ICQTF_UNICODE)
                    {
                        ICQContent.Text = m_filter.loadUnicodeString(obj.getStream((int)PF_ICQStream.ICQS_TEXT), true);
                    }
                    else
                    if (textFormat == (int)PF_ICQTextFormat.ICQTF_UTF8)
                    {
                        ICQContent.Text = m_filter.loadUTF8String(obj.getStream((int)PF_ICQStream.ICQS_TEXT), true);
                    }
                    else
                    {
                        ICQContent.Text = m_filter.loadString(obj.getStream((int)PF_ICQStream.ICQS_TEXT), true);
                    }
                }
                catch (Exception)
                { }
            }
        }

        private void XMPPList_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (XMPPList.SelectedItems.Count == 0)
                return;

            PFObject obj = loadObject((ulong)XMPPList.SelectedItems[0].Tag);
            if (obj != null)
            {
                XMPPContent.Text = m_filter.loadString(obj.getStream(0), true);
            }
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            m_filter.stop();
        }

    }
}