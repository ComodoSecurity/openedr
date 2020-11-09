# Compiler recognition
set(COMPILER_MSVC 0)
set(COMPILER_GCC 0)
set(COMPILER_CLANG 0)

# ToDo: extend as necessary and remove common assumptions
if(MSVC)
    set(COMPILER_MSVC 1)
else()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(COMPILER_CLANG 1)
    else()
        set(COMPILER_GCC 1)
    endif()
    set(USE_GCC_FLAGS 1)
endif()

function(set_compiler_flags target)
    if(NOT MSVC)
        set_gcc_flags()
        target_compile_options(${target} PRIVATE "${AWS_COMPILER_FLAGS}")
        string(REPLACE ";" " " _TMP "${AWS_COMPILER_FLAGS}")
        set(PKG_CONFIG_CFLAGS "${_TMP}" CACHE INTERNAL "C++ compiler flags which affect the ABI")
    endif()
endfunction()

function(set_compiler_warnings target)
    if(NOT MSVC)
        set_gcc_warnings()
        target_compile_options(${target} PRIVATE "${AWS_COMPILER_WARNINGS}")
    endif()
endfunction()


macro(set_gcc_flags)
    list(APPEND AWS_COMPILER_FLAGS "-fno-exceptions" "-std=c++${CPP_STANDARD}")

    if(NOT BUILD_SHARED_LIBS)
        list(APPEND AWS_COMPILER_FLAGS "-fPIC")
    endif()

    if(NOT ENABLE_RTTI)
        list(APPEND AWS_COMPILER_FLAGS "-fno-rtti")
    endif()

    if(MINIMIZE_SIZE AND COMPILER_GCC)
        list(APPEND AWS_COMPILER_FLAGS "-s")
    endif()
endmacro()

macro(set_gcc_warnings)
    list(APPEND AWS_COMPILER_WARNINGS "-Wall" "-Werror" "-pedantic" "-Wextra")
    if(COMPILER_CLANG)
        if(PLATFORM_ANDROID)
            # when using clang with libc and API lower than 21 we need to include Android support headers and ignore the gnu-include-next warning.
            if(ANDROID_STL MATCHES "libc" AND ANDROID_NATIVE_API_LEVEL_NUM LESS "21")
                # NDK lower than 12 doesn't support ignoring the gnu-include-next warning so we need to disable pedantic mode.
                if(NDK_RELEASE_NUMBER LESS "12000")
                    string(REGEX REPLACE "-pedantic" "" AWS_COMPILER_WARNINGS "${AWS_COMPILER_WARNINGS}")
                else()
                    list(APPEND AWS_COMPILER_WARNINGS "-Wno-gnu-include-next")
                endif()
            endif()
        endif()
    endif()
endmacro()

macro(set_msvc_flags)
    if(MSVC)
        # Put all runtime outputs, including DLLs, executables into one directory, so as to avoid copying DLLs.
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

        # Based on the FORCE_SHARED_CRT and BUILD_SHARED_LIBS options, make sure our compile/link flags bring in the right CRT library
        # modified from gtest's version; while only the else clause is actually necessary, do both for completeness/future-proofing
        foreach (var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(BUILD_SHARED_LIBS OR FORCE_SHARED_CRT)
                string(REPLACE "/MT" "/MD" ${var} "${${var}}")
            else()
                string(REPLACE "/MD" "/MT" ${var} "${${var}}")
            endif()
        endforeach()

        # enable parallel builds
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        # some of the clients are exceeding the 16-bit code section limit when building x64 debug, so use /bigobj when we build
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")

        if(NOT ENABLE_RTTI)
            string(REGEX REPLACE "/GR " " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
        endif()

        # special windows build options:
        #   debug info: pdbs with dlls, embedded in static libs
        #   release optimisations to purely focus on size, override debug info settings as necessary
        if(BUILD_SHARED_LIBS)
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
            set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
        #else()
        #    if(CMAKE_CXX_FLAGS MATCHES "/Zi")
        #        string(REGEX REPLACE "/Zi" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        #    endif()
        #    if(CMAKE_CXX_FLAGS_DEBUG MATCHES "/Zi")
        #        message(STATUS "Clearing pdb setting")
        #        string(REGEX REPLACE "/Zi" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        #    endif()
        #
        #    # put Z7 in config-specific flags so we can strip from release if we're concerned about size
        #    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Z7")
        #    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Z7")
        endif()

        if(MINIMIZE_SIZE)
            # strip debug info from release
            string(REGEX REPLACE "/Z[a-zA-Z0-9]" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
            string(REGEX REPLACE "/DEBUG" "" CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")

            # strip optimization settings and replace with
            string(REGEX REPLACE "/O[a-zA-Z0-9]*" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

            # pure size flags
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O1 /Ob0 /Os")
        endif()
    endif()
endmacro()

macro(set_msvc_warnings)
    if(MSVC)
        #remove bogus errors at generation time if these variables haven't been manually set
        if(NOT CMAKE_CONFIGURATION_TYPES)
            set(CMAKE_CONFIGURATION_TYPES "Debug;Release;MinSizeRel;RelWithDebInfo")
        endif()

        if(NOT CMAKE_CXX_FLAGS_DEBUGOPT)
            set(CMAKE_CXX_FLAGS_DEBUGOPT "")
        endif()

        if(NOT CMAKE_EXE_LINKER_FLAGS_DEBUGOPT)
            set(CMAKE_EXE_LINKER_FLAGS_DEBUGOPT "")
        endif()

        if(NOT CMAKE_SHARED_LINKER_FLAGS_DEBUGOPT)
            set(CMAKE_SHARED_LINKER_FLAGS_DEBUGOPT "")
        endif()

        # warnings as errors, max warning level (4)
        if(NOT CMAKE_CXX_FLAGS MATCHES "/WX")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
        endif()

        # taken from http://stackoverflow.com/questions/2368811/how-to-set-warning-level-in-cmake
        if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
            string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
        endif()
    endif(MSVC)
endmacro()
