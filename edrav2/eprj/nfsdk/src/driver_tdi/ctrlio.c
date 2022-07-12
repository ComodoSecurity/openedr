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
#include "gdefs.h"
#include "mempools.h"
#include "gvars.h"
#include "tdiutil.h"
#include "ctrlio.h"
#include "tcpconn.h"
#include "tcprecv.h"
#include "tcpsend.h"
#include "packet.h"
#include "rules.h"
#include "udprecv.h"
#include "udpsend.h"
#include <tdiinfo.h>

#ifdef _WPPTRACE
#include "ctrlio.tmh"
#endif

#define MAX_QE_POOL_SIZE	10

/**
*	Internal IO queue entry
**/
typedef struct _NF_QUEUE_ENTRY
{
	LIST_ENTRY		entry;		// Linkage
	int				code;		// IO code
	HASH_ID			id;			// Endpoint id
	u_long			bufferSize;	// Size of buffer
	u_char			buffer[1];	// Optional data buffer
} NF_QUEUE_ENTRY, *PNF_QUEUE_ENTRY;

typedef struct _SHARED_MEMORY
{
	PMDL					mdl;
	PVOID					userVa;
	PVOID					kernelVa;
    UINT64					bufferLength;
} SHARED_MEMORY, *PSHARED_MEMORY;

static SHARED_MEMORY g_inBuf;
static SHARED_MEMORY g_outBuf;

static BOOLEAN	  g_initialized = FALSE;

NTSTATUS ctrl_createSharedMemory(PSHARED_MEMORY pSharedMemory, UINT64 len);
void ctrl_freeSharedMemory(PSHARED_MEMORY pSharedMemory);


/**
*	Copy pPacket contents to buffer as NF_DATA record. 
*	@param pEntry Data descriptor
*	@param pPacket Data packet
*	@param buffer Pointer to destination memory
*	@param bufferSize Size of destination buffer
*	@param extraBytes Add the specified number of bytes after NF_DATA
**/
NTSTATUS ctrl_copyPacketToBuffer(
				PNF_QUEUE_ENTRY pEntry,
				PNF_PACKET pPacket,
				char ** buffer, 
				unsigned long * bufferSize,
				unsigned long extraBytes)
{
	unsigned long dataSize;
	NF_DATA * pData;

	dataSize = sizeof(NF_DATA) + pPacket->dataSize - 1;

	if (*bufferSize < (dataSize + extraBytes))
	{
		KdPrint((DPREFIX"ctrl_copyPacketToBuffer: not enough room in buffer\n"));
		return STATUS_NO_MEMORY;
	}
				
	pData = (NF_DATA *)*buffer;

	pData->code = pEntry->code;
	pData->id = pEntry->id;
	pData->bufferSize = pPacket->dataSize + extraBytes;

	if (pPacket->dataSize)
	{
		RtlCopyMemory(pData->buffer, pPacket->buffer, pPacket->dataSize);
	}

	*bufferSize -= dataSize;
	*buffer += dataSize;

	return STATUS_SUCCESS;
}


NTSTATUS ctrl_copyUdpPacketToBuffer(
				PNF_QUEUE_ENTRY pEntry,
				PNF_PACKET pPacket,
				char ** ppBuffer, 
				unsigned long * pBufferSize)
{
	unsigned long dataSize;
	NF_DATA * pData;
	NF_UDP_OPTIONS * pOptions;
	unsigned char UNALIGNED * remoteAddr;
	sockaddr_gen * pRemoteSockAddr;	
	ULONG addrLen = 0;
	unsigned long extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + pPacket->connInfo.OptionsLength;

	KdPrint((DPREFIX"ctrl_copyUdpPacketToBuffer packetSize=%d, extraBytes=%d\n", pPacket->dataSize, extraBytes));

	dataSize = sizeof(NF_DATA) - 1 + pPacket->dataSize + extraBytes;

	if (*pBufferSize < dataSize)
	{
		KdPrint((DPREFIX"ctrl_copyUdpPacketToBuffer: not enough room in buffer\n"));
		return STATUS_NO_MEMORY;
	}
				
	pData = (NF_DATA *)*ppBuffer;

	pData->code = pEntry->code;
	pData->id = pEntry->id;
	pData->bufferSize = pPacket->dataSize + extraBytes;

	*ppBuffer += sizeof(NF_DATA) - 1;

	remoteAddr = pPacket->remoteAddr;

	if (remoteAddr && ((PTRANSPORT_ADDRESS)remoteAddr)->Address)
	{
		pRemoteSockAddr = (sockaddr_gen *)&((PTA_ADDRESS)((PTRANSPORT_ADDRESS)remoteAddr)->Address)->AddressType;
		addrLen = ((PTA_ADDRESS)((PTRANSPORT_ADDRESS)remoteAddr)->Address)->AddressLength + sizeof(USHORT);
		
		RtlCopyMemory(*ppBuffer, pRemoteSockAddr, addrLen);
	}
	
	*ppBuffer += NF_MAX_ADDRESS_LENGTH;

	pOptions = (NF_UDP_OPTIONS *)*ppBuffer;	
	
	pOptions->flags = pPacket->flags;
	pOptions->optionsLength = pPacket->connInfo.OptionsLength;

	if ((pPacket->connInfo.OptionsLength > 0) && (pPacket->connInfo.Options != NULL))
	{
		RtlCopyMemory(pOptions->options, pPacket->connInfo.Options, pPacket->connInfo.OptionsLength);
	}

	*ppBuffer += sizeof(NF_UDP_OPTIONS) - 1 + pPacket->connInfo.OptionsLength;

	if (pPacket->dataSize)
	{
		RtlCopyMemory(*ppBuffer, pPacket->buffer, pPacket->dataSize);
	}

	*ppBuffer += pPacket->dataSize;

	*pBufferSize -= dataSize;

	return STATUS_SUCCESS;
}

/**
*	Copy mdl to buffer as NF_DATA record. 
*	@param pEntry Data descriptor
*	@param mdl Buffer descriptor
*	@param mdlBytes Number of bytes to copy
*	@param mdlOffset Offset in buffer described by mdl
*	@param buffer Pointer to destination memory
*	@param bufferSize Size of destination buffer
*	@param extraBytes Add the specified number of bytes after NF_DATA
**/
NTSTATUS ctrl_copyMdlToBuffer(
				PNF_QUEUE_ENTRY pEntry, 
				PMDL mdl, 
				unsigned long mdlBytes,
				unsigned long mdlOffset,
				char ** ppBuffer, 
				unsigned long * pBufferSize,
				unsigned long extraBytes)
{
	unsigned long	dataSize;
	NF_DATA	* pData;
	ULONG			offset;

	KdPrint((DPREFIX"ctrl_copyMdlToBuffer[%I64d]\n", pEntry->id));

	dataSize = sizeof(NF_DATA) + mdlBytes - 1;

	if (*pBufferSize < (dataSize + extraBytes))
	{
		return STATUS_NO_MEMORY;
	}
				
	pData = (PNF_DATA)*ppBuffer;

	pData->code = pEntry->code;
	pData->id = pEntry->id;
	pData->bufferSize = extraBytes;

	if (mdlBytes == 0)
	{
		*pBufferSize -= dataSize;
		*ppBuffer += dataSize;
		return STATUS_SUCCESS;
	}
	
	offset = tu_copyMdlToBuffer(mdl, mdlBytes, mdlOffset, pData->buffer, mdlBytes);

	pData->bufferSize = offset + extraBytes;

	dataSize = sizeof(NF_DATA) + offset - 1;

	*pBufferSize -= dataSize;
	*ppBuffer += dataSize;

	KdPrint((DPREFIX"ctrl_copyMdlToBuffer[%I64d] finished\n", pEntry->id));

	return STATUS_SUCCESS;
}

NTSTATUS ctrl_copyUdpMdlToBuffer(
				PNF_QUEUE_ENTRY pEntry, 
				PMDL mdl, 
				unsigned long mdlBytes,
				unsigned long mdlOffset,
				char ** ppBuffer, 
				unsigned long * pBufferSize,
				unsigned long flags,
				long optionsLength,
				PVOID options,
				unsigned char * remoteAddrTA,
				unsigned char * remoteAddr)
{
	ULONG			dataSize;
	NF_DATA	*	pData;
	NF_UDP_OPTIONS * pOptions;
	ULONG			offset;
	sockaddr_gen * pRemoteSockAddr;	
	ULONG addrLen = 0;
	ULONG			extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + optionsLength;

	KdPrint((DPREFIX"ctrl_copyUdpMdlToBuffer mdlBytes=%d, extraBytes=%d\n", mdlBytes, extraBytes));

	dataSize = sizeof(NF_DATA) - 1 + mdlBytes + extraBytes;

	if (*pBufferSize < dataSize)
	{
		KdPrint((DPREFIX"ctrl_copyUdpMdlToBuffer: not enough room in buffer\n"));
		return STATUS_NO_MEMORY;
	}
				
	pData = (PNF_DATA)*ppBuffer;

	pData->code = pEntry->code;
	pData->id = pEntry->id;
	pData->bufferSize = mdlBytes + extraBytes;

	*ppBuffer += (sizeof(NF_DATA) - 1);

	if (remoteAddrTA && ((PTRANSPORT_ADDRESS)remoteAddrTA)->Address)
	{
		pRemoteSockAddr = (sockaddr_gen *)&((PTA_ADDRESS)((PTRANSPORT_ADDRESS)remoteAddrTA)->Address)->AddressType;
		addrLen = ((PTA_ADDRESS)((PTRANSPORT_ADDRESS)remoteAddrTA)->Address)->AddressLength + sizeof(USHORT);
		
		RtlCopyMemory(*ppBuffer, pRemoteSockAddr, addrLen);
	} else
	if (remoteAddr)
	{
		RtlCopyMemory(*ppBuffer, remoteAddr, NF_MAX_ADDRESS_LENGTH);
	}
	
	*ppBuffer += NF_MAX_ADDRESS_LENGTH;

	pOptions = (PNF_UDP_OPTIONS)*ppBuffer;	
	
	pOptions->flags = flags;
	pOptions->optionsLength = optionsLength;

	if ((optionsLength > 0) && (options != NULL))
	{
		RtlCopyMemory(pOptions->options, options, optionsLength);
	}

	*ppBuffer += (sizeof(NF_UDP_OPTIONS) - 1 + optionsLength);

	if (mdlBytes == 0)
	{
		*pBufferSize -= dataSize;
		return STATUS_SUCCESS;
	}
	
	offset = tu_copyMdlToBuffer(mdl, mdlBytes, mdlOffset, *ppBuffer, mdlBytes);

	pData->bufferSize = offset + extraBytes;

	*ppBuffer += offset;

	*pBufferSize -= dataSize;

	return STATUS_SUCCESS;
}


