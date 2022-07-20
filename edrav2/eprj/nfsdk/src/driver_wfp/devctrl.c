//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#include "stdinc.h"
#include "devctrl.h"
#include "nfdriver.h"
#include "tcpctx.h"
#include "udpctx.h"
#include "ipctx.h"
#include "rules.h"
#include "callouts.h"

#ifdef _WPPTRACE
#include "devctrl.tmh"
#endif

#define NF_CTRL_DEVICE			L"\\Device\\CtrlSM"
#define NF_CTRL_DEVICE_LINK		L"\\DosDevices\\CtrlSM"

static UNICODE_STRING g_ctrlDeviceName;
static wchar_t g_wszCtrlDeviceName[MAX_PATH];

static UNICODE_STRING g_ctrlDeviceLinkName;
static wchar_t g_wszCtrlDeviceLinkName[MAX_PATH];

/**
 *  Our device for comminicating with user-mode
 */
static PDEVICE_OBJECT g_deviceControl;

static NPAGED_LOOKASIDE_LIST g_ioQueueList;

/**
 *	IO structures
 */
static LIST_ENTRY	g_ioQueue;
static LIST_ENTRY	g_pendedIoRequests;
static KSPIN_LOCK	g_slIoQueue;
static PVOID		g_ioThreadObject = NULL;
static KEVENT		g_ioThreadEvent;

void devctrl_ioThread(IN PVOID StartContext);

/**
 *	TRUE, if some process attached to driver API
 */
static BOOLEAN		g_proxyAttached;

/**
 *	pid of the attached process
 */
static HANDLE		g_proxyPid;

typedef struct _SHARED_MEMORY
{
	PMDL					mdl;
	PVOID					userVa;
	PVOID					kernelVa;
    UINT64		bufferLength;
} SHARED_MEMORY, *PSHARED_MEMORY;

static SHARED_MEMORY g_inBuf;
static SHARED_MEMORY g_outBuf;

static BOOLEAN	  g_initialized = FALSE;

typedef struct _NF_QUEUE_ENTRY
{
	LIST_ENTRY		entry;		// Linkage
	int				code;		// IO code
	HASH_ID			id;			// Endpoint id
	union {
		struct {
			PTCPCTX tcpCtx;
		} TCP;
		struct {
			PUDPCTX udpCtx;
		} UDP;
	} context;
} NF_QUEUE_ENTRY, *PNF_QUEUE_ENTRY;

static HANDLE g_injectionHandle = NULL;
static HANDLE g_udpInjectionHandle = NULL;
static HANDLE g_udpNwInjectionHandleV4 = NULL;
static HANDLE g_udpNwInjectionHandleV6 = NULL;
static HANDLE g_nwInjectionHandleV4 = NULL;
static HANDLE g_nwInjectionHandleV6 = NULL;
static PNDIS_GENERIC_OBJECT g_ndisGenericObj = NULL;
static NDIS_HANDLE g_netBufferListPool = NULL;
static BOOLEAN g_shutdown = FALSE;
static BOOLEAN g_udpNwInject = FALSE;
static DWORD g_disabledCallouts = 0;

KSTART_ROUTINE devctrl_injectThread;
void devctrl_injectThread(IN PVOID StartContext);

static PVOID g_threadObject = NULL;
static KEVENT g_threadIoEvent;

static LIST_ENTRY	g_tcpInjectQueue;
static KSPIN_LOCK	g_slTcpInjectQueue;
static volatile long g_injectCount = 0;

typedef struct _NF_UDP_INJECT_CONTEXT
{
	HASH_ID	id;
	int		code;
	PMDL	mdl;
	PUDPCTX	pUdpCtx;
	PNF_UDP_PACKET pPacket;
} NF_UDP_INJECT_CONTEXT, *PNF_UDP_INJECT_CONTEXT;

static NPAGED_LOOKASIDE_LIST g_udpInjectContextLAList;

typedef struct _NF_IP_INJECT_CONTEXT
{
	int		code;
	PMDL	mdl;
	PNF_IP_PACKET pPacket;
} NF_IP_INJECT_CONTEXT, *PNF_IP_INJECT_CONTEXT;

static NPAGED_LOOKASIDE_LIST g_ipInjectContextLAList;

void devctrl_serviceReads();

void devctrl_sleep(UINT ttw) 
{                                                       
    NDIS_EVENT  _SleepEvent;                            
    NdisInitializeEvent(&_SleepEvent);                  
    NdisWaitEvent(&_SleepEvent, ttw);
}

DRIVER_CANCEL devctrl_cancelRead;

DWORD regQueryValueDWORD(PUNICODE_STRING registryPath, const wchar_t * wszValueName)
{
    OBJECT_ATTRIBUTES attributes;
    HANDLE driverRegKey;
    NTSTATUS status;
    ULONG resultLength;
    UNICODE_STRING valueName;
    UCHAR buffer[sizeof( KEY_VALUE_PARTIAL_INFORMATION ) + sizeof( DWORD )];
	DWORD value = 0;

    InitializeObjectAttributes( &attributes,
                                registryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    status = ZwOpenKey( &driverRegKey,
                        KEY_READ,
                        &attributes );
    if (NT_SUCCESS( status )) 
	{
        RtlInitUnicodeString( &valueName, wszValueName );
        
        status = ZwQueryValueKey( driverRegKey,
                                  &valueName,
                                  KeyValuePartialInformation,
                                  buffer,
                                  sizeof(buffer),
                                  &resultLength );

        if (NT_SUCCESS( status )) 
		{
            value = *((DWORD*) &(((PKEY_VALUE_PARTIAL_INFORMATION) buffer)->Data));
        }
    }

    ZwClose( driverRegKey );

	return value;
}


typedef enum _NF_SECURITY_LEVEL
{
	SL_NONE = 0,
	SL_LOCAL_SYSTEM = 1,
	SL_ADMINS = 2,
	SL_USERS = 4
} NF_SECURITY_LEVEL;

NTSTATUS setDeviceDacl(PDEVICE_OBJECT DeviceObject, DWORD secLevel)
{
    SECURITY_DESCRIPTOR sd = { 0 };
    ULONG aclSize = 0;
    PACL pAcl = NULL;
	HANDLE fileHandle = NULL;
	int nAcls;
    NTSTATUS status;

	if (secLevel == 0)
		return STATUS_SUCCESS;

	for (;;)
	{
		status = ObOpenObjectByPointer(DeviceObject,
									   OBJ_KERNEL_HANDLE,
									   NULL,
									   WRITE_DAC,
									   0,
									   KernelMode,
									   &fileHandle);
		if (!NT_SUCCESS(status)) 
		{
			break;
		} 

		nAcls = 0;

		aclSize = sizeof(ACL);

		if (secLevel & SL_LOCAL_SYSTEM)
		{
			aclSize += RtlLengthSid(SeExports->SeLocalSystemSid);
			nAcls++;
		}

		if (secLevel & SL_ADMINS)
		{
			aclSize += RtlLengthSid(SeExports->SeAliasAdminsSid);
			nAcls++;
		}

		if (secLevel & SL_USERS)
		{
			aclSize += RtlLengthSid(SeExports->SeAuthenticatedUsersSid);
			nAcls++;
		}

		aclSize += nAcls * FIELD_OFFSET(ACCESS_ALLOWED_ACE, SidStart);

		pAcl = (PACL) ExAllocatePoolWithTag(PagedPool, aclSize, MEM_TAG);
		if (pAcl == NULL) 
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		status = RtlCreateAcl(pAcl, aclSize, ACL_REVISION);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		if (secLevel & SL_LOCAL_SYSTEM)
		{
			status = RtlAddAccessAllowedAce(pAcl,
											ACL_REVISION,
											GENERIC_READ | GENERIC_WRITE | DELETE,
											SeExports->SeLocalSystemSid);
			if (!NT_SUCCESS(status)) 
			{
				break;
			}
		}

		if (secLevel & SL_ADMINS)
		{
			status = RtlAddAccessAllowedAce(pAcl,
											ACL_REVISION,
											GENERIC_READ | GENERIC_WRITE | DELETE,
											SeExports->SeAliasAdminsSid );
			if (!NT_SUCCESS(status)) 
			{
				break;
			}
		}

		if (secLevel & SL_USERS)
		{
			status = RtlAddAccessAllowedAce(pAcl,
											ACL_REVISION,
											GENERIC_READ | GENERIC_WRITE | DELETE,
											SeExports->SeAuthenticatedUsersSid );
			if (!NT_SUCCESS(status)) 
			{
				break;
			}
		}

		status = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		status = RtlSetDaclSecurityDescriptor(&sd, TRUE, pAcl, FALSE);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		status = ZwSetSecurityObject(fileHandle, DACL_SECURITY_INFORMATION, &sd);
		if (!NT_SUCCESS(status)) 
		{
			break;
		}

		break;
	}

	if (fileHandle != NULL)
	{
		ZwClose(fileHandle);
	}

    if (pAcl != NULL) 
	{
        ExFreePool(pAcl);
    }

    return status;
}

void setDeviceSecurity(
	PDEVICE_OBJECT DeviceObject,
	PUNICODE_STRING registryPath)
{
	DWORD secLevel;

	secLevel = regQueryValueDWORD(registryPath, L"seclevel");

	if (secLevel != 0)
	{
		setDeviceDacl(DeviceObject, secLevel);
	}
}

PDEVICE_OBJECT devctrl_getDeviceObject()
{
	return g_deviceControl;
}

HANDLE	devctrl_getInjectionHandle()
{
	return g_injectionHandle;
}

HANDLE	devctrl_getUdpInjectionHandle()
{
	return g_udpInjectionHandle;
}

HANDLE	devctrl_getUdpNwV4InjectionHandle()
{
	return g_udpNwInjectionHandleV4;
}

HANDLE	devctrl_getUdpNwV6InjectionHandle()
{
	return g_udpNwInjectionHandleV6;
}

HANDLE	devctrl_getNwV4InjectionHandle()
{
	return g_nwInjectionHandleV4;
}

HANDLE	devctrl_getNwV6InjectionHandle()
{
	return g_nwInjectionHandleV6;
}

BOOLEAN devctrl_isProxyAttached()
{
	BOOLEAN		res;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slIoQueue, &lh);
	res = g_proxyAttached;
	sl_unlock(&lh);

	return res;
}

ULONG devctrl_getProxyPid()
{
	return *(ULONG*)&g_proxyPid;
}

BOOLEAN	devctrl_isShutdown()
{
	BOOLEAN		res;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slIoQueue, &lh);
	res = g_shutdown;
	sl_unlock(&lh);
	
	return res;
}

void devctrl_setShutdown()
{
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slIoQueue, &lh);
	g_shutdown = TRUE;
	sl_unlock(&lh);
}

DWORD devctrl_getDisabledCallouts()
{
	return g_disabledCallouts;
}

// EDR_FIX: Send data to user mode API
#include "cmdedr_devctrlext.h"

NTSTATUS devctrl_pushTcpData(HASH_ID id, int code, PTCPCTX pTcpCtx, PNF_PACKET pPacket)
{
	PNF_QUEUE_ENTRY pEntry;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_pushTcpData id=%I64d, code=%d\n", id, code));

	if (!devctrl_isProxyAttached())
		return STATUS_UNSUCCESSFUL;

	if (pTcpCtx)
	{
		if (pTcpCtx->filteringState == UMFS_DISABLED)
		{
			code = NF_TCP_REINJECT;
		}

		if (code == NF_TCP_REINJECT)
		{
			if (pPacket)
			{
				sl_lock(&pTcpCtx->lock, &lh);
				InsertTailList(&pTcpCtx->injectPackets, &pPacket->entry);
				sl_unlock(&lh);

				devctrl_pushTcpInject(pTcpCtx);
			}
		} else
		{
			pEntry = (PNF_QUEUE_ENTRY)ExAllocateFromNPagedLookasideList( &g_ioQueueList );
			if (!pEntry)
				return STATUS_NO_MEMORY;

			memset(pEntry, 0, sizeof(NF_QUEUE_ENTRY));

			pEntry->id = id;
			pEntry->code = code;

			if (pTcpCtx)
			{
				pEntry->context.TCP.tcpCtx = pTcpCtx;
				tcpctx_addRef(pTcpCtx);
			}

			if (pPacket)
			{
				sl_lock(&pTcpCtx->lock, &lh);
				InsertTailList(&pTcpCtx->pendedPackets, &pPacket->entry);
				sl_unlock(&lh);
			}

			sl_lock(&g_slIoQueue, &lh);
			InsertTailList(&g_ioQueue, &pEntry->entry);
			sl_unlock(&lh);

			KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);
		} 
	} else
	{
		pEntry = (PNF_QUEUE_ENTRY)ExAllocateFromNPagedLookasideList( &g_ioQueueList );
		if (!pEntry)
			return STATUS_NO_MEMORY;

		memset(pEntry, 0, sizeof(NF_QUEUE_ENTRY));

		pEntry->id = id;
		pEntry->code = code;

		sl_lock(&g_slIoQueue, &lh);
		InsertTailList(&g_ioQueue, &pEntry->entry);
		sl_unlock(&lh);

		KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);
	}

	return STATUS_SUCCESS;
}

NTSTATUS devctrl_pushUdpData(HASH_ID id, int code, PUDPCTX pUdpCtx, PNF_UDP_PACKET pPacket)
{
	PNF_QUEUE_ENTRY pEntry;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_pushUdpData id=%I64d, code=%d\n", id, code));

	if (!devctrl_isProxyAttached())
		return STATUS_UNSUCCESSFUL;

	pEntry = (PNF_QUEUE_ENTRY)ExAllocateFromNPagedLookasideList( &g_ioQueueList );
	if (!pEntry)
		return STATUS_UNSUCCESSFUL;

	memset(pEntry, 0, sizeof(NF_QUEUE_ENTRY));

	pEntry->id = id;
	pEntry->code = code;

	if (pUdpCtx)
	{
		pEntry->context.UDP.udpCtx = pUdpCtx;
		udpctx_addRef(pUdpCtx);

		if (pPacket)
		{
			sl_lock(&pUdpCtx->lock, &lh);
			if (code == NF_UDP_SEND)
			{
				InsertTailList(&pUdpCtx->pendedSendPackets, &pPacket->entry);
			} else
			if (code == NF_UDP_RECEIVE)
			{
				InsertTailList(&pUdpCtx->pendedRecvPackets, &pPacket->entry);
			}
			sl_unlock(&lh);
		}
	}

	sl_lock(&g_slIoQueue, &lh);
	InsertTailList(&g_ioQueue, &pEntry->entry);
	sl_unlock(&lh);

	KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_SUCCESS;
}

NTSTATUS devctrl_pushIPData(int code)
{
	PNF_QUEUE_ENTRY pEntry;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_pushIPData code=%d\n", code));

	if (!devctrl_isProxyAttached())
		return STATUS_UNSUCCESSFUL;

	pEntry = (PNF_QUEUE_ENTRY)ExAllocateFromNPagedLookasideList( &g_ioQueueList );
	if (!pEntry)
		return STATUS_UNSUCCESSFUL;

	memset(pEntry, 0, sizeof(NF_QUEUE_ENTRY));

	pEntry->id = 0;
	pEntry->code = code;

	sl_lock(&g_slIoQueue, &lh);
	InsertTailList(&g_ioQueue, &pEntry->entry);
	sl_unlock(&lh);

	KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_SUCCESS;
}

