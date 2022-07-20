unit MainUnit;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, Buttons, StdCtrls, ComCtrls, Math,
  ExtCtrls, Grids, ValEdit, WinSock,
  NetFilter2API, ProtocolFiltersAPI;

const
  // Custom message identifier
  WM_FILTEREVENT = WM_USER + 1;

  { FE_EventType : Filter event types }

  FE_tcpConnected = 1;
  FE_tcpClosed = 2;
  FE_udpCreated = 3;
  FE_udpClosed = 4;
  FE_dataAvailable = 5;

  FE_statUpdated = 6;
  FE_tcpConnectRequest = 7;
  FE_udpConnectRequest = 8;

type

  TForm1 = class(TForm)
    Panel1: TPanel;
    StartBtn: TButton;
    StopBtn: TButton;
    GroupBox4: TGroupBox;
    Splitter4: TSplitter;
    Sessions: TValueListEditor;
    Panel5: TPanel;
    GroupBox5: TGroupBox;
    Stat: TValueListEditor;
    Splitter7: TSplitter;
    FilterSSL: TCheckBox;
    FilterRaw: TCheckBox;
    SaveToBin: TCheckBox;
    Panel6: TPanel;
    Label5: TLabel;
    PageControl1: TPageControl;
    TabSheet1: TTabSheet;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    Panel2: TPanel;
    Splitter1: TSplitter;
    HttpList: TListView;
    GroupBox1: TGroupBox;
    Splitter2: TSplitter;
    HttpHeader: TValueListEditor;
    UrlStopWord: TEdit;
    HtmlStopWord: TEdit;
    BlockHtml: TMemo;
    TabSheet2: TTabSheet;
    Panel4: TPanel;
    Splitter5: TSplitter;
    MailList: TListView;
    GroupBox3: TGroupBox;
    MailContent: TRichEdit;
    TabSheet4: TTabSheet;
    Panel3: TPanel;
    Splitter3: TSplitter;
    RawList: TListView;
    GroupBox2: TGroupBox;
    RawContent: TRichEdit;
    StatusBar1: TStatusBar;
    Firewall: TCheckBox;
    SMTPBlackAddress: TEdit;
    POP3Prefix: TEdit;
    Label1: TLabel;
    Label6: TLabel;
    HttpContent: TRichEdit;
    TabSheet3: TTabSheet;
    Panel7: TPanel;
    Splitter6: TSplitter;
    FTPList: TListView;
    GroupBox6: TGroupBox;
    FTPContent: TRichEdit;
    TabSheet5: TTabSheet;
    Panel8: TPanel;
    Splitter8: TSplitter;
    ICQList: TListView;
    GroupBox7: TGroupBox;
    ICQBlockUIN: TEdit;
    ICQBlockString: TEdit;
    Label7: TLabel;
    Label8: TLabel;
    ICQHeader: TValueListEditor;
    Splitter9: TSplitter;
    ICQContent: TRichEdit;
    ICQBlockFileTransfers: TCheckBox;
    TabSheet6: TTabSheet;
    Panel9: TPanel;
    Splitter10: TSplitter;
    XMPPList: TListView;
    GroupBox8: TGroupBox;
    XMPPContent: TRichEdit;
    procedure StartBtnClick(Sender: TObject);
    procedure StopBtnClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure HttpListSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
    procedure MailListSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
    procedure RawListSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
    procedure FormDestroy(Sender: TObject);
    procedure UrlStopWordChange(Sender: TObject);
    procedure HtmlStopWordChange(Sender: TObject);
    procedure FilterSSLClick(Sender: TObject);
    procedure BlockHtmlChange(Sender: TObject);
    procedure FilterRawClick(Sender: TObject);
    procedure FirewallClick(Sender: TObject);
    procedure SMTPBlackAddressChange(Sender: TObject);
    procedure POP3PrefixChange(Sender: TObject);
    procedure FTPListSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
    procedure ICQListSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);
    procedure ICQBlockUINChange(Sender: TObject);
    procedure ICQBlockStringChange(Sender: TObject);
    procedure ICQBlockFileTransfersClick(Sender: TObject);
    procedure XMPPListSelectItem(Sender: TObject; Item: TListItem;
      Selected: Boolean);

  private
    { Private declarations }

    m_tcpSessions : TStringList;
    m_pStgObject : TPFObject;

    // Private methods

    procedure FilterEvent(var Msg: TMessage); message WM_FILTEREVENT;

    function tcpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO) : integer;
    procedure tcpConnected(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO);
    procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO);
    procedure udpCreated(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO);
    procedure udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO);
    procedure dataAvailable(id : ENDPOINT_ID; pObj : TPFObject; blocked : boolean);

    procedure addHttpItem(id : ENDPOINT_ID; pObject : TPFObject; blocked : boolean);
    procedure addMailItem(id : ENDPOINT_ID; pObject : TPFObject);
    procedure addRawItem(id : ENDPOINT_ID; pObject : TPFObject);
    procedure addFTPItem(id : ENDPOINT_ID; pObject : TPFObject);
    procedure addICQItem(id : ENDPOINT_ID; pObject : TPFObject; blocked : boolean);
    procedure addXMPPItem(id : ENDPOINT_ID; pObject : TPFObject);

    procedure updateStatistics(p: pointer);
  public
    { Public declarations }
  end;

  FE_EventType = integer;

  FE_EVENT = packed record
        et : FE_EventType;
	id : ENDPOINT_ID;
        blocked : boolean;
        pData : pointer;
  end;

