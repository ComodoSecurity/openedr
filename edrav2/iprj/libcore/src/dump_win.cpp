//
// edrav2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer: Denis Bogdanov (21.12.2018)
//
///
/// @file Crash handling implementation (Windows only)
///
#include "pch_win.h"
#include <error\dump\minidumpwriter.hpp>

namespace cmd {
namespace error {
namespace dump {
namespace win {

//
// Handles unhandled SEH exceptions
//
LONG WINAPI handleUnhandledException(PEXCEPTION_POINTERS pExInfo)
{
	if (getStatusFromExceptionInfo(pExInfo) == 0xC00000FD)
	{
		// Save minidump in a separate thread if stack overflow occured
		auto result = std::async(std::launch::async,
			[pExInfo]
			{
				detail::MinidumpWriter::getInstance().save(pExInfo);
			});
		result.get();
		return EXCEPTION_EXECUTE_HANDLER;
	}

	detail::MinidumpWriter::getInstance().save(pExInfo);
	return EXCEPTION_EXECUTE_HANDLER;
}

//
//
//
int filterException(int code, PEXCEPTION_POINTERS pExInfo)
{
	if (code == 0xC00000FD)
	{
		// Save minidump in a separate thread if stack overflow occured
		auto result = std::async(std::launch::async,
			[pExInfo]
			{
				detail::MinidumpWriter::getInstance().save(pExInfo);
			});
		result.get();
		return EXCEPTION_EXECUTE_HANDLER;
	}

	detail::MinidumpWriter::getInstance().save(pExInfo);
	return EXCEPTION_EXECUTE_HANDLER;
}

//
// General function for handling non-SEH exceptions
//
void handleNonSEH()
{
	// Use try-except to obtain fake _EXCEPTION_POINTERS
	__try
	{
		*(int*)0 = 0;
	}
	__except (filterException(GetExceptionCode(), GetExceptionInformation()))
	{
	}
}

//
// Handles pure virtual call errors
//
inline void handlePureCall(void)
{
	handleNonSEH();
}

//
// Handles invalid parameter errors.
//
#pragma warning(push)
#pragma warning(disable : 4100)
void handleInvalidParameter(const wchar_t* expression, const wchar_t* function,
	const wchar_t* file, unsigned int line, uintptr_t pReserved)
#pragma warning(pop)
{
	// @ermakov FIXME: We see windows dialog even then we handle invalid parameter. Why?
	error::InvalidArgument(SL, FMT(
		"Invalid parameter detected in function <" << string::convertWCharToUtf8(function) << ">"
		<< "\nFile: " << string::convertWCharToUtf8(file) << ":" << line
		<< "\nExpression: " << string::convertWCharToUtf8(expression))).log();
	// @ermakov FIXME: Why we stop execution in case of invalid parameter
	// handleNonSEH();
}

//
// Handles signals
//
#pragma warning(push)
#pragma warning(disable : 4100)
void handleSignal(int signal)
#pragma warning(pop)
{
	handleNonSEH();
}

//
// Initializes crash handlers on Windows platform
//
void initCrashHandlers(const Variant& vConfig)
{
	detail::MinidumpWriter::getInstance().init(vConfig);

	// signal handlers
	signal(SIGABRT, handleSignal);
	signal(SIGTERM, handleSignal);

	// unhandled SEH handler
	// TODO: Is any actions with previous handler needed?
	::SetUnhandledExceptionFilter(handleUnhandledException);

	// There is a lot of libraries that want to set their own wrong UnhandledExceptionFilter in our application.
	// One of these is MSVCRT with __report_gsfailure and _call_reportfault leading to many
	// of MSVCRT error reports to be redirected to Dr.Watson.
	// DisableSetUnhandledExceptionFilter();

	// other handlers
	::_set_purecall_handler(handlePureCall);
	::_set_invalid_parameter_handler(handleInvalidParameter);

	// TODO: _set_abort_behavior ?
}

} // namespace win
} // namespace dump
} // namespace error
} // namespace cmd
