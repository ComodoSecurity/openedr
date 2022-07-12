//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "devctrl.h"
#include "rules.h"
#include "callouts.h"

#ifdef _WPPTRACE
#include "driver.tmh"
#endif

// EDR_FIX: Rename DriverEntry/driverUnload for call from edrdrv.sys
// DRIVER_INITIALIZE DriverEntry;
// DRIVER_UNLOAD driverUnload;

static HANDLE g_bfeStateSubscribeHandle = NULL;

BOOLEAN regPathExists(wchar_t * registryPath)
{
    OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;
	UNICODE_STRING path;
    HANDLE regKey;
	BOOLEAN result;

	RtlInitUnicodeString(&path, registryPath);

    InitializeObjectAttributes( &attributes,
                                &path,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = ZwOpenKey( &regKey,
                        KEY_READ,
                        &attributes );

    if (NT_SUCCESS( status )) 
	{
	    ZwClose( regKey );
		result = TRUE;
    } else
	{
		result = FALSE;
	}

	return result;
}


void cleanup()
{
	devctrl_setShutdown();

	flowctl_free();
	callouts_free();
	rules_free();
	devctrl_free();
	tcpctx_free();
	udpctx_free();
	ipctx_free();
}


//
// driverUnload is patched for calling from edrdrv.sys
//
VOID
// EDR_FIX: Rename DriverEntry/driverUnload for call from edrdrv.sys
/*driverUnload*/ nfwfpdrvDriverUnload(
   IN  PDRIVER_OBJECT driverObject
   )
{
	UNREFERENCED_PARAMETER(driverObject);

	KdPrint((DPREFIX"driverUnload\n"));

	if (g_bfeStateSubscribeHandle)
	{
		FwpmBfeStateUnsubscribeChanges(g_bfeStateSubscribeHandle);
		g_bfeStateSubscribeHandle = NULL;
	}

	cleanup();

#ifdef _WPPTRACE
	WPP_CLEANUP(driverObject);
#endif
}

VOID NTAPI
bfeStateCallback(
    IN OUT void  *context,
    IN FWPM_SERVICE_STATE  newState
    )
{
	UNREFERENCED_PARAMETER(context);

	if (newState == FWPM_SERVICE_RUNNING)
	{
		NTSTATUS status = callouts_init(devctrl_getDeviceObject());
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"bfeStateCallback callouts_init failed, status=%x\n", status));
		}
	}
}


//
// DriverEntry is patched for calling from edrdrv.sys
//
NTSTATUS
// EDR_FIX: Rename DriverEntry/driverUnload for call from edrdrv.sys
/*DriverEntry*/ nfwfpdrvDriverEntry(
   IN  PDRIVER_OBJECT  driverObject,
   IN  PUNICODE_STRING registryPath
   )
{
	//int i;
	NTSTATUS status;

// EDR_FIX: This actions is done in edrdrv.sys
/*

#ifdef _NXPOOLS
#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
	ExInitializeDriverRuntime(DrvRtPoolNxOptIn);
#endif
#endif
#endif

	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		driverObject->MajorFunction[i] = (PDRIVER_DISPATCH)devctrl_dispatch;
	}

    driverObject->DriverUnload = driverUnload;
*/

	for (;;)
	{
		status = devctrl_init(driverObject, registryPath);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"devctrl_init failed, status=%x\n", status));
			break;
		}

#ifdef _WPPTRACE
	   	WPP_SYSTEMCONTROL(driverObject);
		WPP_INIT_TRACING(devctrl_getDeviceObject(), registryPath);
#endif

		status = rules_init();
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"rules_init failed, status=%x\n", status));
			break;
		}

		if (!flowctl_init())
		{
			KdPrint((DPREFIX"flowctl_init failed\n"));
		}

		if (!tcpctx_init())
		{
			KdPrint((DPREFIX"tcpctx_init failed\n"));
			break;
		}

		if (!udpctx_init())
		{
			KdPrint((DPREFIX"udpctx_init failed\n"));
			break;
		}

		if (!ipctx_init())
		{
			KdPrint((DPREFIX"ipctx_init failed\n"));
			break;
		}

		if (FwpmBfeStateGet() == FWPM_SERVICE_RUNNING)
		{
			status = callouts_init(devctrl_getDeviceObject());
			if (!NT_SUCCESS(status))
			{
				KdPrint((DPREFIX"callouts_init failed, status=%x\n", status));
				break;
			}
		} else
		{
			status = FwpmBfeStateSubscribeChanges(
				devctrl_getDeviceObject(),
				bfeStateCallback,
				NULL,
				&g_bfeStateSubscribeHandle);
			if (!NT_SUCCESS(status))
			{
				KdPrint((DPREFIX"FwpmBfeStateSubscribeChanges failed, status=%x\n", status));
				break;
			}
		}

		break;
	}

	if (!NT_SUCCESS(status))
	{
		cleanup();
	}

	return status;
}