var
  Form1: TForm1;

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

implementation

uses FilterUnit, NetworkEventUnit;

var c : integer;


{$R *.dfm}

// Saves object to a file in current directory
procedure saveObject(id : ENDPOINT_ID; pObject : TPFObject);
var fileName : string;
    f : File;
    tempBuf : array [0..1000] of byte;
    tempLen : integer;
    pStream : TPFStream;
    i : integer;
begin
c := c + 1;
FmtStr(fileName, '%d_%.8d_%d.bin', [id, c, pObject.objectType]);
Assign(f, fileName);
Rewrite(f, 1);

if (IOResult = 0) then
  begin
  for i:=0 to pObject.streamCount do
    begin
    pStream := pObject[i];
    if (pStream <> nil) then
      begin
      pStream.seek(0, FILE_BEGIN);
      repeat
	tempLen := pStream.read(@tempBuf, sizeof(tempBuf));
        if (tempLen <= 0) then
          break;
	BlockWrite(f, tempBuf, tempLen);
      until (false);
      end;
    end;

  CloseFile(f);
  end;
end;

// Saves or loads an object from the first stream of the
// specified storage object, at the specified offset.
function serializeObject(save : boolean;
                offset:tStreamPos;
                pStorage : TPFObject;
                var pObject : TPFObject) : tStreamPos;
var tempBuf : array [0..1000] of byte;
    tempLen, rwLen : integer;
    pStgStream : TPFStream;
    pStream : TPFStream;
    i : integer;
    ot : PF_ObjectType;
    nStreams : integer;
    streamLen : tStreamPos;
begin
if (not assigned(pStorage)) then
  begin
  Result := 0;
  exit;
  end;

pStgStream := pStorage[0];
if (not assigned(pStgStream)) then
  begin
  Result := 0;
  exit;
  end;

