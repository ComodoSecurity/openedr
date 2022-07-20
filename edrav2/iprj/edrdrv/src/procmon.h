//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (31.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Process Context
///
/// @addtogroup edrdrv
/// @{
#pragma once
#include "common.h"
#include "procutils.h"
#include "osutils.h"

namespace cmd {
namespace procmon {
	
//
// Process option state
//
struct OptionState
{
public:
	enum class Reason
	{
		NotInit = 0,
		Default = 1,
		ByRule = 2,
		Inhereted = 3,
		Forced = 4,
	};

private:
	uint8_t m_fValue : 1; // Value (true or false)
	uint8_t m_fInherited : 1; // Value is inherited by child
	uint8_t m_eReason : 3; // Reason

	void setReason(Reason eReason)
	{
		m_eReason = (uint8_t)eReason;
	}

public:

	OptionState()
	{
		m_fValue = 0;
		m_fInherited = 0;
		setReason(Reason::NotInit);
	}

	OptionState(bool fValue, Reason eReason, bool fInhereted)
	{
		m_fValue = fValue ? 1 : 0;
		m_fInherited = fInhereted ? 1 : 0;
		setReason(eReason);
	}

	operator bool () const
	{
		return m_fValue == 1;
	}

	bool isInhereted() const
	{
		return m_fInherited == 1;
	}

	Reason getReason() const
	{
		return (Reason)(uint8_t)m_eReason;
	}

	bool isForced()
	{
		return getReason() == Reason::Forced;
	}

	bool isInitialized()
	{
		return getReason() != Reason::NotInit;
	}

	OptionState copyWithReason(Reason eReason) const
	{
		return OptionState(m_fValue, eReason, m_fInherited);
	}

	const char* printState() const
	{
		if (getReason() == Reason::NotInit)
			return "none";
		if(m_fValue)
			return "true";
		else
			return "false";
	}
};

///
/// The class for event filter.
///
template<typename EventHash_>
class EventFilter
{
public:
	typedef EventHash_ EventHash; ///< Key type (usually hash)
	typedef uint32_t EventTime; ///< type to save event time
private:
	typedef HashMap<EventHash, EventTime> FilterMap;

	FilterMap m_filterMap;
	FastMutex m_mtxFilterMap;

	// garbage collection

	// GC runs inside checkEventSend() if ANY condition is true:
	// * m_nMaxGCperiod ms have passed since the last GC
	// * m_nMaxGCEventCount events allowed since the last GC
	uint64_t m_nMaxGCperiod = 0; // (ms) minimal period between GC
	size_t m_nMaxGCEventCount = 0; // minimal allowed events between GC

	uint64_t m_nLastGCTime = 0; // Time of Last GC
	size_t m_nEventCountSinceLastGC = 0; // Allowed event count since 

	//
	// Should be executed with locked m_filterMap
	//
	void runGNInt(EventTime nSkipTimeout)
	{
		uint64_t nCurTime = getTickCount64();
		EventTime nCurEventTime = (EventTime)nCurTime;

		auto endIter = m_filterMap.end();
		for (auto iter = m_filterMap.begin(); iter != endIter;)
		{
			if (nCurEventTime - iter->second < nSkipTimeout)
				++iter;
			else
				iter = m_filterMap.remove(iter);
		}

		m_nLastGCTime = nCurTime;
		m_nEventCountSinceLastGC = 0;
	}

public:
	///
	/// Constructor.
	///
	/// @param nMaxGCperiod - GC parameter (see GC description).
	/// @param nMaxGCEventCount - GC parameter (see GC description).
	///
	EventFilter(uint64_t nMaxGCperiod, size_t nMaxGCEventCount) :
		m_nMaxGCperiod(nMaxGCperiod), m_nMaxGCEventCount(nMaxGCEventCount)
	{
	}
		
	// Deny copy & move
	EventFilter(const EventFilter&) = delete;
	EventFilter(EventFilter&&) = delete;
	EventFilter& operator=(const EventFilter&) = delete;
	EventFilter& operator=(EventFilter&&) = delete;