/**
*	Copy the contents of next pended TDI_SEND IRP to buffer as NF_DATA record. 
*	Large buffers are splitted to avoid IO buffer overrun.
*	@param pEntry Data descriptor
*	@param pConn Connection object
*	@param buffer Pointer to destination memory
*	@param bufferSize Size of destination buffer
**/
NTSTATUS ctrl_copyTcpSendToBuffer(PNF_QUEUE_ENTRY pEntry, 
								  PCONNOBJ pConn, 
								  char ** ppBuffer, 
								  unsigned long * pBufferSize)
{
	NTSTATUS status	= STATUS_SUCCESS;

	KdPrint((DPREFIX"ctrl_copyTcpSendToBuffer[%I64d]\n", pEntry->id));

	// Do not indicate sends for suspended connections
	if ((pConn->filteringFlag & NF_SUSPENDED) 
		&& ((pConn->sendBytesInTdi > TDI_SNDBUF) 
				|| IsListEmpty(&pConn->receivedPackets)))
	{
		KdPrint((DPREFIX"ctrl_copyTcpSendToBuffer[%I64d]: suspended connection\n", pEntry->id));
		return status;
	}

	if (!(pConn->flags & TDI_SEND_AND_DISCONNECT) && !IsListEmpty(&pConn->pendedSendRequests))
	{
		PLIST_ENTRY					pListEntry;
		PTDI_REQUEST_KERNEL_SEND    prk;
		PIRP						irp;
		PIO_STACK_LOCATION			irpSp;
		unsigned long				totalSendLength = 0;
		unsigned long				sendLength = 0;
		unsigned long				offset = 0;
		unsigned long				flags = 0;

		pListEntry = pConn->pendedSendRequests.Flink;
			
		irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		irpSp = IoGetCurrentIrpStackLocation(irp);
		ASSERT(irpSp);

		offset = (unsigned long)irp->IoStatus.Information;

	    prk = (PTDI_REQUEST_KERNEL_SEND)&irpSp->Parameters;
		if (prk)
		{
			totalSendLength = prk->SendLength;
			sendLength = prk->SendLength - offset;
			flags = prk->SendFlags;

			// Split large buffers to smaller chunks
			if (sendLength > NF_TCP_PACKET_BUF_SIZE)
			{
				sendLength = NF_TCP_PACKET_BUF_SIZE;
			}

			if (totalSendLength == 0)
			{
				// Do not copy the packet and complete send request immediately
				status = STATUS_SUCCESS;
			} else
			{
				if (irp->MdlAddress)
				{
					status = ctrl_copyMdlToBuffer(
									pEntry, 
									irp->MdlAddress, 
									sendLength,
									offset,
									ppBuffer,
									pBufferSize,
									0);
				}
			}
		}

		if (NT_SUCCESS(status))
		{
			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = offset + sendLength;

			if ((offset + sendLength) >= totalSendLength)
			{
				//  All buffer is grabbed. Complete TDI_SEND IRP.
				if (!(flags & TDI_SEND_AND_DISCONNECT))
				{
					RemoveHeadList(&pConn->pendedSendRequests);
					InitializeListHead(&irp->Tail.Overlay.ListEntry);

					IoSetCancelRoutine(irp, NULL);

					// Complete IRP from DPC
					ctrl_deferredCompleteIrp(irp, FALSE);
				} else
				{
					PNF_QUEUE_ENTRY	pNewEntry;
					PNF_PACKET		pPacket;

					// Add disconnect request to IO queue
					KdPrint((DPREFIX"ctrl_copyTcpSendToBuffer[%I64d]: disconnect\n", pEntry->id));
					
					pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
					if (pPacket)
					{
						pPacket->dataSize = 0;
						pPacket->ioStatus = STATUS_UNSUCCESSFUL;

						InsertTailList(&pConn->pendedSendPackets, &pPacket->entry);

						pNewEntry = (PNF_QUEUE_ENTRY)mp_alloc(sizeof(NF_QUEUE_ENTRY));
						if (!pNewEntry)
						{
							return STATUS_UNSUCCESSFUL;
						}
		#if DBG
						g_vars.nIOReq++;
						KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
		#endif

						pNewEntry->code = pEntry->code;
						pNewEntry->id = pEntry->id;

						InsertTailList(&g_vars.ioQueue, &pNewEntry->entry);

						pConn->flags = TDI_SEND_AND_DISCONNECT;
					}
				}

			} else
			{
				PNF_QUEUE_ENTRY	pNewEntry;

				// Add NF_TCP_SEND request to IO queue to indicate the next chunk

				KdPrint((DPREFIX"ctrl_copyTcpSendToBuffer[%I64d]: partial send\n", pEntry->id));

				pNewEntry = (PNF_QUEUE_ENTRY)mp_alloc(sizeof(NF_QUEUE_ENTRY));
				if (!pNewEntry)
				{
					return STATUS_UNSUCCESSFUL;
				}
#if DBG
				g_vars.nIOReq++;
				KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif

				pNewEntry->code = pEntry->code;
				pNewEntry->id = pEntry->id;

				InsertTailList(&g_vars.ioQueue, &pNewEntry->entry);
			}
		}
	} else
	if (!IsListEmpty(&pConn->pendedSendPackets))
	{
		PNF_PACKET	pPacket;
	
		pPacket = (PNF_PACKET)pConn->pendedSendPackets.Flink;

		status = ctrl_copyPacketToBuffer(pEntry, pPacket, ppBuffer, pBufferSize, 0);
		if (NT_SUCCESS(status))
		{
			RemoveEntryList(&pPacket->entry);
			nf_packet_free(pPacket);
		}				
	}

	return status;
}

/**
*	Copy the contents of received packet to buffer as NF_DATA record. 
*	@param pEntry Data descriptor
*	@param pConn Connection object
*	@param buffer Pointer to destination memory
*	@param bufferSize Size of destination buffer
**/
NTSTATUS ctrl_copyTcpReceiveToBuffer(PNF_QUEUE_ENTRY pEntry, 
								  PCONNOBJ pConn, 
								  char ** ppBuffer, 
								  unsigned long * pBufferSize)
{
	NTSTATUS status = STATUS_SUCCESS;
	PNF_PACKET	pPacket;

	KdPrint((DPREFIX"ctrl_copyTcpReceiveToBuffer[%I64d]\n", pEntry->id));

	pPacket = (PNF_PACKET)pConn->pendedReceivedPackets.Flink;
	if (pPacket == (PNF_PACKET)&pConn->pendedReceivedPackets)
	{
		if (!(pConn->filteringFlag & NF_SUSPENDED))
		{
			KdPrint((DPREFIX"ctrl_copyTcpReceiveToBuffer[%I64d]: starting internalReceiveTimer\n", pEntry->id));
			tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
		} else
		{
			KdPrint((DPREFIX"ctrl_copyTcpReceiveToBuffer[%I64d]: connection suspended\n", pConn->id));
		}
		return status;
	}

	status = ctrl_copyPacketToBuffer(pEntry, pPacket, ppBuffer, pBufferSize, 0);
	if (NT_SUCCESS(status))
	{
		RemoveEntryList(&pPacket->entry);

		if (pPacket->dataSize > 0)
		{
			if (!(pConn->filteringFlag & NF_SUSPENDED))
			{
				KdPrint((DPREFIX"ctrl_copyTcpReceiveToBuffer[%I64d]: starting internalReceiveTimer\n", pEntry->id));
				tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
			} else
			{
				KdPrint((DPREFIX"ctrl_copyTcpReceiveToBuffer[%I64d]: connection suspended\n", pConn->id));
			}
		} else
		{
			KdPrint((DPREFIX"ctrl_copyTcpReceiveToBuffer[%I64d]: zero data\n", pEntry->id));
		}

		nf_packet_free(pPacket);
	}				

	return status;
}


/**
*	Fill IO buffer with queued TCP data
*	@param pEntry Data descriptor
*	@param ppBuffer Pointer to destination memory
*	@param pBufferSize Size of destination buffer
**/
NTSTATUS ctrl_popTcpData(PNF_QUEUE_ENTRY pEntry, char ** ppBuffer, unsigned long * pBufferSize)
{
	NTSTATUS			status = STATUS_SUCCESS;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	PNF_PACKET			pPacket;
	unsigned long		dataSize;
	NF_DATA	*	pData;
	KIRQL				irql, irqld;

	KdPrint((DPREFIX"ctrl_popTcpData[%I64d]\n", pEntry->id));

	switch (pEntry->code)
	{
	case NF_TCP_CONNECTED:
	case NF_TCP_CLOSED:
	case NF_TCP_CONNECT_REQUEST:
		{
			dataSize = sizeof(NF_DATA) - 1 + pEntry->bufferSize;

			if (*pBufferSize < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)*ppBuffer;

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = pEntry->bufferSize;
			
			if (pEntry->bufferSize)
			{
				memcpy(pData->buffer, pEntry->buffer, pEntry->bufferSize);
			}

			*pBufferSize -= dataSize;
			*ppBuffer += dataSize;
			
			return STATUS_SUCCESS;
		}
	
	case NF_TCP_CAN_SEND:
	case NF_TCP_CAN_RECEIVE:
	case NF_TCP_REINJECT:
		{
			dataSize = sizeof(NF_DATA) - 1;

			if (*pBufferSize < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)*ppBuffer;

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = 0;
			
			*pBufferSize -= dataSize;
			*ppBuffer += dataSize;
			
			return STATUS_SUCCESS;
		}
	}

	phte = ht_find_entry(g_vars.phtConnById, pEntry->id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		if (pConn->fileObject)
		{
			switch (pEntry->code)
			{
			case NF_TCP_RECEIVE:
				{
					status = ctrl_copyTcpReceiveToBuffer(pEntry, pConn, ppBuffer, pBufferSize);
				}
				break;
			case NF_TCP_SEND:
				{
					status = ctrl_copyTcpSendToBuffer(pEntry, pConn, ppBuffer, pBufferSize);
				}
				break;
			}
		}
	}

	KdPrint((DPREFIX"ctrl_popTcpData[%I64d] finished\n", pEntry->id));

	return status;
}

