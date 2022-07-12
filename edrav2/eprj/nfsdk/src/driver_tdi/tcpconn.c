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
#include "gvars.h"
#include "mempools.h"
#include "tcprecv.h"
#include "tcpsend.h"
#include "packet.h"
#include "ctrlio.h"
#include "rules.h"

#ifdef _WPPTRACE
#include "tcpconn.tmh"
#endif

#define MAX_CONN_POOL_SIZE	0

static NTSTATUS tcp_setConnInfo(PNF_TCP_CONN_INFO pci, PCONNOBJ pConn)
{
	memset(pci, 0, sizeof(NF_TCP_CONN_INFO));

	pci->ip_family = *(short*)pConn->remoteAddr;

	pci->processId = (ULONG)pConn->processId;

	RtlCopyMemory(pci->localAddress, pConn->localAddr, sizeof(pci->localAddress));
	RtlCopyMemory(pci->remoteAddress, pConn->remoteAddr, sizeof(pci->remoteAddress));

	pci->direction = pConn->direction;
	pci->filteringFlag = pConn->filteringFlag;

	return STATUS_SUCCESS;
}


static PCONNOBJ tcp_conn_alloc(PIO_STACK_LOCATION irpSp, void * context)
{
	PCONNOBJ pConn = NULL;

	pConn = (PCONNOBJ)mp_alloc(sizeof(CONNOBJ));

	if (!pConn)
	{
		KdPrint((DPREFIX"tcp_conn_alloc failed\n"));
		return NULL;
	}

#if DBG
	g_vars.nConn++;
	KdPrint(("[counter] nConn=%d\n", g_vars.nConn));
#endif
	
	RtlZeroMemory(pConn, sizeof(CONNOBJ));

	pConn->fileObject = (HASH_ID)irpSp->FileObject;
	pConn->context = (HASH_ID)context;

	// Assign next unique identifier
	g_vars.nextConnId++;

	while ((g_vars.nextConnId == 0) || 
		ht_find_entry(g_vars.phtConnById, g_vars.nextConnId))
	{
		g_vars.nextConnId++;
	}
	
	pConn->id = g_vars.nextConnId;

	KdPrint((DPREFIX"tcp_conn_alloc[%I64d] fileObject=%I64u\n", pConn->id, (HASH_ID)irpSp->FileObject));

	sl_init(&pConn->lock);

	InitializeListHead(&pConn->pendedReceivedPackets);
	InitializeListHead(&pConn->receivedPackets);
	InitializeListHead(&pConn->pendedReceiveRequests);
	
	InitializeListHead(&pConn->pendedSendPackets);
	InitializeListHead(&pConn->sendPackets);
	InitializeListHead(&pConn->pendedSendRequests);

	InitializeListHead(&pConn->pendedDisconnectRequest);

	InitializeListHead(&pConn->pendedConnectRequest);
	
	return pConn;
}

void tcp_conn_free_queues(PCONNOBJ pConn, BOOLEAN disconnect)
{
	PNF_PACKET		pPacket;
	PLIST_ENTRY		pListEntry;
	PIRP			irp;
	KIRQL			irqld;
	
	KdPrint((DPREFIX"tcp_conn_free_queues[%I64d]\n", pConn->id));

	sl_lock(&pConn->lock, &irqld);
		
	while (!IsListEmpty(&pConn->receivedPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pConn->receivedPackets);
	        
		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan receivedPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
				pConn->id, pPacket->ioStatus, pPacket->dataSize ));
	        
		nf_packet_free(pPacket);
	}

	while (!IsListEmpty(&pConn->pendedReceivedPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pConn->pendedReceivedPackets);
	        
		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan pendedReceivedPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
				pConn->id, pPacket->ioStatus, pPacket->dataSize ));
	        
		nf_packet_free(pPacket);
	}

	while (!IsListEmpty(&pConn->sendPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pConn->sendPackets);
	        
		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan sendPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
				pConn->id, pPacket->ioStatus, pPacket->dataSize ));
	        
		nf_packet_free(pPacket);
	}

	while (!IsListEmpty(&pConn->pendedSendPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pConn->pendedSendPackets);
	        
		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan pendedSendPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
				pConn->id, pPacket->ioStatus, pPacket->dataSize ));
	        
		nf_packet_free(pPacket);
	}

	while (!IsListEmpty(&pConn->pendedReceiveRequests))
	{
		pListEntry = pConn->pendedReceiveRequests.Flink;
			
		irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		RemoveHeadList(&pConn->pendedReceiveRequests);
		InitializeListHead(&irp->Tail.Overlay.ListEntry);

		IoSetCancelRoutine(irp, NULL);

		irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
		irp->IoStatus.Information = 0;

		// Complete IRP from DPC
		ctrl_deferredCompleteIrp(irp, disconnect);

		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan pendedReceiveRequests IRP\n", pConn->id));
	}


	while (!IsListEmpty(&pConn->pendedSendRequests))
	{
		pListEntry = pConn->pendedSendRequests.Flink;
			
		irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		RemoveHeadList(&pConn->pendedSendRequests);
		InitializeListHead(&irp->Tail.Overlay.ListEntry);

		IoSetCancelRoutine(irp, NULL);

		irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
		irp->IoStatus.Information = 0;

		// Complete IRP from DPC
		ctrl_deferredCompleteIrp(irp, disconnect);

		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan pendedSendRequests IRP\n", pConn->id));
	}

	while (!IsListEmpty(&pConn->pendedConnectRequest))
	{
		pListEntry = pConn->pendedConnectRequest.Flink;
			
		irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

		RemoveHeadList(&pConn->pendedConnectRequest);
		InitializeListHead(&irp->Tail.Overlay.ListEntry);

		IoSetCancelRoutine(irp, NULL);

		irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
		irp->IoStatus.Information = 0;
		
		// Complete IRP from DPC
		ctrl_deferredCompleteIrp(irp, disconnect);

		KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan pendedConnectRequest IRP\n", pConn->id));
	}

	if (!disconnect)
	{
		while (!IsListEmpty(&pConn->pendedDisconnectRequest))
		{
			pListEntry = pConn->pendedDisconnectRequest.Flink;
				
			irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

			RemoveHeadList(&pConn->pendedDisconnectRequest);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
			irp->IoStatus.Information = 0;
			
			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, disconnect);

			KdPrint((DPREFIX"tcp_conn_free[%I64d]: Orphan pendedDisconnectRequest IRP\n", pConn->id));
		}
	}

	sl_unlock(&pConn->lock, irqld);
}

static void tcp_conn_free(PCONNOBJ pConn)
{
	KIRQL			irqld;
	
	KdPrint((DPREFIX"tcp_conn_free[%I64d]\n", pConn->id));

	sl_lock(&pConn->lock, &irqld);
	pConn->fileObject = 0;
	sl_unlock(&pConn->lock, irqld);

	tcp_conn_free_queues(pConn, FALSE);

	// Return memory back to the pool
	mp_free(pConn, MAX_CONN_POOL_SIZE);

#if DBG
	g_vars.nConn--;
	KdPrint(("[counter] nConn=%d\n", g_vars.nConn));
#endif
}

