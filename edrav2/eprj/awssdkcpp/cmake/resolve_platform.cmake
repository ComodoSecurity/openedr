
# Platform recognition
if(TARGET_ARCH)
    string(TOUPPER ${TARGET_ARCH} __UPPER_TARGET_ARCH)
endif()

if("${__UPPER_TARGET_ARCH}" STREQUAL "WINDOWS" OR 
   "${__UPPER_TARGET_ARCH}" STREQUAL "LINUX" OR 
   "${__UPPER_TARGET_ARCH}" STREQUAL "APPLE" OR 
   "${__UPPER_TARGET_ARCH}" STREQUAL "ANDROID")
    set(PLATFORM_${__UPPER_TARGET_ARCH} 1)
elseif(TARGET_ARCH)
    set(PLATFORM_CUSTOM 1)
else()
    message(STATUS "TARGET_ARCH not specified; inferring host OS to be platform compilation target")
    if(CMAKE_HOST_WIN32)
        set(PLATFORM_WINDOWS 1)
        set(TARGET_ARCH "WINDOWS")
        set(__UPPER_TARGET_ARCH "WINDOWS")
    elseif(CMAKE_HOST_APPLE)
        set(PLATFORM_APPLE 1)
        set(TARGET_ARCH "APPLE")
        set(__UPPER_TARGET_ARCH "APPLE")
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_RPATH "@executable_path")
    elseif(CMAKE_HOST_UNIX)
        set(PLATFORM_LINUX 1)
        set(TARGET_ARCH "LINUX")
        set(__UPPER_TARGET_ARCH "LINUX")
    else()
        message(FATAL_ERROR "Unknown host OS; unable to determine platform compilation target")
    endif()
endif()

# directory defaults; linux overrides these on SIMPLE_INSTALL builds
# user sepficied cmake variables (cmake -DVAR=xx) will further overrides these
set(BINARY_DIRECTORY "bin")
set(LIBRARY_DIRECTORY "lib")
set(INCLUDE_DIRECTORY "include")
if(BUILD_SHARED_LIBS)
    set(ARCHIVE_DIRECTORY "bin")
    set(LIBTYPE SHARED)
    message(STATUS "Building AWS libraries as shared objects")
else()
    set(ARCHIVE_DIRECTORY "lib")
    set(LIBTYPE STATIC)
    message(STATUS "Building AWS libraries as static objects")
endif()

string(TOLOWER ${TARGET_ARCH} __LOWER_ARCH)

# default settings is unix platform
if(PLATFORM_LINUX OR PLATFORM_APPLE OR PLATFORM_ANDROID)
    include(${CMAKE_CURRENT_LIST_DIR}/platform/unix.cmake)
endif()

# if not specified to custom platform, settings above will be reset by specific platform settings
if(NOT PLATFORM_CUSTOM)
    include(${CMAKE_CURRENT_LIST_DIR}/platform/${__LOWER_ARCH}.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/platform/custom.cmake)
endif()

# only usable in .cpp files
add_definitions(-DPLATFORM_${__UPPER_TARGET_ARCH})
