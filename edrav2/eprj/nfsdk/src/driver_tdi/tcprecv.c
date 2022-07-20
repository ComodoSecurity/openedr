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
#include "tcprecv.h"
#include "gvars.h"
#include "addr.h"
#include "packet.h"
#include "ctrlio.h"
#include "mempools.h"

#ifdef _WPPTRACE
#include "tcprecv.tmh"
#endif

void tcp_cancelTdiReceive(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"tcp_cancelTdiReceive\n"));

    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	//  Dequeue and complete the IRP.  
 
	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		sl_lock(&pConn->lock, &irqld);
		if (!IsListEmpty(&irp->Tail.Overlay.ListEntry))
		{
			RemoveEntryList(&irp->Tail.Overlay.ListEntry);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);
			mustComplete = TRUE;
		}
		sl_unlock(&pConn->lock, irqld);
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


NTSTATUS tcp_TdiReceive(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
	KIRQL				irql, irqld;
	PTDI_REQUEST_KERNEL_RECEIVE   prk;

	KdPrint((DPREFIX"tcp_TdiReceive\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	prk = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		if ((pConn->filteringFlag & NF_FILTER) &&
			!pConn->userModeFilteringDisabled)
		{
			if (prk->ReceiveFlags & TDI_RECEIVE_PEEK)
			{
				KdPrint((DPREFIX"tcp_TdiReceive peek [%I64d]\n", pConn->id));

				if (tcp_getTotalReceiveSize(pConn, 0) == 0)
				{
					sl_unlock(&g_vars.slConnections, irql);
				
					KdPrint((DPREFIX"tcp_TdiReceive peek - no data [%I64d]\n", pConn->id));
		
					irp->IoStatus.Information = 0;
					irp->IoStatus.Status = STATUS_SUCCESS;
					IoCompleteRequest(irp, IO_NO_INCREMENT);
					return STATUS_SUCCESS;
				}
			}

			if (ctrl_attached() && !pConn->disconnectEventCalled && pConn->connected)
			{
				sl_lock(&pConn->lock, &irqld);
				InsertTailList(&pConn->pendedReceiveRequests, &irp->Tail.Overlay.ListEntry);
				sl_unlock(&pConn->lock, irqld);

				IoMarkIrpPending(irp);

				IoSetCancelRoutine(irp, tcp_cancelTdiReceive);

				irp->IoStatus.Status = STATUS_PENDING;

				KdPrint((DPREFIX"tcp_TdiReceive pended [%I64d]\n", pConn->id));

				// Indicate received data
				tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);

				status = STATUS_PENDING;
			} else
			{
				sl_unlock(&g_vars.slConnections, irql);
			
				irp->IoStatus.Information = 0;
				irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				return STATUS_INVALID_DEVICE_STATE;
			}
		} else
		{
			KdPrint((DPREFIX"tcp_TdiReceive bypass [%I64d]\n", pConn->id));
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (status == STATUS_PENDING)
		return status;

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiReceiveEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG BytesIndicated,
   ULONG BytesAvailable,
   ULONG *BytesTaken,
   PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_DATA_NOT_ACCEPTED;
	PCONNOBJ	pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	ULONG		flag = 0;
	KIRQL		irql, irqld;
	PTDI_IND_RECEIVE    ev_receive = NULL;           // Receive event handler
    PVOID               ev_receive_context = NULL;    // Receive context

	KdPrint((DPREFIX"tcp_TdiReceiveEventHandler flags=%x, bytes=%d\n", ReceiveFlags, BytesAvailable));
	
	sl_lock(&g_vars.slConnections, &irql);

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiReceiveEventHandler: pAddr == NULL\n"));
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_DATA_NOT_ACCEPTED;
	}

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || !pAddr->ev_receive || !pAddr->ev_receive_context)
	{
		KdPrint((DPREFIX"tcp_TdiReceiveEventHandler: !pAddr->fileObject\n"));
		sl_unlock(&pAddr->lock, irqld);
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_DATA_NOT_ACCEPTED;
	}

	ev_receive = pAddr->ev_receive;
	ev_receive_context = pAddr->ev_receive_context;

	sl_unlock(&pAddr->lock, irqld);
	sl_unlock(&g_vars.slConnections, irql);

	if (ctrl_attached())
	{
		sl_lock(&g_vars.slConnections, &irql);
		phte = ht_find_entry(g_vars.phtConnByContext, (HASH_ID)ConnectionContext);
		if (phte)
		{
			pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, context);

			sl_lock(&pConn->lock, &irqld);

			if (!pConn->userModeFilteringDisabled)
			{
				flag = pConn->filteringFlag;

				KdPrint((DPREFIX"tcp_TdiReceiveEventHandler[%I64d]: flag=%d\n", pConn->id, flag));

				if (flag & NF_FILTER)
				{
					pConn->recvBytesInTdi = BytesAvailable;

					KdPrint((DPREFIX"tcp_TdiReceiveEventHandler[%I64d]: recvBytesInTdi=%d\n", pConn->id, pConn->recvBytesInTdi));

					*BytesTaken = 0;

					// Start TDI_RECEIVE for the indicated bytes 
					if (!(flag & NF_SUSPENDED))
					{
						tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
					} else
					{
						KdPrint((DPREFIX"tcp_TdiReceiveEventHandler[%I64d]: connection suspended\n", pConn->id));
					}
			
					sl_unlock(&pConn->lock, irqld);
					sl_unlock(&g_vars.slConnections, irql);

					return STATUS_DATA_NOT_ACCEPTED;
				}
			} else
			{
				KdPrint((DPREFIX"tcp_TdiReceiveEventHandler[%I64d]: userModeFilteringDisabled\n", pConn->id));
			}
			
			sl_unlock(&pConn->lock, irqld);
		} else
		{
			KdPrint((DPREFIX"tcp_TdiReceiveEventHandler: connection context not found\n"));
		}
		sl_unlock(&g_vars.slConnections, irql);
	}

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiReceiveEventHandler: pAddr == NULL\n"));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	if (ev_receive)
	{
		status = ev_receive(
			ev_receive_context,
			ConnectionContext,   
			ReceiveFlags,        
			BytesIndicated,          
			BytesAvailable,                
			BytesTaken,             
			Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
			IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
			);
	}

	return status;
}