NTSTATUS tcp_TdiOpenConnectionComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context
   )
{
	PIO_STACK_LOCATION irpSp;
	PCONNOBJ pConn = NULL;
	KIRQL irql;

	KdPrint((DPREFIX"tcp_TdiOpenConnectionComplete\n"));

	if (NT_SUCCESS(irp->IoStatus.Status))
    {
		irpSp = IoGetCurrentIrpStackLocation(irp);
		ASSERT(irpSp);

		sl_lock(&g_vars.slConnections, &irql);

		pConn = tcp_conn_alloc(irpSp, context);

		if (pConn)
		{
			if (pDeviceObject == g_vars.deviceTCPFilter)
			{
				pConn->lowerDevice = g_vars.deviceTCP;
			} else
			if (pDeviceObject == g_vars.deviceTCP6Filter)
			{
				pConn->lowerDevice = g_vars.deviceTCP6;
			}

			// Add connection object to hashes 
			ht_add_entry(g_vars.phtConnByFileObject, (PHASH_TABLE_ENTRY)&pConn->fileObject);
			ht_add_entry(g_vars.phtConnByContext, (PHASH_TABLE_ENTRY)&pConn->context);
			ht_add_entry(g_vars.phtConnById, (PHASH_TABLE_ENTRY)&pConn->id);

			// Insert connection to list
			InsertTailList(&g_vars.lConnections, &pConn->entry);
		}

		sl_unlock(&g_vars.slConnections, irql);
    }

    //
    // Propogate the IRP pending flag
    //
    if (irp->PendingReturned)
    {
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS tcp_TdiOpenConnection(PIRP irp, PIO_STACK_LOCATION irpSp, PVOID context, PDEVICE_OBJECT pLowerDevice)
{
	KdPrint((DPREFIX"tcp_TdiOpenConnection\n"));

	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"tcp_TdiOpenConnection Not enough stack locations\n"));

		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

    IoCopyCurrentIrpStackLocationToNext(irp);
        
    IoSetCompletionRoutine(
            irp,
            tcp_TdiOpenConnectionComplete,
            context,
            TRUE,
            TRUE,
            TRUE
            );
        
    return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiCloseConnection(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql;

	KdPrint((DPREFIX"tcp_TdiCloseConnection fileObject=%I64u\n", (HASH_ID)irpSp->FileObject));

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		KdPrint((DPREFIX"tcp_TdiCloseConnection[%I64d]\n", pConn->id));

		// Remove connection from hashes
		ht_remove_entry(g_vars.phtConnByFileObject, pConn->fileObject);
		ht_remove_entry(g_vars.phtConnById, pConn->id);

		if (pConn->pAddr)
		{
			ht_remove_entry(g_vars.phtConnByContext, pConn->context);
			KdPrint((DPREFIX"tcp_TdiCloseConnection[%I64d] : TdiDisassociateAddress was not called\n", pConn->id));
		}

		// Remove connection from list
		RemoveEntryList(&pConn->entry);

		if (ctrl_attached())
		{
			NF_TCP_CONN_INFO ci;
			tcp_setConnInfo(&ci, pConn);		
			ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));
		}

		tcp_conn_free(pConn);
	}

	sl_unlock(&g_vars.slConnections, irql);

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiConnectComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context)
{
	PIO_STACK_LOCATION	irpSp;
	PIO_HOOK_CONTEXT	pHookContext = (PIO_HOOK_CONTEXT)context;
	PCONNOBJ			pConn;
    KIRQL               irql, irqld;
	NTSTATUS			status;

	KdPrint((DPREFIX"tcp_TdiConnectComplete\n"));

	if (!pHookContext)
		return STATUS_SUCCESS;

	pConn = (PCONNOBJ)pHookContext->new_context;

	irpSp = IoGetCurrentIrpStackLocation(irp);
	ASSERT(irpSp);

	if (ctrl_attached() && NT_SUCCESS(irp->IoStatus.Status))
	{
		NF_TCP_CONN_INFO ci;

		sl_lock(&g_vars.slConnections, &irql);

		tcp_setConnInfo(&ci, pConn);

		ctrl_queuePushEx(NF_TCP_CONNECTED, pConn->id, (char*)&ci, sizeof(ci));

		pConn->disconnectCalled = FALSE;
		pConn->disconnectEventCalled = FALSE;
		pConn->disconnectEventPending = FALSE;
		pConn->receiveInProgress = FALSE;
		pConn->recvBytesInTdi = 0;
		pConn->sendInProgress = FALSE;
		pConn->sendBytesInTdi = 0;
		pConn->userModeFilteringDisabled = FALSE;
		pConn->disableUserModeFiltering = FALSE;
		pConn->flags = 0;
		pConn->receiveEventCalled = FALSE;
		pConn->sendError = FALSE;

		sl_lock(&pConn->lock, &irqld);

		if (!IsListEmpty(&pConn->pendedDisconnectRequest))
		{
			PNF_PACKET pPacket;
		
			KdPrint((DPREFIX"tcp_TdiConnectComplete[%I64d] pended disconnect\n", pConn->id));

			pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
			if (pPacket)
			{
				pPacket->dataSize = 0;
				pPacket->ioStatus = STATUS_UNSUCCESSFUL;

				InsertTailList(&pConn->pendedSendPackets, &pPacket->entry);

				ctrl_queuePush(NF_TCP_SEND, pConn->id);
			}
		}

		sl_unlock(&pConn->lock, irqld);

		sl_unlock(&g_vars.slConnections, irql);
	}

	// Propogate the IRP pending flag
    if (irp->PendingReturned && (irp->CurrentLocation <= irp->StackCount))
    {
        IoMarkIrpPending(irp);
    }

	status = STATUS_SUCCESS;

	{   
        BOOLEAN mustCallCr = FALSE;   

		irp->CurrentLocation--;
		irp->Tail.Overlay.CurrentStackLocation--;

		irpSp = IoGetCurrentIrpStackLocation(irp);

		pDeviceObject = irpSp->DeviceObject;

		irpSp->CompletionRoutine = pHookContext->old_cr;
		irpSp->Context = pHookContext->old_context;
		irpSp->Control = pHookContext->old_control;
		irpSp->DeviceObject = pDeviceObject;

		irp->CurrentLocation++;
		irp->Tail.Overlay.CurrentStackLocation++;

        if (irp->Cancel) 
		{   
			if (pHookContext->old_control & SL_INVOKE_ON_CANCEL)   
                mustCallCr = TRUE;   
        } else 
		{   
            if (irp->IoStatus.Status >= STATUS_SUCCESS) 
			{   
                if (pHookContext->old_control & SL_INVOKE_ON_SUCCESS)   
	                 mustCallCr = TRUE;   
            } else 
			{   
                if (pHookContext->old_control & SL_INVOKE_ON_ERROR)   
                     mustCallCr = TRUE;   
            }   
        }   
   
		if (mustCallCr && pHookContext->old_cr)
		{
			status = pHookContext->old_cr(pDeviceObject, irp, pHookContext->old_context);
		}
	}

	free_np(pHookContext);

    return status;
}

