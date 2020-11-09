//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Devices dispatcher (demultiplexor)
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace openEdr {
namespace devdisp {

///
/// Initialize.
///
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS initialize();

///
/// Finalize.
///
void finalize();

} // namespace devdisp
} // namespace openEdr
/// @}
