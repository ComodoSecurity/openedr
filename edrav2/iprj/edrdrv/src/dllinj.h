//
//  edrav2.libdrv
//
//  Driver library for DLL injection
//
///
/// @file libdllinj API
///
/// @addtogroup libdllinj
/// @{
#pragma once

#include "procmon.h"

namespace cmd {
namespace dllinj {

// Internal interface for notification from procmon.
namespace detail {

//
// Callback from procmon (for internal usage)
//
void notifyOnProcessCreation(procmon::Context* pNewProcessCtx, procmon::Context* pParentProcessCtx);

//
// Callback from procmon (for internal usage)
//
void notifyOnProcessTermination(procmon::Context* pProcessCtx);

} // namespace detail


///
/// Set injected DLL list (reset old value).
///
/// @param dllList - TBD.
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS setInjectedDllList(List<DynUnicodeString>& dllList);

///
/// Controls the status of indected DLL verification.
///
/// @param fEnable - enable/disable flag.
///
void enableInjectedDllVerification(bool fEnable);

///
/// Initialization.
///
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS initialize();

///
/// Finalization
///
void finalize();

} // namespace dllinj
} // namespace cmd

/// @}
