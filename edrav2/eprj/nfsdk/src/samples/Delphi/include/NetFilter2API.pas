unit NetFilter2API;

interface

const
  nfapi_module = 'nfapi.dll';

  { NF_STATUS }
  NF_STATUS_SUCCESS		= 0;
  NF_STATUS_FAIL			= -1;
  NF_STATUS_INVALID_ENDPOINT_ID	= -2;
  NF_STATUS_NOT_INITIALIZED	= -3;
  NF_STATUS_IO_ERROR		= -4;

  { NF_DIRECTION }
  NF_D_IN = 1;		// Incoming TCP connection or UDP packet
  NF_D_OUT = 2;		// Outgoing TCP connection or UDP packet
  NF_D_BOTH = 3;	// Any direction

  { NF_FILTERING_FLAG }
  NF_ALLOW = 0;		// Allow the activity without filtering transmitted packets
  NF_BLOCK = 1;		// Block the activity
  NF_FILTER = 2;	// Filter the transmitted packets
  NF_SUSPENDED = 4;	// Suspend receives from server and sends from client
  NF_OFFLINE = 8;	// Emulate establishing a TCP connection with remote server
  NF_INDICATE_CONNECT_REQUESTS = 16; // Indicate outgoing connect requests to API
  NF_DISABLE_REDIRECT_PROTECTION = 32; // Disable blocking indicating connect requests for outgoing connections of local proxies
  NF_PEND_CONNECT_REQUEST = 64; // Pend outgoing connect request to complete it later using nf_complete(TCP|UDP)ConnectRequest
  NF_FILTER_AS_IP_PACKETS = 128; // Indicate the traffic as IP packets via ipSend/ipReceive
  NF_READONLY = 256;	// Don't block the IP packets and indicate them to ipSend/ipReceive only for monitoring
  NF_CONTROL_FLOW = 512; // Use the flow limit rules even without NF_FILTER flag

  { NF_FLAGS }
  NFF_NONE = 0;
  NFF_DONT_DISABLE_TEREDO = 1;
  NFF_DONT_DISABLE_TCP_OFFLOADING = 2;
	NFF_DISABLE_AUTO_REGISTER	= 4;
	NFF_DISABLE_AUTO_START	= 8;

  NF_MAX_ADDRESS_LENGTH	        = 28;
  NF_MAX_IP_ADDRESS_LENGTH	= 16;

  AF_INET = 2;          // internetwork: UDP, TCP, etc.
  AF_INET6 = 23;        // Internetwork Version 6

  IPPROTO_TCP = 6;      // TCP
  IPPROTO_UDP = 17;     // UDP
  IPPROTO_ICMP = 1;     // ICMP

  { NF_IP_FLAG }
  NFIF_NONE = 0;	// No flags
  NFIF_READONLY = 1;	// The packet was not blocked and indicated only for monitoring in read-only mode
  			// (see NF_READ_ONLY flags from NF_FILTERING_FLAG).

  // Flags for NF_UDP_OPTIONS.flags
  TDI_RECEIVE_BROADCAST  =         $00000004; // received TSDU was broadcast.
  TDI_RECEIVE_MULTICAST =          $00000008; // received TSDU was multicast.
  TDI_RECEIVE_PARTIAL =            $00000010; // received TSDU is not fully presented.
  TDI_RECEIVE_NORMAL =             $00000020; // received TSDU is normal data
  TDI_RECEIVE_EXPEDITED =          $00000040; // received TSDU is expedited data
  TDI_RECEIVE_PEEK =               $00000080; // received TSDU is not released
  TDI_RECEIVE_NO_RESPONSE_EXP =    $00000100; // HINT: no back-traffic expected
  TDI_RECEIVE_COPY_LOOKAHEAD =     $00000200; // for kernel-mode indications
  TDI_RECEIVE_ENTIRE_MESSAGE =     $00000400; // opposite of RECEIVE_PARTIAL
                                                   //  (for kernel-mode indications)
  TDI_RECEIVE_AT_DISPATCH_LEVEL =  $00000800; // receive indication called
                                                   //  at dispatch level
  TDI_RECEIVE_CONTROL_INFO =       $00001000; // Control info is being passed up.
  TDI_RECEIVE_FORCE_INDICATION =   $00002000; // reindicate rejected data.
  TDI_RECEIVE_NO_PUSH =            $00004000; // complete only when full.

  // Driver types
  DT_UNKNOWN = 0;
  DT_TDI = 1;
  DT_WFP = 2;