NTSTATUS devctrl_createSharedMemory(PSHARED_MEMORY pSharedMemory, UINT64 len)
{
    PMDL  mdl;
    PVOID userVa = NULL;
    PVOID kernelVa = NULL;
    PHYSICAL_ADDRESS lowAddress;
    PHYSICAL_ADDRESS highAddress;

	memset(pSharedMemory, 0, sizeof(SHARED_MEMORY));

	lowAddress.QuadPart = 0;
	highAddress.QuadPart = 0xFFFFFFFFFFFFFFFF;
 
    mdl = MmAllocatePagesForMdl(lowAddress, highAddress, lowAddress, (SIZE_T)len);
    if(!mdl) 
	{
        return STATUS_INSUFFICIENT_RESOURCES;
    }

	__try 
	{
		kernelVa = MmGetSystemAddressForMdlSafe(mdl, HighPagePriority);
        if (!kernelVa)
		{
			MmFreePagesFromMdl(mdl);       
			IoFreeMdl(mdl);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		//
		// The preferred way to map the buffer into user space
		//
		userVa = MmMapLockedPagesSpecifyCache(mdl,          // MDL
										  UserMode,     // Mode
										  MmCached,     // Caching
										  NULL,         // Address
										  FALSE,        // Bugcheck?
										  HighPagePriority); // Priority
		if (!userVa)
		{
			MmUnmapLockedPages(kernelVa, mdl);
			MmFreePagesFromMdl(mdl);       
			IoFreeMdl(mdl);
			return STATUS_INSUFFICIENT_RESOURCES;
		}
	}
    __except (EXCEPTION_EXECUTE_HANDLER)
	{
	}

    //
    // If we get NULL back, the request didn't work.
    // I'm thinkin' that's better than a bug check anyday.
    //
    if (!userVa || !kernelVa)  
	{
		if (userVa)
		{
			MmUnmapLockedPages(userVa, mdl);
		}
		if (kernelVa)
		{
			MmUnmapLockedPages(kernelVa, mdl);
		}
		MmFreePagesFromMdl(mdl);       
		IoFreeMdl(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Return the allocated pointers
    //
	pSharedMemory->mdl = mdl;
	pSharedMemory->userVa = userVa;
	pSharedMemory->kernelVa = kernelVa;
	pSharedMemory->bufferLength = MmGetMdlByteCount(mdl);

    return STATUS_SUCCESS;
}

void devctrl_freeSharedMemory(PSHARED_MEMORY pSharedMemory)
{
	if (pSharedMemory->mdl)
	{
		__try 
		{
			if (pSharedMemory->userVa)
			{
				MmUnmapLockedPages(pSharedMemory->userVa, pSharedMemory->mdl);
			}
			if (pSharedMemory->kernelVa)
			{
				MmUnmapLockedPages(pSharedMemory->kernelVa, pSharedMemory->mdl);
			}
		}
        __except (EXCEPTION_EXECUTE_HANDLER)
		{
		}

		MmFreePagesFromMdl(pSharedMemory->mdl);       
		IoFreeMdl(pSharedMemory->mdl);

		memset(pSharedMemory, 0, sizeof(SHARED_MEMORY));
	}
}


NTSTATUS devctrl_init(PDRIVER_OBJECT driverObject, PUNICODE_STRING registryPath)
{
	NTSTATUS 	status;
	wchar_t wszDriverName[MAX_PATH] = { L'\0' };
	int i, driverNameLen = 0;
	HANDLE threadHandle;

	g_deviceControl = NULL;
	g_proxyAttached = FALSE;
	g_initialized = FALSE;

	if (regPathExists(L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\services\\webssx"))
	{
		KdPrint((DPREFIX"devctrl_init using compatibility mode for UDP\n"));
		g_udpNwInject = TRUE;
	}

	InitializeListHead(&g_ioQueue);
	InitializeListHead(&g_pendedIoRequests);
	KeInitializeSpinLock(&g_slIoQueue);

	KeInitializeEvent(
		&g_ioThreadEvent,
		SynchronizationEvent,
		FALSE
		);

	status = PsCreateSystemThread(
               &threadHandle,
               THREAD_ALL_ACCESS,
               NULL,
               NULL,
               NULL,
			   devctrl_ioThread,
               NULL
               );

   if (NT_SUCCESS(status))
   {
	   KPRIORITY priority = HIGH_PRIORITY;

	   ZwSetInformationThread(threadHandle, ThreadPriority, &priority, sizeof(priority));

	   status = ObReferenceObjectByHandle(
				   threadHandle,
				   0,
				   NULL,
				   KernelMode,
				   &g_ioThreadObject,
				   NULL
				   );
	   ASSERT(NT_SUCCESS(status));

	   ZwClose(threadHandle);
   }

	KeInitializeSpinLock(&g_slTcpInjectQueue);
	InitializeListHead(&g_tcpInjectQueue);

	KeInitializeEvent(
		&g_threadIoEvent,
		SynchronizationEvent,
		FALSE
		);

	status = PsCreateSystemThread(
               &threadHandle,
               THREAD_ALL_ACCESS,
               NULL,
               NULL,
               NULL,
			   devctrl_injectThread,
               NULL
               );

   if (NT_SUCCESS(status))
   {
	   KPRIORITY priority = HIGH_PRIORITY;

	   ZwSetInformationThread(threadHandle, ThreadPriority, &priority, sizeof(priority));

	   status = ObReferenceObjectByHandle(
				   threadHandle,
				   0,
				   NULL,
				   KernelMode,
				   &g_threadObject,
				   NULL
				   );
	   ASSERT(NT_SUCCESS(status));

	   ZwClose(threadHandle);
   }



	ExInitializeNPagedLookasideList( &g_ioQueueList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_QUEUE_ENTRY ),
                                     MEM_TAG_QUEUE,
                                     0 );

	ExInitializeNPagedLookasideList( &g_udpInjectContextLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_UDP_INJECT_CONTEXT ),
									 MEM_TAG_UDP_INJECT,
                                     0 );

	ExInitializeNPagedLookasideList( &g_ipInjectContextLAList,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof( NF_IP_INJECT_CONTEXT ),
									 MEM_TAG_IP_INJECT,
                                     0 );
	for (;;)
	{
		
		NET_BUFFER_LIST_POOL_PARAMETERS nblPoolParams = {0};
		
		g_ndisGenericObj = NdisAllocateGenericObject(driverObject, MEM_TAG, 0);
		if (g_ndisGenericObj == NULL)
		{
			status = STATUS_NO_MEMORY;
			break;
		}

		nblPoolParams.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
		nblPoolParams.Header.Revision = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
		nblPoolParams.Header.Size = sizeof(nblPoolParams);
		nblPoolParams.fAllocateNetBuffer = TRUE;
		nblPoolParams.DataSize = 0;
		nblPoolParams.PoolTag = MEM_TAG;

		g_netBufferListPool = NdisAllocateNetBufferListPool(g_ndisGenericObj, &nblPoolParams);
		if (g_netBufferListPool == NULL)
		{
			status = STATUS_NO_MEMORY;
			break;
		}

		status = FwpsInjectionHandleCreate(
					AF_UNSPEC,
					FWPS_INJECTION_TYPE_STREAM,
					&g_injectionHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		status = FwpsInjectionHandleCreate(
					AF_UNSPEC,
					FWPS_INJECTION_TYPE_TRANSPORT,
					&g_udpInjectionHandle);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		status = FwpsInjectionHandleCreate(
					AF_INET,
					FWPS_INJECTION_TYPE_NETWORK,
					&g_udpNwInjectionHandleV4);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		status = FwpsInjectionHandleCreate(
					AF_INET6,
					FWPS_INJECTION_TYPE_NETWORK,
					&g_udpNwInjectionHandleV6);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		status = FwpsInjectionHandleCreate(
					AF_INET,
					FWPS_INJECTION_TYPE_NETWORK,
					&g_nwInjectionHandleV4);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		status = FwpsInjectionHandleCreate(
					AF_INET6,
					FWPS_INJECTION_TYPE_NETWORK,
					&g_nwInjectionHandleV6);
		if (!NT_SUCCESS(status))
		{
			break;
		}

		// Get driver name from registry path
		for (i = registryPath->Length / sizeof(wchar_t) - 1; i >= 0; i--)
		{
			if (registryPath->Buffer[i] == L'\\' ||
				registryPath->Buffer[i] == L'/')
			{
				i++;

				while (registryPath->Buffer[i] && 
					(registryPath->Buffer[i] != L'.') &&
					(driverNameLen < MAX_PATH))
				{
					wszDriverName[driverNameLen] = registryPath->Buffer[i];
					i++;
					driverNameLen++;
				}

				wszDriverName[driverNameLen] = L'\0';

				break;
			}
		}

		if (driverNameLen == 0)
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		// Initialize the control device name
		*g_wszCtrlDeviceName = L'\0';
		g_ctrlDeviceName.Buffer = g_wszCtrlDeviceName;
		g_ctrlDeviceName.Length = 0;
		g_ctrlDeviceName.MaximumLength = sizeof(g_wszCtrlDeviceName);
		RtlAppendUnicodeToString(&g_ctrlDeviceName, NF_CTRL_DEVICE);
		RtlAppendUnicodeToString(&g_ctrlDeviceName, wszDriverName);

		// Initialize the control link device name
		*g_wszCtrlDeviceLinkName = L'\0';
		g_ctrlDeviceLinkName.Buffer = g_wszCtrlDeviceLinkName;
		g_ctrlDeviceLinkName.Length = 0;
		g_ctrlDeviceLinkName.MaximumLength = sizeof(g_wszCtrlDeviceLinkName);
		RtlAppendUnicodeToString(&g_ctrlDeviceLinkName, NF_CTRL_DEVICE_LINK);
		RtlAppendUnicodeToString(&g_ctrlDeviceLinkName, wszDriverName);

		status = IoCreateDevice(
					driverObject,
					0,
					&g_ctrlDeviceName,
        			FILE_DEVICE_UNKNOWN,
        			FILE_DEVICE_SECURE_OPEN,
        			FALSE,
        			&g_deviceControl);

		if (!NT_SUCCESS(status))
		{
			break;
		}

		g_deviceControl->Flags &= ~DO_DEVICE_INITIALIZING;

		status = IoCreateSymbolicLink(&g_ctrlDeviceLinkName, &g_ctrlDeviceName);
		if (!NT_SUCCESS(status))
		{
			IoDeleteDevice(g_deviceControl);
			g_deviceControl = NULL;
			break;
		}

		g_deviceControl->Flags &= ~DO_DEVICE_INITIALIZING;
		g_deviceControl->Flags |= DO_DIRECT_IO;

		// Read the security level from driver registry key and apply the required restrictions
		setDeviceSecurity(g_deviceControl, registryPath);

		g_disabledCallouts = regQueryValueDWORD(registryPath, L"disabledcallouts");

		break;
	}

	g_initialized = TRUE;

	if (!NT_SUCCESS(status))
	{
		devctrl_free();
		return status;
	}

	return status;
}

void devctrl_cleanupIoQueueEntry(PNF_QUEUE_ENTRY pEntry)
{
	switch (pEntry->code)
	{
	case NF_TCP_CONNECTED:
	case NF_TCP_CLOSED:
	case NF_TCP_RECEIVE:
	case NF_TCP_SEND:
	case NF_TCP_CAN_SEND:
	case NF_TCP_CAN_RECEIVE:
	case NF_TCP_CONNECT_REQUEST:
		if (pEntry->context.TCP.tcpCtx)
		{
			tcpctx_release(pEntry->context.TCP.tcpCtx);
		}
		break;

	case NF_UDP_CREATED:
	case NF_UDP_CLOSED:
	case NF_UDP_RECEIVE:
	case NF_UDP_SEND:
	case NF_UDP_CAN_SEND:
	case NF_UDP_CAN_RECEIVE:
	case NF_UDP_CONNECT_REQUEST:
		if (pEntry->context.UDP.udpCtx)
		{
			udpctx_release(pEntry->context.UDP.udpCtx);
		}
		break;
	// EDR_FIX: Send data to user mode API
	default:
		if (pEntry->code & 0x10000)
			cmdedr_freeCustomDataInt(pEntry);
		break;
	}
}

void devctrl_cleanupIoQueue()
{
	PNF_QUEUE_ENTRY pEntry;
    KLOCK_QUEUE_HANDLE lh;

    sl_lock(&g_slIoQueue, &lh);	

	while (!IsListEmpty(&g_ioQueue))
	{
		pEntry = (PNF_QUEUE_ENTRY)RemoveHeadList(&g_ioQueue);

		sl_unlock(&lh);	
	
		devctrl_cleanupIoQueueEntry(pEntry);

		ExFreeToNPagedLookasideList( &g_ioQueueList, pEntry );

		sl_lock(&g_slIoQueue, &lh);	
	}

	sl_unlock(&lh);	
}

void devctrl_free()
{
	KdPrint((DPREFIX"devctrl_free\n"));

	if (!g_initialized)
		return;

	if (g_deviceControl != NULL)
	{
		IoDeleteSymbolicLink(&g_ctrlDeviceLinkName);
		IoDeleteDevice(g_deviceControl);
		g_deviceControl = NULL;
	}

//	KeRemoveQueueDpc(&g_ioQueueDpc);

	if (g_threadObject)
	{
		KeSetEvent(&g_threadIoEvent, IO_NO_INCREMENT, FALSE);

		KeWaitForSingleObject(
			g_threadObject,
			Executive,
			KernelMode,
			FALSE,
			NULL
		  );

		ObDereferenceObject(g_threadObject);
		g_threadObject = NULL;
	}

	devctrl_cleanupIoQueue();

	if (g_ioThreadObject)
	{
		KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);

		KeWaitForSingleObject(
			g_ioThreadObject,
			Executive,
			KernelMode,
			FALSE,
			NULL
		  );

		ObDereferenceObject(g_ioThreadObject);
		g_ioThreadObject = NULL;
	}

	ExDeleteNPagedLookasideList( &g_ioQueueList );

	if (g_injectionHandle != NULL)
	{
		FwpsInjectionHandleDestroy(g_injectionHandle);
		g_injectionHandle = NULL;
	}
	if (g_udpInjectionHandle != NULL)
	{
		FwpsInjectionHandleDestroy(g_udpInjectionHandle);
		g_udpInjectionHandle = NULL;
	}
	if (g_udpNwInjectionHandleV4 != NULL)
	{
		FwpsInjectionHandleDestroy(g_udpNwInjectionHandleV4);
		g_udpNwInjectionHandleV4 = NULL;
	}
	if (g_udpNwInjectionHandleV6 != NULL)
	{
		FwpsInjectionHandleDestroy(g_udpNwInjectionHandleV6);
		g_udpNwInjectionHandleV6 = NULL;
	}
	if (g_nwInjectionHandleV4 != NULL)
	{
		FwpsInjectionHandleDestroy(g_nwInjectionHandleV4);
		g_nwInjectionHandleV4 = NULL;
	}
	if (g_nwInjectionHandleV6 != NULL)
	{
		FwpsInjectionHandleDestroy(g_nwInjectionHandleV6);
		g_nwInjectionHandleV6 = NULL;
	}
	if (g_netBufferListPool != NULL)
	{
		NdisFreeNetBufferListPool(g_netBufferListPool);
		g_netBufferListPool = NULL;
	}
	if (g_ndisGenericObj != NULL)
	{
		NdisFreeGenericObject(g_ndisGenericObj);
		g_ndisGenericObj = NULL;
	}

	ExDeleteNPagedLookasideList( &g_udpInjectContextLAList );
	ExDeleteNPagedLookasideList( &g_ipInjectContextLAList );

	g_initialized = FALSE;
}

NTSTATUS devctrl_create(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	KLOCK_QUEUE_HANDLE lh;
	NTSTATUS 	status;
	HANDLE		pid = PsGetCurrentProcessId();

	UNREFERENCED_PARAMETER(irpSp);

	sl_lock(&g_slIoQueue, &lh);
	if (g_proxyAttached)
	{
		status = STATUS_INVALID_DEVICE_REQUEST;
	} else
	{
		g_proxyPid = pid;
		g_proxyAttached = TRUE;
		status = STATUS_SUCCESS;
	}
	sl_unlock(&lh);

	// Put WFP sublayers to WPP log
	callouts_getSublayerWeight();

	irp->IoStatus.Information = 0;
	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

NTSTATUS devctrl_open(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

	if (ioBuffer && (outputBufferLength >= sizeof(NF_BUFFERS)))
	{
		NTSTATUS 	status;

		for (;;)	
		{
			if (!g_inBuf.mdl)
			{
				status = devctrl_createSharedMemory(&g_inBuf, NF_UDP_PACKET_BUF_SIZE * 50);
				if (!NT_SUCCESS(status))
				{
					break;
				}
			}

			if (!g_outBuf.mdl)
			{
				status = devctrl_createSharedMemory(&g_outBuf, NF_UDP_PACKET_BUF_SIZE * 2);
				if (!NT_SUCCESS(status))
				{
					break;
				}
			}

			status = STATUS_SUCCESS;

			break;
		}

		if (!NT_SUCCESS(status))
		{
			devctrl_freeSharedMemory(&g_inBuf);
			devctrl_freeSharedMemory(&g_outBuf);
		} else
		{	
			PNF_BUFFERS pBuffers = (PNF_BUFFERS)ioBuffer;

			pBuffers->inBuf = (UINT64)g_inBuf.userVa;
			pBuffers->inBufLen = g_inBuf.bufferLength;
			pBuffers->outBuf = (UINT64)g_outBuf.userVa;
			pBuffers->outBufLen = g_outBuf.bufferLength;

			udpctx_postCreateEvents();

			irp->IoStatus.Status = STATUS_SUCCESS;
			irp->IoStatus.Information = sizeof(NF_BUFFERS);
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			
			return STATUS_SUCCESS;
		}
	}

	irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_UNSUCCESSFUL;
}

NTSTATUS devctrl_close(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	KLOCK_QUEUE_HANDLE lh;
	PLIST_ENTRY	pEntry;
	PTCPCTX		pTcpCtx;
	int i;

	UNREFERENCED_PARAMETER(irpSp);

	KdPrint((DPREFIX"devctrl_close\n"));

	rules_remove_all();
	flowctl_delete(0);

	sl_lock(&g_slIoQueue, &lh);
	g_proxyPid = 0;
	g_proxyAttached = FALSE;
	sl_unlock(&lh);

	tcpctx_closeConnections();

	for (i=0; i<20; i++)
	{                                                       
		devctrl_sleep(1000);

		if (g_injectCount <= 0)
			break;
    }

	tcpctx_removeFromFlows();
	
	devctrl_sleep(1000);

	for (;;)
	{
		sl_lock(&g_slTcpInjectQueue, &lh);

		if (IsListEmpty(&g_tcpInjectQueue))
		{
			sl_unlock(&lh);
			break;
		}

		pEntry = RemoveHeadList(&g_tcpInjectQueue);
		pTcpCtx = CONTAINING_RECORD(pEntry, TCPCTX, injectQueueEntry);
		pTcpCtx->inInjectQueue = FALSE;

		sl_unlock(&lh);

		tcpctx_release(pTcpCtx);
	}

	devctrl_sleep(1000);

	devctrl_cleanupIoQueue();

	udpctx_cleanupFlows();

	ipctx_cleanup();

	tcpctx_releaseFlows();

	devctrl_freeSharedMemory(&g_inBuf);
	devctrl_freeSharedMemory(&g_outBuf);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

void devctrl_copyTcpConnInfo(PTCPCTX pTcpCtx, PNF_TCP_CONN_INFO pInfo)
{
	pInfo->filteringFlag = pTcpCtx->filteringFlag;
	pInfo->processId = pTcpCtx->processId;
	pInfo->direction = pTcpCtx->direction;
	pInfo->ip_family = pTcpCtx->ip_family;

	memcpy(pInfo->localAddress, pTcpCtx->localAddr, NF_MAX_ADDRESS_LENGTH);
	memcpy(pInfo->remoteAddress, pTcpCtx->remoteAddr, NF_MAX_ADDRESS_LENGTH);
}

NTSTATUS devctrl_copyTcpPacket(PTCPCTX pTcpCtx, UINT64 * pOffset)
{
	NTSTATUS	status = STATUS_SUCCESS;
	UINT64		packetLength;
	UINT64		dataSize;
	UINT64		offset;
	PNF_DATA	pData;
	PNF_PACKET	pPacket = NULL;
	SIZE_T		bytesCopied;
	KLOCK_QUEUE_HANDLE lh;
	BOOLEAN		repeat = FALSE;
	int			code = NF_TCP_SEND;

	sl_lock(&pTcpCtx->lock, &lh);

	for (;;)
	{
		if (IsListEmpty(&pTcpCtx->pendedPackets))
		{
			break;
		}

		pPacket = (PNF_PACKET)RemoveHeadList(&pTcpCtx->pendedPackets);

		code = (pPacket->streamData.flags & FWPS_STREAM_FLAG_SEND)? NF_TCP_SEND : NF_TCP_RECEIVE;

		if ((pTcpCtx->filteringFlag & NF_SUSPENDED) && (code == NF_TCP_SEND) 
			&& !pTcpCtx->sendDisconnectPending 
			&& !pTcpCtx->recvDisconnectPending)
		{
			KdPrint((DPREFIX"devctrl_copyTcpPacket[%I64u]: suspended connection\n", pTcpCtx->id));
			InsertHeadList(&pTcpCtx->pendedPackets, &pPacket->entry);
			pPacket = NULL;
			break;
		}
		
		if (pPacket->streamData.dataLength == 0)
		{
			dataSize = sizeof(NF_DATA) - 1;

			if ((g_inBuf.bufferLength - *pOffset) < dataSize)
			{
				status = STATUS_NO_MEMORY;
				break;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = code;
			pData->id = pTcpCtx->id;
			pData->bufferSize = 0;

			*pOffset += dataSize;
		} else
		{
			if (pPacket->flatStreamData || 
				pPacket->streamData.dataLength > NF_TCP_PACKET_BUF_SIZE)
			{
				if (!pPacket->flatStreamData)
				{
#pragma warning(push)
#pragma warning(disable: 28197) 
					pPacket->flatStreamData = 
						(char*)ExAllocatePoolWithTag(NonPagedPool, pPacket->streamData.dataLength, MEM_TAG_TCP_DATA);
#pragma warning(pop)

					if (!pPacket->flatStreamData)
					{
						ASSERT(0);
						dataSize = sizeof(NF_DATA) - 1;

						if ((g_inBuf.bufferLength - *pOffset) < dataSize)
						{
							status = STATUS_NO_MEMORY;
							break;
						}

						pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

						pData->code = code;
						pData->id = pTcpCtx->id;
						pData->bufferSize = 0;

						*pOffset += dataSize;
						break;
					}

					FwpsCopyStreamDataToBuffer(
						 &pPacket->streamData, 
						 pPacket->flatStreamData,
						 pPacket->streamData.dataLength,
						 &bytesCopied);
					
					ASSERT(bytesCopied == pPacket->streamData.dataLength);
				}

				offset = pPacket->flatStreamOffset;
				packetLength = pPacket->streamData.dataLength - offset;

				if (packetLength > NF_TCP_PACKET_BUF_SIZE)
				{
					packetLength = NF_TCP_PACKET_BUF_SIZE;
				}

				if (packetLength > 0)
				{
					dataSize = sizeof(NF_DATA) - 1 + packetLength;

					if ((g_inBuf.bufferLength - *pOffset) < dataSize)
					{
						status = STATUS_NO_MEMORY;
						break;
					}

					pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

					pData->code = code;
					pData->id = pTcpCtx->id;
					pData->bufferSize = (ULONG)packetLength;

					memcpy(pData->buffer, pPacket->flatStreamData + offset, (SIZE_T)packetLength);

					pPacket->flatStreamOffset += packetLength;
					*pOffset += dataSize;

					KdPrint((DPREFIX"devctrl_copyTcpPacket[%I64u] sent %I64u bytes from %u\n", 
						pTcpCtx->id, pPacket->flatStreamOffset, (ULONG)pPacket->streamData.dataLength));

					if (pPacket->flatStreamOffset < pPacket->streamData.dataLength)
					{
						InsertHeadList(&pTcpCtx->pendedPackets, &pPacket->entry);

						pPacket = NULL;
						repeat = TRUE;
					}
				}
			} else
			{
				dataSize = sizeof(NF_DATA) - 1 + pPacket->streamData.dataLength;

				if ((g_inBuf.bufferLength - *pOffset) < dataSize)
				{
					status = STATUS_NO_MEMORY;
					break;
				}

				pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

				pData->code = code;
				pData->id = pTcpCtx->id;
				pData->bufferSize = (ULONG)pPacket->streamData.dataLength;

				FwpsCopyStreamDataToBuffer(
					 &pPacket->streamData, 
					 pData->buffer,
					 pPacket->streamData.dataLength,
					 &bytesCopied);

				ASSERT(bytesCopied == pPacket->streamData.dataLength);

				*pOffset += dataSize;
			}
		}

		break;
	}

	sl_unlock(&lh);


	if (pPacket)
	{
		if (NT_SUCCESS(status))
		{
			sl_lock(&pTcpCtx->lock, &lh);
		
			if (code == NF_TCP_SEND)
			{
				if (pTcpCtx->pendedSendBytes >= (ULONG)pPacket->streamData.dataLength)
				{
					pTcpCtx->pendedSendBytes -= (ULONG)pPacket->streamData.dataLength;
				} else
				{
					pTcpCtx->pendedSendBytes = 0;
					ASSERT(0);
				}
				KdPrint((DPREFIX"devctrl_copyTcpPacket[%I64u] pendedSendBytes=%u\n", 
					pTcpCtx->id, pTcpCtx->pendedSendBytes));
			} else
			{
				if (pTcpCtx->pendedRecvBytes >= (ULONG)pPacket->streamData.dataLength)
				{
					pTcpCtx->pendedRecvBytes -= (ULONG)pPacket->streamData.dataLength;
				} else
				{
					pTcpCtx->pendedRecvBytes = 0;
					ASSERT(0);
				}

				KdPrint((DPREFIX"devctrl_copyTcpPacket[%I64u] pendedRecvBytes=%u\n", 
					pTcpCtx->id, pTcpCtx->pendedRecvBytes));
			}

			sl_unlock(&lh);

			tcpctx_freePacket(pPacket);

			sl_lock(&pTcpCtx->lock, &lh);

			if (pTcpCtx->recvDeferred &&
				(pTcpCtx->pendedRecvBytes < TCP_RECV_INJECT_LIMIT) &&
				(pTcpCtx->pendedRecvProtBytes < TCP_RECV_INJECT_LIMIT))
			{
				pTcpCtx->recvDeferred = FALSE;
			}

			sl_unlock(&lh);
		} else
		{
			sl_lock(&pTcpCtx->lock, &lh);
			InsertHeadList(&pTcpCtx->pendedPackets, &pPacket->entry);
			sl_unlock(&lh);
		}
	} else
	{
		if (repeat)
		{
			devctrl_pushTcpData(pTcpCtx->id, code, pTcpCtx, NULL);
		}
	}

	return status;

}

NTSTATUS devctrl_popTcpData(PNF_QUEUE_ENTRY pEntry, UINT64 * pOffset)
{
	NTSTATUS	status = STATUS_SUCCESS;
	UINT64		dataSize;
	UINT64		bytesAvailable;
	PNF_DATA	pData;

	KdPrint((DPREFIX"devctrl_popTcpData[%I64u]\n", pEntry->id));

	bytesAvailable = g_inBuf.bufferLength - *pOffset;

	switch (pEntry->code)
	{
	case NF_TCP_CONNECTED:
	case NF_TCP_CONNECT_REQUEST:
		{
			PTCPCTX pTcpCtx = NULL;
		
			dataSize = sizeof(NF_DATA) - 1 + sizeof(NF_TCP_CONN_INFO);

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = sizeof(NF_TCP_CONN_INFO);

			pTcpCtx = pEntry->context.TCP.tcpCtx;
			if (pTcpCtx)
			{
				devctrl_copyTcpConnInfo(pTcpCtx, (PNF_TCP_CONN_INFO)pData->buffer);
				tcpctx_release(pTcpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}
	
	case NF_TCP_CLOSED:
		{
			PTCPCTX pTcpCtx = NULL;
		
			dataSize = sizeof(NF_DATA) - 1 + sizeof(NF_FLOWCTL_STAT);

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = sizeof(NF_FLOWCTL_STAT);

			pTcpCtx = pEntry->context.TCP.tcpCtx;
			if (pTcpCtx)
			{
				PNF_FLOWCTL_STAT pStat = (PNF_FLOWCTL_STAT)pData->buffer;
				pStat->inBytes = pTcpCtx->inCounterTotal;
				pStat->outBytes = pTcpCtx->outCounterTotal;
				tcpctx_release(pTcpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_TCP_CAN_SEND:
	case NF_TCP_CAN_RECEIVE:
	case NF_TCP_REINJECT:
	case NF_TCP_DEFERRED_DISCONNECT:
		{
			dataSize = sizeof(NF_DATA) - 1;

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = 0;
			
			if (pEntry->context.TCP.tcpCtx)
			{
				tcpctx_release(pEntry->context.TCP.tcpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_TCP_RECEIVE:
	case NF_TCP_SEND:
		{
			if (pEntry->context.TCP.tcpCtx)
			{
				status = devctrl_copyTcpPacket(pEntry->context.TCP.tcpCtx, pOffset);
				if (NT_SUCCESS(status))
				{
					tcpctx_release(pEntry->context.TCP.tcpCtx);
				}
			}
			return status;
		}
	}

	KdPrint((DPREFIX"devctrl_popTcpData[%I64u] finished\n", pEntry->id));

	return status;
}



void devctrl_copyUdpConnInfo(PUDPCTX pUdpCtx, PNF_UDP_CONN_INFO pInfo)
{
	pInfo->processId = pUdpCtx->processId;
	pInfo->ip_family = pUdpCtx->ip_family;

	memcpy(pInfo->localAddress, pUdpCtx->localAddr, NF_MAX_ADDRESS_LENGTH);
}

void devctrl_copyUdpConnRequest(PUDPCTX pUdpCtx, PNF_UDP_CONN_REQUEST pInfo)
{
	pInfo->filteringFlag = pUdpCtx->filteringFlag;
	pInfo->processId = pUdpCtx->processId;
	pInfo->ip_family = pUdpCtx->ip_family;

	memcpy(pInfo->localAddress, pUdpCtx->localAddr, NF_MAX_ADDRESS_LENGTH);
	memcpy(pInfo->remoteAddress, pUdpCtx->remoteAddr, NF_MAX_ADDRESS_LENGTH);
}

NTSTATUS devctrl_copyUdpPacket(PUDPCTX pUdpCtx, int code, UINT64 * pOffset)
{
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG		dataSize;
	ULONG		extraBytes;
	ULONG		offset = 0;
	PNF_DATA	pData;
	PNF_UDP_PACKET	pPacket = NULL;
	NF_UDP_OPTIONS * pOptions;
	KLOCK_QUEUE_HANDLE lh;
	ULONG		packetDataLength = 0;

	sl_lock(&pUdpCtx->lock, &lh);

	for (;;)
	{

		if (pUdpCtx->filteringFlag & NF_SUSPENDED) 
		{
			KdPrint((DPREFIX"devctrl_copyUdpPacket[%I64u]: suspended connection\n", pUdpCtx->id));
			break;
		}

		if (code == NF_UDP_SEND)
		{
			if (IsListEmpty(&pUdpCtx->pendedSendPackets))
			{
				break;
			}

			pPacket = (PNF_UDP_PACKET)RemoveHeadList(&pUdpCtx->pendedSendPackets);
		} else
		{
			if (IsListEmpty(&pUdpCtx->pendedRecvPackets))
			{
				break;
			}

			pPacket = (PNF_UDP_PACKET)RemoveHeadList(&pUdpCtx->pendedRecvPackets);
		}

		extraBytes = NF_MAX_ADDRESS_LENGTH + 
			sizeof(NF_UDP_OPTIONS) - 1 + 
			sizeof(NF_UDP_PACKET_OPTIONS) +
			pPacket->options.controlDataLength;

		packetDataLength = pPacket->dataLength;

		dataSize = sizeof(NF_DATA) - 1 + packetDataLength + extraBytes;

		if ((g_inBuf.bufferLength - *pOffset) < dataSize)
		{
			status = STATUS_NO_MEMORY;
			break;
		}

		pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

		pData->code = code;
		pData->id = pUdpCtx->id;
		pData->bufferSize = packetDataLength + extraBytes;

		memcpy(pData->buffer + offset, pPacket->remoteAddr, NF_MAX_ADDRESS_LENGTH);
		offset += NF_MAX_ADDRESS_LENGTH;

		pOptions = (NF_UDP_OPTIONS *)(pData->buffer + offset);	
		
		pOptions->flags = 0;
		pOptions->optionsLength = sizeof(NF_UDP_PACKET_OPTIONS) + 
			pPacket->options.controlDataLength +
			pPacket->options.transportHeaderLength;

		offset += sizeof(NF_UDP_OPTIONS) - 1;

		memcpy(pData->buffer + offset, &pPacket->options, sizeof(NF_UDP_PACKET_OPTIONS));

		offset += sizeof(NF_UDP_PACKET_OPTIONS);

		if (pPacket->controlData && pPacket->options.controlDataLength > 0)
		{
			memcpy(pData->buffer + offset, pPacket->controlData, pPacket->options.controlDataLength);
			offset += pPacket->options.controlDataLength;
		}

		if (pPacket->dataBuffer)
		{
			memcpy(pData->buffer + offset, pPacket->dataBuffer, packetDataLength);
		}

		*pOffset += dataSize;

		break;
	}

	sl_unlock(&lh);

	if (pPacket)
	{
		if (NT_SUCCESS(status))
		{
			sl_lock(&pUdpCtx->lock, &lh);
		
			if (code == NF_UDP_SEND)
			{
				if (pUdpCtx->pendedSendBytes >= (ULONG)pPacket->dataLength)
				{
					pUdpCtx->pendedSendBytes -= (ULONG)pPacket->dataLength;
				} else
				{
					pUdpCtx->pendedSendBytes = 0;
					ASSERT(0);
				}
				KdPrint((DPREFIX"devctrl_copyUdpPacket[%I64u] pendedSendBytes=%u\n", 
					pUdpCtx->id, pUdpCtx->pendedSendBytes));
			} else
			{
				if (pUdpCtx->pendedRecvBytes >= (ULONG)pPacket->dataLength)
				{
					pUdpCtx->pendedRecvBytes -= (ULONG)pPacket->dataLength;
				} else
				{
					pUdpCtx->pendedRecvBytes = 0;
					ASSERT(0);
				}

				KdPrint((DPREFIX"devctrl_copyUdpPacket[%I64u] pendedRecvBytes=%u\n", 
					pUdpCtx->id, pUdpCtx->pendedRecvBytes));
			}

			sl_unlock(&lh);

			udpctx_freePacket(pPacket);
		} else
		{
			sl_lock(&pUdpCtx->lock, &lh);
			if (code == NF_UDP_SEND)
			{
				InsertHeadList(&pUdpCtx->pendedSendPackets, &pPacket->entry);
			} else
			{
				InsertHeadList(&pUdpCtx->pendedRecvPackets, &pPacket->entry);
			}
			sl_unlock(&lh);
		}
	}

	return status;

}

NTSTATUS devctrl_popUdpData(PNF_QUEUE_ENTRY pEntry, UINT64 * pOffset)
{
	NTSTATUS	status = STATUS_SUCCESS;
	UINT64		dataSize;
	UINT64		bytesAvailable;
	PNF_DATA	pData;

	KdPrint((DPREFIX"devctrl_popUdpData[%I64u]\n", pEntry->id));

	bytesAvailable = g_inBuf.bufferLength - *pOffset;

	switch (pEntry->code)
	{
	case NF_UDP_CREATED:
		{
			dataSize = sizeof(NF_DATA) - 1 + sizeof(NF_UDP_CONN_INFO);

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = sizeof(NF_UDP_CONN_INFO);

			if (pEntry->context.UDP.udpCtx && (pEntry->id == pEntry->context.UDP.udpCtx->id))
			{
				devctrl_copyUdpConnInfo(pEntry->context.UDP.udpCtx, (PNF_UDP_CONN_INFO)pData->buffer);
				udpctx_release(pEntry->context.UDP.udpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_UDP_CLOSED:
		{
			PUDPCTX pUdpCtx = NULL;
		
			dataSize = sizeof(NF_DATA) - 1 + sizeof(NF_FLOWCTL_STAT);

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = sizeof(NF_FLOWCTL_STAT);

			pUdpCtx = pEntry->context.UDP.udpCtx;
			if (pUdpCtx)
			{
				PNF_FLOWCTL_STAT pStat = (PNF_FLOWCTL_STAT)pData->buffer;
				
				pStat->inBytes = pUdpCtx->inCounterTotal;
				pStat->outBytes = pUdpCtx->outCounterTotal;
				
				udpctx_release(pUdpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_UDP_CONNECT_REQUEST:
		{
			dataSize = sizeof(NF_DATA) - 1 + sizeof(NF_UDP_CONN_REQUEST);

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = sizeof(NF_UDP_CONN_REQUEST);

			if (pEntry->context.UDP.udpCtx && (pEntry->id == pEntry->context.UDP.udpCtx->id))
			{
				devctrl_copyUdpConnRequest(pEntry->context.UDP.udpCtx, (PNF_UDP_CONN_REQUEST)pData->buffer);
				udpctx_release(pEntry->context.UDP.udpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_UDP_CAN_SEND:
	case NF_UDP_CAN_RECEIVE:
		{
			dataSize = sizeof(NF_DATA) - 1;

			if (bytesAvailable < dataSize)
			{
				return STATUS_NO_MEMORY;
			}

			pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

			pData->code = pEntry->code;
			pData->id = pEntry->id;
			pData->bufferSize = 0;
			
			if (pEntry->context.UDP.udpCtx && (pEntry->id == pEntry->context.UDP.udpCtx->id))
			{
				udpctx_release(pEntry->context.UDP.udpCtx);
			}

			*pOffset += dataSize;
			
			return STATUS_SUCCESS;
		}

	case NF_UDP_RECEIVE:
	case NF_UDP_SEND:
		{
			if (pEntry->context.UDP.udpCtx && (pEntry->id == pEntry->context.UDP.udpCtx->id))
			{
				status = devctrl_copyUdpPacket(pEntry->context.UDP.udpCtx, pEntry->code, pOffset);
				if (NT_SUCCESS(status))
				{
					udpctx_release(pEntry->context.UDP.udpCtx);
				}
			}
			return status;
		}
	}

	KdPrint((DPREFIX"devctrl_popUdpData[%I64u] finished\n", pEntry->id));

	return status;
}

NTSTATUS devctrl_copyIPPacket(UINT64 * pOffset)
{
	NTSTATUS	status = STATUS_SUCCESS;
	ULONG		dataSize;
	PNF_DATA	pData;
	PNF_IP_PACKET	pPacket = NULL;
	KLOCK_QUEUE_HANDLE lh;
	ULONG		packetDataLength = 0;
	PIPCTX		pCtx;

	pCtx = ipctx_get();

	sl_lock(&pCtx->lock, &lh);

	for (;;)
	{
		if (IsListEmpty(&pCtx->pendedPackets))
		{
			break;
		}

		pPacket = (PNF_IP_PACKET)RemoveHeadList(&pCtx->pendedPackets);

		packetDataLength = pPacket->dataLength;

		dataSize = sizeof(NF_DATA) - 1 + sizeof(NF_IP_PACKET_OPTIONS) + packetDataLength;

		if ((g_inBuf.bufferLength - *pOffset) < dataSize)
		{
			status = STATUS_NO_MEMORY;
			break;
		}

		pData = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);

		pData->code = (pPacket->isSend)? NF_IP_SEND : NF_IP_RECEIVE;
		pData->id = 0;
		pData->bufferSize = sizeof(NF_IP_PACKET_OPTIONS) + packetDataLength;

		memcpy(pData->buffer, &pPacket->options, sizeof(NF_IP_PACKET_OPTIONS));

		if (pPacket->dataBuffer)
		{
			memcpy(pData->buffer + sizeof(NF_IP_PACKET_OPTIONS), pPacket->dataBuffer, packetDataLength);
		}

		*pOffset += dataSize;

		if (pPacket->isSend)
		{
			if (pCtx->pendedSendBytes >= (ULONG)pPacket->dataLength)
			{
				pCtx->pendedSendBytes -= (ULONG)pPacket->dataLength;
			} else
			{
				pCtx->pendedSendBytes = 0;
				ASSERT(0);
			}
			KdPrint((DPREFIX"devctrl_copyIPPacket pendedSendBytes=%u\n", 
				pCtx->pendedSendBytes));
		} else
		{
			if (pCtx->pendedRecvBytes >= (ULONG)pPacket->dataLength)
			{
				pCtx->pendedRecvBytes -= (ULONG)pPacket->dataLength;
			} else
			{
				pCtx->pendedRecvBytes = 0;
				ASSERT(0);
			}

			KdPrint((DPREFIX"devctrl_copyIPPacket pendedRecvBytes=%u\n", 
				pCtx->pendedRecvBytes));
		}

		break;
	}

	sl_unlock(&lh);

	if (pPacket)
	{
		if (NT_SUCCESS(status))
		{
			ipctx_freePacket(pPacket);
		} else
		{
			sl_lock(&pCtx->lock, &lh);
			InsertHeadList(&pCtx->pendedPackets, &pPacket->entry);
			sl_unlock(&lh);
		}
	}

	return status;

}

NTSTATUS devctrl_popIPData(PNF_QUEUE_ENTRY pEntry, UINT64 * pOffset)
{
	NTSTATUS	status = STATUS_SUCCESS;
//	UINT64		dataSize;
//	UINT64		bytesAvailable;
//	PNF_DATA	pData;

	KdPrint((DPREFIX"devctrl_popIPData\n"));

//	bytesAvailable = g_inBuf.bufferLength - *pOffset;

	switch (pEntry->code)
	{
	case NF_IP_RECEIVE:
	case NF_IP_SEND:
		{
			status = devctrl_copyIPPacket(pOffset);
			return status;
		}
	}

	KdPrint((DPREFIX"devctrl_popIPData finished\n"));

	return status;
}


UINT64 devctrl_fillBuffer()
{
	PNF_QUEUE_ENTRY	pEntry;
	UINT64		offset = 0;
	NTSTATUS	status = STATUS_SUCCESS;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slIoQueue, &lh);

	while (!IsListEmpty(&g_ioQueue))
	{
		pEntry = (PNF_QUEUE_ENTRY)RemoveHeadList(&g_ioQueue);
	
		sl_unlock(&lh);

		switch (pEntry->code)
		{
		case NF_TCP_CONNECTED:
		case NF_TCP_CLOSED:
		case NF_TCP_RECEIVE:
		case NF_TCP_SEND:
		case NF_TCP_CAN_SEND:
		case NF_TCP_CAN_RECEIVE:
		case NF_TCP_CONNECT_REQUEST:
		case NF_TCP_REINJECT:
		case NF_TCP_DEFERRED_DISCONNECT:
			status = devctrl_popTcpData(pEntry, &offset);
			break;

		case NF_UDP_CREATED:
		case NF_UDP_CLOSED:
		case NF_UDP_RECEIVE:
		case NF_UDP_SEND:
		case NF_UDP_CAN_SEND:
		case NF_UDP_CAN_RECEIVE:
		case NF_UDP_CONNECT_REQUEST:
			status = devctrl_popUdpData(pEntry, &offset);
			break;

		case NF_IP_RECEIVE:
		case NF_IP_SEND:
			status = devctrl_popIPData(pEntry, &offset);
			break;

		default:
			// EDR_FIX: Send data to user mode API
			if (pEntry->code & 0x10000)
			{
				status = cmdedr_popCustomDataInt(pEntry, &offset);
				break;
			}

			{
				ASSERT(0);
				status = STATUS_SUCCESS;
			}
		}

		sl_lock(&g_slIoQueue, &lh);
	
		if (!NT_SUCCESS(status))
		{
			InsertHeadList(&g_ioQueue, &pEntry->entry);
			break;
		}

		ExFreeToNPagedLookasideList( &g_ioQueueList, pEntry );
	}

	sl_unlock(&lh);

	return offset;
}

void devctrl_serviceReads()
{
    PIRP                irp = NULL;
    PLIST_ENTRY         pIrpEntry;
    BOOLEAN             foundPendingIrp = FALSE;
	PNF_READ_RESULT		pResult;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slIoQueue, &lh);

    if (IsListEmpty(&g_pendedIoRequests) || IsListEmpty(&g_ioQueue))
    {
		sl_unlock(&lh);
        return;
	}

    //
    //  Get the first pended Read IRP
    //
    pIrpEntry = g_pendedIoRequests.Flink;
    while (pIrpEntry != &g_pendedIoRequests)
    {
        irp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(irp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            RemoveEntryList(pIrpEntry);
            foundPendingIrp = TRUE;
            break;
        }
        else
        {
            //
            //  The IRP is being cancelled; let the cancel routine handle it.
            //
            KdPrint((DPREFIX"devctrl_serviceReads: skipping cancelled IRP\n"));
            pIrpEntry = pIrpEntry->Flink;
        }
    }

	sl_unlock(&lh);

    if (!foundPendingIrp)
    {
        return;
    }

    pResult = (PNF_READ_RESULT)MmGetSystemAddressForMdlSafe(irp->MdlAddress, HighPagePriority);
	if (!pResult)
	{
		irp->IoStatus.Information = 0;
		irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return;
	}

	pResult->length = devctrl_fillBuffer();

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = sizeof(NF_READ_RESULT);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
}

void devctrl_cancelPendingReads()
{
    PIRP                irp;
    PLIST_ENTRY         pIrpEntry;
	KLOCK_QUEUE_HANDLE lh;

	sl_lock(&g_slIoQueue, &lh);
	
	while (!IsListEmpty(&g_pendedIoRequests))
    {
        //
        //  Get the first pended Read IRP
        //
		pIrpEntry = g_pendedIoRequests.Flink;
        irp = CONTAINING_RECORD(pIrpEntry, IRP, Tail.Overlay.ListEntry);

        //
        //  Check to see if it is being cancelled.
        //
        if (IoSetCancelRoutine(irp, NULL))
        {
            //
            //  It isn't being cancelled, and can't be cancelled henceforth.
            //
            RemoveEntryList(pIrpEntry);

			sl_unlock(&lh);

            //
            //  Complete the IRP.
            //
            irp->IoStatus.Status = STATUS_CANCELLED;
            irp->IoStatus.Information = 0;
            IoCompleteRequest(irp, IO_NO_INCREMENT);

			sl_lock(&g_slIoQueue, &lh);
        }
        else
        {
            //
            //  It is being cancelled, let the cancel routine handle it.
            //
			sl_unlock(&lh);

            //
            //  Give the cancel routine some breathing space, otherwise
            //  we might end up examining the same (cancelled) IRP over
            //  and over again.
            //
			devctrl_sleep(1000);

			sl_lock(&g_slIoQueue, &lh);
        }
    }

	sl_unlock(&lh);
}



VOID
devctrl_cancelRead(
    IN PDEVICE_OBJECT               deviceObject,
    IN PIRP                         irp
    )
{
	KLOCK_QUEUE_HANDLE lh;

    UNREFERENCED_PARAMETER(deviceObject);

    IoReleaseCancelSpinLock(irp->CancelIrql);

	sl_lock(&g_slIoQueue, &lh);

    RemoveEntryList(&irp->Tail.Overlay.ListEntry);

	sl_unlock(&lh);

    irp->IoStatus.Status = STATUS_CANCELLED;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}


NTSTATUS devctrl_read(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	NTSTATUS status = STATUS_SUCCESS;
	KLOCK_QUEUE_HANDLE lh;

    for (;;)
    {
        if (irp->MdlAddress == NULL)
        {
            KdPrint((DPREFIX"devctrl_read: NULL MDL address\n"));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        if (MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority) == NULL ||
			irpSp->Parameters.Read.Length < sizeof(NF_READ_RESULT))
        {
            KdPrint((DPREFIX"devctrl_read: Invalid request\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
    
		sl_lock(&g_slIoQueue, &lh);

        IoSetCancelRoutine(irp, devctrl_cancelRead);

        if (irp->Cancel &&                              
            IoSetCancelRoutine(irp, NULL)) 
        {            
            //
            // IRP has been canceled but the I/O manager did not manage to call our cancel routine. This
            // code is safe referencing the Irp->Cancel field without locks because of the memory barriers
            // in the interlocked exchange sequences used by IoSetCancelRoutine.
            //
        
            status = STATUS_CANCELLED;                   
            // IRP should be completed after releasing the lock
        } 
        else 
        {
            //
            //  Add this IRP to the list of pended Read IRPs
            //
            IoMarkIrpPending(irp);

			InsertTailList(&g_pendedIoRequests, &irp->Tail.Overlay.ListEntry);

            status = STATUS_PENDING;
        }
        
		sl_unlock(&lh);

		KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);

		break;
	}

    if (status != STATUS_PENDING)
    {
        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return status;
}

int devctrl_getListSize(PLIST_ENTRY pList)
{
	int listSize;
	PLIST_ENTRY e;

	listSize = 0;
	e = pList->Flink;
	while (e != pList)
	{
		listSize++;
		e = e->Flink;
	}
	return listSize;
}


ULONG 
devctrl_setTcpConnState(PNF_DATA pData)
{
	PTCPCTX pTcpCtx = NULL;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_setTcpConnState[%I64u]: %d\n", pData->id, pData->code));

	pTcpCtx = tcpctx_find(pData->id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"devctrl_setTcpConnState[%I64u]: TCPCTX not found\n", pData->id));
		return 0;
	}

	sl_lock(&pTcpCtx->lock, &lh);
	
	switch (pData->code)
	{
	case NF_TCP_REQ_SUSPEND:
		{
			KdPrint((DPREFIX"devctrl_setTcpConnState[%I64u]: connection suspended\n", pData->id));
			if (pTcpCtx->filteringFlag & NF_FILTER)
			{
				pTcpCtx->filteringFlag |= NF_SUSPENDED;
			}
		}
		break;
	case NF_TCP_REQ_RESUME:
		{
			int i, listSize;

			if ((pTcpCtx->filteringFlag & NF_FILTER) && (pTcpCtx->filteringFlag & NF_SUSPENDED))
			{
				KdPrint((DPREFIX"devctrl_setTcpConnState[%I64u]: connection resumed\n", pData->id));
		
				pTcpCtx->filteringFlag &= ~NF_SUSPENDED;

				listSize = devctrl_getListSize(&pTcpCtx->pendedPackets);

				sl_unlock(&lh);
			
				for (i=0; i<listSize; i++)
				{
					devctrl_pushTcpData(pTcpCtx->id, NF_TCP_RECEIVE, pTcpCtx, NULL);
				}

				sl_lock(&pTcpCtx->lock, &lh);
			}
		}
		break;
	}

	sl_unlock(&lh);

	tcpctx_release(pTcpCtx);

	return sizeof(NF_DATA) - 1;
}

ULONG devctrl_removeClosed(HASH_ID id) 
{
	PTCPCTX pTcpCtx = NULL;
	
	KdPrint((DPREFIX"devctrl_removeClosed[%I64u]\n", id));

	pTcpCtx = tcpctx_find(id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"devctrl_removeClosed[%I64u]: TCPCTX not found\n", id));
		return 0;
	}

	devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CLOSED, pTcpCtx, NULL);
	
	tcpctx_release(pTcpCtx);
	tcpctx_release(pTcpCtx);

	return sizeof(NF_DATA) - 1;
}


NTSTATUS devctrl_pushTcpInject(PTCPCTX pTcpCtx)
{
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_pushTcpInject id=%I64d\n", pTcpCtx->id));

	if (!devctrl_isProxyAttached())
		return STATUS_UNSUCCESSFUL;

	tcpctx_addRef(pTcpCtx);

	sl_lock(&g_slTcpInjectQueue, &lh);

	if (pTcpCtx->inInjectQueue)
	{
		sl_unlock(&lh);
		tcpctx_release(pTcpCtx);
	    KeSetEvent(&g_threadIoEvent, IO_NO_INCREMENT, FALSE);
		return STATUS_SUCCESS;
	}

	pTcpCtx->inInjectQueue = TRUE;

	InsertTailList(&g_tcpInjectQueue, &pTcpCtx->injectQueueEntry);

	sl_unlock(&lh);

    KeSetEvent(&g_threadIoEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_SUCCESS;
}

void NTAPI 
devctrl_tcpInjectCompletion(
   IN void* context,
   IN OUT NET_BUFFER_LIST* netBufferList,
   IN BOOLEAN dispatchLevel
   )
{
	PNF_PACKET pPacket = (PNF_PACKET)context;
	PTCPCTX pTcpCtx;
	KLOCK_QUEUE_HANDLE lh;

	UNREFERENCED_PARAMETER(dispatchLevel);
	UNREFERENCED_PARAMETER(netBufferList);

	pTcpCtx = pPacket->pTcpCtx;

	KdPrint((DPREFIX"devctrl_tcpInjectCompletion[%I64u]\n", pTcpCtx->id));

	sl_lock(&pTcpCtx->lock, &lh);
	if (pPacket->streamData.flags & FWPS_STREAM_FLAG_SEND)
	{
		if (pTcpCtx->injectedSendBytes >= (ULONG)pPacket->streamData.dataLength)
		{
			pTcpCtx->injectedSendBytes -= (ULONG)pPacket->streamData.dataLength;
		} else
		{
			pTcpCtx->injectedSendBytes = 0;
		}
		if (pTcpCtx->injectedSendBytes <= TCP_SEND_INJECT_LIMIT)
		{
			pTcpCtx->sendInProgress = FALSE;
		}
		
		pTcpCtx->outCounter += (ULONG)pPacket->streamData.dataLength;
		pTcpCtx->outCounterTotal += (ULONG)pPacket->streamData.dataLength;
	} else
	{
		if (pTcpCtx->injectedRecvBytes >= (ULONG)pPacket->streamData.dataLength)
		{
			pTcpCtx->injectedRecvBytes -= (ULONG)pPacket->streamData.dataLength;
		} else
		{
			pTcpCtx->injectedRecvBytes = 0;
		}
		if (pTcpCtx->injectedRecvBytes <= TCP_RECV_INJECT_LIMIT)
		{
			pTcpCtx->recvInProgress = FALSE;
		}
		
		pTcpCtx->inCounter += (ULONG)pPacket->streamData.dataLength;
		pTcpCtx->inCounterTotal += (ULONG)pPacket->streamData.dataLength;
	}

	sl_unlock(&lh);

	if (pPacket->streamData.flags & FWPS_STREAM_FLAG_SEND)
	{
		if (pTcpCtx->fcHandle)
		{
			flowctl_update(pTcpCtx->fcHandle, TRUE, pPacket->streamData.dataLength);
			pTcpCtx->outLastTS = flowctl_getTickCount();
		}

		devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CAN_SEND, NULL, NULL);
	} else
	{
		if (pTcpCtx->fcHandle)
		{
			flowctl_update(pTcpCtx->fcHandle, FALSE, pPacket->streamData.dataLength);
			pTcpCtx->inLastTS = flowctl_getTickCount();
		}

		devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CAN_RECEIVE, NULL, NULL);
	}

	tcpctx_release(pTcpCtx);

	tcpctx_freePacket(pPacket);

	InterlockedDecrement(&g_injectCount);
	KdPrint((DPREFIX"g_injectCount=%d\n", g_injectCount));
}

void NTAPI 
devctrl_tcpCloneInjectCompletion(
   IN void* context,
   IN OUT NET_BUFFER_LIST* netBufferList,
   IN BOOLEAN dispatchLevel
   )
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(dispatchLevel);

	KdPrint((DPREFIX"devctrl_tcpCloneInjectCompletion\n"));

    FwpsFreeCloneNetBufferList(netBufferList, 0);
}


void devctrl_tcpInjectPackets(PTCPCTX pTcpCtx)
{
	KLOCK_QUEUE_HANDLE lh;
	NF_PACKET * pPacket;
	LIST_ENTRY packets;
	NTSTATUS status;
	PNET_BUFFER_LIST pnbl;
	
	KdPrint((DPREFIX"devctrl_tcpInjectPackets[%I64u]\n", pTcpCtx->id));

	sl_lock(&pTcpCtx->lock, &lh);
	
	if (pTcpCtx->closed)
	{
		KdPrint((DPREFIX"devctrl_tcpInjectPackets[%I64u]: closed connection\n", pTcpCtx->id));
		sl_unlock(&lh);
		return;
	}

	InitializeListHead(&packets);

	while (!IsListEmpty(&pTcpCtx->injectPackets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&pTcpCtx->injectPackets);
		InsertTailList(&packets, &pPacket->entry);
	}

	sl_unlock(&lh);

	while (!IsListEmpty(&packets))
	{
		pPacket = (PNF_PACKET)RemoveHeadList(&packets);

		if (pPacket->isClone)
		{
			if (pPacket->streamData.flags & FWPS_STREAM_FLAG_SEND)
			{
				pPacket->streamData.flags |= FWPS_STREAM_FLAG_SEND_NODELAY;
			} else
			if ((pPacket->streamData.flags & FWPS_STREAM_FLAG_RECEIVE) &&
				!(pPacket->streamData.flags & FWPS_STREAM_FLAG_RECEIVE_DISCONNECT))
			{
				pPacket->streamData.flags |= FWPS_STREAM_FLAG_RECEIVE_PUSH;
			}

			__try {
				status = FwpsStreamInjectAsync(g_injectionHandle,
											 0,
											 0,
											 pTcpCtx->flowHandle,
											 pPacket->calloutId,
											 pTcpCtx->layerId,
											 pPacket->streamData.flags,
											 pPacket->streamData.netBufferListChain,
											 pPacket->streamData.dataLength,
											 (FWPS_INJECT_COMPLETE)devctrl_tcpCloneInjectCompletion,
											 0);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				status = STATUS_UNSUCCESSFUL;
			}

			KdPrint((DPREFIX"devctrl_tcpInjectPackets[%I64u] clone inject status=%x\n", pTcpCtx->id, status));

			if (status == STATUS_SUCCESS && pPacket->streamData.netBufferListChain)
			{
				pPacket->streamData.netBufferListChain = NULL;
			}

			tcpctx_freePacket(pPacket);
		} else
		{
			pnbl = pPacket->streamData.netBufferListChain;

			if (pnbl)
			{
				tcpctx_addRef(pTcpCtx);
			}
		
			InterlockedIncrement(&g_injectCount);
			KdPrint((DPREFIX"g_injectCount=%d\n", g_injectCount));
		
			__try {

				status = FwpsStreamInjectAsync(g_injectionHandle,
											 0,
											 0,
											 pTcpCtx->flowHandle,
											 pPacket->calloutId,
											 pTcpCtx->layerId,
											 pPacket->streamData.flags,
											 pPacket->streamData.netBufferListChain,
											 pPacket->streamData.dataLength,
											 (FWPS_INJECT_COMPLETE)devctrl_tcpInjectCompletion,
											 pPacket);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				status = STATUS_UNSUCCESSFUL;
			}

			KdPrint((DPREFIX"devctrl_tcpInjectPackets[%I64u] inject status=%x\n", pTcpCtx->id, status));

			if ((status != STATUS_SUCCESS) || !pnbl)
			{
				if (pPacket->streamData.flags & FWPS_STREAM_FLAG_SEND)
				{
					devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CAN_SEND, NULL, NULL);
				} else
				{
					devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CAN_RECEIVE, NULL, NULL);
				}

				if (pnbl)
				{
					tcpctx_release(pTcpCtx);
				}

				tcpctx_freePacket(pPacket);

				InterlockedDecrement(&g_injectCount);
			
				KdPrint((DPREFIX"g_injectCount=%d\n", g_injectCount));
			}
		}
	}
	
}

void devctrl_injectThread(IN PVOID StartContext)
{
	KLOCK_QUEUE_HANDLE lh;
	PLIST_ENTRY	pEntry;
	PTCPCTX		pTcpCtx;

	UNREFERENCED_PARAMETER(StartContext);

	KdPrint((DPREFIX"devctrl_injectThread\n"));

	for(;;)
	{
		KeWaitForSingleObject(
			&g_threadIoEvent,
			 Executive, 
			 KernelMode, 
			 FALSE, 
			 NULL
         );

		if (devctrl_isShutdown())
		{
			break;
		}

		for (;;)
		{
			sl_lock(&g_slTcpInjectQueue, &lh);

			if (IsListEmpty(&g_tcpInjectQueue))
			{
				sl_unlock(&lh);
				break;
			}

			pEntry = RemoveHeadList(&g_tcpInjectQueue);
			pTcpCtx = CONTAINING_RECORD(pEntry, TCPCTX, injectQueueEntry);
			pTcpCtx->inInjectQueue = FALSE;

			sl_unlock(&lh);
		
			devctrl_tcpInjectPackets(pTcpCtx);

			tcpctx_release(pTcpCtx);
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

ULONG devctrl_processTcpPacket(PNF_DATA pData)
{
	PTCPCTX pTcpCtx = NULL;
	KLOCK_QUEUE_HANDLE lh;
	NTSTATUS status = STATUS_SUCCESS;
	PVOID dataCopy = NULL;
	PMDL mdl = NULL;
	NET_BUFFER_LIST* netBufferList = NULL;
	ULONG result = 0;
	BOOLEAN noDelay = FALSE;
	PNF_PACKET pPacket = NULL;

	KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u]: code=%d, size=%d\n", pData->id, pData->code, pData->bufferSize));

	pTcpCtx = tcpctx_find(pData->id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u]: TCPCTX not found\n", pData->id));
		return sizeof(NF_DATA) + pData->bufferSize - 1;
	}

	sl_lock(&pTcpCtx->lock, &lh);

	if (pTcpCtx->closed)
	{
		KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u]: closed connection\n", pData->id));
		sl_unlock(&lh);
		tcpctx_release(pTcpCtx);
		return 0;
	}

	if (pData->bufferSize > 0)
	{
		if ((pData->code == NF_TCP_SEND && pTcpCtx->sendInProgress && !pTcpCtx->sendDisconnectPending) ||
			(pData->code == NF_TCP_RECEIVE && pTcpCtx->recvInProgress && !pTcpCtx->recvDisconnectPending))
		{
			KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u]: operation %d in progress\n", pData->id, pData->code));
			sl_unlock(&lh);
			tcpctx_release(pTcpCtx);
			return 0;
		}
	}

	if (((pData->code == NF_TCP_SEND && pTcpCtx->sendDisconnectCalled) ||
		(pData->code == NF_TCP_RECEIVE && pTcpCtx->recvDisconnectCalled)))
	{
		sl_unlock(&lh);
		tcpctx_release(pTcpCtx);
		KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u]: bypass packet %d\n", pData->id, pData->code));
		return sizeof(NF_DATA) + pData->bufferSize - 1;
	}

	noDelay = pTcpCtx->noDelay;

	sl_unlock(&lh);

	for (;;) 
	{
		pPacket = tcpctx_allocPacket();
		if (pPacket == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] tcpctx_allocPacket failed\n", 
					pTcpCtx->id));
			break;
		}

		if (pData->bufferSize > 0)
		{
			dataCopy = ExAllocatePoolWithTag(NonPagedPool, pData->bufferSize, MEM_TAG_TCP_DATA_COPY);
			if (dataCopy == NULL)
			{
				status = STATUS_NO_MEMORY;
				KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] malloc_np(pData->bufferSize) failed, status=%x, pData->bufferSize=%d\n", 
					pTcpCtx->id, status, pData->bufferSize));
				break;
			}

			memcpy(dataCopy, pData->buffer, pData->bufferSize);

			mdl = IoAllocateMdl(
					dataCopy,
					pData->bufferSize,
					FALSE,
					FALSE,
					NULL);
			if (mdl == NULL)
			{
				status = STATUS_NO_MEMORY;
				KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] IoAllocateMdl failed, status=%x\n", 
					pTcpCtx->id, status));
				break;
			}

			MmBuildMdlForNonPagedPool(mdl);

			status = FwpsAllocateNetBufferAndNetBufferList(
						  g_netBufferListPool,
						  0,
						  0,
						  mdl,
						  0,
						  pData->bufferSize,
						  &netBufferList);
			if (!NT_SUCCESS(status))
			{
				KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] FwpsAllocateNetBufferAndNetBufferList failed, status=%x\n", 
					pTcpCtx->id, status));
				break;
			}
		}

		pPacket->isClone = FALSE;
		pPacket->pTcpCtx = pTcpCtx;
		pPacket->streamData.netBufferListChain = netBufferList;
		pPacket->streamData.dataLength = pData->bufferSize;
		pPacket->streamData.dataOffset.mdl = mdl;

		if (pData->bufferSize == 0)
		{
			pPacket->streamData.flags = (pData->code == NF_TCP_SEND)? 
					FWPS_STREAM_FLAG_SEND | FWPS_STREAM_FLAG_SEND_DISCONNECT : 
					FWPS_STREAM_FLAG_RECEIVE | FWPS_STREAM_FLAG_RECEIVE_DISCONNECT;
		} else
		{
			pPacket->streamData.flags = (pData->code == NF_TCP_SEND)? 
				(FWPS_STREAM_FLAG_SEND | (noDelay? FWPS_STREAM_FLAG_SEND_NODELAY : 0)) : 
							(FWPS_STREAM_FLAG_RECEIVE | FWPS_STREAM_FLAG_RECEIVE_PUSH);
		
		}

		KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] inject, flags=%x\n", pTcpCtx->id, pPacket->streamData.flags));

		sl_lock(&pTcpCtx->lock, &lh);

		if (pData->code == NF_TCP_SEND)
		{
			pTcpCtx->injectedSendBytes += pData->bufferSize;

			KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] inject, pTcpCtx->injectedSendBytes=%d\n", pTcpCtx->id, pTcpCtx->injectedSendBytes));

			if (pTcpCtx->injectedSendBytes > TCP_SEND_INJECT_LIMIT)
			{
				pTcpCtx->sendInProgress = TRUE;
			}
			if (pData->bufferSize == 0)
			{
				pTcpCtx->sendDisconnectCalled = TRUE;
			}
			
			if (pTcpCtx->sendCalloutInjectable && !pTcpCtx->sendCalloutBypass)
			{
				pPacket->calloutId = pTcpCtx->sendCalloutId;
			} else
			{
				pPacket->calloutId = pTcpCtx->recvCalloutId;
				pTcpCtx->sendCalloutBypass = TRUE;
			}
		} else
		{
			pTcpCtx->injectedRecvBytes += pData->bufferSize;

			KdPrint((DPREFIX"devctrl_processTcpPacket[%I64u] inject, pTcpCtx->injectedRecvBytes=%d\n", pTcpCtx->id, pTcpCtx->injectedRecvBytes));

			if (pTcpCtx->injectedRecvBytes > TCP_RECV_INJECT_LIMIT)
			{
				pTcpCtx->recvInProgress = TRUE;
			}
			if (pData->bufferSize == 0)
			{
				pTcpCtx->recvDisconnectCalled = TRUE;
			}

			if (pTcpCtx->recvCalloutInjectable && !pTcpCtx->recvCalloutBypass)
			{
				pPacket->calloutId = pTcpCtx->recvCalloutId;
			} else
			{
				pPacket->calloutId = pTcpCtx->sendCalloutId;
				pTcpCtx->recvCalloutBypass = TRUE;
			}
		}

		InsertTailList(&pTcpCtx->injectPackets, &pPacket->entry);

		sl_unlock(&lh);

		devctrl_pushTcpInject(pTcpCtx);

		if (pData->code == NF_TCP_SEND)
		{
			devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CAN_SEND, NULL, NULL);
		} else
		{
			devctrl_pushTcpData(pTcpCtx->id, NF_TCP_CAN_RECEIVE, NULL, NULL);
		}

		result = sizeof(NF_DATA) + pData->bufferSize - 1;

		break;
	}

	if (!NT_SUCCESS(status))
	{
		if (netBufferList != NULL)
		{
			FwpsFreeNetBufferList(netBufferList);
		}
		if (mdl != NULL)
		{
			IoFreeMdl(mdl);
		}
		if (dataCopy != NULL)
		{
			free_np(dataCopy);
		}
		if (pPacket != NULL)
		{
			tcpctx_freePacket(pPacket);
		}
	}

	tcpctx_release(pTcpCtx);

	return result;
}


