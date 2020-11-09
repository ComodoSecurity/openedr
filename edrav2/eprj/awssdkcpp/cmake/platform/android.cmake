
macro(verify_tools)
    # TODO: don't do this if the user is supplying their own curl/openssl/zlib
    # minimum version of cmake that 
    #   (1) supports ExternalProject_Add URL_HASH
    #   (2) correctly extracts OPENSSL's version number from openssl/opensslv.h in version 1.0.2d
    cmake_minimum_required (VERSION 3.1.2)

    # TODO: don't do this if the user is supplying their own curl/openssl/zlib
    if(NOT GIT_FOUND)
        message(FATAL_ERROR "Unable to find git; git is required in order to build for Android")
    endif()
endmacro()

macro(determine_stdlib_and_api)
    if(ANDROID_STL MATCHES "libc" OR NOT ANDROID_STL)
        if(FORCE_SHARED_CRT OR BUILD_SHARED_LIBS)
            SET(ANDROID_STL "libc++_shared" CACHE STRING "" FORCE)
        else()
            SET(ANDROID_STL "libc++_static" CACHE STRING "" FORCE)
        endif()

        if(NOT ANDROID_NATIVE_API_LEVEL)
            set(ANDROID_NATIVE_API_LEVEL "android-21")
        endif()

        # API levels below 9 will not build with libc++
        string(REGEX REPLACE "android-(..?)" "\\1" EXTRACTED_API_LEVEL "${ANDROID_NATIVE_API_LEVEL}")
        if(EXTRACTED_API_LEVEL LESS "9")
            message(STATUS "Libc++ requires setting API level to at least 9")
            set(ANDROID_NATIVE_API_LEVEL "android-9" CACHE STRING "" FORCE)
        endif()

        set(STANDALONE_TOOLCHAIN_STL "libc++")
    elseif(ANDROID_STL MATCHES "gnustl")
        if(FORCE_SHARED_CRT OR BUILD_SHARED_LIBS)
            SET(ANDROID_STL "gnustl_shared" CACHE STRING "" FORCE)
        else()
            SET(ANDROID_STL "gnustl_static" CACHE STRING "" FORCE)
        endif()

        # With gnustl, API level can go as low as 3, but let's make a reasonably modern default
        if(NOT ANDROID_NATIVE_API_LEVEL)
            set(ANDROID_NATIVE_API_LEVEL "android-19") 
        endif()

        set(STANDALONE_TOOLCHAIN_STL "gnustl")
    else()
        message(FATAL_ERROR "Invalid value for ANDROID_STL: ${ANDROID_STL}")
    endif()

    string(REGEX REPLACE "android-(..?)" "\\1" ANDROID_NATIVE_API_LEVEL_NUM "${ANDROID_NATIVE_API_LEVEL}")

endmacro()

function(get_android_toolchain_name ABI TOOLCHAIN_VAR)
    if(ABI MATCHES "armeabi")
        set(__toolchain_name "arm-linux-androideabi-clang")
    elseif(ABI MATCHES "arm64")
        set(__toolchain_name "aarch64-linux-android-clang")
    elseif(ABI MATCHES "x86_64")
        set(__toolchain_name "x86_64-clang")
    elseif(ABI MATCHES "x86")
        set(__toolchain_name "x86-clang")
    elseif(ABI MATCHES "mips64")
        set(__toolchain_name "mips64el-linux-android-clang")
    elseif(ABI MATCHES "mips")
        set(__toolchain_name "mipsel-linux-android-clang")
    else()
        message(FATAL_ERROR "Unable to map ANDROID_ABI value ${ABI} to a valid toolchain for make-standalone-toolchain")
    endif()
    set(${TOOLCHAIN_VAR} "${__toolchain_name}" PARENT_SCOPE)
endfunction(get_android_toolchain_name)


function(get_android_arch_name ABI ARCH_VAR)
    if(ABI MATCHES "armeabi")
        set(__arch_name "arm")
    elseif(ABI MATCHES "arm64")
        set(__arch_name "arm64")
    elseif(ABI MATCHES "x86_64")
        set(__arch_name "x86_64")
    elseif(ABI MATCHES "x86")
        set(__arch_name "x86")
    elseif(ABI MATCHES "mips64")
        set(__arch_name "mips64")
    elseif(ABI MATCHES "mips")
        set(__arch_name "mips")
    else()
        message(FATAL_ERROR "Unable to map ANDROID_ABI value ${ABI} to a valid arch for make-standalone-toolchain")
    endif()
    set(${ARCH_VAR} "${__arch_name}" PARENT_SCOPE)
endfunction(get_android_arch_name)


