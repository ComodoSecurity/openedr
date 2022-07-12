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
#include "addr.h"
#include "gvars.h"
#include "mempools.h"
#include "tcpconn.h"
#include "tcprecv.h"
#include "tcpsend.h"
#include "udprecv.h"
#include "udpsend.h"
#include "packet.h"
#include "ctrlio.h"
#include "devctrl.h"

#ifdef _WPPTRACE
#include "addr.tmh"
#endif

#define MAX_ADDR_POOL_SIZE	0

static NTSTATUS addr_setConnInfo(PNF_UDP_CONN_INFO pci, PADDROBJ pAddr)
{
	memset(pci, 0, sizeof(NF_UDP_CONN_INFO));

	pci->processId = (ULONG)pAddr->processId;
	pci->ip_family = *(short*)pAddr->localAddr;

	RtlCopyMemory(pci->localAddress, pAddr->localAddr, sizeof(pci->localAddress));

	return STATUS_SUCCESS;
}


static PADDROBJ addr_alloc(PIO_STACK_LOCATION irpSp, ULONG processId, int protocol, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ pAddr;

	pAddr = (PADDROBJ)mp_alloc(sizeof(ADDROBJ));

	if (!pAddr)
	{
		return NULL;
	}

#if DBG
	g_vars.nAddr++;
	KdPrint(("[counter] nAddr=%d\n", g_vars.nAddr));
#endif

	RtlZeroMemory(pAddr, sizeof(ADDROBJ));

	pAddr->lowerDevice = pLowerDevice;

	pAddr->fileObject = (HASH_ID)irpSp->FileObject;
	pAddr->protocol = protocol;
	
	// Assign next unique identifier
	
	g_vars.nextAddrId++;

	while ((g_vars.nextAddrId == 0) || 
				ht_find_entry(g_vars.phtAddrById, g_vars.nextAddrId))
	{
		g_vars.nextAddrId++;
	}

	pAddr->id = g_vars.nextAddrId;
	
	pAddr->processId = processId;

	sl_init(&pAddr->lock);

	if (protocol == IPPROTO_UDP)
	{
		InitializeListHead(&pAddr->pendedReceivedPackets);
		InitializeListHead(&pAddr->receivedPackets);
		InitializeListHead(&pAddr->pendedReceiveRequests);
		
		InitializeListHead(&pAddr->pendedSendPackets);
		InitializeListHead(&pAddr->sendPackets);
		InitializeListHead(&pAddr->pendedSendRequests);

		InitializeListHead(&pAddr->pendedConnectRequest);
	}

	return pAddr;
}

