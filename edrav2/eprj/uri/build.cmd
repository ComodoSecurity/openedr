@echo off
@call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\Tools\VsDevCmd.bat"

call :buildx64
call :buildx86

exit /b %errorlevel%
 
:buildx64

cmake -Bbuild-x64 ^
 -DUri_BUILD_TESTS=OFF ^
 -DUri_BUILD_DOCS=OFF ^
 -DBOOST_INCLUDEDIR=..\boost_1_69_0 ^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /EHsc /Zi /O2 /Ob2 /DEBUG:FULL /DNDEBUG /D_UNICODE /DUNICODE" ^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /EHsc /Zi /Ob0 /Od /DEBUG:FULL /RTC1 /D_UNICODE /DUNICODE" ^
 -DCMAKE_GENERATOR_PLATFORM=x64

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x64
devenv Uri.sln /build "Release|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy src\Release\network-uri.lib ..\lib\win-Release-x64\ /I /Y
xcopy src\network-uri.dir\Release\network-uri.pdb ..\lib\win-Release-x64\ /I /Y

devenv Uri.sln /build "Debug|x64"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy src\Debug\network-uri.lib ..\lib\win-Debug-x64\ /I /Y
xcopy src\Debug\network-uri.pdb ..\lib\win-Debug-x64\ /I /Y
cd ..

exit /b 0

:buildx86

cmake -Bbuild-x86 ^
 -DUri_BUILD_TESTS=OFF ^
 -DUri_BUILD_DOCS=OFF ^
 -DBOOST_INCLUDEDIR=..\boost_1_69_0 ^
 -DCMAKE_CXX_FLAGS_RELEASE:STRING="/MT /EHsc /Zi /O2 /Ob2 /DEBUG:FULL /DNDEBUG /D_UNICODE /DUNICODE" ^
 -DCMAKE_CXX_FLAGS_DEBUG:STRING="/MTd /EHsc /Zi /Ob0 /Od /DEBUG:FULL /RTC1 /D_UNICODE /DUNICODE" ^
 -DCMAKE_GENERATOR_PLATFORM=Win32

if %errorlevel% neq 0 exit /b %errorlevel%

cd build-x86
devenv Uri.sln /build "Release|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy src\Release\network-uri.lib ..\lib\win-Release-Win32\ /I /Y
xcopy src\network-uri.dir\Release\network-uri.pdb ..\lib\win-Release-Win32\ /I /Y

devenv Uri.sln /build "Debug|Win32"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy src\Debug\network-uri.lib ..\lib\win-Debug-Win32\ /I /Y
xcopy src\Debug\network-uri.pdb ..\lib\win-Debug-Win32\ /I /Y
cd ..

exit /b 0
