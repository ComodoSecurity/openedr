#
# Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# 
#  http://aws.amazon.com/apache2.0
# 
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#


# for make uninstall
if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")
    message(FATAL_ERROR "Cannot find install manifest: ${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")
endif()

file(READ "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt" files)
string(REGEX REPLACE "[\r\n]" ";" files "${files}")

foreach(file ${files})
    message(STATUS "Uninstalling ${file}")
    if(EXISTS "${file}")
        file(REMOVE ${file})
        if (EXISTS "${file}")
            message(FATAL_ERROR "Problem when removing ${file}, please check your permissions")
        endif()
    else()
        message(STATUS "File ${file} does not exist.")
    endif()
endforeach()