if (save) then
  begin
  if (not assigned(pObject)) then
    begin
    Result := 0;
    exit;
    end;

  Result := pStgStream.seek(0, FILE_END);

  ot := pObject.objectType;
  pStgStream.write(@ot, sizeof(ot));
  nStreams := pObject.streamCount;
  pStgStream.write(@nStreams, sizeof(nStreams));

  for i:=0 to nStreams-1 do
    begin
    pStream := pObject[i];
    pStream.seek(0, FILE_BEGIN);
    streamLen := pStream.size;
    pStgStream.write(@streamLen, sizeof(streamLen));
    while (streamLen > 0) do
      begin
      rwLen := min(sizeof(tempBuf), streamLen);
      tempLen := pStream.read(@tempBuf, rwLen);
      if (tempLen <= 0) then
          break;
      pStgStream.write(@tempBuf, tempLen);
      streamLen := streamLen - tempLen;
      end;
    pStream.seek(0, FILE_BEGIN);
    end;

  end else
  begin
  pStgStream.seek(offset, FILE_BEGIN);

  pStgStream.read(@ot, sizeof(ot));
  pStgStream.read(@nStreams, sizeof(nStreams));

  pObject := TPFObject.Create(ot, nStreams);
  if (not assigned(pObject)) then
    begin
    Result := 0;
    exit;
    end;

  for i:=0 to nStreams-1 do
    begin
    pStgStream.read(@streamLen, sizeof(streamLen));
    pStream := pObject[i];
    while (streamLen > 0) do
      begin
      rwLen := min(sizeof(tempBuf), streamLen);
      pStgStream.read(@tempBuf, rwLen);
      pStream.write(@tempBuf, rwLen);
      streamLen := streamLen - rwLen;
      end;
    pStream.seek(0, FILE_BEGIN);
    end;
  Result := pStgStream.seek(0, FILE_END);
  end;
end;

// Handles WM_FILTEREVENT notifications
procedure TForm1.FilterEvent(var Msg: TMessage);
var pte : ^FE_EVENT;
begin
pte := pointer(Msg.lParam);
Msg.Result := 1;
case pte.et of
  FE_tcpConnected:
    begin
    tcpConnected(pte.id, NF_TCP_CONN_INFO(pte.pData^));
    FreeMem(pte.pData);
    FreeMem(pte);
    end;

  FE_tcpClosed:
    begin
    tcpClosed(pte.id, NF_TCP_CONN_INFO(pte.pData^));
    FreeMem(pte.pData);
    FreeMem(pte);
    end;

  FE_udpCreated:
    begin
    udpCreated(pte.id, NF_UDP_CONN_INFO(pte.pData^));
    FreeMem(pte.pData);
    FreeMem(pte);
    end;

  FE_udpClosed:
    begin
    udpClosed(pte.id, NF_UDP_CONN_INFO(pte.pData^));
    FreeMem(pte.pData);
    FreeMem(pte);
    end;

  FE_dataAvailable:
    begin
    dataAvailable(pte.id, pte.pData, pte.blocked);
    TPFObject(pte.pData).Free;
    FreeMem(pte);
    end;

  FE_statUpdated:
    begin
    updateStatistics(pte.pData);
    UI_STAT_VALUE(pte.pData).Free;
    FreeMem(pte);
    end;

  FE_tcpConnectRequest:
    begin
    Msg.Result := tcpConnectRequest(pte.id, NF_TCP_CONN_INFO(pte.pData^));
    end;
else
  Msg.Result := 0;
end;

end;

procedure TForm1.updateStatistics(p: pointer);
var pStatUI : UI_STAT_VALUE;
    s : AnsiString;
begin
pStatUI := UI_STAT_VALUE(p);
FmtStr(s, 'in: %d out: %d', [pStatUI.bytesIn, pStatUI.bytesOut]);
Stat.Values[pStatUI.proc] := s; 
end;

function TForm1.tcpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO) : integer;
var options : integer;
begin
pConnInfo.filteringFlag := NetworkEventForm.showEvent(pConnInfo, options);
Result := options;
end;

procedure TForm1.tcpConnected(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO);
var pAddr : ^sockaddr_in;
    addrLen : integer;
    remoteAddrStr : array [0..MAX_PATH] of AnsiChar;
    localAddrStr : array [0..MAX_PATH] of AnsiChar;
    processName : array [0..MAX_PATH] of AnsiChar;
    processOwner : array [0..MAX_PATH] of AnsiChar;
    pStr : PAnsiChar;
    addrStrLen : Longword;
    ids:AnsiString;
    s : AnsiString;
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

addrStrLen := sizeof(localAddrStr);
pStr := PAnsiChar(@localAddrStr);
pStr[0] := #0;
WSAAddressToStringA(@pConnInfo.localAddress, addrLen, nil, pStr, addrStrLen);