void NTAPI 
devctrl_udpInjectCompletion(
   IN void* context,
   IN OUT NET_BUFFER_LIST* netBufferList,
   IN BOOLEAN dispatchLevel
   )
{
   PNF_UDP_INJECT_CONTEXT inject_context = (PNF_UDP_INJECT_CONTEXT)context;
   PUDPCTX pUdpCtx;
   KLOCK_QUEUE_HANDLE lh;

   UNREFERENCED_PARAMETER(dispatchLevel);

   if (netBufferList)
   {
		PMDL mdl = NET_BUFFER_FIRST_MDL(NET_BUFFER_LIST_FIRST_NB(netBufferList));

		KdPrint((DPREFIX"devctrl_udpInjectCompletion[%I64u]: status=%x\n", inject_context->id, netBufferList->Status));

		if (mdl != inject_context->mdl)
		{
			IoFreeMdl(mdl);
		}

		FwpsFreeNetBufferList(netBufferList);
   }

   if (inject_context != NULL)
   {
		if (inject_context->mdl != NULL)
		{
			free_np(inject_context->mdl->MappedSystemVa);
			IoFreeMdl(inject_context->mdl);
			inject_context->mdl = NULL;
		}

		pUdpCtx = inject_context->pUdpCtx;

		KdPrint((DPREFIX"devctrl_udpInjectCompletion[%I64u]\n", pUdpCtx->id));

		sl_lock(&pUdpCtx->lock, &lh);
		if (inject_context->code == NF_UDP_SEND)
		{
			pUdpCtx->injectedSendBytes -= inject_context->pPacket->dataLength;
			if (pUdpCtx->injectedSendBytes <= UDP_PEND_LIMIT)
			{
				pUdpCtx->sendInProgress = FALSE;
			}
	
			pUdpCtx->outCounter += inject_context->pPacket->dataLength;
			pUdpCtx->outCounterTotal += inject_context->pPacket->dataLength;
		} else
		{
			pUdpCtx->injectedRecvBytes -= inject_context->pPacket->dataLength;
			if (pUdpCtx->injectedRecvBytes <= UDP_PEND_LIMIT)
			{
				pUdpCtx->recvInProgress = FALSE;
			}

			pUdpCtx->inCounter += inject_context->pPacket->dataLength;
			pUdpCtx->inCounterTotal += inject_context->pPacket->dataLength;
		}
		sl_unlock(&lh);

		if (inject_context->code == NF_UDP_SEND)
		{
			if (pUdpCtx->fcHandle)
			{
				flowctl_update(pUdpCtx->fcHandle, TRUE, inject_context->pPacket->dataLength);
				pUdpCtx->outLastTS = flowctl_getTickCount();
			}

			devctrl_pushUdpData(inject_context->id, NF_UDP_CAN_SEND, NULL, NULL);
		} else
		{
			if (pUdpCtx->fcHandle)
			{
				flowctl_update(pUdpCtx->fcHandle, FALSE, inject_context->pPacket->dataLength);
				pUdpCtx->inLastTS = flowctl_getTickCount();
			}

			devctrl_pushUdpData(inject_context->id, NF_UDP_CAN_RECEIVE, NULL, NULL);
		}

		if (inject_context->pPacket)
		{
			udpctx_freePacket(inject_context->pPacket);
		}

		udpctx_release(pUdpCtx);

		ExFreeToNPagedLookasideList( &g_udpInjectContextLAList, inject_context );
   }
}


