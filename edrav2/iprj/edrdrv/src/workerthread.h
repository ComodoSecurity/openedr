//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (12.02.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file SystemThread worker for async tasks
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace openEdr {

//
// 
//
enum class DoWorkResult
{
	HasWork, // Work is still exist. Call again immediately.
	NoWork, // There is no work. Go to sleep until notification.
	Terminate // Terminate processing.
};

///
/// Background worker.
///
struct SystemThreadWorker
{
public:
	// Callback to do a work
	typedef DoWorkResult (*PFnDoWork)(void* pContext);

private:

	void* m_pContext; //< Context of the callbacks
	PFnDoWork m_pfnDoWork; //< Callback to do a work

	bool m_fIsInitialized;
	bool m_fTerminated;

	void* m_ThreadObj; // SystemThread object
	KEVENT m_Event; // notification event
	FAST_MUTEX m_Mutex; 

	ULONG m_nWakeupInterval; //< call doWork every period without notification (milliseconds)

	//
	// static wrapper of runProcessing
	//
	static VOID run(PVOID pStartContext)
	{
		auto pSystemThreadWorker = (SystemThreadWorker*)pStartContext;		
		pSystemThreadWorker->doProcessing();
		PsTerminateSystemThread(STATUS_SUCCESS);
	}

	//
	// Main process circle
	//
	void doProcessing()
	{
		DoWorkResult ePreviousResult = DoWorkResult::NoWork;

		while (ePreviousResult != DoWorkResult::Terminate)
		{
			if (ePreviousResult != DoWorkResult::HasWork)
			{
				LARGE_INTEGER liTimeout = {};
				PLARGE_INTEGER pTimeout = nullptr;
				ExAcquireFastMutex(&m_Mutex);
				if (m_nWakeupInterval != 0)
				{
					liTimeout.QuadPart = (LONGLONG)m_nWakeupInterval * 
						(LONGLONG)1000 /*micro*/ * (LONGLONG)10 /*100 nano*/ * (LONGLONG)-1/*relative*/;
					pTimeout = &liTimeout;
				}
				ExReleaseFastMutex(&m_Mutex);

				const NTSTATUS ns = KeWaitForSingleObject(&m_Event, Executive, KernelMode, FALSE, pTimeout);
				if (!NT_SUCCESS(ns)) { IFERR_LOG(ns); break; }
				KeClearEvent(&m_Event);
			}

			if (m_fTerminated) break;

			// Process Data
			ePreviousResult = m_pfnDoWork(m_pContext);
		}

		m_fTerminated = true;
	}

public:

	///
	/// Constructor.
	///
	/// It must be called once per object.
	/// @param pfnHasWork - callback to check that a work is exist (Attention: called under mutex!!!).
	/// @param pfnDoWork - callback to do a work. If pfnDoWork returns error - Worker terminates processing.
	/// @param pContext - context for the callbacks.
	/// @param nWakeupInterval - ceriodical call interval (milliseconds). 
	///                          Force calling pfnDoWork every period of inactivity (without notification).
	///                          If nWakeupInterval is 0, periodical call is disabled.
	/// @return The function returns `true` if succeded.
	///
	bool initialize(PFnDoWork pfnDoWork, void* pContext, ULONG nWakeupInterval)
	{
		// +FIXME: Why do we do this dirty hack?
		// This type can be as in dynamic as in global.
		// We can't define constructor or any initialization in global variables
		// Normal realization require to use constructors and new/delete
		RtlZeroMemory(this, sizeof(*this));

		if (pfnDoWork == nullptr)
		{
			LOGERROR(STATUS_INVALID_PARAMETER, "SystemThreadWorker::initialize.\r\n");
			return false;
		}

		m_pfnDoWork = pfnDoWork;
		m_pContext = pContext;
		m_nWakeupInterval = nWakeupInterval;

		ExInitializeFastMutex(&m_Mutex);
		KeInitializeEvent(&m_Event, NotificationEvent, FALSE);

		// Create System Thread
		HANDLE threadHandle = nullptr;
		NTSTATUS ns = PsCreateSystemThread(&threadHandle, THREAD_ALL_ACCESS, NULL, NULL, 
			NULL, (PKSTART_ROUTINE) &run, (PVOID)this);
		if (!NT_SUCCESS(ns)) { IFERR_LOG(ns); return false; }
		ns = ObReferenceObjectByHandle(threadHandle, 0, NULL, KernelMode, &m_ThreadObj, NULL);
		ZwClose(threadHandle);  // Close handle. We just use object only.
		if (!NT_SUCCESS(ns)) { IFERR_LOG(ns); return false; }

		LOGINFO3("SystemThreadWorker::initialize. Success initialized.\r\n");

		m_fIsInitialized = true;
		return true;
	}

	///
	/// Set WakeupInterval.
	///
	/// @param nWakeupInterval - TBD.
	///
	void setWakeupInterval(ULONG nWakeupInterval)
	{
		ExAcquireFastMutex(&m_Mutex);
		m_nWakeupInterval = nWakeupInterval;
		ExReleaseFastMutex(&m_Mutex);
	}

	///
	/// Destructor.
	///
	void finalize()
	{
		// FIXME: Why we don't use mutex?
		if (!m_fIsInitialized) return;
		m_fIsInitialized = false;

		if (m_ThreadObj == nullptr) return;

		// Send stop and wake up
		m_fTerminated = true;
		KeSetEvent(&m_Event, 0, FALSE);

		LOGINFO3("SystemThreadWorker::finalize. Wait SystemThread.\r\n");
		KeWaitForSingleObject(m_ThreadObj, Executive, KernelMode, FALSE, nullptr);
		LOGINFO3("SystemThreadWorker::finalize. Free SystemThread.\r\n");
		ObDereferenceObject(m_ThreadObj);
		m_ThreadObj = nullptr;
	}

	///
	/// Notification about adding a new work.
	///
	/// If worker is sleep, it will wake up.
	/// If worker is busy, it will take a work on the next iteration.
	///
	void notify()
	{
		if (!m_fIsInitialized) return;
		if (m_fTerminated) return;

		KeSetEvent(&m_Event, 0, FALSE);
	}

	///
	/// Checking that processing was terminated.
	///
	bool isTerminated()
	{
		if (!m_fIsInitialized) return true;
		return m_fTerminated;
	}
};

} // namespace openEdr
/// @}