processName[0] := #0;
if (not nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
   StrCopy(processName, 'System');

processOwner[0] := #0;
pfc_getProcessOwnerA(pConnInfo.processId, @processOwner, sizeof(processOwner));

FmtStr(s, 'local=%s remote=%s [pid=%d owner=%s] %s',
        [localAddrStr, remoteAddrStr, pConnInfo.processId, processOwner, processName]);

ids := 'TCP:' + IntToStr(id);
m_tcpSessions.Values[ids] := s;

Sessions.InsertRow(ids, s, true);
end;

procedure TForm1.tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO);
var rowId:integer;
    ids : AnsiString;
begin
ids := 'TCP:' + IntToStr(id);
if Sessions.FindRow(ids, rowId) then
  begin
  Sessions.DeleteRow(rowId);
  end;
end;

procedure TForm1.udpCreated(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO);
begin
end;

procedure TForm1.udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO);
begin
end;

procedure TForm1.addHttpItem(id : ENDPOINT_ID; pObject : TPFObject; blocked : boolean);
var item:TListItem;
    s : AnsiString;
    i : integer;
begin
  item :=  HttpList.Items.Add;

  item.Caption := 'TCP:' + IntToStr(id);

  if (pObject.objectType = OT_HTTP_REQUEST) then
    item.SubItems.Add('Request')
    else
  if (pObject.objectType = OT_HTTPS_PROXY_REQUEST) then
    item.SubItems.Add('HTTPS proxy request')
    else
    item.SubItems.Add('Response');

  s := readHttpStatus(pObject);
  i := Pos(#13#10, s);
  if (i > 0) then
    s := Copy(s, 1, i-1);

  item.SubItems.Add(s);

  s := getHttpUrl(pObject);
  item.SubItems.Add(s);

  item.SubItems.Add(IntToStr(pObject[HS_CONTENT].size));

  if (blocked) then
    item.SubItems.Add('BLOCKED')
    else
    item.SubItems.Add('');

  if (m_tcpSessions.IndexOfName(item.Caption) <> -1) then
    begin
    item.SubItems.Add(m_tcpSessions.Values[item.Caption]);
    end;

  item.Data := pointer(serializeObject(true, 0, m_pStgObject, pObject));
end;

procedure TForm1.addMailItem(id : ENDPOINT_ID; pObject : TPFObject);
var item:TListItem;
begin
  item := MailList.Items.Add;
  item.Caption := 'TCP:' + IntToStr(id);

case pObject.objectType of
  OT_POP3_MAIL_INCOMING:
    begin
    item.SubItems.Add('Incoming mail')
    end;
  OT_SMTP_MAIL_OUTGOING:
    begin
    item.SubItems.Add('Outgoing mail');
    end;
  OT_NNTP_ARTICLE:
    begin
    item.SubItems.Add('Incoming news')
    end;
  OT_NNTP_POST:
    begin
    item.SubItems.Add('Outgoing news');
    end;
end;

  item.SubItems.Add(IntToStr(pObject[0].size));

  if (m_tcpSessions.IndexOfName(item.Caption) <> -1) then
    begin
    item.SubItems.Add(m_tcpSessions.Values[item.Caption]);
    end;

  item.Data := pointer(serializeObject(true, 0, m_pStgObject, pObject));
end;

procedure TForm1.addRawItem(id : ENDPOINT_ID; pObject : TPFObject);
var item:TListItem;
begin
  item := RawList.Items.Add;
  item.Caption := 'TCP:' + IntToStr(id);

  if (pObject.objectType = OT_RAW_INCOMING) then
    item.SubItems.Add('Incoming')
    else
    item.SubItems.Add('Outgoing');

  item.SubItems.Add(IntToStr(pObject[0].size));

  if (m_tcpSessions.IndexOfName(item.Caption) <> -1) then
    begin
    item.SubItems.Add(m_tcpSessions.Values[item.Caption]);
    end;

  item.Data := pointer(serializeObject(true, 0, m_pStgObject, pObject));
end;

procedure TForm1.addXMPPItem(id : ENDPOINT_ID; pObject : TPFObject);
var item:TListItem;
begin
  item := XMPPList.Items.Add;
  item.Caption := 'TCP:' + IntToStr(id);

  if (pObject.objectType = OT_XMPP_REQUEST) then
    item.SubItems.Add('Request')
    else
    item.SubItems.Add('Response');

  item.SubItems.Add(IntToStr(pObject[0].size));

  if (m_tcpSessions.IndexOfName(item.Caption) <> -1) then
    begin
    item.SubItems.Add(m_tcpSessions.Values[item.Caption]);
    end;

  item.Data := pointer(serializeObject(true, 0, m_pStgObject, pObject));
end;

procedure TForm1.addFTPItem(id : ENDPOINT_ID; pObject : TPFObject);
var item:TListItem;
begin
  item := FTPList.Items.Add;
  item.Caption := 'TCP:' + IntToStr(id);

case pObject.objectType of
  OT_FTP_COMMAND:
    begin
    item.SubItems.Add('Command');
    end;
  OT_FTP_RESPONSE:
    begin
    item.SubItems.Add('Response');
    end;
  OT_FTP_DATA_OUTGOING:
    begin
    item.SubItems.Add('Outgoing data')
    end;
  OT_FTP_DATA_INCOMING:
    begin
    item.SubItems.Add('Incoming data')
    end;
  OT_FTP_DATA_PART_OUTGOING:
    begin
    item.SubItems.Add('Outgoing data part')
    end;
  OT_FTP_DATA_PART_INCOMING:
    begin
    item.SubItems.Add('Incoming data part')
    end;
end;

  item.SubItems.Add(IntToStr(pObject[0].size));

  if (m_tcpSessions.IndexOfName(item.Caption) <> -1) then
    begin
    item.SubItems.Add(m_tcpSessions.Values[item.Caption]);
    end;

  item.Data := pointer(serializeObject(true, 0, m_pStgObject, pObject));
end;


procedure TForm1.addICQItem(id : ENDPOINT_ID; pObject : TPFObject; blocked : boolean);
var item:TListItem;
begin
  item := ICQList.Items.Add;
  item.Caption := 'TCP:' + IntToStr(id);

case pObject.objectType of
  OT_ICQ_CHAT_MESSAGE_OUTGOING:
    begin
    item.SubItems.Add('Outgoing message')
    end;
  OT_ICQ_CHAT_MESSAGE_INCOMING:
    begin
    item.SubItems.Add('Incoming message')
    end;
end;

  if (blocked) then
    item.SubItems.Add('BLOCKED')
    else
    item.SubItems.Add('');

  item.SubItems.Add(IntToStr(pObject[0].size));

  if (m_tcpSessions.IndexOfName(item.Caption) <> -1) then
    begin
    item.SubItems.Add(m_tcpSessions.Values[item.Caption]);
    end;

  item.Data := pointer(serializeObject(true, 0, m_pStgObject, pObject));
end;

procedure TForm1.dataAvailable(id : ENDPOINT_ID; pObj : TPFObject; blocked : boolean);
begin
// Update UI controls
case pObj.objectType of
  OT_HTTP_REQUEST:
    begin
    addHttpItem(id, pObj, blocked);
    end;
  OT_HTTP_RESPONSE:
    begin
    addHttpItem(id, pObj, blocked);
    end;
  OT_POP3_MAIL_INCOMING:
    begin
    addMailItem(id, pObj);
    end;
  OT_SMTP_MAIL_OUTGOING:
    begin
    addMailItem(id, pObj);
    end;
  OT_RAW_INCOMING:
    begin
    addRawItem(id, pObj);
    end;
  OT_RAW_OUTGOING:
    begin
    addRawItem(id, pObj);
    end;
  OT_HTTPS_PROXY_REQUEST:
    begin
    addHttpItem(id, pObj, blocked);
    end;
  OT_NNTP_ARTICLE:
    begin
    addMailItem(id, pObj);
    end;
  OT_NNTP_POST:
    begin
    addMailItem(id, pObj);
    end;
  OT_FTP_COMMAND:
    begin
    addFTPItem(id, pObj);
    end;
  OT_FTP_RESPONSE:
    begin
    addFTPItem(id, pObj);
    end;
  OT_FTP_DATA_OUTGOING:
    begin
    addFTPItem(id, pObj);
    end;
  OT_FTP_DATA_INCOMING:
    begin
    addFTPItem(id, pObj);
    end;
  OT_FTP_DATA_PART_OUTGOING:
    begin
    addFTPItem(id, pObj);
    end;
  OT_FTP_DATA_PART_INCOMING:
    begin
    addFTPItem(id, pObj);
    end;
  OT_ICQ_CHAT_MESSAGE_OUTGOING:
    begin
    addICQItem(id, pObj, blocked);
    end;
  OT_ICQ_CHAT_MESSAGE_INCOMING:
    begin
    addICQItem(id, pObj, blocked);
    end;
  OT_XMPP_REQUEST:
    begin
    addXMPPItem(id, pObj);
    end;
  OT_XMPP_RESPONSE:
    begin
    addXMPPItem(id, pObj);
    end;
end;

if SaveToBin.Checked then
  begin
  if (pObj.streamCount > 0) then
    begin
    saveObject(id, pObj);
    end;
  end;

end;


procedure TForm1.StartBtnClick(Sender: TObject);
begin
// Set the filtering parameters in FilterUnit
FilterUnit.setStringParam(PT_urlStopWord, AnsiLowerCase(urlStopWord.text));
FilterUnit.setStringParam(PT_htmlStopWord, AnsiLowerCase(htmlStopWord.text));
FilterUnit.setStringParam(PT_blockPage, blockHtml.Lines.Text);
FilterUnit.setBoolParam(PT_filterSSL, FilterSSL.Checked);
FilterUnit.setBoolParam(PT_filterRaw, FilterRaw.Checked);
FilterUnit.setBoolParam(PT_firewall, Firewall.Checked);
FilterUnit.setStringParam(PT_SMTPBlackAddress, AnsiLowerCase(SMTPBlackAddress.text));
FilterUnit.setStringParam(PT_POP3Prefix, AnsiLowerCase(POP3Prefix.text));
FilterUnit.setStringParam(PT_ICQBlockUIN, ICQBlockUIN.text);
FilterUnit.setStringParam(PT_ICQBlockString, ICQBlockString.text);
FilterUnit.setBoolParam(PT_ICQBlockFileTransfers, ICQBlockFileTransfers.Checked);

m_tcpSessions := TStringList.Create;

if (startFilter()) then
  begin
  StartBtn.Enabled := false;
  StopBtn.Enabled := true;
  end;

end;

procedure TForm1.StopBtnClick(Sender: TObject);
begin
if StopBtn.Enabled then
  begin
  stopFilter();
  StartBtn.Enabled := true;
  StopBtn.Enabled := false;
  Sessions.Strings.Clear;
  if (assigned(m_tcpSessions)) then
   begin
   m_tcpSessions.Free;
   m_tcpSessions := nil;
   end;
  end;
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
// Allow access to processes of all users
nf_adjustProcessPriviledges();
// Create storage
m_pStgObject := TPFObject.Create(OT_NULL, 1);

end;

procedure TForm1.HttpListSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var obj : TPFObject;
    ph : TPFHeader;
    i, len : integer;
    pContent : PAnsiChar;
    pStream : TPFStream;
    s : AnsiString;
    headerStmIndex : integer;
begin

if (not Selected) then
  exit;

HttpHeader.Strings.Clear;

// Read an object from stream
serializeObject(false, int64(Item.data), m_pStgObject, obj);

if (not Assigned(obj)) then
  exit;

ph := TPFHeader.create();
if (not assigned(ph)) then
  begin
  obj.Free;
  exit;
  end;

try
if (obj.objectType = OT_HTTPS_PROXY_REQUEST) then
  headerStmIndex := 0
  else
  headerStmIndex := HS_HEADER;

ph.readFromStream(obj[headerStmIndex]);

len := ph.size;

for i:=0 to len-1 do
 begin
 if (headerStmIndex = 0) and (i=0) then
   continue;
 HttpHeader.InsertRow(ph.getFieldName(i), ph.getFieldValue(i), true);
 end;

s := ph.getFieldValue('content-type');
if (Length(s) > 0) then
 begin
 // Show only text content
 if (Pos('text/', s) <> 0) or
    (Pos('application/javascript', s) <> 0) or
    (Pos('application/x-javascript', s) <> 0) or
    (Pos('application/x-www-form-urlencoded', s) <> 0) then
  begin
  pStream := obj[HS_CONTENT];
  if (Assigned(pStream)) then
    begin
    len := pStream.size;
    GetMem(pContent, len + 1);
    if (pContent <> nil) then
      begin
      pStream.seek(0, FILE_BEGIN);
      pStream.read(pContent, len);
      pContent[len] := #0;
      HttpContent.Text := pContent;
      FreeMem(pContent);
      end;
    end;
  end else
  begin
  HttpContent.Text := s;
  end;
 end else
 begin
 HttpContent.Text := '';
 end;
finally
ph.Free;
obj.Free;
end;
end;

procedure TForm1.MailListSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var obj : TPFObject;
    len : integer;
    pContent : PAnsiChar;
    pStream : TPFStream;
begin
if (not Selected) then
  exit;

// Read an object from stream
serializeObject(false, int64(Item.data), m_pStgObject, obj);

if (not Assigned(obj)) then
  exit;

pStream := obj[0];
if (Assigned(pStream)) then
  begin
  len := pStream.size;
  GetMem(pContent, len + 1);
  if (pContent <> nil) then
    begin
    pStream.seek(0, FILE_BEGIN);
    pStream.read(pContent, len);
    pContent[len] := #0;
    MailContent.Text := pContent;
    FreeMem(pContent);
    end;
  end else
  begin
  MailContent.Text := '';
  end;

obj.Free;
end;

procedure TForm1.RawListSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var obj : TPFObject;
    len : integer;
    pContent : PAnsiChar;
    pStream : TPFStream;
begin
if (not Selected) then
  exit;

// Read an object from stream
serializeObject(false, int64(Item.data), m_pStgObject, obj);

if (not Assigned(obj)) then
  exit;

pStream := obj[0];
if (Assigned(pStream)) then
  begin
  len := pStream.size();
  GetMem(pContent, len + 1);
  if (pContent <> nil) then
    begin
    pStream.seek(0, FILE_BEGIN);
    pStream.read(pContent, len);
    pContent[len] := #0;
    RawContent.Text := pContent;
    FreeMem(pContent);
    end;
  end else
  begin
  RawContent.Text := '';
  end;
obj.Free;
end;

procedure TForm1.FormDestroy(Sender: TObject);
begin
StopBtnClick(self);
if (assigned(m_pStgObject)) then
  m_pStgObject.Free;
end;

procedure TForm1.UrlStopWordChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_urlStopWord, AnsiLowerCase(urlStopWord.text));
end;