NTSTATUS tcp_TdiReceiveExpeditedEventHandler(
   PVOID TdiEventContext,     // Context From SetEventHandler
   CONNECTION_CONTEXT ConnectionContext,
   ULONG ReceiveFlags,
   ULONG BytesIndicated,
   ULONG BytesAvailable,
   ULONG *BytesTaken,
   PVOID Tsdu,				// pointer describing this TSDU, typically a lump of bytes
   PIRP *IoRequestPacket	// TdiReceive IRP if MORE_PROCESSING_REQUIRED.
   )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_DATA_NOT_ACCEPTED;
	PCONNOBJ	pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	ULONG		flag = 0;
	KIRQL		irql, irqld;
	PTDI_IND_RECEIVE_EXPEDITED    ev_receive = NULL;           // Receive event handler
    PVOID               ev_receive_context = NULL;    // Receive context

	KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler flags=%x, bytes=%d\n", ReceiveFlags, BytesAvailable));
	
	sl_lock(&g_vars.slConnections, &irql);

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler: pAddr == NULL\n"));
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_DATA_NOT_ACCEPTED;
	}

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || !pAddr->ev_receive || !pAddr->ev_receive_context)
	{
		KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler: !pAddr->fileObject\n"));
		sl_unlock(&pAddr->lock, irqld);
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_DATA_NOT_ACCEPTED;
	}

	ev_receive = pAddr->ev_receive_expedited;
	ev_receive_context = pAddr->ev_receive_expedited_context;

	sl_unlock(&pAddr->lock, irqld);
	sl_unlock(&g_vars.slConnections, irql);

	if (ctrl_attached())
	{
		sl_lock(&g_vars.slConnections, &irql);
		phte = ht_find_entry(g_vars.phtConnByContext, (HASH_ID)ConnectionContext);
		if (phte)
		{
			pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, context);

			sl_lock(&pConn->lock, &irqld);

			if (!pConn->userModeFilteringDisabled)
			{
				flag = pConn->filteringFlag;

				KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler[%I64d]: flag=%d\n", pConn->id, flag));

				if (flag & NF_FILTER)
				{
					// Increase the counter for tcp_TdiStartInternalReceive
					pConn->recvBytesInTdi = BytesAvailable;

					KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler[%I64d]: recvBytesInTdi=%d\n", pConn->id, pConn->recvBytesInTdi));

					*BytesTaken = 0;

					// Start TDI_RECEIVE for the indicated bytes 
					if (!(flag & NF_SUSPENDED))
					{
						tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
					} else
					{
						KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler[%I64d]: connection suspended\n", pConn->id));
					}
			
					sl_unlock(&pConn->lock, irqld);
					sl_unlock(&g_vars.slConnections, irql);

					return STATUS_DATA_NOT_ACCEPTED;
				}
			} else
			{
				KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler[%I64d]: userModeFilteringDisabled\n", pConn->id));
			}
			
			sl_unlock(&pConn->lock, irqld);
		} else
		{
			KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler: connection context not found\n"));
		}
		sl_unlock(&g_vars.slConnections, irql);
	}

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiReceiveExpeditedEventHandler: pAddr == NULL\n"));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	if (ev_receive)
	{
		status = ev_receive(
			ev_receive_context,
			ConnectionContext,   
			ReceiveFlags,        
			BytesIndicated,          
			BytesAvailable,                
			BytesTaken,             
			Tsdu,                   // pointer describing this TSDU, typically a lump of bytes
			IoRequestPacket         // TdiReceive IRP if MORE_PROCESSING_REQUIRED.
			);
	}

	return status;
}

