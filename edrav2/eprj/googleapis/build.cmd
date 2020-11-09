@echo off

set "root_dir=%~dp0"
set "root_dir=%root_dir:~0,-1%"

set "tools_dir=C:\Users\ermakov\apps\vcpkg\installed\x64-windows-static"
set "protoc_exe=%tools_dir%\tools\protobuf\protoc.exe"
set "grpc_cpp_plugin_exe=%tools_dir%\tools\grpc\grpc_cpp_plugin.exe"
set "out_dir=%root_dir%\src"
set "protobuf_include_dir=%root_dir%\..\protobuf\include"

setlocal EnableDelayedExpansion
for /L %%n in (1 1 500) do if "!__cd__:~%%n,1!" neq "" set /a "len=%%n+1"
setlocal DisableDelayedExpansion
for /r . %%g in (*.proto) do (
  set "absPath=%%g"
  setlocal EnableDelayedExpansion
  set "relPath=!absPath:~%len%!"
  call :protoc !relPath!
  endlocal
)
        	
exit /b 0

:protoc

echo(Compiling %~1

"%protoc_exe%" ^
 --proto_path=. ^
 --proto_path="%protobuf_include_dir%" ^
 --grpc_out="%out_dir%" ^
 --cpp_out="%out_dir%" ^
 --plugin=protoc-gen-grpc="%grpc_cpp_plugin_exe%" ^
 %1

@goto :eof