procedure TForm1.HtmlStopWordChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_htmlStopWord, AnsiLowerCase(htmlStopWord.text));
end;

procedure TForm1.FilterSSLClick(Sender: TObject);
begin
FilterUnit.setBoolParam(PT_filterSSL, FilterSSL.Checked);
end;

procedure TForm1.BlockHtmlChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_blockPage, blockHtml.Lines.Text);
end;

procedure TForm1.FilterRawClick(Sender: TObject);
begin
FilterUnit.setBoolParam(PT_filterRaw, FilterRaw.Checked);
end;

procedure TForm1.FirewallClick(Sender: TObject);
begin
FilterUnit.setBoolParam(PT_firewall, Firewall.Checked);
end;

procedure TForm1.SMTPBlackAddressChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_SMTPBlackAddress, AnsiLowerCase(SMTPBlackAddress.text));
end;

procedure TForm1.POP3PrefixChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_POP3Prefix, AnsiLowerCase(POP3Prefix.text));
end;

procedure TForm1.FTPListSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var obj : TPFObject;
    len : integer;
    pContent : PAnsiChar;
    pStream : TPFStream;
    streamIndex : integer;
    prefix : AnsiString;
begin
if (not Selected) then
  exit;

// Read an object from stream
serializeObject(false, int64(Item.data), m_pStgObject, obj);

