unit AddHeaderUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, NetFilter2API, ProtocolFiltersAPI;

type
  TForm1 = class(TForm)
    Start: TButton;
    Stop: TButton;
    HeaderNameEdit: TEdit;
    Name: TLabel;
    Value: TLabel;
    HeaderValueEdit: TEdit;
    procedure StartClick(Sender: TObject);
    procedure StopClick(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

uses WinSock;

{$R *.dfm}

var
  headerName : AnsiString;
  headerValue : AnsiString;
  eh : PFEvents_c;
  err : integer;

procedure threadStart(); cdecl;
begin
end;

procedure threadEnd(); cdecl;
begin
end;

procedure tcpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
end;

procedure tcpConnected(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
if (pConnInfo.direction = NF_D_OUT) then
  begin
  // Add required filters
  pfc_addFilter(id, FT_PROXY, FF_DEFAULT, OT_LAST, FT_NONE);
  pfc_addFilter(id, FT_SSL, FF_SSL_VERIFY, OT_LAST, FT_NONE);
  // Filter only HTTP requests
  pfc_addFilter(id, FT_HTTP, FF_DONT_FILTER_IN, OT_LAST, FT_NONE);
  end;
end;

procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
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

function filterHttpRequest(id : ENDPOINT_ID; obj : TPFObject) : boolean;
var  ph : TPFHeader;
     pStream : TPFStream;
begin
result := false;

pStream := obj.getStream(HS_HEADER);

// Read HTTP header, add a header field, and save it back to stream
ph := TPFHeader.Create();
if (Assigned(ph)) then
  begin
  if (ph.readFromStream(pStream)) then
    begin
    ph.addField(headerName, headerValue, false);
    pStream.reset;
    ph.writeToStream(pStream);
    result := true;
    end;
  end;
end;

procedure dataAvailable(id : ENDPOINT_ID; pObject : PPFObject_c); cdecl;
var obj : TPFObject;
begin
obj := TPFObject.Create(pObject);

if (not obj.readOnly) then
  begin
  case obj.objectType of
   OT_HTTP_REQUEST:
     begin
     filterHttpRequest(id, obj);
     end;
  end;
  pfc_postObject(id, pObject);
  end;

obj.Free;
end;

function dataPartAvailable(id : ENDPOINT_ID; pObject : PPFObject_c) : PF_DATA_PART_CHECK_RESULT; cdecl;
var obj : TPFObject;
begin
obj := TPFObject.Create(pObject);
try
if (obj.objectType = OT_HTTP_REQUEST) then
  begin
  if (filterHttpRequest(id, obj)) then
    begin
    Result := DPCR_UPDATE_AND_BYPASS;
    exit;
    end;
  end;
finally
obj.Free;
end;

Result := DPCR_BYPASS;
end;

function tcpPostSend(id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl;
begin
Result := nf_tcpPostSend(id, buf, len);
end;

function tcpPostReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl;
begin
Result := nf_tcpPostReceive(id, buf, len);
end;

function tcpSetConnectionState(id : ENDPOINT_ID; suspended : integer): integer; cdecl;
begin
Result := nf_tcpSetConnectionState(id, suspended);
end;

function udpPostSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl;
begin
Result := nf_udpPostSend(id, remoteAddress, buf, len, options);
end;

function udpPostReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl;
begin
Result := nf_udpPostReceive(id, remoteAddress, buf, len, options);
end;

function udpSetConnectionState(id : ENDPOINT_ID; suspended : integer): integer; cdecl;
begin
Result := nf_udpSetConnectionState(id, suspended);
end;

procedure tcpReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
begin
pfc_getNFEventHandler().tcpReceive(id, buf, len);
end;

procedure udpReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
pfc_getNFEventHandler().udpReceive(id, remoteAddress, buf, len, options);
end;

function tcpDisableFiltering(id : ENDPOINT_ID) : integer; cdecl;
begin
Result := nf_tcpDisableFiltering(id);
end;

function udpDisableFiltering(id : ENDPOINT_ID) : integer; cdecl;
begin
Result := nf_udpDisableFiltering(id);
end;


function startFilter() : boolean;
var rule : NF_RULE;
    dataFolder : WideString;
    pAddr : ^Longword;
begin

  // VERY IMPORTANT assignment!!!
  // Without this string memory manager uses a single threaded model,
  // and doesn't work properly when the memory is allocated from
  // event handlers, which are executed in a separate thread.

  IsMultiThread := true;

  // Initialize ProtocolFilters event handlers

  FillChar(eh, sizeof(eh), 0);

  eh.threadStart := threadStart;
  eh.threadEnd := threadEnd;

  eh.tcpConnectRequest := tcpConnectRequest;
  eh.tcpConnected := tcpConnected;
  eh.tcpClosed := tcpClosed;

  eh.udpCreated := udpCreated;
  eh.udpConnectRequest := udpConnectRequest;
  eh.udpClosed := udpClosed;

  eh.dataAvailable := dataAvailable;
  eh.dataPartAvailable := dataPartAvailable;

  eh.tcpPostSend := tcpPostSend;
  eh.tcpPostReceive := tcpPostReceive;
  eh.tcpSetConnectionState := tcpSetConnectionState;

  eh.udpPostSend := udpPostSend;
  eh.udpPostReceive := udpPostReceive;
  eh.udpSetConnectionState := udpSetConnectionState;

  // Initialize ProtocolFilters
  dataFolder := 'c:\netfilter2';
  err := pfc_init(eh, PWideChar(dataFolder));
  if (err = 0) then
    begin
    Result := false;
    exit;
    end;

  pfc_setRootSSLCertSubject('Sample CA');

  // Initialize NFAPI
  err := nf_init('netfilter2', pfc_getNFEventHandler()^);
  if (err <> NF_STATUS_SUCCESS) then
    begin
    pfc_free();
    Result := false;
    exit;
    end;

  nf_setTCPTimeout(0);

  // Do not filter local traffic
  FillChar(rule, sizeof(rule), 0);
  rule.filteringFlag := NF_ALLOW;
  rule.ip_family := AF_INET;
  pAddr := @rule.remoteIpAddress;
  pAddr^ := inet_addr('127.0.0.1');
  pAddr := @rule.remoteIpAddressMask;
  pAddr^ := inet_addr('255.0.0.0');
  nf_addRule(rule, 0);

  // Filter TCP connections and indicate tcpConnectRequest events
  FillChar(rule, sizeof(rule), 0);
  rule.filteringFlag := NF_FILTER;
  rule.protocol := IPPROTO_TCP;
  rule.direction := NF_D_OUT;
  nf_addRule(rule, 0);

  // Block QUIC
  FillChar(rule, sizeof(rule), 0);

  rule.protocol := IPPROTO_UDP;
  rule.remotePort := ntohs(80);
  rule.filteringFlag := NF_BLOCK;
  nf_addRule(rule, 1);

  rule.protocol := IPPROTO_UDP;
  rule.remotePort := ntohs(443);
  rule.filteringFlag := NF_BLOCK;
  nf_addRule(rule, 1);

  Result := true;
end;

procedure stopFilter;
begin
  nf_free();
  pfc_free();
end;


procedure TForm1.StartClick(Sender: TObject);
begin
headerName := HeaderNameEdit.Text;
headerValue := HeaderValueEdit.Text;

if (startFilter()) then
  begin
  Start.Enabled := false;
  Stop.Enabled := true;
  end;
end;

procedure TForm1.StopClick(Sender: TObject);
begin
stopFilter();
Start.Enabled := true;
Stop.Enabled := false;
end;

end.
