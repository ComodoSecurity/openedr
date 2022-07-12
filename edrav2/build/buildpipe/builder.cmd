@echo off
pushd %~dp0

set branch=
set forcemode=false
rem ========MSBuild settings======
set vs_ver=2019
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\%vs_ver%\Community\MSBuild\current\Bin\MSBuild.exe" (
	set msbuild="%ProgramFiles(x86)%\Microsoft Visual Studio\%vs_ver%\Community\MSBuild\current\Bin\MSBuild.exe"
) else set msbuild="%ProgramFiles(x86)%\Microsoft Visual Studio\%vs_ver%\Community\MSBuild\15.0\Bin\MSBuild.exe"
set vsdevcmd="%ProgramFiles(x86)%\Microsoft Visual Studio\%vs_ver%\Community\Common7\Tools\VsDevCmd.bat"
set CL=/Zm500
rem ========Project settings=======
set git_url=https://git.brk.nurd.com/common/edrav2.git
set sln="%~dp0_Build_\edrav2\build\vs%vs_ver%\edrav2.sln"
set inst_sln="%~dp0_Build_\edrav2\build\vs%vs_ver%\edrav2-install.sln"
set product=edrav2
rem ========SFTP settings============
set sftphost={sftp_host}
set sftpport={sftp_port}
set sftpuser={sftp_user}
set sftppwd={sftp_password}
rem ========Email settings============
set mailrecipients=-to:{mail_address1} -to:{mail_address2}
set mailserver={mail_server}
set mailsender={mail_sender}
set mailpwd={mail_password}
rem ========Utils settings============
set arc="%~dp0tools\7za.exe"
set mail="%~dp0tools\cmail.exe"
set sftp="%~dp0tools\psftp.exe"
set arcpwd=comodo

for %%I in (%*) do if /I "%%I" EQU "/force" (set forcemode=true) else (set branch=%%I)
call :checkstate || exit /b 1


if not exist "%~dp0_Build_" mkdir "%~dp0_Build_"
if not exist "%~dp0Logs" mkdir "%~dp0Logs"

call :sendmail "Build started"
echo.>"%~dp0.buildinprocess"
call :git_check || goto :nothingtobuild
call :git_download || ((call :error "Cannot download source files") & exit /b 1)

call :setbuildinfo
echo.Building %ver_full%

if "%VSINSTALLDIR%"=="" call %vsdevcmd%

REM Release
call :build %sln% Release || ((call :error "Compillation failed") & exit /b 1)
timeout /t 5 /NOBREAK
call :build %inst_sln% Release || ((call :error "Compillation failed") & exit /b 1)
timeout /t 5 /NOBREAK
call :pack Release || ((call :error "Cannot pack binaries into archive") & exit /b 1)

REM Debug
call :build %sln% Debug || ((call :error "Compillation failed") & exit /b 1)
timeout /t 5 /NOBREAK
call :build %inst_sln% Debug || ((call :error "Compillation failed") & exit /b 1)
timeout /t 5 /NOBREAK
call :pack Debug || ((call :error "Cannot pack binaries into archive") & exit /b 1)

call :unit_test

call :upload || ((call :error "Cannot upload files to SFTP") & exit /b 1)

call :finalyze
timeout /t 5 /NOBREAK

:nothingtobuild
call :cleanup

exit /b 0


:checkstate

if "%branch%"=="" (
  set /p branch=Input a branch name to build for: 
)

if "%branch%"=="" ((echo.[ERRO] Branch name is missed) & exit /b 1)
  
SETLOCAL EnableDelayedExpansion
if exist "%~dp0.error" (
  if not "%forcemode%"=="true" (
	CHOICE /T 10 /C yn /D n /m "Previous run was finished with error. Do you want to delete all data from previous run and proceed from scratch?"
    if !errorlevel! EQU 1 (call :cleanup) else exit /b 1
  ) else (call :cleanup)  
)
endlocal

if exist "%~dp0.buildinprocess" (
  echo.[ERRO] Build is already running
  exit /b 1
)

exit /b 0


:git_check
echo.Checking for changes from last build...
echo.Checking for changes from last build...  2>&1 >>"%~dp0Logs\script.log"
cd "%~dp0_Build_"

git init >"%~dp0Logs\git.log"
git clone -n -b %branch% %git_url% -- >>"%~dp0Logs\git.log" 2>&1 || exit /b 1
cd "%~dp0_Build_\%product%"
if not exist "%~dp0upload" mkdir "%~dp0upload"

