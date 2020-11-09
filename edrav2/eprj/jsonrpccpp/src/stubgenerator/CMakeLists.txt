file(GLOB_RECURSE jsonrpcstub_source *.cpp)
file(GLOB_RECURSE jsonrpcstub_header *.h)
list(REMOVE_ITEM jsonrpcstub_source "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

include_directories(..)
include_directories(${ARGTABLE_INCLUDE_DIRS})

set(ALL_LIBS)


if (BUILD_SHARED_LIBS)
        add_library(libjsonrpcstub SHARED ${jsonrpcstub_source} ${jsonrpcstub_header} ${jsonrpc_helper_source_common})
        target_link_libraries(libjsonrpcstub jsonrpccommon ${ARGTABLE_LIBRARIES})
        set_target_properties(libjsonrpcstub PROPERTIES OUTPUT_NAME jsonrpccpp-stub)
        list(APPEND ALL_LIBS libjsonrpcstub)
endif()

# setup static common library
if (BUILD_STATIC_LIBS OR MSVC)
        add_library(libjsonrpcstubStatic STATIC ${jsonrpcstub_source} ${jsonrpcstub_header} ${jsonrpc_helper_source_common})
        target_link_libraries(libjsonrpcstubStatic common ${ARGTABLE_LIBRARIES})
        set_target_properties(libjsonrpcstubStatic PROPERTIES OUTPUT_NAME jsonrpccpp-stub)

    if (NOT BUILD_SHARED_LIBS)
        add_library(libjsonrpcstub ALIAS libjsonrpcstubStatic)
    endif()

    list(APPEND ALL_LIBS libjsonrpcstubStatic)
endif()

set(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})
set_target_properties(${ALL_LIBS} PROPERTIES VERSION "${VERSION_STRING}" SOVERSION "${SO_VERSION}")

add_executable(jsonrpcstub main.cpp)
if (BUILD_STATIC_LIBS AND NOT BUILD_SHARED_LIBS)
        target_link_libraries(jsonrpcstub jsonrpccommonStatic libjsonrpcstubStatic )
else()
        target_link_libraries(jsonrpcstub jsonrpccommon libjsonrpcstub )
endif()
#Generate manpage
if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    configure_file("${CMAKE_SOURCE_DIR}/doc/manpage.in" "${CMAKE_BINARY_DIR}/jsonrpcstub.1" @ONLY)
    add_custom_target(manpage ALL DEPENDS ${CMAKE_BINARY_DIR}/jsonrpcstub.1)
    install(FILES ${CMAKE_BINARY_DIR}/jsonrpcstub.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1/)
    add_dependencies(jsonrpcstub manpage)
endif()

configure_file(${CMAKE_SOURCE_DIR}/cmake/libjsonrpccpp-stub.pc.cmake ${CMAKE_BINARY_DIR}/libjsonrpccpp-stub.pc)

install(FILES
    "${CMAKE_BINARY_DIR}/libjsonrpccpp-stub.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/stubgenerator/
    DESTINATION include/jsonrpccpp/stubgen
    FILES_MATCHING PATTERN "*.h")

install(TARGETS ${ALL_LIBS} jsonrpcstub
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION bin
)
