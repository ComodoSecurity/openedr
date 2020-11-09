#
# Utility macros for Log4Cplus project
#

# Get Log4cplus version macro
# first param - path to include folder, we will rip version from version.h
macro(log4cplus_get_version _include_PATH vmajor vminor vpatch)
  file(STRINGS "${_include_PATH}/log4cplus/version.h" _log4cplus_VER_STRING_AUX REGEX ".*#define[ ]+LOG4CPLUS_VERSION[ ]+")
  string(REGEX MATCHALL "[0-9]+" _log4clpus_VER_LIST "${_log4cplus_VER_STRING_AUX}")
  list(LENGTH _log4clpus_VER_LIST _log4cplus_VER_LIST_LEN)
# we also count '4' from the name...
  if(_log4cplus_VER_LIST_LEN EQUAL 5)
    list(GET _log4clpus_VER_LIST 2 ${vmajor})
    list(GET _log4clpus_VER_LIST 3 ${vminor})
    list(GET _log4clpus_VER_LIST 4 ${vpatch})
  endif()
endmacro()