ULONG devctrl_processUdpPacket(PNF_DATA pData)
{
	PUDPCTX pUdpCtx = NULL;
	KLOCK_QUEUE_HANDLE lh;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PVOID dataCopy = NULL;
	PMDL mdl = NULL;
	NET_BUFFER_LIST* netBufferList = NULL;
	PNF_UDP_INJECT_CONTEXT context = NULL;
	PNF_UDP_OPTIONS pOptions;
	UINT16		dataLength;
	ULONG		extraBytes;
	NET_BUFFER* netBuffer = NULL;
	PNF_UDP_PACKET pPacket = NULL;
	UDP_HEADER* udpHeader;
	ULONG	offset = 0;
	ULONG	result = 0;
	UCHAR * pRemoteAddr = NULL;
	UCHAR * pLocalAddr = NULL;

	KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: code=%d, size=%d\n", pData->id, pData->code, pData->bufferSize));

	if (pData->bufferSize < NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS))
	{
		KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: invalid request\n", pData->id));
		return sizeof(NF_DATA) - 1 + pData->bufferSize;
	}

	pUdpCtx = udpctx_find(pData->id);
	if (!pUdpCtx)
	{
		KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: UDPCTX not found\n", pData->id));

		if (pData->code != NF_UDP_SEND)
		{
			return sizeof(NF_DATA) -1 + pData->bufferSize;
		}

		pUdpCtx = udpctx_alloc(0);
	}

	sl_lock(&pUdpCtx->lock, &lh);

	if (pUdpCtx->closed)
	{
		KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: closed connection\n", pData->id));
		sl_unlock(&lh);
		udpctx_release(pUdpCtx);
		return sizeof(NF_DATA) -1 + pData->bufferSize;
	}

	if ((pData->code == NF_UDP_SEND && pUdpCtx->sendInProgress) ||
		(pData->code == NF_UDP_RECEIVE && pUdpCtx->recvInProgress))
	{
		KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: operation %d in progress\n", pData->id, pData->code));
		sl_unlock(&lh);
		udpctx_release(pUdpCtx);
		return 0;
	}

	sl_unlock(&lh);

	for (;;) 
	{
		pPacket = udpctx_allocPacket(0);
		if (!pPacket)
		{
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] udpctx_allocPacket failed\n", 
					pUdpCtx->id));
			status = STATUS_NO_MEMORY;
			break;
		}

		offset = 0;

		memcpy(pPacket->remoteAddr, pData->buffer + offset, NF_MAX_ADDRESS_LENGTH);

		offset += NF_MAX_ADDRESS_LENGTH;

		pOptions = (PNF_UDP_OPTIONS)(pData->buffer + offset);

		extraBytes = NF_MAX_ADDRESS_LENGTH + sizeof(NF_UDP_OPTIONS) - 1 + pOptions->optionsLength;

		if (pData->bufferSize < extraBytes)
		{
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: invalid request[1]\n", pData->id));
			status = STATUS_NO_MEMORY;
			break;
		}

		memcpy(&pPacket->options, pOptions->options, sizeof(NF_UDP_PACKET_OPTIONS));
		offset += sizeof(NF_UDP_OPTIONS) - 1 + sizeof(NF_UDP_PACKET_OPTIONS);

		if (pPacket->options.controlDataLength > 0)
		{
			if (pPacket->options.controlDataLength > (pOptions->optionsLength - sizeof(NF_UDP_PACKET_OPTIONS)))
			{
				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: invalid request[2]\n", pData->id));
				status = STATUS_NO_MEMORY;
				break;
			}

#pragma warning(push)
#pragma warning(disable: 28197) 
			pPacket->controlData = (WSACMSGHDR*)
				ExAllocatePoolWithTag(NonPagedPool, pPacket->options.controlDataLength, MEM_TAG_UDP_DATA);
#pragma warning(pop)
			
			if (!pPacket->controlData)
			{
				status = STATUS_NO_MEMORY;
				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] malloc_np(pPacket->options.controlDataLength) failed\n", 
						pUdpCtx->id));
				break;
			}

			memcpy(pPacket->controlData, pData->buffer + offset, pPacket->options.controlDataLength);

			offset += pPacket->options.controlDataLength;
		} else
		{
			pPacket->controlData = NULL;
		}

		dataLength = (UINT16)(pData->bufferSize - offset);

		KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: dataLength = %u\n", pData->id, dataLength));

		if (dataLength == 0)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u]: invalid request\n", pData->id));
			break;
		}

		pPacket->dataLength = dataLength;

		context = (PNF_UDP_INJECT_CONTEXT)ExAllocateFromNPagedLookasideList( &g_udpInjectContextLAList );
		if (context == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] malloc_np(sizeof(NF_UDP_INJECT_CONTEXT)) failed, status=%x\n", 
					pUdpCtx->id, status));
			break;
		}

		dataCopy = ExAllocatePoolWithTag(NonPagedPool, dataLength, MEM_TAG_UDP_DATA_COPY);
		if (dataCopy == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] malloc_np(packetSize) failed, status=%x, packetSize=%d\n", 
				pUdpCtx->id, status, dataLength));
			break;
		}

		memcpy(dataCopy, pData->buffer + offset, dataLength);

		mdl = IoAllocateMdl(
				dataCopy,
				dataLength,
				FALSE,
				FALSE,
				NULL);
		if (mdl == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] IoAllocateMdl failed, status=%x\n", 
				pUdpCtx->id, status));
			break;
		}

		MmBuildMdlForNonPagedPool(mdl);

		status = FwpsAllocateNetBufferAndNetBufferList(
					  g_netBufferListPool,
					  0,
					  0,
					  mdl,
					  0,
					  dataLength,
					  &netBufferList);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] FwpsAllocateNetBufferAndNetBufferList failed, status=%x\n", 
				pUdpCtx->id, status));
			break;
		}

		netBuffer = NET_BUFFER_LIST_FIRST_NB(netBufferList);
		
		context->id = pData->id;
		context->code = pData->code;
		context->mdl = mdl;
		context->pUdpCtx = pUdpCtx;
		context->pPacket = pPacket;

		udpctx_addRef(pUdpCtx);

		sl_lock(&pUdpCtx->lock, &lh);

		if (pData->code == NF_UDP_SEND)
		{
			pUdpCtx->injectedSendBytes += pPacket->dataLength;
			if (pUdpCtx->injectedSendBytes > UDP_PEND_LIMIT)
			{
				pUdpCtx->sendInProgress = TRUE;
			}
		} else
		{
			pUdpCtx->injectedRecvBytes += pPacket->dataLength;
			if (pUdpCtx->injectedRecvBytes > UDP_PEND_LIMIT)
			{
				pUdpCtx->recvInProgress = TRUE;
			}
		}

		sl_unlock(&lh);

		KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] inject\n", pUdpCtx->id));

		if (pData->code == NF_UDP_SEND)
		{
			memset(&pPacket->sendArgs, 0, sizeof(pPacket->sendArgs));

			udpHeader = (UDP_HEADER*)NdisGetDataBuffer(
							netBuffer,
							sizeof(UDP_HEADER),
							NULL,
							1,
							0
							);
			ASSERT(udpHeader != NULL);
			if (!udpHeader)
			{
				status = STATUS_NO_MEMORY;
				break;
			}

			if (pUdpCtx->transportEndpointHandle == 0)
			{
				struct sockaddr_in * pAddr = (struct sockaddr_in *)pPacket->remoteAddr;
				pUdpCtx->ip_family = pAddr->sin_family;
				pUdpCtx->ipProto = IPPROTO_UDP;

				memcpy(&pUdpCtx->localAddr, &pPacket->options.localAddr, NF_MAX_ADDRESS_LENGTH);
			}

			if (pUdpCtx->ip_family == AF_INET)
			{
				struct sockaddr_in * pAddr = (struct sockaddr_in *)pPacket->remoteAddr;
				udpHeader->destPort = pAddr->sin_port;
				udpHeader->length = htons(dataLength);
				udpHeader->checksum = 0;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] remoteAddr=%x:%d\n", 
					pUdpCtx->id, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));

				pPacket->sendArgs.remoteAddress = (UCHAR*)&pAddr->sin_addr;

				pRemoteAddr = (UCHAR*)&pAddr->sin_addr;

				pAddr = (struct sockaddr_in *)pUdpCtx->localAddr;
				pLocalAddr = (UCHAR*)&pAddr->sin_addr;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] localAddr=%x:%d (%d)\n", 
					pUdpCtx->id, 
					pAddr->sin_addr.s_addr, htons(pAddr->sin_port),
					htons(udpHeader->srcPort)));
			} else
			{
				struct sockaddr_in6 * pAddr = (struct sockaddr_in6 *)pPacket->remoteAddr;
				udpHeader->destPort = pAddr->sin6_port;
				udpHeader->length = htons(dataLength);
				udpHeader->checksum = 0;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] remoteAddr=[%x:%x:%x:%x:%x:%x:%x:%x]:%d\n", 
					pUdpCtx->id, 
					pAddr->sin6_addr.u.Word[0], 
					pAddr->sin6_addr.u.Word[1], 
					pAddr->sin6_addr.u.Word[2], 
					pAddr->sin6_addr.u.Word[3], 
					pAddr->sin6_addr.u.Word[4], 
					pAddr->sin6_addr.u.Word[5], 
					pAddr->sin6_addr.u.Word[6], 
					pAddr->sin6_addr.u.Word[7], 
					htons(pAddr->sin6_port)));

				pPacket->sendArgs.remoteAddress = (UCHAR*)&pAddr->sin6_addr;

				pRemoteAddr = (UCHAR*)&pAddr->sin6_addr;

				pAddr = (struct sockaddr_in6 *)pUdpCtx->localAddr;
				pLocalAddr = (UCHAR*)&pAddr->sin6_addr;
			}

			pPacket->sendArgs.remoteScopeId = pPacket->options.remoteScopeId;
			pPacket->sendArgs.controlData = pPacket->controlData;
			pPacket->sendArgs.controlDataLength = pPacket->options.controlDataLength;

			if (g_udpNwInject || (pUdpCtx->transportEndpointHandle == 0))
			{
				status = FwpsConstructIpHeaderForTransportPacket0(
						  netBufferList,
						  0,
						  pUdpCtx->ip_family,
						  pLocalAddr,
						  pRemoteAddr,
						  (IPPROTO)pUdpCtx->ipProto,
						  0,
						  pPacket->controlData,
						  pPacket->options.controlDataLength,
						  0,
						  NULL,
						  pPacket->options.interfaceIndex,
					      pPacket->options.subInterfaceIndex
						  );

			   if (!NT_SUCCESS(status))
			   {
					KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] FwpsConstructIpHeaderForTransportPacket0 failed, status=%x\n", 
						pUdpCtx->id, status));
				   break;
			   }

				if (pUdpCtx->ip_family == AF_INET)
				{
					status = FwpsInjectNetworkSendAsync0(
						   g_udpNwInjectionHandleV4,
						   NULL,
						   0,
						   pPacket->options.compartmentId,
						   netBufferList,
						   devctrl_udpInjectCompletion,
						   context
						   );
				} else
				{
					status = FwpsInjectNetworkSendAsync0(
						   g_udpNwInjectionHandleV6,
						   NULL,
						   0,
						   pPacket->options.compartmentId,
						   netBufferList,
						   devctrl_udpInjectCompletion,
						   context
						   );
				}
			} else
			{
				status = FwpsInjectTransportSendAsync0(
					   g_udpInjectionHandle,
					   NULL,
					   pPacket->options.endpointHandle,
					   0,
					   &pPacket->sendArgs,
					   pUdpCtx->ip_family,
					   pPacket->options.compartmentId,
					   netBufferList,
					   devctrl_udpInjectCompletion,
					   context
					   );
			}

		   if (NT_SUCCESS(status))
		   {
				result = sizeof(NF_DATA) + pData->bufferSize - 1;

				pPacket = NULL;
				netBufferList = NULL;
		   } else
		   {
				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] inject failed, status=%x\n", 
					pUdpCtx->id, status));
		   }

		   devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CAN_SEND, NULL, NULL);

		} else
		{
			udpHeader = (UDP_HEADER*)NdisGetDataBuffer(
							netBuffer,
							sizeof(UDP_HEADER),
							NULL,
							1,
							0
							);
			ASSERT(udpHeader != NULL);
			if (!udpHeader)
			{
				status = STATUS_NO_MEMORY;
				break;
			}
			if (pUdpCtx->ip_family == AF_INET)
			{
				struct sockaddr_in * pAddr = (struct sockaddr_in *)pPacket->remoteAddr;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] remoteAddr=%x:%d\n", 
					pUdpCtx->id, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));

				udpHeader->srcPort = pAddr->sin_port;
				udpHeader->length = htons(dataLength);
				udpHeader->checksum = 0;

				pRemoteAddr = (UCHAR*)&pAddr->sin_addr;

				pAddr = (struct sockaddr_in *)pUdpCtx->localAddr;
				pLocalAddr = (UCHAR*)&pAddr->sin_addr;

				udpHeader->destPort = pAddr->sin_port;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] localAddr=%x:%d\n", 
					pUdpCtx->id, pAddr->sin_addr.s_addr, htons(pAddr->sin_port)));
			} else
			{
				struct sockaddr_in6 * pAddr = (struct sockaddr_in6 *)pPacket->remoteAddr;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] remoteAddr=[%x:%x:%x:%x:%x:%x:%x:%x]:%d\n", 
					pUdpCtx->id, 
					pAddr->sin6_addr.u.Word[0], 
					pAddr->sin6_addr.u.Word[1], 
					pAddr->sin6_addr.u.Word[2], 
					pAddr->sin6_addr.u.Word[3], 
					pAddr->sin6_addr.u.Word[4], 
					pAddr->sin6_addr.u.Word[5], 
					pAddr->sin6_addr.u.Word[6], 
					pAddr->sin6_addr.u.Word[7], 
					htons(pAddr->sin6_port)));

				udpHeader->srcPort = pAddr->sin6_port;
				udpHeader->length = htons(dataLength);
				udpHeader->checksum = 0;

				pRemoteAddr = (UCHAR*)&pAddr->sin6_addr;

				pAddr = (struct sockaddr_in6 *)pUdpCtx->localAddr;
				pLocalAddr = (UCHAR*)&pAddr->sin6_addr;

				udpHeader->destPort = pAddr->sin6_port;

				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] localAddr=[%x:%x:%x:%x:%x:%x:%x:%x]:%d\n", 
					pUdpCtx->id, 
					pAddr->sin6_addr.u.Word[0], 
					pAddr->sin6_addr.u.Word[1], 
					pAddr->sin6_addr.u.Word[2], 
					pAddr->sin6_addr.u.Word[3], 
					pAddr->sin6_addr.u.Word[4], 
					pAddr->sin6_addr.u.Word[5], 
					pAddr->sin6_addr.u.Word[6], 
					pAddr->sin6_addr.u.Word[7], 
					htons(pAddr->sin6_port)));

			}

			status = FwpsConstructIpHeaderForTransportPacket0(
					  netBufferList,
					  0,
					  pUdpCtx->ip_family,
					  pRemoteAddr,
					  pLocalAddr,
					  (IPPROTO)pUdpCtx->ipProto,
					  0,
					  NULL,
					  0,
					  0,
					  NULL,
					   0, //pPacket->options.interfaceIndex,
					   0 //pPacket->options.subInterfaceIndex
					  );

		   if (!NT_SUCCESS(status))
		   {
				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] FwpsConstructIpHeaderForTransportPacket0 failed, status=%x\n", 
					pUdpCtx->id, status));
			   break;
		   }

			status = FwpsInjectTransportReceiveAsync0(
					   g_udpInjectionHandle,
					   NULL,
					   NULL,
					   0,
					   pUdpCtx->ip_family,
					   pPacket->options.compartmentId,
					   pPacket->options.interfaceIndex,
					   pPacket->options.subInterfaceIndex,
					   netBufferList,
					   devctrl_udpInjectCompletion,
					   context
					   );

		   if (NT_SUCCESS(status))
		   {
				result = sizeof(NF_DATA) + pData->bufferSize - 1;
				
				pPacket = NULL;
				netBufferList = NULL;
		   } else
		   {
				KdPrint((DPREFIX"devctrl_processUdpPacket[%I64u] FwpsInjectTransportReceiveAsync0 failed, status=%x\n", 
					pUdpCtx->id, status));
		   }

		   devctrl_pushUdpData(pUdpCtx->id, NF_UDP_CAN_RECEIVE, NULL, NULL);
		}

		if (!NT_SUCCESS(status))
		{
			sl_lock(&pUdpCtx->lock, &lh);
			if (pData->code == NF_UDP_SEND)
			{
				pUdpCtx->injectedSendBytes -= pPacket->dataLength;
				if (pUdpCtx->injectedSendBytes <= UDP_PEND_LIMIT)
				{
					pUdpCtx->sendInProgress = FALSE;
				}
			} else
			{
				pUdpCtx->injectedRecvBytes -= pPacket->dataLength;
				if (pUdpCtx->injectedRecvBytes <= UDP_PEND_LIMIT)
				{
					pUdpCtx->recvInProgress = FALSE;
				}
			}
			sl_unlock(&lh);

			udpctx_release(pUdpCtx);
		}

		break;
	}

	if (!NT_SUCCESS(status))
	{
		if (netBufferList != NULL)
		{
			FwpsFreeNetBufferList(netBufferList);
		}
		if (mdl != NULL)
		{
			IoFreeMdl(mdl);
		}
		if (dataCopy != NULL)
		{
			free_np(dataCopy);
		}
		if (context != NULL)
		{
			ExFreeToNPagedLookasideList( &g_udpInjectContextLAList, context );
		}
		if (pPacket != NULL)
		{
			udpctx_freePacket(pPacket);
		}
	}

	udpctx_release(pUdpCtx);

	return result;
}

