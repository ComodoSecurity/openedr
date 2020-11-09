set (log4cplus_sources
  appenderattachableimpl.cxx
  appender.cxx
  asyncappender.cxx
  callbackappender.cxx
  clogger.cxx
  configurator.cxx
  connectorthread.cxx
  consoleappender.cxx
  cygwin-win32.cxx
  env.cxx
  factory.cxx
  fileappender.cxx
  fileinfo.cxx
  filter.cxx
  global-init.cxx
  hierarchy.cxx
  hierarchylocker.cxx
  layout.cxx
  log4judpappender.cxx
  lockfile.cxx
  logger.cxx
  loggerimpl.cxx
  loggingevent.cxx
  loggingmacros.cxx
  loglevel.cxx
  loglog.cxx
  mdc.cxx
  ndc.cxx
  nullappender.cxx
  objectregistry.cxx
  patternlayout.cxx
  pointer.cxx
  property.cxx
  queue.cxx
  rootlogger.cxx
  snprintf.cxx
  socketappender.cxx
  socketbuffer.cxx
  socket.cxx
  stringhelper.cxx
  stringhelper-clocale.cxx
  stringhelper-cxxlocale.cxx
  stringhelper-iconv.cxx
  syncprims.cxx
  syslogappender.cxx
  threads.cxx
  timehelper.cxx
  tls.cxx
  version.cxx)

#message (STATUS "Type: ${UNIX}|${CYGWIN}|${WIN32}")

if ("${UNIX}" OR "${CYGWIN}")
  set (log4cplus_sources ${log4cplus_sources}
    socket-unix.cxx)
elseif (WIN32)
  set (log4cplus_sources ${log4cplus_sources}
    nteventlogappender.cxx
    socket-win32.cxx
    win32consoleappender.cxx
    win32debugappender.cxx)

  #add_definitions (-DLOG4CPLUS_STATIC)
  #set (log4cplus_postfix "${log4cplus_postfix}S")
endif ()

# Define _GNU_SOURCE so that functions like `pipe2()` are visible.
add_definitions (-D_GNU_SOURCE=1)
if (UNICODE)
  add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
endif (UNICODE)
if (WIN32)
  add_definitions (-DMINGW_HAS_SECURE_API=1)
  add_definitions (-D_WIN32_WINNT=${_WIN32_WINNT})

  if (BUILD_SHARED_LIBS)
    set(log4cplus_build_shared 1)
  else ()
    set(log4cplus_build_shared 0)
  endif ()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/log4cplus.rc.in ${CMAKE_CURRENT_BINARY_DIR}/log4cplus.rc @ONLY)
  list(APPEND log4cplus_sources ${CMAKE_CURRENT_BINARY_DIR}/log4cplus.rc)
endif (WIN32)

add_library (${log4cplus} ${log4cplus_sources})

set (log4cplus_LIBS ${CMAKE_THREAD_LIBS_INIT})
if (LIBRT)
  list (APPEND log4cplus_LIBS ${LIBRT})
endif ()
if (LIBPOSIX4)
  list (APPEND log4cplus_LIBS ${LIBPOSIX4})
endif ()
if (LIBCPOSIX)
  list (APPEND log4cplus_LIBS ${LIBCPOSIX})
endif ()
if (LIBSOCKET)
  list (APPEND log4cplus_LIBS ${LIBSOCKET})
endif ()
if (LIBNSL)
  list (APPEND log4cplus_LIBS ${LIBNSL})
endif ()
if (LOG4CPLUS_WITH_ICONV AND LIBICONV)
  list (APPEND log4cplus_LIBS ${LIBICONV})
endif ()
target_link_libraries (${log4cplus} ${log4cplus_LIBS})

if (ANDROID)
  # Android does not seem to have SO version support.
  set_target_properties (${log4cplus} PROPERTIES
    COMPILE_FLAGS "-DINSIDE_LOG4CPLUS")
else ()
  set_target_properties (${log4cplus} PROPERTIES
    VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}"
    SOVERSION "${log4cplus_soversion}"
    COMPILE_FLAGS "-DINSIDE_LOG4CPLUS")
endif ()

if (MINGW AND LOG4CPLUS_MINGW_STATIC_RUNTIME)
  # avoid dependency from mingw libstdc++/libgcc in resulting dll
  set_target_properties (${log4cplus} PROPERTIES
    LINK_FLAGS "-static -static-libgcc -static-libstdc++")
