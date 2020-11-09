# Copyright 2018, 2019 Peter Dimov
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at http://boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.5)

project(CmakeConfigPythonTest LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../BoostVersion.cmake)

find_package(PythonLibs 2.7 REQUIRED)

if(USE_BOOST_PACKAGE)

  find_package(Boost ${BOOST_VERSION} EXACT REQUIRED COMPONENTS python)

else()

  find_package(boost_python ${BOOST_VERSION} EXACT CONFIG REQUIRED)

endif()

add_executable(main quick.cpp)
target_link_libraries(main Boost::python ${PYTHON_LIBRARIES})
target_include_directories(main PRIVATE ${PYTHON_INCLUDE_DIRS})

enable_testing()
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>)

add_test(main main)
