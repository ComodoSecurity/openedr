//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _PACKET_H
#define _PACKET_H

typedef struct _NF_PACKET
{
	LIST_ENTRY		entry;
	PFILE_OBJECT	fileObject;		// Connection or address file object
	NTSTATUS		ioStatus;		// Status
	u_long			flags;			// UDP flags
	TDI_CONNECTION_INFORMATION	connInfo;  // UDP datagram parameters
	TDI_CONNECTION_INFORMATION	queryConnInfo;  // UDP datagram query parameters
    UCHAR			remoteAddr[TA_ADDRESS_MAX + sizeof(LONG)]; // UDP datagram remote address
	PMDL			pMdl;			// Buffer MDL descriptor
	u_long			offset;			// Current logical offset into packet.
	u_long			dataSize;		// Amount of data in buffer
	u_long			bufferSize;		// Total buffer size
	u_char			buffer[1];		// Packet buffer
} NF_PACKET, *PNF_PACKET;

PNF_PACKET nf_packet_alloc(u_long size);
void nf_packet_free(PNF_PACKET pPacket);

#endif