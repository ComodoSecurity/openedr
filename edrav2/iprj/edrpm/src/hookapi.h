#ifndef __DETOURS_HOOK_API_H__
#define __DETOURS_HOOK_API_H__

namespace detours
{

void CollectAllHooks();
void CommitAllHooks();

bool HookCode(LPVOID Code, LPVOID Callback, LPVOID* NextHook);
bool HookAPI(LPCSTR ModuleName, LPCSTR ApiName, LPVOID Callback, LPVOID* NextHook);
bool UnhookAPI(LPVOID* NextHook, LPVOID Callback);

} // namespace detours

#endif // __DETOURS_HOOK_API_H__
