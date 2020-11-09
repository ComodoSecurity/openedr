//
// edrav2.libsyswin project
//
// Author: Denis Bogdanov (07.02.2019)
// Reviewer: Denis Bogdanov (22.02.2019)
//
///
/// @file Common auxillary functions for interaction with OS Windows API 
/// (implementation)
///
#include "pch.h"
#include "libsyswin.h"

namespace openEdr {
namespace sys {
namespace win {

//
//
//
std::vector<uint8_t> getTokenInformation(HANDLE hToken, TOKEN_INFORMATION_CLASS eTokenType)
{
	DWORD nSize = 0;
	::GetTokenInformation(hToken, eTokenType, nullptr, nSize, &nSize);
	auto ec = GetLastError();
	if (ec != ERROR_INSUFFICIENT_BUFFER && ec != ERROR_BAD_LENGTH)
		error::win::WinApiError(SL, ec, "Fail to get token information size").throwException();
	std::vector<uint8_t> pBuffer(nSize);
	if (!::GetTokenInformation(hToken, eTokenType, pBuffer.data(), nSize, &nSize))
		error::win::WinApiError(SL, "Fail to get token information").throwException();
	return pBuffer;
}

//
//
//
std::vector<uint8_t> getProcessInformation(HANDLE hProcess, TOKEN_INFORMATION_CLASS eTokenType)
{
	ScopedHandle hToken;
	if (!::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken))
		error::win::WinApiError(SL, "Fail to get process token").throwException();
	return getTokenInformation(hToken, eTokenType);
}

//
//
//
bool isProcessElevated(HANDLE hProcess, bool defValue)
{
	CMD_TRY
	{
		auto pInfo = getProcessInformation(hProcess, TokenElevation);
		return PTOKEN_ELEVATION(pInfo.data())->TokenIsElevated != 0;
	}
	CMD_CATCH("Fail to get elevation status of process")
	return defValue;
}

//
//
//
Enum getProcessElevationType(HANDLE hProcess, Enum defValue)
{
	CMD_TRY
	{
		auto pInfo = getProcessInformation(hProcess, TokenElevationType);
		return *PTOKEN_ELEVATION_TYPE(pInfo.data());
	}
	CMD_CATCH("Fail to get elevation type of process")
	return defValue;
}

//
//
//
std::string getProcessSid(HANDLE hProcess, const std::string& defValue)
{
	CMD_TRY
	{
		auto pInfo = getProcessInformation(hProcess, TokenUser);
		ScopedLocalMem sTmp;
		if (!::ConvertSidToStringSidA(PTOKEN_USER(pInfo.data())->User.Sid, (LPSTR*)& sTmp))
			error::win::WinApiError(SL, "Fail to convert SID to string").throwException();
		return (char*)sTmp.get();
	}
	CMD_CATCH("Fail to get process user's SID")
	return defValue;
}

//
//
//
IntegrityLevel getProcessIntegrityLevel(HANDLE hProcess, IntegrityLevel defValue)
{
	CMD_TRY
	{
		auto pInfo = getProcessInformation(hProcess, TokenIntegrityLevel);
		ScopedLocalMem sTmp;
		if (!::ConvertSidToStringSidA(PTOKEN_USER(pInfo.data())->User.Sid, (LPSTR*)& sTmp))
			error::win::WinApiError(SL, "Fail to convert SID to string").throwException();

		PTOKEN_MANDATORY_LABEL pTIL = PTOKEN_MANDATORY_LABEL(pInfo.data());
		auto dwIntegrityLevel = *::GetSidSubAuthority(pTIL->Label.Sid, (DWORD)(UCHAR)(*::GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));

		if (dwIntegrityLevel >= SECURITY_MANDATORY_PROTECTED_PROCESS_RID)
			return IntegrityLevel::Protected;
		if (dwIntegrityLevel >= SECURITY_MANDATORY_SYSTEM_RID)
			return IntegrityLevel::System;
		if (dwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)
			return IntegrityLevel::High;
		if (dwIntegrityLevel >= SECURITY_MANDATORY_MEDIUM_RID)
			return IntegrityLevel::Medium;
		if (dwIntegrityLevel >= SECURITY_MANDATORY_LOW_RID)
			return IntegrityLevel::Low;
		return IntegrityLevel::Untrusted;
	}
	CMD_CATCH("Fail to get process user's SID")
	return defValue;
}

//
//
//
HANDLE getFileHandle(const std::wstring& sPathName, uint32_t nAccess)
{
	HANDLE hFile = ::CreateFileW(sPathName.c_str(),
		nAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		UNICODE_STRING upn;
		RtlInitUnicodeString(&upn, sPathName.c_str());
		OBJECT_ATTRIBUTES oatr;
		InitializeObjectAttributes(&oatr, &upn, OBJ_CASE_INSENSITIVE, 0, 0);

		HANDLE hFile2;
		IO_STATUS_BLOCK State = {};
		auto status = ::NtCreateFile(&hFile2, nAccess, &oatr, &State, 0,
			FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN, 0, NULL, 0);
		if (NT_SUCCESS(status))
			// NtCreateFile() return handle = 0 if fail. We can't use ScopedFileHandle directly
			hFile = hFile2;
	}
	return hFile;
}

// struct which was copiedfrom windows.h causes warning 
#pragma warning(push)
#pragma warning(disable:4201)
typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION {
	SIZE_T Size;    // Ignored as input, written with structure size on output
	PROCESS_BASIC_INFORMATION BasicInfo;
	union {
		ULONG Flags;
		struct {
			ULONG IsProtectedProcess : 1;
			ULONG IsWow64Process : 1;
			ULONG IsProcessDeleting : 1;
			ULONG IsCrossSessionCreate : 1;
			ULONG IsFrozen : 1;
			ULONG IsBackground : 1;
			ULONG IsStronglyNamed : 1;
			ULONG IsSecureProcess : 1;
			ULONG IsSubsystemProcess : 1;
			ULONG SpareBits : 23;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;
#pragma warning(pop)

//
//
//
bool isUwpApp(const HANDLE hProcess)
{
	PROCESS_EXTENDED_BASIC_INFORMATION extendedInfo = { sizeof(extendedInfo) };
	NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
		&extendedInfo, sizeof(extendedInfo), NULL);
	if (NT_SUCCESS(status))
		error::win::WinApiError(SL, "Fail to query ProcessBasicInformation").log();
	return (extendedInfo.Flags & 0xF0);
}

//
// Check if UWP process is suspended
// TODO: Add normal processes check
//
bool isSuspended(const HANDLE hProcess)
{
	// URL: https://stackoverflow.com/questions/47300622/meaning-of-flags-in-process-extended-basic-information-struct
	PROCESS_EXTENDED_BASIC_INFORMATION extendedInfo = { sizeof(extendedInfo) };
	NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessBasicInformation,
		&extendedInfo, sizeof(extendedInfo), NULL);
	if (!NT_SUCCESS(status))
		error::win::WinApiError(SL, "Fail to query ProcessBasicInformation").log();
	return extendedInfo.IsFrozen;
}

//
//
//
bool isCritical(const HANDLE hProcess)
{
	ULONG nCritical = 0;
	return (NT_SUCCESS(::NtQueryInformationProcess(hProcess, ProcessBreakOnTermination,
		&nCritical, sizeof(ULONG), NULL)) && nCritical);
}

//
//
//
bool setPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege)
{
	LUID luid;
	if (!::LookupPrivilegeValue(NULL, Privilege, &luid))
		return false;

	// 
	// first pass.  get current privilege setting
	// 
	TOKEN_PRIVILEGES tp = { 0 };
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	TOKEN_PRIVILEGES tpPrevious;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
	::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious);
	if (GetLastError() != ERROR_SUCCESS)
		return false;

