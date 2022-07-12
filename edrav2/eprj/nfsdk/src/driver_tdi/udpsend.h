//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _UDPSEND_H
#define _UDPSEND_H

#include "addr.h"

NTSTATUS udp_TdiSendDatagram(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

NTSTATUS udp_TdiSend(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

NTSTATUS udp_TdiStartInternalSend(PADDROBJ pAddr);

#endif