ULONG 
devctrl_setUdpConnState(PNF_DATA pData)
{
	PUDPCTX pUdpCtx = NULL;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_setUdpConnState[%I64u]: %d\n", pData->id, pData->code));

	pUdpCtx = udpctx_find(pData->id);
	if (!pUdpCtx)
	{
		KdPrint((DPREFIX"devctrl_setUdpConnState[%I64u]: UDPCTX not found\n", pData->id));
		return 0;
	}

	sl_lock(&pUdpCtx->lock, &lh);
	
	switch (pData->code)
	{
	case NF_UDP_REQ_SUSPEND:
		{
			KdPrint((DPREFIX"devctrl_setUdpConnState[%I64u]: connection suspended\n", pData->id));
			if (pUdpCtx->filteringFlag & NF_FILTER)
			{
				pUdpCtx->filteringFlag |= NF_SUSPENDED;
			}
		}
		break;
	case NF_UDP_REQ_RESUME:
		{
			int i, listSize;

			if ((pUdpCtx->filteringFlag & NF_FILTER) && (pUdpCtx->filteringFlag & NF_SUSPENDED))
			{
				KdPrint((DPREFIX"devctrl_setUdpConnState[%I64u]: connection resumed\n", pData->id));
		
				pUdpCtx->filteringFlag &= ~NF_SUSPENDED;

				listSize = devctrl_getListSize(&pUdpCtx->pendedRecvPackets);

				sl_unlock(&lh);
			
				for (i=0; i<listSize; i++)
				{
					devctrl_pushUdpData(pUdpCtx->id, NF_UDP_RECEIVE, pUdpCtx, NULL);
				}

				sl_lock(&pUdpCtx->lock, &lh);

				listSize = devctrl_getListSize(&pUdpCtx->pendedSendPackets);

				sl_unlock(&lh);

				for (i=0; i<listSize; i++)
				{
					devctrl_pushUdpData(pUdpCtx->id, NF_UDP_SEND, pUdpCtx, NULL);
				}

				sl_lock(&pUdpCtx->lock, &lh);
			}
		}
		break;
	}

	sl_unlock(&lh);

	udpctx_release(pUdpCtx);

	return sizeof(NF_DATA) - 1;
}


