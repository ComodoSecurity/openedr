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
#include <libsysmon\inc\edrdrvapi.hpp>
#include <fltuser.h>

#pragma comment (lib, "fltLib.lib")

namespace cmd {
namespace edrpm {

//
//
//
Injection::Injection()
	: m_hFltPort(c_sPortName)
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
	if (m_fInit.exchange(true))
		return true;

	//dbgPrint("Start injection");
	// Update config 
	//m_fUpdateConfig = !updateConfig(true);

 //	getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtReadVirtualMemory"));
	//getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtWriteVirtualMemory"));
	//getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtCreateFile"));
	//getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtCreateSymbolicLinkObject"));
	//getTrampManager().disableTrampStart(getTrampId("kernel32.dll", "ExitProcess"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "SetWindowsHookExA"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "SetWindowsHookExW"));
	//
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetKeyboardState"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetKeyState"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetAsyncKeyState"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "RegisterHotKey"));
	//// мешает здесь:
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "RegisterRawInputDevices"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "BlockInput"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "EnableWindow"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "GetClipboardData"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "SetClipboardViewer"));
	//// end
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "SendInput"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "keybd_event"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "SystemParametersInfoA"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "SystemParametersInfoW"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "ClipCursor"));
	//getTrampManager().disableTrampStart(getTrampId("user32.dll", "mouse_event"));
	//getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "BitBlt"));
	//getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "MaskBlt"));
	//getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "PlgBlt"));
	//getTrampManager().disableTrampStart(getTrampId("gdi32.dll", "StretchBlt"));
	//getTrampManager().disableTrampStart(getTrampId("ole32.dll", "CoCreateInstance"));
	//getTrampManager().disableTrampStart(getTrampId("combase.dll", "CoCreateInstance"));
	//getTrampManager().disableTrampStart(getTrampId("winmm.dll", "waveInOpen"));

	//getTrampManager().disableTrampStart(getTrampId("advapi32.dll", "ImpersonateNamedPipeClient"));
	//getTrampManager().disableTrampStart(getTrampId("advapi32.dll", "CreateProcessWithLogonW"));
	//getTrampManager().disableTrampStart(getTrampId("sspicli.dll", "LsaLogonUser"));
	//getTrampManager().disableTrampStart(getTrampId("ntdll.dll", "NtSetInformationThread"));

	hookAll();
	logError("Injected into process", ErrorType::Info);
	return true;
}

//
//
//
bool Injection::finalize()
{
	if (!m_fInit)
		return false;

	// Do not change order
	getTrampManager().disableTramps();
	::SetEvent(m_hTerminationEvent);
	::SetEvent(m_hQueueEvent);
	::SetEvent(m_hTimeoutEvent);

	::EnterCriticalSection(&m_mtxQueue);
	if (m_vEventQueue.size() > 0)
	{
		dbgPrint(std::string("Queue is not empty. ") + std::to_string(m_vEventQueue.size()) + std::string(" events left."));
		m_vEventQueue.clear();
	}
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
		constexpr size_t bufferSize = 0x2000;

		Buffer buffer;
		uint8_t retryCount = 0;
		bool result = false;
		
		do
		{
			buffer.resize(buffer.size() + bufferSize);
			result = sendEvent(RawEvent::PROCMON_INJECTION_CONFIG_UPDATE, buffer);
			if (result)
				break;
			
			if (++retryCount >= GetMaxRetryCount())
			{
				logError(std::string("Fail to receive config from service, retryCount=<") + std::to_string(retryCount) + ">");
				return false;
			}
		}
		while (ERROR_MORE_DATA == GetLastError());

		variant::LbvsDeserializer<ConfigField> deserializer;
		if (!result || !deserializer.reset(buffer.data(), buffer.size()))
		{
			logError(std::string("Fail to receive config from service, sendEvent failed, retryCount=<") + std::to_string(retryCount) + ">");
			return false;
		}

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
		{
			auto id = event.first;
			if( id < unsigned(RawEvent::_Max))
				config.pEvents[id].nTimeout = event.second;
		}

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
#if defined(FEATURE_ENABLE_MADCHOOK)
		hCaptureEvent.reset(OpenGlobalEvent(c_sGlobalCaptureEvent));
#else
		hCaptureEvent.reset(::OpenEvent(EVENT_ALL_ACCESS, FALSE, c_sGlobalCaptureEvent));
#endif // FEATURE_ENABLE_MADCHOOK
		if (hCaptureEvent)
			break;
		if (::WaitForSingleObject(m_hTerminationEvent, m_Config.nWaitTimeout) == WAIT_OBJECT_0)
			return;
	}

	HANDLE pHandles[] = {m_hTerminationEvent, hCaptureEvent};
	std::vector<uint8_t> pAnswer(m_Config.fWaitAnswer ? m_Config.nAnswerSize : 0);
	uint8_t updateConfigRetryCount = 0;

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
				if (++updateConfigRetryCount > GetMaxRetryCountAndWait())
				{
					getTrampManager().disableTramps();
					::SetEvent(m_hThreadEvent);
					::ExitThread(0);
					return;
				}

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
	bool fFilter = (m_Config.GetEventTimeout(eEvent) > 0);
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
	auto nTimeout = m_Config.GetEventTimeout(eEvent);
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
#if defined(FEATURE_ENABLE_MADCHOOK)
		if (!SendIpcMessage(c_sIpcEventsPort, pBuffer.data(), DWORD(pBuffer.size()), NULL, 0, m_Config.nSendTimeout))
		{
			dbgPrint("Fail to send event at ExitProcess.");
			break;
		}
#else
		Buffer pAnswer;
		if (!sendEventToSrv(pBuffer, pAnswer))
		{
			dbgPrint("Fail to send event at ExitProcess.");
			break;
		}

#endif // FEATURE_ENABLE_MADCHOOK
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
#if defined(FEATURE_ENABLE_MADCHOOK)
	return SendIpcMessage(c_sIpcEventsPort, (void*)item.data(), DWORD(item.size()),
		answer.data(), DWORD(answer.size()), m_Config.nSendTimeout);
#else
	auto hr = m_hFltPort.Send(item, answer);
	if (FAILED(hr))
	{
		SetLastError(HRESULT_CODE(hr));
		dbgPrint("Sending failed:" + std::to_string(hr));
	}
	return SUCCEEDED(hr);
#endif
}

//
//
//
void Injection::hookAll()
{
#if defined(FEATURE_ENABLE_MADCHOOK)
	CollectHooks();
	getTrampManager().startTramps();
	FlushHooks();
#else
	detours::CollectAllHooks();
	getTrampManager().startTramps();
	detours::CommitAllHooks();
#endif
}

//
//
//
void Injection::unhookAll()
{
#if defined(FEATURE_ENABLE_MADCHOOK)
	getTrampManager().stopTramps();
#else
	detours::CollectAllHooks();
	getTrampManager().stopTramps();
	detours::CommitAllHooks();
#endif
}

} // namespace edrpm
} // namespace cmd

