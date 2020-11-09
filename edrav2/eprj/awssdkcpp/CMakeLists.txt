#
# Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# 
#  http://aws.amazon.com/apache2.0
# 
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#

cmake_minimum_required (VERSION 3.1)
if(POLICY CMP0028)
    cmake_policy(SET CMP0028 NEW)
endif()
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
endif()
if(POLICY CMP0056)
    cmake_policy(SET CMP0056 NEW)
endif()

# 3.0 or higher is strongly suggested; build settings (target_compile_options/etc...) sometimes do not get propagated properly under certain conditions prior to this version
# Making this a hard requirement is potentially disruptive to existing customers who aren't affected by the bad behavior though, so just warn for now
if(CMAKE_MAJOR_VERSION LESS 3)
    message(WARNING "Building with CMake 3.0 or higher is strongly suggested; current version is ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
endif()

get_filename_component(AWS_NATIVE_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# git is required for Android builds and building third-party dependencies
find_package(Git)

# Cmake invocation variables:
#   CUSTOM_MEMORY_MANAGEMENT - if set to 1, generates the sdk project files with custom memory management enabled, otherwise disables it
#   BUILD_ONLY - only build project identified by this variable, a semi-colon delimited list, if this is set we will build only the projects listed. Core will always be built as will its unit tests.
#                    Also if a high level client is specified then we will build its dependencies as well. If a project has tests, the tests will be built.
#   REGENERATE_CLIENTS - all clients being built on this run will be regenerated from the api definitions, this option involves some setup of python, java 8, jdk 1.8, and maven
#   ADD_CUSTOM_CLIENTS - semi-colon delimited list of format serviceName=<yourserviceName>,version=<theVersionNumber>;serviceName2=<yourOtherServiceName>,version=<versionNumber2>
#                          to use these arguments, you should add the api definition .normal.json file for your service to the api-description folder in the generator.
#   NDK_DIR - directory where the android NDK is installed; if not set, the location will be read from the ANDROID_NDK environment variable
#   CUSTOM_PLATFORM_DIR - directory where custom platform scripts, modules, and source resides
#   AWS_SDK_ADDITIONAL_LIBRARIES - names of additional libraries to link into aws-cpp-sdk-core in order to support unusual/unanticipated linking setups (static curl against static-something-other-than-openssl for example)

# TODO: convert boolean invocation variables to options
option(ENABLE_UNITY_BUILD "If enabled, the SDK will be built using a single unified .cpp file for each service library.  Reduces the size of static library binaries on Windows and Linux" OFF)
option(MINIMIZE_SIZE "If enabled, the SDK will be built via a unity aggregation process that results in smaller static libraries; additionally, release binaries will favor size optimizations over speed" OFF)
option(BUILD_SHARED_LIBS "If enabled, all aws sdk libraries will be build as shared objects; otherwise all Aws libraries will be built as static objects" ON)
option(FORCE_SHARED_CRT "If enabled, will unconditionally link the standard libraries in dynamically, otherwise the standard library will be linked in based on the BUILD_SHARED_LIBS setting" ON)
option(DISABLE_ANDROID_STANDALONE_BUILD "If enabled, the android build will not attempt to generate a standalone toolchain and instead rely on externally passed cmake parameters to specify the complete build environment.  The included toolchain file is still used." OFF)
option(SIMPLE_INSTALL "If enabled, removes all the additional indirection (platform/cpu/config) in the bin and lib directories on the install step" ON)
option(NO_HTTP_CLIENT "If enabled, no platform-default http client will be included in the library.  For the library to be used you will need to provide your own platform-specific implementation" OFF)
option(NO_ENCRYPTION "If enabled, no platform-default encryption will be included in the library.  For the library to be used you will need to provide your own platform-specific implementations" OFF)
option(USE_IXML_HTTP_REQUEST_2 "If enabled on windows, the com object IXmlHttpRequest2 will be used for the http stack" OFF)
option(ENABLE_RTTI "Flag to enable/disable rtti within the library" ON)
option(ENABLE_TESTING "Flag to enable/disable building unit and integration tests" ON)
option(AUTORUN_UNIT_TESTS "Flag to enable/disable automatically run unit tests after building" ON)
option(ANDROID_BUILD_CURL "When building for Android, should curl be built as well" ON)
option(ANDROID_BUILD_OPENSSL "When building for Android, should Openssl be built as well" ON)
option(ANDROID_BUILD_ZLIB "When building for Android, should Zlib be built as well" ON)
option(FORCE_CURL "Forces usage of the Curl client rather than the default OS-specific api" OFF)
option(ENABLE_ADDRESS_SANITIZER "Flags to enable/disable Address Sanitizer for gcc or clang" OFF)
option(BYPASS_DEFAULT_PROXY "Bypass the machine's default proxy settings when using IXmlHttpRequest2" ON)
option(BUILD_DEPS "Build third-party dependencies" ON)


set(BUILD_ONLY "" CACHE STRING "A semi-colon delimited list of the projects to build")
set(CPP_STANDARD "11" CACHE STRING "Flag to upgrade the C++ standard used. The default is 11. The minimum is 11.")

# backwards compatibility with old command line params
if("${STATIC_LINKING}" STREQUAL "1")
    set(BUILD_SHARED_LIBS OFF)
endif()

if(MINIMIZE_SIZE)
    message(STATUS "MINIMIZE_SIZE enabled")
    set(ENABLE_UNITY_BUILD ON) # MINIMIZE_SIZE always implies UNITY_BUILD
endif()

set(PYTHON_CMD "python")

# CMAKE_MODULE_PATH is a CMAKE variable. It contains a list of paths 
# which could be used to search CMAKE modules by "include()" or "find_package()", but the default value is empty.
# Add cmake dir to search list
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# include() will "load and run" cmake script
include(resolve_platform)
include(CMakePackageConfigHelpers)

# use response files to prevent command-line-too-big errors for large libraries like iam
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1)
set(CMAKE_CXX_RESPONSE_FILE_LINK_FLAG "@")

if(COMMAND apply_pre_project_platform_settings)
    apply_pre_project_platform_settings()
endif()

include(initialize_project_version)


# build third-party targets
if (BUILD_DEPS)
    # If building third party dependencies, we will move them to the same directory where SDK has been installed during installation.
    # Therefore, we should set rpath to $ORIGIN to let SDK find these third party dependencies.
    # Otherwise, customers are responsible for handling the linkage to these libraries.
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(AWS_DEPS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/.deps)
    if (NOT DEFINED AWS_DEPS_INSTALL_DIR)
        if (DEFINED CMAKE_INSTALL_PREFIX)
            set(AWS_DEPS_INSTALL_DIR ${CMAKE_INSTALL_PREFIX} CACHE STRING "A string describes the path where 3rd-party dependencies will be or have been installed")
        else()
            set(AWS_DEPS_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/.deps/install CACHE STRING "A string describes the path where 3rd-party dependencies will be or have been installed")
        endif()
    endif()
    file(MAKE_DIRECTORY ${AWS_DEPS_BUILD_DIR})
    if(TARGET_ARCH STREQUAL "ANDROID")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR}
            -DTARGET_ARCH=${TARGET_ARCH}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
            -DANDROID_ABI=${ANDROID_ABI}
            -DANDROID_TOOLCHAIN_NAME=${ANDROID_TOOLCHAIN_NAME}
            -DANDROID_STANDALONE_TOOLCHAIN=${ANDROID_STANDALONE_TOOLCHAIN}
            -DANDROID_STL=${ANDROID_STL}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
            -DCMAKE_INSTALL_PREFIX=${AWS_DEPS_INSTALL_DIR}
            -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party
            WORKING_DIRECTORY ${AWS_DEPS_BUILD_DIR}
            RESULT_VARIABLE BUILD_3P_EXIT_CODE)
    elseif(TARGET_ARCH STREQUAL "APPLE" AND DEFINED CMAKE_OSX_ARCHITECTURES AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "")
        message("Cross compiling third-party dependencies for architecture ${CMAKE_OSX_ARCHITECTURES}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR}
            -DTARGET_ARCH=${TARGET_ARCH}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
            -DCMAKE_INSTALL_PREFIX=${AWS_DEPS_INSTALL_DIR}
            -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT}
            -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}
            -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
            -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}
            -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/bin
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party
            WORKING_DIRECTORY ${AWS_DEPS_BUILD_DIR}
            RESULT_VARIABLE BUILD_3P_EXIT_CODE)
    else()
        execute_process(
            COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR}
            -DTARGET_ARCH=${TARGET_ARCH}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}
            -DCMAKE_INSTALL_PREFIX=${AWS_DEPS_INSTALL_DIR}
            -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/bin
            ${CMAKE_CURRENT_SOURCE_DIR}/third-party
            WORKING_DIRECTORY ${AWS_DEPS_BUILD_DIR}
            RESULT_VARIABLE BUILD_3P_EXIT_CODE)
    endif()

    if (NOT ${BUILD_3P_EXIT_CODE} EQUAL 0)
        message(FATAL_ERROR "Failed to configure third-party libraries.")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build ${AWS_DEPS_BUILD_DIR} --config ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE BUILD_3P_EXIT_CODE)

    if (NOT ${BUILD_3P_EXIT_CODE} EQUAL 0)
        message(FATAL_ERROR "Failed to build third-party libraries.")
    endif()
    message(STATUS "Third-party dependencies are installed at: ${AWS_DEPS_INSTALL_DIR}")
    list(APPEND CMAKE_PREFIX_PATH "${AWS_DEPS_INSTALL_DIR}")
