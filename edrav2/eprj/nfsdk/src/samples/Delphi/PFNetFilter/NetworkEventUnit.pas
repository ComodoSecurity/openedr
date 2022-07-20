unit NetworkEventUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, NetFilter2API;

const
   { Firewall options }
   FO_ONCE = 0;
   FO_PROCESS_PORT = 1;
   FO_PROCESS = 2;

type
  TNetworkEventForm = class(TForm)
    Label1: TLabel;
    Process: TEdit;
    User: TEdit;
    Label2: TLabel;
    Label3: TLabel;
    LocalAddress: TEdit;
    Label4: TLabel;
    RemoteAddress: TEdit;
    Allow: TButton;
    Block: TButton;
    Options: TRadioGroup;
    Protocol: TEdit;
    Label5: TLabel;
    Label6: TLabel;
    Direction: TEdit;
    procedure FormShow(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    function showEvent(const pConnInfo : NF_TCP_CONN_INFO; var options : integer) : Longword;
  end;

var
  NetworkEventForm: TNetworkEventForm;

implementation

uses WinSock, MainUnit, ProtocolFiltersAPI;

{$R *.dfm}

function TNetworkEventForm.showEvent(const pConnInfo : NF_TCP_CONN_INFO; var options : integer) : Longword;
var mr : TModalResult;
    pAddr : ^sockaddr_in;
    addrLen : integer;
    remoteAddrStr : array [0..MAX_PATH] of AnsiChar;
    localAddrStr : array [0..MAX_PATH] of AnsiChar;
    processName : array [0..MAX_PATH] of AnsiChar;
    processOwner : array [0..MAX_PATH] of AnsiChar;
    pStr : PAnsiChar;
    addrStrLen : Longword;
begin
pAddr := @pConnInfo.remoteAddress;
if (pAddr.sin_family = AF_INET) then
  addrLen := sizeof(sockaddr_in)
  else
  addrLen := NF_MAX_ADDRESS_LENGTH;

addrStrLen := sizeof(remoteAddrStr);
pStr := PAnsiChar(@remoteAddrStr);
pStr[0] := #0;
WSAAddressToStringA(@pConnInfo.remoteAddress, addrLen, nil, pStr, addrStrLen);

RemoteAddress.Text := remoteAddrStr;

addrStrLen := sizeof(localAddrStr);
pStr := PAnsiChar(@localAddrStr);
pStr[0] := #0;
WSAAddressToStringA(@pConnInfo.localAddress, addrLen, nil, pStr, addrStrLen);

LocalAddress.Text := localAddrStr;

processName[0] := #0;
if (not nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
   StrCopy(processName, 'System');

Process.Text := processName;

processOwner[0] := #0;
pfc_getProcessOwnerA(pConnInfo.processId, @processOwner, sizeof(processOwner));

User.Text := processOwner;

Protocol.Text := 'TCP';
case pConnInfo.direction of
  NF_D_IN:
     Direction.Text := 'Inbound';
  NF_D_OUT:
     Direction.Text := 'Outbound';
end;

mr := self.ShowModal;
case mr of
  mrOk:
    Result := NF_ALLOW or NF_FILTER;
  mrCancel:
    Result := NF_BLOCK;
else
Result := NF_ALLOW;
end;
options := NetworkEventForm.Options.ItemIndex;
end;

procedure TNetworkEventForm.FormShow(Sender: TObject);
begin
SetWindowPos(self.Handle, HWND_TOPMOST, self.Left, self.Top, self.Width, self.Height,
        SWP_SHOWWINDOW or SWP_NOOWNERZORDER);
end;

end.
