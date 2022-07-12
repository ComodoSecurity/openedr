unit MainFormUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, WinSock;

type
  TMainForm = class(TForm)
    GroupBox1: TGroupBox;
    Start: TButton;
    Stop: TButton;
    Label1: TLabel;
    PortEdit: TEdit;
    ProxyAddressEdit: TEdit;
    Label2: TLabel;
    RedirectTCP: TCheckBox;
    GroupBox2: TGroupBox;
    Label4: TLabel;
    DNSServerAddressEdit: TEdit;
    RedirectUDP: TCheckBox;
    Label3: TLabel;
    GroupBox3: TGroupBox;
    Log: TMemo;
    ProxyTypeCombo: TComboBox;
    Label5: TLabel;
    ProxyPidEdit: TEdit;
    procedure StartClick(Sender: TObject);
    procedure StopClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    procedure addLogString(const s : string);
  end;

var
  MainForm: TMainForm;

implementation

uses NetFilter2API, DnsResolverUnit;

{$R *.dfm}

const
  MAX_PATH = 256;

  SOCKS_4 = 4;
  SOCKS_5 = 5;

  S4C_CONNECT = 1;
  S4C_BIND = 2;

  PT_HTTPS = 0;
  PT_HTTP = 1;
  PT_SOCKS = 2;

type

  ORIGINAL_CONN_INFO = class(TObject)
  public
    localAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
    remoteAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
    redirected : boolean;
    pendingSends : AnsiString;
    connectSent : boolean;
  end;

  SOCKS4_REQUEST = packed record
	version : byte;
	command : byte;
	port : word;
	ip : longword;
	userid : byte;
  end;

var     eh : NF_EventHandler;
        err : integer;
        filterTCP, filterUDP : boolean;
        proxyPid : Longword;

        // TCP vars
        tcpPort : Word;
        tcpProxyAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
        proxyType : integer;
        connInfoMap : TStringList;

        // UDP vars
        dnsServerAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
        addrInfoMap : TStringList;

        dnsResolver : TDnsResolver = nil;

//////////////////////////////////////////////////////////
// Support routines

// Import functions from WinSock2 API

function WSAStringToAddressA(
                AddressString : PAnsiChar;
                AddressFamily : Integer;
                lpProtocolInfo : Pointer;
                lpAddress : Pointer;
                var lpAddressLength : Longword) : integer;
                stdcall; external 'ws2_32.dll';

function WSAAddressToStringA(
                lpsaAddress : Pointer;
                dwAddressLength : Longword;
                lpProtocolInfo : Pointer;
                lpszAddressString : PAnsiChar;
                var lpdwAddressStringLength : Longword) : integer;
                stdcall; external 'ws2_32.dll';

procedure freeConnInfoMaps;
var i: integer;
begin
if (connInfoMap <> nil) then
  begin
  for i:=0 to connInfoMap.Count-1 do
    begin
    if (connInfoMap.Objects[i] <> nil) then
      begin
      connInfoMap.Objects[i].Free;
      end;
    end;
  connInfoMap.Free;
  connInfoMap := nil;
  end;

if (addrInfoMap <> nil) then
  begin
  for i:=0 to addrInfoMap.Count-1 do
    begin
    if (addrInfoMap.Objects[i] <> nil) then
      begin
      addrInfoMap.Objects[i].Free;
      end;
    end;
  addrInfoMap.Free;
  addrInfoMap := nil;
  end;
end;

function createConnInfoMaps : boolean;
begin
connInfoMap := TStringList.Create;
if (connInfoMap = nil) then
  begin
  Result := false;
  exit;
  end;

addrInfoMap := TStringList.Create;
if (addrInfoMap = nil) then
  begin
    freeConnInfoMaps();
    Result := false;
    exit;
  end;

Result := true;
end;

procedure copyArray(dst: pointer; src: pointer; len:integer);
var i:integer;
    pDst, pSrc : PAnsiChar;
