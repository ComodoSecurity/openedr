//
// edrav2.libprocmon project
//
// Library classes registration
//
#include "pch.h"
#include "procmonevent.h"
#include "procmon.hpp"

namespace cmd {
namespace win {

	void ProcmonEvent::log(const Variant& vEvent)
	{
		uint32_t nPid = getByPath(vEvent, "process.pid", 0);
		std::string sModule = vEvent.get("path", "");
		std::string sMsg = std::string(vEvent["errMsg"]) + " (pid <" +
			std::to_string(nPid) + ">, name (" + sModule + ">)";
		edrpm::ErrorType eType(vEvent["errType"]);
		switch (eType)
		{
		case edrpm::ErrorType::Error:
			error::RuntimeError(SL, sMsg).log();
			break;
		case edrpm::ErrorType::Warning:
			LOGWRN(sMsg);
			break;
		case edrpm::ErrorType::Info:
			LOGLVL(Debug, sMsg);
			break;
		default:
			LOGWRN("Unknown message type. <" << sMsg << ">");
		}
		return;
	}

	Event g_pEventMap[] = {
	Event::LLE_PROCESS_MEMORY_READ,		// PROCMON_PROCESS_MEMORY_READ = 0x0000,
	Event::LLE_PROCESS_MEMORY_WRITE,	// PROCMON_PROCESS_MEMORY_WRITE = 0x0001,
	Event::LLE_NONE,					// PROCMON_API_SET_WINDOWS_HOOK = 0x0002,
	Event::LLE_NONE,					// PROCMON_STORAGE_RAW_ACCESS_WRITE = 0x0003,
	Event::LLE_NONE,					// PROCMON_STORAGE_RAW_LINK_CREATE = 0x0004,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_GET_KEYBOARD_STATE = 0x0005,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_GET_KEY_STATE = 0x0006,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_REGISTER_HOT_KEY = 0x0007,
	Event::LLE_KEYBOARD_GLOBAL_READ,	// PROCMON_API_REGISTER_RAW_INPUT_DEVICES = 0x0008,
	Event::LLE_KEYBOARD_BLOCK,			// PROCMON_API_BLOCK_INPUT = 0x0009,
	Event::LLE_KEYBOARD_BLOCK,			// PROCMON_API_ENABLE_WINDOW = 0x000A,
	Event::LLE_CLIPBOARD_READ,			// PROCMON_API_GET_CLIPBOARD_DATA = 0x000B,
	Event::LLE_CLIPBOARD_READ,			// PROCMON_API_SET_CLIPBOARD_VIEWER = 0x000C,
	Event::LLE_KEYBOARD_GLOBAL_WRITE,	// PROCMON_API_SEND_INPUT = 0x000D,
	Event::LLE_KEYBOARD_GLOBAL_WRITE,	// PROCMON_API_KEYBD_EVENT = 0x000E,
	Event::LLE_MICROPHONE_ENUM,			// PROCMON_API_ENUM_AUDIO_ENDPOINTS = 0x000F,
	Event::LLE_MICROPHONE_READ,			// PROCMON_API_WAVE_IN_OPEN = 0x0010,
	Event::LLE_MOUSE_GLOBAL_WRITE,		// PROCMON_API_MOUSE_EVENT = 0x0011,
	Event::LLE_WINDOW_DATA_READ,		// PROCMON_COPY_WINDOW_BITMAP = 0x0012,
	Event::LLE_DESKTOP_WALLPAPER_SET,	// PROCMON_DESKTOP_WALLPAPER_SET = 0x0013,
	Event::LLE_MOUSE_BLOCK,				// PROCMON_API_CLIP_CURSOR = 0x0014,
	Event::LLE_USER_LOGON,				// PROCMON_INTERACTIVE_LOGON = 0x0015,
	Event::LLE_USER_IMPERSONATION,		// PROCMON_THREAD_IMPERSONATION = 0x0016,
	Event::LLE_USER_IMPERSONATION,		// PROCMON_PIPE_IMPERSONATION = 0x0017,
	};

	static_assert(std::size(g_pEventMap) == Size((Size)(edrpm::RawEvent::_Max)-edrpm::ProcmonEventsOffset), "Not enough items in the table <g_pEventMap>");