BOOLEAN tcp_isProxy(ULONG processId)
{
	PCONNOBJ pConnIn = NULL;
	PCONNOBJ pConnOut = NULL;
	sockaddr_gen * pLocalAddrOut = NULL;
	sockaddr_gen * pRemoteAddrOut = NULL;
	sockaddr_gen * pLocalAddrIn = NULL;
	sockaddr_gen * pRemoteAddrIn = NULL;

	KdPrint((DPREFIX"tcp_isProxy %d\n", processId));

	pConnIn = (PCONNOBJ)g_vars.lConnections.Flink;
	
	while (pConnIn != (PCONNOBJ)&g_vars.lConnections)
	{
		// Try to find an inbound connection with the same processId
		if (pConnIn->processId == processId && pConnIn->direction == NF_D_IN)
		{
			pLocalAddrIn = (sockaddr_gen *)pConnIn->localAddr;
			pRemoteAddrIn = (sockaddr_gen *)pConnIn->remoteAddr;

			pConnOut = (PCONNOBJ)g_vars.lConnections.Flink;
			
			while (pConnOut != (PCONNOBJ)&g_vars.lConnections)
			{
				// Try to find an appropriate outgoing connection by local address
				if (pConnOut->direction == NF_D_OUT)
				{
					pLocalAddrOut = (sockaddr_gen *)pConnOut->localAddr;
					pRemoteAddrOut = (sockaddr_gen *)pConnOut->remoteAddr;

					if ((pLocalAddrOut->AddressIn.sin_family == pRemoteAddrIn->AddressIn.sin_family) &&
						(pLocalAddrOut->AddressIn.sin_port == pRemoteAddrIn->AddressIn.sin_port))
					{
						// If the connection was redirected from different process,
						// or the original remote address was changed,
						// the process that accepted a connection is a local proxy.
						if ((pConnOut->processId != pConnIn->processId) ||
							(pRemoteAddrOut->AddressIn.sin_port != pLocalAddrIn->AddressIn.sin_port))
						{
							KdPrint((DPREFIX"tcp_isServer %d - is a local proxy\n", processId));
							return TRUE;
						}
					}
				}

				pConnOut = (PCONNOBJ)pConnOut->entry.Flink;
			}
		}
		
		pConnIn = (PCONNOBJ)pConnIn->entry.Flink;
	}

	return FALSE;
}

void tcp_cancelTdiConnect(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"tcp_cancelTdiConnect\n"));

    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	//  Dequeue and complete the IRP.  

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		sl_lock(&pConn->lock, &irqld);

		KdPrint((DPREFIX"tcp_cancelTdiConnect[%I64d]\n", pConn->id));

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

NTSTATUS tcp_TdiConnect(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PTA_ADDRESS          pRemoteAddr = NULL;
	PTA_ADDRESS          pLocalAddr = NULL;
	PTDI_REQUEST_KERNEL  prk;
	PCONNOBJ			 pConn = NULL;
	PHASH_TABLE_ENTRY	 phte;
    KIRQL                irql, irqld;
    char                 NullAddr[TA_ADDRESS_MAX];
	PIO_HOOK_CONTEXT	 pHookContext = NULL;
	BOOLEAN				 validRemoteAddress = TRUE;

	KdPrint((DPREFIX"tcp_TdiConnect\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"tcp_TdiConnect: Not enough stack locations\n"));
/*	
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
*/
	}

	prk = (PTDI_REQUEST_KERNEL )&irpSp->Parameters;

    if (prk &&
        prk->RequestConnectionInformation &&
        prk->RequestConnectionInformation->RemoteAddress)
    {
	    pRemoteAddr = ((TRANSPORT_ADDRESS *)(prk->RequestConnectionInformation->RemoteAddress))->Address;
    } else
    {
        memset(NullAddr, 0, sizeof(NullAddr));
        pRemoteAddr = (PTA_ADDRESS)NullAddr;
        pRemoteAddr->AddressLength = TDI_ADDRESS_LENGTH_IP;
        pRemoteAddr->AddressType = TDI_ADDRESS_TYPE_IP;
    }

	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		if (pRemoteAddr->AddressLength > sizeof(pConn->remoteAddr)) 
		{
			KdPrint(("tcp_TdiConnect: address too long! (%u)\n", pRemoteAddr->AddressLength));
			memcpy(pConn->remoteAddr, (char*)pRemoteAddr + sizeof(USHORT), sizeof(pConn->remoteAddr));
			validRemoteAddress = FALSE;
		} else
		{
			memcpy(pConn->remoteAddr, (char*)pRemoteAddr + sizeof(USHORT), pRemoteAddr->AddressLength + sizeof(USHORT));
		}

		pConn->direction = NF_D_OUT;

		pConn->connected = TRUE;

		pConn->filteringFlag = rule_findByConn(pConn);
	}

	if (!pConn || !ctrl_attached())
	{
		KdPrint((DPREFIX"tcp_TdiConnect: PCONNOBJ not found\n"));

		sl_unlock(&g_vars.slConnections, irql);

		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	} else
	{
		if (validRemoteAddress &&
		    (pConn->filteringFlag & NF_INDICATE_CONNECT_REQUESTS) &&
			((pConn->filteringFlag & NF_DISABLE_REDIRECT_PROTECTION) || !tcp_isProxy(pConn->processId)))
		{
			NF_TCP_CONN_INFO ci;
		    PDRIVER_CANCEL	oldCancelRoutine;

			KdPrint((DPREFIX"tcp_TdiConnect[%I64d] pended\n", pConn->id));

			IoMarkIrpPending(irp);
					
			sl_lock(&pConn->lock, &irqld);
			InsertTailList(&pConn->pendedConnectRequest, &irp->Tail.Overlay.ListEntry);
			sl_unlock(&pConn->lock, irqld);

			oldCancelRoutine = IoSetCancelRoutine(irp, tcp_cancelTdiConnect);
			irp->IoStatus.Status = STATUS_PENDING;

			tcp_setConnInfo(&ci, pConn);

			if (NT_SUCCESS(ctrl_queuePushEx(NF_TCP_CONNECT_REQUEST, pConn->id, (char*)&ci, sizeof(ci))))
			{
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_PENDING;
			}
		
			KdPrint((DPREFIX"tcp_TdiConnect[%I64d] unpended\n", pConn->id));

			sl_lock(&pConn->lock, &irqld);
			RemoveEntryList(&irp->Tail.Overlay.ListEntry);				
			InitializeListHead(&irp->Tail.Overlay.ListEntry);
			sl_unlock(&pConn->lock, irqld);

			IoSetCancelRoutine(irp, oldCancelRoutine);
			irp->IoStatus.Status = STATUS_SUCCESS;
		}

		if (pConn->filteringFlag & NF_BLOCK)
		{
			sl_unlock(&g_vars.slConnections, irql);
		
			KdPrint((DPREFIX"tcp_TdiConnect[%I64d] blocked\n", pConn->id));

			irp->IoStatus.Status = STATUS_REMOTE_NOT_LISTENING;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_REMOTE_NOT_LISTENING;
		} 

		if ((pConn->filteringFlag & (NF_OFFLINE | NF_FILTER)) == (NF_OFFLINE | NF_FILTER))
		{
			// Complete connect request immediately for offline connection
			NF_TCP_CONN_INFO ci;

			KdPrint((DPREFIX"tcp_TdiConnect[%I64d] offline\n", pConn->id));

			tcp_setConnInfo(&ci, pConn);		

			ctrl_queuePushEx(NF_TCP_CONNECTED, pConn->id, (char*)&ci, sizeof(ci));

			sl_unlock(&g_vars.slConnections, irql);

			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = 0;
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_SUCCESS;
		}

		sl_unlock(&g_vars.slConnections, irql);
	}

	pHookContext = (PIO_HOOK_CONTEXT)malloc_np(sizeof(IO_HOOK_CONTEXT));
	if (!pHookContext)
	{
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pLowerDevice, irp);
	}

	pHookContext->old_cr = irpSp->CompletionRoutine;
	pHookContext->new_context = pConn;
	pHookContext->old_context = irpSp->Context;
	pHookContext->fileobj = irpSp->FileObject;
	pHookContext->old_control = irpSp->Control;

	IoSkipCurrentIrpStackLocation(irp);