void addr_free_queues(PADDROBJ pAddr)
{
	PNF_PACKET		pPacket;
	PLIST_ENTRY		pListEntry;
	PIRP			irp;
	KIRQL			irqld;
	
	if (!pAddr)
		return;

	KdPrint((DPREFIX"addr_free_queues[%I64d]\n", pAddr->id));

	if (pAddr->protocol == IPPROTO_UDP)
	{
		sl_lock(&pAddr->lock, &irqld);
			
		while (!IsListEmpty(&pAddr->receivedPackets))
		{
			pPacket = (PNF_PACKET)RemoveHeadList(&pAddr->receivedPackets);
		        
			KdPrint((DPREFIX"addr_free[%I64d]: Orphan receivedPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
					pAddr->id, pPacket->ioStatus, pPacket->dataSize ));
		        
			nf_packet_free(pPacket);
		}

		while (!IsListEmpty(&pAddr->pendedReceivedPackets))
		{
			pPacket = (PNF_PACKET)RemoveHeadList(&pAddr->pendedReceivedPackets);
		        
			KdPrint((DPREFIX"addr_free[%I64d]: Orphan pendedReceivedPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
					pAddr->id, pPacket->ioStatus, pPacket->dataSize ));
		        
			nf_packet_free(pPacket);
		}

		while (!IsListEmpty(&pAddr->sendPackets))
		{
			pPacket = (PNF_PACKET)RemoveHeadList(&pAddr->sendPackets);
		        
			KdPrint((DPREFIX"addr_free[%I64d]: Orphan sendPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
					pAddr->id, pPacket->ioStatus, pPacket->dataSize ));
		        
			nf_packet_free(pPacket);
		}

		while (!IsListEmpty(&pAddr->pendedSendPackets))
		{
			pPacket = (PNF_PACKET)RemoveHeadList(&pAddr->pendedSendPackets);
		        
			KdPrint((DPREFIX"addr_free[%I64d]: Orphan pendedSendPackets NF_PACKET - status=0x%8.8x, bytes=%d\n",
					pAddr->id, pPacket->ioStatus, pPacket->dataSize ));
		        
			nf_packet_free(pPacket);
		}

		while (!IsListEmpty(&pAddr->pendedReceiveRequests))
		{
			pListEntry = pAddr->pendedReceiveRequests.Flink;
				
			irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

			RemoveHeadList(&pAddr->pendedReceiveRequests);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = STATUS_INVALID_CONNECTION;
			irp->IoStatus.Information = 0;

			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, TRUE);

			KdPrint((DPREFIX"addr_free[%I64d]: Orphan pendedReceiveRequests IRP\n", pAddr->id));
		}


		while (!IsListEmpty(&pAddr->pendedSendRequests))
		{
			pListEntry = pAddr->pendedSendRequests.Flink;
				
			irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

			RemoveHeadList(&pAddr->pendedSendRequests);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = STATUS_INVALID_CONNECTION;
			irp->IoStatus.Information = 0;

			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, TRUE);

			KdPrint((DPREFIX"addr_free[%I64d]: Orphan pendedSendRequests IRP\n", pAddr->id));
		}

		while (!IsListEmpty(&pAddr->pendedConnectRequest))
		{
			pListEntry = pAddr->pendedConnectRequest.Flink;
				
			irp = (PIRP)CONTAINING_RECORD(pListEntry, IRP, Tail.Overlay.ListEntry);

			RemoveHeadList(&pAddr->pendedConnectRequest);
			InitializeListHead(&irp->Tail.Overlay.ListEntry);

			IoSetCancelRoutine(irp, NULL);

			irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
			irp->IoStatus.Information = 0;
			
			// Complete IRP from DPC
			ctrl_deferredCompleteIrp(irp, TRUE);

			KdPrint((DPREFIX"addr_free[%I64d]: Orphan pendedConnectRequest IRP\n", pAddr->id));
		}

		sl_unlock(&pAddr->lock, irqld);
	}
}

static void addr_free(PADDROBJ pAddr)
{
	KIRQL	irqld;
	
	if (!pAddr)
		return;

	KdPrint((DPREFIX"addr_free[%I64d]\n", pAddr->id));

	if (pAddr->protocol == IPPROTO_UDP)
	{
		sl_lock(&pAddr->lock, &irqld);
		pAddr->fileObject = 0;
		sl_unlock(&pAddr->lock, irqld);

		addr_free_queues(pAddr);
	}

	// Return memory back to the pool
	mp_free(pAddr, MAX_ADDR_POOL_SIZE);

#if DBG
	g_vars.nAddr--;
	KdPrint(("[counter] nAddr=%d\n", g_vars.nAddr));
#endif
}


NTSTATUS addr_QueryInformationComplete(IN PDEVICE_OBJECT pDeviceObject, 
									 IN PIRP irp, 
									 IN PVOID context)
{
	PADDROBJ pAddr = (PADDROBJ)context;

	KdPrint((DPREFIX"addr_QueryInformationComplete\n"));

	if (pAddr)
	{
		PTDI_ADDRESS_INFO pTai = (PTDI_ADDRESS_INFO)pAddr->queryAddressInfo;
		
		if (pTai)
		{
			PTA_ADDRESS pTAAddr = pTai->Address.Address;
		
			if (pTAAddr)
			{
				if (pTAAddr->AddressLength <= sizeof(pAddr->localAddr)) 
				{
                	memcpy(pAddr->localAddr, (char*)pTAAddr + sizeof(USHORT), pTAAddr->AddressLength + sizeof(USHORT));

					if (pAddr->protocol == IPPROTO_UDP)
					{
						if (ctrl_attached())
						{
							NF_UDP_CONN_INFO ci;
							addr_setConnInfo(&ci, pAddr);		
							ctrl_queuePushEx(NF_UDP_CREATED, pAddr->id, (char*)&ci, sizeof(ci));
						}
					}
				}
			}
		}
	}

	if (irp->MdlAddress) 
	{
		IoFreeMdl(irp->MdlAddress); 
		irp->MdlAddress = NULL;
	}

	// No need to free Irp, because it was allocated with TdiBuildInternalDeviceControlIrp.
	// IO manager frees it automatically.

	return STATUS_SUCCESS;
}


