//
// Comodo EDR extension of NetFilter SDK
//
#pragma once

// Redefine log prefix
#undef DPREFIX
#define DPREFIX "[CMD] "

//
// Checks that need to collect events for the process
//
EXTERN_C BOOLEAN cmdedr_isProcessInWhiteList(ULONG_PTR nProcessId);


//////////////////////////////////////////////////////////////////////////
//
// Callouts API
//

//
// Register additional EDR callouts
// It should be called inside transaction
//
EXTERN_C NTSTATUS cmdedr_registerCallouts(void* pDeviceObject);

//
// Unregister additional EDR callouts
//
EXTERN_C void cmdedr_unregisterCallouts();


struct AddingFilterContext
{
	HANDLE hEngine;
	const FWPM_SUBLAYER* pSubLayer;
	const FWPM_SUBLAYER* pRecvSubLayer;
};

//
// Add filters for additional EDR callouts
// It should be called inside transaction
//
EXTERN_C NTSTATUS cmdedr_addFilters(struct AddingFilterContext* pContext);


//////////////////////////////////////////////////////////////////////////
//
// Send data to user mode API
//

typedef UINT64 EndpointId;

//
// Add data to sending queue (send to user mode). Analog devctrl_pushTcpData.
// It is implemented in devctrl.c because it uses its static variables
//
// @param nType - message type
// @param pData - message data
// @param nId - endpoint ID (optional, just pass to user mode)
//
EXTERN_C NTSTATUS cmdedr_pushCustomData(UINT16 nType, void* pData, EndpointId nId);

//
// Write data to user mode buffer. Analog devctrl_popTcpData.
// Returns STATUS_NO_MEMORY if buffer is too small
// Returns STATUS_NOT_SUPPORTED type is not supported
//
// @param nType - message type (which was passed to cmdedr_pushCustomData)
// @param pData - message data (which was passed to cmdedr_pushCustomData)
// @param pOutBuffer - buffer for writing
// @param pnBufferSize [in, out] - Pointer to buffer size, on success should contain writen size
//
EXTERN_C NTSTATUS cmdedr_popCustomData(UINT16 nType, void* pData, void* pOutBuffer, UINT64* pnBufferSize);

//
// Free data from sending queue (pData).
//
// @param nType - message type (which was passed to cmdedr_pushCustomData)
// @param pData - message data (which was passed to cmdedr_pushCustomData)
// @param nId - ENDPOINT_ID nId (which was passed to cmdedr_pushCustomData)
//
EXTERN_C void cmdedr_freeCustomData(UINT16 nType, void* pData, EndpointId nId);
