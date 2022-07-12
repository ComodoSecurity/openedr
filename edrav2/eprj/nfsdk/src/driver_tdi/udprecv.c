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
#include "udprecv.h"
#include "gvars.h"
#include "addr.h"
#include "packet.h"
#include "ctrlio.h"
#include "rules.h"
#include "tdiutil.h"
#include "mempools.h"

#ifdef _WPPTRACE
#include "udprecv.tmh"
#endif

void udp_cancelTdiReceiveDatagram(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"udp_cancelTdiReceiveDatagram\n"));

    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	//  Dequeue and complete the IRP.  
	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		sl_lock(&pAddr->lock, &irqld);
		if (!IsListEmpty(&irp->Tail.Overlay.ListEntry))
		{
			RemoveEntryList(&irp->Tail.Overlay.ListEntry);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);
			mustComplete = TRUE;
		}
		sl_unlock(&pAddr->lock, irqld);
	}

	sl_unlock(&g_vars.slConnections, irql);

	//  Release the global cancel spin lock.  
	//  Do this while not holding any other spin locks so that we exit at the right IRQL.
	IoReleaseCancelSpinLock(irp->CancelIrql);

	if (mustComplete)
	{
		//  Complete the IRP.  This is a call outside the driver, so all spin locks must be released by this point.
		irp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
	}
}