NTSTATUS addr_TdiOpenAddressComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context)
{
	PADDROBJ pAddr = (PADDROBJ)context;
	KIRQL irql;

	KdPrint((DPREFIX"addr_TdiOpenAddressComplete\n"));

	if (!NT_SUCCESS(irp->IoStatus.Status))
	{
		addr_free(pAddr);
	} else
    {
		sl_lock(&g_vars.slConnections, &irql);

		if (pAddr)
		{
			// Add address object to hashes 
			ht_add_entry(g_vars.phtAddrByFileObject, (PHASH_TABLE_ENTRY)&pAddr->fileObject);
			ht_add_entry(g_vars.phtAddrById, (PHASH_TABLE_ENTRY)&pAddr->id);

			// Insert address to list
			InsertTailList(&g_vars.lAddresses, &pAddr->entry);
		}

		sl_unlock(&g_vars.slConnections, irql);

		if (pAddr)
		{
			NTSTATUS status = STATUS_UNSUCCESSFUL;
			PIO_STACK_LOCATION	irpSp;
			IO_STATUS_BLOCK isb;
			PIRP qirp;
			PMDL mdl;

			irpSp = IoGetCurrentIrpStackLocation(irp);
			ASSERT(irpSp);

			mdl = IoAllocateMdl(pAddr->queryAddressInfo,
				TDI_ADDRESS_INFO_MAX, FALSE, FALSE, NULL);

			if (mdl)
			{
				// Make a request
				qirp = TdiBuildInternalDeviceControlIrp(
					TDI_QUERY_INFORMATION, 
					pDeviceObject,
					irpSp->FileObject, 
					NULL, 
					&isb);
				
				if (qirp)
				{
					MmBuildMdlForNonPagedPool(mdl);

					TdiBuildQueryInformation(
						qirp, 
						pDeviceObject, 
						irpSp->FileObject,
						addr_QueryInformationComplete, 
						pAddr,
						TDI_QUERY_ADDRESS_INFO, 
						mdl);

					status = IoCallDriver(pDeviceObject, qirp);
					mdl = NULL;
					qirp = NULL;
				}

				if (!NT_SUCCESS(status))
				{
					if (mdl != NULL)
					{
						IoFreeMdl(mdl);
					}
				}		
			}
		}
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

NTSTATUS addr_TdiOpenAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ pAddr = NULL;
	int protocol;
	KIRQL irql;
	HANDLE		pid = PsGetCurrentProcessId();
	PDEVICE_OBJECT pDevice;
	PUNICODE_STRING pName = NULL;
	ULONG		nameLen = 0;
	NTSTATUS 	status;

	KdPrint((DPREFIX"addr_TdiOpenAddress\n"));

	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"addr_TdiOpenAddress Not enough stack locations\n"));

		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	if (pLowerDevice == g_vars.deviceTCP)
	{
		protocol = IPPROTO_TCP;
	} else
	if (pLowerDevice == g_vars.deviceTCP6)
	{
		protocol = IPPROTO_TCP;
	} else
	if (pLowerDevice == g_vars.deviceUDP)
	{
		protocol = IPPROTO_UDP;
	} else
	if (pLowerDevice == g_vars.deviceUDP6)
	{
		protocol = IPPROTO_UDP;
	} else
	{
		ASSERT(0);

		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}

	sl_lock(&g_vars.slConnections, &irql);
	pAddr = addr_alloc(irpSp, *(ULONG*)&pid, protocol, pLowerDevice);
	sl_unlock(&g_vars.slConnections, irql);

	if (!pAddr)
	{
		IoSkipCurrentIrpStackLocation(irp);
		return IoCallDriver(pLowerDevice, irp);
	}

	{
	    UNICODE_STRING lcStr;

		status = getProcessName(pid, &pName);
		
		if (NT_SUCCESS(status) && pName)
		{
			if (pName->Buffer && pName->Length)
			{
				nameLen = pName->Length+2;

				if (nameLen > sizeof(pAddr->processName))
				{
					nameLen = sizeof(pAddr->processName);
				}

				lcStr.Buffer = (PWSTR)pAddr->processName;
				lcStr.Length = (USHORT)nameLen;
				lcStr.MaximumLength = lcStr.Length;

			    status = RtlDowncaseUnicodeString(&lcStr, pName, FALSE);
			}
			
			free_np(pName);
		}
	}

    IoCopyCurrentIrpStackLocationToNext(irp);
        
    IoSetCompletionRoutine(
            irp,
            addr_TdiOpenAddressComplete,
            pAddr,
            TRUE,
            TRUE,
            TRUE
            );
        
    return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS addr_TdiCloseAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	KIRQL irql, irqld;
	KEVENT event;
	LARGE_INTEGER li;

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	KdPrint((DPREFIX"addr_TdiCloseAddress\n"));
	
	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (pAddr)
	{
		if (ctrl_attached() && (pAddr->protocol == IPPROTO_UDP))
		{
			{
				li.QuadPart = 100 * -10000;	// 0.1 sec
				KeWaitForSingleObject(&event, UserRequest, KernelMode, FALSE, &li);
			}

			sl_lock(&pAddr->lock, &irqld);

			while (ctrl_attached() && (pAddr->sendBytesInTdi > 0))
			{
				sl_unlock(&pAddr->lock, irqld);

				{
					li.QuadPart = 100 * -10000;	// 0.1 sec
					KeWaitForSingleObject(&event, UserRequest, KernelMode, FALSE, &li);
				}

				sl_lock(&pAddr->lock, &irqld);
			}

			pAddr->closed = TRUE;

			sl_unlock(&pAddr->lock, irqld);
		}

		sl_lock(&g_vars.slConnections, &irql);

		// Remove address from hashes
		ht_remove_entry(g_vars.phtAddrByFileObject, pAddr->fileObject);
		ht_remove_entry(g_vars.phtAddrById, pAddr->id);

		// Remove address from list
		RemoveEntryList(&pAddr->entry);
	
		sl_unlock(&g_vars.slConnections, irql);

		if (pAddr->protocol == IPPROTO_UDP)
		{
			if (ctrl_attached())
			{
				NF_UDP_CONN_INFO ci;
				addr_setConnInfo(&ci, pAddr);		
				ctrl_queuePushEx(NF_UDP_CLOSED, pAddr->id, (char*)&ci, sizeof(ci));
			}
		}

		// Return memory back to the pool
		addr_free(pAddr);
	}


	KdPrint((DPREFIX"addr_TdiCloseAddress complete\n"));

	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}