begin
pDst := dst;
pSrc := src;
for i:=0 to len-1 do
  begin
  pDst[i] := pSrc[i];
  end;
end;

function equalArrays(dst: pointer; src: pointer; len:integer) : boolean;
var i:integer;
    pDst, pSrc : PAnsiChar;
begin
pDst := dst;
pSrc := src;
for i:=0 to len-1 do
  begin
  if (pDst[i] <> pSrc[i]) then
    begin
    Result := false;
    exit;
    end;
  end;
Result := true;
end;

//////////////////////////////////////////////////////////
// Event handlers

procedure threadStart(); cdecl;
begin
end;

procedure threadEnd(); cdecl;
begin
end;

procedure tcpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
var pAddr : ^sockaddr_in;
    pAddrR : ^sockaddr_in;
    addrLen : integer;
    oci : ORIGINAL_CONN_INFO;
    remoteAddrStr : array [0..MAX_PATH] of char;
    pStr : PAnsiChar;
    addrStrLen : Longword;
    s : string;
begin

pAddrR := @tcpProxyAddress;
pAddr := @pConnInfo.remoteAddress;

if (pAddr.sin_family <> pAddrR.sin_family) then
  exit;

if (pConnInfo.processId = proxyPid) then
  exit;

if (pAddr.sin_family = AF_INET) then
  addrLen := sizeof(sockaddr_in)
  else
  addrLen := NF_MAX_ADDRESS_LENGTH;

if (proxyType <> 0) then
  begin
  oci := ORIGINAL_CONN_INFO.Create;
  copyArray(@oci.remoteAddress, @pConnInfo.remoteAddress, sizeof(oci.remoteAddress));
  oci.connectSent := false;
  // Save the original remote address
  connInfoMap.AddObject(IntToStr(id), oci);
  end;

// Do not redirect the connections twice
if (not equalArrays(PAnsiChar(pAddr), @tcpProxyAddress, addrLen)) then
  begin
  // Add log record
  addrStrLen := sizeof(remoteAddrStr);
  pStr := PAnsiChar(@remoteAddrStr);
  pStr[0] := #0;
  WSAAddressToStringA(@pConnInfo.remoteAddress, addrLen, nil, pStr, addrStrLen);
  FmtStr(s, 'TCP connection id=%d remoteAddr=%s is redirected',
        [id, remoteAddrStr]);
  MainForm.addLogString(s);

  // Change the remote address
  copyArray(@pConnInfo.remoteAddress, @tcpProxyAddress, addrLen);

  pConnInfo.processId := proxyPid;

  if (proxyType <> 0) then
    begin
    // We must filter the connections for making tunnels
    pConnInfo.filteringFlag := pConnInfo.filteringFlag or NF_FILTER;
    end;

  end;

end;

