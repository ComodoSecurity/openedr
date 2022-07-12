Overview
=====================================
This is the full sources of NetFilter SDK 2.0 + ProtocolFilters. 

Package contents
=====================================
bin\Release - x86 and x64 versions of APIs with C++ interface, pre-built samples and the driver registration utility.
bin\Release_c_api - x86 and x64 versions of APIs with C interface, pre-built samples and the driver registration utility.
bin\Release_logs - debug build of ProtocolFilters.dll with C++ interface that writes log file ProtocolFiltersLog.txt in the same folder.
bin\Release_no_ssl - ProtocolFilters.dll with C++ interface without SSL filtering support.
bin\Release_static_ssl - ProtocolFilters.dll with C++ interface linked statically with OpenSSL.
bin\Release_c_api_logs - debug build of ProtocolFilters.dll with C interface that writes log file ProtocolFiltersLog.txt in the same folder.
bin\Release_c_api_no_ssl - ProtocolFilters.dll with C interface without SSL filtering support.
bin\Release_c_api_static_ssl - ProtocolFilters.dll with C interface linked statically with OpenSSL.

bin\driver\tdi\std - the binaries of TDI driver for x86 and x64 platforms.
bin\driver\tdi\wpp - the binaries of TDI driver for x86 and x64 platforms with ETW tracing support. 

bin\driver\wfp\*\std - the binaries of WFP driver for x86 and x64 platforms.
bin\driver\wfp\*\wpp - the binaries of WFP driver for x86 and x64 platforms with ETW tracing support. 

driver_tdi - the sources of TDI driver.
driver_wfp - the sources of WFP driver.
nfapi - the sources of driver API.
ProtocolFilters - the sources of ProtocolFilters library

samples - the examples of using APIs in C/C++/Deplhi/.NET
samples\CSharp - .NET API and C# samples.
samples\Delphi - Delphi API and samples.
Help - API documentation.

server - gateway filter.


Driver installation
=====================================
- Use the scripts bin\install_*_driver.bat and bin\install_*_driver_x64.bat for installing and registering the network hooking driver on x86 and x64 systems respectively. 
The driver starts immediately and reboot is not required.

- Run bin\uninstall_driver.bat to remove the driver from system.

Elevated administrative rights must be activated explicitly on Vista for registering the driver (run the scripts using "Run as administrator" context menu item in Windows Explorer). 

For Windows Vista x64 and later versions of the Windows family of operating systems, kernel-mode software must have a digital signature to load on x64-based computer systems. 
The included x64 version of the network hooking driver is not signed. In order to test it on Vista x64 you should press F8 during system boot and choose Disable Driver Signature Enforcement option. 
For the end-user software you have to obtain the Code Signing certificate and sign the driver.


Supported platforms: 
    TDI driver: Windows XP/2003/Vista/7/8/10 x86/x64
    WFP driver: Windows 7/8/2008/2012/10 x86/x64
