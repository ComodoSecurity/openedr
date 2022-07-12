program PassThroughDelphi;

{$APPTYPE CONSOLE}

uses
  SysUtils, WinSock,
  NetFilter2API in '..\include\NetFilter2API.pas';

const MAX_PATH = 256;

var     eh : NF_EventHandler;
        err : integer;
        wd : WSAData;

// Import WSAAddressToStringA from WinSock2 API
function WSAAddressToStringA(
                lpsaAddress : Pointer;
                dwAddressLength : Longword;
                lpProtocolInfo : Pointer;
                lpszAddressString : PAnsiChar;
                var lpdwAddressStringLength : Longword) : integer;
                stdcall; external 'ws2_32.dll';

// Prints TCP connection parameters to stdout
procedure printConnInfo(connected : boolean; id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO);
var     localAddr : array [0..MAX_PATH] of char;
        remoteAddr : array [0..MAX_PATH] of char;
        pStr : PAnsiChar;
        addrStrLen : Longword;
        addrLen : Longword;
        s : string;
        processName : array [0..MAX_PATH] of char;
begin
if (pConnInfo.ip_family = AF_INET) then
  begin
  addrLen := sizeof(sockaddr_in);
  end else
  begin
  addrLen := NF_MAX_ADDRESS_LENGTH;
  end;

addrStrLen := sizeof(localAddr);
pStr := PAnsiChar(@localAddr);
pStr[0] := #0;
WSAAddressToStringA(@pConnInfo.localAddress, addrLen, nil, pStr, addrStrLen);

addrStrLen := sizeof(remoteAddr);
pStr := PAnsiChar(@remoteAddr);
pStr[0] := #0;
WSAAddressToStringA(@pConnInfo.remoteAddress, addrLen, nil, pStr, addrStrLen);

if (connected) then
  begin
  FmtStr(s, 'tcpConnected id=%d flag=%d pid=%d direction=%d local=%s remote=%s',
      [id, pConnInfo.filteringFlag, pConnInfo.processId, pConnInfo.direction,
      PAnsiChar(@localAddr), PAnsiChar(@remoteAddr)]);
  end else
  begin
  FmtStr(s, 'tcpClosed id=%d flag=%d pid=%d direction=%d local=%s remote=%s',
      [id, pConnInfo.filteringFlag, pConnInfo.processId, pConnInfo.direction,
      PAnsiChar(@localAddr), PAnsiChar(@remoteAddr)]);
  end;

writeln(s);

if (nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
  begin
  writeln('    Process: ' + processName);
  end;

end;

// Prints UDP socket parameters to stdout
procedure printAddrInfo(created : boolean; id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO);
var     localAddr : array [0..MAX_PATH] of char;
        pStr : PAnsiChar;
        addrStrLen : Longword;
        addrLen : Longword;
        s : string;
        processName : array [0..MAX_PATH] of char;
begin
if (pConnInfo.ip_family = AF_INET) then
  begin
  addrLen := sizeof(sockaddr_in);
  end else
  begin
  addrLen := NF_MAX_ADDRESS_LENGTH;
  end;

addrStrLen := sizeof(localAddr);
pStr := PAnsiChar(@localAddr);
pStr[0] := #0;
WSAAddressToStringA(@pConnInfo.localAddress, addrLen, nil, pStr, addrStrLen);

if (created) then
  begin
  FmtStr(s, 'udpCreated id=%d pid=%d local=%s',
      [id, pConnInfo.processId, PAnsiChar(@localAddr)]);
  end else
  begin
  FmtStr(s, 'udpClosed id=%d pid=%d local=%s',
      [id, pConnInfo.processId, PAnsiChar(@localAddr)]);
  end;

writeln(s);

if (nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
  begin
  writeln('    Process: ' + processName);
  end;

end;

procedure threadStart(); cdecl;
begin
writeln('treadStart');
end;

procedure threadEnd(); cdecl;
begin
writeln('treadEnd');
end;

procedure tcpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
writeln('tcpConnectRequest ' + IntToStr(id));
end;

procedure tcpConnected(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
printConnInfo(true, id, pConnInfo);
end;

procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
printConnInfo(false, id, pConnInfo);
end;

procedure tcpReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
begin
writeln('tcpReceive ' + IntToStr(id) + ' len=' + IntToStr(len));
nf_tcpPostReceive(id, buf, len);
end;

procedure tcpSend(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
begin
writeln('tcpSend ' + IntToStr(id) + ' len=' + IntToStr(len));
nf_tcpPostSend(id, buf, len);
end;

procedure tcpCanReceive(id : ENDPOINT_ID); cdecl;
begin
writeln('tcpCanReceive ' + IntToStr(id));
end;

procedure tcpCanSend(id : ENDPOINT_ID); cdecl;
begin
writeln('tcpCanSend ' + IntToStr(id));
end;

procedure udpCreated(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
begin
printAddrInfo(true, id, pConnInfo);
end;

procedure udpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_REQUEST); cdecl;
begin
writeln('udpConnectRequest ' + IntToStr(id));
end;

procedure udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
begin
printAddrInfo(false, id, pConnInfo);
end;

procedure udpReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
writeln('udpReceive ' + IntToStr(id) + ' len=' + IntToStr(len));
nf_udpPostReceive(id, remoteAddress, buf, len, options);
end;

procedure udpSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
writeln('udpSend ' + IntToStr(id) + ' len=' + IntToStr(len));
nf_udpPostSend(id, remoteAddress, buf, len, options);
end;

procedure udpCanReceive(id : ENDPOINT_ID); cdecl;
begin
writeln('udpCanReceive ' + IntToStr(id));
end;

procedure udpCanSend(id : ENDPOINT_ID); cdecl;
begin
writeln('udpCanSend ' + IntToStr(id));
end;

procedure start();
var rule : NF_RULE;
begin
FillChar(rule, sizeof(rule), 0);
rule.filteringFlag := NF_FILTER;// or NF_INDICATE_CONNECT_REQUESTS;
nf_addRule(rule, 1);
end;

begin
  // Initialize WinSock (required for WSAAddressToStringA)
  WSAStartup(2 * 256 + 2 , wd);

  // Allow access to the processes of other users
  nf_adjustProcessPriviledges;

  // Initialize event handler structure
  eh.threadStart := threadStart;
  eh.threadEnd := threadEnd;

  eh.tcpConnectRequest := tcpConnectRequest;
  eh.tcpConnected := tcpConnected;
  eh.tcpClosed := tcpClosed;
  eh.tcpReceive := tcpReceive;
  eh.tcpSend := tcpSend;
  eh.tcpCanReceive := tcpCanReceive;
  eh.tcpCanSend := tcpCanSend;

  eh.udpCreated := udpCreated;
  eh.udpConnectRequest := udpConnectRequest;
  eh.udpClosed := udpClosed;
  eh.udpReceive := udpReceive;
  eh.udpSend := udpSend;
  eh.udpCanReceive := udpCanReceive;
  eh.udpCanSend := udpCanSend;

  // Attach to driver
  err := nf_init(PAnsiChar('netfilter2'), eh);
  if (err = NF_STATUS_SUCCESS) then
  begin
    // Add rules
    start();
    writeln('Press Enter to stop');
    readln;
    // Detach from driver
    nf_free();
  end;

  WSACleanup();
end.
