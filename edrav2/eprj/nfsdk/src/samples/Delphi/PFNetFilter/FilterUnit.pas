unit FilterUnit;

interface

uses ProtocolFiltersAPI, NetFilter2API;

const
   PT_htmlStopWord = 1;
   PT_urlStopWord = 2;
   PT_blockPage = 3;

   PT_filterSSL = 4;
   PT_filterRaw = 5;
   PT_firewall = 6;

   PT_SMTPBlackAddress = 7;
   PT_POP3Prefix = 8;

   PT_ICQBlockUIN = 9;
   PT_ICQBlockString = 10;
   PT_ICQBlockFileTransfers = 11;

type
   UI_STAT_VALUE = class (TObject)
   public
        proc : AnsiString;
        bytesIn : Int64;
        bytesOut : Int64;
   end;

function startFilter() : boolean;
procedure stopFilter;

function readHttpStatus(obj : TPFObject) : AnsiString;
function getHttpUrl(obj : TPFObject) : string;
function loadString(obj : TPFObject; streamIndex : integer) : AnsiString;
function loadUnicodeString(obj : TPFObject; streamIndex : integer) : WideString;
function loadInteger(obj : TPFObject; streamIndex : integer) : integer;

procedure setStringParam(paramType : integer; value : AnsiString);
function getStringParam(paramType : integer) : AnsiString;
procedure setBoolParam(paramType : integer; value : boolean);
function getBoolParam(paramType : integer) : boolean;

implementation

uses Windows, Messages, SysUtils, MainUnit, Classes, WinSock, SyncObjs,
  NetworkEventUnit;

type
   STAT_VALUE = class (TObject)
   public
        bytesIn : Int64;
        bytesOut : Int64;
   end;

  FIREWALL_RULE = record
     protocol : integer;
     direction : integer;
     processName : array [0..MAX_PATH] of AnsiChar;
     port : Word;
     filteringFlag : Longword;
  end;


var
        eh : PFEvents_c;
        err : integer;

        // Statistics
        idToProcessMap : TStringList;
        statMap : TStringList;

        // Proxy handlers
        proxyEventHandler : NF_EventHandler;

        // Filtering parameters
        htmlStopWord : AnsiString;
        urlStopWord : AnsiString;
        blockPage : AnsiString;
        filterSSL : boolean;
        filterRaw : boolean;
        firewall : boolean;
        SMTPBlackAddress : AnsiString;
        POP3Prefix : AnsiString;
        ICQBlockUIN : AnsiString;
        ICQBlockString : AnsiString;
        ICQBlockFileTransfers : boolean;

        paramCS : TCriticalSection;

        // Firewall rules
        firewallRules : TList;

{$I-}

procedure setStringParam(paramType : integer; value : AnsiString);
begin
paramCS.Enter;
case paramType of
   PT_htmlStopWord:
        htmlStopWord := value;
   PT_urlStopWord:
        urlStopWord := value;
   PT_blockPage:
        blockPage := value;
   PT_SMTPBlackAddress:
        SMTPBlackAddress := value;
   PT_POP3Prefix:
        POP3Prefix := value;
   PT_ICQBlockUIN:
        ICQBlockUIN := value;
   PT_ICQBlockString:
        ICQBlockString := value;
end;
paramCS.Leave;
end;

procedure setBoolParam(paramType : integer; value : boolean);
begin
paramCS.Enter;
case paramType of
   PT_filterSSL:
        filterSSL := value;
   PT_filterRaw:
        filterRaw := value;
   PT_firewall:
        firewall := value;
   PT_ICQBlockFileTransfers:
        ICQBlockFileTransfers := value;
end;
paramCS.Leave;
end;

function getStringParam(paramType : integer) : AnsiString;
begin
paramCS.Enter;
case paramType of
   PT_htmlStopWord:
        Result := htmlStopWord;
   PT_urlStopWord:
        Result := urlStopWord;
   PT_blockPage:
        Result := blockPage;
   PT_SMTPBlackAddress:
        Result := SMTPBlackAddress;
   PT_POP3Prefix:
        Result := POP3Prefix;
   PT_ICQBlockUIN:
        Result := ICQBlockUIN;
   PT_ICQBlockString:
        Result := ICQBlockString;
end;
paramCS.Leave;
end;

function getBoolParam(paramType : integer) : boolean;
begin
Result:=false;
paramCS.Enter;
case paramType of
   PT_filterSSL:
        Result := filterSSL;
   PT_filterRaw:
        Result := filterRaw;
   PT_firewall:
        Result := firewall;
   PT_ICQBlockFileTransfers:
        Result := ICQBlockFileTransfers;