//    IoCopyCurrentIrpStackLocationToNext(irp);
        
    IoSetCompletionRoutine(
            irp,
            tcp_TdiConnectComplete,
            pHookContext,
            TRUE,
            TRUE,
            TRUE
            );
        
    return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiAssociateAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context)
{
	PIO_STACK_LOCATION irpSp;
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
    PTDI_REQUEST_KERNEL_ASSOCIATE prka = NULL;
    PFILE_OBJECT pAddrFileObject = NULL;
	NTSTATUS status;
	KIRQL irql;

	KdPrint((DPREFIX"tcp_TdiAssociateAddressComplete\n"));

	if (NT_SUCCESS(irp->IoStatus.Status))
	{
		irpSp = IoGetCurrentIrpStackLocation(irp);
		ASSERT(irpSp);

		prka = (PTDI_REQUEST_KERNEL_ASSOCIATE)&irpSp->Parameters;

		if (prka)
		{
			status = ObReferenceObjectByHandle(
							prka->AddressHandle,          // Object Handle
							FILE_ANY_ACCESS,           // Desired Access
							NULL,                      // Object Type
							KernelMode,                // Processor mode
							(PVOID *)&pAddrFileObject,     // File Object pointer
							NULL                       // Object Handle information
					);
			if (!NT_SUCCESS(status))
			{
				pAddrFileObject = NULL;
			}
		}

		if (pAddrFileObject)
		{
			sl_lock(&g_vars.slConnections, &irql);
			
			phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);
			if (phte)
			{
				PADDROBJ pAddr;
				PHASH_TABLE_ENTRY phtea;
		
				pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);
						
				phtea = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)pAddrFileObject);
				if (phtea)
				{
					pAddr = (PADDROBJ)CONTAINING_RECORD(phtea, ADDROBJ, fileObject);

					pConn->pAddr = pAddr;
					
					pConn->processId = pAddr->processId;

					memcpy(pConn->localAddr, pAddr->localAddr, sizeof(pConn->localAddr));
				}

			}

			sl_unlock(&g_vars.slConnections, irql);

			ObDereferenceObject(pAddrFileObject);
		}
	}

    // Propogate IRP pending flag
    if (irp->PendingReturned)
    {
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS tcp_TdiAssociateAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	KdPrint((DPREFIX"tcp_TdiAssociateAddress\n"));

	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"tcp_TdiAssociateAddress Not enough stack locations\n"));
	
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

    IoCopyCurrentIrpStackLocationToNext(irp);
        
    IoSetCompletionRoutine(
            irp,
            tcp_TdiAssociateAddressComplete,
            irp,
            TRUE,
            TRUE,
            TRUE
            );
        
    return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiDisAssociateAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context)
{

    // Propogate IRP pending flag
    if (irp->PendingReturned)
    {
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS tcp_TdiDisAssociateAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;
	KEVENT event;
	LARGE_INTEGER li;

	KdPrint((DPREFIX"tcp_TdiDisAssociateAddress\n"));

	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"tcp_TdiDisAssociateAddress Not enough stack locations\n"));
	
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}


	sl_lock(&g_vars.slConnections, &irql);	

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);
	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		sl_lock(&pConn->lock, &irqld);

		pConn->pAddr = NULL;

		// Fixes an incompatibility with some antiviruses
		ht_remove_entry(g_vars.phtConnByContext, pConn->context);

		sl_unlock(&pConn->lock, irqld);
	}
	
	sl_unlock(&g_vars.slConnections, irql);

	if (ctrl_attached() && pConn)
	{
		KeInitializeEvent(&event, NotificationEvent, FALSE);
	
		sl_lock(&pConn->lock, &irqld);

		while (ctrl_attached() && pConn->receiveEventInProgress)
		{
			sl_unlock(&pConn->lock, irqld);

			{
				li.QuadPart = 100 * -10000;	// 0.1 sec
				KeWaitForSingleObject(&event, UserRequest, KernelMode, FALSE, &li);
			}

			sl_lock(&pConn->lock, &irqld);
		}

		sl_unlock(&pConn->lock, irqld);
	}

    IoCopyCurrentIrpStackLocationToNext(irp);
        
    IoSetCompletionRoutine(
            irp,
            tcp_TdiDisAssociateAddressComplete,
            irp,
            TRUE,
            TRUE,
            TRUE
            );
        
    return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiInternalDisconnectComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context)
{
	PCONNOBJ	pConn = (PCONNOBJ)context;
	KIRQL		irql;
	BOOLEAN		disconnected = FALSE;

	KdPrint((DPREFIX"tcp_TdiInternalDisconnectComplete\n"));

	IoFreeIrp(irp);    // It's Ours. Free it up

	if (!pConn)
	{
		KdPrint((DPREFIX"tcp_TdiInternalDisconnectComplete pConn==NULL\n"));
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

	sl_lock(&pConn->lock, &irql);

	pConn->disconnectCalled = TRUE;

	// Complete TDI_SEND request with TDI_SEND_AND_DISCONNECT flag if the socket is fully disconnected
	if ((pConn->flags & TDI_SEND_AND_DISCONNECT) &&
		pConn->disconnectEventCalled)
	{
		PLIST_ENTRY		pListEntry;
	
		while (!IsListEmpty(&pConn->pendedSendRequests))
		{
			pListEntry = pConn->pendedSendRequests.Flink;
				
			irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

			RemoveHeadList(&pConn->pendedSendRequests);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = STATUS_SUCCESS;

			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, TRUE);

			KdPrint((DPREFIX"tcp_TdiInternalDisconnectComplete[%I64d] pended TDI_SEND completed\n", pConn->id));
		}
		
		disconnected = TRUE;
	} else
	if (pConn->disconnectEventCalled)
	{
		disconnected = TRUE;
	}

	sl_unlock(&pConn->lock, irql);

	// Notify user mode if needed
	if (disconnected)
	{
		sl_lock(&g_vars.slConnections, &irql);
		
		if (ctrl_attached())
		{
			NF_TCP_CONN_INFO ci;
			tcp_setConnInfo(&ci, pConn);
			ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));
		}

		pConn->connected = FALSE;		

		tcp_conn_free_queues(pConn, FALSE);

		sl_unlock(&g_vars.slConnections, irql);
	}

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS tcp_TdiStartInternalDisconnect(PCONNOBJ pConn, KIRQL irql, PDEVICE_OBJECT pLowerDevice)
{
//	PIRP	irp = NULL;
	PFILE_OBJECT fileObject = NULL;

	KdPrint((DPREFIX"tcp_TdiStartInternalDisconnect[%I64d]\n", pConn->id));

	ASSERT( KeGetCurrentIrql() <= DISPATCH_LEVEL );

	fileObject = (PFILE_OBJECT)pConn->fileObject;

	if (!fileObject || !pConn->pAddr || 
		pConn->disconnectCalled || 
		(pConn->disconnectEventCalled && !(pConn->flags & TDI_SEND_AND_DISCONNECT)))
	{
		ASSERT(0);
		sl_unlock(&pConn->lock, irql);
		return STATUS_INVALID_CONNECTION;
	}

	if (!pConn->disconnectEventPending)
	{
		pConn->disconnectEventFlags = TDI_DISCONNECT_ABORT;
		pConn->disconnectEventPending = TRUE;
	}
/*
	irp = IoAllocateIrp(pLowerDevice->StackSize+2, FALSE);
	if (!irp)
	{
		ASSERT(0);
		KdPrint((DPREFIX"tcp_TdiStartInternalDisconnect[%I64d]: IoAllocateIrp failed\n", pConn->id));
		tcp_callTdiDisconnectEventHandler(pConn, irql);	
		return STATUS_UNSUCCESSFUL;
	}
		
	TdiBuildDisconnect(
			irp,							// IRP
			pLowerDevice,				// Device
			fileObject,					// File object
			tcp_TdiInternalDisconnectComplete,	// CompletionRoutine
			pConn,							// Context
			NULL,							// Timeout
			(pConn->flags & TDI_SEND_AND_DISCONNECT)? 
				TDI_DISCONNECT_RELEASE : TDI_DISCONNECT_ABORT,			// Flags
			NULL,							// RequestConnectionInfo 
			NULL							// ReturnConnectionInfo 
		);
*/
	if (!(pConn->flags & TDI_SEND_AND_DISCONNECT))
	{
		tcp_callTdiDisconnectEventHandler(pConn, irql);	
	} else
	{
		sl_unlock(&pConn->lock, irql);
	}

	return STATUS_SUCCESS;

//	return IoCallDriver(pLowerDevice, irp);
}


