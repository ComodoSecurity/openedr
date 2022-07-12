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
#include "microphone.h"
#include "Mmdeviceapi.h"

namespace cmd {
namespace edrpm {

typedef struct MY_IMMDeviceEnumeratorVtbl
{
	BEGIN_INTERFACE

	HRESULT(STDMETHODCALLTYPE *QueryInterface)(
		IMMDeviceEnumerator * This,
		/* [in] */ REFIID riid,
		/* [annotation][iid_is][out] */
		_COM_Outptr_  void **ppvObject);

	ULONG(STDMETHODCALLTYPE *AddRef)(
		IMMDeviceEnumerator * This);

	ULONG(STDMETHODCALLTYPE *Release)(
		IMMDeviceEnumerator * This);

	/* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE *EnumAudioEndpoints)(
		IMMDeviceEnumerator * This,
		/* [annotation][in] */
		_In_  EDataFlow dataFlow,
		/* [annotation][in] */
		_In_  DWORD dwStateMask,
		/* [annotation][out] */
		_Out_  IMMDeviceCollection **ppDevices);

	/* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE *GetDefaultAudioEndpoint)(
		IMMDeviceEnumerator * This,
		/* [annotation][in] */
		_In_  EDataFlow dataFlow,
		/* [annotation][in] */
		_In_  ERole role,
		/* [annotation][out] */
		_Out_  IMMDevice **ppEndpoint);

	/* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE *GetDevice)(
		IMMDeviceEnumerator * This,
		/* [annotation][in] */
		_In_  LPCWSTR pwstrId,
		/* [annotation][out] */
		_Out_  IMMDevice **ppDevice);

	/* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE *RegisterEndpointNotificationCallback)(
		IMMDeviceEnumerator * This,
		/* [annotation][in] */
		_In_  IMMNotificationClient *pClient);

	/* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE *UnregisterEndpointNotificationCallback)(
		IMMDeviceEnumerator * This,
		/* [annotation][in] */
		_In_  IMMNotificationClient *pClient);

	END_INTERFACE
} MY_IMMDeviceEnumeratorVtbl;

interface MY_IMMDeviceEnumerator
{
	CONST_VTBL struct MY_IMMDeviceEnumeratorVtbl *lpVtbl;
};

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);

typedef HRESULT(STDMETHODCALLTYPE *IMMDeviceEnumerator_EnumAudioEndpoints)(
	IMMDeviceEnumerator * This,
	/* [annotation][in] */
	_In_  EDataFlow dataFlow,
	/* [annotation][in] */
	_In_  DWORD dwStateMask,
	/* [annotation][out] */
	_Out_  IMMDeviceCollection **ppDevices);

IMMDeviceEnumerator_EnumAudioEndpoints pEnumAudioEndpoints = NULL;

//
//
//
HRESULT STDMETHODCALLTYPE fnEnumAudioEndpoints(
	IMMDeviceEnumerator * This,
	_In_  EDataFlow dataFlow,
	_In_  DWORD dwStateMask,
	_Out_  IMMDeviceCollection **ppDevices)
{

	if ((dataFlow == eCapture) && (dwStateMask & DEVICE_STATE_ACTIVE))
	{
		//OutputDebugStringW(L"Injection monitor MicroPhone on vista and later\n");
		sendEvent(RawEvent::PROCMON_API_ENUM_AUDIO_ENDPOINTS);
	}

	return pEnumAudioEndpoints(This, dataFlow, dwStateMask, ppDevices);
}

//
//
//
void parseIMMDeviceEnumerator(MY_IMMDeviceEnumerator* pEnumerator)
{
	static bool fHooked = false;
	::EnterCriticalSection(&g_mtxIMMDeviceEnumerator);
	if (!fHooked)
	{
		::LoadLibraryW(L"MMDevAPI.dll"); // TODO: hooking system for delay load DLLs
#if defined(FEATURE_ENABLE_MADCHOOK)
		HookCode(pEnumerator->lpVtbl->EnumAudioEndpoints, fnEnumAudioEndpoints, (LPVOID*)&pEnumAudioEndpoints);
#else
		fHooked = detours::HookCode(pEnumerator->lpVtbl->EnumAudioEndpoints, fnEnumAudioEndpoints, (LPVOID*)&pEnumAudioEndpoints);
#endif
	}
	::LeaveCriticalSection(&g_mtxIMMDeviceEnumerator);
}


//
//
//
REGISTER_POST_CALLBACK_IF(CoCreateInstanceWin7, !IsWindows8OrGreater(), [](
	HRESULT result,
	_In_ REFCLSID rclsid,
	_In_opt_ LPUNKNOWN pUnkOuter,
	_In_ DWORD dwClsContext,
	_In_ REFIID riid,
	LPVOID FAR * ppv)
{
	if (FAILED(result))
		return;

	if (IsEqualCLSID(rclsid, CLSID_MMDeviceEnumerator) &&
		ppv != NULL && *ppv != NULL)
		parseIMMDeviceEnumerator((MY_IMMDeviceEnumerator*)*ppv);
});

//
//
//
REGISTER_POST_CALLBACK_IF(CoCreateInstanceWin8, IsWindows8OrGreater(), [](
	HRESULT result,
	_In_ REFCLSID rclsid,
	_In_opt_ LPUNKNOWN pUnkOuter,
	_In_ DWORD dwClsContext,
	_In_ REFIID riid,
	LPVOID FAR * ppv)
{
	if (FAILED(result))
		return;

	if (IsEqualCLSID(rclsid, CLSID_MMDeviceEnumerator) &&
		ppv != NULL && *ppv != NULL)
		parseIMMDeviceEnumerator((MY_IMMDeviceEnumerator*)*ppv);
});

//
//
//
REGISTER_PRE_CALLBACK(waveInOpen, [](
	LPHWAVEIN phwi,
	_In_ UINT uDeviceID,
	_In_ LPCWAVEFORMATEX pwfx,
	_In_opt_ DWORD_PTR dwCallback,
	_In_opt_ DWORD_PTR dwInstance,
	_In_ DWORD fdwOpen)
{
	sendEvent(RawEvent::PROCMON_API_WAVE_IN_OPEN);
});

} // namespace edrpm
} // namespace cmd