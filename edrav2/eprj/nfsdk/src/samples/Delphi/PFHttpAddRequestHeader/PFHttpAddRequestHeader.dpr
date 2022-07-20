program PFHttpAddRequestHeader;

uses
  Forms,
  AddHeaderUnit in 'AddHeaderUnit.pas' {Form1},
  NetFilter2API in '..\include\NetFilter2API.pas',
  ProtocolFiltersAPI in '..\include\ProtocolFiltersAPI.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
