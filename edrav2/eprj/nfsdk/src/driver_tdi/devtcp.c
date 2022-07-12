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
#include "devtcp.h"
#include "gvars.h"
#include "addr.h"
#include "tcpconn.h"
#include "tcprecv.h"
#include "tcpsend.h"
#include "tdiutil.h"
#include "ctrlio.h"
#include "tdiinfo.h"

#ifdef _WPPTRACE
#include "devtcp.tmh"
#endif

NTSTATUS attach_tcp_device(PDRIVER_OBJECT DriverObject)
{
	tu_attachDeviceInternal(DriverObject,
					NF_TCP_SYSTEM_DEVICE,
					&g_vars.TCPFilterDeviceName,
					&g_vars.TCPFileObject,
					&g_vars.deviceTCP,
					&g_vars.deviceTCPFilter);
	
	tu_attachDeviceInternal(DriverObject,
					NF_TCP6_SYSTEM_DEVICE,
					&g_vars.TCP6FilterDeviceName,
					&g_vars.TCP6FileObject,
					&g_vars.deviceTCP6,
					&g_vars.deviceTCP6Filter);

	return STATUS_SUCCESS;
}

NTSTATUS detach_tcp_device()
{
	if (g_vars.deviceTCP)
	{
		IoDetachDevice(g_vars.deviceTCP);
		g_vars.deviceTCP = NULL;
	}

	if (g_vars.deviceTCPFilter)
	{
		IoDeleteDevice(g_vars.deviceTCPFilter);
		g_vars.deviceTCPFilter = NULL;
	}

	if (g_vars.TCPFileObject)
	{
		ObDereferenceObject(g_vars.TCPFileObject);
		g_vars.TCPFileObject = NULL;
	}

	if (g_vars.deviceTCP6)
	{
		IoDetachDevice(g_vars.deviceTCP6);
		g_vars.deviceTCP6 = NULL;
	}

	if (g_vars.deviceTCP6Filter)
	{
		IoDeleteDevice(g_vars.deviceTCP6Filter);
		g_vars.deviceTCP6Filter = NULL;
	}

	if (g_vars.TCP6FileObject)
	{
		ObDereferenceObject(g_vars.TCP6FileObject);
		g_vars.TCP6FileObject = NULL;
	}

	return STATUS_SUCCESS;
}


