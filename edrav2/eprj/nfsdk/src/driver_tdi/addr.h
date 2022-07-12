//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _ADDR_H
#define _ADDR_H

#include "hashtable.h"

typedef struct _ADDROBJ
{
	/**
	*	Linkage
	*/
    LIST_ENTRY	entry;		  

	/**
	 *  File object in hash entry
	 */
    HASH_ID		fileObject;
	PHASH_TABLE_ENTRY fileObject_next;

	/**
	 *  Address identifier in hash entry
	 */
	HASH_ID		id;
	PHASH_TABLE_ENTRY id_next;

	/**
	 *  Filtering flag (see <tt>NF_FILTERING_FLAG</tt>)
	 */
	ULONG		filteringFlag;

	/**
	 *  Process identifier
	 */
	ULONG		processId;

	/**
	*	Protocol
	*/
    int			protocol;          

	/**
	 *  Local address
	 */
    UCHAR		localAddr[TA_ADDRESS_MAX];

	/**
	 *  Remote address
	 */
	UCHAR		remoteAddr[TA_ADDRESS_MAX];

	//
    // Event Handlers
    //
    PTDI_IND_CONNECT           ev_connect;       // Connect event handle
    PVOID                      ev_connect_context;   // Receive DG context
    PTDI_IND_DISCONNECT        ev_disconnect;    // Disconnect event routine
    PVOID                      ev_disconnect_context;// Disconnect event context
    PTDI_IND_ERROR             ev_error;         // Error event routine
    PVOID                      ev_error_context;    // Error event context
    PTDI_IND_RECEIVE           ev_receive;           // Receive event handler
    PVOID                      ev_receive_context;    // Receive context
    PTDI_IND_CHAINED_RECEIVE   ev_chained_receive;           // Chained receive event handler
    PVOID                      ev_chained_receive_context;    // Chained receive context
    PTDI_IND_RECEIVE_DATAGRAM  ev_receive_dg;         // Receive DG event handler
    PVOID                      ev_receive_dg_context;  // Receive DG context
    PTDI_IND_CHAINED_RECEIVE_DATAGRAM  ev_chained_receive_dg;         // Chained receive DG event handler
    PVOID                      ev_chained_receive_dg_context;  // Chained receive DG context
    PTDI_IND_SEND_POSSIBLE	   ev_send_possible;        
    PVOID                      ev_send_possible_context;  
    PTDI_IND_RECEIVE_EXPEDITED ev_receive_expedited;           // Receive expedited event handler
    PVOID                      ev_receive_expedited_context;    // Receive expedited context

	/**
	 *  Buffer for QueryAddressInfo call
	 */
	UCHAR	queryAddressInfo[TDI_ADDRESS_INFO_MAX];

	BOOLEAN		userModeFilteringDisabled;

	// UDP receives
	LIST_ENTRY	pendedReceivedPackets;
	LIST_ENTRY	receivedPackets;
	LIST_ENTRY	pendedReceiveRequests;

	// UDP sends
	LIST_ENTRY	pendedSendPackets;
	LIST_ENTRY	sendPackets;
	LIST_ENTRY	pendedSendRequests;

	BOOLEAN		sendInProgress;
	int			sendBytesInTdi;
	BOOLEAN		closed;

	// Pended TDI_CONNECT request
	LIST_ENTRY	pendedConnectRequest;

	wchar_t		processName[MAX_PATH];

	// Address spinlock
	KSPIN_LOCK	lock;

	PDEVICE_OBJECT lowerDevice;
} ADDROBJ, * PADDROBJ;

NTSTATUS addr_TdiSetEvent(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS addr_TdiOpenAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS addr_TdiCloseAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

void addr_postUdpEndpoints();
void addr_free_queues(PADDROBJ pAddr);

void udp_startDeferredProc(PADDROBJ pAddr, int code);

VOID udp_procDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );

#endif