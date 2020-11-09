@echo off

set configure=threads no-shared no-asm no-engine no-tests
set "root_dir=%~dp0"
set "root_dir=%root_dir:~0,-1%"
set "work_dir=%root_dir%\openssl"

pushd "%work_dir%"

call :buildx64-debug
call :buildx64-release
call :buildx86-debug
call :buildx86-release

popd

exit /b %errorlevel%
 
:buildx64-debug

setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

set "prefix=%root_dir%\build-x64\Debug"
set "openssl_dir=%prefix%\ssl"
nmake clean
perl Configure VC-WIN64A %configure% --debug --prefix=%prefix% --openssldir=%openssl_dir%
nmake install_sw

xcopy "%prefix%\lib\libcrypto.lib" "%root_dir%\lib\win-Debug-x64\" /I /Y
xcopy "%prefix%\lib\libssl.lib" "%root_dir%\lib\win-Debug-x64\" /I /Y
xcopy "%prefix%\lib\ossl_static.pdb" "%root_dir%\lib\win-Debug-x64\" /I /Y
endlocal
exit /b 0

:buildx64-release

setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

set "prefix=%root_dir%\build-x64\Release"
set "openssl_dir=%prefix%\ssl"
nmake clean
perl Configure VC-WIN64A %configure% --prefix=%prefix% --openssldir=%openssl_dir%
nmake install_sw

xcopy "%prefix%\lib\libcrypto.lib" "%root_dir%\lib\win-Release-x64\" /I /Y
xcopy "%prefix%\lib\libssl.lib" "%root_dir%\lib\win-Release-x64\" /I /Y
xcopy "%prefix%\lib\ossl_static.pdb" "%root_dir%\lib\win-Release-x64\" /I /Y
endlocal
exit /b 0

:buildx86-debug

setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"

set "prefix=%root_dir%\build-x86\Debug"
set "openssl_dir=%prefix%\ssl"
nmake clean
perl Configure VC-WIN32 %configure% --debug --prefix=%prefix% --openssldir=%openssl_dir%
nmake install_sw

xcopy "%prefix%\lib\libcrypto.lib" "%root_dir%\lib\win-Debug-Win32\" /I /Y
xcopy "%prefix%\lib\libssl.lib" "%root_dir%\lib\win-Debug-Win32\" /I /Y
xcopy "%prefix%\lib\ossl_static.pdb" "%root_dir%\lib\win-Debug-Win32\" /I /Y
endlocal
exit /b 0

:buildx86-release

setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"

set "prefix=%root_dir%\build-x86\Release"
set "openssl_dir=%prefix%\ssl"
nmake clean
perl Configure VC-WIN32 %configure% --prefix=%prefix% --openssldir=%openssl_dir%
nmake install_sw

xcopy "%prefix%\lib\libcrypto.lib" "%root_dir%\lib\win-Release-Win32\" /I /Y
xcopy "%prefix%\lib\libssl.lib" "%root_dir%\lib\win-Release-Win32\" /I /Y
xcopy "%prefix%\lib\ossl_static.pdb" "%root_dir%\lib\win-Release-Win32\" /I /Y
endlocal
exit /b 0
