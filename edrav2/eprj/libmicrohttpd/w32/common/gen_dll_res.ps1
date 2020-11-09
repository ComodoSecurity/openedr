param ([string]$BasePath = ".\")

$Host.SetShouldExit(111) # Set non-zero return code until task successfully finished
$ErrorActionPreference = "Stop" # Stop on any error

Remove-Variable MHD_ver,MHD_ver_major,MHD_ver_minor,MHD_ver_patchlev -ErrorAction:SilentlyContinue

Write-Output "Processing: ${BasePath}..\..\configure.ac"
foreach($line in Get-Content "${BasePath}..\..\configure.ac")
{
    if ($line -match '^AC_INIT\(\[(?:GNU )?libmicrohttpd\],\[((\d+).(\d+).(\d+))\]') 
    {
        [string]$MHD_ver = $Matches[1].ToString()
        [string]$MHD_ver_major = $Matches[2].ToString()
        [string]$MHD_ver_minor = $Matches[3].ToString()
        [string]$MHD_ver_patchlev = $Matches[4].ToString()
        break 
    }
}
if ("$MHD_ver" -eq "" -or "$MHD_ver_major" -eq ""  -or "$MHD_ver_minor" -eq "" -or "$MHD_ver_patchlev" -eq "")
{
    Throw "Can't find MHD version in ${BasePath}..\..\configure.ac"
}

Write-Output "Detected MHD version: $MHD_ver"

Write-Output "Generating ${BasePath}microhttpd_dll_res_vc.rc"
Get-Content "${BasePath}microhttpd_dll_res_vc.rc.in" | ForEach-Object {
    $_  -replace '@PACKAGE_VERSION_MAJOR@',"$MHD_ver_major" `
        -replace '@PACKAGE_VERSION_MINOR@', "$MHD_ver_minor" `
        -replace '@PACKAGE_VERSION_SUBMINOR@', "$MHD_ver_patchlev" `
        -replace '@PACKAGE_VERSION@', "$MHD_ver"
} | Out-File -FilePath "${BasePath}microhttpd_dll_res_vc.rc" -Force

$Host.SetShouldExit(0) # Reset return code

Write-Output "${BasePath}microhttpd_dll_res_vc.rc was generated "
exit 0 # Exit with success code
