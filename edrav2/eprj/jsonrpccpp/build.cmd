@echo off
::call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"

@set vcvarsall="%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
@set cmake="%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -G "Visual Studio 16 2019"

::%cmake% --help

call :buildx64
call :buildx86

exit /b %errorlevel%
 
:buildx64
call %vcvarsall% x64
setlocal

%cmake% -Bbuild-x64 ^
 -DCOMPILE_TESTS=NO ^
 -DCOMPILE_STUBGEN=NO ^
 -DCOMPILE_EXAMPLES=NO ^
 -DREDIS_SERVER=NO ^
 -DREDIS_CLIENT=NO ^
 -DFILE_DESCRIPTOR_SERVER=NO ^
 -DFILE_DESCRIPTOR_CLIENT=NO ^
 -DTCP_SOCKET_CLIENT=YES ^
 -DTCP_SOCKET_SERVER=NO ^
 -DHTTP_CLIENT=YES ^
 -DHTTP_SERVER=YES ^
 -DJSONCPP_INCLUDE_DIR=../jsoncpp/include ^
 -DJSONCPP_LIBRARY=../jsoncpp/build/x64/Release/json_lib.lib ^
 -DJSONCPP_LIBRARY_DEBUG=../jsoncpp/build/x64/Debug/json_lib.lib ^
 -DMHD_INCLUDE_DIR=../libmicrohttpd/include ^
 -DMHD_LIBRARY=../libmicrohttpd/x86_64/VS2017/Release-static/libmicrohttpd.lib ^
 -DMHD_LIBRARY_DEBUG=../libmicrohttpd/x86_64/VS2017/Debug-static/libmicrohttpd_d.lib ^
 -DCURL_INCLUDE_DIR=../curl/include ^
 -DCURL_LIBRARY=../curl/out/win-Release-x64/libcurl.lib ^
 -DCURL_LIBRARY_DEBUG=../curl/out/win-Debug-x64/libcurl-d.lib ^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /Zi /O2 /Ob2 /DNDEBUG /D_UNICODE /DUNICODE /DCURL_STATICLIB" ^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Zi /Ob0 /Od /RTC1 /D_UNICODE /DUNICODE /DCURL_STATICLIB" ^
 -DCMAKE_GENERATOR_PLATFORM=x64

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x64
devenv libjson-rpc-cpp.sln /build "Release|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy lib\Release\jsonrpccpp-client.lib ..\lib\win-Release-x64\ /I /Y
xcopy lib\Release\jsonrpccpp-common.lib ..\lib\win-Release-x64\ /I /Y
xcopy lib\Release\jsonrpccpp-server.lib ..\lib\win-Release-x64\ /I /Y
xcopy src\jsonrpccpp\client.dir\Release\client.pdb ..\lib\win-Release-x64\ /I /Y
xcopy src\jsonrpccpp\common.dir\Release\common.pdb ..\lib\win-Release-x64\ /I /Y
xcopy src\jsonrpccpp\server.dir\Release\server.pdb ..\lib\win-Release-x64\ /I /Y

devenv libjson-rpc-cpp.sln /build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy lib\Debug\jsonrpccpp-client.lib ..\lib\win-Debug-x64\ /I /Y
xcopy lib\Debug\jsonrpccpp-common.lib ..\lib\win-Debug-x64\ /I /Y
xcopy lib\Debug\jsonrpccpp-server.lib ..\lib\win-Debug-x64\ /I /Y
xcopy src\jsonrpccpp\client.dir\Debug\client.pdb ..\lib\win-Debug-x64\ /I /Y
xcopy src\jsonrpccpp\common.dir\Debug\common.pdb ..\lib\win-Debug-x64\ /I /Y
xcopy src\jsonrpccpp\server.dir\Debug\server.pdb ..\lib\win-Debug-x64\ /I /Y
cd ..

endlocal

exit /b 0

:buildx86
call %vcvarsall% x64_x86
setlocal

%cmake% -Bbuild-x86 -A Win32 ^
 -DCOMPILE_TESTS=NO ^
 -DCOMPILE_STUBGEN=NO ^
 -DCOMPILE_EXAMPLES=NO ^
 -DREDIS_SERVER=NO ^
 -DREDIS_CLIENT=NO ^
 -DFILE_DESCRIPTOR_SERVER=NO ^
 -DFILE_DESCRIPTOR_CLIENT=NO ^
 -DTCP_SOCKET_CLIENT=YES ^
 -DTCP_SOCKET_SERVER=NO ^
 -DHTTP_CLIENT=YES ^
 -DHTTP_SERVER=YES ^
 -DJSONCPP_INCLUDE_DIR=../jsoncpp/include ^
 -DJSONCPP_LIBRARY=../jsoncpp/build/x86/Release/json_lib.lib ^
 -DJSONCPP_LIBRARY_DEBUG=../jsoncpp/build/x86/Debug/json_lib.lib ^
 -DMHD_INCLUDE_DIR=../libmicrohttpd/include ^
 -DMHD_LIBRARY=../libmicrohttpd/x86/VS2017/Release-static/libmicrohttpd.lib ^
 -DMHD_LIBRARY_DEBUG=../libmicrohttpd/x86/VS2017/Debug-static/libmicrohttpd_d.lib ^
 -DCURL_INCLUDE_DIR=../curl/include ^
 -DCURL_LIBRARY=../curl/out/win-Release-Win32/libcurl.lib ^
 -DCURL_LIBRARY_DEBUG=../curl/out/win-Debug-Win32/libcurl-d.lib ^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /Zi /O2 /Ob2 /DNDEBUG /D_UNICODE /DUNICODE /DCURL_STATICLIB" ^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Zi /Ob0 /Od /RTC1 /D_UNICODE /DUNICODE /DCURL_STATICLIB"

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x86
devenv libjson-rpc-cpp.sln /build "Release|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy lib\Release\jsonrpccpp-client.lib ..\lib\win-Release-Win32\ /I /Y
xcopy lib\Release\jsonrpccpp-common.lib ..\lib\win-Release-Win32\ /I /Y
xcopy lib\Release\jsonrpccpp-server.lib ..\lib\win-Release-Win32\ /I /Y
xcopy src\jsonrpccpp\client.dir\Release\client.pdb ..\lib\win-Release-Win32\ /I /Y
xcopy src\jsonrpccpp\common.dir\Release\common.pdb ..\lib\win-Release-Win32\ /I /Y
xcopy src\jsonrpccpp\server.dir\Release\server.pdb ..\lib\win-Release-Win32\ /I /Y

devenv libjson-rpc-cpp.sln /build "Debug|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy lib\Debug\jsonrpccpp-client.lib ..\lib\win-Debug-Win32\ /I /Y
xcopy lib\Debug\jsonrpccpp-common.lib ..\lib\win-Debug-Win32\ /I /Y
xcopy lib\Debug\jsonrpccpp-server.lib ..\lib\win-Debug-Win32\ /I /Y
xcopy src\jsonrpccpp\client.dir\Debug\client.pdb ..\lib\win-Debug-Win32\ /I /Y
xcopy src\jsonrpccpp\common.dir\Debug\common.pdb ..\lib\win-Debug-Win32\ /I /Y
xcopy src\jsonrpccpp\server.dir\Debug\server.pdb ..\lib\win-Debug-Win32\ /I /Y
cd ..

endlocal

exit /b 0
