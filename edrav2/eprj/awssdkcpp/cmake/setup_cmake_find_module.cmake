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


add_project(AWSSDK "User friendly cmake creator")

# create a new version file for AWSSDK, then find_package will return latest PACKAGE_VERSION
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

file(APPEND
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    "set(AWSSDK_INSTALL_AS_SHARED_LIBS ${BUILD_SHARED_LIBS})\n")

file(WRITE
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/platformDeps.cmake"
"#
# Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the \"License\").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# 
#  http://aws.amazon.com/apache2.0
# 
# or in the \"license\" file accompanying this file. This file is distributed
# on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#\n"
    "set(AWSSDK_PLATFORM_DEPS_LIBS ${PLATFORM_DEP_LIBS_ABSTRACT_NAME})\n"
    "set(AWSSDK_CLIENT_LIBS ${CLIENT_LIBS_ABSTRACT_NAME})\n"
    "set(AWSSDK_CRYPTO_LIBS ${CRYPTO_LIBS_ABSTRACT_NAME})\n"
    "set(AWSSDK_ADDITIONAL_LIBS ${AWS_SDK_ADDITIONAL_LIBRARIES_ABSTRACT_NAME})\n"
    "set(AWSSDK_INSTALL_LIBDIR ${LIBRARY_DIRECTORY})\n"
    "set(AWSSDK_INSTALL_BINDIR ${BINARY_DIRECTORY})\n"
    "set(AWSSDK_INSTALL_INCLUDEDIR ${INCLUDE_DIRECTORY})\n"
    "set(AWSSDK_INSTALL_ARCHIVEDIR ${ARCHIVE_DIRECTORY})\n"
    )

if (NOT SIMPLE_INSTALL)
    file(APPEND
        "${CMAKE_CURRENT_BINARY_DIR}/platformDeps.cmake"
        "set(AWSSDK_PLATFORM_PREFIX ${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER})\n")
endif()

# copy version file to destination
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${LIBRARY_DIRECTORY}/cmake/${PROJECT_NAME}")

# platform external dependencies
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/platformDeps.cmake"
    DESTINATION "${LIBRARY_DIRECTORY}/cmake/${PROJECT_NAME}/")

# copy all cmake files to destination, these files include useful macros, functions and variables for users.
# useful macros and variables will be included in this cmake file for user to use
install(DIRECTORY "${AWS_NATIVE_SDK_ROOT}/cmake/" DESTINATION "${LIBRARY_DIRECTORY}/cmake/${PROJECT_NAME}")

# copy third party dependencies
if (BUILD_DEPS)
    if (NOT DEFINED CMAKE_INSTALL_LIBDIR)
        set(CMAKE_INSTALL_LIBDIR lib)
    endif()
    if (NOT DEFINED CMAKE_INSTALL_BINDIR)
        set(CMAKE_INSTALL_BINDIR bin)
    endif()
    if (EXISTS "${AWS_DEPS_INSTALL_DIR}/include")
        install(DIRECTORY "${AWS_DEPS_INSTALL_DIR}/include/" DESTINATION "${INCLUDE_DIRECTORY}")
    endif()
    if (EXISTS "${AWS_DEPS_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}")
        install(DIRECTORY "${AWS_DEPS_INSTALL_DIR}/${CMAKE_INSTALL_LIBDIR}/" DESTINATION "${LIBRARY_DIRECTORY}")
    endif()
    if (EXISTS "${AWS_DEPS_INSTALL_DIR}/${CMAKE_INSTALL_BINDIR}")
        install(DIRECTORY "${AWS_DEPS_INSTALL_DIR}/${CMAKE_INSTALL_BINDIR}/" DESTINATION "${BINARY_DIRECTORY}")
    endif()
endif()

# following two files are vital for cmake to find correct package, but since we copied all files from above
# we left the code here to give you bettern understanding
#install(
#    FILES "${AWS_NATIVE_SDK_ROOT}/cmake/${PROJECT_NAME}Config.cmake"
#    DESTINATION "${LIBRARY_DIRECTORY}/cmake/${PROJECT_NAME}")

# to make compile time settings consistent with user usage time settings, we copy common settings to 
# destination. These settings will be included by ${PROJECT_NAME}-config.cmake

# internal dependencies
#install(
#    FILES "${AWS_NATIVE_SDK_ROOT}/cmake/sdksCommon.cmake"
#    DESTINATION "${LIBRARY_DIRECTORY}/cmake/${PROJECT_NAME}/")
