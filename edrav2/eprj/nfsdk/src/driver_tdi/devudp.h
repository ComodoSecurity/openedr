//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _DEVUDP_H
#define _DEVUDP_H

NTSTATUS attach_udp_device(PDRIVER_OBJECT DriverObject);

NTSTATUS detach_udp_device();

NTSTATUS dispatch_udp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp, PDEVICE_OBJECT pLowerDevice);

#endif