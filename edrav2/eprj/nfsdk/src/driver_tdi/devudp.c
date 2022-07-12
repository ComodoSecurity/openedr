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
#include "devudp.h"
#include "gvars.h"
#include "addr.h"
#include "udpconn.h"
#include "udprecv.h"
#include "udpsend.h"
#include "tdiutil.h"

NTSTATUS attach_udp_device(PDRIVER_OBJECT DriverObject)
{
	tu_attachDeviceInternal(DriverObject,
					NF_UDP_SYSTEM_DEVICE,
					&g_vars.UDPFilterDeviceName,
					&g_vars.UDPFileObject,
					&g_vars.deviceUDP,
					&g_vars.deviceUDPFilter);
	

	tu_attachDeviceInternal(DriverObject,
					NF_UDP6_SYSTEM_DEVICE,
					&g_vars.UDP6FilterDeviceName,
					&g_vars.UDP6FileObject,
					&g_vars.deviceUDP6,
					&g_vars.deviceUDP6Filter);


	return STATUS_SUCCESS;
}

NTSTATUS detach_udp_device()
{
	if (g_vars.deviceUDP)
	{
		IoDetachDevice(g_vars.deviceUDP);
		g_vars.deviceUDP = NULL;
	}

	if (g_vars.deviceUDPFilter)
	{
		IoDeleteDevice(g_vars.deviceUDPFilter);
		g_vars.deviceUDPFilter = NULL;
	}

	if (g_vars.UDPFileObject)
	{
		ObDereferenceObject(g_vars.UDPFileObject);
		g_vars.UDPFileObject = NULL;
	}

	if (g_vars.deviceUDP6)
	{
		IoDetachDevice(g_vars.deviceUDP6);
		g_vars.deviceUDP = NULL;
	}

	if (g_vars.deviceUDP6Filter)
	{
		IoDeleteDevice(g_vars.deviceUDP6Filter);
		g_vars.deviceUDPFilter = NULL;
	}

	if (g_vars.UDP6FileObject)
	{
		ObDereferenceObject(g_vars.UDP6FileObject);
		g_vars.UDP6FileObject = NULL;
	}

	return STATUS_SUCCESS;
}


NTSTATUS udp_TdiCreate(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	FILE_FULL_EA_INFORMATION            *ea;
	FILE_FULL_EA_INFORMATION UNALIGNED  *targetEA;

	KdPrint((DPREFIX"udp_TdiCreate\n" ));

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
	}

	KdPrint((DPREFIX"udp_TdiCreate: Creating UNKNOWN TDI Object\n" ));

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS udp_TdiCleanup(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	KdPrint((DPREFIX"udp_TdiCleanup\n" ));

	switch ((ULONG_PTR) irpSp->FileObject->FsContext2)
	{
      case TDI_TRANSPORT_ADDRESS_FILE:
         return addr_TdiCloseAddress(irp, irpSp, pLowerDevice);

	  default:
         //
         // This Should Never Happen
         //
         break;
	}

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS dispatch_udp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp, PDEVICE_OBJECT pLowerDevice)
{
	PIO_STACK_LOCATION      irpSp = NULL;

	irpSp = IoGetCurrentIrpStackLocation(irp);
	ASSERT(irpSp);

	KdPrint((DPREFIX"dispatch_udp: Irp=%x\n", irp ));

	switch(irpSp->MajorFunction)
	{
		case IRP_MJ_CREATE:
			return udp_TdiCreate(irp, irpSp, pLowerDevice);

		case IRP_MJ_CLEANUP:
			return udp_TdiCleanup(irp, irpSp, pLowerDevice);
//			return udp_TdiCleanup(irp, irpSp, pLowerDevice);
//			break;

		case IRP_MJ_DEVICE_CONTROL:
			
			if (!NT_SUCCESS(TdiMapUserRequest(DeviceObject, irp, irpSp)))
			{
				break;
			}
		
		case IRP_MJ_INTERNAL_DEVICE_CONTROL:
			{			
				if (((ULONG_PTR)irpSp->FileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE)
				{
					switch(irpSp->MinorFunction)
					{
						case TDI_SET_EVENT_HANDLER:
							KdPrint((DPREFIX"dispatch_udp: TDI_SET_EVENT_HANDLER\n" ));
							return addr_TdiSetEvent(irp, irpSp, pLowerDevice);
							
						case TDI_SEND_DATAGRAM:
							KdPrint((DPREFIX"dispatch_udp: TDI_SEND_DATAGRAM\n" ));
							return udp_TdiSendDatagram(irp, irpSp, pLowerDevice);

						case TDI_RECEIVE_DATAGRAM:
							KdPrint((DPREFIX"dispatch_udp: TDI_RECEIVE_DATAGRAM\n" ));
							return udp_TdiReceiveDatagram(irp, irpSp, pLowerDevice);

						case TDI_CONNECT:
							KdPrint((DPREFIX"dispatch_udp: TDI_CONNECT\n" ));
							return udp_TdiConnect(irp, irpSp, pLowerDevice);

						case TDI_SEND:
							KdPrint((DPREFIX"dispatch_udp: TDI_SEND\n" ));
							return udp_TdiSend(irp, irpSp, pLowerDevice);

						default:
							break;
					}
				} else
				{
					KdPrint((DPREFIX"dispatch_udp: control context\n" ));
				}
			 }
		     break;

      case IRP_MJ_CLOSE:
			KdPrint((DPREFIX"dispatch_udp: IRP_MJ_CLOSE\n" ));
			break;
//			return udp_TdiCleanup(irp, irpSp, pLowerDevice);

	  case IRP_MJ_QUERY_SECURITY:
			KdPrint((DPREFIX"dispatch_udp: IRP_MJ_QUERY_SECURITY\n" ));
			break;

      case IRP_MJ_WRITE:
			KdPrint((DPREFIX"dispatch_udp: IRP_MJ_WRITE\n" ));
			break;

      case IRP_MJ_READ:
			KdPrint((DPREFIX"dispatch_udp: IRP_MJ_READ\n" ));
			break;

      default:
		KdPrint((DPREFIX"dispatch_udp: unknown request\n" ));
        break;
    }

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}