NTSTATUS tcp_TdiChainedReceiveHandler(
    IN PVOID  TdiEventContext,
    IN CONNECTION_CONTEXT  ConnectionContext,
    IN ULONG  ReceiveFlags,
    IN ULONG  ReceiveLength,
    IN ULONG  StartingOffset,
    IN PMDL	  Tsdu,
    IN PVOID  TsduDescriptor
    )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_DATA_NOT_ACCEPTED;
	PCONNOBJ	pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	ULONG		flag = 0;
	KIRQL		irql, irqld;

	KdPrint((DPREFIX"tcp_TdiChainedReceiveHandler\n"));
	
	sl_lock(&g_vars.slConnections, &irql);
	phte = ht_find_entry(g_vars.phtConnByContext, (HASH_ID)ConnectionContext);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, context);

		sl_lock(&pConn->lock, &irqld);
		flag = pConn->filteringFlag;
		sl_unlock(&pConn->lock, irqld);
	}
	sl_unlock(&g_vars.slConnections, irql);

	if (flag & NF_FILTER)
	{
		return STATUS_DATA_NOT_ACCEPTED;
	}

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiChainedReceiveHandler: pAddr == NULL\n"));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	if (pAddr->ev_chained_receive)
	{
		status = pAddr->ev_chained_receive(
			pAddr->ev_chained_receive_context,
			ConnectionContext,   
			ReceiveFlags,
			ReceiveLength,
			StartingOffset,
			Tsdu,
			TsduDescriptor
			);
	}

	return status;
}