	///
	/// Checks that event is allowed to send.
	///
	/// @param nHash - event hash.
	/// @param nSkipTimeout - timeout between same events sending.
	/// @return The function returns `true` if event sending is allowed.
	///
	/// If event with specified hash was happened less than nSkipTimeout ms, it is not allowed (returns false).
	/// If event is allowed (returns true), the method save its time.
	/// Runs GC inside if need.
	///
	bool checkEventSend(EventHash nHash, EventTime nSkipTimeout)
	{
		uint64_t nCurTime = getTickCount64();
		EventTime nEventTime = (EventTime)nCurTime;

		ScopedLock lock(m_mtxFilterMap);

		// Check event repeat
		EventTime* pLastEventTime = nullptr;
		IFERR_LOG(m_filterMap.findOrInsert(nHash, &pLastEventTime));
		// Always allow on error
		if (pLastEventTime == nullptr)
			return true;

		// Check timeout
		if ((nEventTime - *pLastEventTime) < nSkipTimeout)
			return false;
		*pLastEventTime = nEventTime;
		++m_nEventCountSinceLastGC;

		// Check GC is needed
		if ((nCurTime - m_nLastGCTime >= m_nMaxGCperiod) || m_nEventCountSinceLastGC >= m_nMaxGCEventCount)
			runGNInt(nSkipTimeout);

		return true;
	}

	///
	/// Runs GC forcibly.
	///
	/// @param nSkipTimeout - timeout.
	///
	void runGC(EventTime nSkipTimeout)
	{
		ScopedLock lock(m_mtxFilterMap);
		runGNInt(nSkipTimeout);
	}
};

///
/// Process context.
///
struct Context
{
	// FIXME: strange names with _ prefix
	HANDLE _ProcessId; ///< Internal data - Don't use
	volatile LONG _RefCount; ///< Internal data - Don't use
	LIST_ENTRY _ListEntry; ///< Internal data - Don't use
	BOOLEAN _fIsFilled; ///< Internal data - Don't use ///< Internal data - Don't use
	volatile PFLT_PORT ProxyPort; //< Filter proxy port 

	// Context
	CommonProcessInfo processInfo; 

	// Process options
	OptionState fSendEvents; ///< Send events from this process
	OptionState fEnableInject; ///< Allow inject into this process
	OptionState fIsProtected; ///< Self defense is applied for the process
	OptionState fIsTrusted; ///< process ignore self defense

	// Objmon info
	struct OpenProcessInfo
	{
		uint64_t m_LastOpenSend = 0;
	};
	typedef HashMap<uint32_t,  OpenProcessInfo> OpenProcessFilter;
	OpenProcessFilter* pOpenProcessFilter; // Synced by g_pCommonData->mtxOpenProcessFilter
	uint64_t nLastGarbageCollectTime; // Last time of clearing ProcessFilter


	typedef EventFilter<uint64_t> RegMonEventFilter;
	RegMonEventFilter* pRegMonEventFilter;

	NTSTATUS init();
	void free();
	void closePort();
};

//
// Release context.
// @param pCtx[in] - Context. Allow nullptr.
//
void releaseContext(Context* pCtx);

///
/// Smart pointer for procmon::Context.
///
class ContextPtr
{
private:
	Context* m_pCtx = nullptr;

	void cleanup()
	{
		if(m_pCtx != nullptr)
			releaseContext(m_pCtx);
		m_pCtx = nullptr;
	}

public:

	///
	/// Constructor.
	///
	explicit ContextPtr(Context* pCtx = nullptr) : m_pCtx(pCtx)
	{
	}

	///
	/// Destructor.
	///
	~ContextPtr()
	{
		reset();
	}

	//
	// deny coping
	//
	ContextPtr(const ContextPtr&) = delete;
	ContextPtr& operator= (const ContextPtr&) = delete;

	//
	// move constructor
	//
	ContextPtr(ContextPtr && other) : m_pCtx(other.release())
	{
	}

	//
	// move assignment
	//
	ContextPtr& operator= (ContextPtr && other)
	{
		if (this != &other)
			reset(other.release());
		return *this;
	}

	//
	// operator bool
	//
	bool isValid() const
	{
		return m_pCtx != nullptr;
	}

	//
	// operator Context*
	//
	operator Context*() const
	{
		return m_pCtx;
	}

	//
	// operator->
	//
	Context* operator->() const
	{
		return m_pCtx;
	}

	///
	/// Releases the context.
	///
	/// @return The function returns a raw pointer to the context.
	///
	Context* release()
	{
		Context* pCtx = m_pCtx;
		m_pCtx = nullptr;
		return pCtx;
	}

