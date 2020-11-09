@rem Create documentation for project
@set selfDir=%~dp0
@if not "%~1"=="" set ADSROOTPATH=%~1
@if not defined ADSROOTPATH echo ERROR: Path to ADS folder MUST BE passed to script as first argument or defined as an environment variable "ADSROOTPATH"&exit /b 1
@if not exist "%ADSROOTPATH%\makedoc.cmd" echo ERROR: "%ADSROOTPATH%\makedoc.cmd" is not found&exit /b 1
@set prjAdsCfgFld="%selfDir%"
@set prjAdsOutFld="%selfDir%..\..\out\ads"
@set prjSrcFld="%selfDir%..\.."
@set commonAdsCfgFld="%selfDir%..\..\..\..\build\ads"

%ADSROOTPATH%\makedoc.cmd %prjAdsCfgFld% %prjAdsOutFld% %prjSrcFld% %commonAdsCfgFld%


