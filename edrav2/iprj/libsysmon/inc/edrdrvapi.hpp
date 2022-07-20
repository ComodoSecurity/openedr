//
//  EDRAv2.libsysmon
//  Communication protocol with erddrv.sys
//
///
/// @file Communication Port
///
/// @addtogroup sysmon System Monitor library
/// @{
#pragma once

// FIXME: Wrong file headers and documentation links!!!
#include <libcore\inc\variant\lbvs.hpp>

namespace cmd {
namespace edrdrv {

constexpr uint64_t c_nDefaultEventsFlags = 0xffffffffffffffffui64;


#ifdef _DEBUG
constexpr bool c_fDefaultDisableSelfProtection = true;
constexpr uint32_t c_nDefaultLogLevel = 3;
#else // _DEBUG
constexpr bool c_fDefaultDisableSelfProtection = false;
constexpr uint32_t c_nDefaultLogLevel = 0;
#endif

constexpr bool c_nReplyMode = false;

///
/// The enum class of SysMon raw event identifiers.
///
enum class SysmonEvent : uint16_t
{
	ProcessCreate = 0x0000,
	ProcessDelete = 0x0001,
	RegistryKeyNameChange = 0x0002,
	RegistryKeyCreate = 0x0003,
	RegistryKeyDelete = 0x0004,
	RegistryValueSet = 0x0005,
	RegistryValueDelete = 0x0006,
	FileCreate = 0x0007,
	FileDelete = 0x0008,
	FileClose = 0x0009,
	FileDataChange = 0x000A,
	FileDataReadFull = 0x000B,
	FileDataWriteFull = 0x000C,
	ProcessOpen = 0x000D,

	_Max //< "last" event. for internal usage
};

static_assert(((size_t)SysmonEvent::_Max) < (sizeof(uint64_t) * 8) + 1, "Too many event in SysmonEvent. Maximum 64 is allowed.");


///
/// Filter port name.
///
#define CMD_EDRDRV_FLTPORT_NAME "\\{A6F9548E-BE5E-4BE6-A632-18AC626532FE}"


///
/// Driver service name.
///
#define CMD_EDRDRV_SERVICE_NAME "edrdrv"

///
/// PCA service name.
///
#define CMD_PCA_SERVICE_NAME "PcaSvc"

///
/// Driver filename name.
///
#define CMD_EDRDRV_FILE_NAME "edrdrv.sys"

///
/// Special value name for disable driver self-protection (allow driver unload).
/// It is needed to delete this value name from any registry key, to disable self-protection.
///
#define CMD_EDRDRV_DISABLE_SP_VALUE_NAME "{D0FF0352-3B0D-4040-A928_038A41AEDB1B}"


///
/// Procmon proxy port names.
///
#define CMD_EDRDRV_FLTPORT_PROCMON_IN_NAME	L"\\{362D99EB-5419-463B-B97E-845985416904}"
#define CMD_EDRDRV_FLTPORT_PROCMON_OUT_NAME L"\\{CCBB4A34-46D2-4875-8D23-8ED3F033151B}"

///
/// The enum class of edrdrv.sys raw event fields (lbvs protocol).
///
enum class EventField : variant::lbvs::FieldId
{
	RawEventId = 0, ///< int
	TickTime = 1, ///< time
	ProcessPid = 2, ///< int
	ProcessParentPid = 3, ///< int
	ProcessCmdLine = 4, ///< str
	ProcessIsElevated = 5, ///< bool
	ProcessElevationType = 6, ///< int, TOKEN_ELEVATION_TYPE
	ProcessImageFile = 7, ///< str
	ProcessUserSid = 8, ///< str
	ProcessExitCode = 9, ///< int
	//Time = 10, ///< time - DEPRECATED
	RegistryPath = 11, ///< str
	RegistryKeyNewName = 12, ///< str
	RegistryName = 13, ///< str
	RegistryRawData = 14, ///< stream
	RegistryDataType = 15, ///< int
	FilePath = 16, ///< str
	FileVolumeGuid = 17, ///< str
	FileVolumeType = 18, ///< str
	FileVolumeDevice = 19, ///< str
	FileRawHash = 20, ///< str
	ProcessCreationTime = 21, ///< time
	ProcessDeletionTime = 22, ///< time
	ProcessCreatorPid = 23, ///< int. It is sent if creator is not the same as parent (e.g. elevated processes)
	TargetProcessPid = 24, ///< int
	AccessMask = 25, ///< int
};

///
/// edrdrv.sys raw event schema (lbvs protocol).
///
constexpr char c_sEventSchema[] = R"json({
"version": 1,
"fields": [
	{ "name": "rawEventId" },
	{ "name": "tickTime" },
	{ "name": "process.pid" },
	{ "name": "process.parent.pid" },
	{ "name": "process.cmdLine" },
	{ "name": "process.isElevated" },
	{ "name": "process.elevationType" },
	{ "name": "process.imageFile.rawPath" },
	{ "name": "process.userSid" },
	{ "name": "process.exitCode" },
	{ "name": "time" },
	{ "name": "registry.rawPath" },
	{ "name": "registry.keyNewName" },
	{ "name": "registry.name" },
	{ "name": "registry.data" },
	{ "name": "registry.rawType" },
	{ "name": "file.rawPath" },
	{ "name": "file.volume.guid" },
	{ "name": "file.volume.type" },
	{ "name": "file.volume.device" },
	{ "name": "file.rawHash" },
	{ "name": "process.creationTime" },
	{ "name": "process.deletionTime" },
	{ "name": "process.creatorPid" },
	{ "name": "target.pid" },
	{ "name": "accessMask" }
]})json";