#ifndef CTE_TYPEDEFS_DEFINED
#define CTE_TYPEDEFS_DEFINED

typedef unsigned long ulong;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned int uint;

#endif // CTE_TYPEDEFS_DEFINED

#define	CL_TL_ENTITY				0x401
#define	INFO_CLASS_PROTOCOL			0x200
#define	INFO_TYPE_CONNECTION		0x300

//* Structure of an entity ID.
typedef struct TDIEntityID {
	ulong		tei_entity;
	ulong		tei_instance;
} TDIEntityID;

//* Structure of an object ID.
typedef struct TDIObjectID {
	TDIEntityID	toi_entity;
	ulong		toi_class;
	ulong		toi_type;
	ulong		toi_id;
} TDIObjectID;

//
// SetInformationEx IOCTL request structure. This structure is passed as the
// InputBuffer. The space allocated for the structure must be large enough
// to contain the structure and the set data buffer, which begins at the
// Buffer field. The OutputBuffer parameter in the DeviceIoControl is not used.
//
typedef struct tcp_request_set_information_ex {
	TDIObjectID     ID;             // object ID to set.
	unsigned int    BufferSize;     // size of the set data buffer in bytes
	unsigned char   Buffer[1];      // beginning of the set data buffer
} TCP_REQUEST_SET_INFORMATION_EX, *PTCP_REQUEST_SET_INFORMATION_EX;