for /F %%I in ('git rev-parse HEAD') do set gitsha1=%%I
echo.%gitsha1%>"%~dp0upload\git.sha1"
set gitshortsha1=%gitsha1:~0,8%

SETLOCAL EnableDelayedExpansion
set tag=

for /f %%I in ('git tag -l "b.*" --sort=-v:refname') do (
  if "!tag!"=="" set tag=%%I
)
if not "!tag!"=="" (set tag=%tag:b.=%)
endlocal & set lastbuild=%tag%
if not "%lastbuild%"=="" (set /a buildnum=%lastbuild%+1) else (set buildnum=0)

set alreadybuilded=FALSE
for /F %%I in ('git tag --points-at %gitsha1%') do set alreadybuilded=%%I

if NOT "%alreadybuilded%"=="FALSE" call :sendmail "Nothing to build" & exit /b 1

pushd %~dp0
exit /b 0

:git_download
echo.Downloading source files...
echo.Downloading source files...  2>&1 >>"%~dp0Logs\script.log"

cd "%~dp0_Build_\%product%"
git checkout %gitsha1% >>"%~dp0Logs\git.log" 2>&1 || exit /b 1

pushd %~dp0
exit /b 0


:setbuildinfo
echo.Configuring build info... 2>&1 >>"%~dp0Logs\script.log"

for /F "tokens=3" %%I in ('findstr /C:"#define CMD_VERSION_MAJOR" "%~dp0_Build_\edrav2\iprj\libcore\inc\version.h"') do SET ver_major=%%I
for /F "tokens=3" %%I in ('findstr /C:"#define CMD_VERSION_MINOR" "%~dp0_Build_\edrav2\iprj\libcore\inc\version.h"') do SET ver_minor=%%I
for /F "tokens=3" %%I in ('findstr /C:"#define CMD_VERSION_REVISION" "%~dp0_Build_\edrav2\iprj\libcore\inc\version.h"') do SET ver_rev=%%I
for /F "tokens=3" %%I in ('findstr /C:"#define CMD_VERSION_SUFFIX " "%~dp0_Build_\edrav2\iprj\libcore\inc\version.h"') do SET ver_suff=%%~I
set ver_short=%ver_major%.%ver_minor%.%ver_rev%.%buildnum%
if not "%ver_suff%"=="" (set ver_full=%ver_major%.%ver_minor%.%ver_rev%.%buildnum%-%ver_suff%) else set ver_full=%ver_major%.%ver_minor%.%ver_rev%.%buildnum%

for /F "useback delims=" %%I in (`powershell -command "& {get-date -uformat '%%Y.%%m.%%d %%T'}"`) do set buildtime=%%I
set extra_info=Branch: %branch% (%gitshortsha1%), build time: %buildtime%
set destdir=%branch%/%ver_short%
echo.#define CMD_BUILD_EXTRA "%extra_info%">"%~dp0_Build_\edrav2\iprj\libcore\inc\build_info.h"
echo.#define CMD_VERSION_BUILD %buildnum% >>"%~dp0_Build_\edrav2\iprj\libcore\inc\build_info.h"

set buildinfowxi="%~dp0_Build_\edrav2\iprj\installation\src\BuildInfo.wxi"
for /f %%i in ('powershell '{0:D12}' -f %buildnum%') do set hexbuildnum=%%i
echo.^<?xml version="1.0" encoding="utf-8"?^>>%buildinfowxi%
echo.^<Include^>>>%buildinfowxi%
echo. ^<?define strproductid = "{45CC556C-A03B-42FF-A2FE-%hexbuildnum%}" ?^>>>%buildinfowxi%
echo. ^<?define strversion = "%ver_short%" ?^>>>%buildinfowxi%
echo.^</Include^>>>%buildinfowxi%

pushd %~dp0
goto :eof


:build
SETLOCAL
set sln="%~1"
set type=%~2
rem set arch=%~3
for /f %%I in (%sln%) do set sln_name=%%~nxI

echo.Building %type%^(x86^) for %sln_name%...
echo.Building %type%^(x86^) for %sln_name%... 2>&1 >>"%~dp0Logs\script.log"
%msbuild% %sln% /t:Build /p:Configuration=%type% /p:Platform=x86 /m:2 /p:CL_MPCount=2 /noconlog /fl /flp:LogFile="%~dp0Logs\build.log";append /nologo || exit /b 1