NTSTATUS udp_TdiReceiveDatagram(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
	KIRQL				irql, irqld;
	NF_FILTERING_FLAG	flag;
    char                NullAddr[TA_ADDRESS_MAX];
	PTA_ADDRESS         pRemoteAddr = NULL;
    
	KdPrint((DPREFIX"udp_TdiReceiveDatagram\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	memset(NullAddr, 0, sizeof(NullAddr));
    pRemoteAddr = (PTA_ADDRESS)NullAddr;
	pRemoteAddr->AddressLength = TDI_ADDRESS_LENGTH_IP;
    pRemoteAddr->AddressType = TDI_ADDRESS_TYPE_IP;

	// INDICATE
	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		flag = rule_findByAddr(pAddr, NF_D_IN, (sockaddr_gen*)&pRemoteAddr->AddressType);

		if (flag & NF_BLOCK)
		{
			sl_unlock(&g_vars.slConnections, irql);
		
			irp->IoStatus.Status = STATUS_INVALID_ADDRESS;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_INVALID_ADDRESS;
		} else
		if ((flag & NF_FILTER) &&
			!pAddr->userModeFilteringDisabled)
		{		
			sl_lock(&pAddr->lock, &irqld);
			InsertTailList(&pAddr->pendedReceiveRequests, &irp->Tail.Overlay.ListEntry);
			sl_unlock(&pAddr->lock, irqld);

			IoMarkIrpPending(irp);

			IoSetCancelRoutine(irp, udp_cancelTdiReceiveDatagram);

			irp->IoStatus.Status = STATUS_PENDING;

			// Indicate received data
			udp_startDeferredProc(pAddr, DQC_UDP_INDICATE_RECEIVE);

			status = STATUS_PENDING;
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (status == STATUS_PENDING)
		return status;

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}


NTSTATUS udp_TdiInternalReceiveDatagramComplete(PDEVICE_OBJECT pDeviceObject, PIRP irp, void * context)
{
	PNF_PACKET			pPacket = (PNF_PACKET) context;
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = irp->IoStatus.Status;
    ULONG               nByteCount = (ULONG)irp->IoStatus.Information;
	KIRQL				irql, irqld;

	KdPrint((DPREFIX"udp_TdiInternalReceiveDatagramComplete status=0x%8.8x, bytes=%d\n", status, nByteCount));

	if (!ctrl_attached())
	{
		if (pPacket)
		{
			nf_packet_free(pPacket);
		}
		IoFreeIrp(irp);    // It's Ours. Free it up
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	if (!pPacket || !NT_SUCCESS(status))
    {
		if (pPacket)
		{
	        nf_packet_free(pPacket);
		}
		IoFreeIrp(irp);    // It's Ours. Free it up
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

	pPacket->ioStatus = status;
	pPacket->dataSize = nByteCount;

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)pPacket->fileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		sl_lock(&pAddr->lock, &irqld);
		
		InsertTailList(
			&pAddr->pendedReceivedPackets,
			&pPacket->entry
		   );

		ctrl_queuePush(NF_UDP_RECEIVE, pAddr->id);

		sl_unlock(&pAddr->lock, irqld);
	} else
	{
		if (pPacket)
		{
	        nf_packet_free(pPacket);
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

    IoFreeIrp(irp);    // It's Ours. Free it up

	return STATUS_MORE_PROCESSING_REQUIRED;
}


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
   PIRP *IoRequestPacket         // TdiReceiveDatagram IRP if MORE_PROCESSING_REQUIRED.
   )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_DATA_NOT_ACCEPTED;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	PTA_ADDRESS pRemoteAddr = NULL;
	KIRQL		irql, irqld;
	PTDI_IND_RECEIVE_DATAGRAM    ev_receive_dg = NULL;           // Receive event handler
    PVOID               ev_receive_dg_context = NULL;    // Receive context

	KdPrint((DPREFIX"udp_TdiReceiveDGEventHandler ReceiveDatagramFlags=%x, OptionsLength=%d\n",
		ReceiveDatagramFlags, OptionsLength));
	
	sl_lock(&g_vars.slConnections, &irql);

	if (!pAddr)
	{
		KdPrint((DPREFIX"udp_TdiReceiveDGEventHandler: pAddr == NULL\n"));
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_DATA_NOT_ACCEPTED;
	}

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || !pAddr->ev_receive_dg || !pAddr->ev_receive_dg_context)
	{
		KdPrint((DPREFIX"udp_TdiReceiveDGEventHandler: !pAddr->fileObject\n"));
		sl_unlock(&pAddr->lock, irqld);
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_DATA_NOT_ACCEPTED;
	}

	ev_receive_dg = pAddr->ev_receive_dg;
	ev_receive_dg_context = pAddr->ev_receive_dg_context;

	sl_unlock(&pAddr->lock, irqld);
	sl_unlock(&g_vars.slConnections, irql);

	if (ctrl_attached())
	{
		sl_lock(&g_vars.slConnections, &irql);
	
		if (SourceAddress && SourceAddressLength)
		{
			pRemoteAddr = ((PTRANSPORT_ADDRESS)SourceAddress)->Address;

			if (pRemoteAddr->AddressType != TDI_ADDRESS_TYPE_IP &&
				pRemoteAddr->AddressType != TDI_ADDRESS_TYPE_IP6)
			{
				KdPrint((DPREFIX"udp_TdiSendDatagram: Unsupported address type\n"));
			} else
			{
				flag = rule_findByAddr(pAddr, NF_D_IN, (sockaddr_gen*)&pRemoteAddr->AddressType);
			}
		}

		if (flag & NF_BLOCK)
		{
			sl_unlock(&g_vars.slConnections, irql);
			KdPrint((DPREFIX"udp_TdiReceiveDGEventHandler: BLOCKED\n"));
			return STATUS_DATA_NOT_ACCEPTED;
		} else
		if ((flag & NF_FILTER) &&
			!pAddr->userModeFilteringDisabled)
		{
			ULONG		packetSize;
			PNF_PACKET	pPacket = NULL;
			int			pendedPackets;
			
			sl_lock(&pAddr->lock, &irqld);
			pendedPackets = tu_getListSize(&pAddr->pendedReceivedPackets);
			sl_unlock(&pAddr->lock, irqld);

			if (pendedPackets > 200)
			{
				sl_unlock(&g_vars.slConnections, irql);
				KdPrint((DPREFIX"udp_TdiReceiveDGEventHandler: BLOCKED, too large queue\n"));
				return STATUS_DATA_NOT_ACCEPTED;
			}

			packetSize = BytesAvailable;

			*BytesTaken = 0;

			pPacket = nf_packet_alloc(packetSize);
			if (!pPacket)
			{
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_DATA_NOT_ACCEPTED;
			}

			pPacket->fileObject = (PFILE_OBJECT)pAddr->fileObject;

			if (BytesIndicated == BytesAvailable)
			{
				// Copy full Tsdu to the packet buffer
				RtlCopyMemory(pPacket->buffer, Tsdu, BytesAvailable);

				pPacket->dataSize = BytesAvailable;
				
				// Copy remote address to the packet buffer
				if (pRemoteAddr)
				{
					if (SourceAddressLength <= sizeof(pPacket->remoteAddr))
					{
						RtlCopyMemory(pPacket->remoteAddr, SourceAddress, SourceAddressLength);
					}
				}

				pPacket->connInfo.RemoteAddressLength = SourceAddressLength;
				pPacket->connInfo.RemoteAddress = pPacket->remoteAddr;

				// Copy also Options and ReceiveDatagramFlags

				if (OptionsLength > 0)
				{
					pPacket->connInfo.OptionsLength = OptionsLength;
					pPacket->connInfo.Options = malloc_np(OptionsLength);
					if (!pPacket->connInfo.Options)
					{
						nf_packet_free(pPacket);
						sl_unlock(&g_vars.slConnections, irql);
						return STATUS_DATA_NOT_ACCEPTED;
					}
					RtlCopyMemory(pPacket->connInfo.Options, Options, OptionsLength);
				}

				pPacket->flags = ReceiveDatagramFlags;

				*BytesTaken = BytesAvailable;

				sl_lock(&pAddr->lock, &irqld);

				InsertTailList(
					&pAddr->pendedReceivedPackets,
					&pPacket->entry
				   );

				ctrl_queuePush(NF_UDP_RECEIVE, pAddr->id);

				sl_unlock(&pAddr->lock, irqld);
			
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_SUCCESS;
			} else
			{
				PIRP	irp = NULL;
				PDEVICE_OBJECT pLowerDevice = pAddr->lowerDevice;
				
				if (!pLowerDevice)
				{
					ASSERT(0);
					nf_packet_free(pPacket);
					sl_unlock(&g_vars.slConnections, irql);
		
					return STATUS_DATA_NOT_ACCEPTED;
				}

				// Receive all Tsdu via TDI_RECEIVE_DATAGRAM request

				irp = IoAllocateIrp(pLowerDevice->StackSize+2, FALSE);
				if (!irp)
				{
					nf_packet_free(pPacket);
					ASSERT(0);
					sl_unlock(&g_vars.slConnections, irql);
		
					return STATUS_DATA_NOT_ACCEPTED;
				}

				if (SourceAddressLength)
				{
					if (SourceAddressLength <= sizeof(pPacket->remoteAddr))
					{
						RtlCopyMemory(pPacket->remoteAddr, SourceAddress, SourceAddressLength);
					}
				
					pPacket->connInfo.RemoteAddressLength = SourceAddressLength;
					pPacket->connInfo.RemoteAddress = pPacket->remoteAddr;
				}

				pPacket->flags = ReceiveDatagramFlags;

				pPacket->dataSize = BytesAvailable;

				TdiBuildReceiveDatagram(
						irp,							// IRP
						pLowerDevice,				// Device
						(PFILE_OBJECT)pAddr->fileObject, // File object
						udp_TdiInternalReceiveDatagramComplete, // CompletionRoutine
						pPacket,						// Context
						pPacket->pMdl,					// MdlAddress
						pPacket->dataSize,				// ReceiveLength
						&pPacket->queryConnInfo,		// ReceiveDatagramInfo
						&pPacket->connInfo,				// ReturnInfo
						TDI_RECEIVE_NORMAL				// InFlags
					);
			
				IoSetNextIrpStackLocation(irp);

				*IoRequestPacket = irp;

				*BytesTaken = 0;
				
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_MORE_PROCESSING_REQUIRED;
			}

			*BytesTaken = 0;
			
			sl_unlock(&g_vars.slConnections, irql);
			return STATUS_DATA_NOT_ACCEPTED;
		}

		sl_unlock(&g_vars.slConnections, irql);
	}
	
	if (ev_receive_dg)
	{
		status = ev_receive_dg(
				ev_receive_dg_context,
				SourceAddressLength,    // length of the originator of the datagram
				SourceAddress,          // describing the originator of the datagram
				OptionsLength,          // options for the receive
				Options,                //
				ReceiveDatagramFlags,   //
				BytesIndicated,         // number of bytes in this indication
				BytesAvailable,         // number of bytes in complete Tsdu
				BytesTaken,             // number of bytes used by indication routine
				Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
				IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
			);
	}

	return status;
}

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
    )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_DATA_NOT_ACCEPTED;
	NF_FILTERING_FLAG flag = NF_ALLOW;
	KIRQL		irql, irqld;
	PTA_ADDRESS pRemoteAddr = NULL;

	KdPrint((DPREFIX"udp_TdiChainedReceiveDGEventHandler\n"));
	
	if (!pAddr)
	{
		KdPrint((DPREFIX"udp_TdiChainedReceiveDGEventHandler: pAddr == NULL\n"));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	if (ctrl_attached())
	{
		sl_lock(&g_vars.slConnections, &irql);

		if (SourceAddress && SourceAddressLength)
		{
			pRemoteAddr = ((PTRANSPORT_ADDRESS)SourceAddress)->Address;

			flag = rule_findByAddr(pAddr, NF_D_IN, (sockaddr_gen*)&pRemoteAddr->AddressType);
		}

		if (flag & NF_BLOCK)
		{
			sl_unlock(&g_vars.slConnections, irql);
			return STATUS_DATA_NOT_ACCEPTED;
		} else
		if ((flag & NF_FILTER) &&
			!pAddr->userModeFilteringDisabled)
		{
			ULONG		packetSize;
			PNF_PACKET	pPacket = NULL;
			int			pendedPackets;
			
			sl_lock(&pAddr->lock, &irqld);
			pendedPackets = tu_getListSize(&pAddr->pendedReceivedPackets);
			sl_unlock(&pAddr->lock, irqld);

			if (pendedPackets > 20)
			{
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_DATA_NOT_ACCEPTED;
			}

			packetSize = ReceiveDatagramLength;

			pPacket = nf_packet_alloc(packetSize);
			if (!pPacket)
			{
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_DATA_NOT_ACCEPTED;
			}

			pPacket->fileObject = (PFILE_OBJECT)pAddr->fileObject;

			// Copy full Tsdu to the packet buffer
			pPacket->dataSize = tu_copyMdlToBuffer(
									Tsdu, 
									ReceiveDatagramLength, 
									StartingOffset, 
									pPacket->buffer,
									pPacket->bufferSize);

			if (pPacket->dataSize == 0)
			{
				ASSERT(0);
				nf_packet_free(pPacket);
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_DATA_NOT_ACCEPTED;
			}

			// Copy remote address to the packet buffer
			if (pRemoteAddr)
			{
				if (SourceAddressLength <= sizeof(pPacket->remoteAddr))
				{
					RtlCopyMemory(pPacket->remoteAddr, SourceAddress, SourceAddressLength);
				}
			}

			pPacket->connInfo.RemoteAddressLength = SourceAddressLength;
			pPacket->connInfo.RemoteAddress = pPacket->remoteAddr;

			sl_lock(&pAddr->lock, &irqld);

			InsertTailList(
					&pAddr->pendedReceivedPackets,
					&pPacket->entry
				   );

			ctrl_queuePush(NF_UDP_RECEIVE, pAddr->id);

			sl_unlock(&pAddr->lock, irqld);
			
			sl_unlock(&g_vars.slConnections, irql);
			return STATUS_SUCCESS;
		}

		sl_unlock(&g_vars.slConnections, irql);
	}

	if (pAddr->ev_chained_receive_dg)
	{
		status = pAddr->ev_chained_receive_dg(
				pAddr->ev_chained_receive_dg_context,
				SourceAddressLength,    // length of the originator of the datagram
				SourceAddress,          // string describing the originator of the datagram
				OptionsLength,          // options for the receive
				Options,                //
				ReceiveDatagramFlags,   //
				ReceiveDatagramLength,
				StartingOffset,
				Tsdu,
				TsduDescriptor
			);
	}

	return status;
}


NTSTATUS udp_indicateReceivedPacketsForIrp(PIRP irp, PLIST_ENTRY packets)
{
	ULONG			bytesCopied = 0;
	ULONG			bytesRemaining;
	PUCHAR			pVA;
	ULONG			nVARemaining;
	PNF_PACKET		pPacket;
	PTDI_REQUEST_KERNEL_RECEIVEDG prk;
	PIO_STACK_LOCATION	irpSp = NULL;
	PMDL			mdl;
    ULONG           bytesToCopy;
	PTA_ADDRESS     pRemoteAddr = NULL;
    char            NullAddr[TA_ADDRESS_MAX];
	ULONG			receiveFlags;

	KdPrint((DPREFIX"udp_indicateReceivedPacketsForIrp\n"));

	if (irp == NULL)
		return STATUS_SUCCESS;

	irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	prk = (PTDI_REQUEST_KERNEL_RECEIVEDG)&irpSp->Parameters;

	if (prk &&
        prk->ReceiveDatagramInformation &&
        prk->ReceiveDatagramInformation->RemoteAddress &&
		prk->ReceiveDatagramInformation->RemoteAddressLength)
    {
	    pRemoteAddr = ((PTRANSPORT_ADDRESS)(prk->ReceiveDatagramInformation->RemoteAddress))->Address;
    } else
	{
		pRemoteAddr = NULL;
	}

	bytesRemaining = prk->ReceiveLength;
	receiveFlags = prk->ReceiveFlags;

	mdl = irp->MdlAddress;

	if (mdl == NULL)
	{
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		return STATUS_SUCCESS;
	}

	nVARemaining = MmGetMdlByteCount(mdl);

	if (bytesRemaining == 0)
	{
		// If prk->ReceiveLength is zero, all memory mapped by mdl must be used
		bytesRemaining = nVARemaining;
	}

	if (nVARemaining > bytesRemaining)
    {
		nVARemaining = bytesRemaining;
    }

	pVA = MmGetSystemAddressForMdlSafe(mdl, HighPagePriority);
	if (pVA == NULL)
	{
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		return STATUS_SUCCESS;
	}

	if (!IsListEmpty(packets))
	{
		pPacket = (PNF_PACKET)packets->Flink;

		if (!NT_SUCCESS(pPacket->ioStatus))
		{
			NTSTATUS status = pPacket->ioStatus;
			
			KdPrint((DPREFIX"udp_indicateReceivedPacketsForIrp: bad status status=%x, dataSize=%d\n", 
				status, pPacket->dataSize));

			RemoveEntryList(&pPacket->entry);
			nf_packet_free(pPacket);

			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = status;
			irp->IoStatus.Information = 0;

			return status;
		}

		while (mdl)
		{
			bytesToCopy = bytesRemaining;

			if (bytesToCopy > nVARemaining)
			{
				bytesToCopy = nVARemaining;
			}

			if (bytesToCopy > pPacket->dataSize)
			{
				bytesToCopy = pPacket->dataSize;
			}

			RtlCopyMemory(pVA, &pPacket->buffer[pPacket->offset], bytesToCopy);

			bytesRemaining -= bytesToCopy;
			
			bytesCopied += bytesToCopy;

			if (bytesToCopy < pPacket->dataSize)
			{
				pPacket->dataSize -= bytesToCopy;
				pPacket->offset += bytesToCopy;
			} else 
			if (bytesToCopy == pPacket->dataSize)
			{
				if (receiveFlags & TDI_RECEIVE_PEEK)
				{
					break;
				}

				RemoveEntryList(&pPacket->entry);
				nf_packet_free(pPacket);
				break;
			}

			mdl = mdl->Next;

			if (mdl)
			{
				nVARemaining = MmGetMdlByteCount(mdl);

				if (nVARemaining > bytesRemaining)
				{
					nVARemaining = bytesRemaining;
				}

				pVA = MmGetSystemAddressForMdlSafe(mdl, HighPagePriority);
			} 
		}
	}

	InitializeListHead(&irp->Tail.Overlay.ListEntry);

	IoSetCancelRoutine(irp, NULL);

	irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = bytesCopied;

	KdPrint((DPREFIX"udp_indicateReceivedPacketsForIrp: Bytes copied %d\n", bytesCopied));

	return STATUS_SUCCESS;
}

NTSTATUS udp_indicateReceivedPacketsViaEvent(PADDROBJ pAddr, KIRQL * pirqld)
{
	PTDI_IND_RECEIVE_DATAGRAM    ev_receive_dg = NULL;           // Receive event handler
    PVOID               ev_receive_dg_context = NULL;    // Receive context
	NTSTATUS		status = STATUS_SUCCESS;
	PIRP			irp = NULL;
	PNF_PACKET		pPacket;
	ULONG			bytesTaken = 0;

	KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent\n"));

	if (!pAddr)
	{
		KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent: !pAddr\n"));
		status = STATUS_INVALID_ADDRESS;
		goto done;
	}

	if (!pAddr->fileObject)
	{
		KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent: !pAddr->fileObject\n"));
		status = STATUS_INVALID_ADDRESS;
		goto done;
	}

	ev_receive_dg = pAddr->ev_receive_dg;
	ev_receive_dg_context = pAddr->ev_receive_dg_context;

	if (!ev_receive_dg)
	{
		KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent: !pAddr->ev_receive_dg\n"));
		status = STATUS_INVALID_ADDRESS;
		goto done;
	}

    pPacket = (PNF_PACKET)pAddr->receivedPackets.Flink;
    if (!pPacket || (pPacket == (PNF_PACKET)&pAddr->receivedPackets))
	{
		KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent: no packet\n"));
		status = STATUS_INVALID_ADDRESS;
		goto done;
	}

	RemoveEntryList(&pPacket->entry);

	KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent: packet status=%x, dataSize=%d\n",
		pPacket->ioStatus, pPacket->dataSize));

	if (!NT_SUCCESS(pPacket->ioStatus) || !pPacket->dataSize)
	{
		nf_packet_free(pPacket);
		status = STATUS_INVALID_ADDRESS;
		goto done;
	}

	sl_unlock(&pAddr->lock, *pirqld);

	status = ev_receive_dg(
				ev_receive_dg_context,
				pPacket->connInfo.RemoteAddressLength,
				pPacket->connInfo.RemoteAddress,
				pPacket->connInfo.OptionsLength,
				pPacket->connInfo.Options,
				pPacket->flags,
		        pPacket->dataSize,
				pPacket->dataSize,
                &bytesTaken,
				&pPacket->buffer[pPacket->offset],  // Tsdu
                &irp                 // TdiReceiveDatagram IRP if STATUS_MORE_PROCESSING_REQUIRED
			);

	sl_lock(&pAddr->lock, pirqld);

	if (!pAddr->fileObject)
	{
		nf_packet_free(pPacket);
		status = STATUS_INVALID_ADDRESS;
		//if ((status == STATUS_MORE_PROCESSING_REQUIRED) && irp)
		if (irp)
		{
			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = STATUS_INVALID_ADDRESS;
			irp->IoStatus.Information = 0;

			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, FALSE);
		}
		goto done;
	}

	KdPrint((DPREFIX"udp_indicateReceivedPacketsViaEvent: ev_receive status=0x%8.8X, dataSize=%d, taken=%d\n", 
		status, pPacket->dataSize, bytesTaken));

	if (status == STATUS_DATA_NOT_ACCEPTED)
	{
		bytesTaken = 0;
	} else
	if ((status == STATUS_SUCCESS) && (bytesTaken == 0))
	{
		status = STATUS_DATA_NOT_ACCEPTED;
	}

	if ((bytesTaken < pPacket->dataSize) && irp)
	{
		pPacket->dataSize -= bytesTaken;
		pPacket->offset += bytesTaken;

		InsertHeadList(&pAddr->receivedPackets, &pPacket->entry);
	} else 
    {
		nf_packet_free(pPacket);
		pPacket = NULL;
	}

	//if (status == STATUS_MORE_PROCESSING_REQUIRED && irp)
	if (irp)
	{
		status = udp_indicateReceivedPacketsForIrp(irp, &pAddr->receivedPackets);

		// Complete request immediately
		sl_unlock(&pAddr->lock, *pirqld);
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		sl_lock(&pAddr->lock, pirqld);
	}

