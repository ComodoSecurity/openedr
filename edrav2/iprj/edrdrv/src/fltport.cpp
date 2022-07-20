//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Communication Port
///
/// @addtogroup edrdrv
/// @{
#include "common.h"
#include "osutils.h"
#include "workerthread.h"
#include "fltport.h"
#include "procmon.h"

namespace cmd {
namespace fltport {

using cmd::procmon::ContextPtr;
using cmd::procmon::getProcessContext;

//
// RawEvent queue 
//
struct RawEvent
{
	PVOID pDataBuffer;
	ULONG nDataSize;
};

//
//
//
struct RawEventQueue
{
private:
	static inline constexpr size_t c_MaxMessageCount = 0x4000;
	RawEvent m_rawEventQueue[c_MaxMessageCount];
	size_t m_nBeginIndex;
	size_t m_nEndIndex;

	size_t m_nTotalDataSize;
	size_t m_nMemoryLimit;

	bool m_fNoLogLimitExceeded;

	FAST_MUTEX m_mutex;

	//
	//
	//
	inline void logLimitExceeded()
	{
		if (m_fNoLogLimitExceeded) return;
		m_fNoLogLimitExceeded = true;
		LOGINFO1("[WARN] Fltport queue is full.\r\n");
	}

	//
	//
	//
	inline size_t getNextIndex(size_t index) noexcept
	{
		return (index + 1) % c_MaxMessageCount;
	}

	//
	//
	//
	inline NTSTATUS allocItem(const RawEvent& src, RawEvent& dst)
	{
		if (src.pDataBuffer == nullptr || src.nDataSize == 0)
		{
			RawEvent newEvent = {};
			dst = newEvent;
			return STATUS_SUCCESS;
		}

		if (m_nMemoryLimit != 0 && 
		   (m_nTotalDataSize + src.nDataSize) > m_nMemoryLimit)
			return STATUS_QUEUE_LIMIT_EXEEDED;

		PVOID pNewBuffer = ExAllocatePoolWithTag(PagedPool, 
			src.nDataSize, c_nAllocTag);
		if (pNewBuffer == nullptr) return STATUS_NO_MEMORY;

		m_nTotalDataSize += src.nDataSize;

		RawEvent newEvent = {};
		newEvent.nDataSize = src.nDataSize;
		newEvent.pDataBuffer = pNewBuffer;
		memcpy(pNewBuffer, src.pDataBuffer, src.nDataSize);

		dst = newEvent;

		return STATUS_SUCCESS;
	}

	//
	//
	//
	inline void freeItem(RawEvent& rawEvent)
	{
		if (rawEvent.pDataBuffer == nullptr)
			return;
		m_nTotalDataSize -= rawEvent.nDataSize;
		ExFreePoolWithTag(rawEvent.pDataBuffer, c_nAllocTag);
	}

	//
	// +FIXME: Not pop() but delete()
	//
	bool popFrontInternal()
	{
		if (m_nBeginIndex == m_nEndIndex)
			return false;

		RawEvent rawEvent = m_rawEventQueue[m_nBeginIndex];
		freeItem(rawEvent);
		m_nBeginIndex = getNextIndex(m_nBeginIndex);
		m_fNoLogLimitExceeded = false;
		return true;
	}

public:

	//
	//
	//
	void initialize()
	{
		// +FIXME: Why don't use std c++ initialization?
		RtlZeroMemory(this, sizeof(*this));
		ExInitializeFastMutex(&m_mutex);
	}

	//
	//
	//
	void finalize()
	{
		clear();
	}

