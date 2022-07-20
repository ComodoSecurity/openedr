unit MainFormUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, WinSock, ExtCtrls;

type
  TMainForm = class(TForm)
    Start: TButton;
    Stop: TButton;
    Label1: TLabel;
    ProcessName: TEdit;
    Label2: TLabel;
    Limit: TEdit;
    Timer1: TTimer;
    procedure StartClick(Sender: TObject);
    procedure StopClick(Sender: TObject);
    procedure Timer1Timer(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  MainForm: TMainForm;

implementation

uses NetFilter2API;

{$R *.dfm}

const MAX_PATH = 256;

var     eh : NF_EventHandler;
        err : integer;

        g_processName : string;
        g_ioLimit : cardinal;

        g_bytesIn : Cardinal;
        g_bytesOut : Cardinal;

        connInfoMap : TStringList;
        addrInfoMap : TStringList;

//////////////////////////////////////////////////////////
// Support routines

function checkProcessName(processId : Cardinal) : boolean;
var procName : array [0..MAX_PATH] of AnsiChar;
    sName : string;
begin
if (not nf_getProcessNameA(processId, @procName, sizeof(procName))) then
  StrCopy(procName, 'System');

sName := procName;

if (length(sName) < length(g_processName)) then
  begin
  result := false;
  exit;
  end;

if StrIComp(pchar(g_processName), procName + length(sName) - length(g_processName)) = 0 then
  result := true
  else
  result := false;
end;

procedure freeConnInfoMaps;
begin
if (connInfoMap <> nil) then
  begin
  connInfoMap.Free;
  connInfoMap := nil;
  end;

if (addrInfoMap <> nil) then
  begin
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

//////////////////////////////////////////////////////////
// Event handlers

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
if not checkProcessName(pConnInfo.processId) then
  begin
  nf_tcpDisableFiltering(id);
  exit;
  end;

connInfoMap.Add(IntToStr(id));
end;

procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
var idStr : string;
    i : integer;
begin
  idStr := IntToStr(id);
  i := connInfoMap.IndexOf(idStr);
  if (i <> -1) then
    begin
    connInfoMap.Delete(i);
    end;
end;

procedure tcpReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
var idStr : string;
    i : integer;
begin
idStr := IntToStr(id);
i := connInfoMap.IndexOf(idStr);
if (i <> -1) then
  begin
  g_bytesIn := g_bytesIn + cardinal(len);
  if (g_bytesIn > g_ioLimit) then
        nf_tcpSetConnectionState(id, 1);
  end;

nf_tcpPostReceive(id, buf, len);
end;

procedure tcpSend(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
var idStr : string;
    i : integer;
begin
idStr := IntToStr(id);
i := connInfoMap.IndexOf(idStr);
if (i <> -1) then
  begin
  g_bytesOut := g_bytesOut + cardinal(len);
  if (g_bytesOut > g_ioLimit) then
        nf_tcpSetConnectionState(id, 1);
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
if not checkProcessName(pConnInfo.processId) then
  begin
  nf_udpDisableFiltering(id);
  exit;
  end;

addrInfoMap.Add(IntToStr(id));
end;

procedure udpConnectRequest(id : ENDPOINT_ID; var pConnReq : NF_UDP_CONN_REQUEST); cdecl;
begin
end;

procedure udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
var idStr : string;
    i : integer;
begin
idStr := IntToStr(id);
i := addrInfoMap.IndexOf(idStr);
if (i <> -1) then
  begin
  addrInfoMap.Delete(i);
  end;
end;

procedure udpReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
var idStr : string;
    i : integer;
begin
idStr := IntToStr(id);
i := addrInfoMap.IndexOf(idStr);
if (i <> -1) then
  begin
  g_bytesIn := g_bytesIn + cardinal(len);
  if (g_bytesIn > g_ioLimit) then
        nf_udpSetConnectionState(id, 1);
  end;

nf_udpPostReceive(id, remoteAddress, buf, len, options);
end;

procedure udpSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
var idStr : string;
    i : integer;
begin
idStr := IntToStr(id);
i := addrInfoMap.IndexOf(idStr);
if (i <> -1) then
  begin
  g_bytesOut := g_bytesOut + cardinal(len);
  if (g_bytesOut > g_ioLimit) then
        nf_udpSetConnectionState(id, 1);
  end;

nf_udpPostSend(id, remoteAddress, buf, len, options);
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
      FillChar(rule, sizeof(rule), 0);
      rule.protocol := IPPROTO_TCP;
      rule.filteringFlag := NF_FILTER;
      nf_addRule(rule, 1);

      FillChar(rule, sizeof(rule), 0);
      rule.protocol := IPPROTO_UDP;
      rule.filteringFlag := NF_FILTER;
      nf_addRule(rule, 1);

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
begin
// Setup local variables according to selected parameters
g_processName := ProcessName.Text;
g_ioLimit := StrToInt(Limit.Text);

// Try to start the filtering
if (startFilter()) then
  begin
  Start.Enabled := false;
  Stop.Enabled := true;
  Timer1.Enabled := true;
  end else
  begin
  ShowMessage('Unable to attach to driver');
  end;

end;

procedure TMainForm.StopClick(Sender: TObject);
begin
Timer1.Enabled := false;
stopFilter();
Start.Enabled := true;
Stop.Enabled := false;
end;

procedure TMainForm.Timer1Timer(Sender: TObject);
var i, suspend : integer;
    id : ENDPOINT_ID;
begin
// Update IO counters
if (g_bytesIn > g_ioLimit) then
  g_bytesIn := g_bytesIn - g_ioLimit
  else
  g_bytesIn := 0;
if (g_bytesOut > g_ioLimit) then
  g_bytesOut := g_bytesOut - g_ioLimit
  else
  g_bytesOut := 0;

// Suspend or resume TCP/UDP sockets belonging to specified application
if (g_bytesIn > g_ioLimit) or (g_bytesOut > g_ioLimit) then
  suspend := 1
  else
  suspend := 0;

for i:=0 to addrInfoMap.Count-1 do
    begin
    id := StrToInt(addrInfoMap.Strings[i]);
    nf_udpSetConnectionState(id, suspend);
    end;
for i:=0 to connInfoMap.Count-1 do
    begin
    id := StrToInt(connInfoMap.Strings[i]);
    nf_tcpSetConnectionState(id, suspend);
    end;
end;

end.