procedure tcpConnected(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
end;

procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
var idStr : string;
    i : integer;
begin
if (proxyType <> 0) then
  begin
  idStr := IntToStr(id);
  i := connInfoMap.IndexOf(idStr);
  if (i <> -1) then
    begin
    connInfoMap.Objects[i].Free;
    connInfoMap.Delete(i);
    end;
  end;
end;

procedure tcpReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
var idStr : string;
    i : integer;
    oci : ORIGINAL_CONN_INFO;
begin
if ((proxyType <> 0) and (len > 0)) then
  begin
  idStr := IntToStr(id);
  i := connInfoMap.IndexOf(idStr);
  if (i <> -1) then
    begin
    oci := ORIGINAL_CONN_INFO(connInfoMap.Objects[i]);

    if (length(oci.pendingSends) > 0) then
        begin
        nf_tcpPostSend(id, pchar(oci.pendingSends), length(oci.pendingSends));
        end;

    connInfoMap.Objects[i].Free;
    connInfoMap.Delete(i);

    exit;
    end;
  end;

nf_tcpPostReceive(id, buf, len);
end;

procedure tcpSend(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
var idStr : string;
    i : integer;
    oci : ORIGINAL_CONN_INFO;
    s : AnsiString;
    localAddr : array [0..MAX_PATH] of char;
    pAddr : ^sockaddr_in;
    pStr : PAnsiChar;
    addrStrLen : Longword;
    addrLen : Longword;
    socksRequest : SOCKS4_REQUEST;
begin
if (proxyType <> 0) then
  begin
  idStr := IntToStr(id);
  i := connInfoMap.IndexOf(idStr);
  if (i <> -1) then
    begin
    oci := ORIGINAL_CONN_INFO(connInfoMap.Objects[i]);
    if (len > 0) then
        begin
        SetString(s, buf, len);
        oci.pendingSends := oci.pendingSends + s;

        if (not oci.connectSent) then
          begin
          oci.connectSent := true;

          pAddr := @oci.remoteAddress;

          if (pAddr.sin_family = AF_INET) then
             addrLen := sizeof(sockaddr_in)
             else
             addrLen := NF_MAX_ADDRESS_LENGTH;

          if (proxyType = 1) then
            begin
            addrStrLen := sizeof(localAddr);
            pStr := PAnsiChar(@localAddr);
            pStr[0] := #0;
            WSAAddressToStringA(@oci.remoteAddress, addrLen, nil, pStr, addrStrLen);

            FmtStr(s, 'CONNECT %s HTTP/1.0' + #13#10#13#10, [pStr]);

            // Send the request first
            nf_tcpPostSend(id, PAnsiChar(s), Length(s));
            end else
          if (proxyType = 2) then
            begin
            socksRequest.version := SOCKS_4;
	    socksRequest.command := S4C_CONNECT;
	    socksRequest.ip := pAddr.sin_addr.S_addr;
	    socksRequest.port := pAddr.sin_port;
	    socksRequest.userid := 0;

            // Send the request first
            nf_tcpPostSend(id, @socksRequest, sizeof(socksRequest));
            end;
          end;

        end;
    exit;
    end;
  end;

nf_tcpPostSend(id, buf, len);
end;

procedure tcpCanReceive(id : ENDPOINT_ID); cdecl;
begin
end;

procedure tcpCanSend(id : ENDPOINT_ID); cdecl;
begin
end;

procedure udpCreated(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
begin
end;

procedure udpConnectRequest(id : ENDPOINT_ID; var pConnReq : NF_UDP_CONN_REQUEST); cdecl;
begin
end;

procedure udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
begin
end;

procedure udpReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
end;

procedure udpSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
var    request : TDnsRequest;
       pAddr : ^sockaddr_in;
       pServerAddr : ^sockaddr_in;
    addrLen : integer;
    remoteAddrStr : array [0..MAX_PATH] of char;
    pStr : PAnsiChar;
    addrStrLen : Longword;
    s : string;
begin
pAddr := pointer(remoteAddress);
pServerAddr := @dnsServerAddress;
if ((pAddr.sin_family <> AF_INET) or (pAddr.sin_addr.S_addr = pServerAddr.sin_addr.S_addr)) then
  begin
  nf_udpPostSend(id, remoteAddress, buf, len, options);
  exit;
  end;

addrLen := sizeof(sockaddr_in);
addrStrLen := sizeof(remoteAddrStr);
pStr := PAnsiChar(@remoteAddrStr);
pStr[0] := #0;
WSAAddressToStringA(remoteAddress, addrLen, nil, pStr, addrStrLen);
FmtStr(s, 'UDP datagram id=%d remoteAddr=%s is redirected',
        [id, remoteAddrStr]);
MainForm.addLogString(s);

request := TDnsRequest.Create(id, remoteAddress, buf, len, options);
dnsResolver.addRequest(request);
end;

procedure udpCanReceive(id : ENDPOINT_ID); cdecl;
begin
end;

procedure udpCanSend(id : ENDPOINT_ID); cdecl;
begin
end;

//////////////////////////////////////////////////////////
// Main routines

function startFilter() : boolean;
var rule : NF_RULE;
begin
  if (not createConnInfoMaps()) then
    begin
    Result := false;
    exit;
    end;

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

  err := nf_init(PAnsiChar('netfilter2'), eh);
  if (err = NF_STATUS_SUCCESS) then
  begin
    if (filterTCP) then
      begin
      FillChar(rule, sizeof(rule), 0);
      rule.protocol := IPPROTO_TCP;
      rule.direction := NF_D_OUT;
      rule.remotePort := htons(tcpPort);
      rule.filteringFlag := NF_INDICATE_CONNECT_REQUESTS;
      nf_addRule(rule, 1);
      end;

    if (filterUDP) then
      begin
      // Filter outgoing UDP directed to port 53
      FillChar(rule, sizeof(rule), 0);
      rule.protocol := IPPROTO_UDP;
      rule.direction := NF_D_OUT;
      rule.remotePort := htons(53);
      rule.filteringFlag := NF_FILTER;
      nf_addRule(rule, 1);
      end;

    Result := true;
  end else
  begin
    Result := false;
  end;
end;

procedure stopFilter;
begin
  nf_free();
  freeConnInfoMaps();
end;

procedure TMainForm.StartClick(Sender: TObject);
var addrLen : Longword;
begin

IsMultiThread := true;

// Clear the filtering log
Log.Clear;

// Setup local variables according to selected parameters

if (redirectTCP.Checked) then
  begin
  addrLen := sizeof(tcpProxyAddress);
  err := WSAStringToAddressA(PAnsiChar(ProxyAddressEdit.Text), AF_INET, nil, @tcpProxyAddress, addrLen);
  if (err < 0) then
  begin
  addrLen := sizeof(tcpProxyAddress);
  err := WSAStringToAddressA(PAnsiChar(ProxyAddressEdit.Text), AF_INET6, nil, @tcpProxyAddress, addrLen);
  if (err < 0) then
    begin
    ShowMessage('Invalid proxy address');
    exit;
    end;
  end;

  proxyType := ProxyTypeCombo.ItemIndex;

  tcpPort := StrToInt(PortEdit.Text);
  proxyPid := StrToInt(ProxyPidEdit.Text);

  filterTCP := true;
end else
begin
  filterTCP := false;
end;

if (redirectUDP.Checked) then
  begin
  addrLen := sizeof(dnsServerAddress);
  err := WSAStringToAddressA(PAnsiChar(DNSServerAddressEdit.Text), AF_INET, nil, @dnsServerAddress, addrLen);
  if (err < 0) then
  begin
  addrLen := sizeof(dnsServerAddress);
  err := WSAStringToAddressA(PAnsiChar(DNSServerAddressEdit.Text), AF_INET6, nil, @dnsServerAddress, addrLen);
  if (err < 0) then
    begin
    ShowMessage('Invalid proxy address');
    exit;
    end;
  end;

  proxyPid := StrToInt(ProxyPidEdit.Text);

  dnsResolver := TDnsResolver.Create(10, @dnsServerAddress);
  filterUDP := true;
end else
begin
  filterUDP := false;
end;

// Try to start the filtering
if (startFilter()) then
  begin
  Start.Enabled := false;
  Stop.Enabled := true;
  end else
  begin
  ShowMessage('Unable to attach to driver');
  end;

end;

procedure TMainForm.StopClick(Sender: TObject);
begin
stopFilter();
Start.Enabled := true;
Stop.Enabled := false;

if (dnsResolver <> nil) then
    begin
    dnsResolver.Free;
    dnsResolver := nil;
    end;

end;

procedure TMainForm.addLogString(const s : string);
begin
Log.Lines.Add(s);
end;

end.