done:

	return status;
}


NTSTATUS udp_indicateReceivedPackets(PADDROBJ pAddr, HASH_ID id)
{
    PLIST_ENTRY pIrpEntry;
    PIRP		irp;
	KIRQL		irqld;
	NTSTATUS	status = STATUS_SUCCESS;

	KdPrint((DPREFIX"udp_indicateReceivedPackets [%I64d]\n", pAddr->id));

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || (pAddr->id != id))
	{
		goto done;
	}

	if (IsListEmpty(&pAddr->receivedPackets))
	{
		KdPrint((DPREFIX"udp_indicateReceivedPackets: No Data\n" ));
		goto done;
	}
	
	if (!IsListEmpty(&pAddr->pendedReceiveRequests))
	{
		// Indicate the packets to pended TdiReceive requests
		while (!IsListEmpty(&pAddr->pendedReceiveRequests) &&
			!IsListEmpty(&pAddr->receivedPackets))
		{
			pIrpEntry = RemoveHeadList(&pAddr->pendedReceiveRequests);

			if (pIrpEntry && (pIrpEntry != &pAddr->pendedReceiveRequests))
			{
				KdPrint((DPREFIX"udp_indicateReceivedPackets: Processing queued TdiReceiveDatagram IRP\n" ));

				irp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

				InitializeListHead(&irp->Tail.Overlay.ListEntry);
	
				IoSetCancelRoutine(irp, NULL);

				status = udp_indicateReceivedPacketsForIrp(irp, &pAddr->receivedPackets);

				// Complete request immediately
				sl_unlock(&pAddr->lock, irqld);
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				sl_lock(&pAddr->lock, &irqld);

				if (!NT_SUCCESS(status) || !pAddr->fileObject || (pAddr->id != id))
				{
					goto done;
				}
			}
		}
		// Continue indicating packets via events
	}
	
	while (!IsListEmpty(&pAddr->receivedPackets))
	{
		status = udp_indicateReceivedPacketsViaEvent(pAddr, &irqld);

		if (!NT_SUCCESS(status))
		{
			if (status == STATUS_DATA_NOT_ACCEPTED)
			{
				status = STATUS_SUCCESS;
			}
			break;
		}
	}

done:
	
	if (pAddr->fileObject && (pAddr->id == id))
	{
		if (NT_SUCCESS(status))
		{
			if (IsListEmpty(&pAddr->receivedPackets))
			{
				ctrl_queuePush(NF_UDP_CAN_RECEIVE, pAddr->id);
			}
		}
	}

	sl_unlock(&pAddr->lock, irqld);

	return status;
}

