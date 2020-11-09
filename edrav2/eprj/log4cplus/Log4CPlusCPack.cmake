set(CPACK_PACKAGE_NAME log4cplus)
set(CPACK_PACKAGE_VERSION "${log4cplus_version_major}.${log4cplus_version_minor}.${log4cplus_version_patch}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "log4cplus is a log4j-inspired logging library for C++")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${log4cplus_SOURCE_DIR}/README.md")

# This is for WiX so that it does not complain about unsupported extension
# of license file.
configure_file("${log4cplus_SOURCE_DIR}/LICENSE"
  "${log4cplus_BINARY_DIR}/LICENSE.txt" COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${log4cplus_BINARY_DIR}/LICENSE.txt")

set(CPACK_WIX_UPGRADE_GUID "DB673B4F-556C-4CEA-BA9B-48E899E55150")

include(CPack)
