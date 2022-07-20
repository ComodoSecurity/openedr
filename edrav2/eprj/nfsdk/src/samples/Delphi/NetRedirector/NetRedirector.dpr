program NetRedirector;

uses
  Forms,
  WinSock,
  MainFormUnit in 'MainFormUnit.pas' {MainForm},
  NetFilter2API in '..\include\NetFilter2API.pas',
  DnsResolverUnit in 'DnsResolverUnit.pas';

{$R *.res}

var wd : WSAData;

begin
  WSAStartup(2 * 256 + 2 , wd);

  Application.Initialize;
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;

  WSACleanup();
end.
