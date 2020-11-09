
REM Make changes to environment local
setlocal

SET RMDIR=rmdir /S /Q
SET DEL=del /Q

%RMDIR% CMakeFiles
%RMDIR% debug
%RMDIR% log4cplus.dir
%RMDIR% loggingserver.dir
%RMDIR% minsizerel
%RMDIR% release
%RMDIR% relwithdebinfo
%RMDIR% ZERO_CHECK.dir

%DEL% cmake_install.cmake
%DEL% CMakeCache.txt

REM Clean up changes to environment.
endlocal
