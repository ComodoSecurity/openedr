//
// EDRAv2.edrmm project
//
// Autor: Yury Podpruzhnikov (08.04.2019)
// Reviewer: Denis Bogdanov (22.02.2019)
//
/// @addtogroup edrmm edrmm.dll - Memory manager
/// Memory manager provides memory allocation and leak detection functions.
/// 
/// @{
#include "pch.h"
#include "memmgr.h"

//
// DLL Entry point
//
BOOL APIENTRY DllMain(HMODULE hModule, DWORD eCallReason, LPVOID /*pReserved*/)
{
	switch (eCallReason)
	{
		case DLL_PROCESS_ATTACH:
			cmd::edrmm::startMemoryLeaksMonitoring();
			break;
		case DLL_PROCESS_DETACH:
			cmd::edrmm::saveMemoryLeaks();
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}

