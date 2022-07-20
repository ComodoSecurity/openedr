rem @echo off

cd nfapi

nmake -f make/msvc.mak debug=0 c_api=0 static=0 platform=win32 %1
nmake -f make/msvc.mak debug=0 c_api=0 static=1 platform=win32 %1

nmake -f make/msvc.mak debug=0 c_api=1 static=0 platform=win32 %1
nmake -f make/msvc.mak debug=0 c_api=1 static=1 platform=win32 %1

cd ..