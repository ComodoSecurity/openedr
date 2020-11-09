::
:: Boost build script
::   Usage:
::     - set path to vcvarsall.bat for your Visual Studio
::     - run this script
::     - wait and watch build_cmd.log
::     - after finish the lib subdirectory will contains *.lib and  *.pdb files
@echo off

:: Execute from MSVC environment (Visual studio 2019 is current value)
set "vcvarsall=%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
set toolset_name=msvc-14.2

:: Define build path
set "BoostRootDir=%~dp0"
set "BoostRootDir=%BoostRootDir:~0,-1%"
set "BoostBuildTmpDir=%BoostRootDir%\tmp"
::set "OPENSSL_INCLUDE_DIR=%BoostRootDir%\..\openssl\openssl\include"

:: Declare b2 commandlines
::set LibrariesRestrictions= --with-chrono
set LibrariesRestrictions= --without-serialization --without-log --without-graph --without-mpi --without-python --without-graph --without-graph_parallel --without-math --without-wave
set BuildBoost=b2 stage toolset=%toolset_name% link=static debug-symbols=on debug-store=database --build-dir="%BoostBuildTmpDir%" --stagedir="%BoostRootDir%" --hash -a -d0 %LibrariesRestrictions% 
set BuildBoostMthreadRltshared=%BuildBoost% threading=multi runtime-link=shared
set BuildBoostMthreadRltstatic=%BuildBoost% threading=multi runtime-link=static

:: Call StartBuild with output to build_cmd.log
call :StartBuild > build_cmd.log
exit

:: Build boost

:StartBuild

echo Start Build
echo LibrariesRestrictions: %LibrariesRestrictions%

echo ----------------------------------------------------------
echo build b2 tool
echo ----------------------------------------------------------

call bootstrap.bat
@echo off

echo ----------------------------------------------------------
echo Build x86
echo ----------------------------------------------------------

setlocal
call "%vcvarsall%" x86
%BuildBoostMthreadRltstatic% address-model=32 variant=debug
%BuildBoostMthreadRltshared% address-model=32 variant=debug
%BuildBoostMthreadRltstatic% address-model=32 variant=release
%BuildBoostMthreadRltshared% address-model=32 variant=release
endlocal

echo ----------------------------------------------------------
echo Build x64
echo ----------------------------------------------------------

setlocal
call "%vcvarsall%" x64
::set "OPENSSL_LIB_DIR=%BoostRootDir%\..\openssl\lib\win-Debug-x64"
%BuildBoostMthreadRltstatic% address-model=64 variant=debug
%BuildBoostMthreadRltshared% address-model=64 variant=debug
::set "OPENSSL_LIB_DIR=%BoostRootDir%\..\openssl\lib\win-Release-x64"
%BuildBoostMthreadRltstatic% address-model=64 variant=release
%BuildBoostMthreadRltshared% address-model=64 variant=release
endlocal

:CopyPdb

echo ----------------------------------------------------------
echo Copying PDB
echo ----------------------------------------------------------

:: Copy pdb from "out" directory into "lib" directory.
:: Unfortunately, boost build do not do it itself.

set "PdbSrcDirectory=%BoostBuildTmpDir%\boost\bin.v2\libs"
for /r "%PdbSrcDirectory%" %%f in (*.pdb) do xcopy "%%f" "%BoostRootDir%\lib\" /y /exclude:%BoostRootDir%\excluded_pdb_list.txt

echo ----------------------------------------------------------
echo Build is complete
echo ----------------------------------------------------------

exit /b 0
