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
#include "process.h"

namespace openEdr {
namespace edrpm {

#pragma warning(disable : 26812)

//
// FIXME: Move to common functions and change Application for usage it
//
std::wstring getModuleFileName(HMODULE hModule)
{
	wchar_t pFileName[MAX_PATH]; *pFileName = 0;
	auto nStrSize = ::GetModuleFileNameW(hModule, pFileName, DWORD(std::size(pFileName)));
	if (nStrSize == 0)
		return std::wstring();

	if (nStrSize < std::size(pFileName))
		return pFileName;

	static const size_t c_nMaxLen = 0x8000; // FIXME: Use common const variable
	std::wstring pFileNameDyn(c_nMaxLen, 0);
	nStrSize = ::GetModuleFileNameW(hModule, pFileNameDyn.data(), DWORD(pFileNameDyn.size()));
	if (nStrSize == 0 || nStrSize == c_nMaxLen)
		return std::wstring();

	return pFileNameDyn;
}


static const DWORD g_nInvalidPid = 0;

//
//
//
DWORD getParentProcessId(const DWORD nPid)
{
	// TODO: Enum processes with NtQuerySystemInformation + SystemProcessInformation
	// URL: https://stackoverflow.com/questions/50172564/detect-suspended-windows-8-10-process
	ScopedHandle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (hSnapshot == INVALID_HANDLE_VALUE)
		throw std::exception("Fail to create snapshot");

	PROCESSENTRY32 pe32;
	ZeroMemory(&pe32, sizeof(pe32));
	pe32.dwSize = sizeof(pe32);
	if (!::Process32First(hSnapshot, &pe32))
		throw std::exception("Fail to get process info");

	do
	{
		if (pe32.th32ProcessID == nPid)
			return pe32.th32ParentProcessID;
	} while (::Process32Next(hSnapshot, &pe32));

	return g_nInvalidPid;
}

//
//
//
DWORD getParentProcessId(const HANDLE hProcess)
{
	PROCESS_BASIC_INFORMATION pbi;
	memset(&pbi, 0, sizeof(PROCESS_BASIC_INFORMATION));

	if (NT_SUCCESS(::NtQueryInformationProcess(hProcess, ProcessBasicInformation, 
		&pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL)))
		return (DWORD)(ULONG_PTR)pbi.Reserved3;

	DWORD pid = g_nInvalidPid;

	HANDLE hDuplicate;
	auto hCurProcess = ::GetCurrentProcess();
	if (::DuplicateHandle(hCurProcess, hProcess, hCurProcess, &hDuplicate, PROCESS_QUERY_INFORMATION, FALSE, 0) ||
		::DuplicateHandle(hCurProcess, hProcess, hCurProcess, &hDuplicate, PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0))
	{
		memset(&pbi, 0, sizeof(PROCESS_BASIC_INFORMATION));
		if (NT_SUCCESS(::NtQueryInformationProcess(hDuplicate, ProcessBasicInformation,
			&pbi, sizeof(PROCESS_BASIC_INFORMATION), NULL)))
			pid = (DWORD)(ULONG_PTR)pbi.Reserved3;
		::CloseHandle(hDuplicate);
	}

	if (pid == g_nInvalidPid)
		logError("Fail to query ProcessBasicInformation");
	return pid;
}

//
//
//
DWORD getProcessId(HANDLE hProcess)
{
	auto hCurProcess = ::GetCurrentProcess();
	if (hProcess == hCurProcess)
		return ::GetCurrentProcessId();

	HANDLE hDuplicate;
	DWORD pid = ::GetProcessId(hProcess);
	if (pid == 0 &&
		(::DuplicateHandle(hCurProcess, hProcess, hCurProcess, &hDuplicate, PROCESS_QUERY_INFORMATION, FALSE, 0) ||
		::DuplicateHandle(hCurProcess, hProcess, hCurProcess, &hDuplicate, PROCESS_QUERY_LIMITED_INFORMATION, FALSE, 0)))
	{
		pid = ::GetProcessId(hDuplicate);
		::CloseHandle(hDuplicate);
	}
	return pid;
}

//
//
//
std::vector<uint8_t> getTokenInformation(HANDLE hToken, TOKEN_INFORMATION_CLASS eTokenType)
{
	DWORD nSize = 0;
	::GetTokenInformation(hToken, eTokenType, nullptr, nSize, &nSize);
	auto ec = GetLastError();
	if (ec != ERROR_INSUFFICIENT_BUFFER && ec != ERROR_BAD_LENGTH)
		//throw std::exception("Fail to get token information");
		return {}; // Token opened without TOKEN_QUERY rights
	std::vector<uint8_t> pBuffer(nSize);
	if (!::GetTokenInformation(hToken, eTokenType, pBuffer.data(), nSize, &nSize))
		//throw std::exception("Fail to get token information");
		return {};
	return pBuffer;
}

//
//
//
std::string getTokenUser(const HANDLE pToken)
{
	auto pInfo = getTokenInformation(pToken, TokenUser);
	if (pInfo.empty())
		return "";
	ScopedLocalMem sTmp;
	if (!::ConvertSidToStringSidA(PTOKEN_USER(pInfo.data())->User.Sid, (LPSTR*)&sTmp))
		throw std::exception("Fail to convert SID to string");
	return (char*)sTmp.get();
}

//
//
//
bool isTokenElevated(const HANDLE hToken)
{
	auto pInfo = getTokenInformation(hToken, TokenElevation);
	if (pInfo.empty())
		return false;
	return PTOKEN_ELEVATION(pInfo.data())->TokenIsElevated != 0;
}

//
//
//
DWORD getTokenAuthID(const HANDLE pToken)
{
	auto pInfo = getTokenInformation(pToken, TokenStatistics);
	if (pInfo.empty())
		return 0;
	return PTOKEN_STATISTICS(pInfo.data())->AuthenticationId.LowPart;
}

//
//
//
REGISTER_PRE_CALLBACK(NtReadVirtualMemory, [](HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer,
	ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded)
{
	uint32_t nCurrentPid = ::GetCurrentProcessId();
	uint32_t nTargetPid = getProcessId(ProcessHandle);
	if (nTargetPid != nCurrentPid && getParentProcessId(ProcessHandle) != nCurrentPid)
		sendEvent(RawEvent::PROCMON_PROCESS_MEMORY_READ, 
			[nTargetPid](auto serializer)
			{
				return serializer->write(EventField::TargetPid, nTargetPid);
			},
			[nTargetPid](auto hash)
			{
				hash->update({ nTargetPid });
			});
});

//
//
//
REGISTER_PRE_CALLBACK(NtWriteVirtualMemory, [](HANDLE ProcessHandle, PVOID BaseAddress, PVOID Buffer,
	ULONG NumberOfBytesToRead, PULONG NumberOfBytesReaded)
{
	uint32_t nCurrentPid = ::GetCurrentProcessId();
	uint32_t nTargetPid = getProcessId(ProcessHandle);
	if (nTargetPid != nCurrentPid && getParentProcessId(ProcessHandle) != nCurrentPid)
		sendEvent(RawEvent::PROCMON_PROCESS_MEMORY_WRITE, [nTargetPid](auto serializer)
		{
			return serializer->write(EventField::TargetPid, nTargetPid);
		});
});

//
//
//
REGISTER_PRE_CALLBACK(ExitProcess, [](UINT uExitCode)
{
	g_Injection.freeQueue();
});

//
//
//
REGISTER_POST_CALLBACK(CreateProcessWithLogonW, [](BOOL fResult, LPCWSTR lpUsername, LPCWSTR lpDomain, LPCWSTR lpPassword,
	DWORD dwLogonFlags, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, DWORD dwCreationFlags,
	LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	if (!fResult)
		return;

	std::wstring szUser(lpUsername);
	if (lpDomain != NULL)
		szUser += std::wstring(L"@") + lpDomain;
	sendEvent(RawEvent::PROCMON_INTERACTIVE_LOGON, [szUser](auto serializer)
	{
		return serializer->write(EventField::UserName, szUser);
	});
});


// helper for initializing Kernel type ANSI strings from string literals
void initStr(LSA_STRING* a, const char* b)
{
	a->Length = (USHORT)(strlen(b) * sizeof(*b));
	a->MaximumLength = (USHORT)(a->Length + sizeof(*b));
	a->Buffer = const_cast<char*>(b);
}

//
std::string getLogonUserName()
{
/*
	LSA_STRING packageName;
	initStr(&packageName, MSV1_0_PACKAGE_NAME);
	ULONG nAuthPkg = 0;
	if (NT_SUCCESS(LsaLookupAuthenticationPackage(LsaHandle, &packageName, &nAuthPkg)))
	{

	}
*/
	return {};
}

// 
// advapi32.dll::LogonUserA -> advapi32.dll::LogonUserW -> advapi32.dll::LogonUserExExW -> sspicli.dll::LogonUserExExW -> sspicli.dll::LsaLogonUser
// advapi32.dll::LogonUserExW -> advapi32.dll::LogonUserExExW -> sspicli.dll::LogonUserExExW -> sspicli.dll::LsaLogonUser
// secur32.dll -> sspicli.dll::LsaLogonUser
//
REGISTER_POST_CALLBACK(LsaLogonUser, [](NTSTATUS nResult, HANDLE LsaHandle, PLSA_STRING OriginName, 
	SECURITY_LOGON_TYPE LogonType, ULONG AuthenticationPackage, PVOID AuthenticationInformation, 
	ULONG AuthenticationInformationLength, PTOKEN_GROUPS LocalGroups, PTOKEN_SOURCE SourceContext,
	PVOID* ProfileBuffer, PULONG ProfileBufferLength, PLUID LogonId, PHANDLE Token, PQUOTA_LIMITS Quotas,
	PNTSTATUS SubStatus)
{
	if (!NT_SUCCESS(nResult) || LogonType != Interactive)
		return;

	std::string sUser = getLogonUserName();
	sendEvent(RawEvent::PROCMON_INTERACTIVE_LOGON, [sUser](auto serializer)
	{
		return (sUser.empty() ? true : serializer->write(EventField::UserName, sUser));
	});
});

//
//
//
void sendImpersonationEvent(RawEvent eEvent, const HANDLE hThread, HANDLE hToken)
{
	if (hThread == NULL || hToken == NULL)
		return;

	uint32_t nTid = ::GetThreadId(hThread);
	//uint32_t nPid = ::GetProcessIdOfThread(hThread);
	auto sSid = getTokenUser(hToken);
	auto isElevated = isTokenElevated(hToken);
	auto nAuthId = getTokenAuthID(hToken);
	sendEvent(eEvent, [nTid, sSid, isElevated/*, nPid*/](auto serializer)
		{
			return (serializer->write(EventField::ThreadId, nTid) &&
				serializer->write(EventField::UserSid, sSid) &&
				serializer->write(EventField::UserIsElevated, isElevated) /*&&
				serializer->write(EventField::TargetPid, nPid)*/);
		},
		[nTid, nAuthId](auto hash)
		{
			hash->update({ nTid });
			hash->update({ nAuthId });
		});
}

#define ThreadImpersonationToken 5

//
// advapi32.dll::ImpersonateLoggedOnUser -> kernelbase.dll::NtSetInformationThread
// advapi32.dll::SetThreadToken -> kernelbase.dll::NtSetInformationThread
//
REGISTER_PRE_CALLBACK(NtSetInformationThread, [](HANDLE ThreadHandle, 
	THREADINFOCLASS ThreadInformationClass,	PVOID ThreadInformation, ULONG ThreadInformationLength)
{
	// Do not track remote threads
	if (::GetThreadId(ThreadHandle) != ::GetCurrentThreadId())
		return;

	if (ThreadInformationClass == ThreadImpersonationToken && 
		ThreadInformation != NULL && ThreadInformationLength >= sizeof(HANDLE))
		sendImpersonationEvent(RawEvent::PROCMON_THREAD_IMPERSONATION, ThreadHandle, *(HANDLE*)ThreadInformation);
});

//
//
//
REGISTER_POST_CALLBACK(ImpersonateNamedPipeClient, [](BOOL fResult, HANDLE hNamedPipe)
{
	ScopedHandle hToken;
	if (fResult && ::OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken))
		sendImpersonationEvent(RawEvent::PROCMON_PIPE_IMPERSONATION, GetCurrentThread(), hToken);
});

} // namespace edrpm
} // namespace openEdr