NTSTATUS addr_TdiSetEvent(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	PTDI_REQUEST_KERNEL_SET_EVENT pEvent;
	KIRQL irql, irqld;
	NTSTATUS status = STATUS_SUCCESS;

	pEvent = (PTDI_REQUEST_KERNEL_SET_EVENT)&irpSp->Parameters;

	KdPrint((DPREFIX"addr_TdiSetEvent type=%d\n", pEvent->EventType));
/*
	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"addr_TdiSetEvent Not enough stack locations\n"));

		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}
*/
	sl_lock(&g_vars.slConnections, &irql);
	
	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);
	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		sl_lock(&pAddr->lock, &irqld);

		switch (pEvent->EventType)
		{
		case TDI_EVENT_CONNECT:
            if (pEvent->EventHandler)
            {
               pAddr->ev_connect = pEvent->EventHandler;
               pAddr->ev_connect_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) tcp_TdiConnectEventHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_connect = NULL;
               pAddr->ev_connect_context = NULL;
            }
            break;

		case TDI_EVENT_DISCONNECT:
            if (pEvent->EventHandler)
            {
               pAddr->ev_disconnect = pEvent->EventHandler;
               pAddr->ev_disconnect_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) tcp_TdiDisconnectEventHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_disconnect = NULL;
               pAddr->ev_disconnect_context = NULL;
            }
            break;

		case TDI_EVENT_RECEIVE:
            if (pEvent->EventHandler)
            {
               pAddr->ev_receive = pEvent->EventHandler;
               pAddr->ev_receive_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) tcp_TdiReceiveEventHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_receive = NULL;
               pAddr->ev_receive_context = NULL;
            }
            break;

		case TDI_EVENT_RECEIVE_EXPEDITED:
            if (pEvent->EventHandler)
            {
               pAddr->ev_receive_expedited = pEvent->EventHandler;
               pAddr->ev_receive_expedited_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) tcp_TdiReceiveExpeditedEventHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_receive_expedited = NULL;
               pAddr->ev_receive_expedited_context = NULL;
            }
            break;
		
		case TDI_EVENT_CHAINED_RECEIVE:
			status = STATUS_INVALID_DEVICE_STATE;
			/*
            if (pEvent->EventHandler)
            {
			   pAddr->ev_chained_receive = pEvent->EventHandler;
               pAddr->ev_chained_receive_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) tcp_TdiChainedReceiveHandler;
               pEvent->EventContext = (PVOID) pAddr;	

			} else
            {
               pAddr->ev_chained_receive = NULL;
               pAddr->ev_chained_receive_context = NULL;
            }
			*/
            break;

		case TDI_EVENT_RECEIVE_DATAGRAM:
            if (pEvent->EventHandler)
            {
               pAddr->ev_receive_dg = pEvent->EventHandler;
               pAddr->ev_receive_dg_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) udp_TdiReceiveDGEventHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_receive_dg = NULL;
               pAddr->ev_receive_dg_context = NULL;
            }
            break;

		case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
			status = STATUS_INVALID_DEVICE_STATE;
