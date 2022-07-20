//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Registry filter
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace cmd {
namespace regmon {

///
/// Update rules.
///
/// @param pData - a raw pointer to the data.
/// @param nDataSize - a size of the data.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS updateRegRules(const void* pData, size_t nDataSize);

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

} // namespace regmon
} // namespace cmd
/// @}