endif ()

if (WIN32)
  set_target_properties (${log4cplus} PROPERTIES
    DEBUG_POSTFIX "D")
  target_link_libraries (${log4cplus} ws2_32 advapi32)
endif ()

###
# Installation (https://github.com/forexample/package-example)

set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${version_config}"
    VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}"
    COMPATIBILITY SameMajorVersion
)

# Note: variable 'targets_export_name' used
configure_file("cmake/Config.cmake.in" "${project_config}" @ONLY)

install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

install(
    EXPORT "${targets_export_name}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)

install(TARGETS ${log4cplus} EXPORT "${targets_export_name}"
                          INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES ../include/log4cplus/appender.h
              ../include/log4cplus/asyncappender.h
              ../include/log4cplus/callbackappender.h
              ../include/log4cplus/clogger.h
              ../include/log4cplus/config.hxx
              ../include/log4cplus/configurator.h
              ../include/log4cplus/consoleappender.h
              ../include/log4cplus/fileappender.h
              ../include/log4cplus/fstreams.h
              ../include/log4cplus/hierarchy.h
              ../include/log4cplus/hierarchylocker.h
              ../include/log4cplus/initializer.h
              ../include/log4cplus/layout.h
              ../include/log4cplus/log4cplus.h
              ../include/log4cplus/log4judpappender.h
              ../include/log4cplus/logger.h
              ../include/log4cplus/loggingmacros.h
              ../include/log4cplus/loglevel.h
              ../include/log4cplus/mdc.h
              ../include/log4cplus/ndc.h
              ../include/log4cplus/nteventlogappender.h
              ../include/log4cplus/nullappender.h
              ../include/log4cplus/socketappender.h
              ../include/log4cplus/streams.h
              ../include/log4cplus/syslogappender.h
              ../include/log4cplus/tchar.h
              ../include/log4cplus/tracelogger.h
              ../include/log4cplus/tstring.h
              ../include/log4cplus/version.h
              ../include/log4cplus/win32debugappender.h
              ../include/log4cplus/win32consoleappender.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus )

install(FILES ../include/log4cplus/boost/deviceappender.hxx
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/boost )


install(FILES ../include/log4cplus/helpers/appenderattachableimpl.h
              ../include/log4cplus/helpers/connectorthread.h
              ../include/log4cplus/helpers/fileinfo.h
              ../include/log4cplus/helpers/lockfile.h
              ../include/log4cplus/helpers/loglog.h
              ../include/log4cplus/helpers/pointer.h
              ../include/log4cplus/helpers/property.h
              ../include/log4cplus/helpers/queue.h
              ../include/log4cplus/helpers/snprintf.h
              ../include/log4cplus/helpers/socket.h
              ../include/log4cplus/helpers/socketbuffer.h
              ../include/log4cplus/helpers/stringhelper.h
              ../include/log4cplus/helpers/thread-config.h
              ../include/log4cplus/helpers/timehelper.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/helpers )

install(FILES ../include/log4cplus/internal/env.h
              ../include/log4cplus/internal/internal.h
              ../include/log4cplus/internal/socket.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/internal )

install(FILES ../include/log4cplus/spi/appenderattachable.h
              ../include/log4cplus/spi/factory.h
              ../include/log4cplus/spi/filter.h
              ../include/log4cplus/spi/loggerfactory.h
              ../include/log4cplus/spi/loggerimpl.h
              ../include/log4cplus/spi/loggingevent.h
              ../include/log4cplus/spi/objectregistry.h
              ../include/log4cplus/spi/rootlogger.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/spi )

install(FILES ../include/log4cplus/thread/impl/syncprims-cxx11.h
              ../include/log4cplus/thread/impl/syncprims-impl.h
              ../include/log4cplus/thread/impl/syncprims-pmsm.h
              ../include/log4cplus/thread/impl/threads-impl.h
              ../include/log4cplus/thread/impl/tls.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/thread/impl )

install(FILES ../include/log4cplus/thread/syncprims-pub-impl.h
              ../include/log4cplus/thread/syncprims.h
              ../include/log4cplus/thread/threads.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/thread )

install(FILES ../include/log4cplus/config/macosx.h
              ../include/log4cplus/config/win32.h
              ../include/log4cplus/config/windowsh-inc.h
              ${log4cplus_BINARY_DIR}/include/log4cplus/config/defines.hxx
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/log4cplus/config )
