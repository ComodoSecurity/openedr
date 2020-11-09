Sign algorithm of EDR agent drivers.

See:
https://docs.microsoft.com/en-us/windows-hardware/drivers/dashboard/attestation-signing-a-kernel-driver-for-public-release

Points to do:
- Sign sys files in directories: files\x32\ and files\x64\ (the signing the same as other EXE and DLL)
- Create CAB-files with make_cabs.cmd
- Sign CAB-files with COMODO EV sign
- Send CAB-files to Microsoft
- Receive signed files from Microsoft. Files should have 3 signs
- Send files to developers to create installer