function(get_android_ndk_release __NDK_DIR NDK_RELEASE_NUMBER)
    if(NOT NDK_RELEASE)
        if( EXISTS "${__NDK_DIR}/RELEASE.TXT" )
            # RELEASE.txt disappeared starting with r11, grumble
            file( STRINGS "${__NDK_DIR}/RELEASE.TXT" __ANDROID_NDK_RELEASE_FULL LIMIT_COUNT 1 REGEX "r[0-9]+[a-z]?" )
            string( REGEX MATCH "r([0-9]+)([a-z]?)" NDK_RELEASE "${__ANDROID_NDK_RELEASE_FULL}" )
        else()
            # guess based on install directory name
            string( REGEX REPLACE ".*ndk-(r[0-9]+[a-z]?).*" "\\1" NDK_RELEASE "${__NDK_DIR}")
        endif()
    endif()

    if(NOT NDK_RELEASE)
        message(FATAL_ERROR "Unable to detect NDK version; you will need to pass it as a cmake parameter using -DNDK_RELEASE.  For example, cmake -DNDK_RELEASE=r11c ...")
    endif()

    message(STATUS "Detected NDK version ${NDK_RELEASE}")

    string( REGEX REPLACE "r([0-9]+)([a-z]?)" "\\1*1000" __ANDROID_NDK_RELEASE_NUM "${NDK_RELEASE}" )
    string( FIND " abcdefghijklmnopqastuvwxyz" "${CMAKE_MATCH_2}" __ndkReleaseLetterNum )
    math( EXPR __ANDROID_NDK_RELEASE_NUM "${__ANDROID_NDK_RELEASE_NUM}+${__ndkReleaseLetterNum}" )

    set(NDK_RELEASE_NUMBER ${__ANDROID_NDK_RELEASE_NUM} PARENT_SCOPE)
endfunction()

