# Copyright (c) Glyn Matthews 2012-2016.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(Uri_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/uri.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/uri_builder.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/uri_errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/detail/uri_parse.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/detail/uri_parse_authority.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/detail/uri_advance_parts.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/detail/uri_normalize.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/detail/uri_resolve.cpp
  )

add_library(network-uri ${Uri_SRCS})
target_link_libraries(network-uri)
if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
  if (NOT Uri_DISABLE_LIBCXX)
    target_link_libraries(network-uri "c++")
  endif()
endif()
target_include_directories(network-uri PUBLIC ../include)
target_include_directories(network-uri PUBLIC ${Boost_INCLUDE_DIR})

#propagate sources to parent scope for one-lib-build
set(Uri_SRCS ${Uri_SRCS} PARENT_SCOPE)

install(
  TARGETS network-uri
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