	//
	// Add new item into queue
	//
	// @return STATUS_TOO_MANY_NODES if a limit is exceeded
	//
	NTSTATUS pushBack(const RawEvent& rawEvent)
	{
		ExAcquireFastMutex(&m_mutex);
		__try
		{
			const size_t nNewEndIndex = getNextIndex(m_nEndIndex);
			if (nNewEndIndex == m_nBeginIndex)
			{
				logLimitExceeded();
				return STATUS_QUEUE_LIMIT_EXEEDED;
			}

			RawEvent newItem = {};
			const NTSTATUS res = allocItem(rawEvent, newItem);
			if (res == STATUS_QUEUE_LIMIT_EXEEDED)
			{
				logLimitExceeded();
				return STATUS_QUEUE_LIMIT_EXEEDED;
			}
			IFERR_RET(res);

			m_rawEventQueue[m_nEndIndex] = newItem;
			m_nEndIndex = nNewEndIndex;
		}
		__finally
		{
			ExReleaseFastMutex(&m_mutex);
		}

		return STATUS_SUCCESS;
	}

	//
	//
	//
	bool getFront(RawEvent& rawEvent)
	{
		ExAcquireFastMutex(&m_mutex);
		__try
		{
			if (m_nBeginIndex == m_nEndIndex)
				return false;
			rawEvent = m_rawEventQueue[m_nBeginIndex];
		}
		__finally
		{
			ExReleaseFastMutex(&m_mutex);
		}

		return true;
	}

	//
	//
	//
	void popFront()
	{
		ExAcquireFastMutex(&m_mutex);
		__try
		{
			(void)popFrontInternal();
		}
		__finally
		{
			ExReleaseFastMutex(&m_mutex);
		}
	}

	//
	//
	//
	void clear()
	{
		ExAcquireFastMutex(&m_mutex);
		__try
		{
			while (popFrontInternal());
		}
		__finally
		{
			ExReleaseFastMutex(&m_mutex);
		}
	}

	//
	//
	//
	void setMemoryLimit(size_t nLimit)
	{
		m_nMemoryLimit = nLimit;
	}

	//
	//
	//
	bool isEmpty()
	{
		ExAcquireFastMutex(&m_mutex);
		const bool fIsEmpty = m_nBeginIndex == m_nEndIndex;
		ExReleaseFastMutex(&m_mutex);
		return fIsEmpty;
	}

	void logStatistic()
	{
		size_t nCount = 0;
		size_t nTotalDataSize = 0;
		ExAcquireFastMutex(&m_mutex);
		if (m_nEndIndex >= m_nBeginIndex)
			nCount = m_nEndIndex - m_nBeginIndex;
		else
			nCount = c_MaxMessageCount - m_nBeginIndex + m_nEndIndex;
		nTotalDataSize = m_nTotalDataSize;
		ExReleaseFastMutex(&m_mutex);
		LOGINFO2("EventQueue info: count: %u, size: %Iu.\r\n", nCount, nTotalDataSize);
	}
};


//
//
//

//#define ENABLE_LOGGING_STATISTIC

#ifdef ENABLE_LOGGING_STATISTIC
inline constexpr uint64_t c_nStatisticLogPeriod = 1 /*sec*/ * 1000 /*ms*/;
#endif

///
/// Global data of fltport.
///
struct PortData
{
	// CommunicationPort
	PFLT_PORT pServerPort; //< Filter server port 
	PFLT_PORT pClientPort; //< Filter client port 
	PFLT_PORT SenderServerProxyPort; //< Filter proxy server port, client port objects saved in per-process context when it connected
	PFLT_PORT ReceiverServerProxyPort; //< Receiver proxy server port
	PFLT_PORT ReceiverClientProxyPort; //< Receiver proxy client port

	RawEventQueue eventQueue; //< event container
	SystemThreadWorker workerThread; //< working thread

