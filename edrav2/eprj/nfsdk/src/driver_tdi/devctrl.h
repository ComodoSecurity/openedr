//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _DEVCTRL_H
#define _DEVCTRL_H

NTSTATUS create_ctrl_device(PDRIVER_OBJECT DriverObject);

NTSTATUS delete_ctrl_device();

NTSTATUS dispatch_ctrl(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);

NTSTATUS getProcessName(HANDLE processId, PUNICODE_STRING * pName);

#endif