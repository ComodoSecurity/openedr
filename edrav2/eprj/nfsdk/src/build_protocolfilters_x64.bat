rem @echo off

cd protocolfilters

nmake -f make/msvc.mak debug=0 c_api=0 logs=0 static=0 static_ssl=0 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=0 logs=1 static=0 static_ssl=0 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=0 logs=0 static=1 static_ssl=0 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=0 logs=0 static=0 static_ssl=1 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=0 logs=0 static=0 no_ssl=1 platform=x64 %1

nmake -f make/msvc.mak debug=0 c_api=1 logs=0 static=0 static_ssl=0 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=1 logs=1 static=0 static_ssl=0 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=1 logs=0 static=1 static_ssl=0 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=1 logs=0 static=0 static_ssl=1 platform=x64 %1
nmake -f make/msvc.mak debug=0 c_api=1 logs=0 static=0 no_ssl=1 platform=x64 %1

cd ..