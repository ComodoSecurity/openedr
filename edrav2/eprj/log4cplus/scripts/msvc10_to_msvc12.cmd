@echo on
setlocal
set "THIS_SCRIPT_FILE=%~nx0"

if "%1" == "" (
  echo.
  echo USAGE: %THIS_SCRIPT_FILE% log4cplus.sln
  echo.
  exit /b 1
)

if not defined VS120COMNTOOLS (
  echo.
  echo VS120COMNTOOLS environment variable is not defined
  echo.
  exit /b 2
)

set "SLN_PATH=%~f1"
set "SLN_BASE_DIR=%~dp1"
set "SLN_FILE_NAME=%~nx1"
set "MSVC12_BASE_DIR=%SLN_BASE_DIR%\..\msvc12"

rmdir /S /Q "%MSVC12_BASE_DIR%"
mkdir "%MSVC12_BASE_DIR%"
xcopy /F /H /Y /Z /I "%SLN_BASE_DIR%\*.*" "%MSVC12_BASE_DIR%"
xcopy /F /H /Y /Z /I "%SLN_BASE_DIR%\tests\*.*" "%MSVC12_BASE_DIR%\tests"

call "%VS120COMNTOOLS%\..\IDE\devenv.com" "%MSVC12_BASE_DIR%\%SLN_FILE_NAME%" /Upgrade

start "Migration log" "%MSVC12_BASE_DIR%\UpgradeLog.htm"

endlocal