end;
paramCS.Leave;
end;

function postFormMsg(et : FE_EventType; id : ENDPOINT_ID; blocked : boolean; pData : pointer) : LongBool;
var ev : ^FE_EVENT;
begin
GetMem(ev, sizeof(FE_EVENT));
ev.id := id;
ev.et := et;
ev.blocked := blocked;
ev.pData := pData;
Result := PostMessage(Form1.Handle, WM_FILTEREVENT, 0, Integer(ev));
end;

function sendFormMsg(et : FE_EventType; id : ENDPOINT_ID; blocked : boolean; pData : pointer) : integer;
var ev : FE_EVENT;
begin
ev.id := id;
ev.et := et;
ev.blocked := blocked;
ev.pData := pData;
Result := SendMessage(Form1.Handle, WM_FILTEREVENT, 0, Integer(@ev));
end;

procedure initStatMaps;
begin
idToProcessMap := TStringList.Create;
statMap := TStringList.Create;
end;

procedure freeStatMaps;
var i : integer;
begin
if assigned(idToProcessMap) then
  begin
  idToProcessMap.Free;
  idToProcessMap := nil;
  end;

if assigned(statMap) then
  begin
  for i:=0 to statMap.Count-1 do
    begin
    if (statMap.Objects[i] <> nil) then
      begin
      statMap.Objects[i].Free;
      end;
    end;
  statMap.Free;
  statMap := nil;
  end;

end;

procedure addIdToProcessEntry(protocol : integer; id : ENDPOINT_ID; s : AnsiString);
var ids : AnsiString;
begin
ids := IntToStr(protocol) + ':' + IntToStr(id);
if (idToProcessMap.IndexOfName(ids) <> -1) then
  exit;
idToProcessMap.Values[ids] := s;
end;

procedure removeIdToProcessEntry(protocol : integer; id : ENDPOINT_ID);
var ids : AnsiString;
    index : integer;
begin
ids := IntToStr(protocol) + ':' + IntToStr(id);
index := idToProcessMap.IndexOfName(ids);
if (index <> -1) then
  idToProcessMap.Delete(index);
end;

procedure updateStatistics(protocol : integer; id : ENDPOINT_ID; bytesIn, bytesOut : Int64);
var ids : AnsiString;
    index : integer;
    s : AnsiString;
    pStat : STAT_VALUE;
    pStatUI : UI_STAT_VALUE;
begin
ids := IntToStr(protocol) + ':' + IntToStr(id);
index := idToProcessMap.IndexOfName(ids);
if (index <> -1) then
  begin
  s := idToProcessMap.Values[ids];
  index := statMap.IndexOf(s);
  if (index = -1) then
    begin
    pStat := STAT_VALUE.Create;
    pStat.bytesIn := bytesIn;
    pStat.bytesOut := bytesOut;
    statMap.AddObject(s, pStat);
    end else
    begin
    pStat := STAT_VALUE(statMap.Objects[index]);
    pStat.bytesIn := pStat.bytesIn + bytesIn;
    pStat.bytesOut := pStat.bytesOut + bytesOut;
    end;

  pStatUI := UI_STAT_VALUE.Create;
  pStatUI.proc := s;
  pStatUI.bytesIn := pStat.bytesIn;
  pStatUI.bytesOut := pStat.bytesOut;

  postFormMsg(FE_statUpdated, id, false, pStatUI);

  end;
end;

function getProcName(var pConnInfo : NF_TCP_CONN_INFO) : string;
var processName : array [0..MAX_PATH] of AnsiChar;
    p : PAnsiChar;
    pAddr : ^sockaddr_in;
