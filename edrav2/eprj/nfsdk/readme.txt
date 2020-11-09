NetFilter SDK & ProtocolFilter libraries

Structure:
  - "src" directory contains original libraries with some changes
  - "build" directory contains project files for this project
    - new project files were created
  - "lib" directory contains prebuilded libraries
  - "inc" directory contains h-files

Code changes:
  - Remove unused files
    - "release" and "lib" directories were deleted
    - "._*" files were deleted
    - "openssl" and "zlib" libraries were deleted
  - All not trivial fixes are tagged with EDR_FIX
    - But there are some fixes for using *A-function of WinAPI without tag. 
      - E.g. OpenService -> OpenServiceA
  - Fix compilation bugs for support 
    - UNICODE 
    - vs2017 compiler
    - Win7 as minimal OS

