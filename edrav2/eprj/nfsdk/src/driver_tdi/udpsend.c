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
#include "udpsend.h"
#include "gvars.h"
#include "packet.h"
#include "rules.h"
#include "ctrlio.h"
#include "tdiutil.h"

#ifdef _WPPTRACE
#include "udpsend.tmh"
#endif

void udp_cancelTdiSendDatagram(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"udp_cancelTdiSendDatagram\n"));

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


NTSTATUS udp_TdiSendDatagram(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
	KIRQL				irql, irqld;
	NF_FILTERING_FLAG	flag;
    char                NullAddr[TA_ADDRESS_MAX];
	PTA_ADDRESS         pRemoteAddr = NULL;
	PTDI_REQUEST_KERNEL_SENDDG prk;
    PDRIVER_CANCEL		oldCancelRoutine;
    
	KdPrint((DPREFIX"udp_TdiSendDatagram\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	prk = (PTDI_REQUEST_KERNEL_SENDDG)&irpSp->Parameters;

	KdPrint((DPREFIX"udp_TdiSendDatagram length = %d\n", prk->SendLength));

    if (prk &&
        prk->SendDatagramInformation &&
        prk->SendDatagramInformation->RemoteAddress)
    {
	    pRemoteAddr = ((TRANSPORT_ADDRESS *)(prk->SendDatagramInformation->RemoteAddress))->Address;
    } else
    {
        memset(NullAddr, 0, sizeof(NullAddr));
        pRemoteAddr = (PTA_ADDRESS)NullAddr;
        pRemoteAddr->AddressLength = TDI_ADDRESS_LENGTH_IP;
        pRemoteAddr->AddressType = TDI_ADDRESS_TYPE_IP;
    }

	if (pRemoteAddr->AddressType != TDI_ADDRESS_TYPE_IP &&
		pRemoteAddr->AddressType != TDI_ADDRESS_TYPE_IP6)
	{
		KdPrint((DPREFIX"udp_TdiSendDatagram: Unsupported address type\n"));
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pLowerDevice, irp);
	}

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		flag = rule_findByAddr(pAddr, NF_D_OUT, (sockaddr_gen*)&pRemoteAddr->AddressType);

		if (flag & NF_BLOCK)
		{
			sl_unlock(&g_vars.slConnections, irql);
		
			irp->IoStatus.Status = STATUS_REMOTE_NOT_LISTENING;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_REMOTE_NOT_LISTENING;
		} else
		if ((flag & NF_FILTER) &&
			!pAddr->userModeFilteringDisabled &&
			(prk->SendDatagramInformation &&
				prk->SendDatagramInformation->UserDataLength == 0))
		{
			int			pendedPackets;
			
			sl_lock(&pAddr->lock, &irqld);
			pendedPackets = tu_getListSize(&pAddr->pendedSendRequests);
			sl_unlock(&pAddr->lock, irqld);

			if ((pAddr->filteringFlag & NF_SUSPENDED) || (pendedPackets >= 5))
			{
				// Workaround to avoid memory leaks in afd.sys
	
				KdPrint((DPREFIX"udp_TdiSendDatagram: too large queue\n"));
/*
				IoSetCancelRoutine(irp, NULL);

				irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				irp->IoStatus.Information = 0;
				
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				
				status = STATUS_INSUFFICIENT_RESOURCES;
*/
			} else
			{
				sl_lock(&pAddr->lock, &irqld);
				InsertTailList(&pAddr->pendedSendRequests, &irp->Tail.Overlay.ListEntry);
				sl_unlock(&pAddr->lock, irqld);

				IoMarkIrpPending(irp);

				oldCancelRoutine = IoSetCancelRoutine(irp, udp_cancelTdiSendDatagram);
				irp->IoStatus.Status = STATUS_PENDING;

				status = STATUS_PENDING;

				if (!NT_SUCCESS(ctrl_queuePush(NF_UDP_SEND, pAddr->id)))
				{
					sl_lock(&pAddr->lock, &irqld);
					RemoveEntryList(&irp->Tail.Overlay.ListEntry);
					InitializeListHead(&irp->Tail.Overlay.ListEntry);
					sl_unlock(&pAddr->lock, irqld);

					IoSetCancelRoutine(irp, oldCancelRoutine);
					irp->IoStatus.Status = STATUS_SUCCESS;

					status = STATUS_SUCCESS;
				}
			}
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (status == STATUS_PENDING ||
		status == STATUS_INSUFFICIENT_RESOURCES)
		return status;

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}


NTSTATUS udp_TdiSend(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
	KIRQL				irql, irqld;
	NF_FILTERING_FLAG	flag;
    char                NullAddr[TA_ADDRESS_MAX];
	PTA_ADDRESS         pRemoteAddr = NULL;
    PDRIVER_CANCEL		oldCancelRoutine;
    
	KdPrint((DPREFIX"udp_TdiSend\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		if (pAddr->filteringFlag & NF_BLOCK)
		{
			sl_unlock(&g_vars.slConnections, irql);
		
			irp->IoStatus.Status = STATUS_REMOTE_NOT_LISTENING;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_REMOTE_NOT_LISTENING;
		} else
		if ((pAddr->filteringFlag & NF_FILTER) &&
			!pAddr->userModeFilteringDisabled)
		{
			int			pendedPackets;
			
			sl_lock(&pAddr->lock, &irqld);
			pendedPackets = tu_getListSize(&pAddr->pendedSendRequests);
			sl_unlock(&pAddr->lock, irqld);

			if ((pAddr->filteringFlag & NF_SUSPENDED) || (pendedPackets >= 5))
			{
				// Workaround to avoid memory leaks in afd.sys
				KdPrint((DPREFIX"udp_TdiSendDatagram: too large queue[2]\n"));
/*
				IoSetCancelRoutine(irp, NULL);

				irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
				irp->IoStatus.Information = 0;
				
				//ctrl_deferredCompleteIrp(irp);
				IoCompleteRequest(irp, IO_NO_INCREMENT);
				
				status = STATUS_INSUFFICIENT_RESOURCES;
*/
			} else
			{
				sl_lock(&pAddr->lock, &irqld);
				InsertTailList(&pAddr->pendedSendRequests, &irp->Tail.Overlay.ListEntry);
				sl_unlock(&pAddr->lock, irqld);

				IoMarkIrpPending(irp);

				oldCancelRoutine = IoSetCancelRoutine(irp, udp_cancelTdiSendDatagram);
				irp->IoStatus.Status = STATUS_PENDING;

				status = STATUS_PENDING;

				if (!NT_SUCCESS(ctrl_queuePush(NF_UDP_SEND, pAddr->id)))
				{
					sl_lock(&pAddr->lock, &irqld);
					RemoveEntryList(&irp->Tail.Overlay.ListEntry);
					InitializeListHead(&irp->Tail.Overlay.ListEntry);
					sl_unlock(&pAddr->lock, irqld);

					IoSetCancelRoutine(irp, oldCancelRoutine);
					irp->IoStatus.Status = STATUS_SUCCESS;

					status = STATUS_SUCCESS;
				}
			}
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (status == STATUS_PENDING ||
		status == STATUS_INSUFFICIENT_RESOURCES)
		return status;

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}


NTSTATUS udp_TdiInternalSendComplete(PDEVICE_OBJECT pDeviceObject, PIRP irp, void * context)
{
	PNF_PACKET			pPacket = (PNF_PACKET) context;
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = irp->IoStatus.Status;
    ULONG               nByteCount = (ULONG)irp->IoStatus.Information;
	KIRQL				irql, irqld;

	KdPrint((DPREFIX"udp_TdiInternalSendComplete status=0x%8.8x, bytes=%d\n", status, nByteCount));

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

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)pPacket->fileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		sl_lock(&pAddr->lock, &irqld);

		if (pAddr->sendBytesInTdi < (int)pPacket->dataSize)
		{
			pAddr->sendBytesInTdi = 0;
		} else
		{
			pAddr->sendBytesInTdi -= pPacket->dataSize;
		}
	
		if (pAddr->sendBytesInTdi < TDI_SNDBUF)
		{
			pAddr->sendInProgress = FALSE;
		}

		if (IsListEmpty(&pAddr->sendPackets))
		{
			ctrl_queuePush(NF_UDP_CAN_SEND, pAddr->id);
		} 

		sl_unlock(&pAddr->lock, irqld);
	}
	
	sl_unlock(&g_vars.slConnections, irql);

	nf_packet_free(pPacket);

    IoFreeIrp(irp);    // It's Ours. Free it up

	return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS udp_TdiStartInternalSend(PADDROBJ pAddr)
{
	PIRP				irp = NULL;
	PNF_PACKET			pPacket = NULL;
	KIRQL				irqld;
	PTRANSPORT_ADDRESS	pPacketAddr = NULL;
	PDEVICE_OBJECT		pLowerDevice = NULL;
			
	KdPrint((DPREFIX"udp_TdiStartInternalSend\n"));

	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	if (!pAddr)
	{
		return STATUS_INVALID_ADDRESS;
	}

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || pAddr->closed)
	{
		sl_unlock(&pAddr->lock, irqld);
		return STATUS_INVALID_ADDRESS;
	}

	pLowerDevice = pAddr->lowerDevice;

	pPacket = (PNF_PACKET)pAddr->sendPackets.Flink;
	if (!pPacket || (pPacket == (PNF_PACKET)&pAddr->sendPackets))
	{
		sl_unlock(&pAddr->lock, irqld);
		return STATUS_INVALID_ADDRESS;
	}
	
	RemoveEntryList(&pPacket->entry);

	if ((pPacket->dataSize == 0) || !pLowerDevice || pAddr->sendInProgress)
	{
		sl_unlock(&pAddr->lock, irqld);
		return STATUS_UNSUCCESSFUL;
	}

	pPacket->fileObject = (PFILE_OBJECT)pAddr->fileObject;

	irp = IoAllocateIrp(pLowerDevice->StackSize+2, FALSE);
	if (!irp)
	{
		sl_unlock(&pAddr->lock, irqld);
		return STATUS_UNSUCCESSFUL;
	}

	KdPrint((DPREFIX"udp_TdiStartInternalSend[%I64d]: TdiBuildSend\n", pAddr->id));
	
	pPacketAddr = (PTRANSPORT_ADDRESS)pPacket->remoteAddr;
	
	pPacketAddr->TAAddressCount = 1;
	
	pPacket->connInfo.RemoteAddressLength = sizeof(TRANSPORT_ADDRESS) - 4 + 
		((PTA_ADDRESS)pPacketAddr->Address)->AddressLength;

	pPacket->connInfo.RemoteAddress = pPacket->remoteAddr;

	TdiBuildSendDatagram(
			irp,							// IRP
			pLowerDevice,				// Device
			(PFILE_OBJECT)pAddr->fileObject, // File object
			udp_TdiInternalSendComplete,	// CompletionRoutine
			pPacket,						// Context
			pPacket->pMdl,					// MdlAddress
			pPacket->dataSize,				// SendLength
			&pPacket->connInfo				// SendDatagramInformation
		);

	if (pAddr->sendBytesInTdi >= TDI_SNDBUF)
	{
		pAddr->sendInProgress = TRUE;
	}

	pAddr->sendBytesInTdi += pPacket->dataSize;

	sl_unlock(&pAddr->lock, irqld);

	ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

	return IoCallDriver(pLowerDevice, irp);
}

