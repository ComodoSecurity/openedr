if (UNICODE)
  add_definitions (-DUNICODE -D_UNICODE -UMBCS -U_MBCS)
endif (UNICODE)

message (STATUS "Threads: ${CMAKE_THREAD_LIBS_INIT}")

set (loggingserver_sources loggingserver.cxx)

message (STATUS "Sources: ${loggingserver_sources}")

add_executable (loggingserver ${loggingserver_sources})
target_link_libraries (loggingserver ${log4cplus})

install(TARGETS loggingserver DESTINATION ${CMAKE_INSTALL_BINDIR})