NTSTATUS tcp_TdiDisconnectComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context)
{
	PCONNOBJ	pConn = (PCONNOBJ)context;

	if (pConn)
	{
		PTDI_REQUEST_KERNEL_DISCONNECT prk;
		PIO_STACK_LOCATION irpSp;
		PHASH_TABLE_ENTRY	phte;
		KIRQL	irql, irqld;

		KdPrint((DPREFIX"tcp_TdiDisconnectComplete[%I64d]\n", pConn->id));

		irpSp = IoGetCurrentIrpStackLocation(irp);
		ASSERT(irpSp);

		prk = (PTDI_REQUEST_KERNEL_DISCONNECT)&irpSp->Parameters;

		sl_lock(&g_vars.slConnections, &irql);

		phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

		if (phte)
		{
			pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

			if (pConn->disconnectEventCalled || (prk->RequestFlags & TDI_DISCONNECT_ABORT))
			{
				if (ctrl_attached())
				{
					NF_TCP_CONN_INFO ci;
					tcp_setConnInfo(&ci, pConn);		
					ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));
				}

				pConn->connected = FALSE;

				tcp_conn_free_queues(pConn, FALSE);
			} else
			{
				if (pConn->recvBytesInTdi == 0)
					pConn->recvBytesInTdi = 1;

				tcp_startDeferredProc(pConn, DQC_TCP_INTERNAL_RECEIVE);
			}
		}

		sl_unlock(&g_vars.slConnections, irql);
	}

	// Propogate IRP pending flag
    if (irp->PendingReturned)
    {
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS tcp_callTdiDisconnect(PCONNOBJ pConn, KIRQL irql)
{
	PDEVICE_OBJECT pLowerDevice;

	KdPrint((DPREFIX"tcp_callTdiDisconnect[%I64d]\n", pConn->id));

	if (!pConn->fileObject || !pConn->pAddr)
	{
		ASSERT(0);
		sl_unlock(&pConn->lock, irql);
		return STATUS_INVALID_CONNECTION;
	}

	pLowerDevice = pConn->lowerDevice;
	if (!pLowerDevice)
	{
		ASSERT(0);
		sl_unlock(&pConn->lock, irql);
		return STATUS_INVALID_CONNECTION;
	}

	if (!IsListEmpty(&pConn->pendedDisconnectRequest))
	{
		PIRP irp;
		PLIST_ENTRY pEntry;
		NTSTATUS status;

		pConn->disconnectCalled = TRUE;
		
		pEntry = RemoveHeadList(&pConn->pendedDisconnectRequest);
		
		irp = (PIRP)CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
		InitializeListHead(&irp->Tail.Overlay.ListEntry);

		IoSetCancelRoutine(irp, NULL);

		sl_unlock(&pConn->lock, irql);

		if (irp->CurrentLocation == 1)
		{
			KdPrint((DPREFIX"tcp_callTdiDisconnect Not enough stack locations\n"));
		
			// Call event handler anyway
			tcp_TdiDisconnectComplete(pLowerDevice, irp, pConn);
		
			IoSkipCurrentIrpStackLocation(irp);

			status = IoCallDriver(pLowerDevice, irp);
		} else
		{
			IoCopyCurrentIrpStackLocationToNext(irp);

			IoSetCompletionRoutine(
					irp,
					tcp_TdiDisconnectComplete,
					pConn,
					TRUE,
					TRUE,
					TRUE
					);
		        
			status = IoCallDriver(pLowerDevice, irp);
		}

		KdPrint((DPREFIX"tcp_callTdiDisconnect[%I64d] IoCallDriver called\n", pConn->id));
	} else
	{
		tcp_TdiStartInternalDisconnect(pConn, irql, pLowerDevice);
	}

	return STATUS_SUCCESS;
}


void tcp_cancelTdiDisconnect(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"tcp_cancelTdiDisconnect\n"));

    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	//  Dequeue and complete the IRP.  

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		sl_lock(&pConn->lock, &irqld);

		KdPrint((DPREFIX"tcp_cancelTdiDisconnect[%I64d]\n", pConn->id));

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