NTSTATUS tcp_TdiCreate(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	FILE_FULL_EA_INFORMATION            *ea;
	FILE_FULL_EA_INFORMATION UNALIGNED  *targetEA;

	ea = (PFILE_FULL_EA_INFORMATION) irp->AssociatedIrp.SystemBuffer;

	if (ea)
	{
	   //
	   // Handle Address Object Open
	   //
	   targetEA = tu_findEA(ea, TdiTransportAddress, TDI_TRANSPORT_ADDRESS_LENGTH);

	   if (targetEA != NULL)
	   {
		  return addr_TdiOpenAddress(irp, irpSp, pLowerDevice);
	   }

	   //
	   // Handle Connection Object Open
	   //
	   targetEA = tu_findEA(ea, TdiConnectionContext, TDI_CONNECTION_CONTEXT_LENGTH);

	   if (targetEA != NULL)
	   {
		  void * context = *((CONNECTION_CONTEXT UNALIGNED *)&(targetEA->EaName[targetEA->EaNameLength + 1]));
		  return tcp_TdiOpenConnection(irp, irpSp, context, pLowerDevice);
	   }
	}

	KdPrint((DPREFIX"tcp_TdiCreate: Creating UNKNOWN TDI Object\n" ));

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiClose(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	ULONG_PTR context = (ULONG_PTR) irpSp->FileObject->FsContext2;
	switch (context)
	{
      case TDI_TRANSPORT_ADDRESS_FILE:
         return addr_TdiCloseAddress(irp, irpSp, pLowerDevice);

      case TDI_CONNECTION_FILE:
         return tcp_TdiCloseConnection(irp, irpSp, pLowerDevice);

      case TDI_CONTROL_CHANNEL_FILE:
	      default:
         //
         // This Should Never Happen
         //
         break;
	}

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

typedef NTSTATUS t_tcp_SendData(IN PIRP Irp, IN PIO_STACK_LOCATION IrpSp); 

NTSTATUS tcp_SendData(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	PDEVICE_OBJECT DeviceObject = irpSp->DeviceObject;
	
	KdPrint((DPREFIX"tcp_SendData\n" ));

	if (DeviceObject == g_vars.deviceTCPFilter)
	{
		return tcp_TdiSend(irp, irpSp, g_vars.deviceTCP);
	} else
	if (DeviceObject == g_vars.deviceTCP6Filter)
	{
		return tcp_TdiSend(irp, irpSp, g_vars.deviceTCP6);
	} else
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(DeviceObject, irp);
	}
}

NTSTATUS tcp_setInformationEx(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
    PDRIVER_CANCEL		oldCancelRoutine;
	KIRQL				irql;
    PVOID				ioBuffer = irp->AssociatedIrp.SystemBuffer;
	ULONG				inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	BOOLEAN				block = FALSE;

	if (!ctrl_attached() ||
		!ioBuffer ||
		(inputBufferLength < sizeof(TCP_REQUEST_SET_INFORMATION_EX)))
	{
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pLowerDevice, irp);
	}

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		PTCP_REQUEST_SET_INFORMATION_EX info = (PTCP_REQUEST_SET_INFORMATION_EX)ioBuffer;
	
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		switch (info->ID.toi_id)
		{
		case TCP_SOCKET_NODELAY:
			if (pConn->blockSetInformation & BSIC_NODELAY)
				block = TRUE;
			break;
		case TCP_SOCKET_KEEPALIVE:
			if (pConn->blockSetInformation & BSIC_KEEPALIVE)
				block = TRUE;
			break;
		case TCP_SOCKET_OOBINLINE:
			if (pConn->blockSetInformation & BSIC_OOBINLINE)
				block = TRUE;
			break;
		case TCP_SOCKET_BSDURGENT:
			if (pConn->blockSetInformation & BSIC_BSDURGENT)
				block = TRUE;
			break;
		case TCP_SOCKET_ATMARK:
			if (pConn->blockSetInformation & BSIC_ATMARK)
				block = TRUE;
			break;
		case TCP_SOCKET_WINDOW:
			if (pConn->blockSetInformation & BSIC_WINDOW)
				block = TRUE;
			break;
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (block)
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	
	IoSkipCurrentIrpStackLocation(irp);
	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS dispatch_tcp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp, PDEVICE_OBJECT pLowerDevice)
{
	PIO_STACK_LOCATION      irpSp = NULL;

	irpSp = IoGetCurrentIrpStackLocation(irp);
	ASSERT(irpSp);

	KdPrint((DPREFIX"dispatch_tcp: major=%d, minor=%d\n",
		irpSp->MajorFunction, irpSp->MinorFunction));

	switch(irpSp->MajorFunction)
	{
		case IRP_MJ_CREATE:
			return tcp_TdiCreate(irp, irpSp, pLowerDevice);

		case IRP_MJ_CLEANUP:
			KdPrint((DPREFIX"dispatch_tcp: IRP_MJ_CLEANUP\n" ));
			return tcp_TdiClose(irp, irpSp, pLowerDevice);
			//return tcp_TdiCleanup(irp, irpSp);
//			break;

		case IRP_MJ_DEVICE_CONTROL:
			
			if (!NT_SUCCESS(TdiMapUserRequest(DeviceObject, irp, irpSp)))
			{
				if (irpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER)
				{
					void * buf;

					KdPrint((DPREFIX"dispatch_tcp: IOCTL_TDI_QUERY_DIRECT_SEND_HANDLER\n" ));
					
					buf = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
					if (buf)
					{
						*(t_tcp_SendData**)buf = tcp_SendData;
						
						irp->IoStatus.Status = STATUS_SUCCESS;
						IoCompleteRequest(irp, IO_NO_INCREMENT);
						
						return STATUS_SUCCESS;
					}
				}
				break;
			}
		
		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			{			
				if (((ULONG_PTR)irpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE)
				{
					switch(irpSp->MinorFunction)
					{
						case TDI_SEND:
							KdPrint((DPREFIX"dispatch_tcp: TDI_SEND\n" ));
							return tcp_TdiSend(irp, irpSp, pLowerDevice);
							
						case TDI_RECEIVE:
							KdPrint((DPREFIX"dispatch_tcp: TDI_RECEIVE\n" ));
							return tcp_TdiReceive(irp, irpSp, pLowerDevice);
							
						case TDI_ASSOCIATE_ADDRESS:
							KdPrint((DPREFIX"dispatch_tcp: TDI_ASSOCIATE_ADDRESS\n" ));
							return tcp_TdiAssociateAddress(irp, irpSp, pLowerDevice);
							
						case TDI_DISASSOCIATE_ADDRESS:
							KdPrint((DPREFIX"dispatch_tcp: TDI_DISASSOCIATE_ADDRESS\n" ));
							return tcp_TdiDisAssociateAddress(irp, irpSp, pLowerDevice);
							
						case TDI_CONNECT:
							KdPrint((DPREFIX"dispatch_tcp: TDI_CONNECT\n" ));
							return tcp_TdiConnect(irp, irpSp, pLowerDevice);
							
						case TDI_DISCONNECT:
							KdPrint((DPREFIX"dispatch_tcp: TDI_DISCONNECT\n" ));
							return tcp_TdiDisconnect(irp, irpSp, pLowerDevice);
							
						case TDI_LISTEN:
							return tcp_TdiListen(irp, irpSp, pLowerDevice);
							
						case IOCTL_TCP_SET_INFORMATION_EX:
							KdPrint((DPREFIX"dispatch_tcp: IOCTL_TCP_SET_INFORMATION_EX\n" ));
							return tcp_setInformationEx(irp, irpSp, pLowerDevice);

						default:
							break;
					}
				} else 
				if (((ULONG_PTR)irpSp->FileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE)
				{
					switch(irpSp->MinorFunction)
					{
						case TDI_SET_EVENT_HANDLER:
							KdPrint((DPREFIX"dispatch_tcp: TDI_SET_EVENT_HANDLER\n" ));
							return addr_TdiSetEvent(irp, irpSp, pLowerDevice);

						case TDI_SEND_DATAGRAM:
							KdPrint((DPREFIX"dispatch_tcp: TDI_SEND_DATAGRAM\n" ));
							break;

						case TDI_RECEIVE_DATAGRAM:
							KdPrint((DPREFIX"dispatch_tcp: TDI_RECEIVE_DATAGRAM\n" ));
							break;
							
						default:
							break;
					}
				} else
				{
					KdPrint((DPREFIX"dispatch_tcp: control context\n" ));
				}
			 }
		     break;

      case IRP_MJ_CLOSE:
			KdPrint((DPREFIX"dispatch_tcp: IRP_MJ_CLOSE\n" ));
			return tcp_TdiClose(irp, irpSp, pLowerDevice);

	  case IRP_MJ_QUERY_SECURITY:
			KdPrint((DPREFIX"dispatch_tcp: IRP_MJ_QUERY_SECURITY\n" ));
			break;

      case IRP_MJ_WRITE:
			KdPrint((DPREFIX"dispatch_tcp: IRP_MJ_WRITE\n" ));
			break;

      case IRP_MJ_READ:
			KdPrint((DPREFIX"dispatch_tcp: IRP_MJ_READ\n" ));
			break;

      default:
		KdPrint((DPREFIX"dispatch_tcp: unknown request\n" ));
        break;
    }

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}
