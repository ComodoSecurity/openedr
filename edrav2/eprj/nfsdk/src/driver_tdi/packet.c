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
#include "packet.h"
#include "mempools.h"
#include "gvars.h"

#ifdef _WPPTRACE
#include "packet.tmh"
#endif


PNF_PACKET nf_packet_alloc(u_long size)
{
	PNF_PACKET	pPacket = NULL;

	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	
	pPacket = (PNF_PACKET)malloc_np(sizeof(NF_PACKET) + size);
	if (!pPacket)
		return NULL;

#if DBG
	g_vars.nPackets++;
	KdPrint(("[counter] nPackets=%d\n", g_vars.nPackets));
#endif

	RtlZeroMemory(pPacket, sizeof(NF_PACKET));

	pPacket->bufferSize = size;

	// Map exactly <size> bytes instead of full allocated buffer.
	// TDI_SEND and TDI_SEND_DATAGRAM don't work properly when mdl size
	// is larger than SendLen.
	pPacket->pMdl = IoAllocateMdl(pPacket->buffer, size, FALSE, FALSE, NULL);
	if (NULL == pPacket->pMdl) 
	{
    	KdPrint(("nf_paket_alloc: IoAllocateMdl failed\n"));
		free_np(pPacket);
		return NULL;
	}
			
	MmBuildMdlForNonPagedPool(pPacket->pMdl);

	return pPacket;
}


void nf_packet_free(PNF_PACKET pPacket)
{
	if (!pPacket)
		return;

	if (pPacket->connInfo.Options)
	{
		free_np(pPacket->connInfo.Options);
	}

	if (pPacket->pMdl)
	{
		IoFreeMdl(pPacket->pMdl);
	}

	free_np(pPacket);

#if DBG
	g_vars.nPackets--;
	KdPrint(("[counter] nPackets=%d\n", g_vars.nPackets));
#endif
}