//////////////////////////////////////////////////////////////////////////
//
// Config
//

///
/// The enum class of edrdrv.sys config fields (lbvs protocol).
///
enum class ConfigField : variant::lbvs::FieldId
{
	DisableSelfProtection = 0, //< bool
	MaxQueueSize = 1, //< int
	ConnectionTimeot = 2, //< int
	SendMsgTimeout = 3, //< int
	EnableDllInject = 4, //< bool
	LogLevel = 5, //< int
	LogFile = 6, //< str
	//WhiteList = 7, //< seq with str
	EventFlags = 8, //< int
	SentRegTypes = 9, //< int
	MaxRegValueSize = 10, //< int
	MinFullActFileSize = 11, //< int
	MaxFullActFileSize = 12, //< int
	FileMonNameMask = 13, //< str
	InjectedDll = 14, //< seq with str
	VerifyInjectedDll = 15, //< bool
	OpenProcessRepeatTimeout = 16, //< int
	RegEventRepeatTimeout = 17, //< int
};

constexpr uint64_t convertToFlag(ConfigField eFieldId)
{
	return (uint64_t)1u << (uint64_t)eFieldId;
}

///
/// edrdrv.sys config values:
/// * **maxQueueSize** - maximal size of all events buffers.
/// * **connectionTimeot** - timeout (ms) for stopping event collection.
/// * **sendMsgTimeout** - timeout (ms) in case if userMode returns error.
/// * **enableDllInject** - enables injection of Dll from "injectedDll".
/// * **logLevel** - log level.
/// * **logFile** - log file (directories must be exist).
/// * **eventFlags** - bit mask of collected raw events.
/// * **sentRegTypes** - bit mask of collected RegValue.
/// * **maxRegValueSize** - max size of collected RegValue.
/// * **minFullActFileSize** - min file size for fullFileRead/Write.
/// * **maxFullActFileSize** - max file size for fullFileRead/Write.
/// * **fileMonNameMask** - mask (postfix) for file monitor. It collects events of specified files only.
/// * **disableSelfProtection** - control driver selfprotection (allow unload).
/// * **injectedDll** - a sequence of strings with names of injected DLLs.
/// * **verifyInjectedDll** [opt] - bool, enables/disables injected dll verification (default: `true`).
/// * **openProcessRepeatTimeout** - timeout (ms) for sending next openProcess event
///   with the same initiator-target processes.
/// * **regEventRepeatTimeout** - timeout (ms) for sending next same registry event 
///   from same process with the same data.
///
constexpr char c_sConfigSchema[] = R"json({
"version": 1,
"fields": [
	{ "name": "disableSelfProtection", "type" : "bool" },
	{ "name": "maxQueueSize", "type" : "uint64"},
	{ "name": "connectionTimeot", "type" : "uint64" },
	{ "name": "sendMsgTimeout", "type" : "uint64" },
	{ "name": "enableDllInject", "type" : "bool" },
	{ "name": "logLevel", "type" : "uint64" },
	{ "name": "logFile", "type" : "wstr" },
	{ "name": "whiteList[]", "type" : "wstr" },
	{ "name": "eventFlags", "type" : "uint64" },
	{ "name": "sentRegTypes", "type" : "uint64" },
	{ "name": "maxRegValueSize", "type" : "uint64" },
	{ "name": "minFullActFileSize", "type" : "uint64" },
	{ "name": "maxFullActFileSize", "type" : "uint64" },
	{ "name": "fileMonNameMask", "type" : "wstr" },
	{ "name": "injectedDll[]", "type" : "wstr" },
	{ "name": "verifyInjectedDll", "type" : "bool" },
	{ "name": "openProcessRepeatTimeout", "type" : "uint64" },
	{ "name": "regEventRepeatTimeout", "type" : "uint64" }
]})json";