echo.Building %type%^(x64^) for %sln_name%...
echo.Building %type%^(x64^) for %sln_name%... 2>&1 >>"%~dp0Logs\script.log"
%msbuild% %sln% /t:Build /p:Configuration=%type% /p:Platform=x64 /m:2 /p:CL_MPCount=2 /noconlog /fl /flp:LogFile="%~dp0Logs\build.log";append /nologo || exit /b 1

ENDLOCAL
exit /b 0


:unit_test
echo.Performing unit-testing:
echo.Performing unit-testing: 2>&1 >>"%~dp0Logs\script.log"
for /D %%I in ("%~dp0_Build_\edrav2\out\bin\*.*") do (
  for %%J in ("%%~I\tests\*.exe") do (
    echo.     %%~nI\%%~nJ...
    echo.     %%~nI\%%~nJ... 2>&1 >>"%~dp0Logs\script.log"
    if exist "%~dp0_Build_\edrav2\iprj\ats\scenarios\%%~nJ\data" pushd "%~dp0_Build_\edrav2\iprj\ats\scenarios\%%~nJ\data"
    "%%~J" --out="%~dp0Logs\%%~nJ_%%~nI.log" >nul 2>&1 || (
        echo. Failed unit-test: %%~nI\%%~nJ >>"%~dp0testserror.txt"
        if not exist "%~dp0Upload\failed_ut" mkdir "%~dp0Upload\failed_ut"
        copy /b "%~dp0Logs\%%~nJ_%%~nI.log" "%~dp0Upload\failed_ut\%%~nJ_%%~nI.log"
     )
  )
)
pushd %~dp0
for /R "%~dp0_Build_" %%I in (*.dmp) do echo.%%I>>"%~dp0testsdump.txt"
exit /b 0


:pack
SETLOCAL
set buildtype=%~1
echo.Performing archiving...
echo.Performing archiving... 2>&1 >>"%~dp0Logs\script.log"
for /D %%I in ("%~dp0_Build_\edrav2\out\bin\*%buildtype%*") do (
  %arc% a -ssw -r -tzip -x!signpkg -x!unsigned -x!tests -x!pdb -x!*.ilk -x!*.cer "%~dp0Upload\%product%-%ver_full%-%%~nI-bin.zip" "%%~I\*.*" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
  if exist "%%~I\signpkg" %arc% a -ssw -r- -t7z -mhe -p%arcpwd% "%~dp0Upload\%product%-%ver_full%-%%~nI-signpkg.7z" "%%~I\signpkg\*.*" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
  %arc% a -ssw -r- -tzip "%~dp0Upload\%product%-%ver_full%-%%~nI-pdb.zip" "%%~I\pdb\*.*" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
  %arc% a -ssw -r -tzip -x!*.ilk "%~dp0Upload\%product%-%ver_full%-%%~nI-tests.zip" "%%~I\tests\*.*" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
)
if exist "%~dp0_Build_\edrav2\out\install" (
  for /D %%I in ("%~dp0_Build_\edrav2\out\install\*%buildtype%*") do (
    for %%J in ("%%~I\*.msi") do copy /b "%%~J" "%~dp0Upload\%product%-%ver_full%-%%~nI-%%~nxJ" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
    %arc% a -ssw -r- -tzip "%~dp0Upload\%product%-%ver_full%-%%~nI-pdb.zip" "%%~I\*.*pdb" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
  )
)
if exist "%~dp0_Build_\edrav2\out\upgrade" (
  for /D %%I in ("%~dp0_Build_\edrav2\out\upgrade\*%buildtype%*") do (
    for %%J in ("%%~I\*.msi") do copy /b "%%~J" "%~dp0Upload\%product%-%ver_full%-%%~nI-%%~nxJ" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
    %arc% a -ssw -r- -tzip "%~dp0Upload\%product%-%ver_full%-%%~nI-pdb.zip" "%%~I\*.*pdb" >>"%~dp0Logs\7zip.log" 2>&1|| exit /b 1
  )
)
if exist "%~dp0testsdump.txt" %arc% a -ssw "%~dp0Upload\failed_ut\dumps" @"%~dp0testsdump.txt"
ENDLOCAL
exit /b 0