/*            if (pEvent->EventHandler)
            {
               pAddr->ev_chained_receive_dg = pEvent->EventHandler;
               pAddr->ev_chained_receive_dg_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) udp_TdiChainedReceiveDGEventHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_receive_dg = NULL;
               pAddr->ev_receive_dg_context = NULL;
            }
*/
			break;
/*
		case TDI_EVENT_SEND_POSSIBLE:
            if (pEvent->EventHandler)
            {
               pAddr->ev_send_possible = pEvent->EventHandler;
               pAddr->ev_send_possible_context = pEvent->EventContext;

			   pEvent->EventHandler = (PVOID) tcp_TdiSendPossibleHandler;
               pEvent->EventContext = (PVOID) pAddr;	
            } else
            {
               pAddr->ev_send_possible = NULL;
               pAddr->ev_send_possible_context = NULL;
            }
			break;
*/
		}

		sl_unlock(&pAddr->lock, irqld);
	}

	sl_unlock(&g_vars.slConnections, irql);

	if (!NT_SUCCESS(status))
	{
		irp->IoStatus.Status = STATUS_SUCCESS;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	
	IoSkipCurrentIrpStackLocation(irp);

	return IoCallDriver(pLowerDevice, irp);
}


void addr_postUdpEndpoints()
{
	PADDROBJ pAddr = NULL;
	KIRQL irql;

	KdPrint((DPREFIX"addr_postUdpEndpoints\n"));
	
	if (!ctrl_attached())
	{
		return;
	}

	sl_lock(&g_vars.slConnections, &irql);

	pAddr = (PADDROBJ)g_vars.lAddresses.Flink;
	
	while (pAddr != (PADDROBJ)&g_vars.lAddresses)
	{
		if (pAddr->protocol == IPPROTO_UDP)
		{
			NF_UDP_CONN_INFO ci;
			addr_setConnInfo(&ci, pAddr);		
			ctrl_queuePushEx(NF_UDP_CREATED, pAddr->id, (char*)&ci, sizeof(ci));
		}

		pAddr = (PADDROBJ)pAddr->entry.Flink;
	}
	
	sl_unlock(&g_vars.slConnections, irql);
}

void udp_startDeferredProc(PADDROBJ pAddr, int code)
{
	KIRQL irql;
	PDPC_QUEUE_ENTRY pEntry;

	pEntry = (PDPC_QUEUE_ENTRY)mp_alloc(sizeof(DPC_QUEUE_ENTRY));
	if (!pEntry)
		return;

	pEntry->id = pAddr->id;
	pEntry->code = code;
	pEntry->pObj = pAddr;

	sl_lock(&g_vars.slUdpQueue, &irql);
	InsertTailList(&g_vars.udpQueue, &pEntry->entry);
	sl_unlock(&g_vars.slUdpQueue, irql);

	KeInsertQueueDpc(&g_vars.udpDpc, NULL, NULL);
}

VOID udp_procDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
{
	KIRQL irql;
	PDPC_QUEUE_ENTRY pEntry;
	PADDROBJ pAddr = NULL;
	PHASH_TABLE_ENTRY phte;
	BOOLEAN startAnotherDpc = FALSE;

	sl_lock(&g_vars.slUdpQueue, &irql);

	if (IsListEmpty(&g_vars.udpQueue))
	{
		sl_unlock(&g_vars.slUdpQueue, irql);
		return;
	}

	pEntry = (PDPC_QUEUE_ENTRY)RemoveHeadList(&g_vars.udpQueue);

	if (!IsListEmpty(&g_vars.udpQueue))
	{
		startAnotherDpc = TRUE;
	}

	sl_unlock(&g_vars.slUdpQueue, irql);

	pAddr = (PADDROBJ)pEntry->pObj;

	if (pAddr)
	{
		switch (pEntry->code)
		{
		case DQC_UDP_INDICATE_RECEIVE:
			udp_indicateReceivedPackets(pAddr, pEntry->id);
			break;
		}
	}

	mp_free(pEntry, 100);

	if (startAnotherDpc)
	{
		KeInsertQueueDpc(&g_vars.udpDpc, NULL, NULL);
	}
}