NTSTATUS tcp_indicateReceivedPacketsForIrp(PIRP irp, PLIST_ENTRY packets)
{
	ULONG			bytesCopied = 0;
	ULONG			bytesRemaining;
	PUCHAR			pVA;
	ULONG			nVARemaining;
	PNF_PACKET		pPacket;
	PTDI_REQUEST_KERNEL_RECEIVE   prk;
	PIO_STACK_LOCATION	irpSp = NULL;
	PMDL			mdl;
    ULONG           bytesToCopy;
	ULONG			receiveFlags;
	NTSTATUS		status = STATUS_SUCCESS;

	if (irp == NULL)
		return status;

	irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	prk = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;

	bytesRemaining = prk->ReceiveLength;
	receiveFlags = prk->ReceiveFlags;

	KdPrint((DPREFIX"tcp_indicateReceivedPacketsForIrp bytes=%d flags=%d\n", bytesRemaining, receiveFlags));

	mdl = irp->MdlAddress;

	if (mdl == NULL)
	{
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		return status;
	}

	nVARemaining = MmGetMdlByteCount(mdl);

	if (nVARemaining > bytesRemaining)
    {
		nVARemaining = bytesRemaining;
    }

	pVA = MmGetSystemAddressForMdlSafe(mdl, HighPagePriority);
	if (pVA == NULL)
	{
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		return status;
	}

	while (mdl && !IsListEmpty(packets))
	{
		pPacket = (PNF_PACKET)packets->Flink;
		
		if (!NT_SUCCESS(pPacket->ioStatus) || !pPacket->dataSize)
		{
			KdPrint((DPREFIX"tcp_indicateReceivedPacketsForIrp: bad packet status=%x, dataSize=%d\n", 
				pPacket->ioStatus, pPacket->dataSize));

			status = STATUS_CONNECTION_DISCONNECTED;

			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = (bytesCopied > 0)? STATUS_SUCCESS : STATUS_INVALID_DEVICE_STATE;
			irp->IoStatus.Information = bytesCopied;

			return status;
		}

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
			pPacket = NULL;
        }

        if (bytesToCopy < nVARemaining)
		{
            pVA += bytesToCopy;
			nVARemaining -= bytesToCopy;
		} else 
		if (bytesToCopy == nVARemaining)
		{
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

	KdPrint((DPREFIX"tcp_indicateReceivedPacketsForIrp: Bytes copied %d\n", bytesCopied));

	return STATUS_SUCCESS;
}

// Returns the number of bytes in receive queue, or limit
// if the queue contains disconnect packet.
ULONG tcp_getTotalReceiveSize(PCONNOBJ pConn, ULONG limit)
{
	ULONG		size = 0;
	PNF_PACKET	pPacket = (PNF_PACKET)pConn->receivedPackets.Flink;

	while (pPacket != (PNF_PACKET)&pConn->receivedPackets)
	{
		if ((limit > 0) && (pPacket->dataSize == 0))
		{
			return limit;
		}
		size += pPacket->dataSize;
		pPacket = (PNF_PACKET)pPacket->entry.Flink;
	}
	
	return size;
}

NTSTATUS tcp_indicateReceivedPacketsViaEvent(PCONNOBJ pConn, KIRQL * pirqld)
{
	PADDROBJ			pAddr = NULL;
    PTDI_IND_RECEIVE    ev_receive = NULL;           // Receive event handler
    PVOID               ev_receive_context = NULL;    // Receive context
	CONNECTION_CONTEXT	conn_context = NULL;
	NTSTATUS		status = STATUS_SUCCESS;
	PIRP			irp = NULL;
	PNF_PACKET		pPacket;
	ULONG			bytesTaken = 0;
	ULONG			totalSize = 0;

	KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent\n"));

	if (!pConn->fileObject)
	{
		status = STATUS_CONNECTION_DISCONNECTED;
		goto done;
	}

	pAddr = pConn->pAddr;

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: !pAddr\n"));
		status = STATUS_CONNECTION_DISCONNECTED;
		goto done;
	}

	ev_receive = pAddr->ev_receive;
	ev_receive_context = pAddr->ev_receive_context;

	if (!ev_receive)
	{
		KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: !pAddr->ev_receive\n"));
		status = STATUS_CONNECTION_DISCONNECTED;
		goto done;
	}

    pPacket = (PNF_PACKET)pConn->receivedPackets.Flink;
    if (!pPacket || (pPacket == (PNF_PACKET)&pConn->receivedPackets))
	{
		KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: no packet\n"));
		status = STATUS_CONNECTION_DISCONNECTED;
		goto done;
	}

	totalSize = pPacket->dataSize;//tcp_getTotalReceiveSize(pConn, 0);

	RemoveEntryList(&pPacket->entry);

	KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: packet status=%x, dataSize=%d\n",
		pPacket->ioStatus, pPacket->dataSize));

	if (!NT_SUCCESS(pPacket->ioStatus) || !pPacket->dataSize)
	{
		KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: disconnect\n"));
		if (pConn->disconnectEventPending)
		{
			nf_packet_free(pPacket);
			status = STATUS_CONNECTION_DISCONNECTED;
			goto done;
		} else
		{
			totalSize = 0;
		}
	}

	conn_context = (CONNECTION_CONTEXT)pConn->context;

	if (!pConn->receiveEventCalled)
	{
		pConn->receiveEventInProgress = TRUE;

		sl_unlock(&pConn->lock, *pirqld);

		status = ev_receive(
					ev_receive_context,
					conn_context,
					(	
						TDI_RECEIVE_NORMAL 
						| TDI_RECEIVE_ENTIRE_MESSAGE 
						| TDI_RECEIVE_AT_DISPATCH_LEVEL
					),
					pPacket->dataSize,
					totalSize,
					&bytesTaken,
					&pPacket->buffer[pPacket->offset],  // Tsdu
					&irp                 // TdiReceive IRP if STATUS_MORE_PROCESSING_REQUIRED
				);

		sl_lock(&pConn->lock, pirqld);

		pConn->receiveEventInProgress = FALSE;
	} else
	{
		status = STATUS_DATA_NOT_ACCEPTED;
	}

	if (!pConn->fileObject || !pConn->pAddr)
	{
		nf_packet_free(pPacket);

		if ((status == STATUS_MORE_PROCESSING_REQUIRED) && irp)
		{
			irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
			irp->IoStatus.Information = 0;

			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, FALSE);
		}

		status = STATUS_CONNECTION_DISCONNECTED;

		goto done;
	}

	KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: ev_receive status=0x%8.8X, totalSize=%d, dataSize=%d, taken=%d\n", 
		status, totalSize, pPacket->dataSize, bytesTaken));

	if (status == STATUS_DATA_NOT_ACCEPTED)
	{
		bytesTaken = 0;
	} else
	if ((status == STATUS_SUCCESS) && (bytesTaken == 0))
	{
		// This is a workaround for Vista SMB. 
		// (Sometimes it returns STATUS_SUCCESS with zero bytesTaken).
		
		status = STATUS_DATA_NOT_ACCEPTED;
	}

    ASSERT(bytesTaken <= totalSize);
    if (bytesTaken > totalSize)
    {
        bytesTaken = totalSize;
    }

	if (bytesTaken < pPacket->dataSize)
	{
		pPacket->dataSize -= bytesTaken;
		pPacket->offset += bytesTaken;

		InsertHeadList(&pConn->receivedPackets, &pPacket->entry);

		if (status == STATUS_SUCCESS)
		{
			status = STATUS_DATA_NOT_ACCEPTED;
		}
	} else 
    {
		nf_packet_free(pPacket);
		pPacket = NULL;
	}

	if (status == STATUS_DATA_NOT_ACCEPTED)
	{
		pConn->receiveEventCalled = TRUE;
	}

	if ((status == STATUS_MORE_PROCESSING_REQUIRED) && irp)
	{
		status = tcp_indicateReceivedPacketsForIrp(irp, &pConn->receivedPackets);
	
		if (irp->IoStatus.Information > 0)
		{
			// Complete request immediately
			pConn->receiveEventCalled = FALSE;
			sl_unlock(&pConn->lock, *pirqld);
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			sl_lock(&pConn->lock, pirqld);
		} else
		{
			InsertTailList(&pConn->pendedReceiveRequests, &irp->Tail.Overlay.ListEntry);

			IoMarkIrpPending(irp);

			IoSetCancelRoutine(irp, tcp_cancelTdiReceive);

			irp->IoStatus.Status = STATUS_PENDING;

			KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent[%I64d] tcp_TdiReceive pended \n", pConn->id));
		}
	} 

	if (NT_SUCCESS(status))
	{
		pPacket = (PNF_PACKET)pConn->receivedPackets.Flink;
		if (pPacket && (pPacket != (PNF_PACKET)&pConn->receivedPackets))
		{
			KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: next packet status=%x, dataSize=%d\n",
				pPacket->ioStatus, pPacket->dataSize));

			if (pConn->disconnectEventPending && (!NT_SUCCESS(pPacket->ioStatus) || !pPacket->dataSize))
			{
				KdPrint((DPREFIX"tcp_indicateReceivedPacketsViaEvent: disconnect\n"));
				RemoveEntryList(&pPacket->entry);
				nf_packet_free(pPacket);
				status = STATUS_CONNECTION_DISCONNECTED;
				goto done;
			}
		}
	}

