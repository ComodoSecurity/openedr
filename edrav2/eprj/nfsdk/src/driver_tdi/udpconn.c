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
#include "udpconn.h"
#include "gvars.h"
#include "rules.h"
#include "addr.h"
#include "ctrlio.h"

#ifdef _WPPTRACE
#include "udpconn.tmh"
#endif

NTSTATUS udp_setConnRequest(PNF_UDP_CONN_REQUEST pcr, PADDROBJ pAddr)
{
	memset(pcr, 0, sizeof(NF_UDP_CONN_REQUEST));

	pcr->filteringFlag = pAddr->filteringFlag;
	pcr->processId = (ULONG)pAddr->processId;
	pcr->ip_family = *(short*)pAddr->localAddr;

	RtlCopyMemory(pcr->localAddress, pAddr->localAddr, sizeof(pcr->localAddress));
	RtlCopyMemory(pcr->remoteAddress, pAddr->remoteAddr, sizeof(pcr->remoteAddress));

	return STATUS_SUCCESS;
}


void udp_cancelTdiConnect(PDEVICE_OBJECT pDeviceObject, PIRP irp)
{
	PIO_STACK_LOCATION	irpSp;
	PADDROBJ			pAddr = NULL;
	PHASH_TABLE_ENTRY	phte;
	NTSTATUS			status;
	KIRQL				irql, irqld;
	BOOLEAN				mustComplete = FALSE;

    KdPrint((DPREFIX"udp_cancelTdiConnect\n"));

    irpSp = IoGetCurrentIrpStackLocation(irp);
    ASSERT(irpSp);

	//  Dequeue and complete the IRP.  

	sl_lock(&g_vars.slConnections, &irql);

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		sl_lock(&pAddr->lock, &irqld);

		KdPrint((DPREFIX"udp_cancelTdiConnect[%I64d]\n", pAddr->id));

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

NTSTATUS udp_TdiConnect(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice)
{
	PTA_ADDRESS          pRemoteAddr = NULL;
	PTDI_REQUEST_KERNEL  prk;
	PADDROBJ			 pAddr = NULL;
	PHASH_TABLE_ENTRY	 phte;
    KIRQL                irql;
    char                 NullAddr[TA_ADDRESS_MAX];

	KdPrint((DPREFIX"udp_TdiConnect\n"));

	if (!ctrl_attached())
	{
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}
/*
	if (irp->CurrentLocation == 1)
	{
		KdPrint((DPREFIX"udp_TdiConnect: Not enough stack locations\n"));
	
		IoSkipCurrentIrpStackLocation(irp);

		return IoCallDriver(pLowerDevice, irp);
	}
*/
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

	phte = ht_find_entry(g_vars.phtAddrByFileObject, (HASH_ID)irpSp->FileObject);

	if (phte)
	{
		pAddr = (PADDROBJ)CONTAINING_RECORD(phte, ADDROBJ, fileObject);

		if (pRemoteAddr->AddressLength > sizeof(pAddr->remoteAddr)) 
		{
			KdPrint(("udp_TdiConnect: address too long! (%u)\n", pRemoteAddr->AddressLength));
		} else
		{
			memcpy(pAddr->remoteAddr, (char*)pRemoteAddr + sizeof(USHORT), pRemoteAddr->AddressLength + sizeof(USHORT));
		}

		pAddr->filteringFlag = rule_findByAddr(pAddr, NF_D_OUT, (sockaddr_gen*)&pRemoteAddr->AddressType);

		if (pAddr->filteringFlag & NF_INDICATE_CONNECT_REQUESTS)
		{
			NF_UDP_CONN_REQUEST cr;
		    PDRIVER_CANCEL	oldCancelRoutine;
			KIRQL irqld;

			KdPrint((DPREFIX"udp_TdiConnect[%I64d] pended\n", pAddr->id));

			IoMarkIrpPending(irp);
					
			sl_lock(&pAddr->lock, &irqld);
			InsertTailList(&pAddr->pendedConnectRequest, &irp->Tail.Overlay.ListEntry);
			sl_unlock(&pAddr->lock, irqld);

			oldCancelRoutine = IoSetCancelRoutine(irp, udp_cancelTdiConnect);
			irp->IoStatus.Status = STATUS_PENDING;

			udp_setConnRequest(&cr, pAddr);

			if (NT_SUCCESS(ctrl_queuePushEx(NF_UDP_CONNECT_REQUEST, pAddr->id, (char*)&cr, sizeof(cr))))
			{
				sl_unlock(&g_vars.slConnections, irql);
				return STATUS_PENDING;
			}
		
			KdPrint((DPREFIX"udp_TdiConnect[%I64d] unpended\n", pAddr->id));

			sl_lock(&pAddr->lock, &irqld);
			RemoveEntryList(&irp->Tail.Overlay.ListEntry);				
			InitializeListHead(&irp->Tail.Overlay.ListEntry);
			sl_unlock(&pAddr->lock, irqld);

			IoSetCancelRoutine(irp, oldCancelRoutine);
			irp->IoStatus.Status = STATUS_SUCCESS;
		}
	}

	sl_unlock(&g_vars.slConnections, irql);

	IoSkipCurrentIrpStackLocation(irp);
        
    return IoCallDriver(pLowerDevice, irp);
}