/**
*	Copy the contents of pended TDI_SEND_DATAGRAM IRP to specified buffer 
*	@param pEntry Data descriptor
*	@param pAddr Address object
*	@param ppBuffer Pointer to destination memory
*	@param pBufferSize Size of destination buffer
**/
NTSTATUS ctrl_copyUdpSendToBuffer(PNF_QUEUE_ENTRY pEntry, 
								  PADDROBJ pAddr, 
								  char ** ppBuffer, 
								  unsigned long * pBufferSize)
{
	NTSTATUS status	= STATUS_SUCCESS;

	KdPrint((DPREFIX"ctrl_copyUdpSendToBuffer[%I64d]\n", pEntry->id));

	if (pAddr->filteringFlag & NF_SUSPENDED)
	{
		KdPrint((DPREFIX"ctrl_copyUdpSendToBuffer[%I64d]: suspended address\n", pEntry->id));
		return status;
	}

	if (!IsListEmpty(&pAddr->pendedSendRequests))
	{
		PLIST_ENTRY					pListEntry;
		PIRP						irp;
		PIO_STACK_LOCATION			irpSp;
		PTRANSPORT_ADDRESS			pRemoteAddr = NULL;

		pListEntry = pAddr->pendedSendRequests.Flink;
			
		irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		irpSp = IoGetCurrentIrpStackLocation(irp);
		ASSERT(irpSp);

		if (irpSp->MinorFunction == TDI_SEND_DATAGRAM)
		{
			// TDI_SEND_DATAGRAM request

			PTDI_REQUEST_KERNEL_SENDDG  prk;
			unsigned long extraBytes = 0; 
	
			prk = (PTDI_REQUEST_KERNEL_SENDDG)&irpSp->Parameters;
			if (prk)
			{
				if (prk->SendDatagramInformation &&
					prk->SendDatagramInformation->RemoteAddress)
				{
					pRemoteAddr = (TRANSPORT_ADDRESS *)(prk->SendDatagramInformation->RemoteAddress);
				}

				extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + prk->SendDatagramInformation->OptionsLength;
				
				if ((extraBytes + prk->SendLength) > *pBufferSize)
				{
					status = STATUS_NO_MEMORY;
				} else
				{
					if (irp->MdlAddress)
					{
						status = ctrl_copyUdpMdlToBuffer(
											pEntry, 
											irp->MdlAddress, 
											prk->SendLength,
											0,
											ppBuffer,
											pBufferSize,
											0, 
											prk->SendDatagramInformation->OptionsLength,
											prk->SendDatagramInformation->Options,
											(UCHAR*)pRemoteAddr,
											NULL);					
					}
				}
			}

			if (NT_SUCCESS(status))
			{
				RemoveHeadList(&pAddr->pendedSendRequests);
				InitializeListHead(&irp->Tail.Overlay.ListEntry);

				IoSetCancelRoutine(irp, NULL);

				irp->IoStatus.Status = STATUS_SUCCESS;
				irp->IoStatus.Information = prk->SendLength;

				// Complete IRP from DPC
				ctrl_deferredCompleteIrp(irp, FALSE);
			}
		} else
		{
			// TDI_SEND request

			PTDI_REQUEST_KERNEL_SEND    prk;

			prk = (PTDI_REQUEST_KERNEL_SEND)&irpSp->Parameters;
			if (prk)
			{
				unsigned long extraBytes = 0; 
		
				extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1;

				if ((extraBytes + prk->SendLength) > *pBufferSize)
				{
					status = STATUS_NO_MEMORY;
				} else
				{
					if (irp->MdlAddress)
					{
						status = ctrl_copyUdpMdlToBuffer(
											pEntry, 
											irp->MdlAddress, 
											prk->SendLength,
											0,
											ppBuffer,
											pBufferSize,
											0, 
											0,
											NULL,
											NULL,
											(UCHAR*)pAddr->remoteAddr);					
					}
				}
			}

			if (NT_SUCCESS(status))
			{
				RemoveHeadList(&pAddr->pendedSendRequests);
				InitializeListHead(&irp->Tail.Overlay.ListEntry);

				IoSetCancelRoutine(irp, NULL);

				irp->IoStatus.Status = STATUS_SUCCESS;
				irp->IoStatus.Information = prk->SendLength;

				// Complete IRP from DPC
				ctrl_deferredCompleteIrp(irp, FALSE);
			}
		}
	}

	return status;
}

/**
*	Copy the pended UDP packet to specified buffer 
*	@param pEntry Data descriptor
*	@param pAddr Address object
*	@param ppBuffer Pointer to destination memory
*	@param pBufferSize Size of destination buffer
**/
NTSTATUS ctrl_copyUdpReceiveToBuffer(PNF_QUEUE_ENTRY pEntry, 
								  PADDROBJ pAddr, 
								  char ** ppBuffer, 
								  unsigned long * pBufferSize)
{
	NTSTATUS status = STATUS_SUCCESS;
	PNF_PACKET	pPacket;
	unsigned long extraBytes = 0; 

	KdPrint((DPREFIX"ctrl_copyUdpReceiveToBuffer[%I64d]\n", pEntry->id));

	pPacket = (PNF_PACKET)pAddr->pendedReceivedPackets.Flink;
	if (pPacket == (PNF_PACKET)&pAddr->pendedReceivedPackets)
	{
		return status;
	}

	extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + pPacket->connInfo.OptionsLength;

	if ((extraBytes + pPacket->dataSize) > *pBufferSize)
	{
		return STATUS_NO_MEMORY;
	} 

	status = ctrl_copyUdpPacketToBuffer(pEntry, pPacket, ppBuffer, pBufferSize);
	
	if (NT_SUCCESS(status))
	{
		RemoveEntryList(&pPacket->entry);
		nf_packet_free(pPacket);
	}				

	return status;
}

/**
*	Copy the pended UDP data to specified buffer 
*	@param pEntry Data descriptor
*	@param ppBuffer Pointer to destination memory
*	@param pBufferSize Size of destination buffer
**/
NTSTATUS ctrl_popUdpData(PNF_QUEUE_ENTRY pEntry, char ** ppBuffer, unsigned long * pBufferSize)
{
	NTSTATUS			status = STATUS_SUCCESS;
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	PNF_PACKET			pPacket;
	unsigned long		dataSize;
	NF_DATA	*	pData;
	KIRQL				irql, irqld;

	KdPrint((DPREFIX"ctrl_popUdpData[%I64d]\n", pEntry->id));

	switch (pEntry->code)
	{
	case NF_UDP_CREATED:
	case NF_UDP_CLOSED:
	case NF_UDP_CONNECT_REQUEST:
		{
			dataSize = sizeof(NF_DATA) - 1 + pEntry->bufferSize;

			if (*pBufferSize < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)*ppBuffer;

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = pEntry->bufferSize;
			
			if (pEntry->bufferSize)
			{
				memcpy(pData->buffer, pEntry->buffer, pEntry->bufferSize);
			}

			*pBufferSize -= dataSize;
			*ppBuffer += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_UDP_CAN_SEND:
	case NF_UDP_CAN_RECEIVE:
		{
			dataSize = sizeof(NF_DATA) - 1;

			if (*pBufferSize < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)*ppBuffer;

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = 0;
			
			*pBufferSize -= dataSize;
			*ppBuffer += dataSize;
			
			return STATUS_SUCCESS;
		}
	}

	phte = ht_find_entry(g_vars.phtAddrById, pEntry->id);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

		if (pAddr->fileObject)
		{
			switch (pEntry->code)
			{
			case NF_UDP_RECEIVE:
				{
					status = ctrl_copyUdpReceiveToBuffer(pEntry, pAddr, ppBuffer, pBufferSize);
				}
				break;
			case NF_UDP_SEND:
				{
					status = ctrl_copyUdpSendToBuffer(pEntry, pAddr, ppBuffer, pBufferSize);
				}
				break;
			}
		}
	}

	return status;
}

/**
*	Fill the specified buffer with TCP and UDP data, described by ioQueue elements.
*	@param buffer Pointer to destination memory
*	@param bufferSize Size of destination buffer
**/
unsigned long ctrl_fillIoBuffer(char * buffer, unsigned long bufferSize)
{
	PNF_QUEUE_ENTRY	pEntry;
	unsigned long remaining = bufferSize;
	NTSTATUS	status = STATUS_SUCCESS;

	while (!IsListEmpty(&g_vars.ioQueue))
	{
		pEntry = (PNF_QUEUE_ENTRY)g_vars.ioQueue.Flink;

		switch (pEntry->code)
		{
		case NF_TCP_CONNECTED:
		case NF_TCP_CLOSED:
		case NF_TCP_RECEIVE:
		case NF_TCP_SEND:
		case NF_TCP_CAN_SEND:
		case NF_TCP_CAN_RECEIVE:
		case NF_TCP_CONNECT_REQUEST:
		case NF_TCP_REINJECT:
			status = ctrl_popTcpData(pEntry, &buffer, &remaining);
			break;

		case NF_UDP_CREATED:
		case NF_UDP_CLOSED:
		case NF_UDP_RECEIVE:
		case NF_UDP_SEND:
		case NF_UDP_CAN_SEND:
		case NF_UDP_CAN_RECEIVE:
		case NF_UDP_CONNECT_REQUEST:
			status = ctrl_popUdpData(pEntry, &buffer, &remaining);
			break;

		default:
			{
				unsigned long		dataSize;
				PNF_DATA			pData;

				KdPrint((DPREFIX"ctrl_fillIoBuffer[%I64d] - status %d\n", pEntry->id, pEntry->code));
	
				dataSize = sizeof(NF_DATA) - 1;

				if (remaining < dataSize)
				{
					status = STATUS_NO_MEMORY;
				} else
				{
					pData = (PNF_DATA) buffer;

					pData->code = pEntry->code;
					pData->id = pEntry->id;
					pData->bufferSize = 0;
					
					remaining -= dataSize;
					buffer += dataSize;
				}
				break;
			}
		}

		if (!NT_SUCCESS(status))
		{
			break;
		}

		RemoveEntryList(&pEntry->entry);
		
		mp_free(pEntry, MAX_QE_POOL_SIZE);
#if DBG
		g_vars.nIOReq--;
		KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif
	}

	return bufferSize - remaining;
}

/**
*	Try to put the receive packet to queue and indicate the first queued packet to application.
*	@param pConn Connection object
*	@param pPacket Data packet
*	@param pProcessedBytes Number of bytes copied
**/
BOOLEAN ctrl_processTcpReceivePacket(PCONNOBJ pConn, PNF_PACKET pPacket, ULONG * pProcessedBytes)
{
	KdPrint((DPREFIX"ctrl_processTcpReceivePacket[%I64d]\n", pConn->id));

	if (!IsListEmpty(&pConn->receivedPackets) && 
		IsListEmpty(&pConn->pendedReceiveRequests))
	{	
		// Indicate received data
		tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);

		return FALSE;
	}

	pPacket->fileObject = (PFILE_OBJECT)pConn->fileObject;

	InsertTailList(
			&pConn->receivedPackets,
			&pPacket->entry
		   );

	*pProcessedBytes = sizeof(NF_DATA) + pPacket->dataSize - 1;
		
	KdPrint((DPREFIX"ctrl_processTcpReceivePacket[%I64d]: packetProcessed\n", pConn->id));

	// Indicate received data
	tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);

	return TRUE;
}

/**
*	Try to enqueue send packet.
*	@param pConn Connection object
*	@param pPacket Data packet
*	@param pProcessedBytes Number of bytes copied
**/
BOOLEAN ctrl_processTcpSendPacket(PCONNOBJ pConn, PNF_PACKET pPacket, ULONG * pProcessedBytes)
{
	KdPrint((DPREFIX"ctrl_processTcpSendPacket[%I64d]\n", pConn->id));

	if (pConn->sendInProgress && (pPacket->dataSize > 0))
	{	
		return FALSE;
	}

	pPacket->fileObject = (PFILE_OBJECT)pConn->fileObject;

	InsertTailList(
			&pConn->sendPackets,
			&pPacket->entry
		   );

	*pProcessedBytes = sizeof(NF_DATA) + pPacket->dataSize - 1;
		
	KdPrint((DPREFIX"ctrl_processTcpSendPacket[%I64d]: packetProcessed\n", pConn->id));

	// Send data
	//tcp_deferredStartInternalSend(pConn);

	return TRUE;
}

