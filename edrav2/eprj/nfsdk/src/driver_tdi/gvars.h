//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _GVARS_H
#define _GVARS_H

#include "nfdriver.h"
#include "hashtable.h"

/**
*	Internal IO queue entry
**/
typedef struct _DPC_QUEUE_ENTRY
{
	LIST_ENTRY		entry;		// Linkage
	int				code;		// IO code
	HASH_ID			id;			// Endpoint id
	void *			pObj;
} DPC_QUEUE_ENTRY, *PDPC_QUEUE_ENTRY;

typedef enum _DPC_QUEUE_CODE
{
	DQC_TCP_INDICATE_RECEIVE,
	DQC_TCP_INTERNAL_RECEIVE,
	DQC_TCP_SEND,
	DQC_UDP_INDICATE_RECEIVE,
} DPC_QUEUE_CODE;


/**
 *  Global variables
 */
typedef struct _GVARS
{
	UNICODE_STRING TCPFilterDeviceName;
	wchar_t wszTCPFilterDeviceName[MAX_PATH];

	UNICODE_STRING TCP6FilterDeviceName;
	wchar_t wszTCP6FilterDeviceName[MAX_PATH];

	UNICODE_STRING UDPFilterDeviceName;
	wchar_t wszUDPFilterDeviceName[MAX_PATH];

	UNICODE_STRING UDP6FilterDeviceName;
	wchar_t wszUDP6FilterDeviceName[MAX_PATH];

	UNICODE_STRING ctrlDeviceName;
	wchar_t wszCtrlDeviceName[MAX_PATH];

	UNICODE_STRING ctrlDeviceLinkName;
	wchar_t wszCtrlDeviceLinkName[MAX_PATH];


	/**
	 *  Our device for filtering TCP calls
	 */
	PDEVICE_OBJECT deviceTCPFilter;

	/**
	 *  System TCP device
	 */
	PDEVICE_OBJECT deviceTCP;

	/**
	 *  System TCP device file object
	 */
	PFILE_OBJECT   TCPFileObject;

	/**
	 *  Our device for filtering TCP6 calls
	 */
	PDEVICE_OBJECT deviceTCP6Filter;

	/**
	 *  System TCP6 device
	 */
	PDEVICE_OBJECT deviceTCP6;

	/**
	 *  System TCP6 device file object
	 */
	PFILE_OBJECT   TCP6FileObject;

	/**
	 *  Our device for filtering UDP calls
	 */
	PDEVICE_OBJECT deviceUDPFilter;

	/**
	 *  System UDP device
	 */
	PDEVICE_OBJECT deviceUDP;

	/**
	 *  System UDP device file object
	 */
	PFILE_OBJECT   UDPFileObject;

	/**
	 *  Our device for filtering UDP6 calls
	 */
	PDEVICE_OBJECT deviceUDP6Filter;

	/**
	 *  System UDP6 device
	 */
	PDEVICE_OBJECT deviceUDP6;

	/**
	 *  System UDP6 device file object
	 */
	PFILE_OBJECT   UDP6FileObject;

	/**
	 *  Our device for comminicating with user-mode
	 */
	PDEVICE_OBJECT deviceControl;

	/**
	 *  Linked list of RULE structures
	 */
	LIST_ENTRY lRules;
	LIST_ENTRY lTempRules;
	KSPIN_LOCK slRules;

	/**
	 *	Protects lConnections, lAddresses and linked hash tables
	 */
	KSPIN_LOCK	slConnections;

	/**
	 *  Linked list of CONNOBJ structures 
	 */
	LIST_ENTRY	lConnections;
	
	/**
	 *	Hashes 
	 */
	PHASH_TABLE phtConnByFileObject;
	PHASH_TABLE phtConnByContext;
	PHASH_TABLE phtConnById;

	/**
	 *	Next value for connection unique identifier
	 */
	ENDPOINT_ID	nextConnId;

	/**
	 *  Linked list of ADDROBJ structures 
	 */
	LIST_ENTRY	lAddresses;

	/**
	 *	Hashes 
	 */
	PHASH_TABLE phtAddrByFileObject;
	PHASH_TABLE phtAddrById;


	/**
	 *	Next value for address unique identifier
	 */
	ENDPOINT_ID	nextAddrId;
	
	/**
	 *	IO structures
	 */
	LIST_ENTRY	ioQueue;
	LIST_ENTRY	pendedIoRequests;
	KTIMER		completeIoDpcTimer;
	KDPC		completeIoDpc;
	KSPIN_LOCK	slIoQueue;

	/**
	 *	Pended IRPs waiting for completion
	 */
	LIST_ENTRY	pendedIrpsToComplete;
	KSPIN_LOCK	slPendedIrps;

	/**
	 *	TRUE, if some process attached to driver API
	 */
	BOOLEAN		proxyAttached;

	/**
	 *	pid of the attached process
	 */
	HANDLE		proxyPid;

	LIST_ENTRY	tcpQueue;
	KSPIN_LOCK	slTcpQueue;
	KDPC		tcpDpc;

	LIST_ENTRY	udpQueue;
	KSPIN_LOCK	slUdpQueue;
	KDPC		udpDpc;

	KDPC		queueDpc;

#if DBG
	int	nAddr;
	int nConn;
	int nPackets;
	int nIOReq;
#endif

} GVARS, *PGVARS;

extern GVARS g_vars;


/**
* Initialize g_vars. The driver name is extracted from RegistryPath 
* and used for generating device names.
* @return NTSTATUS 
* @param RegistryPath 
**/
NTSTATUS init_gvars(PUNICODE_STRING RegistryPath);


#endif // _GVARS_H