//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Driver IOCTL
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace cmd {
namespace drvioctl {

///
/// Initialization.
///
/// @return The function returns NTSTATUS of the operation.
///
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS initialize();

///
/// Finalization.
///
void finalize();

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

} // namespace drvioctl
} // namespace cmd
/// @}
