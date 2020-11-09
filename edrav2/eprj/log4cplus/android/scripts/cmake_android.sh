#!/bin/sh
cd `dirname $0`/..

mkdir -p build
cd build

cmake -DANDROID_NATIVE_API_LEVEL=android-9 -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON -DCMAKE_TOOLCHAIN_FILE=../android.toolchain.cmake $@ ../..

