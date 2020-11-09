#!/bin/sh
cd `dirname $0`/..

mkdir -p build_armeabi
cd build_armeabi

cmake -DANDROID_NATIVE_API_LEVEL=android-9 -DANDROID_ABI=armeabi -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake $@ ../..

