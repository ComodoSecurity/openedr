//
// EDRAv2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer:
//

///
/// @file Crash handling declaration (via Crashpad)
/// File contains initCrashHandlers() function for integration of the
/// Crashpad library which handles most of abnormal exceptions.
/// Additionally, pure virtual call handler is registered and fire up
/// a manual call for the Crashpad's processing.
///

///
/// @addtogroup errors Errors processing
/// @{

#pragma once

namespace cmd {
namespace error {
namespace dump {
namespace crashpad_ {

///
/// Crashpad's client-side handlers initialization
///
/// @param sHandlerPath The path for Crashpad's handler binary file.
/// @param sDBPath The path to the Crashpad's database directory.
/// @return `true` on success. `false` on failure.
///
bool initCrashHandlers(const std::wstring& sHandlerPath, const std::wstring& sDBPath);

} // namespace crashpad_
} // namespace dump
} // namespace error
} // namespace cmd

/// @}