NTSTATUS tcp_TdiDisconnect(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PCONNOBJ			pConn = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status = STATUS_SUCCESS;
    PDRIVER_CANCEL		oldCancelRoutine;
	PNF_PACKET			pPacket;
	PTDI_REQUEST_KERNEL_DISCONNECT prk;
	KIRQL				irql, irqld;
    
	KdPrint((DPREFIX"tcp_TdiDisconnect\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	prk = (PTDI_REQUEST_KERNEL_DISCONNECT)&irpSp->Parameters;

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

		if ((pConn->filteringFlag & NF_FILTER) &&
			!pConn->userModeFilteringDisabled)
		{
			if (prk->RequestFlags & TDI_DISCONNECT_ABORT)
			{
				NF_TCP_CONN_INFO ci;

				KdPrint((DPREFIX"tcp_TdiDisconnect[%I64d] connection aborted\n", pConn->id));

				tcp_setConnInfo(&ci, pConn);		
				ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));

				pConn->connected = FALSE;
				pConn->disconnectCalled = TRUE;

				tcp_conn_free_queues(pConn, FALSE);
			} else
			{
				// Queue disconnect request and indicate it to user mode API as a zero length send

				IoMarkIrpPending(irp);
				
				sl_lock(&pConn->lock, &irqld);
				InsertTailList(&pConn->pendedDisconnectRequest,	&irp->Tail.Overlay.ListEntry);
				sl_unlock(&pConn->lock, irqld);

				oldCancelRoutine = IoSetCancelRoutine(irp, tcp_cancelTdiDisconnect);
				irp->IoStatus.Status = STATUS_PENDING;

				status = STATUS_PENDING;

				pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
				if (pPacket)
				{
					pPacket->dataSize = 0;
					pPacket->ioStatus = STATUS_UNSUCCESSFUL;

					sl_lock(&pConn->lock, &irqld);
					InsertTailList(&pConn->pendedSendPackets, &pPacket->entry);
					sl_unlock(&pConn->lock, irqld);

					KdPrint((DPREFIX"tcp_TdiDisconnect[%I64d] pended, flags=%I64d\n", pConn->id, prk->RequestFlags));

					if (!NT_SUCCESS(ctrl_queuePush(NF_TCP_SEND, pConn->id)))
					{

						KdPrint((DPREFIX"tcp_TdiDisconnect[%I64d] unpended\n", pConn->id));

						sl_lock(&pConn->lock, &irqld);
						RemoveEntryList(&pPacket->entry);
						RemoveEntryList(&irp->Tail.Overlay.ListEntry);				
						InitializeListHead(&irp->Tail.Overlay.ListEntry);
						sl_unlock(&pConn->lock, irqld);

						nf_packet_free(pPacket);

						IoSetCancelRoutine(irp, oldCancelRoutine);
						irp->IoStatus.Status = STATUS_SUCCESS;

						status = STATUS_SUCCESS;
					}
				} else
				{
					KdPrint((DPREFIX"tcp_TdiDisconnect[%I64d] not pended\n", pConn->id));

					sl_lock(&pConn->lock, &irqld);
					RemoveEntryList(&irp->Tail.Overlay.ListEntry);				
					InitializeListHead(&irp->Tail.Overlay.ListEntry);
					sl_unlock(&pConn->lock, irqld);

					IoSetCancelRoutine(irp, oldCancelRoutine);
					irp->IoStatus.Status = STATUS_SUCCESS;

					status = STATUS_SUCCESS;
				}
			}
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (status == STATUS_PENDING)
		return status;

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiListen(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	KdPrint((DPREFIX"tcp_TdiListen\n"));

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS tcp_TdiAcceptComplete(
        PDEVICE_OBJECT    pDeviceObject,
        PIRP              irp,
        void              *context
       )
{
	PCONNOBJ	pConn = (PCONNOBJ)context;
	KIRQL		irql, irqld;

	if (!pConn)
	{
		KdPrint((DPREFIX"tcp_TdiAcceptComplete pConn==NULL\n"));
		return STATUS_SUCCESS;
	}

	KdPrint((DPREFIX"tcp_TdiAcceptComplete[%I64d] status=%x\n", 
		pConn->id, irp->IoStatus.Status));

	if (ctrl_attached() && NT_SUCCESS(irp->IoStatus.Status))
	{
		sl_lock(&g_vars.slConnections, &irql);

		sl_lock(&pConn->lock, &irqld);

		if (!IsListEmpty(&pConn->pendedDisconnectRequest))
		{
			PNF_PACKET pPacket;
		
			KdPrint((DPREFIX"tcp_TdiAcceptComplete[%I64d] pended disconnect\n", pConn->id));

			pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
			if (pPacket)
			{
				pPacket->dataSize = 0;
				pPacket->ioStatus = STATUS_UNSUCCESSFUL;

				InsertTailList(&pConn->pendedSendPackets, &pPacket->entry);

				ctrl_queuePush(NF_TCP_SEND, pConn->id);
			}
		}

		sl_unlock(&pConn->lock, irqld);

		sl_unlock(&g_vars.slConnections, irql);
	} else
	{
		sl_lock(&g_vars.slConnections, &irql);
		pConn->filteringFlag = NF_ALLOW;
		pConn->connected = FALSE;
		sl_unlock(&g_vars.slConnections, irql);
	}

	// Propogate IRP pending flag
    if (irp->PendingReturned)
    {
        IoMarkIrpPending(irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS tcp_TdiConnectEventHandler(
   IN PVOID TdiEventContext,     
   IN LONG RemoteAddressLength,
   IN PVOID RemoteAddress,
   IN LONG UserDataLength,       
   IN PVOID UserData,            
   IN LONG OptionsLength,
   IN PVOID Options,
   OUT CONNECTION_CONTEXT *ConnectionContext,
   OUT PIRP *hAcceptIrp
   )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_INSUFFICIENT_RESOURCES;
	PTA_ADDRESS pRemoteAddr = NULL;
    char		NullAddr[TA_ADDRESS_MAX];
	CONNOBJ		fakeConn;
	KIRQL		irql, irqld;
	PTDI_IND_CONNECT    ev_connect = NULL;           
    PVOID               ev_connect_context = NULL;   

	KdPrint((DPREFIX"tcp_TdiConnectEventHandler\n"));

	sl_lock(&g_vars.slConnections, &irql);

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiConnectEventHandler: pAddr == NULL\n"));
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || !pAddr->ev_connect || !pAddr->ev_connect_context)
	{
		KdPrint((DPREFIX"tcp_TdiConnectEventHandler: !pAddr->fileObject\n"));
		sl_unlock(&pAddr->lock, irqld);
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ev_connect = pAddr->ev_connect;
	ev_connect_context = pAddr->ev_connect_context;

	sl_unlock(&pAddr->lock, irqld);
	sl_unlock(&g_vars.slConnections, irql);

    if (RemoteAddress)
    {
        pRemoteAddr = ((TRANSPORT_ADDRESS *)RemoteAddress)->Address;
    }
    else
    {
        memset(NullAddr, 0, sizeof(NullAddr));
        pRemoteAddr = (PTA_ADDRESS)NullAddr;
        pRemoteAddr->AddressLength = TDI_ADDRESS_LENGTH_IP;
        pRemoteAddr->AddressType = TDI_ADDRESS_TYPE_IP;
    }

	memset(&fakeConn, 0, sizeof(fakeConn));

	if (ctrl_attached())
	{
		fakeConn.pAddr = pAddr;
		
		fakeConn.processId = pAddr->processId;

		memcpy(fakeConn.localAddr, pAddr->localAddr, sizeof(fakeConn.localAddr));

		if (pRemoteAddr->AddressLength <= sizeof(fakeConn.remoteAddr)) 
		{
			memcpy(fakeConn.remoteAddr, (char*)pRemoteAddr + sizeof(USHORT), pRemoteAddr->AddressLength + sizeof(USHORT));
		}

		fakeConn.direction = NF_D_IN;

		fakeConn.filteringFlag = rule_findByConn(&fakeConn);

		if (fakeConn.filteringFlag == NF_BLOCK)
		{
			NF_TCP_CONN_INFO ci;

			tcp_setConnInfo(&ci, &fakeConn);		
			
			ctrl_queuePushEx(NF_TCP_CLOSED, 0, (char*)&ci, sizeof(ci));

			return STATUS_CONNECTION_REFUSED;
		}
	} else
	{
		fakeConn.filteringFlag = NF_ALLOW;
	}


	if (ev_connect)
	{
		status = ev_connect(
			ev_connect_context,
			RemoteAddressLength,   
			RemoteAddress,        
			UserDataLength,          
			UserData,                
			OptionsLength,             
			Options,            
			ConnectionContext,  
			hAcceptIrp
			);

		KdPrint((DPREFIX"tcp_TdiConnectEventHandler: ev_connect status=%x\n", status));
	
        if ((status == STATUS_MORE_PROCESSING_REQUIRED) && hAcceptIrp && *hAcceptIrp)
        {
            PIRP	irp;
			PIO_STACK_LOCATION	irpSp;
			PHASH_TABLE_ENTRY	phte;
			PCONNOBJ			pConn = NULL;
			KIRQL	irql;
           
            KdPrint((DPREFIX"tcp_TdiConnectEventHandler: accepting connection\n"));
            
            irp = *hAcceptIrp;
            
			irpSp = IoGetCurrentIrpStackLocation(irp);
			ASSERT(irpSp);

			sl_lock(&g_vars.slConnections, &irql);

			phte = ht_find_entry(g_vars.phtConnByFileObject, (HASH_ID)irpSp->FileObject);
			if (!phte)
			{
	            KdPrint((DPREFIX"tcp_TdiConnectEventHandler: connection object not found\n"));
				sl_unlock(&g_vars.slConnections, irql);
				return status;
			}

			pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, fileObject);

			// Server side sockets are often remain opened, and the server
			// simply disconnects sockets and use them for accepting new connections.
			if (pConn->connected)
			{
				// Notify user mode API that the socket is closed 
				NF_TCP_CONN_INFO ci;
				tcp_setConnInfo(&ci, pConn);		
				ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));

				tcp_conn_free_queues(pConn, TRUE);

				ht_remove_entry(g_vars.phtConnById, pConn->id);

				// Assign next unique identifier
				g_vars.nextConnId++;

				while ((g_vars.nextConnId == 0) || 
					ht_find_entry(g_vars.phtConnById, g_vars.nextConnId))
				{
					g_vars.nextConnId++;
				}
				
				pConn->id = g_vars.nextConnId;

				ht_add_entry(g_vars.phtConnById, (PHASH_TABLE_ENTRY)&pConn->id);
			}

			pConn->disconnectCalled = FALSE;
			pConn->disconnectEventCalled = FALSE;
			pConn->disconnectEventPending = FALSE;
			pConn->receiveInProgress = FALSE;
			pConn->recvBytesInTdi = 0;
			pConn->sendInProgress = FALSE;
			pConn->sendBytesInTdi = 0;
			pConn->userModeFilteringDisabled = FALSE;
			pConn->disableUserModeFiltering = FALSE;
			pConn->flags = 0;
			pConn->receiveEventCalled = FALSE;
			pConn->sendError = FALSE;

			if (pRemoteAddr->AddressLength > sizeof(pConn->remoteAddr)) 
			{
				KdPrint((DPREFIX"tcp_TdiConnectEventHandler: address too long! (%u)\n", pRemoteAddr->AddressLength));
			} else
			{
				memcpy(pConn->remoteAddr, (char*)pRemoteAddr + sizeof(USHORT), pRemoteAddr->AddressLength + sizeof(USHORT));
			}

			pConn->direction = NF_D_IN;

			pConn->filteringFlag = fakeConn.filteringFlag;

			pConn->connected = TRUE;

			{
				NF_TCP_CONN_INFO ci;
				tcp_setConnInfo(&ci, pConn);		
				ctrl_queuePushEx(NF_TCP_CONNECTED, pConn->id, (char*)&ci, sizeof(ci));
			}

			sl_unlock(&g_vars.slConnections, irql);

			if (irp->CurrentLocation == 1)
			{
				KdPrint((DPREFIX"tcp_TdiConnectEventHandler: Not enough stack locations\n"));
				return status;
			}
            
            IoCopyCurrentIrpStackLocationToNext(irp);
            
            IoSetCompletionRoutine(
                irp,
                tcp_TdiAcceptComplete,
                pConn,
                TRUE,
                TRUE,
                TRUE
                );
            
            // IoSetNextIrpStackLocation
            irp->CurrentLocation--;
            irp->Tail.Overlay.CurrentStackLocation--;            
		}		
	}

	return status;
}

NTSTATUS tcp_callTdiDisconnectEventHandler(PCONNOBJ pConn, KIRQL irql)
{
    PTDI_IND_DISCONNECT     ev_disconnect = NULL;    
	PVOID                   ev_disconnect_context = NULL;
	CONNECTION_CONTEXT		connContext = NULL;
	ULONG		disconnectEventFlags;
	PADDROBJ	pAddr;
	NTSTATUS	status = STATUS_SUCCESS;

	// Special handling for TDI_SEND with TDI_SEND_AND_DISCONNECT flag
	if (pConn->flags & TDI_SEND_AND_DISCONNECT)
	{
		BOOLEAN disconnected = FALSE;

		KdPrint((DPREFIX"tcp_callTdiDisconnectEventHandler[%I64d] TDI_SEND_AND_DISCONNECT\n", pConn->id));
		
		pConn->disconnectEventCalled = TRUE;

		// Complete TDI_SEND request with TDI_SEND_AND_DISCONNECT flag if the socket is fully disconnected
		if (pConn->disconnectCalled)
		{
			PLIST_ENTRY		pListEntry;
			PIRP			irp;
		
			while (!IsListEmpty(&pConn->pendedSendRequests))
			{
				pListEntry = pConn->pendedSendRequests.Flink;
					
				irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

				RemoveHeadList(&pConn->pendedSendRequests);
				InitializeListHead(&irp->Tail.Overlay.ListEntry);

				IoSetCancelRoutine(irp, NULL);

				irp->IoStatus.Status = STATUS_SUCCESS;

				// Complete IRP from DPC
				ctrl_deferredCompleteIrp(irp, TRUE);

				KdPrint((DPREFIX"tcp_callTdiDisconnectEventHandler[%I64d] pended TDI_SEND completed\n", pConn->id));
			}

			disconnected = TRUE;
		}

		sl_unlock(&pConn->lock, irql);

		// Notify user mode if needed
		if (disconnected)
		{
			sl_lock(&g_vars.slConnections, &irql);
			
			if (ctrl_attached())
			{
				NF_TCP_CONN_INFO ci;
				tcp_setConnInfo(&ci, pConn);
				ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));
			}

			pConn->connected = FALSE;		

			tcp_conn_free_queues(pConn, FALSE);

			sl_unlock(&g_vars.slConnections, irql);
		}
	
		// TdiDisconnectEventHandler is not called in this case
		return status;
	}

	if (!pConn->disconnectEventPending || pConn->disconnectEventCalled)
	{
		KdPrint((DPREFIX"tcp_callTdiDisconnectEventHandler[%I64d] bypass\n", pConn->id));
		sl_unlock(&pConn->lock, irql);
		return status;
	}

	pConn->disconnectEventPending = FALSE;

	pAddr = pConn->pAddr;

	if (pAddr && pAddr->ev_disconnect)
	{
		ev_disconnect = pAddr->ev_disconnect;
		ev_disconnect_context = pAddr->ev_disconnect_context;
		connContext = (CONNECTION_CONTEXT)pConn->context;
		disconnectEventFlags = pConn->disconnectEventFlags;

		pConn->disconnectEventCalled = TRUE;

		pConn->receiveEventInProgress = TRUE;

		sl_unlock(&pConn->lock, irql);

		status = ev_disconnect(
						   ev_disconnect_context,
						   connContext,
						   0,
						   NULL,
						   0,
						   NULL,
						   disconnectEventFlags
						 );					

		KdPrint((DPREFIX"tcp_callTdiDisconnectEventHandler pAddr->ev_disconnect[%I64d] flags=%x status=%x\n", 
			pConn->id, disconnectEventFlags, status));


		sl_lock(&pConn->lock, &irql);
		pConn->receiveEventInProgress = FALSE;
		sl_unlock(&pConn->lock, irql);
	} else
	{
		sl_unlock(&pConn->lock, irql);
	}

	// Notify user mode if needed
	if (pConn->disconnectCalled || (disconnectEventFlags & TDI_DISCONNECT_ABORT))
	{
		sl_lock(&g_vars.slConnections, &irql);
		
		if (ctrl_attached())
		{
			NF_TCP_CONN_INFO ci;
			tcp_setConnInfo(&ci, pConn);		
			ctrl_queuePushEx(NF_TCP_CLOSED, pConn->id, (char*)&ci, sizeof(ci));
		}

		pConn->connected = FALSE;		

		tcp_conn_free_queues(pConn, FALSE);

		sl_unlock(&g_vars.slConnections, irql);
	}

	return status;
}

