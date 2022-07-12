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
#include "common.h"
#include "osutils.h"
#include "ioctl.h"
#include "netmon.h"

namespace cmd {
namespace devdisp {


DRIVER_DISPATCH dispatchDeviceIrp;

//
// Demultiplexor for several devices
//

NTSTATUS dispatchDeviceIrp(_In_ _DEVICE_OBJECT * pDeviceObject, _Inout_ _IRP * pIrp)
{
	// Process ioctl device for edrdrv
	if (drvioctl::detail::isSupportDevice(pDeviceObject))
		return drvioctl::detail::dispatchIrp(pDeviceObject, pIrp);

	// Process device for nfwfpdrv
	if (netmon::detail::isSupportDevice(pDeviceObject))
		return netmon::detail::dispatchIrp(pDeviceObject, pIrp);

	// rest IRP
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//
//
//
NTSTATUS initialize()
{
	LOGINFO2("Register devices dispatcher\r\n");

	// Set dispatching function
	for (size_t i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; ++i)
	{
		#pragma warning(suppress : 28168 28175)
		g_pCommonData->pDriverObject->MajorFunction[i] = static_cast<PDRIVER_DISPATCH> (dispatchDeviceIrp);
	}

	return STATUS_SUCCESS;
}

//
//
//
void finalize() noexcept
{
}

} // namespace devdisp
} // namespace cmd
/// @}
