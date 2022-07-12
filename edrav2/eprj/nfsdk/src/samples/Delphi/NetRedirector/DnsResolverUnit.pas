unit DnsResolverUnit;

interface

uses Windows, SyncObjs, WinSock, Classes, Contnrs, NetFilter2API;

type

  TDnsRequest = class (TObject)
  private
     m_id : ENDPOINT_ID;
     m_remoteAddress : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
     m_buf : PAnsiChar;
     m_len : integer;
     m_options : ^NF_UDP_OPTIONS;
  public
    constructor Create(id : ENDPOINT_ID;
        remoteAddress : PAnsiChar;
        buf : PAnsiChar;
        len : integer;
        options : pointer);
    destructor Destroy; override;
  end;


  TDnsResolver = class;

  TDnsResolverThread = class (TThread)
  public
    constructor Create(dnsResolver : TDnsResolver);
    destructor Destroy; override;
  protected
    procedure processRequest(request : TDnsRequest);
    procedure Execute; override;
  private
    m_dnsResolver : TDnsResolver;
    m_socket : integer;
  end;

  TDnsResolver = class
  private
    m_threads : TList;
    m_dnsRequestsQueue : TQueue;
    m_cs : TCriticalSection;
    m_requestEvent : TEvent;
    m_stopEvent : TEvent;
    m_redirectTo : array [0..NF_MAX_ADDRESS_LENGTH-1] of byte;
  public
    constructor Create(threadCount: integer; redirectTo : pointer);
    destructor Destroy; override;
    function getRedirectTo : pointer;
    procedure addRequest(request : TDnsRequest);
    function getNextRequest : TDnsRequest;
    function waitForRequests : boolean;
  end;

implementation

uses SysUtils;

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


constructor TDnsRequest.Create(id : ENDPOINT_ID;
        remoteAddress : PAnsiChar;
        buf : PAnsiChar;
        len : integer;
        options : pointer);
var pOptions : ^NF_UDP_OPTIONS;
begin
m_id := id;
copyArray(@m_remoteAddress, remoteAddress, NF_MAX_ADDRESS_LENGTH);
if (buf <> nil) then
  begin
  GetMem(m_buf, len);
  m_len := len;
  copyArray(m_buf, buf, len);
  end else
  begin
  m_buf := nil;
  m_len := 0;
  end;
if (options <> nil) then
  begin
  pOptions := options;
  GetMem(m_options, sizeof(NF_UDP_OPTIONS) + pOptions.optionsLength);
  copyArray(m_options, options, sizeof(NF_UDP_OPTIONS) + pOptions.optionsLength - 1);
  end else
  begin
  m_options := nil;
  end;
end;

destructor TDnsRequest.Destroy;
begin
if (m_buf <> nil) then
  FreeMem(m_buf);
if (m_options <> nil) then
  FreeMem(m_options);
inherited;
end;

constructor TDnsResolverThread.Create(dnsResolver : TDnsResolver);
begin
m_dnsResolver := dnsResolver;
m_socket := socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
inherited Create(false);
end;

destructor TDnsResolverThread.Destroy;
begin
closesocket(m_socket);
inherited;
end;

procedure TDnsResolverThread.processRequest(request : TDnsRequest);
var addr : sockaddr_in;
    fdr, fde : TFDSet;
    t : timeval;
    len : integer;
    packet : array [0..1024] of byte;
    fromlen : integer;
begin
copyArray(@addr, m_dnsResolver.getRedirectTo, sizeof(sockaddr_in));

if (request.m_len > sizeof(packet)) then
  exit;

copyArray(@packet, request.m_buf, request.m_len);

len := sendto(m_socket, packet, request.m_len, 0, addr, sizeof(sockaddr_in));
if (len <> SOCKET_ERROR) then
  begin
  FD_ZERO(fdr);
  FD_SET(m_socket, fdr);
  FD_ZERO(fde);
  FD_SET(m_socket, fde);

  t.tv_sec := 5;
  t.tv_usec := 0;

  len := select(1, @fdr, nil, @fde, @t);
  if (len <> SOCKET_ERROR) then
    begin
    if (FD_ISSET(m_socket, fdr)) then
      begin
      fromlen := sizeof(sockaddr_in);
      len := recvfrom(m_socket, packet, sizeof(packet), 0, addr, fromlen);
      if (len <> SOCKET_ERROR) then
        begin
        nf_udpPostReceive(request.m_id, @request.m_remoteAddress, @packet, len, request.m_options);
        end;
      end;
    end;
  end;

end;

procedure TDnsResolverThread.Execute;
var request : TDnsRequest;
begin
while m_dnsResolver.waitForRequests do
  begin
  repeat
    request := m_dnsResolver.getNextRequest;
    if (request <> nil) then
      begin
      processRequest(request);
      request.Free;
      end;
  until request = nil;
  end;
end;

constructor TDnsResolver.Create(threadCount: integer; redirectTo : pointer);
var i: integer;
    thread : TDnsResolverThread;
begin
m_cs := TCriticalSection.Create;
m_requestEvent := TEvent.Create(nil, false, false, '');
m_stopEvent := TEvent.Create(nil, true, false, '');
m_dnsRequestsQueue := TQueue.Create;
m_threads := TList.Create;
copyArray(@m_redirectTo, redirectTo, NF_MAX_ADDRESS_LENGTH);

for i:=1 to threadCount do
  begin
  thread := TDnsResolverThread.Create(self);
  m_threads.Add(thread);
  end;
end;

destructor TDnsResolver.Destroy;
var request : TDnsRequest;
    i: integer;
    thread : TDnsResolverThread;
begin
m_stopEvent.SetEvent;

for i:=0 to m_threads.Count-1 do
  begin
  thread := m_threads.Items[i];
  thread.WaitFor;
  thread.Free;
  end;

m_threads.Free;

while (m_dnsRequestsQueue.Count > 0) do
begin
request := m_dnsRequestsQueue.Pop;
if (request <> nil) then
  begin
  request.Free;
  end;
end;

m_dnsRequestsQueue.Free;
m_requestEvent.Free;
m_stopEvent.Free;
m_cs.Free;
inherited;
end;

function TDnsResolver.getRedirectTo : pointer;
begin
result := @m_redirectTo;
end;

procedure TDnsResolver.addRequest(request : TDnsRequest);
begin
m_cs.Enter;
m_dnsRequestsQueue.Push(request);
m_cs.Leave;

m_requestEvent.SetEvent;
end;

function TDnsResolver.getNextRequest : TDnsRequest;
begin
m_cs.Enter;
if m_dnsRequestsQueue.Count > 0 then
  begin
  result := m_dnsRequestsQueue.Pop;
  end else
  begin
  result := nil;
  end;
m_cs.Leave;
end;

function TDnsResolver.waitForRequests : boolean;
var handles : TWOHandleArray;
    res : Cardinal;
begin
handles[0] := m_requestEvent.Handle;
handles[1] := m_stopEvent.Handle;
res := WaitForMultipleObjects(2, @handles, false, INFINITE);
if (res = (WAIT_OBJECT_0 + 1)) then
  begin
  result := false;
  exit;
  end;
result := true;
end;



end.
