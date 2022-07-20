//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _TCPCONN_H
#define _TCPCONN_H

#include "addr.h"
#include "hashtable.h"

enum _BLOCK_SET_INFORMATION_CODE
{
	BSIC_NONE = 0,
	BSIC_NODELAY = 1,
	BSIC_KEEPALIVE = 2,
	BSIC_OOBINLINE = 4,
	BSIC_BSDURGENT = 8,
	BSIC_ATMARK = 16,
	BSIC_WINDOW = 32
};

typedef struct _CONNOBJ
{
	/**
	*	Linkage
	*/
	LIST_ENTRY entry;
	
	/**
	 *  File object
	 */
    HASH_ID	fileObject;
	PHASH_TABLE_ENTRY fileObject_next;

	/**
	 *  Connection context
	 */
	HASH_ID	context; 
	PHASH_TABLE_ENTRY context_next;

	/**
	 *  Connection identifier
	 */
	HASH_ID	id;
	PHASH_TABLE_ENTRY id_next;

	/**
	 *  Associated address
	 */
	PADDROBJ	pAddr;

	/**
	 *  Filtering flag (see <tt>NF_FILTERING_FLAG</tt>)
	 */
	ULONG		filteringFlag;

	/**
	 *  Owner pid (a copy of processId from pAddr)
	 */
	ULONG		processId;

	/**
	 *  Local address
	 */
    UCHAR		localAddr[TA_ADDRESS_MAX];

	/**
	 *  Remote address
	 */
	UCHAR		remoteAddr[TA_ADDRESS_MAX];

	/**
	 *  See <tt>NF_DIRECTION</tt>
	 */
	UCHAR		direction;

	/**
	 *	TRUE if TDI_CONNECT or TDI_ACCEPT succeeded for the connection
	 */
	BOOLEAN		connected;

	BOOLEAN		disableUserModeFiltering;
	BOOLEAN		userModeFilteringDisabled;

	// Receives
	LIST_ENTRY	pendedReceivedPackets;
	LIST_ENTRY	receivedPackets;
	LIST_ENTRY	pendedReceiveRequests;

	int			recvBytesInTdi;
	
	BOOLEAN		receiveInProgress;
	BOOLEAN		receiveEventCalled;

	BOOLEAN		receiveEventInProgress;
	
	// Sends
	LIST_ENTRY	pendedSendPackets;
	LIST_ENTRY	sendPackets;
	LIST_ENTRY	pendedSendRequests;

	BOOLEAN		sendInProgress;
	int			sendBytesInTdi;
	BOOLEAN		sendError;

	// Pended TDI_CONNECT request
	LIST_ENTRY	pendedConnectRequest;

	// Disconnects
	LIST_ENTRY	pendedDisconnectRequest;

	BOOLEAN		disconnectEventPending;
	ULONG		disconnectEventFlags;

	ULONG		flags;

	BOOLEAN		disconnectCalled;
	BOOLEAN		disconnectEventCalled;

	// Connection spinlock
	KSPIN_LOCK	lock;

	PDEVICE_OBJECT lowerDevice;

	ULONG		blockSetInformation;
} CONNOBJ, *PCONNOBJ; 

typedef struct _IO_HOOK_CONTEXT
{
    PIO_COMPLETION_ROUTINE	old_cr;	
    PVOID					old_context;
	PVOID					new_context;
	PFILE_OBJECT			fileobj;	
	UCHAR					old_control;
} IO_HOOK_CONTEXT, *PIO_HOOK_CONTEXT;

// TDI TCP device request handlers
NTSTATUS tcp_TdiOpenConnection(PIRP irp, PIO_STACK_LOCATION irpSp, PVOID context, PDEVICE_OBJECT pLowerDevice);
NTSTATUS tcp_TdiCloseConnection(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS tcp_TdiConnect(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS tcp_TdiAssociateAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS tcp_TdiDisAssociateAddress(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS tcp_TdiDisconnect(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);
NTSTATUS tcp_TdiListen(PIRP irp, PIO_STACK_LOCATION irpSp, PDEVICE_OBJECT pLowerDevice);

// Event handlers
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
   );

NTSTATUS tcp_TdiDisconnectEventHandler(
       IN PVOID TdiEventContext,
       IN CONNECTION_CONTEXT ConnectionContext,
       IN LONG DisconnectDataLength,
       IN PVOID DisconnectData,
       IN LONG DisconnectInformationLength,
       IN PVOID DisconnectInformation,
       IN ULONG DisconnectFlags
      );

// Disconnect routines
NTSTATUS tcp_callTdiDisconnectEventHandler(PCONNOBJ pConn, KIRQL irql);
NTSTATUS tcp_callTdiDisconnect(PCONNOBJ pConn, KIRQL irql);

void tcp_conn_free_queues(PCONNOBJ pConn, BOOLEAN disconnect);

NTSTATUS tcp_TdiConnectComplete(
   PDEVICE_OBJECT    pDeviceObject,
   PIRP              irp,
   void              * context);

void tcp_startDeferredProc(PCONNOBJ pConn, int code);

VOID tcp_procDpc(
    IN PKDPC  Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );

VOID tcp_procReinject();

BOOLEAN tcp_isProxy(ULONG processId);


#endif