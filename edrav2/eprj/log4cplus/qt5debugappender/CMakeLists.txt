set (qt5debugappender_sources
  qt5debugappender.cxx)

if (${BUILD_SHARED_LIBS})
  add_definitions (-D${log4cplus}_EXPORTS)
endif ()
if (UNICODE)
  add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
endif (UNICODE)

find_package (Qt5Core REQUIRED)
#include (${QT_USE_FILE})

set (qt5debugappender log4cplusqt5debugappender${log4cplus_postfix})
add_library (${qt5debugappender} ${qt5debugappender_sources})
target_link_libraries (${qt5debugappender}
  ${log4cplus}
  ${Qt5Widgets_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

set_target_properties (${qt5debugappender} PROPERTIES
  VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}"
  SOVERSION "${log4cplus_soversion}"
  COMPILE_FLAGS "-DINSIDE_LOG4CPLUS_QT5DEBUGAPPENDER")

qt5_use_modules(${qt5debugappender} Core)

if (WIN32)
  set_target_properties (${qt5debugappender} PROPERTIES
    DEBUG_POSTFIX "D")
endif ()

install (TARGETS ${qt5debugappender}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install (FILES ../include/log4cplus/qt5debugappender.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus)
