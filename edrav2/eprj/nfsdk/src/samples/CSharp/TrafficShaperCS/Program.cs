/**
*	Limits the bandwidth for the specified process
*	Usage: TrafficShaper.exe <process name> <limit>
*	<process name> - short process name, e.g. firefox.exe
*	<limit> - network IO limit in bytes per second for all instances of the specified process
*
*	Example: TrafficShaper.exe firefox.exe 10000
*	Limits TCP/UDP traffic to 10000 bytes per second for all instances of Firefox
**/

using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Collections;
using System.Threading;
using nfapinet;

namespace TrafficShaperCS
{
    // API events handler
    unsafe public class EventHandler : NF_EventHandler
    {
        public string   m_processName;
        public uint     m_ioLimit;

        class NET_IO_COUNTERS
        {
            public NET_IO_COUNTERS()
            {
                ioTime = 0;
                bytesIn = 0;
                bytesOut = 0;
            }

            public ulong ioTime;
            public UInt64 bytesIn;
            public UInt64 bytesOut;
        }

        private NET_IO_COUNTERS m_io = new NET_IO_COUNTERS();

        Hashtable m_tcpSet = new Hashtable();
        Hashtable m_udpSet = new Hashtable();

        private bool checkProcessName(string processName)
        {
	        if (processName.Length < m_processName.Length)
		        return false;

            return processName.EndsWith(m_processName, true, System.Globalization.CultureInfo.CurrentCulture);
        }

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
        }

        public unsafe void tcpConnected(ulong id, NF_TCP_CONN_INFO connInfo)
		{
            string processName = nfapinet.NFAPI.nf_getProcessName(connInfo.processId);

            string s = "TCP id=" + id + " ";
            s += ((NF_DIRECTION)connInfo.direction == NF_DIRECTION.NF_D_IN) ? "[in]" : "[out]";
            s += " process: " + processName + "\n";

            Console.Out.WriteLine(s);

		    if ((connInfo.filteringFlag & (int)NF_FILTERING_FLAG.NF_FILTER) > 0)
		    {
		        if (!checkProcessName(processName))
		        {
			        // Do not filter other processes
			        NFAPI.nf_tcpDisableFiltering(id);
			        return;
		        }

                Hashtable.Synchronized(m_tcpSet).Add(id, null);
		    }

        }

		public void tcpClosed(ulong id, NF_TCP_CONN_INFO connInfo)
		{
            string s = "TCP id=" + id + " ";

            s += ((NF_DIRECTION)connInfo.direction == NF_DIRECTION.NF_D_IN) ? "[in]" : "[out]";

            s += " closed ";

            Console.Out.WriteLine(s);

            Hashtable.Synchronized(m_tcpSet).Remove(id);
        }

        public void tcpReceive(ulong id, IntPtr buf, int len)
		{
            NFAPI.nf_tcpPostReceive(id, buf, len);

            if (Hashtable.Synchronized(m_tcpSet).ContainsKey(id))
            {
                m_io.bytesIn += (ulong)len;

                if (m_io.bytesIn > m_ioLimit)
                {
                    NFAPI.nf_tcpSetConnectionState(id, 1);
                }
            }
		}

        public void tcpSend(ulong id, IntPtr buf, int len)
		{
            NFAPI.nf_tcpPostSend(id, buf, len);

            if (Hashtable.Synchronized(m_tcpSet).ContainsKey(id))
            {
                m_io.bytesOut += (ulong)len;

                if (m_io.bytesOut > m_ioLimit)
                {
                    NFAPI.nf_tcpSetConnectionState(id, 1);
                }
            }
        }

		public void tcpCanReceive(ulong id)
		{
        }

		public void tcpCanSend(ulong id)
		{
        }

		public void udpCreated(ulong id, NF_UDP_CONN_INFO connInfo)
		{
            string processName = nfapinet.NFAPI.nf_getProcessName(connInfo.processId);

            if (!checkProcessName(processName))
            {
                // Do not filter other processes
                NFAPI.nf_udpDisableFiltering(id);
                return;
            }

            Hashtable.Synchronized(m_udpSet).Add(id, null);
        }

