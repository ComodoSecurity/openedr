//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _UDPRECV_H
#define _UDPRECV_H

#include "addr.h"

NTSTATUS udp_TdiReceiveDatagram(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

NTSTATUS udp_TdiReceiveDGEventHandler(
   PVOID TdiEventContext,        // Context From SetEventHandler
   LONG SourceAddressLength,     // length of the originator of the datagram
   PVOID SourceAddress,          // string describing the originator of the datagram
   LONG OptionsLength,           // options for the receive
   PVOID Options,                //
   ULONG ReceiveDatagramFlags,   //
   ULONG BytesIndicated,         // number of bytes in this indication
   ULONG BytesAvailable,         // number of bytes in complete Tsdu
   ULONG *BytesTaken,            // number of bytes used by indication routine
   PVOID Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   );

NTSTATUS
  udp_TdiChainedReceiveDGEventHandler(
    IN PVOID  TdiEventContext,
    IN LONG  SourceAddressLength,
    IN PVOID  SourceAddress,
    IN LONG  OptionsLength,
    IN PVOID  Options,
    IN ULONG  ReceiveDatagramFlags,
    IN ULONG  ReceiveDatagramLength,
    IN ULONG  StartingOffset,
    IN PMDL  Tsdu,
    IN PVOID  TsduDescriptor
    );

NTSTATUS udp_indicateReceivedPackets(PADDROBJ pAddr, HASH_ID id);


#endif