//
//  edrav2.edrdrv project
//
///
/// @file Wrapper of nfwfpdrv.lib
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include <nfwfpdrv.h>
#include "netmon.h"

namespace openEdr {
namespace netmon {

//
//
//
NTSTATUS initialize()
{
	if (g_pCommonData->fNetMonStarted)
	{
		ASSERT(false);
		return STATUS_SUCCESS;
	}

	LOGINFO2("Register network monitor\r\n");

	IFERR_RET(nfwfpdrvDriverEntry(g_pCommonData->pDriverObject, (PUNICODE_STRING)g_pCommonData->usRegistryPath));

	g_pCommonData->fNetMonStarted = TRUE;
	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (!g_pCommonData->fNetMonStarted)
		return;
	nfwfpdrvDriverUnload(g_pCommonData->pDriverObject);
}

namespace detail {

//
//
//
bool isSupportDevice(_DEVICE_OBJECT * pDeviceObject)
{
	return pDeviceObject == devctrl_getDeviceObject();
}

//
//
//
NTSTATUS dispatchIrp(_DEVICE_OBJECT * pDeviceObject, _IRP * pIrp)
{
	return devctrl_dispatch(pDeviceObject, pIrp);
}

} // namespace detail


} // namespace dllinj
} // namespace openEdr

/// @}