//////////////////////////////////////////////////////////////////////////
//
// Self defense
//

//
// Common
//

///
/// The enum class of self-defense access types.
///
enum class AccessType : uint8_t
{
	Invalid = 0, ///< Invalid value (for internal usage)
	NoAccess = 1, ///< Access denied
	ReadOnly = 2, ///< Read only access
	Full = 3, ///< Full access (read and write)

	_Max, ///< For internal usage
};


///
/// The enum class of modes for update self defense rules.
///
/// Action with rules list.
///
enum class UpdateRulesMode : uint8_t
{
	Invalid = 0, ///< Invalid value (For internal usage)
	Replace = 1, ///< replace entire list with new rules
	Clear = 2, ///< clear entire list
	PushBack = 3, ///< add new rules to the list tail
	PushFront = 4, ///< add new rules to the list head
	DeleteByTag = 5, ///< remove rules by tag

	_Max, ///< For internal usage
};


//
// Process
//

///
/// The enum class of updateProcessRules types.
///
enum class RuleType : uint8_t
{
	Invalid = 0, ///< Invalid value (For internal usage)
	Trusted = 1, ///< 'trusted' option rule
	Protected = 2, ///< 'protected' option rule 
	EnableInject = 3, ///< 'injection' option rule
	SendEvent = 4, ///< 'sendEvent' option rule

	_Max, ///< For internal usage
};


///
/// The enum class of SelfDefence control fields (lbvs protocol).
///
enum class UpdateRulesField : variant::lbvs::FieldId
{
	Type = 0, //< int (ProcessRuleType)
	Persistent = 1, //< bool
	Mode = 2, //< int (UpdateRulesMode)
	Rule = 3, //< sequence of dict
	RuleImagePath = 4, //< str
	RuleValue = 5, //< bool or int
	RuleInherite = 6, //< bool
	RuleTag = 7, //< str
	Tag = 8, //< str (tag for deletion)
	RuleRecursive = 9, //< bool
	RulePath = 10, //< str
};

///
/// edrdrv.sys update ProcessRules values:
/// * **maxQueueSize** - maximal size of all events buffers.
///
constexpr char c_sUpdateRulesSchema[] = R"json({
"version": 1,
"fields": [
	{ "name": "type", "type" : "uint32" }, 
	{ "name": "persistent", "type" : "bool" },
	{ "name": "mode", "type" : "uint32" },
	{ "name": "rules[]" },
	{ "name": "rules[].imagePath", "type" : "wstr" },
	{ "name": "rules[].value" },
	{ "name": "rules[].inherit", "type" : "bool" },
	{ "name": "rules[].tag", "type" : "wstr" },
	{ "name": "tag", "type" : "wstr" },
	{ "name": "rules[].recursive", "type" : "bool" },
	{ "name": "rules[].path", "type" : "wstr" }
]})json";


///
/// The enum class of SetProcessInfo fields (lbvs protocol).
///
enum class SetProcessInfoField : variant::lbvs::FieldId
{
	Pid = 0, //< int
	Trusted = 1, //< bool
	Protected = 2, //< bool
	SendEvent = 3, //< bool
	EnableInject = 4, //< bool
};

///
/// edrdrv.sys update sefldefence values:
/// * **maxQueueSize** - maximal size of all events buffers.
///
constexpr char c_sSetProcessInfoSchema[] = R"json({
"version": 1,
"fields": [
	{ "name": "pid", "type" : "uint64" }, 
	{ "name": "trusted", "type" : "bool" },
	{ "name": "protected", "type" : "bool" },
	{ "name": "sendEvent", "type" : "bool" },
	{ "name": "enableInject", "type" : "bool" }
]})json";


//
// Files
//



//
// Registry
//



//////////////////////////////////////////////////////////////////////////
//
// IOCTL
//

