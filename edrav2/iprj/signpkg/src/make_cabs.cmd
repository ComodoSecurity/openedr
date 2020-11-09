@echo off

::
:: Parameters for signing
::
:: Path to signtool.exe
set SignToolPath="C:\Program Files (x86)\Windows Kits\8.1\bin\x86\signtool.exe"
:: Thumbprint of sign
set SignThumbprint=0123456789ABCDEF0123456789ABCDEF01234567


:: Create CAB-files
echo Create edrdrv_x64.cab ...
MakeCab.exe /V1 /f x64.ddf
echo Create edrdrv_x32.cab ...
MakeCab.exe /V1 /f x32.ddf

:: Remove temprorary files of CAB creation
rm setup.inf
rm setup.rpt

:: Sign CAB-files
::echo Sign edrdrv_x64.cab ...
::%SignToolPath% sign /sha1 %SignThumbprint% /v /ph /fd sha256 /tr http://timestamp.globalsign.com/?signature=sha2 /td sha256 edrdrv_x64.cab
::echo Sign edrdrv_x32.cab ...
::%SignToolPath% sign /sha1 %SignThumbprint% /v /ph /fd sha256 /tr http://timestamp.globalsign.com/?signature=sha2 /td sha256 edrdrv_x32.cab

pause