	uint64_t nSleepEndTime; //< Send messages is denied before this time (getTickCount) 
	uint64_t nMonitoringEndTime; //< Monitoring will be stopped after this time (getTickCount)  
	uint64_t nNextStatisticLogTime; //< Next time to log statistic

};


static PortData g_PortData = {0};


//
//
//
NTSTATUS checkClientConnection()
{
	if (g_PortData.pClientPort == NULL)
	{
		LOGINFO2("UserMode client is not connected.\r\n");
		return STATUS_UNSUCCESSFUL;
	}
	
	return STATUS_SUCCESS;
}

//
//
//
bool isClientConnected()
{
	if (!g_pCommonData->fFltPortIsInitialized)
		return false;

	return g_PortData.pClientPort != nullptr;
}

//
//
//
NTSTATUS sendRawEventInternal(const RawEvent& rawEvent)
{
	LARGE_INTEGER liTimeout = {};
	liTimeout.QuadPart = (LONGLONG)g_pCommonData->nFltPortSendMessageTimeout * 
		(LONGLONG)1000 /*micro*/ * (LONGLONG)10 /*100 nano*/ * (LONGLONG)-1/*relative*/;

	NTSTATUS eSendResult = STATUS_SUCCESS;
	if constexpr (edrdrv::c_nReplyMode)
	{
		UINT64 FictiveReplyBuffer;
		ULONG nReplySize = sizeof(FictiveReplyBuffer);
		eSendResult = FltSendMessage(g_pCommonData->pFilter, &g_PortData.pClientPort,
			rawEvent.pDataBuffer, rawEvent.nDataSize, &FictiveReplyBuffer, &nReplySize, &liTimeout);
	}
	else
		eSendResult = FltSendMessage(g_pCommonData->pFilter, &g_PortData.pClientPort,
			rawEvent.pDataBuffer, rawEvent.nDataSize, NULL, NULL, &liTimeout);

	return eSendResult;
}


//
//
//
NTSTATUS sendRawEvent(const void* pRawEvent, size_t nRawEventSize)
{
	if (!g_pCommonData->fFltPortIsInitialized)
		return LOGERROR(STATUS_UNSUCCESSFUL);

	if (nRawEventSize > ULONG_MAX) 
		return LOGERROR(STATUS_BUFFER_OVERFLOW);
	
	if (pRawEvent == nullptr || nRawEventSize == 0)
		return LOGERROR(STATUS_INVALID_PARAMETER);
	
	RawEvent rawEvent;
	rawEvent.pDataBuffer = (PVOID) pRawEvent;
	rawEvent.nDataSize = (ULONG) nRawEventSize;
	
#ifdef USE_SYNC_SEND
	return sendRawEventInternal(rawEvent);
#else
	const NTSTATUS res = g_PortData.eventQueue.pushBack(rawEvent);
	IFERR_RET(res);

	// Waking up the worker if event was added and a client is connected
	if (res == STATUS_SUCCESS && g_PortData.pClientPort != nullptr)
		g_PortData.workerThread.notify();

	return res;
#endif
}


//
// Sending one event in worker thread
//
DoWorkResult sendNextEvent(void* /*pContext*/)
{
	// No connection - returns no work
	if (g_PortData.pClientPort == nullptr)
	{
		// Check stop monitoring
		if (g_PortData.nMonitoringEndTime == 0) return DoWorkResult::NoWork;
		
		if (getTickCount64() > g_PortData.nMonitoringEndTime)
		{
			LOGINFO1("[WARN] Stop monitoring after unexpected disconnection.\r\n");
			stopMonitoring(true);
			g_PortData.nMonitoringEndTime = 0;
		}
		return DoWorkResult::NoWork;
	}

	// Log statistic
#ifdef ENABLE_LOGGING_STATISTIC
	if (g_PortData.nNextStatisticLogTime != 0)
	{
		if (getTickCount64() > g_PortData.nNextStatisticLogTime)
		{
			g_PortData.eventQueue.logStatistic();
			g_PortData.nNextStatisticLogTime = getTickCount64() + c_nStatisticLogPeriod;
		}
	}
#endif

	// No event - returns no work
	RawEvent rawEvent;
	if (!g_PortData.eventQueue.getFront(rawEvent))
		return DoWorkResult::NoWork;

	// Sleep time - returns no work
	if (g_PortData.nSleepEndTime != 0)
	{
		if(getTickCount64() < g_PortData.nSleepEndTime)
			return DoWorkResult::NoWork;
		g_PortData.nSleepEndTime = 0; // reset time
	}

	const NTSTATUS eSendResult = sendRawEventInternal(rawEvent);

	// Timeout is occurred - send NextEvent again immediately (timeout was already waited).
	if (eSendResult == STATUS_TIMEOUT)
	{
		LOGINFO2("[WARN] FltSendMessage timeout is occured.\r\n");
		return DoWorkResult::HasWork;
	}

	// Disconnection - stops sending.
	if (eSendResult == STATUS_PORT_DISCONNECTED)
	{
		LOGINFO2("[WARN] FltSendMessage disconnection is occured.\r\n");
		return DoWorkResult::NoWork;
	}

	// Unexpected error - starts sleep time.
	if (!NT_SUCCESS(eSendResult))
	{
		g_PortData.nSleepEndTime = getTickCount64() + g_pCommonData->nFltPortSendMessageTimeout;
		LOGERROR(eSendResult, "FltSendMessage returns error.\r\n");
		return DoWorkResult::NoWork;
	}

	// Success - removes event and returns actual status.
	g_PortData.eventQueue.popFront();
	return g_PortData.eventQueue.isEmpty() ? DoWorkResult::NoWork : DoWorkResult::HasWork;
}

//
//
//
NTSTATUS notifyOnConnect(
    _In_ PFLT_PORT ClientPort, _In_opt_ PVOID /*ServerPortCookie*/,
    _In_ PVOID /*ConnectionContext*/,
    _In_ ULONG /*SizeOfContext*/, _Outptr_result_maybenull_ PVOID* ppConnectionCookie)
{
	if (ppConnectionCookie != nullptr)
		*ppConnectionCookie = nullptr;

	// Support only one Client
	if (g_PortData.pClientPort != NULL) return LOGERROR(STATUS_UNSUCCESSFUL);
	g_PortData.pClientPort = ClientPort;

	procmon::ContextPtr pProcCtx;
	IFERR_LOG(procmon::fillCurProcContext(pProcCtx));

	// Mark connected process as trusted and notSendEvent
	if (pProcCtx)
	{
		pProcCtx->fIsTrusted = procmon::OptionState(true, procmon::OptionState::Reason::Forced, false);
		pProcCtx->fSendEvents = procmon::OptionState(false, procmon::OptionState::Reason::Forced, false);
	}

	g_pCommonData->nConnetedProcess = (ULONG_PTR)PsGetCurrentProcessId();
	g_PortData.nSleepEndTime = 0; // reset sleep time
	g_PortData.nMonitoringEndTime = 0;

#ifdef ENABLE_LOGGING_STATISTIC
	g_PortData.nNextStatisticLogTime = getTickCount64() + c_nStatisticLogPeriod;
#endif

	// Waking up the worker
	g_PortData.workerThread.notify();

	LOGINFO2("fltport connected from ProcessId: %Iu\r\n", (ULONG_PTR)PsGetCurrentProcessId());
    return STATUS_SUCCESS;
}

//
//
//
VOID notifyOnDisconnect(_In_opt_ PVOID /*ConnectionCookie*/)
{
	// Starting to wait timeout before stop monitoring
	g_PortData.nMonitoringEndTime = getTickCount64() + g_pCommonData->nFltPortConnectionTimeot;

	FltCloseClientPort(g_pCommonData->pFilter, &g_PortData.pClientPort);
	g_pCommonData->nConnetedProcess = 0;

#ifdef ENABLE_LOGGING_STATISTIC
	g_PortData.nNextStatisticLogTime = 0;
#endif

	LOGINFO2("fltport disconnected\r\n");
}

//
//
//
NTSTATUS createServerPort(PFLT_FILTER pFilter, PFLT_PORT *ppServerPort)
{
	PSECURITY_DESCRIPTOR sd = NULL;
	PFLT_PORT pServerPort = NULL;
	__try
	{
		IFERR_RET(FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS));

		PRESET_STATIC_UNICODE_STRING(c_usPortName, L"" CMD_EDRDRV_FLTPORT_NAME);

		OBJECT_ATTRIBUTES oa = {};
		InitializeObjectAttributes(&oa, &c_usPortName, 
			OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, sd);

		IFERR_RET(FltCreateCommunicationPort(pFilter, &pServerPort, &oa, nullptr,
			notifyOnConnect, notifyOnDisconnect, nullptr, 1));

		*ppServerPort = pServerPort;
		pServerPort = NULL;
		return STATUS_SUCCESS;
	}
	__finally
	{
		if (sd != NULL)
			FltFreeSecurityDescriptor(sd);
		if (pServerPort != NULL)
			FltCloseCommunicationPort(pServerPort);
		// +FIXME: What is the return status in case of error?
	}
}

//
//
//
NTSTATUS notifyOnProxyServerConnect(
	_In_ PFLT_PORT ClientPort,
	_In_opt_ PVOID ServerPortCookie,
	_In_ PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID* ppConnectionCookie)
{
	if (ppConnectionCookie != nullptr)
		*ppConnectionCookie = nullptr;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	ContextPtr processContext;
	IFERR_RET(getProcessContext(PsGetCurrentProcessId(), processContext));

	PFLT_PORT port = (PFLT_PORT)InterlockedExchangePointer((PVOID*)&processContext->ProxyPort, ClientPort);
	if (port != nullptr)
	{
		FltCloseClientPort(g_pCommonData->pFilter, &port);
	}

	LOGINFO1("fltProxyPort connected from ProcessId: %Iu\r\n", (ULONG_PTR)PsGetCurrentProcessId());
	return STATUS_SUCCESS;
}

//
//
//
VOID notifyOnProxyServerDisconnect(_In_opt_ PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	ContextPtr processContext;
	if (!NT_SUCCESS(getProcessContext(PsGetCurrentProcessId(), processContext)))
		return;

	PFLT_PORT port = (PFLT_PORT)InterlockedExchangePointer((PVOID*)&processContext->ProxyPort, nullptr);
	if (port != nullptr)
	{
		FltCloseClientPort(g_pCommonData->pFilter, &port);
	}

	LOGINFO1("fltProxyServerPort disconnected from ProcessId: %Iu\r\n", (ULONG_PTR)PsGetCurrentProcessId());
}

NTSTATUS
notifyOnProxyServerMessage(
	_In_opt_ PVOID PortCookie,
	_In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
	_In_ ULONG InputBufferLength,
	_Out_writes_bytes_to_opt_(OutputBufferLength, *ReturnOutputBufferLength) PVOID OutputBuffer,
	_In_ ULONG OutputBufferLength,
	_Out_ PULONG ReturnOutputBufferLength
	)
{
	UNREFERENCED_PARAMETER(PortCookie);

	if (nullptr == g_PortData.ReceiverClientProxyPort)
		return LOGERROR(STATUS_PORT_DISCONNECTED, "Failed to send message, client proxy disconnected\r\n");

	if (nullptr == InputBuffer || 0 == InputBufferLength)
	{
		NT_ASSERT(FALSE); // strange behavior
		return STATUS_SUCCESS;
	}

	Blob replyBuffer;
	IFERR_RET(replyBuffer.alloc(OutputBufferLength), "Failed to allocate memory\r\n");

	NT_ASSERT(g_pCommonData->pFilter);
	NT_ASSERT(g_PortData.ReceiverClientProxyPort);

	NTSTATUS status = STATUS_UNSUCCESSFUL;

	__try
	{	
		LARGE_INTEGER liTimeout = {}; // 20sec by default
		liTimeout.QuadPart = (LONGLONG)g_pCommonData->nFltPortSendMessageTimeout *
			(LONGLONG)1000 /*micro*/ * (LONGLONG)100 /*100 nano*/ * (LONGLONG)-1/*relative*/;

		ULONG resultLength = OutputBufferLength;
		status = FltSendMessage(
			g_pCommonData->pFilter,
			&g_PortData.ReceiverClientProxyPort,
			InputBuffer,
			InputBufferLength,
			replyBuffer.getData(),
			&resultLength,
			&liTimeout);

		if (NT_SUCCESS(status) || (STATUS_BUFFER_OVERFLOW == status) || (STATUS_BUFFER_TOO_SMALL == status) || (STATUS_INFO_LENGTH_MISMATCH == status))
		{
			if (ARGUMENT_PRESENT(ReturnOutputBufferLength))
			{
				*ReturnOutputBufferLength = resultLength;
			}

			if (NT_SUCCESS(status) || (STATUS_BUFFER_OVERFLOW == status))
			{
				if (ARGUMENT_PRESENT(OutputBuffer) && (OutputBufferLength >= resultLength))
				{
					RtlCopyMemory(OutputBuffer, replyBuffer.getData(), resultLength);
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		status = GetExceptionCode();
	}

	IFERR_RET(status, "notifyOnProxyServerMessage failed\r\n");
	return status;
}

//
// 
//
NTSTATUS notifyOnProxyClientConnect(
	_In_ PFLT_PORT ClientPort,
	_In_opt_ PVOID ServerPortCookie,
	_In_ PVOID ConnectionContext,
	_In_ ULONG SizeOfContext,
	_Outptr_result_maybenull_ PVOID* ppConnectionCookie)
{
	if (ppConnectionCookie != nullptr)
		*ppConnectionCookie = nullptr;

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionContext);
	UNREFERENCED_PARAMETER(SizeOfContext);

	PFLT_PORT port = (PFLT_PORT)InterlockedExchangePointer((PVOID*)&g_PortData.ReceiverClientProxyPort, ClientPort);
	if (port != nullptr)
	{
		FltCloseClientPort(g_pCommonData->pFilter, &port);
	}

	LOGINFO1("fltProxyClientPort connected from ProcessId: %Iu\r\n", (ULONG_PTR)PsGetCurrentProcessId());
	return STATUS_SUCCESS;
}

//
//
//
VOID notifyOnProxyClientDisconnect(_In_opt_ PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);

	g_pCommonData->fProxyShutdown = true;

	PFLT_PORT port = (PFLT_PORT)InterlockedExchangePointer((PVOID*)&g_PortData.ReceiverClientProxyPort, nullptr);
	if (port != nullptr)
	{
		FltCloseClientPort(g_pCommonData->pFilter, &port);
	}

	LOGINFO1("fltProxyClientPort disconnected from ProcessId: %Iu\r\n", (ULONG_PTR)PsGetCurrentProcessId());
}

//
//
//
NTSTATUS createProxyPort(PFLT_FILTER Filter, PFLT_PORT* SenderServerProxyPort, PFLT_PORT* ReceiverServerProxyPort)
{
	UniquePtr<SECURITY_DESCRIPTOR> securityDescriptor(cmd::createFltPortEveryoneAllAccessAllowMandantorySD());
	PFLT_PORT portSenderServer = nullptr, portReceiverServer = nullptr;
	
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	__try
	{
		PRESET_STATIC_UNICODE_STRING(inPortName, L"" CMD_EDRDRV_FLTPORT_PROCMON_IN_NAME);
		PRESET_STATIC_UNICODE_STRING(outPortName, L"" CMD_EDRDRV_FLTPORT_PROCMON_OUT_NAME);

		OBJECT_ATTRIBUTES oa = {};
		InitializeObjectAttributes(&oa, &inPortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, securityDescriptor.get());

		IFERR_RET(FltCreateCommunicationPort(Filter, &portSenderServer, &oa, nullptr,
			notifyOnProxyServerConnect, notifyOnProxyServerDisconnect, notifyOnProxyServerMessage, MAXLONG));

		InitializeObjectAttributes(&oa, &outPortName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, securityDescriptor.get());

		status = FltCreateCommunicationPort(Filter, &portReceiverServer, &oa, nullptr, notifyOnProxyClientConnect, notifyOnProxyClientDisconnect, nullptr, 1);
		if (!NT_SUCCESS(status))
			__leave;

		*SenderServerProxyPort = portSenderServer;
		portSenderServer = nullptr;

		*ReceiverServerProxyPort = portReceiverServer;
		portReceiverServer = nullptr;

		return STATUS_SUCCESS;
	}
	__finally
	{
		if (portSenderServer != nullptr)
			FltCloseCommunicationPort(portSenderServer);

		if (portReceiverServer != nullptr)
			FltCloseCommunicationPort(portReceiverServer);
	}
	return status;
}

//
//
//
void start()
{
	if (!g_pCommonData->fFltPortIsInitialized)
		return;

	g_PortData.workerThread.setWakeupInterval(1000);
}

//
//
//
void stop(bool fClearQueue)
{
	g_PortData.workerThread.setWakeupInterval(10000);
	if (fClearQueue)
		g_PortData.eventQueue.clear();
}

//
//
//
void reloadConfig()
{
	if (!g_pCommonData->fFltPortIsInitialized) return;
	g_PortData.eventQueue.setMemoryLimit((size_t)g_pCommonData->nFltPortMaxQueueSize);
}

//
//
//
NTSTATUS initialize()
{
	if (g_pCommonData->fFltPortIsInitialized)
		return STATUS_SUCCESS;

	LOGINFO2("Create a communication port.\r\n");

	RtlZeroMemory(&g_PortData, sizeof(g_PortData));

	bool fQueueIsInitialized = false;
	bool fWorkerThreadIsReady = false;

	__try
	{
		IFERR_RET(createServerPort(g_pCommonData->pFilter, &g_PortData.pServerPort));

		g_PortData.eventQueue.initialize();
		fQueueIsInitialized = true;

		fWorkerThreadIsReady = g_PortData.workerThread.initialize(&sendNextEvent, nullptr, 1000);
		if (!fWorkerThreadIsReady)
		{
			// +FIXME: Who calls FltCloseCommunicationPort() for allocated port here?
			LOGERROR("Can't start worker thread for event port.\r\n");
			return STATUS_UNSUCCESSFUL;
		}

		NTSTATUS status = createProxyPort(g_pCommonData->pFilter, &g_PortData.SenderServerProxyPort, &g_PortData.ReceiverServerProxyPort);
		if (!NT_SUCCESS(status))
			__leave;

		g_pCommonData->fFltPortIsInitialized = true;
		g_pCommonData->fProxyShutdown = false;
	}
	__finally
	{
		// +FIXME: We don't need global flag check. We just free resources!
		// If initialization is successful, free is not called
		if (!g_pCommonData->fFltPortIsInitialized)
		{
			if (fWorkerThreadIsReady)
				g_PortData.workerThread.finalize();

			if (g_PortData.pServerPort != nullptr)
			{
				FltCloseCommunicationPort(g_PortData.pServerPort);
				g_PortData.pServerPort = nullptr;
			}

			if (fQueueIsInitialized)
				g_PortData.eventQueue.finalize();
		}
	}

	return STATUS_SUCCESS;
}

//
//
//
void finalize()
{
	// +FIXME: The same idea. Why do we check special flags?
	// If initialization is not called, we should not call finalize
	if (!g_pCommonData->fFltPortIsInitialized)
		return;

	g_pCommonData->fProxyShutdown = true;

	g_PortData.workerThread.finalize();

	if (g_PortData.pServerPort != nullptr)
	{
		FltCloseCommunicationPort(g_PortData.pServerPort);
		g_PortData.pServerPort = nullptr;
	}

	if (g_PortData.ReceiverServerProxyPort != nullptr)
	{
		FltCloseCommunicationPort(g_PortData.ReceiverServerProxyPort);
		g_PortData.ReceiverServerProxyPort = nullptr;
	}

	if (g_PortData.SenderServerProxyPort != nullptr)
	{
		FltCloseCommunicationPort(g_PortData.SenderServerProxyPort);
		g_PortData.SenderServerProxyPort = nullptr;
	}

	cmd::procmon::closeAllClientPorts();

	g_PortData.eventQueue.finalize();
	g_pCommonData->fFltPortIsInitialized = false;
}

}
} // namespace cmd
/// @}
