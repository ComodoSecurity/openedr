cmake_minimum_required(VERSION 3.0)

# setup directory where we should look for cmake files
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

option(HUNTER_ENABLED "Enable Hunter package manager" NO)
include(HunterGate)
HunterGate(
	URL "https://github.com/ruslo/hunter/archive/v0.18.47.tar.gz"
	SHA1 "f0c476a3a0f9edd6182936d8c4460811f112189c"
)

project(libjson-rpc-cpp)

# list command no longer ignores empty elements
cmake_policy(SET CMP0007 NEW)
cmake_policy(SET CMP0012 NEW)

if (${CMAKE_MAJOR_VERSION} GREATER 2)
    # old policy do not use MACOSX_RPATH
    cmake_policy(SET CMP0042 OLD)
endif()

set(MAJOR_VERSION 1)
set(MINOR_VERSION 1)
set(PATCH_VERSION 1)
set(SO_VERSION    1)

if(NOT MSVC)
	set(BUILD_SHARED_LIBS YES CACHE BOOL "Build shared libraries")
	set(BUILD_STATIC_LIBS NO CACHE BOOL "Build static libraries")
else()
	set(BUILD_SHARED_LIBS NO CACHE BOOL "Build shared libraries")
	set(BUILD_STATIC_LIBS YES CACHE BOOL "Build static libraries")
endif()
set(LIB_SUFFIX "" CACHE STRING "Suffix for library directory (32/64)")

if (NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "Both BUILD_SHARED_LIBS and BUILD_STATIC_LIBS are disabled")
endif()

# defaults for modules that can be enabled/disabled
if(UNIX)
    set(UNIX_DOMAIN_SOCKET_SERVER NO CACHE BOOL "Include Unix Domain Socket server")
    set(UNIX_DOMAIN_SOCKET_CLIENT NO CACHE BOOL "Include Unix Domain Socket client")
    set(FILE_DESCRIPTOR_SERVER NO CACHE BOOL "Include File Descriptor server")
    set(FILE_DESCRIPTOR_CLIENT NO CACHE BOOL "Include File Descriptor client")
endif(UNIX)

set(TCP_SOCKET_SERVER NO CACHE BOOL "Include Tcp Socket server")
set(TCP_SOCKET_CLIENT NO CACHE BOOL "Include Tcp Socket client")
set(REDIS_SERVER YES CACHE BOOL "Include Redis server using hiredis")
set(REDIS_CLIENT YES CACHE BOOL "Include Redis client using hiredis")
set(HTTP_SERVER YES CACHE BOOL "Include HTTP server using libmicrohttpd")
set(HTTP_CLIENT YES CACHE BOOL "Include HTTP client support using curl")
set(COMPILE_TESTS YES CACHE BOOL "Compile test framework")
set(COMPILE_STUBGEN YES CACHE BOOL "Compile the stubgenerator")
set(COMPILE_EXAMPLES YES CACHE BOOL "Compile example programs")

option(WITH_COVERAGE "Build with code coverage flags" ON)

# print actual settings
if(UNIX)
    message(STATUS "UNIX_DOMAIN_SOCKET_SERVER: ${UNIX_DOMAIN_SOCKET_SERVER}")
    message(STATUS "UNIX_DOMAIN_SOCKET_CLIENT: ${UNIX_DOMAIN_SOCKET_CLIENT}")
endif(UNIX)
message(STATUS "TCP_SOCKET_SERVER: ${TCP_SOCKET_SERVER}")
message(STATUS "TCP_SOCKET_CLIENT: ${TCP_SOCKET_CLIENT}")
message(STATUS "HTTP_SERVER: ${HTTP_SERVER}")
message(STATUS "HTTP_CLIENT: ${HTTP_CLIENT}")
message(STATUS "REDIS_SERVER: ${REDIS_SERVER}")
message(STATUS "REDIS_CLIENT: ${REDIS_CLIENT}")
if(UNIX)
    message(STATUS "UNIXDOMAINSOCKET_SERVER: ${UNIX_DOMAIN_SOCKET_SERVER}")
    message(STATUS "UNIXDOMAINSOCKET_CLIENT: ${UNIX_DOMAIN_SOCKET_CLIENT}")
endif(UNIX)
message(STATUS "COMPILE_TESTS: ${COMPILE_TESTS}")
message(STATUS "COMPILE_STUBGEN: ${COMPILE_STUBGEN}")
message(STATUS "COMPILE_EXAMPLES: ${COMPILE_EXAMPLES}")

# setup compiler settings && dependencies
include(CMakeCompilerSettings)
include(CMakeDependencies)
include(GNUInstallDirs)

# setup output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

# setup global include_directories (for backwards compatibility)
include_directories("${CMAKE_BINARY_DIR}/gen/")
include_directories("${CMAKE_BINARY_DIR}/gen/jsonrpccpp/common")

# the core framework
add_subdirectory(src/jsonrpccpp)

#the stubgenerator
if (COMPILE_STUBGEN)
    add_subdirectory(src/stubgenerator)
endif()

# setup examples
if (COMPILE_EXAMPLES)
    add_subdirectory(src/examples)
endif()

# setup test suite
if (COMPILE_TESTS)
	enable_testing()
	add_subdirectory(src/test)
endif()

# create documentation
if (DOXYGEN_FOUND)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc)
    message(STATUS "Found doxygen: ${DOXYGEN_EXECUTABLE}")
    configure_file("${PROJECT_SOURCE_DIR}/doc/doxyfile.in" "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/doc COMMENT "Generating API documentation")
endif(DOXYGEN_FOUND)

# setup uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

add_custom_target(
    uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

# CMake package
set(package_name "libjson-rpc-cpp")
include(CMakePackageConfigHelpers)
set(config_install_destination ${CMAKE_INSTALL_LIBDIR}/${package_name}/cmake)
configure_package_config_file(
	cmake/${package_name}Config.cmake.in ${package_name}Config.cmake
	INSTALL_DESTINATION ${config_install_destination}
)
write_basic_package_version_file(
	${package_name}ConfigVersion.cmake
	VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}
	COMPATIBILITY ExactVersion
)
install(
	EXPORT ${package_name}Targets
	DESTINATION ${config_install_destination}
	NAMESPACE ${package_name}::
)
install(
	FILES
	${CMAKE_CURRENT_BINARY_DIR}/${package_name}Config.cmake
	${CMAKE_CURRENT_BINARY_DIR}/${package_name}ConfigVersion.cmake
	DESTINATION ${config_install_destination}
)

# packaging stuff
include(CMakePackaging)