///
/// edrdrv IOCTL device NT name.
///
#define CMD_ERDDRV_IOCTLDEVICE_NT_NAME	        L"\\Device\\{5FD68A2A-350B-44CC-961F-65CFCEAC7B4F}"

///
/// edrdrv IOCTL device symlink name.
///
#define CMD_ERDDRV_IOCTLDEVICE_SYMLINK_NAME    L"\\DosDevices\\{157980D8-09B4-4580-B8B6-D32971D056DA}"

///
/// edrdrv config registry value name.
///
#define CMD_ERDDRV_CFG_REG_VALUE_NAME L"config"


///
/// edrdrv IOCTL device win32 name (use in CreateFile).
///
#define CMD_ERDDRV_IOCTLDEVICE_WIN32_NAME      L"\\\\.\\{157980D8-09B4-4580-B8B6-D32971D056DA}"

#define CMD_ERDDRV_CTL_CODE(_code,_read,_write)	\
	DWORD(CTL_CODE ((0x8001),					\
	((0x0800) + (_code)),						\
	(METHOD_BUFFERED),							\
	(((_read)  ? (FILE_READ_ACCESS)  : 0) |		\
	((_write) ? (FILE_WRITE_ACCESS) : 0))))

//
// Codes
//

///
/// Start events processing.
///
/// Input: NONE
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_START CMD_ERDDRV_CTL_CODE(0x01, FALSE, FALSE)

///
/// Stop events processing.
///
/// Input: NONE
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_STOP CMD_ERDDRV_CTL_CODE(0x02, FALSE, FALSE)

///
/// Send LBVS config to driver.
///
/// Input: Buffer with LBVS data (ConfigFields).
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_SET_CONFIG CMD_ERDDRV_CTL_CODE(0x04, TRUE, FALSE)

///
/// Update process rules.
///
/// Input: Buffer with LBVS data (UpdateRulesField).
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_UPDATE_PROCESS_RULES CMD_ERDDRV_CTL_CODE(0x05, TRUE, FALSE)

///
/// Set process info.
///
/// Input: Buffer with LBVS data (ProcessInfoField).
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_SET_PROCESS_INFO CMD_ERDDRV_CTL_CODE(0x06, TRUE, FALSE)

///
/// Update file rules.
///
/// Input: Buffer with LBVS data (UpdateRulesField).
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_UPDATE_FILE_RULES CMD_ERDDRV_CTL_CODE(0x07, TRUE, FALSE)

///
/// Update reg rules.
///
/// Input: Buffer with LBVS data (UpdateRulesField).
/// Output: NONE
///
#define CMD_ERDDRV_IOCTL_UPDATE_REG_RULES CMD_ERDDRV_CTL_CODE(0x08, TRUE, FALSE)

//
// Structures
//

// CMD_ERDDRV_IOCTL_SET_CONFIG
constexpr uint32_t c_nMaxQueueSize = 2 /*mbyte*/ * 1000 /*kbyte*/ * 0x400 /*byte*/;
constexpr uint32_t c_nConnectionTimeot = 1 /*min*/ * 60 /*sec*/ * 1000 /*ms*/;
constexpr uint32_t c_nSendMsgTimeout = 2 /*sec*/ * 1000 /*ms*/;
constexpr uint32_t c_nDefaultSentRegTypesFlags =
	1 << REG_SZ | 1 << REG_EXPAND_SZ | 1 << REG_DWORD | 1 << REG_DWORD_BIG_ENDIAN |
	1 << REG_MULTI_SZ | 1 << REG_QWORD;
constexpr uint32_t c_nDefaultMaxRegValueSize = 64 /*kb*/ * 1024 /*b*/; // Max UNICODE_STRING

constexpr uint64_t c_nDefaultMaxFullActFileSize = 85 /*mb*/ * 1024 /*kb*/ * 1024 /*b*/;
constexpr uint64_t c_nDefaultMinFullActFileSize = 1 /*b*/;

constexpr uint64_t c_nOpenProcessRepeatTimeout = 60 /*sec*/ * 1000 /*ms*/;
constexpr uint64_t c_nRegEventRepeatTimeout = 10 /*sec*/ * 1000 /*ms*/;

///
/// Altitude for: 
/// * filemon (fs minifilter)
/// * regmon(CmRegisterCallbackEx)
/// * objmon (ObRegisterCallbacks)
///
constexpr wchar_t c_sAltitudeValue[] = L"368325";

} // namespace edrdrv
} // namespace cmd
/// @}



