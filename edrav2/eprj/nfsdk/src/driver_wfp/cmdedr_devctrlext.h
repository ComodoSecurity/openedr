//
// Comodo EDR extension of NetFilter SDK
//
// This file should be included in devctrl.c
//

//
// Push custom data to usermode queue
//
EXTERN_C NTSTATUS cmdedr_pushCustomData(UINT16 nType, void* pData, EndpointId nId)
{
	// convert type to code
	int nCode = ((int)0x10000) | nType;
	PNF_QUEUE_ENTRY pEntry;
	KLOCK_QUEUE_HANDLE lh;

	KdPrint((DPREFIX"cmdedr_pushCustomData type=%d\n", nType));

	if (!devctrl_isProxyAttached())
		return STATUS_UNSUCCESSFUL;

	pEntry = (PNF_QUEUE_ENTRY)ExAllocateFromNPagedLookasideList(&g_ioQueueList);
	if (!pEntry)
		return STATUS_UNSUCCESSFUL;
	memset(pEntry, 0, sizeof(NF_QUEUE_ENTRY));

	pEntry->id = nId;
	pEntry->code = nCode;
	pEntry->context.TCP.tcpCtx = (PTCPCTX)pData;

	sl_lock(&g_slIoQueue, &lh);
	InsertTailList(&g_ioQueue, &pEntry->entry);
	sl_unlock(&lh);

	KeSetEvent(&g_ioThreadEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_SUCCESS;
}

// 
// Write custom data to usermode buffer
//
NTSTATUS cmdedr_popCustomDataInt(PNF_QUEUE_ENTRY pEntry, UINT64* pOffset)
{
	KdPrint((DPREFIX"cmdedr_popCustomDataInt\n"));

	// convert code to type
	UINT16 nType = (UINT16)(pEntry->code & 0xFFFF);
	void* pData = pEntry->context.TCP.tcpCtx;

	PNF_DATA pHeader = (PNF_DATA)((char*)g_inBuf.kernelVa + *pOffset);
	UINT64 nBufferSize = g_inBuf.bufferLength - *pOffset;
	static const UINT64 c_nHeaderSize = sizeof(NF_DATA) - sizeof(pHeader->buffer);
	if (nBufferSize < c_nHeaderSize)
		return STATUS_NO_MEMORY;

	pHeader->code = pEntry->code;
	pHeader->id = pEntry->id;

	
	UINT64 nDataSize = nBufferSize - c_nHeaderSize;
	NTSTATUS ns = cmdedr_popCustomData(nType, pData, pHeader->buffer, &nDataSize);
	// Check errors
	if (!NT_SUCCESS(ns))
	{
		// If unsuported nType - skip pEntry
		if (ns == STATUS_NOT_SUPPORTED)
			return STATUS_SUCCESS;
		// If small buffer - wait next buffer
		if (ns == STATUS_NO_MEMORY)
			return STATUS_NO_MEMORY;

		// Other errors - skip data
		cmdedr_freeCustomData(nType, pData, pEntry->id);
		ASSERT(0);
		return STATUS_SUCCESS;
	}

	pHeader->bufferSize = (unsigned long) nDataSize;
	*pOffset += nDataSize + c_nHeaderSize;
	cmdedr_freeCustomData(nType, pData, pEntry->id);

	KdPrint((DPREFIX"cmdedr_popCustomDataInt finished\n"));

	return STATUS_SUCCESS;
}

// 
// Free custom data
//
void cmdedr_freeCustomDataInt(PNF_QUEUE_ENTRY pEntry)
{
	// convert code to type
	UINT16 nType = (UINT16)(pEntry->code & 0xFFFF);
	void* pData = pEntry->context.TCP.tcpCtx;
	cmdedr_freeCustomData(nType, pData, pEntry->id);
}