done:

	return status;
}


NTSTATUS tcp_indicateReceivedPackets(PCONNOBJ pConn, HASH_ID id)
{
    PLIST_ENTRY pIrpEntry;
    PIRP		irp;
	KIRQL		irqld;
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG		receiveLimit = TDI_SNDBUF;

	sl_lock(&pConn->lock, &irqld);

	KdPrint((DPREFIX"tcp_indicateReceivedPackets [%I64d]\n", pConn->id));

	if (!pConn->fileObject || !pConn->pAddr || (pConn->id != id))
	{
		goto done;
	}

	if (IsListEmpty(&pConn->receivedPackets))
	{
		KdPrint((DPREFIX"tcp_indicateReceivedPackets [%I64d]: No Data\n", pConn->id));
		
		if (!pConn->pAddr->ev_receive)
		{

			KdPrint((DPREFIX"tcp_indicateReceivedPackets [%I64d]: no receive handler\n", pConn->id));
			
			pConn->recvBytesInTdi = 1;

			tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
		}

		goto done;
	}
	
	while (!IsListEmpty(&pConn->receivedPackets))
	{
		if (!IsListEmpty(&pConn->pendedReceiveRequests))
		{
			// Indicate the packets to pended TdiReceive requests
			while (!IsListEmpty(&pConn->pendedReceiveRequests) &&
				!IsListEmpty(&pConn->receivedPackets))
			{
				pIrpEntry = RemoveHeadList(&pConn->pendedReceiveRequests);

				if (pIrpEntry) 
				{
					PTDI_REQUEST_KERNEL_RECEIVE   prk;
					PIO_STACK_LOCATION	irpSp = NULL;
					ULONG           receiveLength = 0;
					ULONG			receiveFlags = 0;

					irp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

					irpSp = IoGetCurrentIrpStackLocation(irp);
					ASSERT(irpSp);

					prk = (PTDI_REQUEST_KERNEL_RECEIVE)&irpSp->Parameters;

					receiveLength = prk->ReceiveLength;
					receiveFlags = prk->ReceiveFlags;

					KdPrint((DPREFIX"tcp_indicateReceivedPackets: Processing queued TdiReceive IRP flags=%x, length=%u\n", 
						receiveFlags, receiveLength));

					if (receiveFlags & TDI_RECEIVE_NO_PUSH)
					{
						// Indicate the full buffer when TDI_RECEIVE_NO_PUSH flag is specified for TDI_RECEIVE
						if ((tcp_getTotalReceiveSize(pConn, receiveLength) < receiveLength) &&
							(receiveLength <= TDI_SNDBUF))
						{
							KdPrint((DPREFIX"tcp_indicateReceivedPackets: TdiReceive IRP TDI_RECEIVE_NO_PUSH, wait for more data\n" ));
							InsertHeadList(&pConn->pendedReceiveRequests, pIrpEntry);
							status = STATUS_SUCCESS;
							goto done;
						}
					}

					InitializeListHead(&irp->Tail.Overlay.ListEntry);
		
					IoSetCancelRoutine(irp, NULL);

					status = tcp_indicateReceivedPacketsForIrp(irp, &pConn->receivedPackets);

					pConn->receiveEventCalled = FALSE;
					// Complete request immediately
					sl_unlock(&pConn->lock, irqld);
					IoCompleteRequest(irp, IO_NO_INCREMENT);
					sl_lock(&pConn->lock, &irqld);

					if (!pConn->fileObject || (pConn->id != id))
						goto done;

					if (!NT_SUCCESS(status))
					{
						PNF_PACKET pPacket;
					
						// Repeat zero packet (disconnect)
						if (!IsListEmpty(&pConn->receivedPackets))
						{
							pPacket = (PNF_PACKET)pConn->receivedPackets.Flink;
							if (!NT_SUCCESS(pPacket->ioStatus) || !pPacket->dataSize)
							{
								KdPrint((DPREFIX"tcp_indicateReceivedPackets [%I64d]: Repeat disconnect\n", pConn->id));
						
								RemoveEntryList(&pPacket->entry);
								InsertTailList(&pConn->pendedReceivedPackets, &pPacket->entry);
								ctrl_queuePush(NF_TCP_RECEIVE, pConn->id);
							}
						}

						status = STATUS_SUCCESS;
						goto done;
					}
				}
			}
			// Continue indication via events
		}
		
		if (!IsListEmpty(&pConn->receivedPackets))
		{
			status = tcp_indicateReceivedPacketsViaEvent(pConn, &irqld);

			if (!NT_SUCCESS(status))
			{
				if (status == STATUS_DATA_NOT_ACCEPTED)
				{
					// The process refused data packet
					status = STATUS_SUCCESS;
					receiveLimit = 0;
				}
				break;
			}
		}
	}

done:
	
	if (pConn->fileObject && (pConn->id == id))
	{
		if (NT_SUCCESS(status))
		{
			// Request new packets if necessary
			if (tcp_getTotalReceiveSize(pConn, 0) < receiveLimit)
			{
				if (pConn->disableUserModeFiltering)
				{
					if (pConn->recvBytesInTdi > 0)
					{
						tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
					} 
				} else
				{
					ctrl_queuePush(NF_TCP_CAN_RECEIVE, pConn->id);
				}
			}
		}
	}

	if (!NT_SUCCESS(status) && pConn->fileObject && (pConn->id == id))
	{
		KdPrint((DPREFIX"tcp_indicateReceivedPackets[%I64d]: Status=%x, calling disconnect event\n", 
			pConn->id, status ));
			
		tcp_callTdiDisconnectEventHandler(pConn, irqld);
	} else
	{
		sl_unlock(&pConn->lock, irqld);
	}

	return status;
}

