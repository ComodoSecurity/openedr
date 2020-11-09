//
// edrav2.edrpm project
//
// Author: Denis Kroshin (30.04.2019)
// Reviewer: Denis Bogdanov (05.05.2019)
//
///
/// @file Injection library
///
/// @addtogroup edrpm
/// 
/// Injection library for process monitoring.
/// 
/// @{
#include "pch.h"
#include "injection.h"
#include "process.h"
#include "keyboard.h"
#include "disk.h"
#include "microphone.h"
#include "screen.h"

namespace openEdr {
namespace edrpm {

//
//
//
Injection::Injection()
{
	::InitializeCriticalSection(&m_mtxQueue);
	::InitializeCriticalSection(&m_mtxThread);
	::InitializeCriticalSection(&m_mtxEventTimeout);
	::InitializeSRWLock(&m_srwConfig);
	::InitializeCriticalSection(&g_mtxIMMDeviceEnumerator);

	m_hQueueEvent.reset(::CreateEvent(NULL, FALSE, FALSE, NULL));
	m_hTimeoutEvent.reset(::CreateEvent(NULL, FALSE, FALSE, NULL));
	m_hThreadEvent.reset(::CreateEvent(NULL, TRUE, FALSE, NULL));
	m_hTerminationEvent.reset(::CreateEvent(NULL, TRUE, FALSE, NULL));
}

//
//
//
Injection::~Injection()
{
	::DeleteCriticalSection(&m_mtxQueue);
	::DeleteCriticalSection(&m_mtxThread);
	::DeleteCriticalSection(&m_mtxEventTimeout);
	::DeleteCriticalSection(&g_mtxIMMDeviceEnumerator);
}

/*
A couple of issues worth mentioning:

1. Creating a thread during a DLL notification may not work exactly as
expected. Since DLL initialization routines are single-threaded, the new
thread's start routine will not begin execution until the current thread
"unwinds" out of the DLL notification. (Think about it this way. If you
create a worker thread in a DLL_PROCESS_ATTACH notification, the worker
thread cannot issue the DLL_THREAD_ATTACH while the current thread is still
processing the process attach.)

2. Likewise, if you request a worker thread to kill itself (say, by
signaling an event) from within a DLL notification, the thread will not
become "fully terminated" (i.e. the thread handle will not become signaled)
until the current thread "unwinds" out of the DLL notification.

Here's one way to use worker threads from within a DLL:

1. Before you create the worker thread (this could be during
DLL_PROCESS_ATTACH, but it's better do defer this until it's actually
needed), use GetModuleFileName() and LoadLibrary() to add an "artificial"
reference to the DLL. This will keep the DLL from disappearing from
"beneath" the worker thread. You will need to either pass the DLL's module
handle to the worker thread, or store it away in a global.

2. When it's time to kill the worker thread, signal an event (or use
whatever mechanism you need to tell the worker thread to kill itself). If
you're doing this from within a DLL attach/detach notification, then *do
not* wait for the thread to die, or you will deadlock.

3. The last thing the worker thread should do is call
FreeLibraryAndExitThread(). This will remove the artificial DLL reference
and terminate the worker thread. Since the thread will terminate during this
call, it will not have a chance to return back to the (possibly freed) DLL.
*/

//
//
//
bool Injection::init()
{
	dbgPrint("Start injection");
	// Update config 
	//m_fUpdateConfig = !updateConfig(true);

/*
// 	getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtReadVirtualMemory"));
	getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtWriteVirtualMemory"));
	getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtCreateFile"));
	getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtCreateSymbolicLinkObject"));
//	getTrampManager().disableTrampStart(getTrampId("kernel32.dll", "ExitProcess"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "SetWindowsHookExA"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "SetWindowsHookExW"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetKeyboardState"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetKeyState"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetAsyncKeyState"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "RegisterHotKey"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "RegisterRawInputDevices"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "BlockInput"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "EnableWindow"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetClipboardData"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "SetClipboardViewer"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "SendInput"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "keybd_event"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "SystemParametersInfoA"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "SystemParametersInfoW"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "ClipCursor"));
	getTrampManager().disableTrampStart(getTrampId("user32.dll", "mouse_event"));
	getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "BitBlt"));
	getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "MaskBlt"));
	getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "PlgBlt"));
	getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "StretchBlt"));
	getTrampManager().disableTrampStart(getTrampId("ole32.dll", "CoCreateInstance"));
	getTrampManager().disableTrampStart(getTrampId("combase.dll", "CoCreateInstance"));
	getTrampManager().disableTrampStart(getTrampId("winmm.dll", "waveInOpen"));
*/

	hookAll();
	//logError("Injected into process", ErrorType::Info);
	m_fInit = true;
	return true;
}

//
//
//
bool Injection::finalize()
{
	// Do not change order
	getTrampManager().disableTramps();
	::SetEvent(m_hTerminationEvent);
	::SetEvent(m_hQueueEvent);
	::SetEvent(m_hTimeoutEvent);

	::EnterCriticalSection(&m_mtxQueue);
	if (m_vEventQueue.size())
		dbgPrint(std::string("Queue is not empty. ") + std::to_string(m_vEventQueue.size()) + std::string(" events left."));
	m_vEventQueue.clear();
	::LeaveCriticalSection(&m_mtxQueue);

	if (m_hThread)
	{
		// Wait thread to exit
		HANDLE pHandles[] = { m_hThread, m_hThreadEvent };
		auto nStatus = WaitForMultipleObjects(DWORD(std::size(pHandles)), pHandles, FALSE, INFINITE);
		// EDR-2405: Dirty hack around thread termination
		if (nStatus == WAIT_OBJECT_0 + 1)
		{
			while (!m_fWait);
#pragma warning(suppress: 6258)
			::TerminateThread(m_hThread, 2);
		}
	}
	unhookAll();
	dbgPrint("Finish injection");
	return true;
}

//
//
//
bool Injection::updateConfig()
{
	bool fStatus = false;
	TRY
	{
		static const size_t nBufferSize = 0x1000;
		Buffer pBuffer;
		variant::LbvsDeserializer<ConfigField> deserializer;
		do
		{
			pBuffer.resize(pBuffer.size() + nBufferSize);
			if (!sendEvent(RawEvent::PROCMON_INJECTION_CONFIG_UPDATE, pBuffer))
			{
				logError("Fail to receive config from service");
				return false;
			}
		} while (!deserializer.reset(pBuffer.data(), pBuffer.size()));

		Config config;
		std::vector<std::pair<size_t, uint64_t>> vEvents;
		for (auto& item : deserializer)
		{
			switch (item.id)
			{
			case ConfigField::QueueSize:
				if (!item.getValue(config.nQueueSize))
					logError("Invalid config field 'queueSize'");
				break;
			case ConfigField::SendTimeout:
				if (!item.getValue(config.nSendTimeout))
					logError("Invalid config field 'sendTimeout'");
				break;
			case ConfigField::WaitTimeout:
				if (!item.getValue(config.nWaitTimeout))
					logError("Invalid config field 'waitTimeout'");
				break;
			case ConfigField::WaitAnswer:
				if (!item.getValue(config.fWaitAnswer))
					logError("Invalid config field 'waitAnswer'");
				break;
			case ConfigField::AnswerSize:
				if (!item.getValue(config.nAnswerSize))
					logError("Invalid config field 'answerSize'");
				break;
			case ConfigField::Events:
				vEvents.resize(vEvents.size() + 1);
				break;
			case ConfigField::EventId:
				if (!item.getValue(vEvents.back().first))
					logError("Invalid config field 'events[].id'");
				break;
			case ConfigField::EventTimeout:
				if (!item.getValue(vEvents.back().second))
					logError("Invalid config field 'events[].timeout'");
				break;
			default:
				break;
			}
		}

		for (auto event : vEvents)
			config.pEvents[event.first].nTimeout = event.second;

		::AcquireSRWLockExclusive(&m_srwConfig);
		m_Config = config;
		::ReleaseSRWLockExclusive(&m_srwConfig);
		fStatus = true;
		dbgPrint("Configuration updated");
	}
	CATCH;
	return fStatus;
}

//
//
//
void Injection::workerThreadFunction()
{
	dbgPrint("Worker thread started");
	
	// Wait for global event
	ScopedHandle hCaptureEvent;
	for (;;)
	{
		hCaptureEvent.reset(OpenGlobalEvent(c_sGlobalCaptureEvent));
		if (hCaptureEvent)
			break;
		if (::WaitForSingleObject(m_hTerminationEvent, m_Config.nWaitTimeout) == WAIT_OBJECT_0)
			return;
	}

	HANDLE pHandles[] = {m_hTerminationEvent, hCaptureEvent};
	std::vector<uint8_t> pAnswer(m_Config.fWaitAnswer ? m_Config.nAnswerSize : 0);

	while (hCaptureEvent)
	{
		// Always update config after service restart
		if (::WaitForSingleObject(hCaptureEvent, 0) != WAIT_OBJECT_0)
			m_fUpdateConfig = true;

		// Wait for signaled global event
		auto nStatus = ::WaitForMultipleObjects(DWORD(std::size(pHandles)), pHandles, FALSE, INFINITE);
		if (nStatus != WAIT_OBJECT_0 + HandleType::Capture)
			break; // Process termination

		// NOTE: We do not need to sync Config access in this thread;
		if (m_fUpdateConfig)
		{
			if (!updateConfig())
			{
				::WaitForSingleObject(m_hTimeoutEvent, m_Config.nWaitTimeout);
				continue;
			}

			pAnswer.resize(m_Config.fWaitAnswer ? m_Config.nAnswerSize : 0);
			m_fUpdateConfig = false;
		}

		// Wait for event
		::WaitForSingleObject(m_hQueueEvent, m_Config.nWaitTimeout);

		// Recheck service status after sleep
		nStatus = ::WaitForMultipleObjects(DWORD(std::size(pHandles)), pHandles, FALSE, 0);
		if (nStatus != WAIT_OBJECT_0 + HandleType::Capture)
			continue; // Service stopped while we wait event or possible termination

		// Get event from queue
		::EnterCriticalSection(&m_mtxQueue);
		if (m_vEventQueue.empty())
		{
			::LeaveCriticalSection(&m_mtxQueue);
			continue;
		}
		auto pBuffer = m_vEventQueue.front();
		::LeaveCriticalSection(&m_mtxQueue);

		// Send data to service
		if (pBuffer.empty() || sendEventToSrv(pBuffer, pAnswer))
		{
			// Remove event from queue
			::EnterCriticalSection(&m_mtxQueue);
			m_vEventQueue.pop_front();
			::LeaveCriticalSection(&m_mtxQueue);
		}
		else
		{
			m_fUpdateConfig = true;
			logError("Fail to send event. Will retry.", ErrorType::Warning);
			::WaitForSingleObject(m_hTimeoutEvent, m_Config.nWaitTimeout);
		}

		// Set auto-reset event to check next item in queue
		::SetEvent(m_hQueueEvent);
	}
}

//
//
//
DWORD WINAPI Injection::workerThread(void* data)
{
	Injection* pThis = (Injection*)data;
	if (pThis == nullptr)
		return 0;
	pThis->workerThreadFunction();
	dbgPrint("Exit worker thread");
	::SetEvent(pThis->m_hThreadEvent);
	// Wait for thread termination
	pThis->m_fWait = true;
	while (pThis->m_fWait);
	::ExitThread(0);
}

//
//
//
bool Injection::needToFilter(RawEvent eEvent)
{
	if (eEvent >= RawEvent::_Max)
		return false;
	::AcquireSRWLockShared(&m_srwConfig);
	bool fFilter = (m_Config.pEvents[unsigned(eEvent)].nTimeout > 0);
	::ReleaseSRWLockShared(&m_srwConfig);
	return fFilter;
}

//
//
//
bool Injection::skipEvent(RawEvent eEvent, EventHash nEventHash)
{
	if (eEvent >= RawEvent::_Max)
		return false;
	::AcquireSRWLockShared(&m_srwConfig);
	auto nTimeout = m_Config.pEvents[unsigned(eEvent)].nTimeout;
	::ReleaseSRWLockShared(&m_srwConfig);
	if (nTimeout == 0)
		return false;

	::EnterCriticalSection(&m_mtxEventTimeout);
	Time nCurTime = ::GetTickCount64();
	if (nCurTime - m_pEventTimes[nEventHash] < nTimeout)
	{
		::LeaveCriticalSection(&m_mtxEventTimeout);
		return true;
	}
	m_pEventTimes[nEventHash] = nCurTime;
	::LeaveCriticalSection(&m_mtxEventTimeout);
	return false;
}

//
//
//
bool Injection::isQueueFull()
{
	::AcquireSRWLockShared(&m_srwConfig);
	size_t nQueueSize = m_Config.nQueueSize;
	::ReleaseSRWLockShared(&m_srwConfig);

	::EnterCriticalSection(&m_mtxQueue);
	if (m_vEventQueue.size() >= nQueueSize)
	{
		if (m_fSendQueueErr)
		{
			logError("Queue is full");
			m_fSendQueueErr = false;
		}
		else if ((++m_nQueueErr & 0xFFF) == 0)
			m_fSendQueueErr = true;
		::LeaveCriticalSection(&m_mtxQueue);
		return true;
	}

	if (!m_fSendQueueErr && m_vEventQueue.size() < nQueueSize / 4 * 3)
		m_fSendQueueErr = true;
	::LeaveCriticalSection(&m_mtxQueue);
	return false;
}

//
//
//
void Injection::freeQueue()
{
	getTrampManager().disableTramps();
	::SetEvent(m_hTerminationEvent);
	::SetEvent(m_hQueueEvent);
	::SetEvent(m_hTimeoutEvent);

	::EnterCriticalSection(&m_mtxQueue);
	if (m_vEventQueue.size())
		dbgPrint(std::string("Queue is not empty at process termination. ") +
			std::to_string(m_vEventQueue.size()) + std::string(" events left."));
	while (!m_vEventQueue.empty())
	{
		auto& pBuffer = m_vEventQueue.front();
		if (!SendIpcMessage(c_sIpcEventsPort, pBuffer.data(), DWORD(pBuffer.size()), NULL, 0, m_Config.nSendTimeout))
		{
			dbgPrint("Fail to send event at ExitProcess.");
			break;
		}
		m_vEventQueue.pop_front();
	}
	::LeaveCriticalSection(&m_mtxQueue);
}

//
//
//
void Injection::addEventToQueue(const Buffer& item)
{
	::EnterCriticalSection(&m_mtxThread);
	if (!m_hThread && m_fInit &&
		// Do not create treads on termination
		::WaitForSingleObject(m_hTerminationEvent, 0) != WAIT_OBJECT_0)
	{
		m_hThread.reset(::CreateThread(NULL, 0, workerThread, this, 0, NULL));
		if (!m_hThread)
		{
			getTrampManager().disableTramps();
			logError("Fail to create events parsing thread.");
		}
	}
	::LeaveCriticalSection(&m_mtxThread);

	::EnterCriticalSection(&m_mtxQueue);
	m_vEventQueue.push_back(item);
	::SetEvent(m_hQueueEvent);
	::LeaveCriticalSection(&m_mtxQueue);
}

//
//
//
bool Injection::sendEventToSrv(const Buffer& item, Buffer& answer)
{
	return SendIpcMessage(c_sIpcEventsPort, (void*)item.data(), DWORD(item.size()),
		answer.data(), DWORD(answer.size()), m_Config.nSendTimeout);
}

//
//
//
void Injection::hookAll()
{
	CollectHooks();
	getTrampManager().startTramps();
	FlushHooks();
}

//
//
//
void Injection::unhookAll()
{
	getTrampManager().stopTramps();
}

} // namespace edrpm
} // namespace openEdr

