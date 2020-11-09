# setup common headers and sources
file(GLOB jsonrpc_header *.h)
file(GLOB jsonrpc_header_common common/*.h)
file(GLOB jsonrpc_source_common common/*.c*)

# setup server headers and sources
file(GLOB jsonrpc_install_header_server
	server/requesthandlerfactory.h
	server/abstractserver.h
	server/abstractserverconnector.h
        server/abstractthreadedserver.h
	server/iprocedureinvokationhandler.h
	server/iclientconnectionhandler.h
        server/threadpool.h
)
file(GLOB jsonrpc_header_server server/*.h)
file(GLOB jsonrpc_source_server server/*.c*)

# setup client headers and sources
file(GLOB jsonrpc_install_header_client
    client/batchcall.h
    client/batchresponse.h
    client/client.h
    client/iclientconnector.h
)
file(GLOB jsonrpc_header_client client/*.h)
file(GLOB jsonrpc_source_client client/*.c*)

# setup connector variables defaults
set(client_connector_source "")
set(client_connector_header "")
set(client_connector_libs   "")
set(server_connector_source "")
set(server_connector_header "")
set(server_connector_libs   "")

# setup sources for http connectors
if (HTTP_CLIENT)
	list(APPEND client_connector_header "client/connectors/httpclient.h")
	list(APPEND client_connector_source "client/connectors/httpclient.cpp")
	list(APPEND client_connector_libs ${CURL_LIBRARIES})
endif()

if (HTTP_SERVER)
	list(APPEND server_connector_header "server/connectors/httpserver.h")
	list(APPEND server_connector_source "server/connectors/httpserver.cpp")
	list(APPEND server_connector_libs ${CMAKE_THREAD_LIBS_INIT} ${MHD_LIBRARIES})
endif()

# setup sources for redis connectors
if (REDIS_CLIENT)
    list(APPEND client_connector_header "client/connectors/redisclient.h")
    list(APPEND client_connector_source "client/connectors/redisclient.cpp")
    list(APPEND client_connector_libs ${HIREDIS_LIBRARIES})
    include_directories(${HIREDIS_INCLUDE_DIRS})
endif()

if (REDIS_SERVER)
    list(APPEND server_connector_header "server/connectors/redisserver.h")
    list(APPEND server_connector_source "server/connectors/redisserver.cpp")
    list(APPEND server_connector_libs ${CMAKE_THREAD_LIBS_INIT} ${HIREDIS_LIBRARIES})
endif()

# setup sources for unix domain socket connectors
if (UNIX_DOMAIN_SOCKET_SERVER)
    list(APPEND server_connector_header "server/connectors/unixdomainsocketserver.h")
    list(APPEND server_connector_source "server/connectors/unixdomainsocketserver.cpp")
    list(APPEND server_connector_libs ${CMAKE_THREAD_LIBS_INIT})
endif()

if (UNIX_DOMAIN_SOCKET_CLIENT)
    list(APPEND client_connector_header "client/connectors/unixdomainsocketclient.h")
    list(APPEND client_connector_source "client/connectors/unixdomainsocketclient.cpp")
endif()

# setup sources for tcp socket connectors
if (TCP_SOCKET_SERVER)
    list(APPEND server_connector_header "server/connectors/tcpsocketserver.h")
    list(APPEND server_connector_source "server/connectors/tcpsocketserver.cpp")
    if (WIN32)
        list(APPEND server_connector_header "server/connectors/windowstcpsocketserver.h")
        list(APPEND server_connector_source "server/connectors/windowstcpsocketserver.cpp")
        list(APPEND server_connector_libs ws2_32)
    endif()
    if(UNIX)
        list(APPEND server_connector_header "server/connectors/linuxtcpsocketserver.h")
        list(APPEND server_connector_source "server/connectors/linuxtcpsocketserver.cpp")
    endif()
    list(APPEND server_connector_libs ${CMAKE_THREAD_LIBS_INIT})
endif()

if (TCP_SOCKET_CLIENT)
    list(APPEND client_connector_header "client/connectors/tcpsocketclient.h")
    list(APPEND client_connector_source "client/connectors/tcpsocketclient.cpp")
    if (WIN32)
        list(APPEND client_connector_header "client/connectors/windowstcpsocketclient.h")
        list(APPEND client_connector_source "client/connectors/windowstcpsocketclient.cpp")
        list(APPEND client_connector_libs ws2_32)
    endif()
    if(UNIX)
        list(APPEND client_connector_header "client/connectors/linuxtcpsocketclient.h")
        list(APPEND client_connector_source "client/connectors/linuxtcpsocketclient.cpp")
    endif()
    list(APPEND client_connector_libs ${CMAKE_THREAD_LIBS_INIT})
endif()

if (FILE_DESCRIPTOR_SERVER)
    list(APPEND server_connector_header "server/connectors/filedescriptorserver.h")
    list(APPEND server_connector_source "server/connectors/filedescriptorserver.cpp")
    list(APPEND server_connector_libs ${CMAKE_THREAD_LIBS_INIT})
endif()

if (FILE_DESCRIPTOR_CLIENT)
    list(APPEND client_connector_header "client/connectors/filedescriptorclient.h")
    list(APPEND client_connector_source "client/connectors/filedescriptorclient.cpp")
    list(APPEND client_connector_libs ${CMAKE_THREAD_LIBS_INIT})
endif()

# configure a header file to pass some of the CMake settings to the source code
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/gen/jsonrpccpp/common")
configure_file("${PROJECT_SOURCE_DIR}/src/jsonrpccpp/version.h.in" "${CMAKE_BINARY_DIR}/gen/jsonrpccpp/version.h")
configure_file("${PROJECT_SOURCE_DIR}/src/jsonrpccpp/common/jsonparser.h.in" "${CMAKE_BINARY_DIR}/gen/jsonrpccpp/common/jsonparser.h")

install(FILES "${CMAKE_BINARY_DIR}/gen/jsonrpccpp/version.h" DESTINATION include/jsonrpccpp)
install(FILES "${PROJECT_BINARY_DIR}/gen/jsonrpccpp/common/jsonparser.h" DESTINATION include/jsonrpccpp/common)

# include required directories
include_directories(..)
include_directories(${MHD_INCLUDE_DIRS})

# setup shared common library
if (BUILD_SHARED_LIBS)
	add_library(jsonrpccommon SHARED ${jsonrpc_source_common} ${jsonrpc_header} ${jsonrpc_helper_source_common})
	target_link_libraries(jsonrpccommon jsoncpp_lib_static)
	set_target_properties(jsonrpccommon PROPERTIES OUTPUT_NAME jsonrpccpp-common)
endif()

# setup static common library
if (BUILD_STATIC_LIBS OR MSVC)
	add_library(common STATIC ${jsonrpc_source_common} ${jsonrpc_header} ${jsonrpc_helper_source_common})
	target_link_libraries(common jsoncpp_lib_static)
	set_target_properties(common PROPERTIES OUTPUT_NAME jsonrpccpp-common)

	if (NOT BUILD_SHARED_LIBS)
		add_library(jsonrpccommon ALIAS common)
	endif()
endif()

# setup shared client library
if (BUILD_SHARED_LIBS)
    add_library(jsonrpcclient SHARED ${jsonrpc_source_client} ${jsonrpc_header} ${jsonrpc_header_client} ${client_connector_source})
    add_dependencies(jsonrpcclient jsonrpccommon)
    target_link_libraries(jsonrpcclient jsonrpccommon ${client_connector_libs})
    set_target_properties(jsonrpcclient PROPERTIES OUTPUT_NAME jsonrpccpp-client)
endif()

# setup static client library
if (BUILD_STATIC_LIBS OR MSVC)
	add_library(client STATIC ${jsonrpc_source_client} ${jsonrpc_header} ${jsonrpc_header_client} ${client_connector_source})
	target_link_libraries(client common ${client_connector_libs})
	set_target_properties(client PROPERTIES OUTPUT_NAME jsonrpccpp-client)

	if (NOT BUILD_SHARED_LIBS)
		add_library(jsonrpcclient ALIAS client)
	endif()
endif()

# setup shared server library
if (BUILD_SHARED_LIBS)
    add_library(jsonrpcserver SHARED ${jsonrpc_source_server} ${jsonrpc_header} ${jsonrpc_header_server} ${server_connector_source})
    add_dependencies(jsonrpcserver jsonrpccommon)
    target_link_libraries(jsonrpcserver jsonrpccommon ${server_connector_libs})
    set_target_properties(jsonrpcserver PROPERTIES OUTPUT_NAME jsonrpccpp-server)
endif()

# setup static server library
if (BUILD_STATIC_LIBS OR MSVC)
	add_library(server STATIC ${jsonrpc_source_server} ${jsonrpc_header} ${jsonrpc_header_server} ${server_connector_source})
	target_link_libraries(server common ${server_connector_libs})
	set_target_properties(server PROPERTIES OUTPUT_NAME jsonrpccpp-server)

	if (NOT BUILD_SHARED_LIBS)
		add_library(jsonrpcserver ALIAS server)
	endif()
endif()

set(ALL_LIBS)

if (BUILD_SHARED_LIBS OR NOT BUILD_STATIC_LIBS)
    list(APPEND ALL_LIBS jsonrpccommon jsonrpcclient jsonrpcserver)
endif()

if (BUILD_STATIC_LIBS OR MSVC)
	list(APPEND ALL_LIBS common client server)
endif()

# setup version
set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
set_target_properties(
	${ALL_LIBS}
	PROPERTIES VERSION "${VERSION_STRING}" SOVERSION "${SO_VERSION}"
)

# install libraries
install(FILES ${jsonrpc_header}                 DESTINATION include/jsonrpccpp)
install(FILES ${jsonrpc_header_common}          DESTINATION include/jsonrpccpp/common)
install(FILES ${jsonrpc_helper_header_common}   DESTINATION include/jsonrpccpp/common/helper)
install(FILES ${jsonrpc_install_header_client}  DESTINATION include/jsonrpccpp/client)
install(FILES ${jsonrpc_install_header_server}  DESTINATION include/jsonrpccpp/server)
install(FILES ${client_connector_header}        DESTINATION include/jsonrpccpp/client/connectors)
install(FILES ${server_connector_header}        DESTINATION include/jsonrpccpp/server/connectors)

if (WIN32 AND NOT HUNTER_ENABLED)
	# On Windows copy manually installed dependencies unless Hunter is used.
	file(GLOB win32_libs ${CMAKE_SOURCE_DIR}/win32-deps/lib/*.dll)
	file(COPY ${win32_libs} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	install(FILES ${win32_libs} DESTINATION bin)
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/win32-deps/include DESTINATION .)
endif()

install(TARGETS ${ALL_LIBS} EXPORT libjson-rpc-cppTargets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION bin
)

#set pkg-config
get_filename_component(FULL_PATH_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
set(FULL_PATH_INCLUDEDIR "${FULL_PATH_INSTALL_PREFIX}/include")
set(FULL_PATH_LIBDIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

configure_file(${PROJECT_SOURCE_DIR}/cmake/libjsonrpccpp-client.pc.cmake ${CMAKE_BINARY_DIR}/libjsonrpccpp-client.pc)
configure_file(${PROJECT_SOURCE_DIR}/cmake/libjsonrpccpp-server.pc.cmake ${CMAKE_BINARY_DIR}/libjsonrpccpp-server.pc)
configure_file(${PROJECT_SOURCE_DIR}/cmake/libjsonrpccpp-common.pc.cmake ${CMAKE_BINARY_DIR}/libjsonrpccpp-common.pc)

INSTALL(FILES
    "${CMAKE_BINARY_DIR}/libjsonrpccpp-server.pc"
    "${CMAKE_BINARY_DIR}/libjsonrpccpp-client.pc"
    "${CMAKE_BINARY_DIR}/libjsonrpccpp-common.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
