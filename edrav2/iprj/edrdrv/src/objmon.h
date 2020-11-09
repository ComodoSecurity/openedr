//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (31.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Processes access filter (self protection)
///
/// @addtogroup edrdrv
/// @{
#pragma once

#include "procmon.h"

namespace openEdr {
namespace objmon {

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
/// Initialization.
///
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS initialize();

///
/// Finalization.
///
void finalize();

} // namespace objmon
} // namespace openEdr
/// @}

