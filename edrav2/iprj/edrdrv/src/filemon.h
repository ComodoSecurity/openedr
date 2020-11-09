//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (31.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file File filter
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace openEdr {
namespace filemon {

///
/// Update file rules.
///
/// @param pData - TBD.
/// @param nDataSize - TBD.
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS updateFileRules(const void* pData, size_t nDataSize);

///
/// Print all attached instances.
///
/// @returns The function returns NTSTATUS of the operation.
///
/// applyForEachInstance sample.
///
NTSTATUS printInstances();

///
/// Initialization.
///
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS initialize();

///
/// Finalization.
///
void finalize();

} // namespace filemon
} // namespace openEdr
/// @}
