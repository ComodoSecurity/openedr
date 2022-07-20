//
// edrav2.edrpm project
//
// Author: Denis Kroshin (30.04.2019)
// Reviewer: Denis Bogdanov (05.05.2019)
//
///
/// @file Injection library
///
/// @addtogroup edrpm
/// 
/// Injection library for process monitoring.
/// 
/// @{
#include "pch.h"
#include "winapi.h"
#include "keyboard.h"
#include "process.h"

namespace cmd {
namespace edrpm {

std::string c_pHookType[] = { "WH_JOURNALRECORD", "WH_JOURNALPLAYBACK",
	"WH_KEYBOARD", "WH_GETMESSAGE", "WH_CALLWNDPROC", "WH_CBT", "WH_SYSMSGFILTER",
	"WH_MOUSE", "WH_HARDWARE", "WH_DEBUG", "WH_SHELL", "WH_FOREGROUNDIDLE",
	"WH_CALLWNDPROCRET", "WH_KEYBOARD_LL", "WH_MOUSE_LL" };

//
//
//
void fnSetWindowsHookEx(int idHook, HINSTANCE hmod)
{
	sendEvent(RawEvent::PROCMON_API_SET_WINDOWS_HOOK, [idHook, hmod](auto serializer)
	{
		bool result = true;
		if (idHook < 0 || unsigned(idHook) >= std::size(c_pHookType))
			result = serializer->write(EventField::HookType, "WH_UNKNOWN");
		else
			result = serializer->write(EventField::HookType, c_pHookType[idHook]);
		if (!result)
			return result;

		auto sModuleName(getModuleFileName(hmod));
		if (!sModuleName.empty())
			result = serializer->write(EventField::ModulePath, sModuleName);
		return result;
	});
}

//
// FIXME: Please use single format for REGISTER_*_CALLBACK functions
//
REGISTER_PRE_CALLBACK(SetWindowsHookExA, [](
	_In_ int idHook,
	_In_ HOOKPROC lpfn,
	_In_opt_ HINSTANCE hmod,
	_In_ DWORD dwThreadId)
{
	//dbgPrint("Inside SetWindowsHookExA");
	if (hmod != NULL)
		fnSetWindowsHookEx(idHook, hmod);
});

//
//
//
REGISTER_PRE_CALLBACK(SetWindowsHookExW, [](
	_In_ int idHook,
	_In_ HOOKPROC lpfn,
	_In_opt_ HINSTANCE hmod,
	_In_ DWORD dwThreadId)
{
	//dbgPrint("Inside SetWindowsHookExW");
	if (hmod != NULL)
		fnSetWindowsHookEx(idHook, hmod);
});

//
//
//
class KeyboardChecker
{
private:
	struct State
	{
		size_t nCount;
		uint64_t nProbe;
	};

	State m_pKeyboardState = {};
	State m_pKeyState[0x100] = {};
	State m_pHotkeyState = {};
	State m_pClipboardState = {};

	std::mutex m_mtxKeyboard;
	std::mutex m_mtxKey;
	std::mutex m_mtxHotkey;
	std::mutex m_mtxClipboard;

	inline bool isNonCharKey(unsigned char nCh)
	{
		return ((0x01 <= nCh) && (nCh <= 0x06) ||
			(0x08 <= nCh) && (nCh <= 0x19) ||
			(0x1B <= nCh) && (nCh <= 0x1F) ||
			(0x20 <= nCh) && (nCh <= 0x2F) ||
			(0x5B <= nCh) && (nCh <= 0x5F) ||
			(0x70 <= nCh) && (nCh <= 0x96) ||
			(0xA0 <= nCh) && (nCh <= 0xB7));
	}

public:
	static const uint16_t c_nVkAny = 0x100;

	bool checkKeyboardState()
	{
		std::scoped_lock lock(m_mtxKeyboard);
		uint64_t now = ::GetTickCount64();
		if (now - m_pKeyboardState.nProbe >= 100)
			m_pKeyboardState.nCount = 0;
		m_pKeyboardState.nCount++;
		m_pKeyboardState.nProbe = now;
		return m_pKeyboardState.nCount >= 10;
	}

	bool checkKeyState(int nKey)
	{
		if (isNonCharKey(nKey) || nKey < 0 || unsigned(nKey) >= std::size(m_pKeyState))
			return false;

		std::scoped_lock lock(m_mtxKey);
		uint64_t now = ::GetTickCount64();
		if (now - m_pKeyState[nKey].nProbe >= 100)
			m_pKeyState[nKey].nCount = 0;
		m_pKeyState[nKey].nCount++;

		size_t nKeyCount = 0;
		if (m_pKeyState[nKey].nCount >= 10)
		{
			for (auto keyState : m_pKeyState)
				if (keyState.nCount >= 10 && now - keyState.nProbe < 100)
					nKeyCount++;
		}
		m_pKeyState[nKey].nProbe = now;
		return nKeyCount >= 27;
	}

