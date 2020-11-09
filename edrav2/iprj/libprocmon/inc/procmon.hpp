//
// edrav2.libcore project
// 
// Author: Denis Kroshin (20.01.2019)
// Reviewer: Denis Bogdanov (13.05.2019)
//
///
/// @file Process Monitor Controller declaration
///
/// @addtogroup procmon Process Monitor library
/// @{
#pragma once

namespace openEdr {
namespace edrpm {

inline constexpr uint64_t c_nDefaultEventsFlags = 0xffffffffffffffffui64;

static const char c_sIpcEventsPort[] = "edrIpcEventsPort";
static const char c_sIpcErrorsPort[] = "edrIpcErrorsPort";

static const char c_sGlobalCaptureEvent[] = "edrCaptureEvent";

///
/// The enum class of ProcMon raw event identifiers.
///
enum class RawEvent : uint32_t
{
	PROCMON_PROCESS_MEMORY_READ = 0x0000,
	PROCMON_PROCESS_MEMORY_WRITE = 0x0001,
	PROCMON_API_SET_WINDOWS_HOOK = 0x0002,
	PROCMON_STORAGE_RAW_ACCESS_WRITE = 0x0003,
	PROCMON_STORAGE_RAW_LINK_CREATE = 0x0004,
	PROCMON_API_GET_KEYBOARD_STATE = 0x0005,
	PROCMON_API_GET_KEY_STATE = 0x0006,
	PROCMON_API_REGISTER_HOT_KEY = 0x0007,
	PROCMON_API_REGISTER_RAW_INPUT_DEVICES = 0x0008,
	PROCMON_API_BLOCK_INPUT = 0x0009,
	PROCMON_API_ENABLE_WINDOW = 0x000A,
	PROCMON_API_GET_CLIPBOARD_DATA = 0x000B,
	PROCMON_API_SET_CLIPBOARD_VIEWER = 0x000C,
	PROCMON_API_SEND_INPUT = 0x000D,
	PROCMON_API_KEYBD_EVENT = 0x000E,
	PROCMON_API_ENUM_AUDIO_ENDPOINTS = 0x000F,
	PROCMON_API_WAVE_IN_OPEN = 0x0010,
	PROCMON_API_MOUSE_EVENT = 0x0011,
	PROCMON_COPY_WINDOW_BITMAP = 0x0012,
	PROCMON_DESKTOP_WALLPAPER_SET = 0x0013,
	PROCMON_API_CLIP_CURSOR = 0x0014,
	PROCMON_INTERACTIVE_LOGON = 0x0015,
	PROCMON_THREAD_IMPERSONATION = 0x0016,
	PROCMON_PIPE_IMPERSONATION = 0x0017,

	_Max, //< "last" event. for internal usage
	PROCMON_LOG_MESSAGE = 0xFFFE,
	PROCMON_INJECTION_CONFIG_UPDATE = 0xFFFF
};

///
/// The enum class of ProcMon raw event fields (lbvs protocol).
///
enum class EventField : variant::lbvs::FieldId
{
	RawEventId = 0, ///< int
	//Time = 1, ///< time - DEPRECATED
	TickTime = 2, ///< time
	ProcessPid = 3, ///< int
	TargetPid = 4, ///< int
	FilePath = 5, ///< str
	HookType = 6, ///< str
	ModulePath = 7, ///< str
	ObjectType = 8, ///< int
	Path = 9, ///< str
	Link = 10, ///< str
	UserName = 11,
	UserSid = 12,
	UserIsElevated = 13,
	ThreadId = 14,
	ErrorType = 15,
	ErrorMsg = 16
};

///
/// ProcMon raw event schema (lbvs protocol).
///
inline constexpr char c_sEventSchema[] = R"json({
"version": 1,
"fields": [
	{ "name": "rawEventId" },
	{ "name": "time" },
	{ "name": "tickTime" },
	{ "name": "process.pid" },
	{ "name": "target.pid" },
	{ "name": "file.rawPath" },
	{ "name": "hookType" },
	{ "name": "module.rawPath" },
	{ "name": "objectType" },
	{ "name": "path" },
	{ "name": "link" },
	{ "name": "user.name" },
	{ "name": "user.sid" },
	{ "name": "user.isElevated" },
	{ "name": "thread.tid" },
	{ "name": "errType" },
	{ "name": "errMsg" }
]})json";

//
//
//
enum class ErrorType {
	Error = 0,
	Warning = 1,
	Info = 2,
};


///
/// The enum class pf ProcMon config fields (lbvs protocol).
///
enum class ConfigField : variant::lbvs::FieldId
{
	QueueSize = 0, ///< int
	SendTimeout = 1, ///< time
	WaitTimeout = 2, ///< time
	WaitAnswer = 3, ///< bool
	AnswerSize = 4, ///< int
	Events = 5, ///< seq
	EventId = 6, ///< int
	EventTimeout = 7, ///< time
};

///
/// ProcMon config schema (lbvs protocol)./
///
inline constexpr char c_sConfigSchema[] = R"json({
"version": 1,
"fields": [
	{ "name": "queueSize" },
	{ "name": "sendTimeout" },
	{ "name": "waitTimeout" },
	{ "name": "waitAnswer" },
	{ "name": "answerSize" },
	{ "name": "events[]" },
	{ "name": "events[].id" },
	{ "name": "events[].timeout" }
]})json";

} // namespace edrpm
} // namespace openEdr

/// @}
