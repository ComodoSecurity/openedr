#!/bin/sh

set -x

for f in CMakeFiles \
    debug \
    log4cplus.dir \
    loggingserver.dir \
    minsizerel \
    release \
    relwithdebinfo \
    ZERO_CHECK.dir \
    cmake_install.cmake \
    CMakeCache.txt \
    ;
do
    rm -rf "${f}"
done
