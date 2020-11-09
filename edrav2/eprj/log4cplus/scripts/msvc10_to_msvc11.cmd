@echo on
setlocal
set "THIS_SCRIPT_FILE=%~nx0"

if "%1" == "" (
  echo.
  echo USAGE: %THIS_SCRIPT_FILE% log4cplus.sln
  echo.
  exit /b 1
)

if not defined VS110COMNTOOLS (
  echo.
  echo VS110COMNTOOLS environment variable is not defined
  echo.
  exit /b 2
)

set "SLN_PATH=%~f1"
set "SLN_BASE_DIR=%~dp1"
set "SLN_FILE_NAME=%~nx1"
set "MSVC11_BASE_DIR=%SLN_BASE_DIR%\..\msvc11"

rmdir /S /Q "%MSVC11_BASE_DIR%"
mkdir "%MSVC11_BASE_DIR%"
xcopy /F /H /Y /Z /I "%SLN_BASE_DIR%\*.*" "%MSVC11_BASE_DIR%"
xcopy /F /H /Y /Z /I "%SLN_BASE_DIR%\tests\*.*" "%MSVC11_BASE_DIR%\tests"

call "%VS110COMNTOOLS%\..\IDE\devenv.com" "%MSVC11_BASE_DIR%\%SLN_FILE_NAME%" /Upgrade

start "Migration log" "%MSVC11_BASE_DIR%\UpgradeLog.htm"

endlocal

