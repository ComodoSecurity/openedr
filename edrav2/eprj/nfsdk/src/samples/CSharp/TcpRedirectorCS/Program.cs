/**
	This sample allows to redirect TCP connections with given parameters to the specified endpoint.
	It also supports redirecting to HTTPS or SOCKS proxy.

	Usage: TcpRedirectorCS.exe [-s|t] [-p Port] [-pid ProcessId] -r IP:Port 
		-t : redirect via HTTPS proxy at IP:Port. The proxy must support HTTP tunneling (CONNECT requests)
		-s : redirect via SOCKS4 proxy at IP:Port. 
		Port : remote port to intercept
		ProcessId : redirect connections of the process with given PID
		ProxyPid : it is necessary to specify proxy process id if the connection is redirected to local proxy
		IP:Port : redirect connections to the specified IP endpoint

	Example:
		
		TcpRedirectorCS.exe -t -p 110 -r 163.15.64.8:3128

		The sample will redirect POP3 connections to HTTPS proxy at 163.15.64.8:3128 with tunneling to real server. 
**/

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Collections;
using nfapinet;

namespace TcpRedirectorCS
{
    enum eSOCKS_VERSION
    {
	    SOCKS_4 = 4,
	    SOCKS_5 = 5
    };

    enum eSOCKS4_COMMAND
    {
	    S4C_CONNECT = 1,
	    S4C_BIND = 2
    };

    struct SOCKS4_REQUEST
    {
	    public byte	version;
        public byte command;
        public short port;
        public int ip;
        public byte userid;
    };

    enum eSOCKS4_RESCODE
    {
	    S4RC_SUCCESS = 0x5a
    };

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

        public static IPEndPoint stringToAddress(string saddr)
        {
            int div = saddr.LastIndexOf(':');
            if (div > 0)
            {
                string sIp = saddr.Substring(0, div);
                IPAddress ipAddr = IPAddress.Parse(sIp);

                string sPort = saddr.Substring(div+1, saddr.Length - div-1);
                ushort port = (ushort)Convert.ToUInt16(sPort);

                return new IPEndPoint(ipAddr, port);
            }

            return null;
        }