if (not Assigned(obj)) then
  exit;

if ((obj.objectType = OT_FTP_DATA_INCOMING) or
    (obj.objectType = OT_FTP_DATA_OUTGOING) or
    (obj.objectType = OT_FTP_DATA_PART_INCOMING) or
    (obj.objectType = OT_FTP_DATA_PART_OUTGOING)) then
  begin
  streamIndex := 1;
  prefix := 'FTP data are in stream 0.' + #13#10 + 'Stream 1 contents:' + #13#10;
  end else
  begin
  streamIndex := 0;
  end;

pStream := obj[streamIndex];
if (Assigned(pStream)) then
  begin
  len := pStream.size();
  GetMem(pContent, len + 1);
  if (pContent <> nil) then
    begin
    pStream.seek(0, FILE_BEGIN);
    pStream.read(pContent, len);
    pContent[len] := #0;
    FTPContent.Text := prefix + pContent;
    FreeMem(pContent);
    end;
  end else
  begin
  FTPContent.Text := '';
  end;
obj.Free;
end;

procedure TForm1.ICQListSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var obj : TPFObject;
    textFormat : integer;
begin

if (not Selected) then
  exit;

ICQHeader.Strings.Clear;
ICQContent.Text := '';

// Read an object from stream
serializeObject(false, int64(Item.data), m_pStgObject, obj);

