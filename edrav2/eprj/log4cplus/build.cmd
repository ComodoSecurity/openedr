@echo off
@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"

set "cmd_dir=%~dp0"
set "cmd_dir=%cmd_dir:~0,-1%"
set "sln_file=%cmd_dir%\msvc14\log4cplus.sln"

call :build-x64-release
call :build-x64-debug
call :build-x86-release
call :build-x86-debug                 

exit /b %errorlevel%
 
:build-x64-release

devenv "%sln_file%" /project "log4cplusS" /build "Release|x64"
if %errorlevel% neq 0 exit /b %errorlevel%

set "build_dir=%cmd_dir%\msvc14\x64\bin.Release"
set "target_dir=%cmd_dir%\lib\win-Release-x64"

xcopy "%build_dir%\log4cplusS.lib" "%target_dir%\" /I /Y
xcopy "%build_dir%\log4cplusS.c.pdb" "%target_dir%\" /I /Y

exit /b 0

:build-x64-debug

devenv "%sln_file%" /project "log4cplusS" /build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%

set "build_dir=%cmd_dir%\msvc14\x64\bin.Debug"
set "target_dir=%cmd_dir%\lib\win-Debug-x64"

xcopy "%build_dir%\log4cplusSD.lib" "%target_dir%\" /I /Y
xcopy "%build_dir%\log4cplusSD.c.pdb" "%target_dir%\" /I /Y

exit /b 0

:build-x86-release

devenv "%sln_file%" /project "log4cplusS" /build "Release|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%

set "build_dir=%cmd_dir%\msvc14\Win32\bin.Release"
set "target_dir=%cmd_dir%\lib\win-Release-Win32"

xcopy "%build_dir%\log4cplusS.lib" "%target_dir%\" /I /Y
xcopy "%build_dir%\log4cplusS.c.pdb" "%target_dir%\" /I /Y

exit /b 0

:build-x86-debug

devenv "%sln_file%" /project "log4cplusS" /build "Debug|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%

set "build_dir=%cmd_dir%\msvc14\Win32\bin.Debug"
set "target_dir=%cmd_dir%\lib\win-Debug-Win32"

xcopy "%build_dir%\log4cplusSD.lib" "%target_dir%\" /I /Y
xcopy "%build_dir%\log4cplusSD.c.pdb" "%target_dir%\" /I /Y

exit /b 0
