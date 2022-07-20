#include "pch.h"

//
// This is fake function should never been called and 
// used only for inject this DLL via import modification of main EXE
//
void InjectorInit()
{
	__debugbreak();
}

#if !defined(FEATURE_ENABLE_MADCHOOK)

namespace detours
{

bool HookAttach(LPVOID Code, LPVOID Callback, LPVOID* NextHook)
{
	*NextHook = Code;
	LONG errorCode = DetourAttach(NextHook, Callback);
	if (NO_ERROR != errorCode)
	{
		SetLastError(errorCode);
	}
	return NO_ERROR == errorCode;
}

bool HookDetach(LPVOID* NextHook, LPVOID Callback)
{
	if (nullptr == NextHook)
		return false;
	
	LONG errorCode = DetourDetach(NextHook, Callback);
	if (NO_ERROR != errorCode)
	{
		SetLastError(errorCode);
	}
	return NO_ERROR == errorCode;
}

bool HookCode(LPVOID Code, LPVOID Callback, LPVOID* NextHook)
{
	CollectAllHooks();
	bool result = HookAttach(Code, Callback, NextHook);
	CommitAllHooks();
	
	return result;
}

void CollectAllHooks()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
}

void CommitAllHooks()
{
	DetourTransactionCommit();
}

bool HookAPI(LPCSTR ModuleName, LPCSTR ApiName, LPVOID Callback, LPVOID* NextHook)
{
	LPVOID codePtr = DetourFindFunction(ModuleName, ApiName);
	if (nullptr == codePtr)
		return false;

	return HookAttach(codePtr, Callback, NextHook);
}

bool UnhookAPI(LPVOID* NextHook, LPVOID Callback)
{
	return HookDetach(NextHook, Callback);
}

#if 0 // TODO: hooking system for delay load DLLs
VOID CALLBACK LdrDllNotificationCallback(
	_In_     ULONG                       NotificationReason,
	_In_     PCLDR_DLL_NOTIFICATION_DATA NotificationData,
	_In_opt_ PVOID                       Context
)
{
	UNREFERENCED_PARAMETER(NotificationData);
	UNREFERENCED_PARAMETER(Context);

	if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED)
	{

	}
}

static PVOID g_LdrNotifyCookie = NULL;

VOID initDynamicPatch()
{
	LdrRegisterDllNotification_t registerDllNotification = (LdrRegisterDllNotification_t)GetProcAddress(GetModuleHandleA("ntdll.dll"), "LdrRegisterDllNotification");
	if (registerDllNotification == NULL)
		return;

	registerDllNotification(0, LdrDllNotificationCallback, NULL, &g_LdrNotifyCookie);
}
#endif

} // namespace detours

#endif // FEATURE_ENABLE_MADCHOOK





