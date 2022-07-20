import_root_cert - imports the specified in command line *.cer file to Windows, Mozilla and Opera CA storages.
If no file name is provided, it searches for a first *.cer file in the same directory with executable.
certutil.exe with its dependencies is used for updating Mozilla certificate storage. These files
must be stored in "nss" subfolder in the same directory with executable.

Example: 

import_root_cert.exe NetFilterSDK.cer
