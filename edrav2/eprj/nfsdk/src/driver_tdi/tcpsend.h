//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _TCPSEND_H
#define _TCPSEND_H

NTSTATUS tcp_TdiSend(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

NTSTATUS tcp_TdiSendPossibleHandler(
    IN PVOID  TdiEventContext,
    IN PVOID  ConnectionContext,
    IN ULONG  BytesAvailable
    );

NTSTATUS tcp_TdiStartInternalSend(PCONNOBJ pConn, HASH_ID id);

#endif