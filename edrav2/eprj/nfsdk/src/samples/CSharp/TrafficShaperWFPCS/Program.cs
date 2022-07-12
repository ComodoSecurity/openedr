/**
*	Limits the bandwidth for the specified process
*	Usage: TrafficShaperWFPCS.exe <process name> <limit>
*	<process name> - short process name, e.g. firefox.exe
*	<limit> - network IO limit in bytes per second for all instances of the specified process
*
*	Example: TrafficShaperWFPCS.exe firefox.exe 10000
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
        public UInt32   m_flowControlHandle = 0;

        private bool checkProcessName(string processName)
        {
	        if (processName.Length < m_processName.Length)
		        return false;

            return processName.EndsWith(m_processName, true, System.Globalization.CultureInfo.CurrentCulture);
        }

        public void threadStart()
        {
        }
		
        public void threadEnd()
		{
        }

        public void tcpConnectRequest(ulong id, ref NF_TCP_CONN_INFO connInfo)
        {
        }

        public unsafe void tcpConnected(ulong id, NF_TCP_CONN_INFO connInfo)
		{
            string processName = nfapinet.NFAPI.nf_getProcessName(connInfo.processId);

	        if (checkProcessName(processName))
	        {
                NFAPI.nf_setTCPFlowCtl(id, m_flowControlHandle);
	        }
        }

		public void tcpClosed(ulong id, NF_TCP_CONN_INFO connInfo)
		{
        }

        public void tcpReceive(ulong id, IntPtr buf, int len)
		{
            NFAPI.nf_tcpPostReceive(id, buf, len);
		}

        public void tcpSend(ulong id, IntPtr buf, int len)
		{
            NFAPI.nf_tcpPostSend(id, buf, len);
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

            if (checkProcessName(processName))
            {
                NFAPI.nf_setUDPFlowCtl(id, m_flowControlHandle);
            }
        }

        public void udpConnectRequest(ulong id, ref NF_UDP_CONN_REQUEST connReq)
        {
        }

        public void udpClosed(ulong id, NF_UDP_CONN_INFO connInfo)
		{
        }

        public void udpReceive(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen)
		{
            NFAPI.nf_udpPostReceive(id, remoteAddress, buf, len, options);
        }

        public void udpSend(ulong id, IntPtr remoteAddress, IntPtr buf, int len, IntPtr options, int optionsLen)
		{
            NFAPI.nf_udpPostSend(id, remoteAddress, buf, len, options);
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

        public static void logStatistics()
        {
	        NF_FLOWCTL_STAT stat = new NF_FLOWCTL_STAT(), oldStat = new NF_FLOWCTL_STAT();
	        UInt32 ts = 0, tsSpeed = 0;
	        const int scd = 4;
	        ulong  inSpeed = 0, outSpeed = 0;

            for (;;)
            {
                System.Threading.Thread.Sleep(100);

                if (Console.KeyAvailable)
                    return;

                // Check the time
                if (((UInt32)Environment.TickCount - ts) < 1000)
                    continue;

                ts = (UInt32)Environment.TickCount;

                if (NFAPI.nf_getFlowCtlStat(m_eh.m_flowControlHandle, ref stat) == NF_STATUS.NF_STATUS_SUCCESS)
                {
                    if ((ts - tsSpeed) >= scd * 1000)
                    {
                        inSpeed = (stat.inBytes - oldStat.inBytes) / scd;
                        outSpeed = (stat.outBytes - oldStat.outBytes) / scd;
                        tsSpeed = ts;
                        oldStat = stat;
                    }

                    Console.Out.Write("in " + stat.inBytes + " [" + inSpeed + "], out " + stat.outBytes + " [" + outSpeed + "] \t\t\t\t\r");
                }

            }
        }

        unsafe static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.Out.WriteLine("Usage: TrafficShaperWFPCS.exe <process name> <limit>\n");
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

	        NF_FLOWCTL_DATA data = new NF_FLOWCTL_DATA();
	        data.inLimit = m_eh.m_ioLimit;
	        data.outLimit = m_eh.m_ioLimit;

	        if (NFAPI.nf_addFlowCtl(ref data, ref m_eh.m_flowControlHandle) != NF_STATUS.NF_STATUS_SUCCESS)
	        {
		        Console.Out.WriteLine("Unable to add flow control context");
		        return;
	        }

            NF_RULE rule = new NF_RULE();

            // Do not filter local traffic
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_ALLOW;
            rule.ip_family = (ushort)AddressFamily.InterNetwork;
            rule.remoteIpAddress = IPAddress.Parse("127.0.0.1").GetAddressBytes();
            rule.remoteIpAddressMask = IPAddress.Parse("255.0.0.0").GetAddressBytes();
            NFAPI.nf_addRule(rule, 0);

        	// Control flow for all other TCP/UDP traffic
            rule = new NF_RULE();
            rule.filteringFlag = (uint)NF_FILTERING_FLAG.NF_CONTROL_FLOW;
            NFAPI.nf_addRule(rule, 0);

            Console.Out.WriteLine("Press any key to stop...");

            logStatistics();

            NFAPI.nf_free();
        }
    }
}