endif()

# build the sdk targets
project("aws-cpp-sdk-all" VERSION "${PROJECT_VERSION}" LANGUAGES CXX)

# http client, encryption, zlib
include(external_dependencies)

if(COMMAND apply_post_project_platform_settings)
    apply_post_project_platform_settings()
endif()

set(CMAKE_CONFIGURATION_TYPES
        Debug                   # Setup for easy debugging. No optimizations.
        DebugOpt                # An optimized version of Debug.
        Release                 # Fully optimized, no debugging information.
        RelWithDebInfo          # A debuggable version of Release.
        MinSizeRel              # Like Release, but optimized for memory rather than speed.
        )

include(compiler_settings)
# Instead of calling functions/macros inside included cmake scripts, we should call them in our main CMakeList.txt
set_msvc_flags()
set_msvc_warnings()

include(sdks)

include(utilities)

include(build_external)

if(ENABLE_BCRYPT_ENCRYPTION)
    set(CRYPTO_LIBS Bcrypt)
    set(CRYPTO_LIBS_ABSTRACT_NAME Bcrypt)
elseif(ENABLE_OPENSSL_ENCRYPTION)
    set(CRYPTO_LIBS ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES})
    set(CRYPTO_LIBS_ABSTRACT_NAME crypto ssl z)
