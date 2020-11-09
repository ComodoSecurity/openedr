
message(STATUS "Generating linux build config")
set(SDK_INSTALL_BINARY_PREFIX "linux")

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
include(CheckLibraryExists)

set(ATOMIC_TEST_CXX_SOURCE "
#include <cstdint>
#include <atomic>
std::atomic<uintptr_t> x;
std::atomic<uintmax_t> y;
int main() {
    return x + y;
}")

macro(apply_post_project_platform_settings)
    include(GNUInstallDirs)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(SDK_INSTALL_BINARY_PREFIX "${SDK_INSTALL_BINARY_PREFIX}/intel64")
    else()
        set(SDK_INSTALL_BINARY_PREFIX "${SDK_INSTALL_BINARY_PREFIX}/ia32")
    endif()

    set(PLATFORM_DEP_LIBS pthread)
    set(PLATFORM_DEP_LIBS_ABSTRACT_NAME pthread)

    # libatomic - some arches need to link against libatomic.so, some don't
    cmake_push_check_state()
    list(APPEND CMAKE_REQUIRED_FLAGS "-std=c++${CPP_STANDARD}")
    check_cxx_source_compiles("${ATOMIC_TEST_CXX_SOURCE}" HAVE_ATOMICS_WITHOUT_LIBATOMIC)
    if(NOT HAVE_ATOMICS_WITHOUT_LIBATOMIC)
        if("${CMAKE_VERSION}" VERSION_LESS "3.4.0")
            enable_language(C)
        endif()
        check_library_exists(atomic __atomic_load_8 "" LIBATOMIC_EXISTS)
        if(LIBATOMIC_EXISTS)
            set(CMAKE_REQUIRED_LIBRARIES atomic)
            check_cxx_source_compiles("${ATOMIC_TEST_CXX_SOURCE}" HAVE_ATOMICS_WITH_LIBATOMIC)
        endif()
        if(HAVE_ATOMICS_WITH_LIBATOMIC)
            list(APPEND PLATFORM_DEP_LIBS atomic)
        else()
            message(FATAL_ERROR "Could not determine support for atomic operations.")
        endif()
    endif()
    cmake_pop_check_state()
endmacro()