	void ProcmonEvent::parseEvent(edrpm::RawEvent eEvent, Variant vEvent)
	{
		auto nEvent = uint32_t(eEvent);
		if (nEvent >= std::size(g_pEventMap))
		{
			LOGLVL(Trace, vEvent.print());
			error::InvalidArgument(SL, FMT("There is no mapping for event <" << nEvent << ">")).throwException();
		}

		vEvent.put("baseType", g_pEventMap[nEvent]);

		switch (eEvent)
		{
		case edrpm::RawEvent::PROCMON_API_SET_WINDOWS_HOOK:
		{
			std::string sHookType = vEvent["hookType"];
			if (sHookType == "WH_KEYBOARD" || sHookType == "WH_KEYBOARD_LL")
				vEvent.put("baseType", Event::LLE_KEYBOARD_GLOBAL_READ);
			else
				vEvent.put("baseType", Event::LLE_WINDOW_PROC_GLOBAL_HOOK);
			break;
		}
		case edrpm::RawEvent::PROCMON_STORAGE_RAW_ACCESS_WRITE:
		{
			uint32_t nObjectType = vEvent["objectType"];
			if (nObjectType == 0)
				vEvent.put("baseType", Event::LLE_DISK_RAW_WRITE_ACCESS);
			else if (nObjectType == 1)
				vEvent.put("baseType", Event::LLE_VOLUME_RAW_WRITE_ACCESS);
			else if (nObjectType == 2)
				vEvent.put("baseType", Event::LLE_DEVICE_RAW_WRITE_ACCESS);
			else
				error::InvalidFormat(SL, FMT("Fail to parse event <" << uint32_t(eEvent) << ">")).throwException();
			break;
		}
		case edrpm::RawEvent::PROCMON_STORAGE_RAW_LINK_CREATE:
		{
			uint32_t nObjectType = vEvent["objectType"];
			if (nObjectType == 0)
				vEvent.put("baseType", Event::LLE_DISK_LINK_CREATE);
			else if (nObjectType == 1)
				vEvent.put("baseType", Event::LLE_VOLUME_LINK_CREATE);
			else if (nObjectType == 2)
				vEvent.put("baseType", Event::LLE_DEVICE_LINK_CREATE);
			else
				error::InvalidFormat(SL, FMT("Fail to parse event <" << uint32_t(eEvent) << ">")).throwException();
			break;
		}
		case edrpm::RawEvent::PROCMON_THREAD_IMPERSONATION:
		{
			vEvent.put("impersonationType", sys::win::ImpersonationType::Local);
			break;
		}
		case edrpm::RawEvent::PROCMON_PIPE_IMPERSONATION:
		{
			vEvent.put("impersonationType", sys::win::ImpersonationType::Ipc);
			break;
		}
		}
	}

	void ProcmonEvent::updateConfig(bool& fHasAnswer)
	{
		if (pAnswerBuf == nullptr || dwAnswerLen < sizeof(variant::lbvs::LbvsHeader))
			error::InvalidArgument(SL, "Invalid answer buffer for IPC command").throwException();

		std::vector<uint8_t> pLbvs;
		if (!variant::serializeToLbvs(injectionConfig, configSchema, pLbvs))
			error::InvalidFormat(SL, "Can't serialize injection config to LBVS").throwException();

		memcpy(pAnswerBuf, pLbvs.data(), std::min<size_t>(dwAnswerLen, pLbvs.size()));
		fHasAnswer = true;
	}

	edrpm::RawEvent ProcmonEvent::adjustEventId(edrpm::RawEvent eventId)
	{
		return edrpm::RawEvent((uint32_t)eventId - edrpm::ProcmonEventsOffset);
	}

	bool ProcmonEvent::handle(bool& fHasAnswer)
	{
		Variant vEvent = variant::deserializeFromLbvs(pMessageBuf, dwMessageLen, eventSchema);
		edrpm::RawEvent nRawEventId = vEvent["rawEventId"];

		if (nRawEventId == edrpm::RawEvent::PROCMON_LOG_MESSAGE)
		{
#undef CMD_COMPONENT
#define CMD_COMPONENT "inject"
			log(vEvent);
			return true;
#undef CMD_COMPONENT
#define CMD_COMPONENT "procmon"
		}
		if (nRawEventId == edrpm::RawEvent::PROCMON_INJECTION_CONFIG_UPDATE)
		{
			updateConfig(fHasAnswer);
			return true;
		}

		// Drop events if service is stopped
		if (fServiceEnabled)
			return false;

		nRawEventId = adjustEventId(nRawEventId);

		LOGLVL(Trace, "Parse raw event <" << size_t(nRawEventId) <<
			"> from process <" << getByPath(vEvent, "process.pid", -1) << ">");

		vEvent.put("rawEventId", createRaw(nClassId, (uint32_t)nRawEventId));
		parseEvent(nRawEventId, vEvent);

		// Send message to receiver
		if (!receiver)
			error::InvalidArgument(SL, "Receiver interface is undefined").throwException();
		receiver->put(vEvent);
		return true;
	}

	ProcmonEvent::ProcmonEvent(const Byte* _pMessageBuf,
		unsigned long _dwMessageLen, Byte* _pAnswerBuf, unsigned long _dwAnswerLen,
		bool _fServiceEnabled, ObjPtr<IDataReceiver>& _receiver, const Variant& _eventSchema,
		const Variant& _injectionConfig, const Variant& _configSchema, ClassId _nClassId)
		: pMessageBuf(_pMessageBuf)
		, dwMessageLen(_dwMessageLen)
		, pAnswerBuf(_pAnswerBuf)
		, dwAnswerLen(_dwAnswerLen)
		, fServiceEnabled(_fServiceEnabled)
		, receiver(_receiver)
		, eventSchema(_eventSchema)
		, injectionConfig(_injectionConfig)
		, configSchema(_configSchema)
		, nClassId(_nClassId)
	{

	}

} // namespace win
} // namespace cmd 