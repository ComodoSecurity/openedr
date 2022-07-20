========================================================================
    CONSOLE APPLICATION : TcpRedirector Project Overview
========================================================================

	This sample allows to redirect TCP connections with given parameters to the specified endpoint.
	It also supports redirecting to HTTPS or SOCKS proxy.

	Usage: TcpRedirector.exe [-s|t] [-p Port] [-pid ProcessId] -r IP:Port 
		-t : redirect via HTTPS proxy at IP:Port. The proxy must support HTTP tunneling (CONNECT requests)
		-s : redirect via SOCKS4 proxy at IP:Port. 
		Port : remote port to intercept
		ProcessId : redirect connections of the process with given PID
		ProxyPid : it is necessary to specify proxy process id if the connection is redirected to local proxy
		IP:Port : redirect connections to the specified IP endpoint

	Example:
		
		TcpRedirector.exe -t -p 110 -r 163.15.64.8:3128

		The sample will redirect POP3 connections to HTTPS proxy at 163.15.64.8:3128 with tunneling to real server. 