	// 
	// second pass.  set privilege based on previous setting
	// 
	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if (bEnablePrivilege)
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	else
		tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

	::AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL);
	if (GetLastError() != ERROR_SUCCESS)
		return false;

	return true;
}

//
//
//
std::wstring getKnownPath(const GUID& ePathId, const HANDLE hToken)
{
	constexpr auto eFlags = KF_FLAG_DONT_VERIFY | KF_FLAG_NO_ALIAS;
	PWSTR psPath = nullptr;
	HRESULT eStatus = ::SHGetKnownFolderPath(ePathId, 0, hToken, &psPath);
	if (eStatus != S_OK || psPath == nullptr)
		return {};

	std::wstring sResult = psPath;
	::CoTaskMemFree(psPath);
	return sResult;
}

//
//
//
HANDLE searchProcessBySid(const std::string& sSid, const bool fElevated)
{
	if (sSid.empty())
		return NULL;

	// Create toolhelp snapshot.
	sys::win::ScopedFileHandle snapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
	if (!snapshot)
	{
		error::win::WinApiError(SL, "Can't create processes snapshot").log();
		return NULL;
	}

	ScopedHandle hProcessFallback;

	// Walk through all processes.
	PROCESSENTRY32W process = {};
	process.dwSize = sizeof(process);
	if (Process32FirstW(snapshot, &process))
	{
		do
		{
			CMD_TRY
			{
				if (process.th32ProcessID == 0)
					continue;

				ScopedHandle hProcess(::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process.th32ProcessID));
				if (!hProcess)
					hProcess.reset(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process.th32ProcessID));
				if (!hProcess)
					continue;

				ScopedHandle hToken;
				if ((!::OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken) &&
					!::OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) ||
					getProcessSid(hProcess) != sSid)
					continue;

				if (isProcessElevated(hProcess) == fElevated)
					return hProcess.release();

				if (!hProcessFallback)
					hProcessFallback.reset(hProcess.release());
			}
			CMD_CATCH(FMT("Fail to get SID for process <" << process.th32ProcessID << ">"))
		} while (Process32Next(snapshot, &process));
	}

	return hProcessFallback ? hProcessFallback.release() : NULL;
}

#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

//
//
//
std::wstring getProcessImagePath(const HANDLE hProcess)
{
	uint8_t pBuffer[0x1000];
	std::vector<uint8_t> pDynBuffer;
	PUNICODE_STRING pStr = (PUNICODE_STRING)pBuffer;

	ULONG nBufferSize = 0;
	NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessImageFileName, pBuffer, sizeof(pBuffer), &nBufferSize);
	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		pDynBuffer.resize(nBufferSize);
		pStr = (PUNICODE_STRING)pDynBuffer.data();
		status = ::NtQueryInformationProcess(hProcess, ProcessImageFileName, pDynBuffer.data(), ULONG(pDynBuffer.size()), &nBufferSize);
	}
	if (status)
	{
		LOGWRN("Fail to query image path of process <" << ::GetProcessId(hProcess) << ">, status <" << std::to_string(status) << ">");
		return {};
	}

	if (pStr->Buffer == NULL)
		return (::GetProcessId(hProcess) == g_nSystemPid) ? L"System" : L"";

	return std::wstring(pStr->Buffer, pStr->Length / sizeof(wchar_t));
}

} // namespace win
} // namespace sys
} // namespace openEdr
