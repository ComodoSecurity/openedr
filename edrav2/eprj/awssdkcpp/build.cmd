@echo off
@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"

call :buildx64
call :buildx86

exit /b %errorlevel%
 
:buildx64

cmake -Bbuild-x64 ^
 -G "Visual Studio 15 2017 Win64" -T "host=x64" ^
 -DBUILD_ONLY="firehose" ^
 -DBUILD_SHARED_LIBS=OFF ^
 -DFORCE_SHARED_CRT=OFF ^
 -DENABLE_TESTING=OFF ^
 -DCPP_STANDARD=17 ^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /Zi /O2 /Ob2 /DNDEBUG /D_UNICODE /DUNICODE" ^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Zi /Ob0 /Od /RTC1 /D_UNICODE /DUNICODE"

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x64
devenv AWSSDK.sln /build "Release|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy aws-cpp-sdk-core\Release\aws-cpp-sdk-core.lib ..\lib\win-Release-x64\ /I /Y
xcopy aws-cpp-sdk-core\aws-cpp-sdk-core.dir\Release\aws-cpp-sdk-core.pdb ..\lib\win-Release-x64\ /I /Y
xcopy aws-cpp-sdk-firehose\Release\aws-cpp-sdk-firehose.lib ..\lib\win-Release-x64\ /I /Y
xcopy aws-cpp-sdk-firehose\aws-cpp-sdk-firehose.dir\Release\aws-cpp-sdk-firehose.pdb ..\lib\win-Release-x64\ /I /Y

devenv AWSSDK.sln /build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy aws-cpp-sdk-core\Debug\aws-cpp-sdk-core.lib ..\lib\win-Debug-x64\ /I /Y
xcopy aws-cpp-sdk-core\Debug\aws-cpp-sdk-core.pdb ..\lib\win-Debug-x64\ /I /Y
xcopy aws-cpp-sdk-firehose\Debug\aws-cpp-sdk-firehose.lib ..\lib\win-Debug-x64\ /I /Y
xcopy aws-cpp-sdk-firehose\Debug\aws-cpp-sdk-firehose.pdb ..\lib\win-Debug-x64\ /I /Y
cd ..

exit /b 0

:buildx86

cmake -Bbuild-x86 ^
 -T "host=x64" ^
 -DBUILD_ONLY="firehose" ^
 -DBUILD_SHARED_LIBS=OFF ^
 -DFORCE_SHARED_CRT=OFF ^
 -DENABLE_TESTING=OFF ^
 -DCPP_STANDARD=17 ^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /Zi /O2 /Ob2 /DNDEBUG /D_UNICODE /DUNICODE" ^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /Zi /Ob0 /Od /RTC1 /D_UNICODE /DUNICODE"

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x86
devenv AWSSDK.sln /build "Release|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy aws-cpp-sdk-core\Release\aws-cpp-sdk-core.lib ..\lib\win-Release-Win32\ /I /Y
xcopy aws-cpp-sdk-core\aws-cpp-sdk-core.dir\Release\aws-cpp-sdk-core.pdb ..\lib\win-Release-Win32\ /I /Y
xcopy aws-cpp-sdk-firehose\Release\aws-cpp-sdk-firehose.lib ..\lib\win-Release-Win32\ /I /Y
xcopy aws-cpp-sdk-firehose\aws-cpp-sdk-firehose.dir\Release\aws-cpp-sdk-firehose.pdb ..\lib\win-Release-Win32\ /I /Y

devenv AWSSDK.sln /build "Debug|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy aws-cpp-sdk-core\Debug\aws-cpp-sdk-core.lib ..\lib\win-Debug-Win32\ /I /Y
xcopy aws-cpp-sdk-core\Debug\aws-cpp-sdk-core.pdb ..\lib\win-Debug-Win32\ /I /Y
xcopy aws-cpp-sdk-firehose\Debug\aws-cpp-sdk-firehose.lib ..\lib\win-Debug-Win32\ /I /Y
xcopy aws-cpp-sdk-firehose\Debug\aws-cpp-sdk-firehose.pdb ..\lib\win-Debug-Win32\ /I /Y

cd ..

exit /b 0
