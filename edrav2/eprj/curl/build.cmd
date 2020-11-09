@echo off
@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"

call :buildx64
call :buildx86

exit /b %errorlevel%
 
:buildx64

cmake -Bbuild-x64 ^
 -DHTTP_ONLY=YES ^
 -DENABLE_IPV6=NO ^
 -DBUILD_TESTING=NO ^
 -DBUILD_CURL_EXE=NO ^
 -DCURL_STATIC_CRT=YES ^
 -DBUILD_SHARED_LIBS=NO ^
 -DCURL_STATICLIB=YES ^
 -DCMAKE_C_FLAGS_RELEASE:STRING="/MT /Zi /O2 /Ob2 /DNDEBUG /D_UNICODE /DUNICODE /DCURL_STATICLIB" ^
 -DCMAKE_C_FLAGS_DEBUG:STRING="/MTd /Zi /Ob0 /Od /RTC1 /D_UNICODE /DUNICODE /DCURL_STATICLIB" ^
 -DCMAKE_GENERATOR_PLATFORM=x64

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x64
devenv CURL.sln /build "Release|x64"
xcopy lib\Release\libcurl.lib ..\out\win-Release-x64\ /I /Y
xcopy lib\libcurl.dir\Release\libcurl.pdb ..\out\win-Release-x64\ /I /Y

devenv CURL.sln /build "Debug|x64"
xcopy lib\Debug\libcurl-d.lib ..\out\win-Debug-x64\ /I /Y
xcopy lib\libcurl.dir\Debug\libcurl.pdb ..\out\win-Debug-x64\ /I /Y
cd ..

exit /b 0

:buildx86

cmake -Bbuild-x86 ^
 -DHTTP_ONLY=YES ^
 -DENABLE_IPV6=NO ^
 -DBUILD_TESTING=NO ^
 -DBUILD_CURL_EXE=NO ^
 -DCURL_STATIC_CRT=YES ^
 -DBUILD_SHARED_LIBS=NO ^
 -DCURL_STATICLIB=YES ^
 -DCMAKE_C_FLAGS_RELEASE:STRING="/MT /Zi /O2 /Ob2 /DNDEBUG /D_UNICODE /DUNICODE /DCURL_STATICLIB" ^
 -DCMAKE_C_FLAGS_DEBUG:STRING="/MTd /Zi /Ob0 /Od /RTC1 /D_UNICODE /DUNICODE /DCURL_STATICLIB"

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x86
devenv CURL.sln /build "Release|Win32"
xcopy lib\Release\libcurl.lib ..\out\win-Release-Win32\ /I /Y
xcopy lib\libcurl.dir\Release\libcurl.pdb ..\out\win-Release-Win32\ /I /Y

devenv CURL.sln /build "Debug|Win32"
xcopy lib\Debug\libcurl-d.lib ..\out\win-Debug-Win32\ /I /Y
xcopy lib\libcurl.dir\Debug\libcurl.pdb ..\out\win-Debug-Win32\ /I /Y
cd ..

exit /b 0
