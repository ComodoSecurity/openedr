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
#pragma once

#include "FltPort.h"
#include "../../libsysmon/inc/edrdrvapi.hpp"


namespace cmd {
namespace edrpm {

//
//
//
class Injection
{
private:	
	enum HandleType
	{
		Shutdown = 0,
		Capture = 1,
		Max
	};

	struct Event
	{
		uint64_t nTimeout;
	};

	struct Config
	{
		uint32_t nQueueSize = 10000;
		uint32_t nSendTimeout = 500;
		uint32_t nWaitTimeout = 1000;
		bool fWaitAnswer = false;
		uint32_t nAnswerSize = 0x1000;
		Event pEvents[unsigned(RawEvent::_Max)] = {
			{60000},	// PROCMON_PROCESS_MEMORY_READ = 0x0000,
			{1000},		// PROCMON_PROCESS_MEMORY_WRITE = 0x0001,
			{0},		// PROCMON_API_SET_WINDOWS_HOOK = 0x0002,
			{0},		// PROCMON_STORAGE_RAW_ACCESS_WRITE = 0x0003,
			{0},		// PROCMON_STORAGE_RAW_LINK_CREATE = 0x0004,
			{60000},	// PROCMON_API_GET_KEYBOARD_STATE = 0x0005,
			{60000},	// PROCMON_API_GET_KEY_STATE = 0x0006,
			{60000},	// PROCMON_API_REGISTER_HOT_KEY = 0x0007,
			{0},		// PROCMON_API_REGISTER_RAW_INPUT_DEVICES = 0x0008,
			{0},		// PROCMON_API_BLOCK_INPUT = 0x0009,
			{0},		// PROCMON_API_ENABLE_WINDOW = 0x000A,
			{0},		// PROCMON_API_GET_CLIPBOARD_DATA = 0x000B,
			{0},		// PROCMON_API_SET_CLIPBOARD_VIEWER = 0x000C,
			{0},		// PROCMON_API_SEND_INPUT = 0x000D,
			{0},		// PROCMON_API_KEYBD_EVENT = 0x000E,
			{0},		// PROCMON_API_ENUM_AUDIO_ENDPOINTS = 0x000F,
			{0},		// PROCMON_API_WAVE_IN_OPEN = 0x0010,
			{0},		// PROCMON_API_MOUSE_EVENT = 0x0011,
			{0},		// PROCMON_COPY_WINDOW_BITMAP = 0x0012,
			{0},		// PROCMON_DESKTOP_WALLPAPER_SET = 0x0013,
			{0},		// PROCMON_API_CLIP_CURSOR = 0x0014,
			{0},		// PROCMON_INTERACTIVE_LOGON = 0x0015,
			{60000},	// PROCMON_THREAD_IMPERSONATION = 0x0016,
			{0},		// PROCMON_PIPE_IMPERSONATION = 0x0017,
		};
		inline auto GetEventTimeout(RawEvent eventId)
		{
			auto id = unsigned(eventId) - ProcmonEventsOffset;
			return pEvents[id].nTimeout;
		}
	};

	CRITICAL_SECTION m_mtxQueue;
	std::deque<Buffer> m_vEventQueue;
	bool m_fSendQueueErr = true;
	size_t m_nQueueErr = 0;

	SRWLOCK m_srwConfig;
	Config m_Config;
	bool m_fUpdateConfig = true;

	CRITICAL_SECTION m_mtxEventTimeout;
	std::map<EventHash, Time> m_pEventTimes;

	CRITICAL_SECTION m_mtxThread;
	ScopedHandle m_hThread;
	std::atomic_bool m_fInit = false;
	std::atomic_bool m_fWait = false;
	std::atomic_bool m_fScheduleApc = true;

	ScopedHandle m_hThreadEvent;
	ScopedHandle m_hQueueEvent;
	ScopedHandle m_hTimeoutEvent;
	ScopedHandle m_hTerminationEvent;

	// update config retry attempts
	uint8_t m_MaxRetryCountAndWait = 10;
	uint8_t m_MaxRetryCount = 3;

