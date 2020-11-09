//
// EDRAv2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer: Denis Bogdanov (17.01.2019)
//
///
/// @file Crash handling declaration (Windows only)
///
//
// Crash handlers initialization (for Windows platform)
//
// File contains crash handlers and initCrashHandlers() function for
// its registration on Windows platform. In case of abnormal exception
// in application, handlers call MinidumpWriter to save a dump.
//
/// @addtogroup errors Errors processing
/// @{
#pragma once

#ifndef _WIN32
#error You should include this file only into windows-specific code
#endif // _WIN32

namespace openEdr {
namespace error {
namespace dump {
namespace win {

/// This function registers handlers for these types of exceptional
/// situations:
///  * SIGABRT signal
///  * SIGTERM signal
///  * SEH
///  * pure virtual calls
///  * invalid parameter
///
/// @param vConfig [opt] - the configuration (default: blank) including the following
/// fields:
///   **prefix** [opt] - prefix of the dumpfile's name. If parameter is 
///     omitted current module's name is used.
///   **dumpDirectory** [opt] - directory to save dumps. If parameter is omitted 
///     directory 'dump' is used inside current module's directory.
///
void initCrashHandlers(const Variant& vConfig = {});

} // namespace win
} // namespace dump
} // namespace error
} // namespace openEdr
/// @}