        public static byte[] stringToBuffer(string str)
        {
	        Encoding e = Encoding.GetEncoding("ISO-8859-1");
	        return e.GetBytes(str);
        }
    }

    // API events handler
    unsafe public class EventHandler : NF_EventHandler
    {
        public enum PROXY_TYPE
        {
            PT_NONE,
            PT_HTTPS,
            PT_SOCKS
        };

        public PROXY_TYPE m_proxyType = PROXY_TYPE.PT_NONE;
        public SocketAddress redirectTo = null;
        public uint proxyPid = 0;

        public class ORIGINAL_CONN_INFO
        {
            public byte[] remoteAddress;
        }

        Hashtable connInfoMap = new Hashtable();
              

        public void threadStart()
        {
            Console.Out.WriteLine("threadStart");
        }
		
        public void threadEnd()
		{
            Console.Out.WriteLine("threadEnd");
        }

        public void tcpConnectRequest(ulong id, ref NF_TCP_CONN_INFO connInfo)
        {
            Console.Out.WriteLine("connectRequest id=" + id);

            SocketAddress sa = NFUtil.convertAddress(connInfo.remoteAddress);

            if (m_proxyType != PROXY_TYPE.PT_NONE)
		    {
			    ORIGINAL_CONN_INFO oci = new ORIGINAL_CONN_INFO();
			    
                oci.remoteAddress = (byte[])connInfo.remoteAddress.Clone();

			    // Save the original remote address
			    connInfoMap[id] = oci;
		    }

		    // Redirect the connection if it is not already redirected
		    if (sa != redirectTo &&
                connInfo.processId != proxyPid)
		    {
                try
                {
                    Console.Out.WriteLine("Redirecting from " + NFUtil.addressToString(NFUtil.convertAddress(connInfo.remoteAddress)) +
                        " to " + NFUtil.addressToString(redirectTo));
                }
                catch (Exception)
                {
                }

                // Change the remote address
                for (int i = 0; i < redirectTo.Size; i++)
                {
                    connInfo.remoteAddress[i] = redirectTo[i];
                }

                connInfo.processId = proxyPid;

                if (m_proxyType != PROXY_TYPE.PT_NONE)
			    {
				    // Filtering is required only for HTTP tunneling.
				    // The first incoming packet will be a response from proxy that must be skipped.
				    connInfo.filteringFlag = (int)NF_FILTERING_FLAG.NF_FILTER;
			    }
		    }

        }

        public unsafe void tcpConnected(ulong id, NF_TCP_CONN_INFO connInfo)
		{
            string s = "TCP id=" + id + " ";

            s += ((NF_DIRECTION)connInfo.direction == NF_DIRECTION.NF_D_IN) ? "[in]" : "[out]";

            s += " process: ";

            s += nfapinet.NFAPI.nf_getProcessName(connInfo.processId);

            s += "\n";

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

            s += " ";

            Console.Out.WriteLine(s);

		    if ((connInfo.filteringFlag & (int)NF_FILTERING_FLAG.NF_FILTER) > 0 &&
			    (connInfo.direction == (int)NF_DIRECTION.NF_D_OUT))
		    {
                switch (m_proxyType)
                {
                    case PROXY_TYPE.PT_HTTPS:
                        {
                            // Generate CONNECT request using saved original remote address

                            if (!connInfoMap.ContainsKey(id))
                                return;

                            ORIGINAL_CONN_INFO oci = (ORIGINAL_CONN_INFO)connInfoMap[id];

                            SocketAddress sa = NFUtil.convertAddress(oci.remoteAddress);

                            string request = "CONNECT " + NFUtil.addressToString(sa) + " HTTP/1.0\r\n\r\n";

                            // Send the request first

                            fixed (byte* p = NFUtil.stringToBuffer(request))
                            {
                                NFAPI.nf_tcpPostSend(id, (IntPtr)p, request.Length);
                            }
                        }
                        break;
                    case PROXY_TYPE.PT_SOCKS:
                        {
                            // Generate S4C_CONNECT request using saved original remote address

                            if (!connInfoMap.ContainsKey(id))
                                return;

                            ORIGINAL_CONN_INFO oci = (ORIGINAL_CONN_INFO)connInfoMap[id];

                            SocketAddress sa = NFUtil.convertAddress(oci.remoteAddress);
                            if (sa.Family == AddressFamily.InterNetworkV6)
                                return;

                            IPEndPoint ipep = new IPEndPoint(0, 0);
                            ipep = (IPEndPoint)ipep.Create(sa);

                            SOCKS4_REQUEST request = new SOCKS4_REQUEST();
                            request.version = (byte)eSOCKS_VERSION.SOCKS_4;
                            request.command = (byte)eSOCKS4_COMMAND.S4C_CONNECT;
                            request.port = (short)IPAddress.HostToNetworkOrder((short)ipep.Port);
                            request.ip = (int)ipep.Address.Address;
                            request.userid = 0;

                            // Send the request first
                            NFAPI.nf_tcpPostSend(id, (IntPtr)(byte*)&request, 9);
                        }
                        break;
                }
		    }

        }

		public void tcpClosed(ulong id, NF_TCP_CONN_INFO connInfo)
		{
            string s = "TCP id=" + id + " ";

            s += ((NF_DIRECTION)connInfo.direction == NF_DIRECTION.NF_D_IN) ? "[in]" : "[out]";

            s += " closed ";

            Console.Out.WriteLine(s);

            if (m_proxyType != PROXY_TYPE.PT_NONE)
            {
                connInfoMap.Remove(id);
            }
        }

        public unsafe void tcpReceive(ulong id, byte[] buf)
        {
            // Filter the data in buf

            fixed (byte* p = buf)
            {
                NFAPI.nf_tcpPostReceive(id, (IntPtr)p, buf.Length);
            }
        }

        public void tcpReceive(ulong id, IntPtr buf, int len)
		{
            string s = "TCP id=" + id + " receive len=" + len;
            Console.Out.WriteLine(s);

            if (m_proxyType != PROXY_TYPE.PT_NONE)
            {
                if (connInfoMap.ContainsKey(id))
                {
                    connInfoMap.Remove(id);
                    // The first packet is a response from proxy server.
                    // Skip it.
                    return;
                }
            }
            
            // Copy the data to managed buffer for convenience
            byte[] mbuf = new byte[len];
            Marshal.Copy((IntPtr)buf, mbuf, 0, len);

            tcpReceive(id, mbuf);

            NFAPI.nf_tcpDisableFiltering(id);
		}

        public unsafe void tcpSend(ulong id, byte[] buf)
        {
            // Filter the data in buf

            fixed (byte* p = buf)
            {
                NFAPI.nf_tcpPostSend(id, (IntPtr)p, buf.Length);
            }
        }

        public void tcpSend(ulong id, IntPtr buf, int len)
		{
            string s = "TCP id=" + id + " send len=" + len;
            Console.Out.WriteLine(s);

            byte[] mbuf = new byte[len];
            Marshal.Copy((IntPtr)buf, mbuf, 0, len);
            tcpSend(id, mbuf);
        }

		public void tcpCanReceive(ulong id)
		{
        }

		public void tcpCanSend(ulong id)
		{
        }

		public void udpCreated(ulong id, NF_UDP_CONN_INFO connInfo)
		{
        }

        public void udpConnectRequest(ulong id, ref NF_UDP_CONN_REQUEST connReq)
        {
        }

        public void udpClosed(ulong id, NF_UDP_CONN_INFO connInfo)
		{
        }

        public void udpReceive(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen)
		{
        }

        public void udpSend(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen)
		{
        }

		public void udpCanReceive(ulong id)
		{
        }

		public void udpCanSend(ulong id)
		{
        }
    }

    class Program
    {
        static EventHandler m_eh = new EventHandler();

        static void usage()
        {
            Console.Out.WriteLine("Usage: TcpRedirector.exe [-t] [-p Port] [-pid ProcessId] -r IP:Port");
            Console.Out.WriteLine("Port : remote port to intercept");
            Console.Out.WriteLine("ProcessId : redirect connections of the process with given PID");
            Console.Out.WriteLine("ProxyPid : it is necessary to specify proxy process id if the connection is redirected to local proxy");
            Console.Out.WriteLine("IP:Port : redirect connections to the specified IP endpoint");
            Console.Out.WriteLine("-t : turn on tunneling via HTTPS proxy at IP:Port. ");
            Console.Out.WriteLine("-s : turn on tunneling via SOCKS4 proxy at IP:Port. ");
        }

        unsafe static void Main(string[] args)
        {
            NF_RULE rule = new NF_RULE();
            // Filter TCP connections 
            rule.protocol = (int)ProtocolType.Tcp;
            // Use tcpConnectRequest
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_INDICATE_CONNECT_REQUESTS;

            for (int i = 0; i < args.Length; i += 2)
            {
                if (args[i] == "-p")
                {
                    rule.remotePort = (ushort)IPAddress.HostToNetworkOrder((Int16)Convert.ToUInt16(args[i+1]));
                    Console.Out.WriteLine("Remote port: " + args[i + 1]);
                } else
                if (args[i] == "-pid")
                {
                    rule.processId = (UInt32)Convert.ToUInt32(args[i + 1]);
                    Console.Out.WriteLine("Process Id: " + args[i + 1]);
                } else
                if (args[i] == "-pid")
                {
                    rule.processId = (UInt32)Convert.ToUInt32(args[i + 1]);
                    Console.Out.WriteLine("Process Id: " + args[i + 1]);
                } else
                if (args[i] == "-proxypid")
                {
                    m_eh.proxyPid = (UInt32)Convert.ToUInt32(args[i + 1]);
                    Console.Out.WriteLine("Proxy process Id: " + args[i + 1]);
                } else
                if (args[i] == "-t")
                {
                    m_eh.m_proxyType = EventHandler.PROXY_TYPE.PT_HTTPS;
                    i--;
                    Console.Out.WriteLine("Tunnel via HTTP proxy");
                } else
                if (args[i] == "-s")
                {
                    m_eh.m_proxyType = EventHandler.PROXY_TYPE.PT_SOCKS;
                    i--;
                    Console.Out.WriteLine("Tunnel via SOCKS proxy");
                } else
                if (args[i] == "-r")
                {
                    m_eh.redirectTo = NFUtil.stringToAddress(args[i + 1]).Serialize();
                    if (m_eh.redirectTo != null)
                    {
                        Console.Out.WriteLine("Redirecting to: " + args[i + 1]);
                    }
                } else
                {
                    usage();
                    return;
                }
            }

            if (m_eh.redirectTo == null)
            {
                usage();
                return;
            }

            if (NFAPI.nf_init("netfilter2", m_eh) != 0)
                return;
           
            NFAPI.nf_addRule(rule, 1);

            Console.In.ReadLine();

            NFAPI.nf_free();
        }
    }
}
