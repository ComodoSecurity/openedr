@echo off

rem Set here the path to WDK
set _ddkpath_=D:\WINDDK\7600.16385.1

for /f "tokens=1 delims=:" %%a in ('cd') do set _srcdrv_=%%a
for /f "tokens=1 delims=" %%a in ('cd') do set _srcpath_=%%a

call %_ddkpath_%\bin\setenv.bat %_ddkpath_% %1 %2 %4

%_srcdrv_%:
cd "%_srcpath_%"

build.exe %3
rem prefast build.exe %3
rem prefast view

