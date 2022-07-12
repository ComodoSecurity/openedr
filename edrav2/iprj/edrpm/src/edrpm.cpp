//
// EDRAv2.edrpm project
//
// Author: Denis Kroshin (08.04.2019)
// Reviewer:
//
/// @addtogroup edrpm edrpm.dll - Injection module
///
/// Injection library for process monitoring.
/// 
/// @{
#include "pch.h"
#include "injection.h"
#include <iterator>
#include <algorithm>

namespace cmd {
namespace edrpm {
	Injection g_Injection;
}
}

// There is A LOT of restrictions for DllMain. Read documentation.
// URL: https://docs.microsoft.com/en-us/windows/desktop/dlls/dynamic-link-library-best-practices

static HANDLE threadHandle = nullptr;

DWORD WINAPI InitializationThread(LPVOID Context)
{
	WaitForInputIdle(GetCurrentProcess(), 3000);

	cmd::edrpm::g_Injection.init();

	if (threadHandle)
	{
		CloseHandle(threadHandle), threadHandle = nullptr;
	}

	return NO_ERROR;
}

#if defined(_DEBUG)
#define DEBUG_ATTACH
#endif

#if defined(DEBUG_ATTACH)
#include <Wtsapi32.h>
#pragma comment(lib, "WtsApi32.lib")
#endif

//
// DLL Entry point
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD eCallReason, LPVOID /*pReserved*/)
{
	switch (eCallReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		DisableThreadLibraryCalls(hModule);
#if defined(FEATURE_ENABLE_MADCHOOK)
		StaticLibHelper_Init((PVOID)&DllMain);
#endif
#if defined(DEBUG_ATTACH)
#define DEBUG_ATTACH_TEXT (LPWSTR)L"Attach now!"

		std::vector<WCHAR> message(MAX_PATH, 0);
		swprintf_s(&message[0], message.size(), L"[%i] I am waiting for attach debugger...", GetCurrentProcessId());

		DWORD response = 0;

		WTSSendMessageW(
			WTS_CURRENT_SERVER_HANDLE,
			WTSGetActiveConsoleSessionId(),
			DEBUG_ATTACH_TEXT,
			sizeof(DEBUG_ATTACH_TEXT) - sizeof(WCHAR),
			message.data(),
			string_byte_size(wcslen(message.data())),
			MB_OK,
			0,
			&response,
			TRUE);
#endif
		//
		// Workaround for buggy software 
		// CES-9749: [EDR][ Agent ] Excel DDE command problem
		//
		WCHAR appName[MAX_PATH] = {0};
		GetModuleBaseNameW(GetCurrentProcess(), NULL, &appName[0], _countof(appName));
		if (0 == _wcsnicmp(L"EXCEL.EXE", appName, _countof(appName)))
		{
			threadHandle = ::CreateThread(nullptr, 0, InitializationThread, nullptr, 0, nullptr);
		}
		else
		{
			cmd::edrpm::g_Injection.init();
		}
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		cmd::edrpm::g_Injection.finalize();
#if defined(FEATURE_ENABLE_MADCHOOK)
		StaticLibHelper_Final((PVOID)&DllMain);
#endif
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif
	void logMadCHookError(const wchar_t* sMsg)
	{
		std::string s;
		std::transform(sMsg, sMsg + std::wcslen(sMsg), std::back_inserter(s), [](wchar_t c) { return char(c); });
		cmd::edrpm::logError(s);
	}

	void logMadCHookInfo(const wchar_t* sMsg)
	{
		std::string s;
		std::transform(sMsg, sMsg + std::wcslen(sMsg), std::back_inserter(s), [](wchar_t c) { return char(c); });
		cmd::edrpm::logError(s, cmd::edrpm::ErrorType::Info);
	}
#ifdef __cplusplus
} // extern "C" 
#endif
