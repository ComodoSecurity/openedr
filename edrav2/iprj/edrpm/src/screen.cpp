//
// edrav2.edrpm project
//
// Author: Denis Kroshin (03.07.2019)
// Reviewer: Denis Bogdanov (xx.xx.2019)
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
#include "screen.h"

namespace openEdr {
namespace edrpm {

//
//
//
bool SkipDC(HDC hdcSrc)
{
	if (hdcSrc == NULL)
		return true;

	HWND hWnd = ::WindowFromDC(hdcSrc);
	if (hWnd == NULL)
		return true;

	DWORD dwTargetPid = 0;
	::GetWindowThreadProcessId(hWnd, &dwTargetPid);
	return (dwTargetPid == ::GetCurrentProcessId());
}

//
//
//
REGISTER_PRE_CALLBACK(SystemParametersInfoA, [](
	UINT  uiAction,
	UINT  uiParam,
	PVOID pvParam,
	UINT  fWinIni)
{
	if (uiAction != SPI_SETDESKWALLPAPER)
		return;
	sendEvent(RawEvent::PROCMON_DESKTOP_WALLPAPER_SET);
});

//
//
//
REGISTER_PRE_CALLBACK(SystemParametersInfoW, [](
	UINT  uiAction,
	UINT  uiParam,
	PVOID pvParam,
	UINT  fWinIni)
{
	if (uiAction != SPI_SETDESKWALLPAPER)
		return;
	sendEvent(RawEvent::PROCMON_DESKTOP_WALLPAPER_SET);
});

//
//
//
REGISTER_PRE_CALLBACK(ClipCursor, [](
	const RECT *lpRect)
{
	sendEvent(RawEvent::PROCMON_API_CLIP_CURSOR);
});

//
//
//
REGISTER_PRE_CALLBACK(mouse_event, [](
	DWORD     dwFlags,
	DWORD     dx,
	DWORD     dy,
	DWORD     dwData,
	ULONG_PTR dwExtraInfo)
{
	sendEvent(RawEvent::PROCMON_API_MOUSE_EVENT);
});

//
//
//
REGISTER_PRE_CALLBACK(BitBlt, [](
	HDC   hdc,
	int   x,
	int   y,
	int   cx,
	int   cy,
	HDC   hdcSrc,
	int   x1,
	int   y1,
	DWORD rop)
{
	if (SkipDC(hdcSrc))
		return;
	sendEvent(RawEvent::PROCMON_COPY_WINDOW_BITMAP);
});

//
//
//
REGISTER_PRE_CALLBACK(MaskBlt, [](
	HDC     hdcDest,
	int     xDest,
	int     yDest,
	int     width,
	int     height,
	HDC     hdcSrc,
	int     xSrc,
	int     ySrc,
	HBITMAP hbmMask,
	int     xMask,
	int     yMask,
	DWORD   rop)
{
	if (SkipDC(hdcSrc))
		return;
	sendEvent(RawEvent::PROCMON_COPY_WINDOW_BITMAP);
});

//
//
//
REGISTER_PRE_CALLBACK(PlgBlt, [](
	HDC         hdcDest,
	const POINT *lpPoint,
	HDC         hdcSrc,
	int         xSrc,
	int         ySrc,
	int         width,
	int         height,
	HBITMAP     hbmMask,
	int         xMask,
	int         yMask)
{
	if (SkipDC(hdcSrc))
		return;
	sendEvent(RawEvent::PROCMON_COPY_WINDOW_BITMAP);
});

//
//
//
REGISTER_PRE_CALLBACK(StretchBlt, [](
	HDC   hdcDest,
	int   xDest,
	int   yDest,
	int   wDest,
	int   hDest,
	HDC   hdcSrc,
	int   xSrc,
	int   ySrc,
	int   wSrc,
	int   hSrc,
	DWORD rop)
{
	if (SkipDC(hdcSrc))
		return;
	sendEvent(RawEvent::PROCMON_COPY_WINDOW_BITMAP);
});

} // namespace edrpm
} // namespace openEdr