:upload
echo.Uploading files to storage...
echo.Uploading files to storage... 2>&1 >>"%~dp0Logs\script.log"

%arc% a -ssw -r -tzip "%~dp0logs" "%~dp0logs\*.*" >nul 2>&1 && copy /b "%~dp0logs.zip" "%~dp0Upload\logs.zip" >nul 2>&1 

if exist "%~dp0sftp.batch" del /q "%~dp0sftp.batch"

echo.mkdir repository/%product%/%branch%>"%~dp0sftp.batch"
%sftp% -P %sftpport% -l %sftpuser% -pw %sftppwd% -batch -bc -be -b "%~dp0sftp.batch" %sftphost% >>"%~dp0Logs\sftp.log" 2>&1

if exist "%~dp0testserror.txt" set destdir=%destdir%_utfailed

echo.put -r -- "%~dp0Upload" "repository/%product%/%destdir%">"%~dp0sftp.batch"
echo.quit>>"%~dp0sftp.batch"
%sftp% -P %sftpport% -l %sftpuser% -pw %sftppwd% -batch -bc -b "%~dp0sftp.batch" %sftphost% >"%~dp0Logs\sftp.log" 2>&1 || exit /b 1
exit /b 0


:error
SETLOCAL EnableDelayedExpansion
set errmessage=%~1
cd %~dp0

echo.>"%~dp0.error"
echo.[ERRO]  %errmessage%
echo.[ERRO]  %errmessage% 2>&1 >>"%~dp0Logs\script.log"
echo.[ERRO]  %errmessage% >"%~dp0mailbody.txt"


if "%errmessage%"=="Compillation failed" (
  powershell -command "& {get-content '%~dp0logs\build.log' | select-object -skip (Select-String 'Build FAILED.' '%~dp0logs\build.log' | Select-Object -ExpandProperty LineNumber)}">>"%~dp0mailbody.txt"
)

%arc% a -ssw -r -tzip "%~dp0logs" "%~dp0logs\*.*" >nul 2>&1
call :sendmail "Build failed"
ENDLOCAL
goto :eof

:finalyze
if exist "%~dp0testserror.txt" (
   echo.[ERRO]  Some unit-test is failed>>"%~dp0mailbody.txt"
   type "%~dp0testserror.txt">>"%~dp0mailbody.txt"
   echo.>>"%~dp0mailbody.txt"
   echo.>>"%~dp0mailbody.txt"
)

cd "%~dp0_Build_\%product%"
git tag b.%buildnum% %gitsha1% >>"%~dp0Logs\git.log" 2>&1
git push origin --tags >>"%~dp0Logs\git.log" 2>&1 || echo.[ERRO] Cannot set tag to build>>"%~dp0mailbody.txt"
pushd %~dp0

echo.SFTP: repository/%product%/%destdir%>>"%~dp0mailbody.txt"
for %%I in ("%~dp0upload\*.*") do echo.     %%~nxI>>"%~dp0mailbody.txt"

%arc% u -ssw -r -tzip "%~dp0logs" "%~dp0logs\*.*" >nul 2>&1
call :sendmail "Build finished"
goto :eof


:cleanup
echo.Cleaning up...
cd %~dp0
del /q /f "%~dp0*.txt" >nul 2>&1
del /q /f "%~dp0.error" >nul 2>&1
del /q /f "%~dp0sftp.batch" >nul 2>&1
del /q /f "%~dp0logs.zip" >nul 2>&1
rmdir /q /s "%~dp0_Build_" >nul 2>&1
rmdir /q /s "%~dp0Logs" >nul 2>&1
rmdir /q /s "%~dp0Upload" >nul 2>&1
del /q /f "%~dp0.buildinprocess"

goto :eof

:sendmail
SETLOCAL
set subject=%~1 for branch '%branch%'
echo.Sending mail "%subject%"...
echo.Sending mail "%subject%"... 2>&1 >>"%~dp0Logs\script.log"
if exist "%~dp0mailbody.txt" (set body=-body-file:"%~dp0mailbody.txt") else set body=
if exist "%~dp0logs.zip" (set attach=-a:"%~dp0logs.zip") else set attach=

%mail% -starttls -host:%mailsender%:%mailpwd%@%mailserver% -from:"%mailsender%:EDRBuilder" %mailrecipients% -subject:"%subject%" %body% %attach%
ENDLOCAL
goto :eof