endif()

if(ENABLE_CURL_CLIENT)
    set(CLIENT_LIBS ${CURL_LIBRARIES})
    set(CLIENT_LIBS_ABSTRACT_NAME curl)
elseif(ENABLE_WINDOWS_CLIENT)
    if(USE_IXML_HTTP_REQUEST_2)
        set(CLIENT_LIBS msxml6 runtimeobject)
        set(CLIENT_LIBS_ABSTRACT_NAME msxml6 runtimeobject)
        if(BYPASS_DEFAULT_PROXY)
            list(APPEND CLIENT_LIBS winhttp)
            list(APPEND CLIENT_LIBS_ABSTRACT_NAME winhttp)
        endif()
    else()
        set(CLIENT_LIBS Wininet winhttp)
        set(CLIENT_LIBS_ABSTRACT_NAME Wininet winhttp)
    endif()
endif()

# setup user specified installation directory if any, regardless previous platform default settings
if (CMAKE_INSTALL_BINDIR)
    set(BINARY_DIRECTORY "${CMAKE_INSTALL_BINDIR}")
endif()

if (CMAKE_INSTALL_LIBDIR)
    set(LIBRARY_DIRECTORY "${CMAKE_INSTALL_LIBDIR}")
endif()

if (CMAKE_INSTALL_INCLUDEDIR)
    set(INCLUDE_DIRECTORY "${CMAKE_INSTALL_INCLUDEDIR}")
endif()

if(BUILD_SHARED_LIBS)
    set(ARCHIVE_DIRECTORY "${BINARY_DIRECTORY}")
else()
    set(ARCHIVE_DIRECTORY "${LIBRARY_DIRECTORY}")
endif()

if (ENABLE_ADDRESS_SANITIZER)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -fno-omit-frame-pointer")
endif()

include(CheckCXXSymbolExists)

check_cxx_symbol_exists("pathconf" "unistd.h" HAS_PATHCONF)
if (HAS_PATHCONF)
    add_definitions(-DHAS_PATHCONF)
endif()

check_cxx_symbol_exists("umask" "sys/stat.h" HAS_UMASK)
if (HAS_UMASK)
    add_definitions(-DHAS_UMASK)
endif()

add_sdks()

# for user friendly cmake usage
include(setup_cmake_find_module)

# for generating make uninstall target
if (NOT TARGET uninstall)
    ADD_CUSTOM_TARGET(uninstall "${CMAKE_COMMAND}" -P "${AWS_NATIVE_SDK_ROOT}/cmake/make_uninstall.cmake")
else()
    ADD_CUSTOM_TARGET(uninstall-awssdk "${CMAKE_COMMAND}" -P "${AWS_NATIVE_SDK_ROOT}/cmake/make_uninstall.cmake")
endif()
