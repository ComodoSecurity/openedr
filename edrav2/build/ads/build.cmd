@rem Create documentation for edrav2
set selfDir=%~dp0
@if not "%~1"=="" set ADSROOTPATH=%~1
@if not defined ADSROOTPATH echo ERROR: Path to ADS folder MUST BE passed to script as first argument or defined as an environment variable "ADSROOTPATH"&exit /b 1
@if not exist "%ADSROOTPATH%\makedoc.cmd" echo ERROR: "%ADSROOTPATH%\makedoc.cmd" is not found&exit /b 1

@FOR /R %selfDir%..\..\iprj %%I IN (.) DO @if exist %%I\build\ads\build.cmd call %%I\build\ads\build.cmd %ADSROOTPATH%