NTSTATUS tcp_TdiDisconnectEventHandler(
       IN PVOID TdiEventContext,
       IN CONNECTION_CONTEXT ConnectionContext,
       IN LONG DisconnectDataLength,
       IN PVOID DisconnectData,
       IN LONG DisconnectInformationLength,
       IN PVOID DisconnectInformation,
       IN ULONG DisconnectFlags
      )
{
	PADDROBJ	pAddr = (PADDROBJ)TdiEventContext;
	NTSTATUS	status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp;
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;
	PNF_PACKET pPacket;
	BOOLEAN	callEvent = TRUE;
	PTDI_IND_DISCONNECT    ev_disconnect = NULL;           
    PVOID               ev_disconnect_context = NULL;    

	KdPrint((DPREFIX"tcp_TdiDisconnectEventHandler\n"));

	sl_lock(&g_vars.slConnections, &irql);

	if (!pAddr)
	{
		KdPrint((DPREFIX"tcp_TdiDisconnectEventHandler: pAddr == NULL\n"));
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_SUCCESS;
	}

	sl_lock(&pAddr->lock, &irqld);

	if (!pAddr->fileObject || !pAddr->ev_disconnect || !pAddr->ev_disconnect_context)
	{
		KdPrint((DPREFIX"tcp_TdiDisconnectEventHandler: !pAddr->fileObject\n"));
		sl_unlock(&pAddr->lock, irqld);
		sl_unlock(&g_vars.slConnections, irql);
		return STATUS_SUCCESS;
	}

	ev_disconnect = pAddr->ev_disconnect;
	ev_disconnect_context = pAddr->ev_disconnect_context;

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

			KdPrint((DPREFIX"tcp_TdiDisconnectEventHandler [%I64d]\n", pConn->id));
	
			if ((pConn->filteringFlag & NF_FILTER) &&
				!pConn->userModeFilteringDisabled)
			{
				// Queue disconnect event as a zero length receive

				callEvent = FALSE;
		
				pConn->disconnectEventPending = TRUE;
				pConn->disconnectEventFlags = DisconnectFlags;

				pPacket = nf_packet_alloc(NF_TCP_PACKET_BUF_SIZE);
				if (!pPacket)
				{
					status = STATUS_DATA_NOT_ACCEPTED;
				} else
				{
					pPacket->dataSize = 0;
					pPacket->ioStatus = STATUS_UNSUCCESSFUL;
					
					if (pConn->disableUserModeFiltering)
					{
						InsertTailList(
								&pConn->receivedPackets,
								&pPacket->entry
							   );
		
						tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);
					} else
					{
						InsertTailList(&pConn->pendedReceivedPackets, &pPacket->entry);

						if (!NT_SUCCESS(ctrl_queuePush(NF_TCP_RECEIVE, pConn->id)))
						{
							RemoveEntryList(&pPacket->entry);

							InsertTailList(
									&pConn->receivedPackets,
									&pPacket->entry
								   );

							// Indicate received data
							tcp_startDeferredProc(pConn, DQC_TCP_INDICATE_RECEIVE);
						}
					}
					KdPrint((DPREFIX"tcp_TdiDisconnectEventHandler[%I64d]: disconnect packet sent to queue\n", pConn->id));
				}
			}

			sl_unlock(&pConn->lock, irqld);
		}

		sl_unlock(&g_vars.slConnections, irql);
	}

	if (callEvent && ev_disconnect)
	{
		status = ev_disconnect(
				ev_disconnect_context,
				ConnectionContext,   
				DisconnectDataLength,
				DisconnectData,
				DisconnectInformationLength,
				DisconnectInformation,
				DisconnectFlags
				);
	}

	return status;
}

