# all dependencies that are not directly included in the libjson-rpc-cpp distribution are defined here!
# default search directory for dependencies is ${CMAKE_SOURCE_DIR}/win32-deps (for backwards compatibility)
# if your dependencies directory is different, please run cmake with CMAKE_PREFIX_PATH option eg:
#
# cmake -DCMAKE_PREFIX_PATH=path_to_your_dependencies .

# set default dependencies search path
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/win32-deps")

find_package(Jsoncpp REQUIRED)
message(STATUS "Jsoncpp header: ${JSONCPP_INCLUDE_DIR}")
message(STATUS "Jsoncpp lib   : ${JSONCPP_LIBRARY}")
include_directories(${JSONCPP_INCLUDE_DIR})
include_directories(${JSONCPP_INCLUDE_DIR}/jsoncpp)

if(${COMPILE_STUBGEN})
    find_package(Argtable REQUIRED)
    message(STATUS "Argtable header: ${ARGTABLE_INCLUDE_DIRS}")
    message(STATUS "Argtable lib   : ${ARGTABLE_LIBRARIES}")
endif()

if(${HTTP_CLIENT})
    find_package(CURL REQUIRED)
    find_package(Threads)
    message(STATUS "CURL header: ${CURL_INCLUDE_DIRS}")
    message(STATUS "CURL lib   : ${CURL_LIBRARIES}")
	include_directories(${CURL_INCLUDE_DIR})
endif()

if (${HTTP_SERVER})
    find_package(MHD REQUIRED)
    message(STATUS "MHD header: ${MHD_INCLUDE_DIRS}")
    message(STATUS "MHD lib   : ${MHD_LIBRARIES}")
endif()

if (${REDIS_SERVER} OR ${REDIS_CLIENT})
    find_package(Hiredis REQUIRED)
    message(STATUS "Hiredis header: ${HIREDIS_INCLUDE_DIRS}")
    message(STATUS "Hiredis lib   : ${HIREDIS_LIBRARIES}")
endif()

find_package(Threads REQUIRED)
find_package(Doxygen)

if (${COMPILE_TESTS})
    find_package(Catch)
    if(NOT CATCH_FOUND)
        message(STATUS "Could not find catch, downloading it now")
        add_subdirectory(${CMAKE_SOURCE_DIR}/src/catch)
        include_directories(${CATCH_INCLUDE_DIR} ${COMMON_INCLUDES})
        message(STATUS "Catch directory: ${CATCH_INCLUDE_DIR} ${COMMON_INCLUDES}")
    else()
        include_directories(${CATCH_INCLUDE_DIRS})
    endif()
endif()