ULONG   
devctrl_setTcpOpt(PNF_DATA pData)
{   
	PTCPCTX pTcpCtx = NULL;
	PTCP_REQUEST_SET_INFORMATION_EX info;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"devctrl_setTcpOpt[%I64u]: %d\n", pData->id, pData->code));

	info = (PTCP_REQUEST_SET_INFORMATION_EX)pData->buffer;

	pTcpCtx = tcpctx_find(pData->id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"devctrl_setTcpOpt[%I64u]: TCPCTX not found\n", pData->id));
		return 0;
	}

	sl_lock(&pTcpCtx->lock, &lh);
	
	if (info->ID.toi_id == TCP_SOCKET_NODELAY &&
		info->BufferSize == sizeof(int))
	{
		pTcpCtx->noDelay = (*(int*)info->Buffer != 0);
	}

	sl_unlock(&lh);

	tcpctx_release(pTcpCtx);

	return sizeof(NF_DATA) - 1;
}   

ULONG   
devctrl_processTcpConnect(PNF_DATA pData)
{   
	PTCPCTX pTcpCtx = NULL;
	PNF_TCP_CONN_INFO pInfo;
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint((DPREFIX"devctrl_processTcpConnect[%I64u]: %d\n", pData->id, pData->code));

	pInfo = (PNF_TCP_CONN_INFO)pData->buffer;

	pTcpCtx = tcpctx_find(pData->id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"devctrl_processTcpConnect[%I64u]: TCPCTX not found\n", pData->id));
		return 0;
	}

	if (pTcpCtx->redirectInfo.isPended)
	{
		FWPS_CONNECT_REQUEST * pConnectRequest;
		int addrLen = 0;
		
		pTcpCtx->filteringFlag = pInfo->filteringFlag;

		if (pTcpCtx->ip_family == AF_INET)
		{
			addrLen = sizeof(struct sockaddr_in);
		} else
		{
			addrLen = sizeof(struct sockaddr_in6);
		}

		// Change the request address using FwpsApplyModifiedLayerData only if the address is changed in pInfo.
		// If the connection is not redirected, but has localhost remote address, the connection is dropped 
		// when no process id is specified in localRedirectTargetPID.

		if ((memcmp(pTcpCtx->remoteAddr, pInfo->remoteAddress, addrLen) != 0) ||
			(pTcpCtx->filteringFlag & NF_BLOCK))
		{
			KdPrint((DPREFIX"devctrl_processTcpConnect[%I64u]: redirection\n", pData->id));
		
			status = FwpsAcquireWritableLayerDataPointer(pTcpCtx->redirectInfo.classifyHandle,
														pTcpCtx->redirectInfo.filterId,
														0,
														(PVOID*)&pConnectRequest,
														&pTcpCtx->redirectInfo.classifyOut);
			if (status == STATUS_SUCCESS)
			{
				if (pTcpCtx->filteringFlag & NF_BLOCK)			
				{
					memset(&pConnectRequest->remoteAddressAndPort, 0, NF_MAX_ADDRESS_LENGTH);
				} else
				{
					memcpy(&pConnectRequest->remoteAddressAndPort, pInfo->remoteAddress, NF_MAX_ADDRESS_LENGTH);
				}

				pConnectRequest->localRedirectTargetPID = pInfo->processId;
				
#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
				pConnectRequest->localRedirectHandle = pTcpCtx->redirectInfo.redirectHandle;
#endif
#endif

				FwpsApplyModifiedLayerData(pTcpCtx->redirectInfo.classifyHandle,
										 pConnectRequest,
										 0);
			}
		}

		if (pTcpCtx->filteringFlag & NF_BLOCK)
		{
			pTcpCtx->redirectInfo.classifyOut.actionType = FWP_ACTION_BLOCK;
		} else
		{
			pTcpCtx->redirectInfo.classifyOut.actionType = FWP_ACTION_PERMIT;
		}

		tcpctx_purgeRedirectInfo(pTcpCtx);
	}

	if (pTcpCtx->filteringFlag & NF_BLOCK)
	{
		tcpctx_release(pTcpCtx);
	}

	tcpctx_release(pTcpCtx);

	return sizeof(NF_DATA) - 1;
}   

unsigned long devctrl_tcpDisableUserModeFiltering(PNF_DATA pData)
{
	PTCPCTX pTcpCtx = NULL;
	KLOCK_QUEUE_HANDLE lh;
	unsigned long result = 0;

	KdPrint((DPREFIX"devctrl_tcpDisableFiltering[%I64d]\n", pData->id));

	pTcpCtx = tcpctx_find(pData->id);
	if (!pTcpCtx)
	{
		KdPrint((DPREFIX"devctrl_tcpDisableFiltering[%I64u]: TCPCTX not found\n", pData->id));
		return 0;
	}

	sl_lock(&pTcpCtx->lock, &lh);
	
	if (pTcpCtx->closed)
	{
		KdPrint((DPREFIX"devctrl_tcpDisableFiltering[%I64u]: closed connection\n", pData->id));
		sl_unlock(&lh);
		tcpctx_release(pTcpCtx);
		return 0;
	}

	if (pTcpCtx->filteringFlag & NF_FILTER)
	{
		pTcpCtx->filteringState = UMFS_DISABLE;

		if (IsListEmpty(&pTcpCtx->pendedPackets) &&
			IsListEmpty(&pTcpCtx->injectPackets) &&
			pTcpCtx->injectedSendBytes == 0 &&
			pTcpCtx->injectedRecvBytes == 0)
		{
			KdPrint((DPREFIX"devctrl_tcpDisableFiltering[%I64d]: user mode filtering disabled\n", pData->id));

			pTcpCtx->filteringState = UMFS_DISABLED;
//			pTcpCtx->filteringFlag = NF_ALLOW;
			result = sizeof(NF_DATA) - 1;	
		}
	}

	sl_unlock(&lh);

	tcpctx_release(pTcpCtx);

	return result;
}

unsigned long devctrl_udpDisableUserModeFiltering(PNF_DATA pData)
{
	PUDPCTX pUdpCtx = NULL;
	KLOCK_QUEUE_HANDLE lh;
	unsigned long result = 0;

	KdPrint((DPREFIX"devctrl_udpDisableFiltering[%I64d]\n", pData->id));

	pUdpCtx = udpctx_find(pData->id);
	if (!pUdpCtx)
	{
		KdPrint((DPREFIX"devctrl_udpDisableFiltering[%I64u]: UDPCTX not found\n", pData->id));
		return 0;
	}

	sl_lock(&pUdpCtx->lock, &lh);
	
	pUdpCtx->filteringDisabled = TRUE;

	result = sizeof(NF_DATA) - 1;	

	sl_unlock(&lh);

	udpctx_release(pUdpCtx);

	return result;
}

ULONG   
devctrl_processUdpConnect(PNF_DATA pData)
{   
	PUDPCTX pUdpCtx = NULL;
	PNF_UDP_CONN_REQUEST pInfo;
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint((DPREFIX"devctrl_processUdpConnect[%I64u]: %d\n", pData->id, pData->code));

	pInfo = (PNF_UDP_CONN_REQUEST)pData->buffer;

	pUdpCtx = udpctx_find(pData->id);
	if (!pUdpCtx)
	{
		KdPrint((DPREFIX"devctrl_processUdpConnect[%I64u]: UDPCTX not found\n", pData->id));
		return 0;
	}

	if (pUdpCtx->redirectInfo.isPended)
	{
		FWPS_CONNECT_REQUEST * pConnectRequest;
		int addrLen = 0;
		
		pUdpCtx->filteringFlag = pInfo->filteringFlag;

		if (pUdpCtx->ip_family == AF_INET)
		{
			addrLen = sizeof(struct sockaddr_in);
		} else
		{
			addrLen = sizeof(struct sockaddr_in6);
		}

		// Change the request address using FwpsApplyModifiedLayerData only if the address is changed in pInfo.
		// If the connection is not redirected, but has localhost remote address, the connection is dropped 
		// when no process id is specified in localRedirectTargetPID.

		if (memcmp(pUdpCtx->remoteAddr, pInfo->remoteAddress, addrLen) != 0)
		{
			status = FwpsAcquireWritableLayerDataPointer(pUdpCtx->redirectInfo.classifyHandle,
														pUdpCtx->redirectInfo.filterId,
														0,
														(void **)&pConnectRequest,
														&pUdpCtx->redirectInfo.classifyOut);
			if (status == STATUS_SUCCESS)
			{
				memcpy(&pConnectRequest->remoteAddressAndPort, pInfo->remoteAddress, NF_MAX_ADDRESS_LENGTH);

				pConnectRequest->localRedirectTargetPID = pInfo->processId;
				
#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
				pConnectRequest->localRedirectHandle = pUdpCtx->redirectInfo.redirectHandle;
#endif
#endif

				FwpsApplyModifiedLayerData(pUdpCtx->redirectInfo.classifyHandle,
										 pConnectRequest,
										 0);

				pUdpCtx->redirected = TRUE;
			}
		}

		if (pUdpCtx->filteringFlag & NF_BLOCK)
		{
			pUdpCtx->redirectInfo.classifyOut.actionType = FWP_ACTION_BLOCK;
		} else
		{
			pUdpCtx->redirectInfo.classifyOut.actionType = FWP_ACTION_PERMIT;
		}

		udpctx_purgeRedirectInfo(pUdpCtx);
	}

	udpctx_release(pUdpCtx);

	return sizeof(NF_DATA) - 1;
}   

void NTAPI 
devctrl_IPPacketInjectCompletion(
   IN void* context,
   IN OUT NET_BUFFER_LIST* netBufferList,
   IN BOOLEAN dispatchLevel
   )
{
   PNF_IP_INJECT_CONTEXT inject_context = (PNF_IP_INJECT_CONTEXT)context;

   UNREFERENCED_PARAMETER(dispatchLevel);

   KdPrint((DPREFIX"devctrl_IPPacketInjectCompletion\n"));

   if (netBufferList)
   {
		PMDL mdl = NET_BUFFER_FIRST_MDL(NET_BUFFER_LIST_FIRST_NB(netBufferList));

		KdPrint((DPREFIX"devctrl_IPPacketInjectCompletion status=%x\n", netBufferList->Status));

		if (mdl != inject_context->mdl)
		{
			IoFreeMdl(mdl);
		}

		FwpsFreeNetBufferList(netBufferList);
   }

   if (inject_context != NULL)
   {
		if (inject_context->mdl != NULL)
		{
			free_np(inject_context->mdl->MappedSystemVa);
			IoFreeMdl(inject_context->mdl);
			inject_context->mdl = NULL;
		}

		if (inject_context->pPacket)
		{
			ipctx_freePacket(inject_context->pPacket);
		}

		ExFreeToNPagedLookasideList( &g_ipInjectContextLAList, inject_context );
   }
}

static UINT16 devctrl_checksum(const void * pseudo_header, size_t pseudo_header_len, const void *data, size_t len)
{
    register const UINT16 *data16;
    register size_t len16;
    register UINT32 sum = 0;
    size_t i;

	if (pseudo_header != NULL)
	{
		data16 = (const UINT16 *)pseudo_header;
		len16 = pseudo_header_len >> 1;

		for (i = 0; i < len16; i++)
		{
			sum += (UINT32)data16[i];
		}
	}

    data16 = (const UINT16 *)data;
    len16 = len >> 1;
    for (i = 0; i < len16; i++)
    {
        sum += (UINT32)data16[i];
    }
    
    if (len & 0x1)
    {
        const UINT8 *data8 = (const UINT8 *)data;
        sum += (UINT32)data8[len-1];
    }

    sum = (sum & 0xFFFF) + (sum >> 16);
    sum += (sum >> 16);
    sum = ~sum;
    
	return (UINT16)sum;
}

static void devctrl_updateChecksum(const void * buf, size_t len, BOOLEAN updateIP, BOOLEAN updateNext)
{
	struct
    {
        UINT32 SrcAddr;
        UINT32 DstAddr;
        UINT8  Zero;
        UINT8  Protocol;
        UINT16 TransLength;
    } pseudo_header;
	
	size_t ip_header_len, trans_len;
	void *trans_header;
	PIP_HEADER_V4 pIPHdr;
	PTCP_HEADER	pTCPHdr;
	PUDP_HEADER	pUDPHdr;
    UINT16 *trans_check_ptr;
    UINT sum;

	if (len < sizeof(IP_HEADER_V4))
    {
        return;
    }

	pIPHdr = (PIP_HEADER_V4)buf;

	if (pIPHdr->version != 4)
		return;

	ip_header_len = pIPHdr->headerLength *sizeof(UINT32);
    if (len < ip_header_len)
    {
        return;
    }

	if (updateIP)
	{
		KdPrint((DPREFIX"devctrl_updateChecksum old IP cksum = %u\n", pIPHdr->checksum));
		pIPHdr->checksum = devctrl_checksum(NULL, 0, buf, ip_header_len);
		KdPrint((DPREFIX"devctrl_updateChecksum new IP cksum = %u\n", pIPHdr->checksum));
	}

	if (updateNext)
	{
		trans_len = RtlUshortByteSwap(pIPHdr->totalLength) - ip_header_len;
		trans_header = (UINT8 *)pIPHdr + ip_header_len;
		
		switch (pIPHdr->protocol)
		{
			case IPPROTO_TCP:
				pTCPHdr = (PTCP_HEADER)trans_header;
				if (trans_len < sizeof(TCP_HEADER))
				{
					return;
				}
//				if (pTCPHdr->th_sum != 0)
//					return;
				KdPrint((DPREFIX"devctrl_updateChecksum old TCP cksum = %u\n", pTCPHdr->th_sum));
				trans_check_ptr = &pTCPHdr->th_sum;
				break;

			case IPPROTO_UDP:
				pUDPHdr = (PUDP_HEADER)trans_header;
				if (trans_len < sizeof(UDP_HEADER))
				{
					return;
				}
//				if (pUDPHdr->checksum != 0)
//					return;
				trans_check_ptr = &pUDPHdr->checksum;
				break;

			default:
				return;
		}

		pseudo_header.SrcAddr     = ((struct iphdr*)pIPHdr)->SrcAddr;
		pseudo_header.DstAddr     = ((struct iphdr*)pIPHdr)->DstAddr;
		pseudo_header.Zero        = 0x0;
		pseudo_header.Protocol    = pIPHdr->protocol;
		pseudo_header.TransLength = RtlUshortByteSwap((UINT16)trans_len);
		*trans_check_ptr = 0x0;
    
		sum = devctrl_checksum(&pseudo_header, sizeof(pseudo_header), trans_header, trans_len);

		KdPrint((DPREFIX"devctrl_updateChecksum new next cksum = %u\n", sum));

		if (sum == 0 && pIPHdr->protocol == IPPROTO_UDP)
		{
			*trans_check_ptr = 0xFFFF;
		} else
		{
			*trans_check_ptr = (UINT16)sum;
		}
	}
}

ULONG devctrl_processIPPacket(PNF_DATA pData)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PVOID dataCopy = NULL;
	PMDL mdl = NULL;
	NET_BUFFER_LIST* netBufferList = NULL;
	PNF_IP_INJECT_CONTEXT context = NULL;
	UINT16		dataLength;
	NET_BUFFER* netBuffer = NULL;
	PNF_IP_PACKET pPacket = NULL;
	ULONG	result = 0;

	KdPrint((DPREFIX"devctrl_processIPPacket code=%d, size=%d\n", pData->code, pData->bufferSize));

	if (pData->bufferSize < sizeof(NF_IP_PACKET_OPTIONS))
	{
		KdPrint((DPREFIX"devctrl_processIPPacket invalid request\n"));
		return sizeof(NF_DATA) - 1 + pData->bufferSize;
	}

	for (;;) 
	{
		dataLength = (UINT16)(pData->bufferSize - sizeof(NF_IP_PACKET_OPTIONS));

		KdPrint((DPREFIX"devctrl_processIPPacket dataLength = %u\n", dataLength));

		pPacket = ipctx_allocPacket(0);
		if (!pPacket)
		{
			KdPrint((DPREFIX"devctrl_processIPPacket ipctx_allocPacket failed\n"));
			status = STATUS_NO_MEMORY;
			break;
		}

		memcpy(&pPacket->options, (PNF_IP_PACKET_OPTIONS)(pData->buffer), sizeof(NF_IP_PACKET_OPTIONS));
		pPacket->isSend = (pData->code == NF_IP_SEND);
		pPacket->dataLength = dataLength;

		context = (PNF_IP_INJECT_CONTEXT)ExAllocateFromNPagedLookasideList( &g_ipInjectContextLAList );
		if (context == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processIPPacket malloc_np(sizeof(NF_IP_INJECT_CONTEXT)) failed, status=%x\n", 
					status));
			break;
		}

		dataCopy = ExAllocatePoolWithTag(NonPagedPool, dataLength, MEM_TAG_IP_DATA_COPY);
		if (dataCopy == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processIPPacket malloc_np(packetSize) failed, status=%x, packetSize=%d\n", 
				status, dataLength));
			break;
		}

		memcpy(dataCopy, pData->buffer + sizeof(NF_IP_PACKET_OPTIONS), dataLength);

		mdl = IoAllocateMdl(
				dataCopy,
				dataLength,
				FALSE,
				FALSE,
				NULL);
		if (mdl == NULL)
		{
			status = STATUS_NO_MEMORY;
			KdPrint((DPREFIX"devctrl_processIPPacket IoAllocateMdl failed, status=%x\n", 
				status));
			break;
		}

		MmBuildMdlForNonPagedPool(mdl);

		status = FwpsAllocateNetBufferAndNetBufferList(
					  g_netBufferListPool,
					  0,
					  0,
					  mdl,
					  0,
					  dataLength,
					  &netBufferList);
		if (!NT_SUCCESS(status))
		{
			KdPrint((DPREFIX"devctrl_processIPPacket FwpsAllocateNetBufferAndNetBufferList failed, status=%x\n", 
				status));
			break;
		}

		netBuffer = NET_BUFFER_LIST_FIRST_NB(netBufferList);
		
		context->code = pData->code;
		context->mdl = mdl;
		context->pPacket = pPacket;

		KdPrint((DPREFIX"devctrl_processIPPacket inject\n"));

		if (pData->code == NF_IP_SEND)
		{
			devctrl_updateChecksum(dataCopy, dataLength, TRUE, TRUE);

			status = FwpsInjectNetworkSendAsync0(
					(pPacket->options.ip_family == AF_INET)? g_nwInjectionHandleV4 : g_nwInjectionHandleV6,
					(HANDLE)pPacket->options.interfaceIndex,
					0,
					(COMPARTMENT_ID)pPacket->options.compartmentId,
					netBufferList,
					devctrl_IPPacketInjectCompletion,
					context
					);

		   if (NT_SUCCESS(status))
		   {
				result = sizeof(NF_DATA) + pData->bufferSize - 1;

				pPacket = NULL;
				netBufferList = NULL;
		   } else
		   {
				KdPrint((DPREFIX"devctrl_processIPPacket inject failed, status=%x\n", 
					status));	
		   }

		} else
		{

			KdPrint((DPREFIX"FwpsInjectNetworkReceiveAsync0 interface=%d\n",
				pPacket->options.interfaceIndex));

			status = FwpsInjectNetworkReceiveAsync0(
					(pPacket->options.ip_family == AF_INET)? g_nwInjectionHandleV4 : g_nwInjectionHandleV6,
					(HANDLE)pPacket->options.interfaceIndex,
					0,
					(COMPARTMENT_ID)pPacket->options.compartmentId,
					pPacket->options.interfaceIndex,
					pPacket->options.subInterfaceIndex,
					netBufferList,
					devctrl_IPPacketInjectCompletion,
					context
					);

		   if (NT_SUCCESS(status))
		   {
				result = sizeof(NF_DATA) + pData->bufferSize - 1;

				pPacket = NULL;
				netBufferList = NULL;
		   } else
		   {
				KdPrint((DPREFIX"devctrl_processIPPacket inject failed, status=%x\n", 
					status));
		   }

		}

		break;
	}

	if (!NT_SUCCESS(status))
	{
		if (netBufferList != NULL)
		{
			FwpsFreeNetBufferList(netBufferList);
		}
		if (mdl != NULL)
		{
			IoFreeMdl(mdl);
		}
		if (dataCopy != NULL)
		{
			free_np(dataCopy);
		}
		if (context != NULL)
		{
			ExFreeToNPagedLookasideList( &g_ipInjectContextLAList, context );
		}
		if (pPacket != NULL)
		{
			ipctx_freePacket(pPacket);
		}
	}

	return result;
}


