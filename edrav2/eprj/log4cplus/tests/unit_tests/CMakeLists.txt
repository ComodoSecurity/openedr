add_executable (unit_tests unit_tests.cxx)
target_link_libraries (unit_tests ${log4cplus})
add_test(NAME unit_tests
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  COMMAND unit_tests)
