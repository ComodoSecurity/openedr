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
    GroupBox1: TGroupBox;
    Label3: TLabel;
    InBytesLabel: TLabel;
    Label4: TLabel;
    InSpeedLabel: TLabel;
    Label5: TLabel;
    OutBytesLabel: TLabel;
    Label7: TLabel;
    OutSpeedLabel: TLabel;
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

        g_flowCtlHandle : Longword;

const  scd = 4;

var
        stat, oldStat : NF_FLOWCTL_STAT;
	ts, tsSpeed : Longword;
	inSpeed, outSpeed : Int64;

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

if AnsiStrIComp(pchar(g_processName), procName + length(sName) - length(g_processName)) = 0 then
  result := true
  else
  result := false;
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
if checkProcessName(pConnInfo.processId) then
  begin
  nf_setTCPFlowCtl(id, g_flowCtlHandle);
  end;
end;

procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
end;

procedure tcpReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
begin
nf_tcpPostReceive(id, buf, len);
end;

procedure tcpSend(id : ENDPOINT_ID; buf : PAnsiChar; len : integer); cdecl;
begin
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
if checkProcessName(pConnInfo.processId) then
  begin
  nf_setUDPFlowCtl(id, g_flowCtlHandle);
  end;
end;

procedure udpConnectRequest(id : ENDPOINT_ID; var pConnReq : NF_UDP_CONN_REQUEST); cdecl;
begin
end;

procedure udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
begin
end;

procedure udpReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
nf_udpPostReceive(id, remoteAddress, buf, len, options);
end;

procedure udpSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
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
    data : NF_FLOWCTL_DATA;
begin
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
      data.inLimit := g_ioLimit;
      data.outLimit := g_ioLimit;
      nf_addFlowCtl(data, g_flowCtlHandle);

      FillChar(rule, sizeof(rule), 0);
      rule.filteringFlag := NF_CONTROL_FLOW;
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
end;

procedure TMainForm.StartClick(Sender: TObject);
begin
// Setup local variables according to selected parameters
g_processName := ProcessName.Text;
g_ioLimit := StrToInt(Limit.Text);

FillChar(stat, sizeof(stat), 0);
FillChar(oldStat, sizeof(oldStat), 0);
ts := 0;
tsSpeed := 0;
inSpeed := 0;
outSpeed := 0;

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
begin
ts := GetTickCount();

if (nf_getFlowCtlStat(g_flowCtlHandle, stat) = NF_STATUS_SUCCESS) then
  begin
  if ((ts - tsSpeed) >= scd * 1000) then
    begin
    inSpeed := (stat.inBytes - oldStat.inBytes) div scd;
    outSpeed := (stat.outBytes - oldStat.outBytes) div scd;
    tsSpeed := ts;
    oldStat := stat;
    end;

    InBytesLabel.Caption := IntToStr(stat.inBytes);
    InSpeedLabel.Caption := IntToStr(inSpeed);
    OutBytesLabel.Caption := IntToStr(stat.outBytes);
    OutSpeedLabel.Caption := IntToStr(outSpeed);
  end;
end;

end.