        public void udpConnectRequest(ulong id, ref NF_UDP_CONN_REQUEST connReq)
        {
        }

        public void udpClosed(ulong id, NF_UDP_CONN_INFO connInfo)
		{
            Hashtable.Synchronized(m_udpSet).Remove(id);
        }

        public void udpReceive(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen)
		{
            NFAPI.nf_udpPostReceive(id, remoteAddress, buf, len, options);

            if (Hashtable.Synchronized(m_udpSet).ContainsKey(id))
            {
                m_io.bytesIn += (ulong)len;

                if (m_io.bytesIn > m_ioLimit)
                {
                    NFAPI.nf_udpSetConnectionState(id, 1);
                }
            }
        }

        public void udpSend(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen)
		{
            NFAPI.nf_udpPostSend(id, remoteAddress, buf, len, options);

            if (Hashtable.Synchronized(m_udpSet).ContainsKey(id))
            {
                m_io.bytesOut += (ulong)len;

                if (m_io.bytesOut > m_ioLimit)
                {
                    NFAPI.nf_udpSetConnectionState(id, 1);
                }
            }
        }

		public void udpCanReceive(ulong id)
		{
        }

		public void udpCanSend(ulong id)
		{
        }

        public void shape()
        {
            for (;;)
            {
                System.Threading.Thread.Sleep(100);

                if (Console.KeyAvailable)
                    return;

                // Check the time
                if (((ulong)Environment.TickCount - m_io.ioTime) < 1000)
                    continue;

                m_io.ioTime = (ulong)Environment.TickCount;

                // Update IO counters
                m_io.bytesIn = (m_io.bytesIn > m_ioLimit) ? m_io.bytesIn - m_ioLimit : 0;
                m_io.bytesOut = (m_io.bytesOut > m_ioLimit) ? m_io.bytesOut - m_ioLimit : 0;

                // Suspend or resume TCP/UDP sockets belonging to specified application
                int suspend = (m_io.bytesIn > m_ioLimit || m_io.bytesOut > m_ioLimit) ? 1 : 0;

                foreach (ulong id in Hashtable.Synchronized(m_tcpSet).Keys)
                {
                    NFAPI.nf_tcpSetConnectionState(id, suspend);
                }

                foreach (ulong id in Hashtable.Synchronized(m_udpSet).Keys)
                {
                    NFAPI.nf_udpSetConnectionState(id, suspend);
                }
            }
        }
    }

    class Program
    {
        static EventHandler m_eh = new EventHandler();


        unsafe static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.Out.WriteLine("Usage: TrafficShaperCS.exe <process name> <limit>\n");
                Console.Out.WriteLine("\t<process name> - short process name, e.g. firefox.exe\n");
                Console.Out.WriteLine("\t<limit> - network IO limit in bytes per second for all instances of the specified process");
                return;
            }

            m_eh.m_processName = args[0];
            Console.Out.WriteLine("Process name: " + args[0]);

            m_eh.m_ioLimit = (UInt32)Convert.ToUInt32(args[1]);
            Console.Out.WriteLine("IO limit (bytes): " + args[1]);

            if (NFAPI.nf_init("netfilter2", m_eh) != 0)
            {
        		Console.Out.WriteLine("Failed to connect to driver");
                return;
            }

            NF_RULE rule = new NF_RULE();

            // Do not filter local traffic
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_ALLOW;
            rule.ip_family = (ushort)AddressFamily.InterNetwork;
            rule.remoteIpAddress = IPAddress.Parse("127.0.0.1").GetAddressBytes();
            rule.remoteIpAddressMask = IPAddress.Parse("255.0.0.0").GetAddressBytes();
            NFAPI.nf_addRule(rule, 0);

        	// Filter all other TCP/UDP traffic
            rule = new NF_RULE();
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_FILTER;
            NFAPI.nf_addRule(rule, 0);

            Console.Out.WriteLine("Press any key to stop...");

            m_eh.shape();

            NFAPI.nf_free();
        }
    }
}