macro(initialize_toolchain)
    # if not explicitly disabled, generate a standalone toolchain
    if(NOT DISABLE_ANDROID_STANDALONE_BUILD AND NOT ANDROID_STANDALONE_TOOLCHAIN)

        if( NOT NDK_DIR )
            set( NDK_DIR $ENV{ANDROID_NDK} )
        endif()

        if( NOT IS_DIRECTORY "${NDK_DIR}" )
            message(FATAL_ERROR "Could not find Android NDK (${NDK_DIR}); either set the ANDROID_NDK environment variable or pass the path in via -DNDK_DIR=..." )
        endif()

        get_android_ndk_release(${NDK_DIR} NDK_RELEASE_NUMBER)

        if(NDK_RELEASE_NUMBER LESS "10000")
            message(FATAL_ERROR "NDK versions less than 10 currently not supported")
        endif()

        set(TOOLCHAIN_ROOT "${CMAKE_CURRENT_BINARY_DIR}/toolchains/android")

        if( NOT EXISTS ${TOOLCHAIN_ROOT})
            file(MAKE_DIRECTORY ${TOOLCHAIN_ROOT})
        endif()

        set(STANDALONE_TOOLCHAIN_DIR "${TOOLCHAIN_ROOT}/${ANDROID_ABI}-${ANDROID_TOOLCHAIN_NAME}-${ANDROID_NATIVE_API_LEVEL}-${ANDROID_STL}-${NDK_RELEASE_NUMBER}")
        if( NOT EXISTS ${STANDALONE_TOOLCHAIN_DIR} )
            message(STATUS "Could not find an appropriate standalone toolchain.  Generating one into ${STANDALONE_TOOLCHAIN_DIR}")

            set(__MAKE_TOOLCHAIN_OPTIONS "--stl=${STANDALONE_TOOLCHAIN_STL} --install-dir=${STANDALONE_TOOLCHAIN_DIR}")

            if(NDK_RELEASE_NUMBER LESS "12000")
                # Releases prior to 12 use a shell script to make standalone toolchains
                if(CMAKE_HOST_WIN32)
                    find_program(SH_FOUND sh)
                    if(NOT SH_FOUND)
                        message(FATAL_ERROR "Unable to find sh; sh is required to generate a standalone toolchain for NDK r11 and lower")
                    endif()
                    set(TOOLCHAIN_SCRIPT_HOST "sh")
                endif()

                get_android_toolchain_name(${ANDROID_ABI} __TOOLCHAIN)

                if(NDK_RELEASE_NUMBER LESS "11000")
                    set(__TOOLCHAIN "${__TOOLCHAIN}3.5")
                    set(LLVM_OPTIONS "--llvm-version=3.5")
                else()
                    set(LLVM_OPTIONS "--use-llvm")
                endif()

                set(__MAKE_TOOLCHAIN_OPTIONS "${__MAKE_TOOLCHAIN_OPTIONS} --ndk-dir=${NDK_DIR} --toolchain=${__TOOLCHAIN} ${LLVM_OPTIONS} --platform=${ANDROID_NATIVE_API_LEVEL}")

                execute_process(
                    COMMAND ${TOOLCHAIN_SCRIPT_HOST} ${NDK_DIR}/build/tools/make-standalone-toolchain.sh --ndk-dir=${NDK_DIR} --toolchain=${__TOOLCHAIN} ${LLVM_OPTIONS} 
                    --platform=${ANDROID_NATIVE_API_LEVEL} --stl=${STANDALONE_TOOLCHAIN_STL} --install-dir=${STANDALONE_TOOLCHAIN_DIR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                )
            else()
                find_program(PYTHON_FOUND python)
                if(NOT PYTHON_FOUND)
                    message(FATAL_ERROR "Unable to find python; python is required to generate a standalone toolchain for NDK r12 and higher")
                endif()

                # new releases use a python script with (naturally) different argument names
                get_android_arch_name(${ANDROID_ABI} __ARCH)

                execute_process(
                    COMMAND python ${NDK_DIR}/build/tools/make_standalone_toolchain.py --arch=${__ARCH} --api=${ANDROID_NATIVE_API_LEVEL_NUM} --stl=${STANDALONE_TOOLCHAIN_STL} --install-dir=${STANDALONE_TOOLCHAIN_DIR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                )
            endif()

            execute_process(COMMAND chmod a+r ${STANDALONE_TOOLCHAIN_DIR})
        else()
            message(STATUS "Using existing standalone toolchain located at ${STANDALONE_TOOLCHAIN_DIR}")
        endif()

        set(ANDROID_STANDALONE_TOOLCHAIN "${STANDALONE_TOOLCHAIN_DIR}")
    else()
        message(STATUS "Standalone toolchain disabled; this is not a well-supported option")
    endif()
endmacro()

macro(apply_pre_project_platform_settings)
    verify_tools()

    set(CMAKE_TOOLCHAIN_FILE ${AWS_NATIVE_SDK_ROOT}/cmake/platform/android.toolchain.cmake)

    # android-specific required overrrides
    if (NOT DEFINED CUSTOM_MEMORY_MANAGEMENT)
        set(CUSTOM_MEMORY_MANAGEMENT "1")
    endif()

    set(ANDROID_STL_FORCE_FEATURES "OFF")

    # android-specific parameter defaults
    if(NOT ANDROID_ABI)
        set(ANDROID_ABI "armeabi-v7a")
        message(STATUS "Android ABI: none specified, defaulting to ${ANDROID_ABI}")
    else()
        message(STATUS "Android ABI: ${ANDROID_ABI}")
    endif()

    if(NOT ANDROID_TOOLCHAIN_NAME)
        set(ANDROID_TOOLCHAIN_NAME "standalone-clang")
        message(STATUS "Android toolchain unspecified, defaulting to ${ANDROID_TOOLCHAIN_NAME}")
    endif()

    determine_stdlib_and_api()

    message(STATUS "Android std lib: ${ANDROID_STL}")
    message(STATUS "Android API level: ${ANDROID_NATIVE_API_LEVEL}")

    initialize_toolchain()

    add_definitions("-DDISABLE_HOME_DIR_REDIRECT")

endmacro()

macro(apply_post_project_platform_settings)
    set(SDK_INSTALL_BINARY_PREFIX "${SDK_INSTALL_BINARY_PREFIX}/${ANDROID_ABI}-api-${ANDROID_NATIVE_API_LEVEL}")

    set(PLATFORM_DEP_LIBS log atomic)
    set(PLATFORM_DEP_LIBS_ABSTRACT_NAME log atomic)

    # Workaround for problem when ndk 13, gcc, and libc++ are used together.
    # See https://www.bountysource.com/issues/38341727-stddef-h-no-such-file-or-directory
    if(NOT NDK_RELEASE_NUMBER LESS "13000")
        if(ANDROID_TOOLCHAIN_NAME STREQUAL "standalone" AND ANDROID_STL MATCHES "libc" AND ANDROID_STANDALONE_TOOLCHAIN)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${ANDROID_STANDALONE_TOOLCHAIN}/include/c++/4.9.x")
        endif()
    endif()
    if(ANDROID_STL MATCHES "libc" AND ANDROID_NATIVE_API_LEVEL_NUM LESS "21")
        include_directories("${NDK_DIR}/sources/android/support/include")
    endif()

endmacro()

