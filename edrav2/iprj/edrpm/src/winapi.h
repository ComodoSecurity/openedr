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
#pragma once

namespace cmd {
namespace edrpm {

#define NtCurrentProcess() ( (HANDLE)(LONG_PTR) -1 )  
#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )   

//////////////////////////////////////////////////////////////////////////
// ntdll.dll
NTSYSAPI NTSTATUS NTAPI NtCreateSymbolicLinkObject(
	OUT PHANDLE             pHandle,
	IN ACCESS_MASK          DesiredAccess,
	IN POBJECT_ATTRIBUTES   ObjectAttributes,
	IN PUNICODE_STRING      DestinationName);

extern "C"
NTSYSAPI NTSTATUS NTAPI NtDuplicateObject(
	IN HANDLE               SourceProcessHandle,
	IN HANDLE               SourceHandle,
	IN HANDLE               TargetProcessHandle,
	OUT PHANDLE             TargetHandle,
	IN ACCESS_MASK          DesiredAccess OPTIONAL,
	IN BOOLEAN              InheritHandle,
	IN ULONG                Options);

NTSYSAPI NTSTATUS NTAPI NtReadVirtualMemory(
	IN HANDLE               ProcessHandle,
	IN PVOID                BaseAddress,
	OUT PVOID               Buffer,
	IN ULONG                NumberOfBytesToRead,
	OUT PULONG              NumberOfBytesReaded OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtWriteVirtualMemory(
	IN HANDLE               ProcessHandle,
	IN PVOID                BaseAddress,
	IN PVOID                Buffer,
	IN ULONG                NumberOfBytesToWrite,
	OUT PULONG              NumberOfBytesWritten OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtSetInformationThread(
	IN HANDLE              ThreadHandle,
	IN THREADINFOCLASS     ThreadInformationClass,
	IN PVOID               ThreadInformation,
	IN ULONG               ThreadInformationLength);

//////////////////////////////////////////////////////////////////////////
// ntdll.dll
DECLARE_TRAMP("ntdll.dll", NtReadVirtualMemory);
DECLARE_TRAMP("ntdll.dll", NtWriteVirtualMemory);
DECLARE_TRAMP_FLAGS("ntdll.dll", NtCreateFile, FOLLOW_JMP);
DECLARE_TRAMP("ntdll.dll", NtCreateSymbolicLinkObject);
DECLARE_TRAMP("ntdll.dll", NtSetInformationThread);

//////////////////////////////////////////////////////////////////////////
// kernel32.dll
DECLARE_TRAMP_FLAGS("kernel32.dll", ExitProcess, NO_SAFE_UNHOOKING);

//////////////////////////////////////////////////////////////////////////
// user32.dll
DECLARE_TRAMP("user32.dll", SetWindowsHookExA);
DECLARE_TRAMP("user32.dll", SetWindowsHookExW);
DECLARE_TRAMP("user32.dll", GetKeyboardState);
DECLARE_TRAMP("user32.dll", GetKeyState);
DECLARE_TRAMP("user32.dll", GetAsyncKeyState);
DECLARE_TRAMP("user32.dll", RegisterHotKey);
DECLARE_TRAMP("user32.dll", RegisterRawInputDevices);
DECLARE_TRAMP("user32.dll", BlockInput);
DECLARE_TRAMP("user32.dll", EnableWindow);
DECLARE_TRAMP("user32.dll", GetClipboardData);
DECLARE_TRAMP("user32.dll", SetClipboardViewer);
DECLARE_TRAMP("user32.dll", SendInput);
DECLARE_TRAMP("user32.dll", keybd_event);
DECLARE_TRAMP("user32.dll", SystemParametersInfoA);
DECLARE_TRAMP("user32.dll", SystemParametersInfoW);
DECLARE_TRAMP("user32.dll", ClipCursor);
DECLARE_TRAMP("user32.dll", mouse_event);

//////////////////////////////////////////////////////////////////////////
// gdi32.dll
DECLARE_TRAMP("gdi32.dll", BitBlt);
DECLARE_TRAMP("gdi32.dll", MaskBlt);
DECLARE_TRAMP("gdi32.dll", PlgBlt);
DECLARE_TRAMP("gdi32.dll", StretchBlt);

//////////////////////////////////////////////////////////////////////////
// ole32.dll
DECLARE_TRAMP_ALIAS("ole32.dll", CoCreateInstance, CoCreateInstanceWin7);

//////////////////////////////////////////////////////////////////////////
// combase.dll
DECLARE_TRAMP_ALIAS("combase.dll", CoCreateInstance, CoCreateInstanceWin8);

//////////////////////////////////////////////////////////////////////////
// combase.dll
DECLARE_TRAMP("winmm.dll", waveInOpen);

//////////////////////////////////////////////////////////////////////////
// advapi32.dll
DECLARE_TRAMP("advapi32.dll", CreateProcessWithLogonW);
DECLARE_TRAMP("advapi32.dll", ImpersonateNamedPipeClient);

//////////////////////////////////////////////////////////////////////////
// sspicli.dll
DECLARE_TRAMP("sspicli.dll", LsaLogonUser);

} // namespace edrpm
} // namespace cmd