begin
processName[0] := #0;
if (not nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
  StrCopy(processName, 'System');
p := StrRScan(processName, '\');
if (p <> nil) then
  p := p + 1
  else
  p := processName;
if (pConnInfo.direction = NF_D_IN) then
  begin
  pAddr := @pConnInfo.localAddress;
  FmtStr(Result, '[TCP-IN] %s:%d', [p, ntohs(pAddr.sin_port)]);
  end else
  begin
  pAddr := @pConnInfo.remoteAddress;
  FmtStr(Result, '[TCP-OUT] %s:%d', [p, ntohs(pAddr.sin_port)]);
  end;
end;

function getFirewallAction(protocol : integer;
                        direction : integer;
                        const pConnInfo : NF_TCP_CONN_INFO;
                        var filteringFlag : Longword)
                        : boolean;
var i : integer;
    pRule : ^FIREWALL_RULE;
    processName : array [0..MAX_PATH] of AnsiChar;
    pAddr : ^sockaddr_in;
begin

filteringFlag := NF_ALLOW or NF_FILTER;
Result := false;

processName[0] := #0;
if (not nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
  StrCopy(processName, 'System');

if (direction = NF_D_IN) then
  pAddr := @pConnInfo.localAddress
  else
  pAddr := @pConnInfo.remoteAddress;

for i:=0 to firewallRules.Count-1 do
  begin
  pRule := firewallRules.Items[i];
  if (pRule.protocol <> protocol) then
     continue;
  if (pRule.direction <> direction) then
     continue;
  if (pRule.processName <> AnsiString(processName)) then
     continue;
  if (pRule.port <> 0) and (pRule.port <> pAddr.sin_port) then
     continue;

  filteringFlag := pRule.filteringFlag;
  Result := true;
  exit;
  end;

end;

procedure addFirewallRule(protocol : integer;
                        direction : integer;
                        const pConnInfo : NF_TCP_CONN_INFO;
                        usePort : boolean;
                        filteringFlag : Longword);
var pRule : ^FIREWALL_RULE;
    processName : array [0..MAX_PATH] of AnsiChar;
    pAddr : ^sockaddr_in;
begin
GetMem(pRule, sizeof(FIREWALL_RULE));
if (not assigned(pRule)) then
  exit;

processName[0] := #0;
if (not nf_getProcessNameA(pConnInfo.processId, @processName, sizeof(processName))) then
  StrCopy(processName, 'System');

if (direction = NF_D_IN) then
  pAddr := @pConnInfo.localAddress
  else
  pAddr := @pConnInfo.remoteAddress;

pRule.protocol := protocol;
pRule.direction := direction;
StrCopy(pRule.processName, processName);
if (usePort) then
  pRule.port := pAddr.sin_port
  else
  pRule.port := 0;
pRule.filteringFlag := filteringFlag;

firewallRules.Add(pRule);

end;

function filterTCPRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO) : Longword;
var option : integer;
    filteringFlag : Longword;
begin
if (not getBoolParam(PT_firewall)) then
  begin
  Result := NF_ALLOW or NF_FILTER;
  exit;
  end;

if (pConnInfo.direction = NF_D_OUT) then
  begin
  if (getFirewallAction(IPPROTO_TCP, NF_D_OUT, pConnInfo, filteringFlag)) then
    begin
    if (filteringFlag <> NF_BLOCK) then
     pConnInfo.filteringFlag := filteringFlag or NF_FILTER
     else
     pConnInfo.filteringFlag := filteringFlag;
    end else
    begin
    option := sendFormMsg(FE_tcpConnectRequest, id, false, @pConnInfo);
    case option of
      FO_ONCE:
        ;
      FO_PROCESS_PORT:
        addFirewallRule(IPPROTO_TCP, NF_D_OUT, pConnInfo, true, pConnInfo.filteringFlag);
      FO_PROCESS:
        addFirewallRule(IPPROTO_TCP, NF_D_OUT, pConnInfo, false, pConnInfo.filteringFlag);
      end;
    end;
  end else
  begin
  if (getFirewallAction(IPPROTO_TCP, NF_D_IN, pConnInfo, filteringFlag)) then
    begin
    if (filteringFlag = NF_BLOCK) then
      nf_tcpClose(id);
    end else
    begin
    option := sendFormMsg(FE_tcpConnectRequest, id, false, @pConnInfo);
    case option of
      FO_ONCE:
        ;
      FO_PROCESS_PORT:
        addFirewallRule(IPPROTO_TCP, NF_D_IN, pConnInfo, true, pConnInfo.filteringFlag);
      FO_PROCESS:
        addFirewallRule(IPPROTO_TCP, NF_D_IN, pConnInfo, false, pConnInfo.filteringFlag);
      end;
    if (pConnInfo.filteringFlag = NF_BLOCK) then
      nf_tcpClose(id);
    end;
  end;

Result := pConnInfo.filteringFlag;
end;


procedure threadStart(); cdecl;
begin
end;

procedure threadEnd(); cdecl;
begin
end;

procedure tcpConnectRequest(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
begin
filterTCPRequest(id, pConnInfo);
end;

procedure tcpConnected(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
var p : ^NF_TCP_CONN_INFO;
    sslFlags : integer;
begin
if (pConnInfo.direction = NF_D_IN) then
  begin
  if ((filterTCPRequest(id, pConnInfo) and NF_BLOCK) > 0) then
    exit;
  end;

// Add required filters
pfc_addFilter(id, FT_PROXY, FF_DEFAULT, OT_LAST, FT_NONE);

if getBoolParam(PT_filterSSL) then
  begin
  pfc_addFilter(id, FT_SSL, FF_DEFAULT, OT_LAST, FT_NONE);
  sslFlags := FF_SSL_TLS
  end else
  begin
  sslFlags := FF_DEFAULT;
  end;

pfc_addFilter(id, FT_POP3, sslFlags, OT_LAST, FT_NONE);

pfc_addFilter(id, FT_SMTP, sslFlags, OT_LAST, FT_NONE);

pfc_addFilter(id, FT_HTTP, FF_HTTP_BLOCK_SPDY, OT_LAST, FT_NONE);

pfc_addFilter(id, FT_NNTP, sslFlags, OT_LAST, FT_NONE);

pfc_addFilter(id, FT_FTP,
        sslFlags or FF_READ_ONLY_IN or FF_READ_ONLY_OUT,
        OT_LAST, FT_NONE);

pfc_addFilter(id, FT_ICQ, FF_DEFAULT, OT_LAST, FT_NONE);

pfc_addFilter(id, FT_XMPP, sslFlags or FF_READ_ONLY_IN or FF_READ_ONLY_OUT, OT_LAST, FT_NONE);

if getBoolParam(PT_filterRaw) then
  pfc_addFilter(id, FT_RAW, FF_DEFAULT, OT_LAST, FT_NONE);

// Notify UI
try
GetMem(p, sizeof(NF_TCP_CONN_INFO));
p^ := pConnInfo;
postFormMsg(FE_tcpConnected, id, false, p);
finally
end;

// Update statistics map
addIdToProcessEntry(IPPROTO_TCP, id, getProcName(pConnInfo));

end;

procedure tcpClosed(id : ENDPOINT_ID; var pConnInfo : NF_TCP_CONN_INFO); cdecl;
var p : ^NF_TCP_CONN_INFO;
begin
// Notify UI
try
GetMem(p, sizeof(NF_TCP_CONN_INFO));
p^ := pConnInfo;
postFormMsg(FE_tcpClosed, id, false, p);
finally
end;

// Update statistics map
removeIdToProcessEntry(IPPROTO_TCP, id);

end;

procedure udpCreated(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
var p : ^NF_UDP_CONN_INFO;
begin
// Notify UI
try
GetMem(p, sizeof(NF_UDP_CONN_INFO));
p^ := pConnInfo;
postFormMsg(FE_udpCreated, id, false, p);
finally
end;

end;

procedure udpConnectRequest(id : ENDPOINT_ID; var pConnReq : NF_UDP_CONN_REQUEST); cdecl;
begin
end;

procedure udpClosed(id : ENDPOINT_ID; var pConnInfo : NF_UDP_CONN_INFO); cdecl;
var p : ^NF_UDP_CONN_INFO;
begin
// Notify UI
try
GetMem(p, sizeof(NF_UDP_CONN_INFO));
p^ := pConnInfo;
postFormMsg(FE_udpClosed, id, false, p);
finally
end;

end;

function loadString(obj : TPFObject; streamIndex : integer) : AnsiString;
var p : PAnsiChar;
    pStream : TPFStream;
    len : integer;
begin
pStream := obj[streamIndex];
if (not Assigned(pStream)) then
  begin
  Result := '';
  exit;
  end;
len := pStream.size;
GetMem(p, len+1);
if (not Assigned(p)) then
  begin
  Result := '';
  exit;
  end;
pStream.seek(0, FILE_BEGIN);
pStream.read(p, len);
p[len] := #0;
Result := p;
FreeMem(p);
end;

function loadUnicodeString(obj : TPFObject; streamIndex : integer) : WideString;
var p : PWideChar;
    pStream : TPFStream;
    len : integer;
begin
pStream := obj[streamIndex];
if (not Assigned(pStream)) then
  begin
  Result := '';
  exit;
  end;
len := pStream.size;
GetMem(p, len+2);
if (not Assigned(p)) then
  begin
  Result := '';
  exit;
  end;
pStream.seek(0, FILE_BEGIN);
pStream.read(PAnsiChar(p), len);
p[len div 2] := #0;
Result := p;
FreeMem(p);
end;

function loadInteger(obj : TPFObject; streamIndex : integer) : integer;
var p : integer;
    pStream : TPFStream;
    len : integer;
begin
pStream := obj[streamIndex];
if (not Assigned(pStream)) then
  begin
  Result := 0;
  exit;
  end;
len := pStream.size;
if (len < 4) then
  begin
  Result := 0;
  exit;
  end;
pStream.seek(0, FILE_BEGIN);
pStream.read(@p, len);
Result := p;
end;


function readHttpStatus(obj : TPFObject) : AnsiString;
var p : PAnsiChar;
    pStream : TPFStream;
    len : integer;
begin
pStream := obj[HS_STATUS];
if (not Assigned(pStream)) then
  begin
  Result := '';
  exit;
  end;
len := pStream.size;
GetMem(p, len+1);
if (not Assigned(p)) then
  begin
  Result := '';
  exit;
  end;
pStream.seek(0, FILE_BEGIN);
pStream.read(p, len);
p[len] := #0;
Result := p;
FreeMem(p);
end;

function writeHttpStatus(obj : TPFObject; status : AnsiString) : boolean;
var pStream : TPFStream;
begin
pStream := obj[HS_STATUS];
if (not Assigned(pStream)) then
  begin
  Result := false;
  exit;
  end;
status := status + #13#10;
pStream.reset();
pStream.write(PAnsiChar(status), length(status));
Result := true;
end;

function getHttpUrl(obj : TPFObject) : string;
var ph : TPFHeader;
    host, url, status:AnsiString;
    i, j : integer;
begin
if (obj.objectType = OT_HTTP_REQUEST) then
  begin
  // HTTP request

  // Get HTTP header
  ph := TPFHeader.Create();
  if (Assigned(ph)) then
    begin
    if (ph.readFromStream(obj[HS_HEADER])) then
      begin
      host := ph.getFieldValue('host');
      end;
    ph.Free;
    end;

  // Get HTTP status header
  status := readHttpStatus(obj);
  if (length(status) <> 0) then
   begin
   // Extract URL
   i := AnsiPos(' ', status);
   if (i <> 0) then
     begin
     for j:=i+1 to length(status) do
       begin
       if (status[j] = ' ') then
         break;
       url := url + status[j];
       end;
     end;
    end;

  if (Pos('http', url) <> 1) then
    url := host + url;
  end else
if (obj.objectType = OT_HTTP_RESPONSE) then
  begin
  // HTTP response

  // Get HTTP header
  ph := TPFHeader.Create();
  if (Assigned(ph)) then
    begin
    if (ph.readFromStream(obj[HS_HEADER])) then
      begin
      // Get host from custom header
      host := ph.getFieldValue(HTTP_EXHDR_RESPONSE_HOST);
      // Get request status from custom header
      status := ph.getFieldValue(HTTP_EXHDR_RESPONSE_REQUEST);
      if (length(status) <> 0) then
        begin
        // Extract URL
        i := AnsiPos(' ', status);
        if (i <> 0) then
          begin
          for j:=i+1 to length(status) do
            begin
            if (status[j] = ' ') then
              break;
            url := url + status[j];
            end;
          end;

        if (Pos('http', url) <> 1) then
          url := host + url;
        end;
      end;
    ph.Free;
    end;
 end;

Result := url;

end;

function postBlockHttpResponse(id : ENDPOINT_ID) : boolean;
var ph:TPFHeader;
    newObject:TPFObject;
    pStream : TPFStream;
    bPage : AnsiString;
begin
 bPage := getStringParam(PT_blockPage);

 // Create a response with specified HTML
 newObject := TPFObject.Create(OT_HTTP_RESPONSE, 3);

 // Set status
 writeHttpStatus(newObject, 'HTTP/1.1 404 Not OK');

 // Assign header
 ph := TPFHeader.Create();
 ph.addField('Content-Type', 'text/html', true);
 ph.addField('Content-Length', IntToStr(length(bPage)), true);
 ph.addField('Connection', 'close', true);
 ph.writeToStream(newObject[HS_HEADER]);
 ph.Free;

 // Add new content
 pStream := newObject[HS_CONTENT];
 if (not Assigned(pStream)) then
    begin
    newObject.Free;
    Result := false;
    exit;
    end;

 pStream.reset();
 pStream.write(PAnsiChar(bPage), length(bPage));

 newObject.post(id);

 newObject.Free;

 Result := true;
end;

function filterHttpRequest(id : ENDPOINT_ID; obj : TPFObject) : boolean;
var url:AnsiString;
    stopWord : AnsiString;
begin
stopWord := getStringParam(PT_urlStopWord);

url := getHttpUrl(obj);

if (length(stopWord) = 0) then
 begin
 Result := false;
 exit;
 end;

if (AnsiPos(stopWord, url) <> 0) then
 begin
 postBlockHttpResponse(id);
 Result := true;
 end else
 begin
 Result := false;
 end;

end;

function filterHttpResponse(id : ENDPOINT_ID; obj:TPFObject) : boolean;
var pStream : TPFStream;
    pContent : PAnsiChar;
    len : integer;
    stopWord : AnsiString;
    ph : TPFHeader;
    s : string;
begin
Result := false;

stopWord := getStringParam(PT_htmlStopWord);
if (length(stopWord) = 0) then
 exit;

// Filter only HTML pages
ph := TPFHeader.Create();
if (Assigned(ph)) then
  begin
  if (ph.readFromStream(obj[HS_HEADER])) then
    begin
    s := ph.getFieldValue('Content-Type');
    if (Length(s) > 0) then
      begin
      if (Pos('text/html', s) = 0) then
        begin
        ph.Free;
        exit;
        end;
      end;
    end;
  ph.Free;
  end;


pStream := obj[HS_CONTENT];
if (not Assigned(pStream)) then
  exit;

len := pStream.size;
GetMem(pContent, len + 1);
if (pContent <> nil) then
  begin
  pStream.seek(0, FILE_BEGIN);
  pStream.read(pContent, len);
  pContent[len] := #0;
  StrLower(pContent);
  if (AnsiPos(stopWord, pContent) <> 0) then
    begin
    postBlockHttpResponse(id);
    Result := true;
    end else
    begin
    Result := false;
    end;
  FreeMem(pContent);
  end;
end;

function filterOutgoingMail(id : ENDPOINT_ID; obj:TPFObject) : boolean;
var blackAddr : AnsiString;
    ph : TPFHeader;
    newObject : TPFObject;
    s : AnsiString;
begin
Result := false;

blackAddr := getStringParam(PT_SMTPBlackAddress);
if (length(blackAddr) = 0) then
 exit;

ph := TPFHeader.Create();
if (not assigned(ph)) then
  exit;

try
if (ph.readFromStream(obj[0])) then
  begin
  s := ph.getFieldValue('to');
  if (Length(s) = 0) then
     s := ph.getFieldValue('newsgroups');

  if (Length(s) > 0) then
    begin
    if (Pos(blackAddr, LowerCase(s)) <> 0) then
      begin
      // Create a blocking response
      newObject := TPFObject.Create(OT_RAW_INCOMING, 1);
      s := '554 Message blocked!' + #13#10;
      newObject[0].write(PAnsiChar(s), Length(s));
      newObject.post(id);
      newObject.Free;
      Result := true;
      end;
    end;
  end;
finally
ph.Free;
end;

end;

function filterIncomingMail(id : ENDPOINT_ID; obj : TPFObject) : boolean;
var pStream : TPFStream;
    prefix : AnsiString;
    ph : TPFHeader;
    s : AnsiString;
    p : PAnsiChar;
    pContent : PAnsiChar;
    len : integer;
begin
Result := false;

prefix := getStringParam(PT_POP3Prefix);
if (length(prefix) = 0) then
 exit;

pStream := obj[0];
if (not Assigned(pStream)) then
  exit;

len := pStream.size;
GetMem(pContent, len + 1);
if (pContent = nil) then
  exit;

pStream.seek(0, FILE_BEGIN);
pStream.read(pContent, len);
pContent[len] := #0;

p := StrPos(pContent, #13#10#13#10);
if (p = nil) then
  begin
  FreeMem(pContent);
  exit;
  end;

ph := TPFHeader.Create(pContent, len);
if (not assigned(ph)) then
  begin
  FreeMem(pContent);
  exit;
  end;

try
s := ph.getFieldValue('Subject');
s := POP3Prefix + s;

ph.removeFields('Subject', true);
ph.addField('Subject', s, false);

// Update object
pStream.reset();
ph.writeToStream(pStream);
p := p + 4;
pStream.write(p, len - (p - pContent));
finally
ph.Free;
FreeMem(pContent);
end;

end;

procedure blockICQMessage(id : ENDPOINT_ID; obj : TPFObject);
var newObject:TPFObject;
    pStream : TPFStream;
    pContent : PAnsiChar;
    len : integer;
begin

 if (obj.objectType = OT_ICQ_CHAT_MESSAGE_OUTGOING) then
   begin
   newObject := TPFObject.Create(OT_ICQ_REQUEST, 1);
   end else
 if (obj.objectType = OT_ICQ_CHAT_MESSAGE_INCOMING) then
   begin
   newObject := TPFObject.Create(OT_ICQ_RESPONSE, 1);
   end else
   begin
   exit;
   end;

 // Update content

 pStream := obj[ICQS_RAW];
 if (not Assigned(pStream)) then
    begin
    newObject.Free;
    exit;
    end;

 len := pStream.size;
 GetMem(pContent, len);
 if ((pContent = nil) or (len < 27)) then
   begin
   newObject.Free;
   exit;
   end;
 pStream.seek(0, FILE_BEGIN);
 pStream.read(pContent, len);

 pContent[26] := #0;

 pStream := newObject[ICQS_RAW];
 pStream.reset();
 pStream.write(pContent, len);

 newObject.post(id);

 newObject.Free;
end;


function filterICQMessage(id : ENDPOINT_ID; obj : TPFObject) : boolean;
var blockUIN, blockString : AnsiString;
    blockFileTransfer : boolean;
    ws : WideString;
    textFormat : integer;
begin
blockUIN := getStringParam(PT_ICQBlockUIN);
if (length(blockUIN) <> 0) then
  begin
  if (blockUIN = loadString(obj, ICQS_CONTACT_UIN)) then
    begin
    Result := true;
    exit;
    end;
  end;

textFormat := loadInteger(obj, ICQS_TEXT_FORMAT);

blockString := LowerCase(getStringParam(PT_ICQBlockString));
if (length(blockString) <> 0) then
  begin
  case textFormat of
    ICQTF_ANSI:
        begin
        ws := loadString(obj, ICQS_TEXT);
        end;

    ICQTF_UTF8:
        begin
        ws := UTF8Decode(loadString(obj, ICQS_TEXT));
        end;

    ICQTF_UNICODE:
        begin
        ws := loadUnicodeString(obj, ICQS_TEXT);
        end;
  end;

  ws := LowerCase(ws);

  if (Pos(blockString, ws) <> 0) then
    begin
    Result := true;
    exit;
    end;

  end;

blockFileTransfer := getBoolParam(PT_ICQBlockFileTransfers);
if (blockFileTransfer) then
  begin
  if (textFormat = ICQTF_FILE_TRANSFER) then
    begin
    Result := true;
    exit;
    end;
  end;

Result := false;
end;


procedure dataAvailable(id : ENDPOINT_ID; pObject : PPFObject_c); cdecl;
var obj : TPFObject;
    blocked : boolean;
begin
// Filter data
blocked := false;

obj := TPFObject.Create(pObject);

if (not obj.readOnly) then
begin
case obj.objectType of
  OT_HTTP_REQUEST:
    begin
    blocked := filterHttpRequest(id, obj);
    end;
  OT_HTTP_RESPONSE:
    begin
    blocked := filterHttpResponse(id, obj);
    end;
  OT_SMTP_MAIL_OUTGOING:
    begin
    blocked := filterOutgoingMail(id, obj);
    end;
  OT_POP3_MAIL_INCOMING:
    begin
    filterIncomingMail(id, obj);
    end;
  OT_NNTP_POST:
    begin
    filterOutgoingMail(id, obj);
    end;
  OT_NNTP_ARTICLE:
    begin
    filterIncomingMail(id, obj);
    end;
  OT_ICQ_CHAT_MESSAGE_OUTGOING:
    begin
    blocked := filterICQMessage(id, obj);
    if (blocked) then
        blockICQMessage(id, obj);
    end;
  OT_ICQ_CHAT_MESSAGE_INCOMING:
    begin
    blocked := filterICQMessage(id, obj);
    if (blocked) then
        blockICQMessage(id, obj);
    end;
end;

if (not blocked) then
  pfc_postObject(id, pObject);
end;

// Notify UI
try
postFormMsg(FE_dataAvailable, id, blocked, obj.detach());
finally
obj.Free;
end;

end;

function dataPartAvailable(id : ENDPOINT_ID; pObject : PPFObject_c) : PF_DATA_PART_CHECK_RESULT; cdecl;
var obj : TPFObject;
    ph : TPFHeader;
    s : AnsiString;
begin
obj := TPFObject.Create(pObject);
try
if (obj.objectType = OT_HTTP_RESPONSE) then
  begin
  // Switch to read-only mode if there is no need to filter HTML
  if (Length(getStringParam(PT_htmlStopWord)) = 0) then
    begin
    Result := DPCR_FILTER_READ_ONLY;
    exit;
    end;
  // Filter only HTML pages
  ph := TPFHeader.Create();
  if (Assigned(ph)) then
    begin
    if (ph.readFromStream(obj[HS_HEADER])) then
      begin
      s := ph.getFieldValue('Content-Type');
      if (Length(s) > 0) then
        begin
        if (Pos('text/html', s) <> 0) then
          begin
          ph.Free;
          Result := DPCR_FILTER;
          exit;
          end;
        end;
      end;
    ph.Free;
    end;
  end else
if (obj.objectType = OT_HTTP_REQUEST) then
  begin
  // Check request URI for large requests
  if (filterHttpRequest(id, obj)) then
    begin
    Result := DPCR_BLOCK;
    exit;
    end;
  end;
finally
obj.Free;
end;

Result := DPCR_FILTER_READ_ONLY;
end;

function tcpPostSend(id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl;
begin
Result := nf_tcpPostSend(id, buf, len);
if (pfc_canDisableFiltering(id) <> 0) then
  begin
  nf_tcpDisableFiltering(id);
  end;
updateStatistics(IPPROTO_TCP, id, 0, len);
end;

function tcpPostReceive(id : ENDPOINT_ID; buf : PAnsiChar; len : Longword): integer; cdecl;
begin
Result := nf_tcpPostReceive(id, buf, len);
if (pfc_canDisableFiltering(id) <> 0) then
  begin
  nf_tcpDisableFiltering(id);
  end;
end;

function tcpSetConnectionState(id : ENDPOINT_ID; suspended : integer): integer; cdecl;
begin
Result := nf_tcpSetConnectionState(id, suspended);
end;

function udpPostSend(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : Longword; options : pointer): integer; cdecl;
begin
Result := nf_udpPostSend(id, remoteAddress, buf, len, options);
updateStatistics(IPPROTO_UDP, id, 0, len);
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
updateStatistics(IPPROTO_TCP, id, len, 0);
end;

procedure udpReceive(id : ENDPOINT_ID; remoteAddress : PAnsiChar; buf : PAnsiChar; len : integer; options : pointer); cdecl;
begin
pfc_getNFEventHandler().udpReceive(id, remoteAddress, buf, len, options);
updateStatistics(IPPROTO_UDP, id, len, 0);
end;

function tcpDisableFiltering(id : ENDPOINT_ID) : integer; cdecl;
begin
Result := nf_tcpDisableFiltering(id);
end;

function udpDisableFiltering(id : ENDPOINT_ID) : integer; cdecl;
begin
Result := nf_udpDisableFiltering(id);
end;

//////////////////////////////////////////////////////////
// Main routines

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

  pfc_setExceptionsTimeout(EXC_GENERIC, 60 * 60);
  pfc_setExceptionsTimeout(EXC_TLS, 60 * 60);
  pfc_setExceptionsTimeout(EXC_CERT_REVOKED, 60 * 60);

  pfc_setRootSSLCertSubject('Sample CA');

  // Use proxy handler to hook tcpReceive and udpReceive events
  proxyEventHandler := pfc_getNFEventHandler()^;
  proxyEventHandler.tcpReceive := tcpReceive;
  proxyEventHandler.udpReceive := udpReceive;

  // Initialize NFAPI
  err := nf_init('netfilter2', proxyEventHandler);
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
  rule.filteringFlag := NF_FILTER or NF_INDICATE_CONNECT_REQUESTS;
  rule.protocol := IPPROTO_TCP;
//  rule.direction := NF_D_OUT;
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


  initStatMaps;

  Result := true;
end;

procedure stopFilter;
var i : integer;
    pRule : ^FIREWALL_RULE;
begin
  pfc_free();
  nf_free();
  freeStatMaps;

  if (assigned(firewallRules)) then
    begin
    for i:=0 to firewallRules.Count-1 do
      begin
      pRule := firewallRules.Items[i];
      FreeMem(pRule);
      end;
    firewallRules.Clear;
    end;
end;

initialization
  begin
  paramCS := TCriticalSection.Create;
  firewallRules := TList.Create;
  end;

finalization
  begin
  paramCS.Free;
  if (assigned(firewallRules)) then
    begin
    firewallRules.Free;
    end;
  end;
end.
