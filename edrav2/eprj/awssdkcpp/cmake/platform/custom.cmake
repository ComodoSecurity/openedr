
if(NOT CUSTOM_PLATFORM_DIR )
    message(FATAL_ERROR "Custom platforms require the CUSTOM_PLATFORM_DIR parameter to be set")
endif()

file(TO_CMAKE_PATH "${CUSTOM_PLATFORM_DIR}" __custom_platform_dir)
if(NOT IS_DIRECTORY "${__custom_platform_dir}")
    message(FATAL_ERROR "${CUSTOM_PLATFORM_DIR} is not a valid directory for a custom platform")
endif()

set(__toolchain_file ${__custom_platform_dir}/${TARGET_ARCH}.toolchain.cmake)
IF(NOT (EXISTS ${__toolchain_file}))
    message(FATAL_ERROR "Custom architecture target ${TARGET_ARCH} requires a cmake toolchain file at: ${__toolchain_file}")
endif()
SET(CMAKE_TOOLCHAIN_FILE ${__toolchain_file})

set(CUSTOM_PLATFORM_SOURCE_PATH "${__custom_platform_dir}/src")
list(APPEND CMAKE_MODULE_PATH "${__custom_platform_dir}/modules")

string(TOLOWER "${TARGET_ARCH}" __lower_arch)
set(SDK_INSTALL_BINARY_PREFIX ${__lower_arch})
include(${__custom_platform_dir}/${__lower_arch}_platform.cmake)

message(STATUS "Generating ${__lower_arch} build config")
