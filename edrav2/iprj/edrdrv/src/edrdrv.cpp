//
// EDRAv2.edrflt project
//
// Author: Yury Podpruzhnikov (30.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Main file (DriverEntry)
///
/// @addtogroup edrdrv
///
/// @{
#include "common.h"
#include "osutils.h"
#include "filemon.h"
#include "regmon.h"
#include "procmon.h"
#include "objmon.h"
#include "netmon.h"
#include "devdisp.h"
#include "ioctl.h"
#include "fltport.h"
#include "config.h"
#include "dllinj.h"

//////////////////////////////////////////////////////////////////////////
//
// Predefinition
//
//////////////////////////////////////////////////////////////////////////

//EXTERN_C NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT pDriverObject, _In_ PUNICODE_STRING pusRegistryPath);
//
//#ifdef ALLOC_PRAGMA
//#pragma alloc_text(INIT, DriverEntry)
//#endif

namespace openEdr {

//////////////////////////////////////////////////////////////////////////
//
// Global data
//
//////////////////////////////////////////////////////////////////////////

CommonGlobalData* g_pCommonData;

//////////////////////////////////////////////////////////////////////////
//
// Driver Load/Unload routines
//
//////////////////////////////////////////////////////////////////////////

//
//
//
NTSTATUS startMonitoring()
{
	g_pCommonData->fEnableMonitoring = true;
	// The monitors are enabled by flag 
	fltport::start();

	return STATUS_SUCCESS;
}

//
//
//
void stopMonitoring(bool fResetQueue)
{
	g_pCommonData->fEnableMonitoring = false;
	// The monitors are disabled by flag 
	fltport::stop(fResetQueue);
}


//
//
//
VOID unloadDriver(_In_ PDRIVER_OBJECT /*pDriverObject*/)
{
	LOGINFO2("%s: Entered\r\n", __FUNCTION__);
	LOGINFO1("Unload\r\n");

	dllinj::finalize();
	netmon::finalize();
	drvioctl::finalize();
	objmon::finalize();
	regmon::finalize();
	filemon::finalize();
	procmon::finalize();
	devdisp::finalize();
	cfg::finalize();

	// Wait writting data
	{
		// Sleep 100 ms
		LARGE_INTEGER delay;
		delay.QuadPart = (LONGLONG)100 * -1 /*relative*/ * 1 * 10 * 1000 /*ms*/;
		KeDelayExecutionThread(KernelMode, FALSE, &delay);
	}
	log::finalize();

	delete g_pCommonData;
	g_pCommonData = nullptr;
}



//
// Gets procedures address
//
// TODO: move to global config initialize (but after logging initialize)
NTSTATUS getProceduresAddress()
{
	IFERR_RET(getProcAddressAndLog(U_STAT(L"ZwQueryInformationProcess"), false, &g_pCommonData->fnZwQueryInformationProcess));
	IFERR_RET(getProcAddressAndLog(U_STAT(L"ZwQuerySystemInformation"), false, &g_pCommonData->fnZwQuerySystemInformation));
	IFERR_RET(getProcAddressAndLog(U_STAT(L"CmCallbackGetKeyObjectIDEx"), true, &g_pCommonData->fnCmCallbackGetKeyObjectIDEx));
	IFERR_RET(getProcAddressAndLog(U_STAT(L"CmCallbackReleaseKeyObjectIDEx"), true, &g_pCommonData->fnCmCallbackReleaseKeyObjectIDEx));
	return STATUS_SUCCESS;
}

} // namespace openEdr

using namespace openEdr;

//
// DriverEntry 
//
_Function_class_(DRIVER_INITIALIZE)
_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
EXTERN_C NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT pDriverObject, _In_ PUNICODE_STRING pusRegistryPath)
{
	// Enable POOL_NX_OPTIN
	// Set NonPagedPool
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);

	//
	// Init global data
	//

	g_pCommonData = new (NonPagedPool) CommonGlobalData;
	if (g_pCommonData == nullptr)
		return STATUS_NO_MEMORY;
	IFERR_RET_NOLOG(g_pCommonData->Initialize());

	BOOLEAN fSuccess = FALSE;
	__try
	{
		g_pCommonData->pDriverObject = pDriverObject;
		g_pCommonData->pDriverObject->DriverUnload = unloadDriver;
		IFERR_RET(g_pCommonData->usRegistryPath.assign(pusRegistryPath));

		LOGINFO1("DriverEntry starts...\r\n");

		// Check minimum OS Version
		IFERR_RET(checkOSVersionSupport(6, 1), "Unsupported OS version. Need Win7+\r\n");
		if (NT_SUCCESS(checkOSVersionSupport(6, 2)))
			g_pCommonData->fIsWin8orHigher = TRUE;

		log::initialize();

		LOGINFO1("Starting edrdrv.sys...\r\n");

		IFERR_RET(getProceduresAddress(), "Can't getProceduresAddress\r\n");

		IFERR_RET(cfg::initialize(), "Can't initialize config\r\n");
		IFERR_RET(devdisp::initialize(), "Can't initialize devices disatcher\r\n");
		IFERR_RET(procmon::initialize(), "Can't initialize process monitor\r\n");
		IFERR_RET(filemon::initialize(), "Can't initialize file filter\r\n");
		IFERR_RET(regmon::initialize(), "Can't initialize registry filter\r\n");
		IFERR_RET(objmon::initialize(), "Can't initialize process filter\r\n");
		IFERR_RET(drvioctl::initialize(), "Can't initialize IOCTL device\r\n");
		IFERR_RET(netmon::initialize(), "Can't initialize network monitor\r\n");
		IFERR_RET(dllinj::initialize(), "Can't initialize DLL injector\r\n");

		g_pCommonData->pfnSaveDriverUnload = g_pCommonData->pDriverObject->DriverUnload;
		if (!g_pCommonData->fDisableSelfProtection)
			g_pCommonData->pDriverObject->DriverUnload = nullptr;

		LOGINFO1("Loading config...\r\n");
		IFERR_LOG(loadMainConfigFromRegistry(), "Can't load config.\r\n");


		fSuccess = TRUE;
		LOGINFO1("Initialization successed\r\n");
	}
	__finally
	{
		if (!fSuccess)
		{
			LOGINFO1("Initialization failed\r\n");
			unloadDriver(pDriverObject);
		}
	}

	return STATUS_SUCCESS;
}

/// @}
