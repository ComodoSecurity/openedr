//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "gvars.h"
#include "mempools.h"
#include "devctrl.h"
#include "devtcp.h"
#include "devudp.h"

#ifdef _WPPTRACE
#include "drventry.tmh"
#endif

DWORD regQueryValueDWORD(PUNICODE_STRING registryPath, const wchar_t * wszValueName)
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( DWORD )];
	DWORD value = 0;

    InitializeObjectAttributes( &attributes,
                                registryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = ZwOpenKey( &driverRegKey,
                        KEY_READ,
                        &attributes );
    if (NT_SUCCESS( status )) 
	{
        RtlInitUnicodeString( &valueName, wszValueName );
        
        status = ZwQueryValueKey( driverRegKey,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  buffer,
                                  sizeof(buffer),
                                  &resultLength );

        if (NT_SUCCESS( status )) 
		{
            value = *((DWORD*) &(((PKEY_VALUE_PARTIAL_INFORMATION) buffer)->Data));
        }
    }

    ZwClose( driverRegKey );

	return value;
}


typedef enum _NF_SECURITY_LEVEL
{
	SL_NONE = 0,
	SL_LOCAL_SYSTEM = 1,
	SL_ADMINS = 2,
	SL_USERS = 4
} NF_SECURITY_LEVEL;

NTSTATUS setDeviceDacl(PDEVICE_OBJECT DeviceObject, DWORD secLevel)
{
    SECURITY_DESCRIPTOR sd = { 0 };
    ULONG aclSize = 0;
    PACL pAcl = NULL;
	HANDLE fileHandle = NULL;
	int nAcls;
    NTSTATUS status;

	if (secLevel == 0)
		return STATUS_SUCCESS;

	for (;;)
	{
		status = ObOpenObjectByPointer(DeviceObject,
									   OBJ_KERNEL_HANDLE,
									   NULL,
									   WRITE_DAC,
									   0,
									   KernelMode,
									   &fileHandle);
		if (!NT_SUCCESS(status)) 
		{
			break;
		} 

		nAcls = 0;

		aclSize = sizeof(ACL);

		if (secLevel & SL_LOCAL_SYSTEM)
		{
			aclSize += RtlLengthSid(SeExports->SeLocalSystemSid);
			nAcls++;
		}

		if (secLevel & SL_ADMINS)
		{
			aclSize += RtlLengthSid(SeExports->SeAliasAdminsSid);
			nAcls++;
		}

		if (secLevel & SL_USERS)
		{
			aclSize += RtlLengthSid(SeExports->SeAuthenticatedUsersSid);
			nAcls++;
		}

		aclSize += nAcls * FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);

		pAcl = (PACL) ExAllocatePoolWithTag(PagedPool, aclSize, MEM_TAG);
		if (pAcl == NULL) 
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		status = RtlCreateAcl(pAcl, aclSize, ACL_REVISION);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		if (secLevel & SL_LOCAL_SYSTEM)
		{
			status = RtlAddAccessAllowedAce(pAcl,
											ACL_REVISION,
											GENERIC_READ | GENERIC_WRITE | DELETE,
											SeExports->SeLocalSystemSid);
			if (!NT_SUCCESS(status)) 
			{
				break;
			}
		}

		if (secLevel & SL_ADMINS)
		{
			status = RtlAddAccessAllowedAce(pAcl,
											ACL_REVISION,
											GENERIC_READ | GENERIC_WRITE | DELETE,
											SeExports->SeAliasAdminsSid );
			if (!NT_SUCCESS(status)) 
			{
				break;
			}
		}

		if (secLevel & SL_USERS)
		{
			status = RtlAddAccessAllowedAce(pAcl,
											ACL_REVISION,
											GENERIC_READ | GENERIC_WRITE | DELETE,
											SeExports->SeAuthenticatedUsersSid );
			if (!NT_SUCCESS(status)) 
			{
				break;
			}
		}

		status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		status = RtlSetDaclSecurityDescriptor(&sd, TRUE, pAcl, FALSE);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		status = ZwSetSecurityObject(fileHandle, DACL_SECURITY_INFORMATION, &sd);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		break;
	}

	if (fileHandle != NULL)
	{
		ZwClose(fileHandle);
	}

    if (pAcl != NULL) 
	{
        ExFreePool(pAcl);
    }

    return status;
}

void setDeviceSecurity(
	PDEVICE_OBJECT DeviceObject,
	PUNICODE_STRING registryPath)
{
	DWORD secLevel;

	secLevel = regQueryValueDWORD(registryPath, L"seclevel");

	if (secLevel != 0)
	{
		setDeviceDacl(DeviceObject, secLevel);
	}
}


NTSTATUS dispatch_request(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	KdPrint(("dispatch_request"));

	if (DeviceObject == g_vars.deviceControl)
	{
		KdPrint(("dispatch_ctrl"));
		return dispatch_ctrl(DeviceObject, irp);
	} else 
	if (DeviceObject == g_vars.deviceTCPFilter)
	{
		return dispatch_tcp(DeviceObject, irp, g_vars.deviceTCP);
	} else
	if (DeviceObject == g_vars.deviceTCP6Filter)
	{
		return dispatch_tcp(DeviceObject, irp, g_vars.deviceTCP6);
	} else
	if (DeviceObject == g_vars.deviceUDPFilter)
	{
		return dispatch_udp(DeviceObject, irp, g_vars.deviceUDP);
	} else
	if (DeviceObject == g_vars.deviceUDP6Filter)
	{
		return dispatch_udp(DeviceObject, irp, g_vars.deviceUDP6);
	} 

	irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
	irp->IoStatus.Information = 0; 
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_INVALID_DEVICE_REQUEST;
}


void onUnload(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint((DPREFIX"onUnload\n"));

	delete_ctrl_device();
	detach_tcp_device();
	detach_udp_device();

	mempools_free();
}


NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	int 	i;
	NTSTATUS status;

	// Set dispatch routine
	for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = dispatch_request;
	}

	//DriverObject->DriverUnload = onUnload;

	// Initialize memory pools
	mempools_init();

	// Initialize global variables
	status = init_gvars(RegistryPath);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	
	KdPrint((DPREFIX"create_ctrl_device\n"));

	// Create control device
	status = create_ctrl_device(DriverObject);
	if (!NT_SUCCESS(status))
	{
		onUnload(DriverObject);
		return status;
	}

	// Read the security level from driver registry key and apply the required restrictions
	setDeviceSecurity(g_vars.deviceControl, RegistryPath);

#ifdef _WPPTRACE
   	//
   	// include this macro to support Win2K.
   	//
   	WPP_SYSTEMCONTROL(DriverObject);

	//
	// This macro is required to initialize software tracing. 
	//
	// Win2K use the deviceobject as the first argument.
	//
	// XP and beyond does not require device object. First argument
	// is ignored. 
	// 
	WPP_INIT_TRACING(g_vars.deviceControl, RegistryPath);
#endif

	KdPrint((DPREFIX"attach_tcp_device\n"));

	// Attach to TCP/TCP6
	status = attach_tcp_device(DriverObject);
	if (!NT_SUCCESS(status))
	{
		onUnload(DriverObject);
		return status;
	}

	KdPrint((DPREFIX"attach_udp_device\n"));

	// Attach to UDP/UDP6
	status = attach_udp_device(DriverObject);
	if (!NT_SUCCESS(status))
	{
		onUnload(DriverObject);
		return status;
	}

	KdPrint((DPREFIX"initialized\n"));

	return STATUS_SUCCESS;
}
