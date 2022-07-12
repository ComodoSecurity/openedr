//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _DEVTCP_H
#define _DEVTCP_H

NTSTATUS attach_tcp_device(PDRIVER_OBJECT DriverObject);

NTSTATUS detach_tcp_device();

NTSTATUS dispatch_tcp(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp, PDEVICE_OBJECT pLowerDevice);

#endif