//
//  edrav2.edrdrv project
//
///
/// @file Wrapper of nfwfpdrv.lib
///
/// @addtogroup edrdrv
/// @{
#pragma once

//#define FEATURE_ENABLE_NETMON

#if defined(FEATURE_ENABLE_NETMON)
#pragma comment(lib, "nfwfpdrv.lib")
#endif //defined(FEATURE_ENABLE_NETMON)

namespace cmd {
namespace netmon {

// Internal interface for device dispatcher
namespace detail {

//
// Callback for check device support
//
bool isSupportDevice(_DEVICE_OBJECT * pDeviceObject);

//
// Process IRP of own device 
//
NTSTATUS dispatchIrp(_DEVICE_OBJECT * pDeviceObject, _IRP * pIrp);

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

} // namespace netmon
} // namespace cmd

/// @}