/**
*	Try to process TCP-related request from proxy.
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_processTcpPacket(PNF_DATA pData)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	PNF_PACKET pPacket;
	unsigned long result = 0;
	BOOLEAN	packetProcessed = FALSE;
//	PFILE_OBJECT fileObject = NULL;
	KIRQL irql, irqld;
	NTSTATUS status;

	KdPrint((DPREFIX"ctrl_processTcpPacket[%I64d]\n", pData->id));

	if (pData->bufferSize > NF_TCP_PACKET_BUF_SIZE)
		return 0;

	pPacket = nf_packet_alloc(pData->bufferSize);
	if (!pPacket)
		return 0;

	if (pData->bufferSize)
	{
		RtlCopyMemory(pPacket->buffer, pData->buffer, pData->bufferSize);
	}

	pPacket->dataSize = pData->bufferSize;

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnById, pData->id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		sl_lock(&pConn->lock, &irqld);
		
		if (pConn->fileObject)
		{
//			fileObject = (PFILE_OBJECT)pConn->fileObject;
//			ObReferenceObject(fileObject);

			switch (pData->code)
			{
			case NF_TCP_RECEIVE:
				{
					packetProcessed = ctrl_processTcpReceivePacket(pConn, pPacket, &result);
				} 
				break;
			case NF_TCP_SEND:
				{
					packetProcessed = ctrl_processTcpSendPacket(pConn, pPacket, &result);
				} 
				break;
			}
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_processTcpPacket[%I64d]: connection not found\n", pData->id));
		result = sizeof(NF_DATA) + pData->bufferSize - 1;
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (!packetProcessed)
	{
		KdPrint((DPREFIX"ctrl_processTcpPacket[%I64d]: !packetProcessed\n", pData->id));
		nf_packet_free(pPacket);
	} else
	{
		if (pData->code == NF_TCP_SEND)
		{
			// Issue TDI_SEND requests from PASSIVE IRQL

			if (!NT_SUCCESS(tcp_TdiStartInternalSend(pConn, pData->id)))
			{
				result = 0;
			}
		}
	}
/*
	if (fileObject)
	{
		ObDereferenceObject(fileObject);
	}
*/
	return result;
}


/**
*	Process the filtered TCP connect request
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_processTcpConnect(PNF_DATA pData)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	unsigned long result = sizeof(NF_DATA) + pData->bufferSize - 1;
	KIRQL irql, irqld;
	PIRP	irp = NULL;
	PIO_STACK_LOCATION irpSp = NULL;
	PDEVICE_OBJECT pLowerDevice;

	KdPrint((DPREFIX"ctrl_processTcpConnect[%I64d]\n", pData->id));

	if (pData->bufferSize < sizeof(NF_TCP_CONN_INFO))
		return 0;

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnById, pData->id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		sl_lock(&pConn->lock, &irqld);
		
		if (pConn->fileObject)
		{
			PNF_TCP_CONN_INFO pci = (PNF_TCP_CONN_INFO)pData->buffer;
			
			// Update filtering flag
			pConn->filteringFlag = pci->filteringFlag;
			
			// Update remote address
			memcpy(pConn->remoteAddr, pci->remoteAddress, sizeof(pci->remoteAddress));

			if (!IsListEmpty(&pConn->pendedConnectRequest))
			{
				PLIST_ENTRY pEntry;
				PTDI_REQUEST_KERNEL  prk;

				pEntry = RemoveHeadList(&pConn->pendedConnectRequest);
				
				irp = (PIRP)CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
				InitializeListHead(&irp->Tail.Overlay.ListEntry);

				IoSetCancelRoutine(irp, NULL);

				irpSp = IoGetCurrentIrpStackLocation(irp);
				ASSERT(irpSp);

				prk = (PTDI_REQUEST_KERNEL )&irpSp->Parameters;

				if (prk &&
					prk->RequestConnectionInformation &&
					prk->RequestConnectionInformation->RemoteAddress)
				{
					PTA_ADDRESS pRemoteAddr = NULL;
					int addrLen = 0;

					pRemoteAddr = ((TRANSPORT_ADDRESS *)(prk->RequestConnectionInformation->RemoteAddress))->Address;
					if (pRemoteAddr)
					{
						addrLen = min(pRemoteAddr->AddressLength + sizeof(USHORT), sizeof(pConn->remoteAddr));
						// Replace remote address for TDI_CONNECT request
						memcpy((char*)pRemoteAddr + sizeof(USHORT), pConn->remoteAddr, addrLen);
					}
				} 

				if (pConn->filteringFlag & NF_BLOCK)
				{
					irp->IoStatus.Status = STATUS_REMOTE_NOT_LISTENING;
					irp->IoStatus.Information = 0;
					ctrl_deferredCompleteIrp(irp, FALSE);
					irp = NULL;
				} else
				if ((pConn->filteringFlag & (NF_OFFLINE | NF_FILTER)) == (NF_OFFLINE | NF_FILTER))
				{
					// Complete connect request immediately for offline connection
					ctrl_queuePushEx(NF_TCP_CONNECTED, pConn->id, (char*)pci, sizeof(NF_TCP_CONN_INFO));

					irp->IoStatus.Status = STATUS_SUCCESS;
					irp->IoStatus.Information = 0;

					ctrl_deferredCompleteIrp(irp, FALSE);
					irp = NULL;
				} else
				{
					pLowerDevice = pConn->lowerDevice;
					if (!pLowerDevice)
					{
						ASSERT(0);
						irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
						irp->IoStatus.Information = 0;
						
						// Complete IRP from DPC
						ctrl_deferredCompleteIrp(irp, FALSE);
						irp = NULL;
					}
				}
			}
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_processTcpConnect[%I64d]: connection not found\n", pData->id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (pConn && irp && pLowerDevice)
	{
		PIO_HOOK_CONTEXT pHookContext = (PIO_HOOK_CONTEXT)malloc_np(sizeof(IO_HOOK_CONTEXT));
		if (!pHookContext)
		{
			IoSkipCurrentIrpStackLocation(irp);
			IoCallDriver(pLowerDevice, irp);
			return result;
		}

		pHookContext->old_cr = irpSp->CompletionRoutine;
		pHookContext->new_context = pConn;
		pHookContext->old_context = irpSp->Context;
		pHookContext->fileobj = irpSp->FileObject;
		pHookContext->old_control = irpSp->Control;

		IoSkipCurrentIrpStackLocation(irp);
//		IoCopyCurrentIrpStackLocationToNext(irp);
	        
		IoSetCompletionRoutine(
				irp,
				tcp_TdiConnectComplete,
				pHookContext,
				TRUE,
				TRUE,
				TRUE
				);
	        
		IoCallDriver(pLowerDevice, irp);
	}

	return result;
}

/**
*	Change the state of TCP connection according to pData request
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_setTcpConnState(PNF_DATA pData)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;

	KdPrint((DPREFIX"ctrl_setTcpConnState[%I64d]: %d\n", pData->id, pData->code));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnById, pData->id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		sl_lock(&pConn->lock, &irqld);
		
		if (pConn->fileObject)
		{
			switch (pData->code)
			{
			case NF_TCP_REQ_SUSPEND:
				{
					KdPrint((DPREFIX"ctrl_setTcpConnState[%I64d]: connection suspended\n", pData->id));
					if (pConn->filteringFlag & NF_FILTER)
					{
						pConn->filteringFlag |= NF_SUSPENDED;
					}
				}
				break;
			case NF_TCP_REQ_RESUME:
				{
					int i, listSize;

					if ((pConn->filteringFlag & NF_FILTER) && (pConn->filteringFlag & NF_SUSPENDED))
					{
						KdPrint((DPREFIX"ctrl_setTcpConnState[%I64d]: connection resumed\n", pData->id));
				
						pConn->filteringFlag &= ~NF_SUSPENDED;

						listSize = tu_getListSize(&pConn->pendedReceivedPackets);
						for (i=0; i<listSize; i++)
						{
							ctrl_queuePush(NF_TCP_RECEIVE, pConn->id);
						}

						if ((pConn->direction == NF_D_IN) ||
							((pConn->filteringFlag & (NF_OFFLINE | NF_FILTER)) != (NF_OFFLINE | NF_FILTER)))
						{
							KdPrint((DPREFIX"ctrl_setTcpConnState[%I64d]: starting internalReceiveTimer\n", pData->id));
							tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
						}

						listSize = tu_getListSize(&pConn->pendedSendRequests);
						for (i=0; i<listSize; i++)
						{
							ctrl_queuePush(NF_TCP_SEND, pConn->id);
						}

						listSize = tu_getListSize(&pConn->pendedSendPackets);
						for (i=0; i<listSize; i++)
						{
							ctrl_queuePush(NF_TCP_SEND, pConn->id);
						}
					}
				}
				break;
			}
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_setTcpConnState[%I64d]: connection not found\n", pData->id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	return sizeof(NF_DATA) - 1;
}

unsigned long ctrl_addRule(PNF_DATA pData, BOOLEAN toHead)
{
	PRULE_ENTRY pRule;

	if (pData->bufferSize < sizeof(NF_RULE))
		return 0;

	pRule = (PRULE_ENTRY)mp_alloc(sizeof(RULE_ENTRY));
	if (!pRule)
		return 0;

	memset(&pRule->rule, 0, sizeof(NF_RULE_EX));
	memcpy(&pRule->rule, pData->buffer, sizeof(NF_RULE));

	rule_add(pRule, toHead);

	return sizeof(NF_DATA) - 1 + pData->bufferSize;
}

/**
*	Disables indicating packets to user mode for the specified connection
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_tcpDisableUserModeFiltering(PNF_DATA pData)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;
	unsigned long result = 0;

	KdPrint((DPREFIX"ctrl_tcpDisableFiltering[%I64d]\n", pData->id));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnById, pData->id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		sl_lock(&pConn->lock, &irqld);
		
		if (pConn->fileObject && pConn->lowerDevice && (pConn->filteringFlag & NF_FILTER))
		{
			PNF_PACKET	pPacket = NULL;

			pConn->disableUserModeFiltering = TRUE;

			if (!IsListEmpty(&pConn->pendedReceivedPackets))
			{
				while (!IsListEmpty(&pConn->pendedReceivedPackets))
				{
					pPacket = (PNF_PACKET)RemoveHeadList(&pConn->pendedReceivedPackets);

					InsertTailList(
						&pConn->receivedPackets,
						&pPacket->entry
					   );
				}

				tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);
			}

			while (!IsListEmpty(&pConn->pendedSendRequests))
			{
				PIRP irp;
				PIO_STACK_LOCATION	irpSp;
				PLIST_ENTRY pEntry;
				NTSTATUS status;

				pEntry = pConn->pendedSendRequests.Flink;
			
				irp = (PIRP)CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);

				irpSp = IoGetCurrentIrpStackLocation(irp);
				ASSERT(irpSp);

				if (irp->IoStatus.Information > 0)
					break;

				RemoveHeadList(&pConn->pendedSendRequests);
				
				InitializeListHead(&irp->Tail.Overlay.ListEntry);

				IoSetCancelRoutine(irp, NULL);

				sl_unlock(&pConn->lock, irqld);
				sl_unlock(&g_vars.slConnections, irql);

				IoSkipCurrentIrpStackLocation(irp);

				status = IoCallDriver(pConn->lowerDevice, irp);

				sl_lock(&g_vars.slConnections, &irql);
				sl_lock(&pConn->lock, &irqld);
			}

			if (IsListEmpty(&pConn->pendedSendRequests) &&
				IsListEmpty(&pConn->pendedSendPackets) &&
				IsListEmpty(&pConn->pendedReceiveRequests) &&
				IsListEmpty(&pConn->pendedReceivedPackets) &&
				IsListEmpty(&pConn->pendedDisconnectRequest) &&
				IsListEmpty(&pConn->receivedPackets) &&
				!pConn->receiveInProgress &&
				!pConn->recvBytesInTdi &&
				IsListEmpty(&pConn->sendPackets) &&
				!pConn->sendInProgress)
			{
				KdPrint((DPREFIX"ctrl_tcpDisableFiltering[%I64d]: user mode filtering disabled\n", pData->id));

				pConn->userModeFilteringDisabled = TRUE;
				pConn->filteringFlag = NF_ALLOW;
				result = sizeof(NF_DATA) - 1;	
			}
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_tcpDisableFiltering[%I64d]: connection not found\n", pData->id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	return result;
}

//
//  UDP
//

/**
*	Try to put the receive packet to queue and indicate the first queued packet to application.
*	@param pAddr Address object
*	@param pPacket Data packet
*	@param pProcessedBytes Number of bytes copied
**/
BOOLEAN ctrl_processUdpReceivePacket(PADDROBJ pAddr, PNF_PACKET pPacket, ULONG * pProcessedBytes)
{
	KdPrint((DPREFIX"ctrl_processUdpReceivePacket[%I64d]\n", pAddr->id));

	if (!IsListEmpty(&pAddr->receivedPackets))
	{	
		// Indicate received data
		udp_startDeferredProc(pAddr, DQC_UDP_INDICATE_RECEIVE);

		return FALSE;
	}

	pPacket->fileObject = (PFILE_OBJECT)pAddr->fileObject;

	InsertTailList(
			&pAddr->receivedPackets,
			&pPacket->entry
		   );

	*pProcessedBytes = sizeof(NF_DATA) + pPacket->dataSize - 1;
		
	KdPrint((DPREFIX"ctrl_processUdpReceivePacket[%I64d]: packetProcessed\n", pAddr->id));

	// Indicate received data
	udp_startDeferredProc(pAddr, DQC_UDP_INDICATE_RECEIVE);

	return TRUE;
}