NTSTATUS tcp_TdiInternalReceiveComplete(PDEVICE_OBJECT pDeviceObject, PIRP irp, void * context)
{
	PNF_PACKET			pPacket = (PNF_PACKET) context;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = irp->IoStatus.Status;
    ULONG               nByteCount = (ULONG)irp->IoStatus.Information;
	KIRQL				irql, irqld;

	KdPrint((DPREFIX"tcp_TdiInternalReceiveComplete status=0x%8.8x, bytes=%d\n", status, nByteCount));

	if (!pPacket)
    {
		IoFreeIrp(irp);    // It's Ours. Free it up

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

	if (!ctrl_attached())
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

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)pPacket->fileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		sl_lock(&pConn->lock, &irqld);
		
		pConn->receiveInProgress = FALSE;
		
		// Update the counter for tcp_TdiStartInternalReceive
		if (pConn->recvBytesInTdi > (int)nByteCount)
		{
			pConn->recvBytesInTdi -= nByteCount;
		} else
		{
			pConn->recvBytesInTdi = 0;
		}

		KdPrint((DPREFIX"tcp_TdiInternalReceiveComplete[%I64d] recvBytesInTdi=%d\n", pConn->id, pConn->recvBytesInTdi));

		if (nByteCount > 0)
		{
			if (pConn->disableUserModeFiltering)
			{
				InsertTailList(
					&pConn->receivedPackets,
					&pPacket->entry
				   );
				
				tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);
			} else
			{
				InsertTailList(
					&pConn->pendedReceivedPackets,
					&pPacket->entry
				   );
				
				ctrl_queuePush(NF_TCP_RECEIVE, pConn->id);
			}
		} else
		{
			// Receive failed, treat this as a disconnect request
			
			if (pConn->disconnectEventCalled)
			{
				nf_packet_free(pPacket);
			} else
			{
				pConn->disconnectEventPending = TRUE;
				pConn->disconnectEventFlags = TDI_DISCONNECT_RELEASE;

				if (pConn->disableUserModeFiltering)
				{
					InsertTailList(
						&pConn->receivedPackets,
						&pPacket->entry
					   );
					
					tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);
				} else
				{
					InsertTailList(
						&pConn->pendedReceivedPackets,
						&pPacket->entry
					   );

					ctrl_queuePush(NF_TCP_RECEIVE, pConn->id);
				}
			}
		}

		sl_unlock(&pConn->lock, irqld);

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

