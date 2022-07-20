//
//  edrav2.edrdrv project
//
///
/// @file Wrapper of nfwfpdrv.lib
///
/// @addtogroup edrdrv
/// @{
#include "common.h"

#include "netmon.h"
#if defined(FEATURE_ENABLE_NETMON)
#include <nfwfpdrv.h>
#else
#pragma warning (disable:4100)
#pragma warning (disable:4505)
#include "ioctl.h"
#endif


namespace cmd {
namespace netmon {

//
//
//
NTSTATUS initialize()
{
#if defined(FEATURE_ENABLE_NETMON)
	if (g_pCommonData->fNetMonStarted)
	{
		ASSERT(false);
		return STATUS_SUCCESS;
	}

	LOGINFO2("Register network monitor\r\n");

	IFERR_RET(nfwfpdrvDriverEntry(g_pCommonData->pDriverObject, (PUNICODE_STRING)g_pCommonData->usRegistryPath));

	g_pCommonData->fNetMonStarted = TRUE;
#endif
	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	if (!g_pCommonData->fNetMonStarted)
		return;

#if defined(FEATURE_ENABLE_NETMON)
	nfwfpdrvDriverUnload(g_pCommonData->pDriverObject);
#endif
}

namespace detail {

//
//
//
bool isSupportDevice(_DEVICE_OBJECT * pDeviceObject)
{
#if defined(FEATURE_ENABLE_NETMON)
	return pDeviceObject == devctrl_getDeviceObject();
#else
	return false;
#endif
}

//
//
//
NTSTATUS dispatchIrp(_DEVICE_OBJECT * pDeviceObject, _IRP * pIrp)
{
#if defined(FEATURE_ENABLE_NETMON)
	return devctrl_dispatch(pDeviceObject, pIrp);
#else
	return STATUS_SUCCESS;
#endif
}

} // namespace detail


} // namespace dllinj
} // namespace cmd

/// @}