/**
*	Try to enqueue send packet.
*	@param pAddr Address object
*	@param pPacket Data packet
*	@param pProcessedBytes Number of bytes copied
**/
BOOLEAN ctrl_processUdpSendPacket(PADDROBJ pAddr, PNF_PACKET pPacket, ULONG * pProcessedBytes)
{
	KdPrint((DPREFIX"ctrl_processUdpSendPacket[%I64d]\n", pAddr->id));

	if (pAddr->sendInProgress)
	{	
		return FALSE;
	}

	pPacket->fileObject = (PFILE_OBJECT)pAddr->fileObject;

	InsertTailList(
			&pAddr->sendPackets,
			&pPacket->entry
		   );

	*pProcessedBytes = sizeof(NF_DATA) + pPacket->dataSize - 1;
		
	KdPrint((DPREFIX"ctrl_processUdpSendPacket[%I64d]: packetProcessed\n", pAddr->id));

	// Send data
	//udp_deferredStartInternalSend(pAddr);

	return TRUE;
}

/**
*	Try to process UDP-related request from proxy.
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_processUdpPacket(PNF_DATA pData)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	PNF_PACKET pPacket;
	unsigned long result = 0;
	BOOLEAN	packetProcessed = FALSE;
	KIRQL irql, irqld;
	ULONG		packetSize;
	ULONG		extraBytes;
	PNF_UDP_OPTIONS pOptions;
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint((DPREFIX"ctrl_processUdpPacket[%I64d]\n", pData->id));

	if (pData->bufferSize < NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS))
		return 0;
		
	pOptions = (PNF_UDP_OPTIONS)(pData->buffer + NF_MAX_ADDRESS_LENGTH);

	extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + pOptions->optionsLength;

	KdPrint((DPREFIX"ctrl_processUdpPacket[%I64d] optionsLength=%d, extraBytes=%d\n", pData->id, pOptions->optionsLength, extraBytes));

	if (pData->bufferSize < extraBytes)
		return 0;

	packetSize = pData->bufferSize - extraBytes;

	pPacket = nf_packet_alloc(packetSize);
	if (!pPacket)
		return 0;

	if (pData->bufferSize)
	{
		UCHAR * pDataBuffer = pData->buffer;
		ULONG	bufferSize = pData->bufferSize;
		PTA_ADDRESS			pTAAddr;
		PTRANSPORT_ADDRESS	pTRAddr;
	
		pPacket->flags = pOptions->flags;

		pTRAddr = (PTRANSPORT_ADDRESS)pPacket->remoteAddr;

		pTRAddr->TAAddressCount = 1;
		
		pTAAddr = pTRAddr->Address;

		RtlCopyMemory(&pTAAddr->AddressType, pDataBuffer, NF_MAX_ADDRESS_LENGTH);

		pTAAddr->AddressLength = (pTAAddr->AddressType == AF_INET6)? 
						sizeof(struct sockaddr_in6) - sizeof(USHORT) : 
						sizeof(struct sockaddr_in) - sizeof(USHORT);

		pPacket->connInfo.RemoteAddressLength = sizeof(TRANSPORT_ADDRESS) - 4 + 
								pTAAddr->AddressLength;

		pPacket->connInfo.RemoteAddress = pPacket->remoteAddr;
	
		pDataBuffer += NF_MAX_ADDRESS_LENGTH;
		bufferSize -= NF_MAX_ADDRESS_LENGTH;

		if (pOptions->optionsLength > 0)
		{
			pPacket->connInfo.OptionsLength = pOptions->optionsLength;
			pPacket->connInfo.Options = malloc_np(pOptions->optionsLength);
			if (!pPacket->connInfo.Options)
			{
				nf_packet_free(pPacket);
				return 0;
			}
			RtlCopyMemory(pPacket->connInfo.Options, pOptions->options, pOptions->optionsLength);
		}

		pDataBuffer += (sizeof(NF_UDP_OPTIONS) - 1 + pOptions->optionsLength);
		bufferSize -= (sizeof(NF_UDP_OPTIONS) - 1 + pOptions->optionsLength);

		RtlCopyMemory(pPacket->buffer, pDataBuffer, bufferSize);

		pPacket->dataSize = bufferSize;
	}

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtAddrById, pData->id);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

		sl_lock(&pAddr->lock, &irqld);
		
		if (pAddr->fileObject)
		{
			switch (pData->code)
			{
			case NF_UDP_RECEIVE:
				{
					packetProcessed = ctrl_processUdpReceivePacket(pAddr, pPacket, &result);
				} 
				break;
			case NF_UDP_SEND:
				{
					packetProcessed = ctrl_processUdpSendPacket(pAddr, pPacket, &result);
				} 
				break;
			}
		}

		sl_unlock(&pAddr->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_processUdpPacket[%I64d]: address not found\n", pData->id));
		result = sizeof(NF_DATA) + pData->bufferSize - 1;
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (!packetProcessed)
	{
		KdPrint((DPREFIX"ctrl_processUdpPacket[%I64d]: !packetProcessed\n", pData->id));
		nf_packet_free(pPacket);
	} else
	{
		if (pData->code == NF_UDP_SEND)
		{
			// Issue TDI_SEND_DATAGRAM requests from PASSIVE IRQL

			status = udp_TdiStartInternalSend(pAddr);
			if (status == STATUS_UNSUCCESSFUL)
			{
				nf_packet_free(pPacket);
			}

			return result;
		}
	}

	return result;
}


/**
*	Process the filtered UDP connect request
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_processUdpConnect(PNF_DATA pData)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	unsigned long result = sizeof(NF_DATA) + pData->bufferSize - 1;
	KIRQL irql, irqld;
	PIRP	irp = NULL;
	PDEVICE_OBJECT pLowerDevice;

	KdPrint((DPREFIX"ctrl_processUdpConnect[%I64d]\n", pData->id));

	if (pData->bufferSize < sizeof(NF_UDP_CONN_REQUEST))
		return 0;

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtAddrById, pData->id);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

		sl_lock(&pAddr->lock, &irqld);
		
		if (pAddr->fileObject)
		{
			PNF_UDP_CONN_REQUEST pcr = (PNF_UDP_CONN_REQUEST)pData->buffer;
			
			// Update filtering flag
			pAddr->filteringFlag = pcr->filteringFlag;
			
			// Update remote address
			memcpy(pAddr->remoteAddr, pcr->remoteAddress, sizeof(pcr->remoteAddress));

			if (!IsListEmpty(&pAddr->pendedConnectRequest))
			{
				PLIST_ENTRY pEntry;
				PIO_STACK_LOCATION irpSp;
				PTDI_REQUEST_KERNEL  prk;

				pEntry = RemoveHeadList(&pAddr->pendedConnectRequest);
				
				irp = (PIRP)CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
				InitializeListHead(&irp->Tail.Overlay.ListEntry);

				IoSetCancelRoutine(irp, NULL);

				irpSp = IoGetCurrentIrpStackLocation(irp);
				ASSERT(irpSp);

				prk = (PTDI_REQUEST_KERNEL )&irpSp->Parameters;

				if (prk &&
					prk->RequestConnectionInformation &&
					prk->RequestConnectionInformation->RemoteAddress)
				{
					PTA_ADDRESS pRemoteAddr = NULL;
					int addrLen = 0;

					pRemoteAddr = ((TRANSPORT_ADDRESS *)(prk->RequestConnectionInformation->RemoteAddress))->Address;
					if (pRemoteAddr)
					{
						addrLen = min(pRemoteAddr->AddressLength + sizeof(USHORT), sizeof(pAddr->remoteAddr));
						// Replace remote address for TDI_CONNECT request
						memcpy((char*)pRemoteAddr + sizeof(USHORT), pAddr->remoteAddr, addrLen);
					}
				} 

				if (pAddr->filteringFlag & NF_BLOCK)
				{
					irp->IoStatus.Status = STATUS_REMOTE_NOT_LISTENING;
					irp->IoStatus.Information = 0;
					ctrl_deferredCompleteIrp(irp, FALSE);
					irp = NULL;
				} else
				if ((pAddr->filteringFlag & (NF_OFFLINE | NF_FILTER)) == (NF_OFFLINE | NF_FILTER))
				{
					irp->IoStatus.Status = STATUS_SUCCESS;
					irp->IoStatus.Information = 0;

					ctrl_deferredCompleteIrp(irp, FALSE);
					irp = NULL;
				} else
				{
					pLowerDevice = pAddr->lowerDevice;
					if (!pLowerDevice)
					{
						ASSERT(0);
						irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
						irp->IoStatus.Information = 0;
						
						// Complete IRP from DPC
						ctrl_deferredCompleteIrp(irp, FALSE);
						irp = NULL;
					}
				}
			}
		}

		sl_unlock(&pAddr->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_processUdpConnect[%I64d]: connection not found\n", pData->id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (pAddr && irp && pLowerDevice)
	{
		IoSkipCurrentIrpStackLocation(irp);
	        
		IoCallDriver(pLowerDevice, irp);
	}

	return result;
}


/**
*	Change the state of UDP address object according to pData request
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_setUdpConnState(PNF_DATA pData)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;

	KdPrint((DPREFIX"ctrl_setUdpConnState[%I64d]: %d\n", pData->id, pData->code));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtAddrById, pData->id);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

		sl_lock(&pAddr->lock, &irqld);
		
		if (pAddr->fileObject)
		{
			switch (pData->code)
			{
			case NF_UDP_REQ_SUSPEND:
				{
					KdPrint((DPREFIX"ctrl_setUdpConnState[%I64d]: address suspended\n", pData->id));
					pAddr->filteringFlag |= NF_SUSPENDED;
				}
				break;
			case NF_UDP_REQ_RESUME:
				{
					int i, listSize;

					if (pAddr->filteringFlag & NF_SUSPENDED)
					{
						KdPrint((DPREFIX"ctrl_setUdpConnState[%I64d]: address resumed\n", pData->id));
				
						pAddr->filteringFlag &= ~NF_SUSPENDED;

						listSize = tu_getListSize(&pAddr->pendedReceivedPackets);
						if (listSize > 0)
						{
							for (i=0; i<listSize; i++)
							{
								ctrl_queuePush(NF_UDP_RECEIVE, pAddr->id);
							}
						}

						listSize = tu_getListSize(&pAddr->pendedSendRequests);
						for (i=0; i<listSize; i++)
						{
							ctrl_queuePush(NF_UDP_SEND, pAddr->id);
						}
					}
				}
				break;
			}
		}

		sl_unlock(&pAddr->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_setUdpConnState[%I64d]: address not found\n", pData->id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	return sizeof(NF_DATA) - 1;
}

/**
*	Disables indicating packets to user mode for the specified endpoint
*	@return Number of bytes processed
*	@param pData IO request descriptor
**/
unsigned long ctrl_udpDisableUserModeFiltering(PNF_DATA pData)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;

	KdPrint((DPREFIX"ctrl_udpDisableFiltering[%I64d]\n", pData->id));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtAddrById, pData->id);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, id);

		sl_lock(&pAddr->lock, &irqld);
		
		if (pAddr->fileObject)
		{
			pAddr->userModeFilteringDisabled = TRUE;
		}

		sl_unlock(&pAddr->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_udpDisableFiltering[%I64d]: endpoint not found\n", pData->id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	return sizeof(NF_DATA) - 1;
}

