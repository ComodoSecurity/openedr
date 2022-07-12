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
#include "tcpconn.h"
#include "tcpsend.h"
#include "gvars.h"
#include "addr.h"
#include "packet.h"
#include "ctrlio.h"
#include "mempools.h"

#ifdef _WPPTRACE
#include "tcpsend.tmh"
#endif

void tcp_cancelTdiSend(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"tcp_cancelTdiSend\n"));

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


NTSTATUS tcp_TdiSend(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
    PDRIVER_CANCEL		oldCancelRoutine;
	KIRQL				irql, irqld;
    
	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	// INDICATE

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		{
			PTDI_REQUEST_KERNEL_SEND    prk;

			prk = (PTDI_REQUEST_KERNEL_SEND)&irpSp->Parameters;
			if (prk)
			{
				KdPrint((DPREFIX"tcp_TdiSend[%I64d] length=%d, flags=%x\n", pConn->id, prk->SendLength, prk->SendFlags));

				if (prk->SendFlags & TDI_SEND_EXPEDITED)
				{
					sl_unlock(&g_vars.slConnections, irql);

					IoSkipCurrentIrpStackLocation(irp);

					return IoCallDriver(pLowerDevice, irp);
				}
			}
		}

		if ((pConn->filteringFlag & NF_FILTER) &&
			!pConn->userModeFilteringDisabled)
		{
			if (ctrl_attached() && 
				!pConn->disconnectCalled &&
				!pConn->sendError)
			{
				sl_lock(&pConn->lock, &irqld);
				InsertTailList(&pConn->pendedSendRequests, &irp->Tail.Overlay.ListEntry);
				sl_unlock(&pConn->lock, irqld);

				IoMarkIrpPending(irp);

				// This field is used in ctrl_copyTcpSendToBuffer as the current
				// buffer offset. It must be zero at the beginning. 
				irp->IoStatus.Information = 0;

				oldCancelRoutine = IoSetCancelRoutine(irp, tcp_cancelTdiSend);
				irp->IoStatus.Status = STATUS_PENDING;

				status = STATUS_PENDING;

				KdPrint((DPREFIX"tcp_TdiSend pended [%I64d]\n", pConn->id));

				if (!NT_SUCCESS(ctrl_queuePush(NF_TCP_SEND, pConn->id)))
				{
					sl_lock(&pConn->lock, &irqld);
					RemoveEntryList(&irp->Tail.Overlay.ListEntry);
					InitializeListHead(&irp->Tail.Overlay.ListEntry);
					sl_unlock(&pConn->lock, irqld);

					IoSetCancelRoutine(irp, oldCancelRoutine);
					irp->IoStatus.Status = STATUS_SUCCESS;

					status = STATUS_SUCCESS;
				}

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
			KdPrint((DPREFIX"tcp_TdiSend bypass [%I64d]\n", pConn->id));
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (status == STATUS_PENDING)
		return status;

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiSendPossibleHandler(
    IN PVOID  TdiEventContext,
    IN PVOID  ConnectionContext,
    IN ULONG  BytesAvailable
    )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_SUCCESS;

	KdPrint((DPREFIX"tcp_TdiSendPossibleHandler\n"));
	
	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiSendPossibleHandler: pAddr == NULL\n"));
		return STATUS_DATA_NOT_ACCEPTED;
	}

	if (pAddr->ev_send_possible)
	{
		status = pAddr->ev_send_possible(
			pAddr->ev_send_possible_context,
			ConnectionContext,   
			BytesAvailable
			);
	}

	return status;
}


NTSTATUS tcp_TdiInternalSendComplete(PDEVICE_OBJECT pDeviceObject, PIRP irp, void * context)
{
	PNF_PACKET			pPacket = (PNF_PACKET) context;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = irp->IoStatus.Status;
    ULONG               nByteCount = (ULONG)irp->IoStatus.Information;
	KIRQL				irql, irqld;

	KdPrint((DPREFIX"tcp_TdiInternalSendComplete status=0x%8.8x, bytes=%d\n", 
		status, nByteCount));

	if (!ctrl_attached() || !pPacket)
	{
		if (pPacket)
		{
			nf_packet_free(pPacket);
		}
		IoFreeIrp(irp);    // It's Ours. Free it up
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)pPacket->fileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		sl_lock(&pConn->lock, &irqld);

		pConn->sendBytesInTdi -= pPacket->dataSize;
	
		if (pConn->sendBytesInTdi < TDI_SNDBUF)
		{
			pConn->sendInProgress = FALSE;
		}
	
		if (NT_SUCCESS(status))
		{
			nf_packet_free(pPacket);

			if (IsListEmpty(&pConn->sendPackets))
			{
				ctrl_queuePush(NF_TCP_CAN_SEND, pConn->id);
			}
		} else
		{
			pConn->sendError = TRUE;

			if (pConn->disconnectEventPending || pConn->disconnectEventCalled)
			{
				// Send request failed because remote peer has disconnected
				nf_packet_free(pPacket);
			} else
			{
				// Send request failed unexpectedly. 
				// Treat this as a disconnect request from remote peer.

				pPacket->ioStatus = status;
				pPacket->dataSize = 0;

				KdPrint((DPREFIX"tcp_TdiSend [%I64d]: Disconnecting\n", pConn->id));

				pConn->disconnectEventPending = TRUE;
				pConn->disconnectEventFlags = TDI_DISCONNECT_RELEASE;

				InsertTailList(
					&pConn->pendedReceivedPackets,
					&pPacket->entry
				   );

				ctrl_queuePush(NF_TCP_RECEIVE, pConn->id);
			}

			if (IsListEmpty(&pConn->sendPackets))
			{
				ctrl_queuePush(NF_TCP_CAN_SEND, pConn->id);
			}
		}

		sl_unlock(&pConn->lock, irqld);
	} else
	{
		KdPrint((DPREFIX"tcp_TdiInternalSendComplete: free packet(2)\n"));
		nf_packet_free(pPacket);
	}

	sl_unlock(&g_vars.slConnections, irql);

    IoFreeIrp(irp);    // It's Ours. Free it up

	return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS tcp_TdiStartInternalSend(PCONNOBJ pConn, HASH_ID id)
{
	PIRP				irp = NULL;
	PNF_PACKET			pPacket = NULL;
	KIRQL				irqld;
	PDEVICE_OBJECT		pLowerDevice = NULL;
	NTSTATUS			status;

	KdPrint((DPREFIX"tcp_TdiStartInternalSend [%I64d]\n", pConn->id));

	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	if (!pConn)
	{
		return STATUS_UNSUCCESSFUL;
	}

	sl_lock(&pConn->lock, &irqld);

	if (!pConn->fileObject || !pConn->pAddr || (pConn->id != id))
	{
		ASSERT(0);
		sl_unlock(&pConn->lock, irqld);
		return STATUS_UNSUCCESSFUL;
	}

	pPacket = (PNF_PACKET)pConn->sendPackets.Flink;
	if (!pPacket || (pPacket == (PNF_PACKET)&pConn->sendPackets))
	{
		sl_unlock(&pConn->lock, irqld);
		return STATUS_UNSUCCESSFUL;
	}
	
	RemoveEntryList(&pPacket->entry);

	if (pPacket->dataSize > 0)
	{
		if (pConn->disconnectCalled)
		{
			ctrl_queuePush(NF_TCP_CAN_SEND, pConn->id);
			ctrl_queuePush(NF_TCP_CAN_RECEIVE, pConn->id);

			sl_unlock(&pConn->lock, irqld);

			nf_packet_free(pPacket);

			return STATUS_SUCCESS;
		}

		if (pConn->sendInProgress)
		{
			// Too much data in TDI
			sl_unlock(&pConn->lock, irqld);

			nf_packet_free(pPacket);

			return STATUS_UNSUCCESSFUL;
		}
	} else
	{
		// Call TDI_DISCONNECT

		nf_packet_free(pPacket);

		if (pConn->flags & TDI_SEND_AND_DISCONNECT)
		{
			if (ctrl_attached() && (pConn->filteringFlag & NF_FILTER) && (pConn->sendBytesInTdi > 0))
			{
				KdPrint((DPREFIX"tcp_TdiStartInternalSend [%I64d] TDI_SEND_AND_DISCONNECT flag is active for filtered connection, waiting for send completion\n", pConn->id));
				sl_unlock(&pConn->lock, irqld);
				return STATUS_UNSUCCESSFUL;
			}
		}

		return tcp_callTdiDisconnect(pConn, irqld);
	}

	pLowerDevice = pConn->lowerDevice;
	if (!pLowerDevice)
	{
		sl_unlock(&pConn->lock, irqld);

		ASSERT(0);

		nf_packet_free(pPacket);

		return STATUS_UNSUCCESSFUL;
	}

	pPacket->fileObject = (PFILE_OBJECT)pConn->fileObject;

	irp = IoAllocateIrp(pLowerDevice->StackSize+2, FALSE);
	if (!irp)
	{
		sl_unlock(&pConn->lock, irqld);

		ASSERT(0);

		nf_packet_free(pPacket);

		return STATUS_UNSUCCESSFUL;
	}

	KdPrint((DPREFIX"tcp_TdiStartInternalSend[%I64d]: TdiBuildSend size=%d\n",
		pConn->id, pPacket->dataSize));
		
	TdiBuildSend(
			irp,							// IRP
			pLowerDevice,				// Device
			(PFILE_OBJECT)pConn->fileObject, // File object
			tcp_TdiInternalSendComplete,	// CompletionRoutine
			pPacket,						// Context
			pPacket->pMdl,					// MdlAddress
			0,								// SendFlags
			pPacket->dataSize				// SendLength
		);

	if (pConn->sendBytesInTdi >= TDI_SNDBUF)
	{
		pConn->sendInProgress = TRUE;
	}

	pConn->sendBytesInTdi += pPacket->dataSize;

	sl_unlock(&pConn->lock, irqld);

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	status = IoCallDriver(pLowerDevice, irp);
	if (NT_ERROR(status))
	{
		KdPrint((DPREFIX"tcp_TdiInternalSend[%I64d] IoCallDriver status=0x%8.8x\n", pConn->id, status));
		return STATUS_SUCCESS;
	}
	return status;
}

