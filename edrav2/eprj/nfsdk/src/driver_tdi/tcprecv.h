//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _TCPRECV_H
#define _TCPRECV_H

#include "tcpconn.h"

NTSTATUS tcp_TdiReceive(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

NTSTATUS tcp_TdiReceiveEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG BytesIndicated,
   ULONG BytesAvailable,
   ULONG *BytesTaken,
   PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   );

NTSTATUS tcp_TdiReceiveExpeditedEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG BytesIndicated,
   ULONG BytesAvailable,
   ULONG *BytesTaken,
   PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   );

NTSTATUS tcp_TdiChainedReceiveHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL  Tsdu,
    IN PVOID  TsduDescriptor
    );

NTSTATUS tcp_indicateReceivedPackets(PCONNOBJ pConn, HASH_ID id);
NTSTATUS tcp_TdiStartInternalReceive(PCONNOBJ pConn, HASH_ID id);

ULONG tcp_getTotalReceiveSize(PCONNOBJ pConn, ULONG limit);


#endif