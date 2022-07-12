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

namespace cmd {
namespace edrpm {

inline constexpr uint64_t c_nDefaultEventsFlags = 0xffffffffffffffffui64;

#if defined(FEATURE_ENABLE_MADCHOOK)
static const char c_sIpcEventsPort[] = "edrIpcEventsPort";
static const char c_sIpcErrorsPort[] = "edrIpcErrorsPort";
static const char c_sGlobalCaptureEvent[] = "edrCaptureEvent";
inline constexpr uint32_t ProcmonEventsOffset = 0;
#else
inline constexpr uint32_t ProcmonEventsOffset = 0;
static const wchar_t c_sGlobalCaptureEvent[] = L"Global\\edrCaptureEvent";
#endif // FEATURE_ENABLE_MADCHOOK


///
/// The enum class of ProcMon raw event identifiers.
///
enum class RawEvent : uint32_t
{
	PROCMON_PROCESS_MEMORY_READ = ProcmonEventsOffset,
	PROCMON_PROCESS_MEMORY_WRITE,
	PROCMON_API_SET_WINDOWS_HOOK,
	PROCMON_STORAGE_RAW_ACCESS_WRITE,
	PROCMON_STORAGE_RAW_LINK_CREATE,
	PROCMON_API_GET_KEYBOARD_STATE,
	PROCMON_API_GET_KEY_STATE,
	PROCMON_API_REGISTER_HOT_KEY,
	PROCMON_API_REGISTER_RAW_INPUT_DEVICES,
	PROCMON_API_BLOCK_INPUT,
	PROCMON_API_ENABLE_WINDOW,
	PROCMON_API_GET_CLIPBOARD_DATA,
	PROCMON_API_SET_CLIPBOARD_VIEWER,
	PROCMON_API_SEND_INPUT,
	PROCMON_API_KEYBD_EVENT,
	PROCMON_API_ENUM_AUDIO_ENDPOINTS,
	PROCMON_API_WAVE_IN_OPEN,
	PROCMON_API_MOUSE_EVENT,
	PROCMON_COPY_WINDOW_BITMAP,
	PROCMON_DESKTOP_WALLPAPER_SET,
	PROCMON_API_CLIP_CURSOR,
	PROCMON_INTERACTIVE_LOGON,
	PROCMON_THREAD_IMPERSONATION,
	PROCMON_PIPE_IMPERSONATION,

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
} // namespace cmd

/// @}