	bool checkRegisterHotkey()
	{
		std::scoped_lock lock(m_mtxHotkey);
		uint64_t now = ::GetTickCount64();
		if (now - m_pHotkeyState.nProbe >= 100)
			m_pHotkeyState.nCount = 0;
		m_pHotkeyState.nCount++;
		m_pHotkeyState.nProbe = now;
		return m_pHotkeyState.nCount >= 10;
	}

	bool checkClipboardData()
	{
		std::scoped_lock lock(m_mtxClipboard);
		uint64_t now = ::GetTickCount64();
		if (now - m_pClipboardState.nProbe >= 100)
			m_pClipboardState.nCount = 0;
		m_pClipboardState.nCount++;
		m_pClipboardState.nProbe = now;
		return m_pClipboardState.nCount >= 10;
	}

};

KeyboardChecker g_kbdChecker;

//
//
//
REGISTER_POST_CALLBACK(GetKeyboardState, [](BOOL fResult, PBYTE lpKeyState)
{
	if (!fResult || lpKeyState == NULL)
		return;

	bool fKeyPressed = false;
	for (size_t i = 0; i < 256, !fKeyPressed; ++i)
		fKeyPressed = (lpKeyState[i] != 0);

	if (fKeyPressed && g_kbdChecker.checkKeyboardState())
		sendEvent(RawEvent::PROCMON_API_GET_KEYBOARD_STATE);
});

//
//
//
REGISTER_PRE_CALLBACK(GetKeyState, [](int nVirtKey)
{
	if (g_kbdChecker.checkKeyState(nVirtKey))
		sendEvent(RawEvent::PROCMON_API_GET_KEY_STATE);
});

//
//
//
REGISTER_PRE_CALLBACK(GetAsyncKeyState, [](int vKey)
{
	if (g_kbdChecker.checkKeyState(vKey))
		sendEvent(RawEvent::PROCMON_API_GET_KEY_STATE);
});

//
//
//
REGISTER_PRE_CALLBACK(RegisterHotKey, [](HWND hWnd, int id, UINT fsModifiers, UINT vk)
{
	if (fsModifiers != 0)
		return;

	if (g_kbdChecker.checkRegisterHotkey())
		sendEvent(RawEvent::PROCMON_API_REGISTER_HOT_KEY);
});

//
//
//
REGISTER_PRE_CALLBACK(RegisterRawInputDevices, [](PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize)
{
	sendEvent(RawEvent::PROCMON_API_REGISTER_RAW_INPUT_DEVICES);
});

//
//
//
REGISTER_PRE_CALLBACK(BlockInput, [](
	BOOL fBlockIt)
{
	if (fBlockIt == FALSE)
		return;

	sendEvent(RawEvent::PROCMON_API_BLOCK_INPUT);
});

//
//
//
REGISTER_PRE_CALLBACK(EnableWindow, [](
	_In_ HWND hWnd,
	_In_ BOOL bEnable)
{
	if (hWnd == NULL || bEnable == TRUE)
		return;

	DWORD dwTargetPid = 0;
	::GetWindowThreadProcessId(hWnd, &dwTargetPid);
	if (dwTargetPid == ::GetCurrentProcessId())
		return;

	sendEvent(RawEvent::PROCMON_API_ENABLE_WINDOW);
// 	sendEvent(RawEvent::PROCMON_API_ENABLE_WINDOW, [dwTargetPid](auto serializer)
// 	{
// 		return serializer->write(EventField::TargetPid, uint32_t(dwTargetPid));
// 	});
});

//
//
//
REGISTER_PRE_CALLBACK(GetClipboardData, [](
	_In_ UINT uFormat)
{
	if (g_kbdChecker.checkClipboardData())
		sendEvent(RawEvent::PROCMON_API_GET_CLIPBOARD_DATA);
});

//
//
//
REGISTER_PRE_CALLBACK(SetClipboardViewer, [](
	_In_ HWND hWndNewViewer)
{
	if (hWndNewViewer == NULL)
		return;

	sendEvent(RawEvent::PROCMON_API_SET_CLIPBOARD_VIEWER);
});

//
//
//
REGISTER_PRE_CALLBACK(SendInput, [](
	_In_ UINT cInputs,
	_In_reads_(cInputs) LPINPUT pInputs,
	_In_ int cbSize)
{
	UINT nInput = UINT_MAX;
	for (size_t i = 0; i < cInputs && nInput == UINT_MAX; i++)
	{
		if (pInputs[i].type == INPUT_KEYBOARD && pInputs[i].ki.wVk != 0)
			nInput = INPUT_KEYBOARD;
		else if (pInputs[i].type == INPUT_MOUSE)
			nInput = INPUT_MOUSE;
	}
	if (nInput == UINT_MAX)
		return;

	sendEvent(RawEvent::PROCMON_API_SEND_INPUT);
});

//
//
//
REGISTER_PRE_CALLBACK(keybd_event, [](
	_In_ BYTE bVk,
	_In_ BYTE bScan,
	_In_ DWORD dwFlags,
	_In_ ULONG_PTR dwExtraInfo)
{
	sendEvent(RawEvent::PROCMON_API_KEYBD_EVENT);
});

} // namespace edrpm
} // namespace cmd