#!/bin/sh
scripts_dir=`cd $(dirname $0);pwd`
cd $scripts_dir/..

mkdir -p build_i386
cd build_i386

cmake -G "Xcode" -DBUILD_SHARED_LIBS="FALSE" \
                 -DIOS_PLATFORM="SIMULATOR" \
                 -DCMAKE_TOOLCHAIN_FILE=$scripts_dir/../iOS.cmake \
                 -DLOG4CPLUS_SINGLE_THREADED="TRUE" \
                 -DLOG4CPLUS_BUILD_TESTING="OFF" \
                 -DLOG4CPLUS_QT4="OFF" \
                 -DLOG4CPLUS_BUILD_LOGGINGSERVER="OFF" \
                 -DLOG4CPLUS_CONFIGURE_CHECKS_PATH=$scripts_dir/../ConfigureChecks.cmake \
                 -DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=$scripts_dir/../build_i386/Binaries \
                 $@ \
                 $scripts_dir/../..

