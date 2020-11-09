#!/bin/sh
cd `dirname $0`/..

mkdir -p build_mips
cd build_mips

cmake -DANDROID_NATIVE_API_LEVEL=android-9 -DANDROID_ABI=mips -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake $@ ../..