NTSTATUS   
ctrl_setTcpConnInformation(PNF_DATA pData)
{   
    IO_STATUS_BLOCK iosb;   
    KEVENT event;   
    NTSTATUS status;   
    PIRP irp;   
    PIO_STACK_LOCATION irpSp;   
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	PFILE_OBJECT fileObject = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
	KIRQL irql, irqld;
	HASH_ID id;
	PTCP_REQUEST_SET_INFORMATION_EX info;
    ULONG infoSize;

	if (!pData)
		return STATUS_INVALID_HANDLE;

	id = pData->id;
	info = (PTCP_REQUEST_SET_INFORMATION_EX)pData->buffer;
	infoSize = pData->bufferSize;

	KdPrint((DPREFIX"ctrl_setTcpConnInformation[%I64d]\n", id));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnById, id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		sl_lock(&pConn->lock, &irqld);
		
		if (pConn->fileObject && pConn->lowerDevice)
		{
			fileObject = (PFILE_OBJECT)pConn->fileObject;
			deviceObject = pConn->lowerDevice;

			// Increment reference counter
			ObReferenceObject(fileObject);

			switch (info->ID.toi_id)
			{
			case TCP_SOCKET_NODELAY:
				pConn->blockSetInformation |= BSIC_NODELAY;
				break;
			case TCP_SOCKET_KEEPALIVE:
				pConn->blockSetInformation |= BSIC_KEEPALIVE;
				break;
			case TCP_SOCKET_OOBINLINE:
				pConn->blockSetInformation |= BSIC_OOBINLINE;
				break;
			case TCP_SOCKET_BSDURGENT:
				pConn->blockSetInformation |= BSIC_BSDURGENT;
				break;
			case TCP_SOCKET_ATMARK:
				pConn->blockSetInformation |= BSIC_ATMARK;
				break;
			case TCP_SOCKET_WINDOW:
				pConn->blockSetInformation |= BSIC_WINDOW;
				break;
			}
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_setTcpConnInformation[%I64d]: connection not found\n", id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (!fileObject)
		return STATUS_INVALID_HANDLE;

	//   
    // Initialize the event that we use to wait.   
    //   
    KeInitializeEvent(&event, NotificationEvent, FALSE);   
   
    //   
    // Create and initialize the IRP for this operation.   
    //   
    irp = IoBuildDeviceIoControlRequest(IOCTL_TCP_SET_INFORMATION_EX,   
                                        deviceObject,   
                                        info,   
                                        infoSize,   
                                        NULL,   // output buffer   
                                        0,      // output buffer length   
                                        FALSE,  // internal device control?   
                                        &event,   
                                        &iosb);   
    if (irp == NULL)
	{
		// Derement reference counter
		ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;   
	}

    iosb.Status = STATUS_UNSUCCESSFUL;   
    iosb.Information = 0;   
   
    irpSp = IoGetNextIrpStackLocation(irp);   
    irpSp->FileObject = fileObject;   
   
    status = IoCallDriver(deviceObject, irp);   
    if (status == STATUS_PENDING) 
	{   
        KeWaitForSingleObject(&event, Executive, KernelMode,   
                              FALSE, NULL);   
        status = iosb.Status;   
    }   
   
	// Derement reference counter
	ObDereferenceObject(fileObject);

	return status;   
}   

NTSTATUS ctrl_queryInformationComplete(IN PDEVICE_OBJECT pDeviceObject, 
									 IN PIRP irp, 
									 IN PVOID context)
{
	if (irp->MdlAddress) 
	{
		IoFreeMdl(irp->MdlAddress); 
		irp->MdlAddress = NULL;
	}
	// No need to free Irp, because it was allocated with TdiBuildInternalDeviceControlIrp.
	// IO manager frees it automatically.

	return STATUS_SUCCESS;
}


NTSTATUS   
ctrl_getAddrInformation(PNF_DATA pData)   
{   
    IO_STATUS_BLOCK iosb;   
    KEVENT event;   
    NTSTATUS status = STATUS_UNSUCCESSFUL;   
    PIRP irp;   
    PIO_STACK_LOCATION irpSp;   
	PCONNOBJ pConn = NULL;
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	PFILE_OBJECT fileObject = NULL;
    PDEVICE_OBJECT deviceObject = NULL;
	KIRQL irql, irqld;
	HASH_ID id;
	PMDL mdl;

	if (!pData)
		return STATUS_INVALID_HANDLE;

	if (pData->bufferSize < NF_MAX_ADDRESS_LENGTH)
		return STATUS_NO_MEMORY;

	id = pData->id;

	KdPrint((DPREFIX"ctrl_getAddrInformation[%I64d]\n", id));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnById, id);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);

		sl_lock(&pConn->lock, &irqld);
		
		if (pConn->pAddr && pConn->fileObject && pConn->lowerDevice)
		{
			pAddr = pConn->pAddr;
			fileObject = (PFILE_OBJECT)pConn->fileObject;
			deviceObject = pConn->lowerDevice;
	
			// Increment reference counter
			ObReferenceObject(fileObject);
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"ctrl_getAddrInformation[%I64d]: connection not found\n", id));
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (!fileObject)
		return STATUS_INVALID_HANDLE;

    KeInitializeEvent(&event, NotificationEvent, FALSE);   
   
	mdl = IoAllocateMdl(pAddr->queryAddressInfo,
		TDI_ADDRESS_INFO_MAX, FALSE, FALSE, NULL);

	if (mdl)
	{
		MmBuildMdlForNonPagedPool(mdl);

		// Make a request
		irp = TdiBuildInternalDeviceControlIrp(
			TDI_QUERY_INFORMATION, 
			deviceObject,
			fileObject, 
			&event, 
			&iosb);
		
		if (irp)
		{
			TdiBuildQueryInformation(
				irp, 
				deviceObject, 
				fileObject,
				ctrl_queryInformationComplete, 
				NULL,
				TDI_QUERY_ADDRESS_INFO, 
				mdl);

			status = IoCallDriver(deviceObject, irp);
			
			if (status == STATUS_PENDING) 
			{   
				KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);   
				status = iosb.Status;   
			}

			if (NT_SUCCESS(status))
			{
				PTDI_ADDRESS_INFO pTai = (PTDI_ADDRESS_INFO)pAddr->queryAddressInfo;
			
				status = STATUS_NO_MEMORY;

				if (pTai)
				{
					PTA_ADDRESS pTAAddr = pTai->Address.Address;
				
					if (pTAAddr)
					{
						if ((pTAAddr->AddressLength + sizeof(USHORT)) <= pData->bufferSize) 
						{
                			memcpy(pData->buffer, (char*)pTAAddr + sizeof(USHORT), pTAAddr->AddressLength + sizeof(USHORT));
                			memcpy(pConn->localAddr, (char*)pTAAddr + sizeof(USHORT), pTAAddr->AddressLength + sizeof(USHORT));
							status = STATUS_SUCCESS;
						}
					}
				}
			}
		} else
		{
			IoFreeMdl(mdl);
		}
	}

	ObDereferenceObject(fileObject);

	return status;   
}