NTSTATUS tcp_TdiStartInternalReceive(PCONNOBJ pConn, HASH_ID id)
{
	PIRP				irp = NULL;
	PNF_PACKET			pPacket = NULL;
	KIRQL				irqld;
	PDEVICE_OBJECT		pLowerDevice = NULL;

	KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]\n", pConn->id));

	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	if (!pConn)
	{
		KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]: !pConn\n", pConn->id));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	sl_lock(&pConn->lock, &irqld);

	if (!pConn->fileObject || 
		!pConn->pAddr || 
		(pConn->id != id) ||
		pConn->receiveInProgress || 
		!IsListEmpty(&pConn->pendedReceivedPackets) ||
		(pConn->recvBytesInTdi == 0) || // Resolve an incompatibility with ZoneAlarm
		pConn->userModeFilteringDisabled
		)
	{
		sl_unlock(&pConn->lock, irqld);
		KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]: bypass\n", pConn->id));
	    return STATUS_DATA_NOT_ACCEPTED;
	}

	if (pConn->disableUserModeFiltering &&
		(!IsListEmpty(&pConn->pendedReceivedPackets) ||
		!IsListEmpty(&pConn->receivedPackets)))
	{
		KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]: bypass 2\n", pConn->id));
		sl_unlock(&pConn->lock, irqld);
	    return STATUS_DATA_NOT_ACCEPTED;
	}

	pLowerDevice = pConn->lowerDevice;
	if (!pLowerDevice)
	{
		sl_unlock(&pConn->lock, irqld);
		KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]: no lower device\n", pConn->id));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
	if (!pPacket)
	{
		sl_unlock(&pConn->lock, irqld);
		KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]: nf_packet_alloc failed\n", pConn->id));
		ASSERT(0);
	    return STATUS_DATA_NOT_ACCEPTED;
	}
	
	pPacket->fileObject = (PFILE_OBJECT)pConn->fileObject;

	irp = IoAllocateIrp(pLowerDevice->StackSize+2, FALSE);
	if (!irp)
	{
		sl_unlock(&pConn->lock, irqld);
        nf_packet_free(pPacket);
		ASSERT(0);
		KdPrint((DPREFIX"tcp_TdiStartInternalReceive [%I64d]: IoAllocateIrp failed\n", pConn->id));
        return STATUS_DATA_NOT_ACCEPTED;
	}
		
	TdiBuildReceive(
			irp,							// IRP
			pLowerDevice,				// Device
			(PFILE_OBJECT)pConn->fileObject, // File object
			tcp_TdiInternalReceiveComplete, // CompletionRoutine
			pPacket,						// Context
			pPacket->pMdl,					// MdlAddress
			0,				// ReceiveFlags
			pPacket->bufferSize				// ReceiveLength
		);

	pConn->receiveInProgress = TRUE;

	sl_unlock(&pConn->lock, irqld);

	return IoCallDriver(pLowerDevice, irp);
}