void tcp_startDeferredProc(PCONNOBJ pConn, int code)
{
	KIRQL irql;
	PDPC_QUEUE_ENTRY pEntry;

	pEntry = (PDPC_QUEUE_ENTRY)mp_alloc(sizeof(DPC_QUEUE_ENTRY));
	if (!pEntry)
		return;

	pEntry->id = pConn->id;
	pEntry->code = code;
	pEntry->pObj = pConn;

	sl_lock(&g_vars.slTcpQueue, &irql);
	InsertTailList(&g_vars.tcpQueue, &pEntry->entry);
	sl_unlock(&g_vars.slTcpQueue, irql);

	KeInsertQueueDpc(&g_vars.tcpDpc, NULL, NULL);
}

VOID tcp_procDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
{
	ctrl_queuePush(NF_TCP_REINJECT, 0);
}

VOID tcp_procReinject()
{
	KIRQL irql;
	PDPC_QUEUE_ENTRY pEntry;
	PCONNOBJ pConn = NULL;
	PHASH_TABLE_ENTRY phte;
	NTSTATUS status = STATUS_SUCCESS;

	for (;;)
	{
		sl_lock(&g_vars.slTcpQueue, &irql);

		if (IsListEmpty(&g_vars.tcpQueue))
		{
			sl_unlock(&g_vars.slTcpQueue, irql);
			return;
		}

		pEntry = (PDPC_QUEUE_ENTRY)RemoveHeadList(&g_vars.tcpQueue);

		sl_unlock(&g_vars.slTcpQueue, irql);

		sl_lock(&g_vars.slConnections, &irql);
		
		phte = ht_find_entry(g_vars.phtConnById, pEntry->id);
		if (phte)
		{
			pConn = (PCONNOBJ)CONTAINING_RECORD(phte, CONNOBJ, id);
		} else
		{
			pConn = NULL;
		}
	
		sl_unlock(&g_vars.slConnections, irql);

		if (pConn)
		{
			switch (pEntry->code)
			{
			case DQC_TCP_INDICATE_RECEIVE:
				status = tcp_indicateReceivedPackets(pConn, pEntry->id);
				break;

			case DQC_TCP_INTERNAL_RECEIVE:
				status = tcp_TdiStartInternalReceive(pConn, pEntry->id);
				break;

			case DQC_TCP_SEND:
				status = tcp_TdiStartInternalSend(pConn, pEntry->id);
				break;
			}
		}

		KdPrint((DPREFIX"tcp_procReinject [%d] status=%x\n", pEntry->code, status));

		mp_free(pEntry, 100);
	}
}