/**
*	Process IO request from proxy
*	@return Number of bytes processed
*	@param buffer IO buffer
*	@param bufferSize IO buffer size
**/
unsigned long ctrl_processRequest(char * buffer, unsigned long bufferSize)
{
	PNF_DATA pData = (PNF_DATA)buffer;

	KdPrint((DPREFIX"ctrl_processRequest"));
		
	if (bufferSize < (sizeof(NF_DATA) + pData->bufferSize - 1))
	{
		return 0;
	}

	switch (pData->code)
	{
	case NF_TCP_RECEIVE:
	case NF_TCP_SEND:
		return ctrl_processTcpPacket(pData);
	
	case NF_TCP_CONNECT_REQUEST:
		return ctrl_processTcpConnect(pData);

	case NF_TCP_REQ_SUSPEND:
	case NF_TCP_REQ_RESUME:
		return ctrl_setTcpConnState(pData);

	case NF_UDP_RECEIVE:
	case NF_UDP_SEND:
		return ctrl_processUdpPacket(pData);
	
	case NF_UDP_REQ_SUSPEND:
	case NF_UDP_REQ_RESUME:
		return ctrl_setUdpConnState(pData);

	case NF_UDP_CONNECT_REQUEST:
		return ctrl_processUdpConnect(pData);

	case NF_REQ_ADD_HEAD_RULE:
		return ctrl_addRule(pData, TRUE);

	case NF_REQ_ADD_TAIL_RULE:
		return ctrl_addRule(pData, FALSE);

	case NF_REQ_DELETE_RULES:
		rule_remove_all();
		return sizeof(NF_DATA) - 1;

	case NF_TCP_DISABLE_USER_MODE_FILTERING:
		return ctrl_tcpDisableUserModeFiltering(pData);

	case NF_UDP_DISABLE_USER_MODE_FILTERING:
		return ctrl_udpDisableUserModeFiltering(pData);

	case NF_REQ_SET_TCP_OPT:
		return (NT_SUCCESS(ctrl_setTcpConnInformation(pData)))? sizeof(NF_DATA) - 1 : 0;

	case NF_REQ_IS_PROXY:
		{
			BOOLEAN isProxy = FALSE;
			KIRQL irql;
			
			sl_lock(&g_vars.slConnections, &irql);
			isProxy = tcp_isProxy((ULONG)pData->id);
			sl_unlock(&g_vars.slConnections, irql);

			return (isProxy)? sizeof(NF_DATA) - 1 : 0;
		}

	case NF_TCP_REINJECT:
		{
			tcp_procReinject();
			break;
		}
	}

	return 0;
}

/**
* Complete the pended IO IRP from attached process
* @return NTSTATUS 
**/
NTSTATUS ctrl_completePendedIoRequest()
{
    ULONG		bufferLength;
	ULONG		readBytes;
    PLIST_ENTRY pIrpEntry;
    PIRP		irp;
	PIO_STACK_LOCATION	irpSp;
	PNF_READ_RESULT		pResult;
	KIRQL		irql, irqld;

	KdPrint((DPREFIX"ctrl_completePendedIoRequest\n"));

	sl_lock(&g_vars.slConnections, &irqld);
	sl_lock(&g_vars.slIoQueue, &irql);
	
	while (!IsListEmpty(&g_vars.pendedIoRequests))
	{
		pIrpEntry = RemoveHeadList(&g_vars.pendedIoRequests);

		if (pIrpEntry && (pIrpEntry != &g_vars.pendedIoRequests))
		{
			KdPrint((DPREFIX"ctrl_completePendedIoRequest: Processing queued IO IRP\n" ));

			irp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

			InitializeListHead(&irp->Tail.Overlay.ListEntry);

	        if (!IoSetCancelRoutine(irp, NULL))
				continue;

			irpSp = IoGetCurrentIrpStackLocation(irp);
			ASSERT(irpSp);
		
			pResult = (PNF_READ_RESULT)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
			if (!pResult ||
				irpSp->Parameters.Read.Length < sizeof(NF_READ_RESULT))
			{
				sl_unlock(&g_vars.slIoQueue, irql);
				sl_unlock(&g_vars.slConnections, irqld);

				irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				IoCompleteRequest(irp, IO_NO_INCREMENT);

				sl_lock(&g_vars.slConnections, &irqld);
				sl_lock(&g_vars.slIoQueue, &irql);
				continue;
			}

			pResult->length = ctrl_fillIoBuffer(g_inBuf.kernelVa, (ULONG)g_inBuf.bufferLength);

			sl_unlock(&g_vars.slIoQueue, irql);
			sl_unlock(&g_vars.slConnections, irqld);

			irp->IoStatus.Information = sizeof(NF_READ_RESULT);
			irp->IoStatus.Status = STATUS_SUCCESS;
			IoCompleteRequest(irp, IO_NO_INCREMENT);

			sl_lock(&g_vars.slConnections, &irqld);
			sl_lock(&g_vars.slIoQueue, &irql);
		}		
	}

	sl_unlock(&g_vars.slIoQueue, irql);
	sl_unlock(&g_vars.slConnections, irqld);


	return STATUS_SUCCESS;
}

/**
* Add IO data to queue
* @return NTSTATUS 
* @param code IO code
* @param id Endpoint id
**/
NTSTATUS ctrl_queuePush(int code, HASH_ID id)
{
	PNF_QUEUE_ENTRY	pEntry;
	NTSTATUS	status = STATUS_SUCCESS;
	KIRQL		irql;

	KdPrint((DPREFIX"ctrl_queuePush id=%I64d, code=%d\n", id, code));

	pEntry = (PNF_QUEUE_ENTRY)mp_alloc(sizeof(NF_QUEUE_ENTRY));
	if (!pEntry)
	{
		return STATUS_UNSUCCESSFUL;
	}

#if DBG
	g_vars.nIOReq++;
	KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif

	pEntry->code = code;
	pEntry->id = id;

	sl_lock(&g_vars.slIoQueue, &irql);
	if (g_vars.proxyAttached)
	{
		InsertTailList(&g_vars.ioQueue, &pEntry->entry);
		
		ctrl_defferedCompletePendedIoRequest();
	} else
	{
		KdPrint((DPREFIX"ctrl_queuePush: Proxy is unavailable\n"));
		mp_free(pEntry, MAX_QE_POOL_SIZE);
		status = STATUS_UNSUCCESSFUL;
#if DBG
		g_vars.nIOReq--;
		KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif
	}
	sl_unlock(&g_vars.slIoQueue, irql);

	return status;
}

/**
* Add IO data with associated buffer to queue
* @return NTSTATUS 
* @param code IO code
* @param id Endpoint id
* @param buf Associated buffer
* @param len Associated buffer size
**/
NTSTATUS ctrl_queuePushEx(int code, HASH_ID id, char * buf, int len)
{
	PNF_QUEUE_ENTRY	pEntry;
	NTSTATUS	status = STATUS_SUCCESS;
	KIRQL		irql;

	KdPrint((DPREFIX"ctrl_queuePushEx id=%I64d, code=%d, bufLen=%d\n", id, code, len));

	pEntry = (PNF_QUEUE_ENTRY)mp_alloc(sizeof(NF_QUEUE_ENTRY) - 1 + len);
	if (!pEntry)
	{
		return STATUS_UNSUCCESSFUL;
	}

#if DBG
	g_vars.nIOReq++;
	KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif

	pEntry->code = code;
	pEntry->id = id;
	pEntry->bufferSize = len;
	if (len > 0)
	{
		memcpy(pEntry->buffer, buf, len);
	}

	sl_lock(&g_vars.slIoQueue, &irql);
	if (g_vars.proxyAttached)
	{
		InsertTailList(&g_vars.ioQueue, &pEntry->entry);
		
		ctrl_defferedCompletePendedIoRequest();
	} else
	{
		KdPrint((DPREFIX"ctrl_queuePush: Proxy is unavailable\n"));
		mp_free(pEntry, MAX_QE_POOL_SIZE);
		status = STATUS_UNSUCCESSFUL;
#if DBG
		g_vars.nIOReq--;
		KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif
	}
	sl_unlock(&g_vars.slIoQueue, irql);

	return status;
}

/**
* IRP_MJ_CREATE handler
**/
NTSTATUS ctrl_create(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	KIRQL		irql;
	NTSTATUS 	status;
	HANDLE		pid = PsGetCurrentProcessId();

	KdPrint((DPREFIX"ctrl_create\n"));

	sl_lock(&g_vars.slIoQueue, &irql);
	if (g_vars.proxyAttached)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
	} else
	{
		g_vars.proxyPid = pid;
		g_vars.proxyAttached = TRUE;
		status = STATUS_SUCCESS;
	}
	sl_unlock(&g_vars.slIoQueue, irql);

	if (status == STATUS_SUCCESS)
	{
		// Fill the list of existing UDP sockets
		addr_postUdpEndpoints();
	}

	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

/**
* Free IO queue elements
**/
NTSTATUS ctrl_clearIoQueue()
{
	PNF_QUEUE_ENTRY	pEntry;

	KdPrint((DPREFIX"ctrl_clearIoQueue\n"));

	while (!IsListEmpty(&g_vars.ioQueue))
	{
		pEntry = (PNF_QUEUE_ENTRY)g_vars.ioQueue.Flink;

		RemoveEntryList(&pEntry->entry);
		
		mp_free(pEntry, MAX_QE_POOL_SIZE);
#if DBG
		g_vars.nIOReq--;
		KdPrint(("[counter] nIOReq=%d\n", g_vars.nIOReq));
#endif
	}

	return STATUS_SUCCESS;
}

/**
* Disconnect TCP connections having NF_FILTER flag
**/
NTSTATUS ctrl_closeFilteredConnections()
{
	PCONNOBJ pConn = NULL;
	PADDROBJ pAddr = NULL;
	PNF_PACKET pPacket;
	unsigned long result = 0;
	KIRQL irql, irqld;

	KdPrint((DPREFIX"ctrl_closeFilteredConnections\n"));

	sl_lock(&g_vars.slConnections, &irql);

	pConn = (PCONNOBJ)g_vars.lConnections.Flink;
	
	while (pConn != (PCONNOBJ)&g_vars.lConnections)
	{
		if (pConn->filteringFlag & NF_FILTER)
		{
			// Complete pending requests
			tcp_conn_free_queues(pConn, TRUE);

			sl_lock(&pConn->lock, &irqld);
	
			pConn->filteringFlag = NF_ALLOW;
			pConn->sendInProgress = FALSE;

			pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
			if (pPacket)
			{
				pPacket->dataSize = 0;
				pPacket->fileObject = (PFILE_OBJECT)pConn->fileObject;

				InsertTailList(
						&pConn->sendPackets,
						&pPacket->entry
					   );

				// Send data
				tcp_startDeferredProc(pConn, DQC_TCP_SEND);
			}

			sl_unlock(&pConn->lock, irqld);
		}
		

		pConn = (PCONNOBJ)pConn->entry.Flink;
	}

	pAddr = (PADDROBJ)g_vars.lAddresses.Flink;
	
	while (pAddr != (PADDROBJ)&g_vars.lAddresses)
	{
		// Complete pending requests
		addr_free_queues(pAddr);

		pAddr->filteringFlag = NF_ALLOW;

		pAddr = (PADDROBJ)pAddr->entry.Flink;
	}

	sl_unlock(&g_vars.slConnections, irql);

	return STATUS_SUCCESS;
}

