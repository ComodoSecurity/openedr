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

namespace openEdr {
namespace edrpm {
	Injection g_Injection;
}
}

// There is A LOT of restrictions for DllMain. Read documentation.
// URL: https://docs.microsoft.com/en-us/windows/desktop/dlls/dynamic-link-library-best-practices

//
// DLL Entry point
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD eCallReason, LPVOID /*pReserved*/)
{

	switch (eCallReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		//DisableThreadLibraryCalls(hModule);
		StaticLibHelper_Init((PVOID)&DllMain);
		openEdr::edrpm::g_Injection.init();
		break;
	}
	case DLL_PROCESS_DETACH:
	{
		openEdr::edrpm::g_Injection.finalize();
		StaticLibHelper_Final((PVOID)&DllMain);
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
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
		openEdr::edrpm::logError(s);
	}

	void logMadCHookInfo(const wchar_t* sMsg)
	{
		std::string s;
		std::transform(sMsg, sMsg + std::wcslen(sMsg), std::back_inserter(s), [](wchar_t c) { return char(c); });
		openEdr::edrpm::logError(s, openEdr::edrpm::ErrorType::Info);
	}
#ifdef __cplusplus
} // extern "C" 
#endif