	inline static const wchar_t c_sPortName[] = L"" CMD_EDRDRV_FLTPORT_PROCMON_IN_NAME;
	cmd::win::FltPort m_hFltPort;

	bool updateConfig();
	void workerThreadFunction();
	static DWORD WINAPI workerThread(void* data);

	void hookAll();
	void unhookAll();
	const uint8_t GetMaxRetryCount() const { return m_MaxRetryCount; };
	const uint8_t GetMaxRetryCountAndWait() const { return m_MaxRetryCountAndWait; };
public:
	Injection();
	virtual ~Injection();

	bool init();
	bool finalize();

	bool needToFilter(RawEvent eEvent);
	bool skipEvent(RawEvent eEvent, EventHash hash);
	
	bool isQueueFull();
	void freeQueue();

	void addEventToQueue(const Buffer& item);
	bool sendEventToSrv(const Buffer& item, Buffer& answer);
};


//////////////////////////////////////////////////////////////////////////
// Events handling

//
//
//
extern Injection g_Injection;

// PID of current process
static const uint32_t g_nSelfPid = ::GetCurrentProcessId();

template<typename Fn>
bool getEventData(RawEvent eEvent, Fn fnWriteData, Buffer& pData)
{
	try
	{
		variant::LbvsSerializer<EventField> serializer;
		if (serializer.write(EventField::RawEventId, uint32_t(eEvent)) &&
			//serializer.write(EventField::Time, getSystemTime()) &&
			serializer.write(EventField::TickTime, ::GetTickCount64()) &&
			serializer.write(EventField::ProcessPid, g_nSelfPid) &&
			fnWriteData(&serializer))
			return serializer.getData(pData);
	}
	catch (const std::exception & ex)
	{
		logError(std::string("Exception while serialize data of event <") + std::to_string(size_t(eEvent)) + ">, error <" + ex.what() + ">");
	}
	catch (...)
	{
		logError(std::string("Exception while serialize data of event <") + std::to_string(size_t(eEvent)) + ">");
	}
	return false;
}

//
//
//
template<typename Fn1, typename Fn2>
bool sendEvent(RawEvent eEvent, Fn1 fnWriteData, Fn2 fnHashData, Buffer& pAnswer)
{
	if (eEvent < RawEvent::_Max)
	{
		if (g_Injection.needToFilter(eEvent))
		{
			xxh::hash_state64_t hash;
			hash.update({ eEvent });
			fnHashData(&hash);
			if (g_Injection.skipEvent(eEvent, hash.digest()))
				return false;
		}

		if (g_Injection.isQueueFull())
			return false;
	}

	Buffer pData;
	if (!getEventData(eEvent, fnWriteData, pData))
	{
		logError(std::string("Fail to get data for event <") + std::to_string(uint32_t(eEvent)) + ">");
		return false;
	}
	bool result = true;
	if (eEvent < RawEvent::_Max)
	{
		dbgPrint(std::string("Add to queue event <") + std::to_string(uint32_t(eEvent)) + ">");
		g_Injection.addEventToQueue(pData);
	}
	else
	{
		result = g_Injection.sendEventToSrv(pData, pAnswer);
	}
	return result;
}

//
//
//
template<typename Fn1, typename Fn2>
void sendEvent(RawEvent eEvent, Fn1 fnWriteData, Fn2 fnHashData)
{
	Buffer pAnswer;
	(void)sendEvent(eEvent, fnWriteData, fnHashData, pAnswer);
}

//
//
//
template<typename Fn>
void sendEvent(RawEvent eEvent, Fn fnWriteData)
{
	Buffer pAnswer;
	(void)sendEvent(eEvent, fnWriteData, [](auto) {}, pAnswer);
}

//
//
//
inline void sendEvent(RawEvent eEvent)
{
	Buffer pAnswer;
	(void)sendEvent(eEvent, [](auto) -> bool { return true; }, [](auto) {}, pAnswer);
}

//
//
//
inline bool sendEvent(RawEvent eEvent, Buffer& pAnswer)
{
	return sendEvent(eEvent, [](auto) -> bool { return true; }, [](auto) {}, pAnswer);
}

} // namespace edrpm
} // namespace cmd