void ctrl_cancelPendingReads()
{
    PIRP                irp;
    PLIST_ENTRY         pIrpEntry;
	KIRQL				irql;

	sl_lock(&g_vars.slIoQueue, &irql);
	
	while (!IsListEmpty(&g_vars.pendedIoRequests))
    {
		pIrpEntry = g_vars.pendedIoRequests.Flink;

		RemoveEntryList(pIrpEntry);

		irp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(irp, NULL))
        {
			sl_unlock(&g_vars.slIoQueue, irql);

            //
            //  Complete the IRP.
            //
            irp->IoStatus.Status = STATUS_CANCELLED;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);

			sl_lock(&g_vars.slIoQueue, &irql);
        }
    }

	sl_unlock(&g_vars.slIoQueue, irql);
}


/**
* IRP_MJ_CLOSE handler
**/
NTSTATUS ctrl_close(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	KIRQL		irql, irqld;

	KdPrint((DPREFIX"ctrl_close\n"));

	rule_remove_all();

	sl_lock(&g_vars.slConnections, &irqld);
	sl_lock(&g_vars.slIoQueue, &irql);
	
	g_vars.proxyPid = 0;
	g_vars.proxyAttached = FALSE;

	ctrl_clearIoQueue();

	sl_unlock(&g_vars.slIoQueue, irql);
	sl_unlock(&g_vars.slConnections, irqld);

	ctrl_cancelPendingReads();

	ctrl_closeFilteredConnections();

	ctrl_freeSharedMemory(&g_inBuf);
	ctrl_freeSharedMemory(&g_outBuf);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

void ctrl_cancelIoRequest(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	KIRQL				irql;

    KdPrint((DPREFIX"ctrl_cancelIoRequest\n"));

	//  Release the global cancel spin lock.  
	//  Do this while not holding any other spin locks so that we exit at the right IRQL.
	IoReleaseCancelSpinLock(irp->CancelIrql);

	//  Dequeue and complete the IRP.  
	sl_lock(&g_vars.slIoQueue, &irql);
	RemoveEntryList(&irp->Tail.Overlay.ListEntry);
	InitializeListHead(&irp->Tail.Overlay.ListEntry);
	sl_unlock(&g_vars.slIoQueue, irql);

	//  Complete the IRP.  This is a call outside the driver, so all spin locks must be released by this point.
	irp->IoStatus.Status = STATUS_CANCELLED;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
}

/**
* IRP_MJ_READ handler
**/
NTSTATUS ctrl_read(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	ULONG readBytes;
	KIRQL irql, irqld;
	NTSTATUS status;
	PNF_READ_RESULT		pResult;

    pResult = (PNF_READ_RESULT) MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	if (!pResult || irpSp->Parameters.Read.Length < sizeof(NF_READ_RESULT))
	{
		irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	sl_lock(&g_vars.slConnections, &irqld);
	sl_lock(&g_vars.slIoQueue, &irql);

	pResult->length = ctrl_fillIoBuffer(g_inBuf.kernelVa, (ULONG)g_inBuf.bufferLength);

    if (irp->Cancel &&                              
        IoSetCancelRoutine(irp, NULL)) 
    {            
        //
        // IRP has been canceled but the I/O manager did not manage to call our cancel routine. This
        // code is safe referencing the Irp->Cancel field without locks because of the memory barriers
        // in the interlocked exchange sequences used by IoSetCancelRoutine.
        //
    
        status = STATUS_CANCELLED;                   
        // IRP should be completed after releasing the lock
    } else
	{
		if (pResult->length || !IsListEmpty(&g_vars.pendedIoRequests))
		{
			status = STATUS_SUCCESS;
		} else
		{
			IoMarkIrpPending(irp);
			
			irp->IoStatus.Status = STATUS_PENDING;

			InsertTailList(&g_vars.pendedIoRequests, &irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, ctrl_cancelIoRequest);

			status = STATUS_PENDING;
		}
	}

	sl_unlock(&g_vars.slIoQueue, irql);
	sl_unlock(&g_vars.slConnections, irqld);

	if (status != STATUS_PENDING)
	{
		irp->IoStatus.Information = sizeof(NF_READ_RESULT);
		irp->IoStatus.Status = status;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}

	return status;
}

/**
* IRP_MJ_WRITE handler
**/
NTSTATUS ctrl_write(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PNF_READ_RESULT pRes;
    ULONG bufferLength = irpSp->Parameters.Write.Length;

    pRes = MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	if (!pRes || bufferLength < sizeof(NF_READ_RESULT))
	{
		irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		KdPrint((DPREFIX"ctrl_write invalid irp\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	irp->IoStatus.Information = ctrl_processRequest(g_outBuf.kernelVa, (ULONG)pRes->length);

	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

/**
* Returns TRUE if a proxy process is attached
**/
BOOLEAN ctrl_attached()
{
	return g_vars.proxyAttached;
}

/**
* Complete IRP asynchronously
**/
void ctrl_deferredCompleteIrp(PIRP irp, BOOLEAN longDelay)
{
	KIRQL	irql;
	LARGE_INTEGER li = { 0 };

	sl_lock(&g_vars.slPendedIrps, &irql);
	InsertTailList(&g_vars.pendedIrpsToComplete, &irp->Tail.Overlay.ListEntry);			
	sl_unlock(&g_vars.slPendedIrps, irql);

	KeSetTimer(&g_vars.completeIoDpcTimer, li, &g_vars.completeIoDpc);
}

void ctrl_completeIoDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
{
	LIST_ENTRY	pendedIrpsToComplete;
	PLIST_ENTRY	pListEntry;
	PIRP	irp;
	KIRQL	irql;

	KdPrint((DPREFIX"ctrl_completeIoDpc\n"));

	InitializeListHead(&pendedIrpsToComplete);

	sl_lock(&g_vars.slPendedIrps, &irql);

	while (!IsListEmpty(&g_vars.pendedIrpsToComplete))
	{
		pListEntry = RemoveHeadList(&g_vars.pendedIrpsToComplete); 
		InsertTailList(&pendedIrpsToComplete, pListEntry);
	}

	sl_unlock(&g_vars.slPendedIrps, irql);

	while (!IsListEmpty(&pendedIrpsToComplete))
	{
		pListEntry = RemoveHeadList(&pendedIrpsToComplete); 

		irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}

	KdPrint((DPREFIX"ctrl_completeIoDpc finished\n"));
}

void ctrl_defferedCompletePendedIoRequest()
{
	KeInsertQueueDpc(&g_vars.queueDpc, NULL, NULL);
}

void ctrl_completePendedIoRequestDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
{
	KdPrint((DPREFIX"ctrl_completePendedIoRequestDpc\n"));

	ctrl_completePendedIoRequest();
}

NTSTATUS ctrl_createSharedMemory(PSHARED_MEMORY pSharedMemory, UINT64 len)
{
    PMDL  mdl;
    PVOID userVa = NULL;
    PVOID kernelVa = NULL;
    PHYSICAL_ADDRESS lowAddress;
    PHYSICAL_ADDRESS highAddress;

	memset(pSharedMemory, 0, sizeof(SHARED_MEMORY));

	lowAddress.QuadPart = 0;
	highAddress.QuadPart = 0xFFFFFFFFFFFFFFFF;
 
    mdl = MmAllocatePagesForMdl(lowAddress, highAddress, lowAddress, (SIZE_T)len);
    if(!mdl) 
	{
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    __try 
	{
		kernelVa = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
        if (!kernelVa)
		{
			MmFreePagesFromMdl(mdl);       
			IoFreeMdl(mdl);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		//
		// The preferred way to map the buffer into user space
		//
		userVa = MmMapLockedPagesSpecifyCache(mdl,          // MDL
										  UserMode,     // Mode
										  MmCached,     // Caching
										  NULL,         // Address
										  FALSE,        // Bugcheck?
										  HighPagePriority); // Priority
		if (!userVa)
		{
			MmUnmapLockedPages(kernelVa, mdl);
			MmFreePagesFromMdl(mdl);       
			IoFreeMdl(mdl);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
	}

    //
    // If we get NULL back, the request didn't work.
    // I'm thinkin' that's better than a bug check anyday.
    //
    if (!userVa || !kernelVa)  
	{
		if (userVa)
		{
			MmUnmapLockedPages(userVa, mdl);
		}
		if (kernelVa)
		{
			MmUnmapLockedPages(kernelVa, mdl);
		}
		MmFreePagesFromMdl(mdl);       
		IoFreeMdl(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Return the allocated pointers
    //
	pSharedMemory->mdl = mdl;
	pSharedMemory->userVa = userVa;
	pSharedMemory->kernelVa = kernelVa;
	pSharedMemory->bufferLength = MmGetMdlByteCount(mdl);

    return STATUS_SUCCESS;
}

void ctrl_freeSharedMemory(PSHARED_MEMORY pSharedMemory)
{
	if (pSharedMemory->mdl)
	{
		__try 
		{
			if (pSharedMemory->userVa)
			{
				MmUnmapLockedPages(pSharedMemory->userVa, pSharedMemory->mdl);
			}
			if (pSharedMemory->kernelVa)
			{
				MmUnmapLockedPages(pSharedMemory->kernelVa, pSharedMemory->mdl);
			}
		}
        __except (EXCEPTION_EXECUTE_HANDLER)
		{
		}

		MmFreePagesFromMdl(pSharedMemory->mdl);       
		IoFreeMdl(pSharedMemory->mdl);

		memset(pSharedMemory, 0, sizeof(SHARED_MEMORY));
	}
}

NTSTATUS ctrl_open(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

	if (ioBuffer && (outputBufferLength >= sizeof(NF_BUFFERS)))
	{
		NTSTATUS 	status;

		do	
		{
			if (!g_inBuf.mdl)
			{
				status = ctrl_createSharedMemory(&g_inBuf, NF_UDP_PACKET_BUF_SIZE * 50);
				if (!NT_SUCCESS(status))
				{
					break;
				}
			}

			if (!g_outBuf.mdl)
			{
				status = ctrl_createSharedMemory(&g_outBuf, NF_UDP_PACKET_BUF_SIZE * 2);
				if (!NT_SUCCESS(status))
				{
					break;
				}
			}

			status = STATUS_SUCCESS;
		} while (FALSE);

		if (!NT_SUCCESS(status))
		{
			ctrl_freeSharedMemory(&g_inBuf);
			ctrl_freeSharedMemory(&g_outBuf);
		} else
		{	
			PNF_BUFFERS pBuffers = (PNF_BUFFERS)ioBuffer;

			pBuffers->inBuf = (UINT64)g_inBuf.userVa;
			pBuffers->inBufLen = g_inBuf.bufferLength;
			pBuffers->outBuf = (UINT64)g_outBuf.userVa;
			pBuffers->outBufLen = g_outBuf.bufferLength;

			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(NF_BUFFERS);
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_SUCCESS;
		}
	}

	irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_UNSUCCESSFUL;
}
