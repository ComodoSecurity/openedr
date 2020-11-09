//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Driver logging
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace openEdr {
namespace log {

///
/// The logging subsystem initialization.
///
void initialize();

///
/// Set log filename.
///
/// @param sFileName - a name of the log file.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS setLogFile(const WCHAR* sFileName);

///
/// Set log filename.
///
/// @param usFileName - a name of the log file.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS setLogFile(UNICODE_STRING usFileName);

///
/// Size of buffer for file logging.
///
inline constexpr size_t c_nLogFileBufferSize = 256 * 1024; // in bytes

///
/// Logging the info with printf-style formatting.
///
/// @param sFormat - printf-style format string.
///
/// logInfo() works at PASSIVE_LEVEL only. 
/// logInfo() can be called at a higher level, but logs nothing.
///
/// logInfo() pass the info  
///   - into the debug log (vDbgPrint) in current thread and
///   - into the log file in additional thread using a circle buffer.
///
void logInfo(const char* sFormat, ...);

// An overloading of logInfo without parameters
// Need for LOGERROR(ntStatus)
inline void logInfo(){ logInfo("\r\n"); };

///
/// Finalize logging subsystem
///
void finalize();

} // namespace log
} // namespace openEdr

//////////////////////////////////////////////////////////////////////////
//
// Logging macroses
//
//////////////////////////////////////////////////////////////////////////

#define _LOGINFO_RAW(...) openEdr::log::logInfo(__VA_ARGS__)
#define _LOGINFON(_MsgLogLevel, ...) ( (g_pCommonData->nLogLevel >= _MsgLogLevel) ?  (_LOGINFO_RAW(__VA_ARGS__), ((int)0)) :  ((int)0) )

///
/// logInfo on loglevel >= 1
///
/// Pass all params to logInfo.
/// @see logInfo
///
#define LOGINFO1(...) _LOGINFON(1, __VA_ARGS__)

///
/// logInfo on loglevel >= 2
///
/// Pass all params to logInfo.
/// @see logInfo
///
#define LOGINFO2(...) _LOGINFON(2, __VA_ARGS__)

///
/// logInfo on loglevel >= 3
///
/// Pass all params to logInfo.
/// @see logInfo
///
#define LOGINFO3(...) _LOGINFON(3, __VA_ARGS__)

///
/// logInfo on loglevel >= 4
///
/// Pass all params to logInfo.
/// @see logInfo
///
#define LOGINFO4(...) _LOGINFON(4, __VA_ARGS__)

///
/// Log error with message.
///
/// @param eStatus - NTSTATUS for message.
/// @param ... [opt] - additional message: format + parameters.
///
#define LOGERROR(eStatus, ...) (_LOGINFO_RAW("[ERROR] Line: %s(%d). Errno: %08X. ", __FILE__, __LINE__, eStatus), _LOGINFO_RAW(__VA_ARGS__), (eStatus))

///
/// if x (NTSTATUS) is error, logs error and return this NTSTATUS.
///
/// @param x - expression returned NTSTATUS.
/// @param ... [opt] - additional message: format + parameters.
///
#define IFERR_RET(x, ...) {const NTSTATUS _ns = (x); if(!NT_SUCCESS(_ns)) {return LOGERROR(_ns, __VA_ARGS__);}}

///
/// if x (NTSTATUS) is error, logs error.
///
/// @param x - expression returned NTSTATUS.
/// @param ... [opt] - additional message: format + parameters.
///
#define IFERR_LOG(x, ...) {const NTSTATUS _ns = (x); if(!NT_SUCCESS(_ns)) {LOGERROR(_ns, __VA_ARGS__);}}

///
/// if x (NTSTATUS) is error, return this error.
///
/// @param x - expression returned NTSTATUS.
///
#define IFERR_RET_NOLOG(x) {const NTSTATUS _ns = (x); if(!NT_SUCCESS(_ns)) {return _ns;}}

/// @}
