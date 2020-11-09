set (qt4debugappender_sources
  qt4debugappender.cxx)

if (${BUILD_SHARED_LIBS})
  add_definitions (-D${log4cplus}_EXPORTS)
endif ()
if (UNICODE)
  add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
endif (UNICODE)

find_package (Qt4 REQUIRED)
include (${QT_USE_FILE})

set (qt4debugappender log4cplusqt4debugappender${log4cplus_postfix})
add_library (${qt4debugappender} ${qt4debugappender_sources})
target_link_libraries (${qt4debugappender}
  ${log4cplus}
  ${QT_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

set_target_properties (${qt4debugappender} PROPERTIES
  VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}"
  SOVERSION "${log4cplus_soversion}"
  COMPILE_FLAGS "-DINSIDE_LOG4CPLUS_QT4DEBUGAPPENDER")

if (WIN32)
  set_target_properties (${qt4debugappender} PROPERTIES
    DEBUG_POSTFIX "D")
endif ()

install (TARGETS ${qt4debugappender}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install (FILES ../include/log4cplus/qt4debugappender.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus)