if (not Assigned(obj)) then
  exit;

try

ICQHeader.InsertRow('User', loadString(obj, ICQS_USER_UIN), true);
ICQHeader.InsertRow('Contact', loadString(obj, ICQS_CONTACT_UIN), true);

textFormat := loadInteger(obj, ICQS_TEXT_FORMAT);

case textFormat of
  ICQTF_ANSI:
   begin
   ICQContent.Text := loadString(obj, ICQS_TEXT);
   end;

  ICQTF_UTF8:
   begin
   ICQContent.Text := UTF8Decode(loadString(obj, ICQS_TEXT));
   end;

  ICQTF_UNICODE:
   begin
   ICQContent.Text := loadUnicodeString(obj, ICQS_TEXT);
   end;

  ICQTF_FILE_TRANSFER:
   begin
   ICQContent.Text := loadString(obj, ICQS_TEXT);
   end;
end;


finally
obj.Free;
end;

end;

procedure TForm1.ICQBlockUINChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_ICQBlockUIN, ICQBlockUIN.text);
end;

procedure TForm1.ICQBlockStringChange(Sender: TObject);
begin
FilterUnit.setStringParam(PT_ICQBlockString, ICQBlockString.text);
end;

procedure TForm1.ICQBlockFileTransfersClick(Sender: TObject);
begin
FilterUnit.setBoolParam(PT_ICQBlockFileTransfers, ICQBlockFileTransfers.Checked);
end;

procedure TForm1.XMPPListSelectItem(Sender: TObject; Item: TListItem;
  Selected: Boolean);
var obj : TPFObject;
    len : integer;
    pContent : PAnsiChar;
    pStream : TPFStream;
begin
if (not Selected) then
  exit;

// Read an object from stream
serializeObject(false, int64(Item.data), m_pStgObject, obj);

if (not Assigned(obj)) then
  exit;

pStream := obj[0];
if (Assigned(pStream)) then
  begin
  len := pStream.size();
  GetMem(pContent, len + 1);
  if (pContent <> nil) then
    begin
    pStream.seek(0, FILE_BEGIN);
    pStream.read(pContent, len);
    pContent[len] := #0;
    XMPPContent.Text := pContent;
    FreeMem(pContent);
    end;
  end else
  begin
  XMPPContent.Text := '';
  end;
obj.Free;
end;

end.
