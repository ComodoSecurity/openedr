using System;
using System.Text;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace SocksRedirectorCS
{
    class SRN_API
    {
        public enum eOPTION_TYPE
        {
            OT_NONE,
            OT_DRIVER_NAME,             // NFSDK driver name, by default - netfilter2
            OT_PROTOCOL,                // "tcp" or "udp". Default value is empty, which means any protocol
            OT_PROCESS_NAME,            // Add process name to list
            OT_REMOTE_ADDRESS,          // Add remote IP or network (e.g. 192.168.137.0/24) to list
            OT_REMOTE_PORT,             // Add remote port to list
        	OT_ACTION,                  // "redirect" - redirect to proxy, "bypass" - don't redirect. By default - redirect.
            OT_PROXY_ADDRESS,           // SOCKS5 proxy IP:port, e.g. 192.168.137.105:1080
            OT_PROXY_USER_NAME,         // SOCKS5 proxy user name
            OT_PROXY_PASSWORD           // SOCKS5 proxy user password
        };

        // Initialize the library
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern 
        bool srn_init();

        // Stop redirection and free the library
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern 
        void srn_free();

        // Delete all added options
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern 
        bool srn_clearOptions();

        // Start adding options for a rule
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern
        void srn_startRule();

        // Push added options to list of rules
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern
        void srn_endRule();

        // Add an option as a string with specified type
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern
        bool srn_addOption(eOPTION_TYPE optionType, string optionValue);

        // Start or stop redirection
        [DllImport("SocksRedirectorNet", CallingConvention = CallingConvention.Cdecl)]
        public static extern
        bool srn_enable(int start);
    }

    class Program
    {
        unsafe static void Main(string[] args)
        {
            if (!SRN_API.srn_init())
            {
                Console.Out.WriteLine("Init failed");
                return;
            }

            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_DRIVER_NAME, "netfilter2");

            SRN_API.srn_startRule();
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_REMOTE_ADDRESS, "217.112.35.75");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_REMOTE_PORT, "5000-6000");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_REMOTE_PORT, "80");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_REMOTE_PORT, "443");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROCESS_NAME, "chrome.exe");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROCESS_NAME, "firefox.exe");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_ACTION, "redirect");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROXY_ADDRESS, "192.168.137.105:1080");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROXY_USER_NAME, "test");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROXY_PASSWORD, "test");
            SRN_API.srn_endRule();

            SRN_API.srn_startRule();
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_ACTION, "redirect");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROXY_ADDRESS, "192.168.137.105:1080");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROXY_USER_NAME, "test");
            SRN_API.srn_addOption(SRN_API.eOPTION_TYPE.OT_PROXY_PASSWORD, "test");
            SRN_API.srn_endRule();

            if (!SRN_API.srn_enable(1))
            {
                Console.Out.WriteLine("Enable failed");
                return;
            }

            Console.Out.WriteLine("Redirecting, press Enter to stop...");
            
            Console.In.ReadLine();

            SRN_API.srn_enable(0);

            SRN_API.srn_free();
        }
    }
}