	///
	/// Resets the context.
	///
	/// @param pCtx [opt] - a raw pointer to the context (default: `nullptr`).
	///
	void reset(Context* pCtx = nullptr)
	{
		cleanup();
		m_pCtx = pCtx;
	}
};

///
/// Getting context by PID.
///
/// If if it is not filled, do it.
///
/// @param nPid - process id.
/// @param hProcess [opt] - process handle.
/// @param pCtx [out] - result pointer.
/// @return The function returns a NTSTATUS of the operation.
///
/// nPid or hProcess sould be specified
///
NTSTATUS fillContext(HANDLE nPid, HANDLE hProcess, ContextPtr& pCtx);

///
/// Getting context of current process.
///
/// @param pCtx - context.
/// @return The function returns a NTSTATUS of the operation.
///
/// If if it is not filled, do it.
///
inline NTSTATUS fillCurProcContext(ContextPtr& pCtx)
{
	return fillContext(PsGetCurrentProcessId(), ZwCurrentProcess(), pCtx);
}

///
/// Calling the fnProcess for each process context.
///
/// @param fnProcess - processor function. It will be called for every Context. 
/// @return The function returns a NTSTATUS of the operation.
///
/// If fnProcess returns not STATUS_SUCCESS, applyForEachProcessCtx break Context enumeration
///
NTSTATUS applyForEachProcessCtx(NTSTATUS (*fnProcess)(Context* /*pProcCtx*/, void* /*pCallContext*/), void* pCallContext);

///
/// Calling the processFn for each exist process context.
///
/// Adapter for lambda
///
template<typename Fn>
NTSTATUS applyForEachProcessCtx(Fn fnProcessor)
{
	struct FnAdapter
	{
		Fn& m_fnProcessor;
		FnAdapter(Fn& fnProcessor) : m_fnProcessor(fnProcessor) {}

		static NTSTATUS process(Context* pProcCtx, void* pCallContext)
		{
			auto pThis = (FnAdapter*)pCallContext;
			return pThis->m_fnProcessor(pProcCtx);
		}
	};

	// +FIXME: Why we are doing this call using additional object?
	FnAdapter adapter(fnProcessor);
	return applyForEachProcessCtx(&FnAdapter::process, &adapter);
}

///
/// Repeates process rules application.
///
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS reapplyRulesForAllProcesses();


///
/// Checks a process is in white list.
///
/// @param pProcessCtx - a raw pointer to the context.
/// @return The function returns `true` if the process is in the white list.
///
inline bool isProcessInWhiteList(Context* pProcessCtx)
{
	if (pProcessCtx == nullptr)
		return false;
	if (!pProcessCtx->fSendEvents)
		return true;
	return false;
}

///
/// Checks a process is in white list.
///
/// @param nProcessId - a process pid.
/// @return The function returns `true` if the process is in the white list.
///
inline bool isProcessInWhiteList(ULONG_PTR nProcessId)
{
	if (nProcessId == 0)
		return false;

	ContextPtr pProcessCtx;
	(void)fillContext((HANDLE)nProcessId, nullptr, pProcessCtx);
	return isProcessInWhiteList(pProcessCtx);
}

///
/// Checks a process is in white list.
///
/// @return The function returns `true` if the current process is in the white list.
///
inline bool isCurProcessInWhiteList()
{
	ContextPtr pProcessCtx;
	(void)fillCurProcContext(pProcessCtx);
	return isProcessInWhiteList(pProcessCtx);
}


///
/// Checks a process is trusted.
///
/// @param pProcessCtx - a raw pointer to the context.
/// @return The function returns `true` if the process is trusted.
///
inline bool isProcessTrusted(Context* pProcessCtx)
{
	if (g_pCommonData->fDisableSelfProtection)
		return true;
	if (pProcessCtx == nullptr)
		return false;
	if (pProcessCtx->fIsTrusted)
		return true;
	return false;
}

///
/// Checks a process is trusted.
///
/// @param nProcessId - a process pid.
/// @return The function returns `true` if the process is trusted.
///
inline bool isProcessTrusted(ULONG_PTR nProcessId)
{
	if (g_pCommonData->fDisableSelfProtection)
		return true;

	if (nProcessId == 0)
		return false;

	ContextPtr pProcessCtx;
	(void)fillContext((HANDLE)nProcessId, nullptr, pProcessCtx);
	return isProcessTrusted(pProcessCtx);
}

///
/// Checks the current process is trusted
///
inline bool isCurProcessTrusted()
{
	if (g_pCommonData->fDisableSelfProtection)
		return true;

	ContextPtr pProcessCtx;
	(void)fillCurProcContext(pProcessCtx);
	return isProcessTrusted(pProcessCtx);
}


///
/// Update process rules.
///
/// @param pData - a raw pointer to the data.
/// @param nDataSize - a size of the data.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS updateProcessRules(const void* pData, size_t nDataSize);

///
/// Set process info.
///
/// @param deserializer - deserializer.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS setProcessInfo(variant::BasicLbvsDeserializer<SetProcessInfoField>& deserializer);


///
/// Initialization.
///
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS initialize();

///
/// Finalization.
///
void finalize();

///
/// Get context of process.
///
NTSTATUS getProcessContext(HANDLE ProcessId, ContextPtr& ProcessContext);


///
/// Close all client proxy ports for all processes.
///
void closeAllClientPorts();

} // namespace procmon
} // namespace cmd
/// @}