ULONG devctrl_processRequest(ULONG bufferSize)
{
	PNF_DATA pData = (PNF_DATA)g_outBuf.kernelVa;

//	KdPrint((DPREFIX"devctrl_processRequest\n"));
		
	if (bufferSize < (sizeof(NF_DATA) + pData->bufferSize - 1))
	{
		return 0;
	}

	switch (pData->code)
	{
	case NF_TCP_RECEIVE:
	case NF_TCP_SEND:
		return devctrl_processTcpPacket(pData);
	
	case NF_TCP_CONNECT_REQUEST:
		return devctrl_processTcpConnect(pData);

	case NF_TCP_REQ_SUSPEND:
	case NF_TCP_REQ_RESUME:
		return devctrl_setTcpConnState(pData);

	case NF_REQ_ADD_HEAD_RULE:
		rules_add((PNF_RULE)pData->buffer, TRUE);
		return bufferSize;

	case NF_REQ_ADD_TAIL_RULE:
		rules_add((PNF_RULE)pData->buffer, FALSE);
		return bufferSize;

	case NF_REQ_DELETE_RULES:
		rules_remove_all();
		return bufferSize;

	case NF_UDP_CONNECT_REQUEST:
		return devctrl_processUdpConnect(pData);

	case NF_UDP_RECEIVE:
		return devctrl_processUdpPacket(pData);

	case NF_UDP_SEND:
		return devctrl_processUdpPacket(pData);
	
	case NF_UDP_REQ_SUSPEND:
	case NF_UDP_REQ_RESUME:
		return devctrl_setUdpConnState(pData);

	case NF_TCP_DISABLE_USER_MODE_FILTERING:
		return devctrl_tcpDisableUserModeFiltering(pData);

	case NF_UDP_DISABLE_USER_MODE_FILTERING:
		return devctrl_udpDisableUserModeFiltering(pData);

	case NF_REQ_SET_TCP_OPT:
		return devctrl_setTcpOpt(pData);

	case NF_REQ_IS_PROXY:
		return (tcpctx_isProxy((ULONG)pData->id))? sizeof(NF_DATA) - 1 : 0;
/*
	case NF_TCP_REINJECT:
		return devctrl_reinject(pData->id);
*/
//	case NF_TCP_REMOVE_CLOSED:
	//	return devctrl_removeClosed(pData->id);

	case NF_IP_RECEIVE:
	case NF_IP_SEND:
		return devctrl_processIPPacket(pData);
	}

	return 0;
}

NTSTATUS devctrl_write(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PNF_READ_RESULT pRes;
    ULONG bufferLength = irpSp->Parameters.Write.Length;

    pRes = (PNF_READ_RESULT)MmGetSystemAddressForMdlSafe(irp->MdlAddress, NormalPagePriority);
	if (!pRes || bufferLength < sizeof(NF_READ_RESULT))
	{
		irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		KdPrint((DPREFIX"devctrl_write invalid irp\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	irp->IoStatus.Information = devctrl_processRequest((ULONG)pRes->length);
	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

void devctrl_ioThread(IN PVOID StartContext)
{
	UNREFERENCED_PARAMETER(StartContext);

	KdPrint((DPREFIX"devctrl_ioThread\n"));

	for(;;)
	{
		KeWaitForSingleObject(
			&g_ioThreadEvent,
			 Executive, 
			 KernelMode, 
			 FALSE, 
			 NULL
         );

		if (devctrl_isShutdown())
		{
			break;
		}

		devctrl_serviceReads();
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}


#define _FLT_MAX_PATH 260 * 2

typedef NTSTATUS (*QUERY_INFO_PROCESS) (
    __in HANDLE ProcessHandle,
    __in PROCESSINFOCLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

static QUERY_INFO_PROCESS ZwQueryInformationProcess;

#pragma warning(disable:4055)
#pragma warning(disable:4312)

NTSTATUS getProcessName(HANDLE processId, PUNICODE_STRING * pName)
{
    NTSTATUS	status;
	PVOID		buf = NULL;
	ULONG		len = 0;
	PEPROCESS	eProcess = NULL;
	HANDLE		hProcess = NULL;
   
    if (NULL == ZwQueryInformationProcess) 
	{
        UNICODE_STRING routineName;

        RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

        ZwQueryInformationProcess =
               (QUERY_INFO_PROCESS) MmGetSystemRoutineAddress(&routineName);

        if (NULL == ZwQueryInformationProcess) 
		{
            KdPrint(("Cannot resolve ZwQueryInformationProcess\n"));
			return STATUS_UNSUCCESSFUL;
        }
    }

	status = PsLookupProcessByProcessId(processId, &eProcess);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = ObOpenObjectByPointer(eProcess, OBJ_KERNEL_HANDLE, NULL, 0, 0, KernelMode, &hProcess);
	if (!NT_SUCCESS(status))
	{
		ObDereferenceObject(eProcess);
		return status;
	}
	
    status = ZwQueryInformationProcess(
            hProcess,
            ProcessImageFileName,
            NULL,
            0,
            &len
        );

	if (STATUS_INFO_LENGTH_MISMATCH != status)
	{
		goto fin;
	}

	buf = malloc_np(len);
	if (!buf)
	{
		goto fin;
	}

    status = ZwQueryInformationProcess(
            hProcess,
            ProcessImageFileName,
            buf,
            len,
            &len
        );
	if (!NT_SUCCESS(status) || len == 0)
	{
		free_np(buf);
		goto fin;
	}

	*pName = (PUNICODE_STRING)buf;
	status = STATUS_SUCCESS;

fin:

	if (hProcess != NULL)
	{
		ZwClose(hProcess);
	}

	if (eProcess != NULL)
	{
		ObDereferenceObject(eProcess);
	}

	return status;
}

NTSTATUS devctrl_getProcessName(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG	outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
	PUNICODE_STRING pName = NULL;
	ULONG		nameLen = 0;
	NTSTATUS 	status = STATUS_SUCCESS;

	if (ioBuffer && (inputBufferLength >= sizeof(ULONG)))
	{
		status = getProcessName((HANDLE)*(ULONG*)ioBuffer, &pName);
		if (NT_SUCCESS(status) && pName)
		{
			if (pName->Buffer && pName->Length)
			{
				nameLen = pName->Length+2;

				if (nameLen > outputBufferLength)
				{
					free_np(pName);
					irp->IoStatus.Status = STATUS_NO_MEMORY;
					irp->IoStatus.Information = nameLen;
					IoCompleteRequest(irp, IO_NO_INCREMENT);
					return STATUS_NO_MEMORY;
				}

				RtlStringCchCopyUnicodeString((PWSTR)ioBuffer, nameLen/2, pName);
			}
			free_np(pName);
		} else
		{
			status = STATUS_INVALID_PARAMETER;
		}
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = nameLen;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return status;
}

NTSTATUS devctrl_getDriverType(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

	if (ioBuffer && (outputBufferLength >= sizeof(ULONG)))
	{
		*(ULONG*)ioBuffer = DT_WFP;
		irp->IoStatus.Information = sizeof(ULONG);
	} else
	{
		irp->IoStatus.Information = 0;
	}

	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return STATUS_SUCCESS;
}

NTSTATUS devctrl_tcpAbort(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	PTCPCTX pTcpCtx = NULL;
	ENDPOINT_ID id;
	KLOCK_QUEUE_HANDLE lh;
	NTSTATUS 	status = STATUS_SUCCESS;

	if (ioBuffer && (inputBufferLength >= sizeof(ENDPOINT_ID)))
	{
		id = *(ENDPOINT_ID*)ioBuffer;

		KdPrint((DPREFIX"devctrl_tcpAbort[%I64u]\n", id));

		pTcpCtx = tcpctx_find(id);
		if (pTcpCtx)
		{
			sl_lock(&pTcpCtx->lock, &lh);
			pTcpCtx->abortConnection = TRUE;	
			sl_unlock(&lh);

#ifdef USE_NTDDI
#if (NTDDI_VERSION >= NTDDI_WIN8)
			FwpsFlowAbort(pTcpCtx->flowHandle);
#endif
#endif

			tcpctx_release(pTcpCtx);
		} else
		{
			status = STATUS_INVALID_PARAMETER;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return status;
}

NTSTATUS devctrl_addFlowCtl(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;

	if (ioBuffer && (outputBufferLength >= sizeof(ULONG)) && (inputBufferLength >= sizeof(NF_FLOWCTL_DATA)))
	{
		PNF_FLOWCTL_DATA pFlowData = (PNF_FLOWCTL_DATA)ioBuffer;

		*(ULONG*)ioBuffer = flowctl_add(pFlowData->inLimit, pFlowData->outLimit);
		irp->IoStatus.Information = sizeof(ULONG);
	} else
	{
		irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		irp->IoStatus.Information = 0;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return STATUS_SUCCESS;
}

NTSTATUS devctrl_deleteFlowCtl(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(ULONG)))
	{
		ULONG fcHandle = *(ULONG*)ioBuffer;

		if (flowctl_delete(fcHandle))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_INVALID_PARAMETER;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_setTCPFlowCtl(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_FLOWCTL_SET_DATA)))
	{
		PNF_FLOWCTL_SET_DATA pSetFlowLimit = (PNF_FLOWCTL_SET_DATA)ioBuffer;

		if (tcpctx_setFlowControlHandle(pSetFlowLimit->endpointId, pSetFlowLimit->fcHandle))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_INVALID_PARAMETER;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return status;
}

NTSTATUS devctrl_setUDPFlowCtl(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_FLOWCTL_SET_DATA)))
	{
		PNF_FLOWCTL_SET_DATA pSetFlowLimit = (PNF_FLOWCTL_SET_DATA)ioBuffer;

		if (udpctx_setFlowControlHandle(pSetFlowLimit->endpointId, pSetFlowLimit->fcHandle))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_INVALID_PARAMETER;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return status;
}

NTSTATUS devctrl_modifyFlowCtl(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_FLOWCTL_MODIFY_DATA)))
	{
		PNF_FLOWCTL_MODIFY_DATA pFlowData = (PNF_FLOWCTL_MODIFY_DATA)ioBuffer;

		if (flowctl_modifyLimits(pFlowData->fcHandle, pFlowData->data.inLimit, pFlowData->data.outLimit))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_INVALID_PARAMETER;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return status;
}

NTSTATUS devctrl_getFlowCtlStat(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	outputBufferLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (outputBufferLength >= sizeof(NF_FLOWCTL_STAT)) && (inputBufferLength >= sizeof(ULONG)))
	{
		ULONG fcHandle = *(ULONG*)ioBuffer;

		if (flowctl_getStat(fcHandle, (PNF_FLOWCTL_STAT)ioBuffer))
		{
			irp->IoStatus.Information = sizeof(NF_FLOWCTL_STAT);
			status = STATUS_SUCCESS;
		} else
		{
			irp->IoStatus.Information = 0;
			status = STATUS_INVALID_PARAMETER;
		}
	} else
	{
		irp->IoStatus.Information = 0;
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
		
	return status;
}

NTSTATUS devctrl_clearTempRules(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	NTSTATUS status;

	rules_remove_all_temp();

	status = STATUS_SUCCESS;

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addTempRule(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_RULE)))
	{
		PNF_RULE pRule = (PNF_RULE)ioBuffer;
		
		if (rules_add_temp(pRule))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_NO_MEMORY;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_setTempRules(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	NTSTATUS status;

	rules_set_temp();

	status = STATUS_SUCCESS;

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addBindingRule(PIRP irp, PIO_STACK_LOCATION irpSp, BOOLEAN toHead)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_BINDING_RULE)))
	{
		PNF_BINDING_RULE pRule = (PNF_BINDING_RULE)ioBuffer;
		
		rules_bindingAdd(pRule, toHead);

		status = STATUS_SUCCESS;
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_deleteBindingRules(PIRP irp, PIO_STACK_LOCATION irpSp)
{
	NTSTATUS status;

	rules_bindingRemove_all();

	status = STATUS_SUCCESS;

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addRuleEx(PIRP irp, PIO_STACK_LOCATION irpSp, BOOLEAN toHead)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_RULE_EX)))
	{
		PNF_RULE_EX pRule = (PNF_RULE_EX)ioBuffer;
		
		rules_addEx(pRule, toHead);

		status = STATUS_SUCCESS;
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS devctrl_addTempRuleEx(PIRP irp, PIO_STACK_LOCATION irpSp)
{
    PVOID	ioBuffer = irp->AssociatedIrp.SystemBuffer;
    ULONG	inputBufferLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS status;

	if (ioBuffer && (inputBufferLength >= sizeof(NF_RULE_EX)))
	{
		PNF_RULE_EX pRule = (PNF_RULE_EX)ioBuffer;
		
		if (rules_add_tempEx(pRule))
		{
			status = STATUS_SUCCESS;
		} else
		{
			status = STATUS_NO_MEMORY;
		}
	} else
	{
		status = STATUS_INVALID_PARAMETER;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}


NTSTATUS devctrl_dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP irp)
{
	PIO_STACK_LOCATION irpSp;
	
	UNREFERENCED_PARAMETER(DeviceObject);

	irpSp = IoGetCurrentIrpStackLocation(irp);
	ASSERT(irpSp);
	
	KdPrint((DPREFIX"devctrl_dispatch mj=%d\n", irpSp->MajorFunction));

	switch (irpSp->MajorFunction) 
	{
	case IRP_MJ_CREATE:
		return devctrl_create(irp, irpSp);

	case IRP_MJ_READ:
		return devctrl_read(irp, irpSp);

	case IRP_MJ_WRITE:
		return devctrl_write(irp, irpSp);

	case IRP_MJ_CLOSE:
		return devctrl_close(irp, irpSp);

	case IRP_MJ_DEVICE_CONTROL:
		switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
		{
		case IOCTL_DEVCTRL_OPEN:
			return devctrl_open(irp, irpSp);

		case NF_REQ_GET_PROCESS_NAME:
			return devctrl_getProcessName(irp, irpSp);

		case NF_REQ_GET_DRIVER_TYPE:
			return devctrl_getDriverType(irp, irpSp);

		case NF_REQ_TCP_ABORT:
			return devctrl_tcpAbort(irp, irpSp);

		case NF_REQ_ADD_FLOW_CTL:
			return devctrl_addFlowCtl(irp, irpSp);

		case NF_REQ_DELETE_FLOW_CTL:
			return devctrl_deleteFlowCtl(irp, irpSp);

		case NF_REQ_SET_TCP_FLOW_CTL:
			return devctrl_setTCPFlowCtl(irp, irpSp);

		case NF_REQ_SET_UDP_FLOW_CTL:
			return devctrl_setUDPFlowCtl(irp, irpSp);

		case NF_REQ_MODIFY_FLOW_CTL:
			return devctrl_modifyFlowCtl(irp, irpSp);

		case NF_REQ_GET_FLOW_CTL_STAT:
			return devctrl_getFlowCtlStat(irp, irpSp);

		case NF_REQ_CLEAR_TEMP_RULES:
			return devctrl_clearTempRules(irp, irpSp);

		case NF_REQ_ADD_TEMP_RULE:
			return devctrl_addTempRule(irp, irpSp);

		case NF_REQ_SET_TEMP_RULES:
			return devctrl_setTempRules(irp, irpSp);

		case NF_REQ_ADD_HEAD_BINDING_RULE:
			return devctrl_addBindingRule(irp, irpSp, TRUE);

		case NF_REQ_ADD_TAIL_BINDING_RULE:
			return devctrl_addBindingRule(irp, irpSp, FALSE);

		case NF_REQ_DELETE_BINDING_RULES:
			return devctrl_deleteBindingRules(irp, irpSp);

		case NF_REQ_ADD_HEAD_RULE_EX:
			return devctrl_addRuleEx(irp, irpSp, TRUE);

		case NF_REQ_ADD_TAIL_RULE_EX:
			return devctrl_addRuleEx(irp, irpSp, FALSE);

		case NF_REQ_ADD_TEMP_RULE_EX:
			return devctrl_addTempRuleEx(irp, irpSp);
		}
	}	

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

