//
//  EDRAv2.nfsdk
//  nfwfpdrv.lib header file
//
#pragma once

//
// Driver entry
//
EXTERN_C NTSTATUS nfwfpdrvDriverEntry(IN  PDRIVER_OBJECT  driverObject, IN  PUNICODE_STRING registryPath);

//
// Driver unload
//
EXTERN_C NTSTATUS nfwfpdrvDriverUnload(IN  PDRIVER_OBJECT  driverObject);

//
// Returns created device pointer
//
EXTERN_C PDEVICE_OBJECT devctrl_getDeviceObject();


//
// Dispatches IRP
//
EXTERN_C NTSTATUS devctrl_dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp);