type

  ENDPOINT_ID = Int64;

  NF_RULE = packed record
        protocol : integer;	// IPPROTO_TCP or IPPROTO_UDP
        processId : Longword;	// Process identifier
	direction : byte;	// See NF_DIRECTION
	localPort : word;	// Local port
	remotePort : word;	// Remote port
	ip_family : word;	// AF_INET for IPv4 and AF_INET6 for IPv6

	// Local IP (or network if localIpAddressMask is not zero)
	localIpAddress : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	// Local IP mask
	localIpAddressMask : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	// Remote IP (or network if remoteIpAddressMask is not zero)
	remoteIpAddress : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	// Remote IP mask
	remoteIpAddressMask : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	filteringFlag : Longword;	// See NF_FILTERING_FLAG
  end;

  NF_RULE_EX = packed record
        protocol : integer;	// IPPROTO_TCP or IPPROTO_UDP
        processId : Longword;	// Process identifier
	direction : byte;	// See NF_DIRECTION
	localPort : word;	// Local port
	remotePort : word;	// Remote port
	ip_family : word;	// AF_INET for IPv4 and AF_INET6 for IPv6

	// Local IP (or network if localIpAddressMask is not zero)
	localIpAddress : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	// Local IP mask
	localIpAddressMask : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	// Remote IP (or network if remoteIpAddressMask is not zero)
	remoteIpAddress : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	// Remote IP mask
	remoteIpAddressMask : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	filteringFlag : Longword;	// See NF_FILTERING_FLAG

	  // Tail part of the process path
  	processName : array [0..260] of WideChar;
  end;

  NF_TCP_CONN_INFO = packed record
	filteringFlag : Longword;	// See NF_FILTERING_FLAG
  processId : Longword;	// Process identifier
	direction : byte;	// See NF_DIRECTION
	ip_family : word;	// AF_INET for IPv4 and AF_INET6 for IPv6

	// Local address as sockaddr_in for IPv4 and sockaddr_in6 for IPv6
	localAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;

	// Remote address as sockaddr_in for IPv4 and sockaddr_in6 for IPv6
	remoteAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
  end;

  NF_UDP_CONN_INFO = packed record
  processId : Longword;	// Process identifier
	ip_family : word;	// AF_INET for IPv4 and AF_INET6 for IPv6

	// Local address as sockaddr_in for IPv4 and sockaddr_in6 for IPv6
	localAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
  end;

  NF_UDP_OPTIONS = packed record
  flags : Longword;	// UDP flags
	optionsLength : Longword; // Length of options
  options : pointer;
  end;

  NF_UDP_CONN_REQUEST = packed record
	filteringFlag : Longword;	// See NF_FILTERING_FLAG
  processId : Longword;	// Process identifier
	ip_family : word;	// AF_INET for IPv4 and AF_INET6 for IPv6

	// Local address as sockaddr_in for IPv4 and sockaddr_in6 for IPv6
	localAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;

	// Remote address as sockaddr_in for IPv4 and sockaddr_in6 for IPv6
	remoteAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
  end;

  NF_IP_PACKET_OPTIONS = packed record
  ip_family : word;             // AF_INET for IPv4 and AF_INET6 for IPv6
	ipHeaderSize : Longword;      // Size in bytes of IP header
	compartmentId : Longword;     // Network routing compartment identifier (can be zero)
	interfaceIndex : Longword;    // Index of the interface on which the original packet data was received (irrelevant to outgoing packets)
	subInterfaceIndex : Longword; // Index of the subinterface on which the original packet data was received (irrelevant to outgoing packets)
  flags : Longword; // Can be a combination of flags from NF_IP_FLAG
  end;

  NF_FLOWCTL_DATA = packed record
    inLimit : Int64;
    outLimit : Int64;
  end;

  NF_FLOWCTL_MODIFY_DATA = packed record
    fcHandle : Longword;
    data : NF_FLOWCTL_DATA;
  end;

  NF_FLOWCTL_STAT = packed record
    inBytes : Int64;
    outBytes : Int64;
  end;

  NF_FLOWCTL_SET_DATA = packed record
    endpointId : ENDPOINT_ID;
    fcHandle : Longword;
  end;

  NF_BINDING_RULE = packed record
    protocol : Longword;	// IPPROTO_TCP or IPPROTO_UDP
	  processId : Longword;	// Process identifier

	  // Tail part of the process path
  	processName : array [0..260] of WideChar;

    // Local port
	  localPort : word;

    // AF_INET for IPv4 and AF_INET6 for IPv6
	  ip_family : word;

	  // Local IP (or network if localIpAddressMask is not zero)
  	localIpAddress : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;
	
	  // Local IP mask
	  localIpAddressMask : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte;

	  // Redirect bind request to this IP
  	newLocalIpAddress : array [0..NF_MAX_IP_ADDRESS_LENGTH-1] of byte; 

	  // Redirect bind request to this port, if it is not zero
  	newLocalPort : word;

    // See NF_FILTERING_FLAG, NF_ALLOW to bypass or NF_FILTER to redirect
  	filteringFlag : Longword;
  end;

  NF_EventHandler = packed record
	threadStart : procedure(); cdecl;
	threadEnd : procedure(); cdecl;

	tcpConnectRequest : procedure(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
	tcpConnected : procedure(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
	tcpClosed : procedure(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
	tcpReceive : procedure(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
	tcpSend	: procedure(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
	tcpCanReceive : procedure(id : ENDPOINT_ID); cdecl;
	tcpCanSend : procedure(id : ENDPOINT_ID); cdecl;

	udpCreated : procedure(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
	udpConnectRequest : procedure(id : ENDPOINT_ID; var pConnReq : NF_UDP_CONN_REQUEST); cdecl;
	udpClosed : procedure(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
	udpReceive : procedure(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
	udpSend	: procedure(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
	udpCanReceive : procedure(id : ENDPOINT_ID); cdecl;
	udpCanSend : procedure(id : ENDPOINT_ID); cdecl;
  end;

  NF_IPEventHandler = packed record
        ipReceive : procedure(buf : PAnsiChar; len : integer; var options : NF_IP_PACKET_OPTIONS); cdecl;
        ipSend : procedure(buf : PAnsiChar; len : integer; var options : NF_IP_PACKET_OPTIONS); cdecl;
  end;

  function nf_init(driverName : PAnsiChar; var pHandler : NF_EventHandler) : integer; cdecl; external nfapi_module;
  procedure nf_free(); cdecl; external nfapi_module;

  function nf_registerDriver(driverName : PAnsiChar): integer; cdecl; external nfapi_module;
  function nf_registerDriverEx(driverName : PAnsiChar; driverPath : PAnsiChar): integer; cdecl; external nfapi_module;
  function nf_unRegisterDriver(driverName : PAnsiChar): integer; cdecl; external nfapi_module;

  function nf_tcpSetConnectionState(id : ENDPOINT_ID; suspended : integer): integer; cdecl; external nfapi_module;
  function nf_tcpPostSend(id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl; external nfapi_module;
  function nf_tcpPostReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl; external nfapi_module;
  function nf_tcpClose(id : ENDPOINT_ID): integer; cdecl; external nfapi_module;

  function nf_udpSetConnectionState(id : ENDPOINT_ID; suspended : integer): integer; cdecl; external nfapi_module;
  function nf_udpPostSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl; external nfapi_module;
  function nf_udpPostReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl; external nfapi_module;

  function nf_ipPostSend(buf : PAnsiChar; len : Longword; var options : NF_IP_PACKET_OPTIONS): integer; cdecl; external nfapi_module;
  function nf_ipPostReceive(buf : PAnsiChar; len : Longword; var options : NF_IP_PACKET_OPTIONS): integer; cdecl; external nfapi_module;

  function nf_addRule(var rule : NF_RULE; toHead : integer): integer; cdecl; external nfapi_module;
  function nf_deleteRules(): integer; cdecl; external nfapi_module;
  function nf_setRules(rules : pointer; count : integer): integer; cdecl; external nfapi_module;

  function nf_addRuleEx(var rule : NF_RULE_EX; toHead : integer): integer; cdecl; external nfapi_module;
  function nf_setRulesEx(rules : pointer; count : integer): integer; cdecl; external nfapi_module;

  function nf_setTCPTimeout(timeout : Longword): integer; cdecl; external nfapi_module;

  function nf_tcpDisableFiltering(id : ENDPOINT_ID): integer; cdecl; external nfapi_module;
  function nf_udpDisableFiltering(id : ENDPOINT_ID): integer; cdecl; external nfapi_module;

  function nf_tcpSetSockOpt(id : ENDPOINT_ID; optname : integer; optval : pointer; optlen : integer): integer; cdecl; external nfapi_module;

  procedure nf_setIPEventHandler(var pHandler : NF_IPEventHandler); cdecl; external nfapi_module;

  // Helper routines

  // Allow access from current process to the processes of other users
  procedure nf_adjustProcessPriviledges();

  // Returns the full name of a process with given processId (Ansi version)
  function nf_getProcessNameA(processId : Longword; buf : PAnsiChar; len : integer) : boolean;

  // Returns the full name of a process with given processId (Unicode version)
  function nf_getProcessNameW(processId : Longword; buf : PWideChar; len : integer) : boolean;

  // Returns the full name of a process with given processId (Unicode version).
  // Doesn't require administrative privileges.
  function nf_getProcessNameFromKernel(processId : Longword; buf : PWideChar; len : integer) : boolean; cdecl; external nfapi_module;

  // Returns TRUE if the specified process acts as a local proxy, accepting the redirected TCP connections.
  function nf_tcpIsProxy(processId : Longword) : integer; cdecl; external nfapi_module;

  {
  * Set the number of worker threads and initialization flags.
  * The function should be called before nf_init.
  * By default nThreads = 1 and flags = 0
  * @param nThreads Number of worker threads for NF_EventHandler events
  * @param flags A combination of flags from <tt>NF_FLAGS</tt>
  }
  procedure nf_setOptions(nThreads : Longword; flags : Longword); cdecl; external nfapi_module;

  { Complete TCP connect request pended using flag NF_PEND_CONNECT_REQUEST. }
  function nf_completeTCPConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO): integer; cdecl; external nfapi_module;

  { Complete UDP connect request pended using flag NF_PEND_CONNECT_REQUEST. }
  function nf_completeUDPConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_REQUEST): integer; cdecl; external nfapi_module;

  { Returns in pConnInfo the properties of TCP connection with specified id. }
  function nf_getTCPConnInfo(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO): integer; cdecl; external nfapi_module;

  { Returns in pConnInfo the properties of UDP socket with specified id. }
  function nf_getUDPConnInfo(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO): integer; cdecl; external nfapi_module;


  { Add flow control context }
  function nf_addFlowCtl(var pData : NF_FLOWCTL_DATA; var pFcHandle : Longword): integer; cdecl; external nfapi_module;

  { Delete flow control context }
  function nf_deleteFlowCtl(fcHandle : Longword): integer; cdecl; external nfapi_module;

  { Associate flow control context with TCP connection }
  function nf_setTCPFlowCtl(id : ENDPOINT_ID; fcHandle : Longword): integer; cdecl; external nfapi_module;

  { Associate flow control context with UDP socket }
  function nf_setUDPFlowCtl(id : ENDPOINT_ID; fcHandle : Longword): integer; cdecl; external nfapi_module;

  { Modify flow control context limits }
  function nf_modifyFlowCtl(fcHandle : Longword; var pData : NF_FLOWCTL_DATA): integer; cdecl; external nfapi_module;

  { Get flow control context statistics as the numbers of in/out bytes }
  function nf_getFlowCtlStat(fcHandle : Longword; var pStat : NF_FLOWCTL_STAT): integer; cdecl; external nfapi_module;

  { 
	Get TCP connection statistics as the numbers of in/out bytes.
	The function can be called only from tcpClosed handler!
  }
  function nf_getTCPStat(id : ENDPOINT_ID; var pStat : NF_FLOWCTL_STAT): integer; cdecl; external nfapi_module;

  { 
	Get UDP socket statistics as the numbers of in/out bytes.
	The function can be called only from udpClosed handler!
  }
  function nf_getUDPStat(id : ENDPOINT_ID; var pStat : NF_FLOWCTL_STAT): integer; cdecl; external nfapi_module;

  {
  Add binding rule to driver
  }
  function nf_addBindingRule(var pRule : NF_BINDING_RULE; toHead : integer) : integer; cdecl; external nfapi_module;

  {
  Delete all binding rules from driver
  }
  function nf_deleteBindingRules : integer; cdecl; external nfapi_module;

  {
  Returns the type of attached driver (DT_WFP, DT_TDI or DT_UNKNOWN)
  }
  function nf_getDriverType : integer; cdecl; external nfapi_module;


implementation

uses Windows;

function GetModuleFileNameExA(
    hProcess : Cardinal;
    hModule : HMODULE;
    lpFilename : PAnsiChar;
    nSize : Longword) : Longword;
    stdcall; external 'psapi.dll';

function GetModuleFileNameExW(
    hProcess : Cardinal;
    hModule : HMODULE;
    lpFilename : PWideChar;
    nSize : Longword) : Longword;
    stdcall; external 'psapi.dll';

function GetLongPathNameA(
    lpFilename : PAnsiChar;
    lpLongFilename : PAnsiChar;
    nSize : Longword) : Longword;
    stdcall; external 'kernel32.dll';

function GetLongPathNameW(
    lpFilename : PWideChar;
    lpLongFilename : PWideChar;
    nSize : Longword) : Longword;
    stdcall; external 'kernel32.dll';

procedure nf_adjustProcessPriviledges();
var hProcess : THandle;
    hToken : THandle;
    n : DWORD;
    tp : TOKEN_PRIVILEGES;
    dummy: PTokenPrivileges;
    luid : TLargeInteger;
begin
hProcess := OpenProcess(PROCESS_ALL_ACCESS, false, GetCurrentProcessId());
if (hProcess <> 0) then
  begin

  if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, hToken)) then
    begin
    if (not LookupPrivilegeValue(nil, 'SeDebugPrivilege', luid)) then
      begin
      CloseHandle(hToken);
      CloseHandle(hProcess);
      exit;
      end;

    tp.PrivilegeCount := 1;
    tp.Privileges[0].Luid := luid;
    tp.Privileges[0].Attributes := SE_PRIVILEGE_ENABLED;

    dummy := nil;
    AdjustTokenPrivileges(hToken, false, tp, sizeof(TOKEN_PRIVILEGES), dummy^, n);
    CloseHandle(hToken);
    end;
  CloseHandle(hProcess);
  end;
end;

function nf_getProcessNameA(processId : Longword; buf : PAnsiChar; len : integer) : boolean;
var hProcess: Cardinal;
    longPath : array [0..MAX_PATH] of AnsiChar;
    i, n : integer;
begin
Result := false;
hProcess := OpenProcess(PROCESS_ALL_ACCESS, false, processId);
if (hProcess <> 0) then
  begin
  if (GetModuleFileNameExA(hProcess, 0, buf, len) <> 0) then
    Result := true;

  n := GetLongPathNameA(buf, longPath, sizeof(longPath));
  if ((n > 0) and (n < len)) then
    begin
    for i:=0 to n do
     buf[i] := longPath[i];
    end;

  CloseHandle(hProcess);
  end;
end;

function nf_getProcessNameW(processId : Longword; buf : PWideChar; len : integer) : boolean;
var hProcess: Cardinal;
    longPath : array [0..MAX_PATH] of WideChar;
    i, n : integer;
begin
Result := false;
hProcess := OpenProcess(PROCESS_ALL_ACCESS, false, processId);
if (hProcess <> 0) then
  begin
  if (GetModuleFileNameExW(hProcess, 0, buf, len) <> 0) then
    Result := true;

  n := GetLongPathNameW(buf, @longPath, sizeof(longPath) div sizeof(longPath[0]));
  if ((n > 0) and (n < len)) then
    begin
    for i:=0 to n do
     buf[i] := longPath[i];
    end;
  CloseHandle(hProcess);
  end;
end;

end.
