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
#include "devctrl.h"
#include "gvars.h"
#include "nfdriver.h"
#include "ctrlio.h"
#include "gdefs.h"
#include "rules.h"
#include "mempools.h"

#ifdef _WPPTRACE
#include "devctrl.tmh"
#endif

NTSTATUS create_ctrl_device(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS 	status;

	status = IoCreateDevice(
			DriverObject,
			0,
			&g_vars.ctrlDeviceName,
        	FILE_DEVICE_UNKNOWN,
        	FILE_DEVICE_SECURE_OPEN,
        	FALSE,
        	&g_vars.deviceControl);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	g_vars.deviceControl->Flags &= ~DO_DEVICE_INITIALIZING;

	status = IoCreateSymbolicLink(&g_vars.ctrlDeviceLinkName, &g_vars.ctrlDeviceName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(g_vars.deviceControl);
		
		g_vars.deviceControl = NULL;

		return status;
	}

	g_vars.deviceControl->Flags &= ~DO_DEVICE_INITIALIZING;

	g_vars.deviceControl->Flags |= DO_DIRECT_IO;

	return STATUS_SUCCESS;
}

NTSTATUS delete_ctrl_device()
{
	if (g_vars.deviceControl != NULL)
	{
		IoDeleteSymbolicLink(&g_vars.ctrlDeviceLinkName);
		IoDeleteDevice(g_vars.deviceControl);
		g_vars.deviceControl = NULL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS dispatch_getAddrInformation(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

	if (ioBuffer && 
		(inputBufferLength >= sizeof(NF_DATA)) && 
		(outputBufferLength >= sizeof(NF_DATA)))
	{
		irp->IoStatus.Status = ctrl_getAddrInformation((PNF_DATA)ioBuffer);
		
		if (NT_SUCCESS(irp->IoStatus.Status))
		{	
			irp->IoStatus.Information = outputBufferLength;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return STATUS_SUCCESS;
		}
	}

	irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_UNSUCCESSFUL;
}


#define _FLT_MAX_PATH 260 * 2

typedef NTSTATUS (*QUERY_INFO_PROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

static QUERY_INFO_PROCESS ZwQueryInformationProcess;

NTSTATUS getProcessName(HANDLE processId, PUNICODE_STRING * pName)
{
    NTSTATUS	status;
	PVOID		buf = NULL;
	ULONG		len = 0;
	PEPROCESS	eProcess = NULL;
	HANDLE		hProcess = NULL;
   
    if (NULL == ZwQueryInformationProcess) 
	{
        UNICODE_STRING routineName;

        RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

        ZwQueryInformationProcess =
               (QUERY_INFO_PROCESS) MmGetSystemRoutineAddress(&routineName);

        if (NULL == ZwQueryInformationProcess) 
		{
            KdPrint(("Cannot resolve ZwQueryInformationProcess\n"));
			return STATUS_UNSUCCESSFUL;
        }
    }

	status = PsLookupProcessByProcessId(processId, &eProcess);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = ObOpenObjectByPointer(eProcess, OBJ_KERNEL_HANDLE, NULL, 0, 0, KernelMode, &hProcess);
	if (!NT_SUCCESS(status))
	{
		ObDereferenceObject(eProcess);
		return status;
	}
	
    status = ZwQueryInformationProcess(
            hProcess,
            ProcessImageFileName,
            NULL,
            0,
            &len
        );

	if (STATUS_INFO_LENGTH_MISMATCH != status)
	{
		goto fin;
	}

	buf = malloc_np(len);
	if (!buf)
	{
		goto fin;
	}

    status = ZwQueryInformationProcess(
            hProcess,
            ProcessImageFileName,
            buf,
            len,
            &len
        );
	if (!NT_SUCCESS(status) || len == 0)
	{
		free_np(buf);
		goto fin;
	}

	*pName = buf;
	status = STATUS_SUCCESS;

fin:

	if (hProcess != NULL)
	{
		ZwClose(hProcess);
	}

	if (eProcess != NULL)
	{
		ObDereferenceObject(eProcess);
	}

	return status;
}

#pragma warning(disable:4312)

NTSTATUS devctrl_getProcessName(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG	outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	PUNICODE_STRING pName = NULL;
	ULONG		nameLen = 0;
	NTSTATUS 	status;

	if (ioBuffer && (inputBufferLength >= sizeof(ULONG)))
	{
		status = getProcessName((HANDLE)*(ULONG*)ioBuffer, &pName);
		if (NT_SUCCESS(status) && pName)
		{
			if (pName->Buffer && pName->Length)
			{
				nameLen = pName->Length+2;

				if (nameLen > outputBufferLength)
				{
					free_np(pName);
					irp->IoStatus.Status = STATUS_NO_MEMORY;
					irp->IoStatus.Information = nameLen;
					IoCompleteRequest(irp, IO_NO_INCREMENT);
					return STATUS_NO_MEMORY;
				}

				RtlStringCchCopyUnicodeString((PWSTR)ioBuffer, nameLen/2, pName);
			}
			free_np(pName);
		}
	}


	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = nameLen;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return STATUS_SUCCESS;
}

NTSTATUS devctrl_clearTempRules(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	NTSTATUS status;

	rule_remove_all_temp();

	status = STATUS_SUCCESS;

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addTempRule(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_RULE)))
	{
		PNF_RULE pRule = (PNF_RULE)ioBuffer;
		
		if (rule_add_temp(pRule))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_NO_MEMORY;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_setTempRules(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	NTSTATUS status;

	rule_set_temp();

	status = STATUS_SUCCESS;

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addRuleEx(PIRP irp, PIO_STACK_LOCATION irpSp, BOOLEAN toHead)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_RULE_EX)))
	{
		PNF_RULE_EX pRule = (PNF_RULE_EX)ioBuffer;
		PRULE_ENTRY pRuleEntry;

		pRuleEntry = (PRULE_ENTRY)mp_alloc(sizeof(RULE_ENTRY));
		if (!pRuleEntry)
			return 0;

		memcpy(&pRuleEntry->rule, pRule, sizeof(NF_RULE_EX));

		rule_add(pRuleEntry, toHead);

		status = STATUS_SUCCESS;
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addTempRuleEx(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_RULE_EX)))
	{
		PNF_RULE_EX pRule = (PNF_RULE_EX)ioBuffer;
		
		if (rule_add_tempEx(pRule))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_NO_MEMORY;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS dispatch_ctrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	NTSTATUS status;
	PIO_STACK_LOCATION irpSp;
	
	irpSp = IoGetCurrentIrpStackLocation(irp);
	ASSERT(irpSp);
	
	KdPrint(("dispatch_ctrl mj=%d", irpSp->MajorFunction));

	switch (irpSp->MajorFunction) 
	{
	case IRP_MJ_CREATE:
		return ctrl_create(irp, irpSp);

	case IRP_MJ_READ:
		return ctrl_read(irp, irpSp);

	case IRP_MJ_WRITE:
		return ctrl_write(irp, irpSp);

	case IRP_MJ_CLOSE:
		return ctrl_close(irp, irpSp);

	case IRP_MJ_DEVICE_CONTROL:
		switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_DEVCTRL_OPEN:
			return ctrl_open(irp, irpSp);

		case NF_REQ_GET_ADDR_INFO:
			return dispatch_getAddrInformation(irp, irpSp);

		case NF_REQ_GET_PROCESS_NAME:
			return devctrl_getProcessName(irp, irpSp);

		case NF_REQ_CLEAR_TEMP_RULES:
			return devctrl_clearTempRules(irp, irpSp);

		case NF_REQ_ADD_TEMP_RULE:
			return devctrl_addTempRule(irp, irpSp);

		case NF_REQ_SET_TEMP_RULES:
			return devctrl_setTempRules(irp, irpSp);

		case NF_REQ_ADD_HEAD_RULE_EX:
			return devctrl_addRuleEx(irp, irpSp, TRUE);

		case NF_REQ_ADD_TAIL_RULE_EX:
			return devctrl_addRuleEx(irp, irpSp, FALSE);

		case NF_REQ_ADD_TEMP_RULE_EX:
			return devctrl_addTempRuleEx(irp, irpSp);
		}

//	case TDI_QSA:
//		if (irpSp->MinorFunction != TDI_QSA)
//			return STATUS_INVALID_DEVICE_REQUEST;
	}	

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}
