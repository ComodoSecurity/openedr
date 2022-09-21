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

#pragma comment(lib,"Version.lib")

namespace cmd {
namespace sys {
namespace win {

	CommandLineToArgvWFunction pCommandLineToArgvW = NULL;

	CMDLINE_ENTRY g_CMDLineTable[] = {
	{ L"python.exe", EXCLUDE_NO_FILE, pythonParseEmbeddedScript, genericIsScriptAFile, pythonParseCmdLine },
	{ L"pythonw.exe", EXCLUDE_NO_FILE, pythonParseEmbeddedScript, genericIsScriptAFile, pythonParseCmdLine },
	{ L"cmd.exe", EXCLUDE_EXE_FILE | EXCLUDE_MSI_FILE, cmdParseEmbeddedScript, cmdIsScriptAFile, cmdParseCmdLine },
	{ L"powershell.exe", EXCLUDE_EXE_FILE, powerShellParseEmbeddedScript, powerShellIsScriptAFile, powershellParseCmdLine },
	{ NULL, EXCLUDE_NO_FILE, NULL, NULL, genericParseCommandline }
	};

	CMD_PATH_CONVERSION PathConversion[2];
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

bool enableAllPrivileges()
{
	ScopedHandle tokenHandle;
	if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tokenHandle))
		return false;
	
	//
	// Following privileges is default disabled for service,
	// skip SE_BACKUP_NAME / SE_RESTORE_NAME and SE_TAKE_OWNERSHIP_NAME
	//
	constexpr LPCTSTR privileges[] =
	{
		SE_ASSIGNPRIMARYTOKEN_NAME,
		SE_MANAGE_VOLUME_NAME,
		SE_ASSIGNPRIMARYTOKEN_NAME,
		SE_INCREASE_QUOTA_NAME,
		SE_LOAD_DRIVER_NAME,
		SE_MANAGE_VOLUME_NAME,
		SE_SECURITY_NAME,
		SE_SHUTDOWN_NAME,
		SE_SYSTEM_ENVIRONMENT_NAME,
		SE_SYSTEMTIME_NAME,
		SE_UNDOCK_NAME
	};

	bool result = false;
	for (auto& privelege : privileges)
	{
		if (!setPrivilege(tokenHandle, privelege, true))
		{
			SetLastError(ERROR_NOT_ALL_ASSIGNED);
		}
		else
		{
			result = true;
		}
	}

	return result;
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

NTSTATUS getProcessBasicInformation(HANDLE ProcessHandle, PPROCESS_BASIC_INFORMATION BasicInformation)
{
	return NtQueryInformationProcess(
		ProcessHandle,
		ProcessBasicInformation,
		BasicInformation,
		sizeof(PROCESS_BASIC_INFORMATION),
		NULL
	);
}

NTSTATUS getProcessPeb32(HANDLE ProcessHandle, PVOID* Peb32)
{
	NTSTATUS status;
	ULONG_PTR wow64;

	status = NtQueryInformationProcess(
		ProcessHandle,
		ProcessWow64Information,
		&wow64,
		sizeof(ULONG_PTR),
		NULL
	);

	if (NT_SUCCESS(status))
	{
		*Peb32 = (PVOID)wow64;
	}

	return status;
}

NTSTATUS getProcessPebString(HANDLE ProcessHandle, int Offset, std::wstring &String)
{
	NTSTATUS status = STATUS_SUCCESS;
	ULONG offset = 0;

#define PEB_OFFSET_CASE(Enum, Field) \
    case Enum: offset = FIELD_OFFSET(EDR_RTL_USER_PROCESS_PARAMETERS, Field); break; \
    case Enum | PEBWow64: offset = FIELD_OFFSET(EDR_RTL_USER_PROCESS_PARAMETERS32, Field); break

	switch (Offset)
	{
		PEB_OFFSET_CASE(PEBCurrentDirectory, CurrentDirectory);
		PEB_OFFSET_CASE(PEBDllPath, DllPath);
		PEB_OFFSET_CASE(PEBImagePathName, ImagePathName);
		PEB_OFFSET_CASE(PEBCommandLine, CommandLine);
		PEB_OFFSET_CASE(PEBWindowTitle, WindowTitle);
		PEB_OFFSET_CASE(PEBDesktopInfo, DesktopInfo);
		PEB_OFFSET_CASE(PEBShellInfo, ShellInfo);
		PEB_OFFSET_CASE(PEBRuntimeData, RuntimeData);
	default:
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!(Offset & PEBWow64))
	{
		PROCESS_BASIC_INFORMATION basicInfo;
		PVOID processParameters;
		UNICODE_STRING unicodeString;

		// Get the PEB address.
		if (!NT_SUCCESS(status = getProcessBasicInformation(ProcessHandle, &basicInfo)))
			return status;

		// Read the address of the process parameters.
		if (!::ReadProcessMemory(
			ProcessHandle,
			PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessParameters)),
			&processParameters,
			sizeof(PVOID),
			NULL))
		{
			return STATUS_UNSUCCESSFUL;
		}

		// Read the string structure.
		if (!::ReadProcessMemory(
			ProcessHandle,
			PTR_ADD_OFFSET(processParameters, offset),
			&unicodeString,
			sizeof(UNICODE_STRING),
			NULL))
		{
			return STATUS_UNSUCCESSFUL;
		}

		if (unicodeString.Length == 0)
		{
			return STATUS_UNSUCCESSFUL;
		}

		String.resize(unicodeString.Length);

		// Read the string contents.
		if (!::ReadProcessMemory(
			ProcessHandle,
			unicodeString.Buffer,
			String.data(),
			String.size(),
			NULL
		))
		{
			return STATUS_UNSUCCESSFUL;
		}
	}
	else
	{
		PVOID peb32;
		ULONG processParameters32;
		UNICODE_STRING32 unicodeString32;

		if (!NT_SUCCESS(status = getProcessPeb32(ProcessHandle, &peb32)))
			return STATUS_UNSUCCESSFUL;

		if (!::ReadProcessMemory(
			ProcessHandle,
			PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, ProcessParameters)),
			&processParameters32,
			sizeof(ULONG),
			NULL
		))
		{
			return STATUS_UNSUCCESSFUL;
		}

		if (!::ReadProcessMemory(
			ProcessHandle,
			PTR_ADD_OFFSET(processParameters32, offset),
			&unicodeString32,
			sizeof(UNICODE_STRING32),
			NULL
		))
		{
			return STATUS_UNSUCCESSFUL;
		}

		if (unicodeString32.Length == 0)
		{
			return status;
		}

		String.resize(unicodeString32.Length);

		// Read the string contents.
		if (!::ReadProcessMemory(
			ProcessHandle,
			UlongToPtr(unicodeString32.Buffer),
			String.data(),
			String.size(),
			NULL
		))
		{
			return STATUS_UNSUCCESSFUL;
		}
	}

	return status;
}

NTSTATUS getProcessIsWow64(HANDLE hProcess, PBOOLEAN isWow64)
{
	NTSTATUS status;
	ULONG_PTR wow64;

	status = NtQueryInformationProcess(
		hProcess,
		ProcessWow64Information,
		&wow64,
		sizeof(ULONG_PTR),
		NULL
	);

	if (NT_SUCCESS(status))
	{
		*isWow64 = !!wow64;
	}

	return status;
}

NTSTATUS getProcessCurrentDirectory(HANDLE hProcess, std::wstring& currentDirectory)
{
	NTSTATUS status;

#ifdef _WIN64
	BOOLEAN isWow64 = FALSE;
	getProcessIsWow64(hProcess, &isWow64);
	status = getProcessPebString(hProcess, (cmd::sys::win::EDR_PEB_OFFSET) (PEBCurrentDirectory | (isWow64 ? PEBWow64 : 0)), currentDirectory);
#else
	status = getProcessPebString(hProcess, PEBCurrentDirectory, currentDirectory);
#endif

	return status;
}


NTSTATUS getProcessImageFullPath(HANDLE hProcess, std::wstring& imageFullPath)
{
	NTSTATUS status;

#ifdef _WIN64
	BOOLEAN isWow64 = FALSE;
	getProcessIsWow64(hProcess, &isWow64);
	status = getProcessPebString(hProcess, (cmd::sys::win::EDR_PEB_OFFSET)(PEBImagePathName | (isWow64 ? PEBWow64 : 0)), imageFullPath);
#else
	status = getProcessPebString(hProcess, PEBImagePathName, imageFullPath);
#endif

	return status;
}

VOID TrimLeft(IN OUT LPWSTR lpszString, IN LPCWSTR lpszTarget)
{
	LPWSTR lpTempString = NULL;
	LPWSTR lpValidString = NULL;
	ULONG ulInvalidCharCount = 0;

	if (lpszString != NULL)
	{
		lpTempString = lpszString;
		while (*lpTempString)
		{
			if (wcschr(lpszTarget, (*lpTempString)) != NULL)
			{
				++ulInvalidCharCount;
			}
			else
			{
				break;
			}

			++lpTempString;
		}

		if (ulInvalidCharCount > 0)
		{
			lpTempString = lpszString;
			lpValidString = lpszString + ulInvalidCharCount;
			while (*lpValidString)
			{
				(*lpTempString) = (*lpValidString);

				++lpTempString;
				++lpValidString;
			}

			(*lpTempString) = L'\0';
		}
	}
}

VOID TrimRight(IN OUT LPWSTR lpszString, IN LPCWSTR lpszPattern)
{
	LPWSTR lpTempString = NULL;
	ULONG ulStringLength = 0;

	if (lpszString != NULL)
	{
		ulStringLength = (ULONG)wcslen(lpszString);
		while (ulStringLength > 0)
		{
			lpTempString = lpszString + ulStringLength - 1;
			if (wcschr(lpszPattern, (*lpTempString)) != NULL)
			{
				(*lpTempString) = L'\0';
			}
			else
			{
				break;
			}

			--ulStringLength;
		}
	}
}

DWORD getScriptLength(LPWSTR lpScript)
{
	DWORD i = 0;
	while (TRUE)
	{
		if (lpScript[i] == 0 && lpScript[i + 1] == 0)
			break;
		i++;
	}

	return i;
}

DWORD GetFullPath(LPCWSTR lpCurrentDirectory, LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer)
{
	DWORD nIndex = 0;

	do
	{
		if (lpFileName == NULL || (nBufferLength < _CMD_MAX_PATH - 1))
		{
			break;
		}

		if (lpCurrentDirectory)
		{
			size_t nLength = wcslen(lpCurrentDirectory) + wcslen(lpFileName) + 2;

			LPWSTR lpszFullPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, nLength * sizeof(WCHAR));
			if (lpszFullPath == NULL)
			{
				break;
			}

			wcsncpy_s(lpszFullPath + nIndex, nLength, lpCurrentDirectory, _TRUNCATE);
			nIndex += (DWORD)wcslen(lpCurrentDirectory);

			if (lpCurrentDirectory[wcslen(lpCurrentDirectory) - 1] != L'\\')
			{
				wcsncpy_s(lpszFullPath + nIndex, (nLength - nIndex), L"\\", _TRUNCATE);
				nIndex += _countof(L"\\") - 1;
			}

			wcsncpy_s(lpszFullPath + nIndex, (nLength - nIndex), lpFileName, _TRUNCATE);
			nIndex += (DWORD)wcslen(lpFileName);

			WCHAR wszSearchWorkDir[_CMD_MAX_PATH] = { 0 };
			nIndex = ::GetFullPathName(lpszFullPath, _CMD_MAX_PATH, wszSearchWorkDir, NULL);
			if (nIndex > 0 && (nIndex < _CMD_MAX_PATH - 1))
			{
				wcsncpy_s(lpBuffer, nBufferLength, wszSearchWorkDir, nIndex);
			}
			else
			{
				nIndex = 0;
			}

			LocalFree((HLOCAL)lpszFullPath);
		}
		else
		{
			WCHAR wszSearchWorkDir[_CMD_MAX_PATH] = { 0 };
			nIndex = ::GetFullPathName(lpFileName, _CMD_MAX_PATH - 1, wszSearchWorkDir, NULL);
			if (nIndex > 0 && (nIndex < _CMD_MAX_PATH - 1))
			{
				wcsncpy_s(lpBuffer, nBufferLength, wszSearchWorkDir, nIndex);
			}
			else
			{
				nIndex = 0;
			}
		}
	} while (FALSE);

	return nIndex;
}

DWORD searchPathExW(LPCWSTR lpCurrentDirectory, LPCWSTR lpFileName, LPCWSTR lpExtension, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR* lpFilePart)
{
	DWORD Length = 0;

	if (lpCurrentDirectory != NULL)
	{
		Length = SearchPathW(lpCurrentDirectory, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
	}

	if (Length == 0)
	{
		Length = SearchPathW(NULL, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart);
	}

	if (Length == 0)
	{
		Length = GetFullPath(lpCurrentDirectory, lpFileName, nBufferLength, lpBuffer);
	}

	if (Length > 0)
	{
		DWORD dwAttr = GetFileAttributes(lpBuffer);
		if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			Length = 0;
			lpBuffer[0] = 0;
		}
	}

	return Length;
}

BOOL genericSearchPath(IN LPCWSTR lpCurrentDirectory, IN LPCWSTR lpFileName, IN DWORD nBufferLength, OUT LPWSTR lpBuffer)
{
	BOOL bResult = FALSE;
	DWORD dwReturn = 0;

	do
	{
		dwReturn = searchPathExW(lpCurrentDirectory, lpFileName, NULL, nBufferLength, lpBuffer, NULL);
		if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
		{
			break;
		}

		bResult = TRUE;

	} while (FALSE);

	return bResult;
}

static BOOL isSingleScript(LPWSTR lpScript)
{
	int i = 0;
	while (lpScript[i] != 0)
		i++;

	return lpScript[i + 1] == 0;
}

BOOL genericIsScriptAFile(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath)
{
	if (isSingleScript(lpScript) && genericSearchPath(lpCurrentDirectory, lpScript, (_CMD_MAX_PATH - 1), lpFilePath))
	{
		return TRUE;
	}

	return FALSE;
}

static LPWSTR appendScript(LPWSTR lpSrc, LPWSTR lpScript)
{
	DWORD dwSrcLen = 0;
	if (lpSrc != NULL)
		dwSrcLen = getScriptLength(lpSrc) + 1;

	LPWSTR pNewScript = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (dwSrcLen + wcslen(lpScript) + 2) * sizeof(WCHAR));
	if (pNewScript == NULL)
		return lpSrc;

	ZeroMemory(pNewScript, (dwSrcLen + wcslen(lpScript) + 2) * sizeof(WCHAR));

	if (lpSrc != NULL)
	{
		memcpy(pNewScript, lpSrc, dwSrcLen * sizeof(WCHAR));
	}

	wcscpy_s(pNewScript + dwSrcLen, dwSrcLen + wcslen(lpScript) + 2, lpScript);

	if (lpSrc)
	{
		LocalFree(lpSrc);
	}

	return pNewScript;
}

BOOL CmdSearchPath(IN LPCWSTR lpCurrentDirectory, IN LPCWSTR lpFileName, IN DWORD nBufferLength, OUT LPWSTR lpBuffer)
{
	BOOL bResult = FALSE;
	DWORD dwReturn = 0;

	do
	{
		dwReturn = searchPathExW(lpCurrentDirectory, lpFileName, NULL, nBufferLength, lpBuffer, NULL);
		if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
		{
			dwReturn = searchPathExW(lpCurrentDirectory, lpFileName, L".exe", nBufferLength, lpBuffer, NULL);
			if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
			{
				dwReturn = searchPathExW(lpCurrentDirectory, lpFileName, L".bat", nBufferLength, lpBuffer, NULL);
				if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
				{
					dwReturn = searchPathExW(lpCurrentDirectory, lpFileName, L".cmd", nBufferLength, lpBuffer, NULL);
					if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
					{
						break;
					}
				}
			}
		}

		bResult = TRUE;

	} while (FALSE);

	return bResult;
}

BOOL cmdIsScriptAFile(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath)
{
	BOOL bResult = FALSE;
	LPWSTR* lpszArgList = NULL;
	int nArgCount = 0;

	if (!isSingleScript(lpScript))
	{
		return FALSE;
	}

	do
	{
		if (lpScript == NULL || lpScript[0] == 0)
		{
			break;
		}

		bResult = CmdSearchPath(lpCurrentDirectory, lpScript, (_CMD_MAX_PATH - 1), lpFilePath);
		if (bResult)
		{
			break;
		}

		if (pCommandLineToArgvW)
			lpszArgList = pCommandLineToArgvW(lpScript, &nArgCount);
		if (lpszArgList == NULL || nArgCount == 0)
		{
			break;
		}

		bResult = CmdSearchPath(lpCurrentDirectory, lpszArgList[0], (_CMD_MAX_PATH - 1), lpFilePath);

	} while (FALSE);

	if (lpszArgList != NULL)
	{
		LocalFree(lpszArgList);
		lpszArgList = NULL;
	}

	return bResult;
}

BOOL expandEnvironmentAndPathPatternMatch(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, IN pathPatternMatchFunction lpPathPatternMatchFun, OUT LPWSTR* lppScriptPath)
{
	UNREFERENCED_PARAMETER(lpFullPath);

	BOOL bResult = FALSE;
	DWORD dwReturn = 0;
	LPWSTR lpszExpandedCmdLineWithoutPath = NULL;
	LPWSTR lpszValidExpandedCmdLineWithoutPath = NULL;

	do
	{
		if (lpCmdLineWithoutPath == NULL || lppScriptPath == NULL || lpPathPatternMatchFun == NULL)
		{
			break;
		}

		lpszExpandedCmdLineWithoutPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (_CMD_MAX_PATH * sizeof(WCHAR)));
		if (lpszExpandedCmdLineWithoutPath == NULL)
		{
			break;
		}

		dwReturn = ExpandEnvironmentStringsW(lpCmdLineWithoutPath, lpszExpandedCmdLineWithoutPath, (_CMD_MAX_PATH - 1));
		if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
		{
			break;
		}

		lpszValidExpandedCmdLineWithoutPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (_CMD_MAX_PATH * sizeof(WCHAR)));
		if (lpszValidExpandedCmdLineWithoutPath == NULL)
		{
			break;
		}

		wcsncpy_s(lpszValidExpandedCmdLineWithoutPath, _CMD_MAX_PATH, lpszExpandedCmdLineWithoutPath, _TRUNCATE);

		TrimLeft(lpszValidExpandedCmdLineWithoutPath, L" \t");

		bResult = lpPathPatternMatchFun(lpszValidExpandedCmdLineWithoutPath, lpCurrentDirectory, lppScriptPath);

	} while (FALSE);

	if (lpszValidExpandedCmdLineWithoutPath != NULL)
	{
		LocalFree(lpszValidExpandedCmdLineWithoutPath);
		lpszValidExpandedCmdLineWithoutPath = NULL;
	}

	if (lpszExpandedCmdLineWithoutPath != NULL)
	{
		LocalFree(lpszExpandedCmdLineWithoutPath);
		lpszExpandedCmdLineWithoutPath = NULL;
	}

	return bResult;
}

BOOL pythonEmbeddedScriptPatternMatch(IN LPWSTR lpCmdLine, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	UNREFERENCED_PARAMETER(lpCurrentDirectory);

	BOOL bResult = FALSE;
	LPWSTR* lpszArgList = NULL;
	int nArgCount = 0;
	LPWSTR lpszArg = NULL;
	LPWSTR lpScriptPath = NULL;

	do
	{
		if (lpCmdLine == NULL || lppScriptPath == NULL)
		{
			break;
		}

		if (lpCmdLine[0] == L'\0')
		{
			break;
		}

		if (pCommandLineToArgvW)
			lpszArgList = pCommandLineToArgvW(lpCmdLine, &nArgCount);
		if (lpszArgList == NULL || nArgCount == 0)
		{
			break;
		}

		for (int i = 0; i < nArgCount; ++i)
		{
			lpszArg = lpszArgList[i];
			if (_wcsicmp(lpszArg, L"-c") == 0)
			{
				if (i < nArgCount - 1)
				{
					lpScriptPath = appendScript(lpScriptPath, lpszArgList[i + 1]);
					i++;
				}
			}
		}

	} while (FALSE);

	if (lpScriptPath)
	{
		*lppScriptPath = lpScriptPath;
		bResult = TRUE;
	}

	if (lpszArgList != NULL)
	{
		LocalFree(lpszArgList);
		lpszArgList = NULL;
	}

	return bResult;
}

BOOL pythonParseEmbeddedScript(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return expandEnvironmentAndPathPatternMatch(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, pythonEmbeddedScriptPatternMatch, lppScriptPath);
}

BOOL pythonParseCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return genericParseCommandline(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, lppScriptPath);
}

BOOL cmdEmbeddedScriptPatternMatch(IN LPWSTR lpCmdLine, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	UNREFERENCED_PARAMETER(lpCurrentDirectory);

	BOOL bResult = FALSE;
	LPWSTR lpScriptPath = NULL;

	do
	{
		if (lpCmdLine == NULL || lppScriptPath == NULL)
		{
			break;
		}

		if (lpCmdLine[0] == L'\0')
		{
			break;
		}

		WCHAR* p = NULL;
		if ((p = wcsstr(lpCmdLine, L"/c")) != NULL || (p = wcsstr(lpCmdLine, L"/C")) != NULL
			|| (p = wcsstr(lpCmdLine, L"/k")) != NULL || (p = wcsstr(lpCmdLine, L"/K")) != NULL)
		{
			p = p + 2;
			WCHAR* pp = wcschr(p, L'&');
			while (pp)
			{
				if (pp != p)
				{
					LPWSTR lpOneScript = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (ULONG_PTR)pp - (ULONG_PTR)p + sizeof(WCHAR));
					if (lpOneScript == NULL)
					{
						break;
					}

					ZeroMemory(lpOneScript, (ULONG_PTR)pp - (ULONG_PTR)p + sizeof(WCHAR));

					memcpy(lpOneScript, p, (ULONG_PTR)pp - (ULONG_PTR)p);

					TrimLeft(lpOneScript, L" \t\"");
					TrimRight(lpOneScript, L" \t\"");

					if (*lpOneScript != 0)
						lpScriptPath = appendScript(lpScriptPath, lpOneScript);
					LocalFree(lpOneScript);
				}

				p = pp + 1;
				pp = wcschr(p, L'&');
			}

			if (*p != 0)
			{
				LPWSTR lpOneScript = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, wcslen(p) * sizeof(WCHAR) + sizeof(WCHAR));
				if (lpOneScript == NULL)
				{
					break;
				}

				ZeroMemory(lpOneScript, wcslen(p) * sizeof(WCHAR) + sizeof(WCHAR));

				wcscpy_s(lpOneScript, wcslen(p) + 1, p);
				TrimLeft(lpOneScript, L" \t\"");
				TrimRight(lpOneScript, L" \t\"");
				if (*lpOneScript != 0)
					lpScriptPath = appendScript(lpScriptPath, lpOneScript);
				LocalFree(lpOneScript);
			}
		}

		if (lpScriptPath)
		{
			*lppScriptPath = lpScriptPath;
			bResult = TRUE;
		}

	} while (FALSE);

	return bResult;
}

BOOL cmdPathPatternMatch(IN LPWSTR lpCmdLine, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	BOOL bResult = FALSE;
	BOOL bFind = FALSE;
	LPWSTR* lpszArgList = NULL;
	int nArgCount = 0;
	LPWSTR lpszArg = NULL;
	LPWSTR lpStart = NULL;
	LPWSTR lpScriptPath = NULL;

	do
	{
		if (lpCmdLine == NULL || lppScriptPath == NULL)
		{
			break;
		}

		if (lpCmdLine[0] == L'\0')
		{
			break;
		}

		if (pCommandLineToArgvW)
			lpszArgList = pCommandLineToArgvW(lpCmdLine, &nArgCount);
		if (lpszArgList == NULL || nArgCount == 0)
		{
			break;
		}

		lpScriptPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (_CMD_MAX_PATH * sizeof(WCHAR)));
		if (lpScriptPath == NULL)
		{
			break;
		}

		for (int i = 0; i < nArgCount; ++i)
		{
			lpszArg = lpszArgList[i];

			if (((lpStart = wcsstr(lpszArg, L"/c")) == NULL) && ((lpStart = wcsstr(lpszArg, L"/C")) == NULL) &&
				((lpStart = wcsstr(lpszArg, L"/k")) == NULL) && ((lpStart = wcsstr(lpszArg, L"/K")) == NULL))
			{
				continue;
			}

			// Maybe option and path are connected together.
			lpszArg = lpStart + 2;
			if ((*lpszArg) != L'\0')
			{
				TrimLeft(lpszArg, L"\"");
				TrimRight(lpszArg, L" \t\"");

				if (CmdSearchPath(lpCurrentDirectory, lpszArg, (_CMD_MAX_PATH - 1), lpScriptPath))
				{
					bFind = TRUE;
					break;
				}
			}

			// Maybe option and path are separated.
			if (i < nArgCount - 1)
			{
				lpszArg = lpszArgList[i + 1];

				if (CmdSearchPath(lpCurrentDirectory, lpszArg, (_CMD_MAX_PATH - 1), lpScriptPath))
				{
					bFind = TRUE;
					break;
				}
			}

			break;
		}

		if (!bFind)
		{
			break;
		}

		*lppScriptPath = lpScriptPath;
		lpScriptPath = NULL;

		bResult = TRUE;

	} while (FALSE);

	if (lpScriptPath != NULL)
	{
		LocalFree(lpScriptPath);
		lpScriptPath = NULL;
	}

	if (lpszArgList != NULL)
	{
		LocalFree(lpszArgList);
		lpszArgList = NULL;
	}

	return bResult;
}

BOOL cmdParseEmbeddedScript(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return expandEnvironmentAndPathPatternMatch(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, cmdEmbeddedScriptPatternMatch, lppScriptPath);
}

BOOL cmdParseCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return expandEnvironmentAndPathPatternMatch(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, cmdPathPatternMatch, lppScriptPath);
}

BOOL powerShellEmbeddedScriptPatternMatch(IN LPWSTR lpCmdLine, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	UNREFERENCED_PARAMETER(lpCurrentDirectory);

	BOOL bResult = FALSE;
	LPWSTR lpScriptPath = NULL;
	LPWSTR* lpszArgList = NULL;
	int nArgCount = 0;

	do
	{
		if (lpCmdLine == NULL || lppScriptPath == NULL)
		{
			break;
		}

		if (lpCmdLine[0] == L'\0')
		{
			break;
		}

		BOOL bScript = FALSE;

		if (pCommandLineToArgvW)
			lpszArgList = pCommandLineToArgvW(lpCmdLine, &nArgCount);
		if (lpszArgList == NULL || nArgCount == 0)
		{
			break;
		}

		// all cases are embedded scripts except for "-NoExit -Command Set-Location -LiteralPath '%L'"
		int i = 0;
		for (; i < nArgCount; i++)
		{
			if (_wcsicmp(lpszArgList[i], L"-noexit") != 0
				&& _wcsicmp(lpszArgList[i], L"-command") != 0
				&& _wcsicmp(lpszArgList[i], L"/command") != 0
				&& _wcsicmp(lpszArgList[i], L"set-location") != 0
				&& _wcsicmp(lpszArgList[i], L"-literalpath") != 0)
			{
				TrimLeft(lpszArgList[i], L"\'");
				TrimRight(lpszArgList[i], L"\'");

				DWORD dwAttr = GetFileAttributes(lpszArgList[i]);
				if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
				{
					bScript = TRUE;
					break;
				}
			}
		}

		if (bScript)
		{
			WCHAR* nameEnd = NULL;
			WCHAR* nameStart = lpCmdLine;

			bool isCommand =
				_wcsnicmp(lpszArgList[0], L"/command", wcslen(lpszArgList[0])) == 0 ||
				_wcsnicmp(lpszArgList[0], L"-command", wcslen(lpszArgList[0])) == 0;

			bool isFile = !isCommand &&
				(_wcsicmp(lpszArgList[0], L"/file") == 0 || _wcsicmp(lpszArgList[0], L"-file") == 0);

			if (isCommand || isFile)
			{
				nameStart = wcsstr(lpCmdLine, lpszArgList[0]);
				if (nameStart)
				{
					nameStart += wcslen(lpszArgList[0]);

					if (isFile)
					{
						for (; *nameStart; ++nameStart)
						{
							if (!isspace(*nameStart))
								break;
						}

						if (*nameStart)
						{

							if (*nameStart == '\'' || *nameStart == '\"')
							{
								nameEnd = wcspbrk(++nameStart, L"\'\"");
							}
							else
							{
								nameEnd = wcspbrk(nameStart, L"\t ");
							}
						}
					}
					else
					{
						while (*nameStart == L'\"')
							nameStart++;
						while (*nameStart == L'\t' || *nameStart == L' ')
							nameStart++;
					}
				}
			}

			if (!nameStart)
				break;

			// if nameEnd is null, we use all remaining part of the string
			if (nameEnd == NULL)
				nameEnd = nameStart + wcslen(nameStart);

			const SIZE_T scriptPathLen = nameEnd - nameStart;
			if (scriptPathLen == 0)
				break;

			// script path should be double zero-terminated string
			lpScriptPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (scriptPathLen + 2) * sizeof(WCHAR));
			if (lpScriptPath)
			{
				memcpy(lpScriptPath, nameStart, scriptPathLen * sizeof(WCHAR));
				*lppScriptPath = lpScriptPath;
				bResult = TRUE;
			}
		}

	} while (FALSE);

	if (lpszArgList != NULL)
	{
		LocalFree(lpszArgList);
		lpszArgList = NULL;
	}

	return bResult;
}

BOOL powerShellIsScriptAFile(LPWSTR lpScript, LPCWSTR lpCurrentDirectory, LPWSTR lpFilePath)
{
	if (!isSingleScript(lpScript))
	{
		return FALSE;
	}

	LPWSTR pTmpScript = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (wcslen(lpScript) + 1) * sizeof(WCHAR));
	if (pTmpScript == NULL)
	{
		return FALSE;
	}

	ZeroMemory(pTmpScript, (wcslen(lpScript) + 1) * sizeof(WCHAR));

	LPWSTR pTmpScriptToFree = pTmpScript;

	wcscpy_s(pTmpScript, wcslen(lpScript) + 1, lpScript);

	TrimLeft(pTmpScript, L"{ \t\"&\'");
#define PS_RIGHTCLICK_COMMAND L"if((Get-ExecutionPolicy ) -ne \'AllSigned\') { Set-ExecutionPolicy -Scope Process Bypass };"
	if (_wcsnicmp(pTmpScript, PS_RIGHTCLICK_COMMAND, wcslen(PS_RIGHTCLICK_COMMAND)) == 0)
	{
		pTmpScript += wcslen(PS_RIGHTCLICK_COMMAND);
		TrimLeft(pTmpScript, L"{ \t\"&\'");
	}
	TrimRight(pTmpScript, L"} \t\"&\'");

	BOOL bRet = genericSearchPath(lpCurrentDirectory, pTmpScript, (_CMD_MAX_PATH - 1), lpFilePath);
	LocalFree(pTmpScriptToFree);

	return bRet;
}

BOOL powerShellParseEmbeddedScript(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return expandEnvironmentAndPathPatternMatch(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, powerShellEmbeddedScriptPatternMatch, lppScriptPath);
}

BOOL powershellParseCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return genericParseCommandline(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, lppScriptPath);
}

BOOL genericPathPatternMatch(IN LPWSTR lpCmdLine, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	BOOL bResult = FALSE;
	DWORD dwReturn = 0;
	BOOL bFind = FALSE;
	LPWSTR* lpszArgList = NULL;
	int nArgCount = 0;
	LPWSTR lpszArg = NULL;
	LPWSTR lpScriptPath = NULL;

	do
	{
		if (lpCmdLine == NULL || lppScriptPath == NULL)
		{
			break;
		}

		if (lpCmdLine[0] == L'\0')
		{
			break;
		}

		if (pCommandLineToArgvW)
			lpszArgList = pCommandLineToArgvW(lpCmdLine, &nArgCount);
		if (lpszArgList == NULL || nArgCount == 0)
		{
			break;
		}

		lpScriptPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (_CMD_MAX_PATH * sizeof(WCHAR)));
		if (lpScriptPath == NULL)
		{
			break;
		}

		for (int i = 0; i < nArgCount; ++i)
		{
			lpszArg = lpszArgList[i];

			dwReturn = searchPathExW(lpCurrentDirectory, lpszArg, NULL, (_CMD_MAX_PATH - 1), lpScriptPath, NULL);
			if (dwReturn == 0 || dwReturn > (_CMD_MAX_PATH - 1))
			{
				continue;
			}

			bFind = TRUE;
			break;
		}

		if (!bFind)
		{
			break;
		}

		*lppScriptPath = lpScriptPath;
		lpScriptPath = NULL;

		bResult = TRUE;

	} while (FALSE);

	if (lpScriptPath != NULL)
	{
		LocalFree(lpScriptPath);
		lpScriptPath = NULL;
	}

	if (lpszArgList != NULL)
	{
		LocalFree(lpszArgList);
		lpszArgList = NULL;
	}

	return bResult;
}

BOOL genericParseCommandline(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	return expandEnvironmentAndPathPatternMatch(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, genericPathPatternMatch, lppScriptPath);
}

BOOL getOriginalFileName(LPCWSTR lpFullPath, LPWSTR lpszValue, DWORD dwLength)
{
	BOOL bRet = FALSE;
	DWORD dwDummy = 0;
	DWORD dwSize = GetFileVersionInfoSizeW(lpFullPath, &dwDummy);
	if (dwSize == 0)
		return FALSE;

	LPBYTE pbyVIB = new (std::nothrow) BYTE[dwSize];
	if (pbyVIB == NULL)
		return FALSE;

	if (!GetFileVersionInfoW(lpFullPath, 0, dwSize, pbyVIB))
	{
		delete[] pbyVIB;
		return FALSE;
	}

	UINT   uLen = 0;
	LPVOID lpBuf = NULL;

	if (VerQueryValueW(pbyVIB, L"\\VarFileInfo\\Translation", (LPVOID*)&lpBuf, &uLen))
	{
		LPDWORD pTrans = (LPDWORD)lpBuf;

		for (UINT i = 0; i < uLen / sizeof(DWORD); i++)
		{
			TCHAR szSFI[MAX_PATH] = { 0 };
			::wsprintf(szSFI, L"\\StringFileInfo\\%04X%04X\\%s",
				LOWORD(pTrans[i]), HIWORD(pTrans[i]), L"OriginalFilename");

			LPTSTR lpszBuf = NULL;

			if (VerQueryValueW(pbyVIB, (LPTSTR)szSFI, (LPVOID*)&lpszBuf, &uLen) && lpszBuf != NULL)
			{
				_wcslwr_s(lpszBuf, uLen);
				WCHAR* p = wcsstr(lpszBuf, L".mui");
				if (p)
					*p = 0;
				if (lpszBuf[0] != 0)
				{
					wcsncpy_s(lpszValue, dwLength, lpszBuf, _TRUNCATE);
					bRet = TRUE;
					break;
				}
			}
		}
	}

	delete[] pbyVIB;
	return bRet;
}

CMDLINE_ENTRY* isProcessToWatch(LPWSTR szPath, DWORD* pParserFlags)
{
	CMDLINE_ENTRY* rtn = NULL;

	const WCHAR* name = wcsrchr(szPath, L'\\');
	const WCHAR* frwd = wcsrchr(szPath, L'/');

	name = __max(name, frwd);

	if (name == NULL)
		name = (WCHAR*)szPath;
	else
		name = name + 1;

	WCHAR lpAppName[MAX_PATH + 4] = L"X:\\"; // construct a fake path to match rules
	getOriginalFileName(szPath, lpAppName + 3, MAX_PATH);

	INT_PTR i = 0;
	// original name is different from file name!
	if (lpAppName[3] != 0 && _wcsicmp(name, lpAppName + 3) != 0)
	{
		for (i = 0; ; i++)
		{
			if (g_CMDLineTable[i].m_szName == NULL)
				break;

			if (_wcsnicmp(g_CMDLineTable[i].m_szName, lpAppName + 3, wcslen(g_CMDLineTable[i].m_szName)) == 0)
			{
				*pParserFlags = CMD_PARSER_FILE | CMD_PARSER_SCRIPT;
				rtn = &g_CMDLineTable[i];
				return rtn;
			}
		}

		if (g_CMDLineTable[i].m_szName == NULL)
		{
			rtn = &g_CMDLineTable[i];
			return rtn;
		}
	}

	for (i = 0; ; i++)
	{
		if (g_CMDLineTable[i].m_szName == NULL)
			break;

		if (_wcsnicmp(g_CMDLineTable[i].m_szName, name, wcslen(g_CMDLineTable[i].m_szName)) == 0)
		{
			*pParserFlags = CMD_PARSER_FILE | CMD_PARSER_SCRIPT;

			rtn = &g_CMDLineTable[i];

			break;
		}
	}

	if (g_CMDLineTable[i].m_szName == NULL)
	{
		rtn = &g_CMDLineTable[i];
	}

	return rtn;
}

BOOL isValidPEFile(LPWSTR strPathName)
{
	DWORD dwFileSize = 0;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hMMFile = NULL;
	LPVOID pMap = NULL;
	BOOL bRtn = TRUE;

	do 
	{
		hFile = CreateFileW(strPathName,
			GENERIC_READ,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			bRtn = FALSE;
			break;
		}

		dwFileSize = GetFileSize(hFile, NULL);

		if (dwFileSize < sizeof(IMAGE_DOS_HEADER) || dwFileSize == INVALID_FILE_SIZE)
		{
			bRtn = FALSE;
			break;
		}

		hMMFile = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMMFile == NULL) {
			bRtn = FALSE;
			break;
		}

		pMap = MapViewOfFile(hMMFile, FILE_MAP_READ, 0, 0, 0);
		if (pMap == NULL)
		{
			bRtn = FALSE;
			break;
		}

		PIMAGE_DOS_HEADER pMZ = (PIMAGE_DOS_HEADER)pMap;

		if ((pMZ->e_magic != 0x5A4D) &&
			(pMZ->e_magic != 0x4D5A))
		{
			bRtn = FALSE;
			break;
		}

		if (pMZ->e_lfanew + sizeof(IMAGE_NT_HEADERS32) > dwFileSize)
		{
			bRtn = FALSE;
			break;
		}

		PIMAGE_NT_HEADERS32 pNTHeader = (PIMAGE_NT_HEADERS32)((BYTE*)pMap + pMZ->e_lfanew);

		if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		{
			bRtn = FALSE;
			break;
		}

		if (pNTHeader->FileHeader.Characteristics & IMAGE_FILE_DLL)
		{
			bRtn = FALSE;
			break;
		}

		if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			PIMAGE_NT_HEADERS64 pNt64Header = (PIMAGE_NT_HEADERS64)pNTHeader;

			if (pNt64Header->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE)
			{
				bRtn = TRUE;
				break;
			}
		}
		else
		{
			if (pNTHeader->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_NATIVE)
			{
				bRtn = TRUE;
				break;
			}
		}
	} while (FALSE);

	if (pMap)
	{
		UnmapViewOfFile(pMap);
	}

	if (hMMFile)
	{
		CloseHandle(hMMFile);
	}

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}

	return bRtn;
}

BOOL isMSIFile(LPWSTR lpPath)
{
	WCHAR* pExtension = NULL;
	size_t pathLen = wcslen(lpPath);

	if (pathLen < 5)
	{
		return FALSE;
	}
	pExtension = lpPath + pathLen - 4;
	if (!pExtension)
	{
		return FALSE;
	}

	if ((_wcsicmp(pExtension, L".msi") == 0))
	{
		return TRUE;
	}

	return FALSE;
}

static BOOL shouldFileTypeBeSkipped(DWORD dwExcludeType, LPWSTR lpPath)
{
	if (lpPath == NULL || lpPath[0] == 0)
		return FALSE;

	if ((dwExcludeType & EXCLUDE_EXE_FILE) &&
		isValidPEFile(lpPath))
	{
		return TRUE;
	}
	else if ((dwExcludeType & EXCLUDE_MSI_FILE) &&
		isMSIFile(lpPath))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL resolveReparsePointTarget(
	_In_ LPCWSTR fileName,
	_Out_ LPWSTR targetFileName
)
{
	PREPARSE_DATA_BUFFER reparseBuffer = NULL;
	ULONG reparseLength = 0;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	DWORD lpBytesReturned = 0;
	BOOL bSuccess = FALSE;

	fileHandle = CreateFileW(fileName,
		FILE_READ_ATTRIBUTES | FILE_READ_DATA | SYNCHRONIZE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT,
		NULL);

	if (fileHandle != INVALID_HANDLE_VALUE)
	{

		reparseLength = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
		reparseBuffer = (PREPARSE_DATA_BUFFER)new (std::nothrow) BYTE[reparseLength];

		if (DeviceIoControl(
			fileHandle,
			FSCTL_GET_REPARSE_POINT,
			NULL,
			0,
			reparseBuffer,
			reparseLength,
			&lpBytesReturned,
			NULL
		))
		{
			if (
				IsReparseTagMicrosoft(reparseBuffer->ReparseTag) &&
				reparseBuffer->ReparseTag == IO_REPARSE_TAG_APPEXECLINK
				)
			{
				typedef struct _AppExecLinkReparseBuffer
				{
					ULONG StringCount;
					WCHAR StringList[1];
				} AppExecLinkReparseBuffer, * PAppExecLinkReparseBuffer;

				PAppExecLinkReparseBuffer appexeclink;
				PWSTR string;

				appexeclink = (PAppExecLinkReparseBuffer)reparseBuffer->GenericReparseBuffer.DataBuffer;
				string = (PWSTR)appexeclink->StringList;

				for (ULONG i = 0; i < appexeclink->StringCount; i++)
				{
					if (i == 2)
					{
						DWORD dwAttr = GetFileAttributes(string);
						if (dwAttr != INVALID_FILE_ATTRIBUTES && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) == 0)
						{
							wcsncpy_s(targetFileName, _CMD_MAX_PATH, string, _TRUNCATE);
							bSuccess = TRUE;
						}
						break;
					}

					string += wcslen(string) + 1;
				}
			}
		}
	}

	if (reparseBuffer)
	{
		delete[] reparseBuffer;
		reparseBuffer = NULL;
	}

	CloseHandle(fileHandle);

	return bSuccess;
}

//
//
//
BOOL IsWow64(HANDLE hProcessHandle, PBOOL pbWow64)
{
	typedef BOOL (WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

	BOOL bWow64 = TRUE;
	static LPFN_ISWOW64PROCESS fnIsWow64Process = NULL;
	if (fnIsWow64Process == NULL)
	{
		fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
	}
	if (fnIsWow64Process)
	{
		if (fnIsWow64Process(hProcessHandle, &bWow64) == FALSE)
			return FALSE;
	}
	else
		return FALSE;

	if (pbWow64)
		*pbWow64 = bWow64;

	return TRUE;
}

BOOL InitPathConversion()
{
	PathConversion[0].m_From[0] = PathConversion[1].m_From[0] = 0;

	WCHAR wszSystem[32];
	if (GetSystemDirectory(wszSystem, 32) == 0)
		return FALSE;

	WCHAR* p = wcsrchr(wszSystem, L'\\');
	if (p == NULL)
		return FALSE;

	*p = 0;
	wcscpy_s(PathConversion[0].m_From, wszSystem);
	wcscat_s(PathConversion[0].m_From, L"\\System32\\");
	wcscpy_s(PathConversion[0].m_To, wszSystem);
	wcscat_s(PathConversion[0].m_To, L"\\Syswow64\\");
	wcscpy_s(PathConversion[1].m_From, wszSystem);
	wcscat_s(PathConversion[1].m_From, L"\\sysnative\\");
	wcscpy_s(PathConversion[1].m_To, wszSystem);
	wcscat_s(PathConversion[1].m_To, L"\\Syswow64\\");

	return TRUE;
}

VOID fixFilePath(LPWSTR lpFullPath)
{
	if (lpFullPath == NULL)
		return;

	if (!InitPathConversion())
		return;

	WCHAR wszTmpPath[_CMD_MAX_PATH] = { 0 };
	for (int i = 0; i < sizeof(PathConversion) / sizeof(CMD_PATH_CONVERSION); i++)
	{
		if (wcslen(PathConversion[i].m_From) == 0)
			continue;

		if (_wcsnicmp(lpFullPath, PathConversion[i].m_From, wcslen(PathConversion[i].m_From)) == 0)
		{
			wcscpy_s(wszTmpPath, lpFullPath);
			wcscpy_s(lpFullPath, _CMD_MAX_PATH, PathConversion[i].m_To);
			wcscat_s(lpFullPath, _CMD_MAX_PATH, wszTmpPath + wcslen(PathConversion[i].m_From));

			break;
		}

	}
}

BOOL parseScriptPathInCmdLine(IN LPCWSTR lpFullPath, IN LPCWSTR lpCmdLineWithoutPath, IN LPCWSTR lpCurrentDirectory, OUT LPWSTR* lppScriptPath)
{
	BOOL rtn = FALSE;
	DWORD dwFlags = 0;
	LPWSTR lpScriptPath = NULL;

	CMDLINE_ENTRY* ParsCommandLineEntry = NULL;

	if (lpCmdLineWithoutPath == NULL || lpCmdLineWithoutPath[0] == 0 || lppScriptPath == NULL)
		return FALSE;

	ParsCommandLineEntry = isProcessToWatch((LPWSTR)lpFullPath, &dwFlags);

	if (!ParsCommandLineEntry)
		return FALSE;

	if (pCommandLineToArgvW == NULL)
	{
		WCHAR wszuser32[_CMD_MAX_PATH];
		GetWindowsDirectory(wszuser32, _CMD_MAX_PATH);
		wcscat_s(wszuser32, L"\\system32\\shell32.dll");
		HMODULE hModule = GetModuleHandle(L"shell32.dll");
		if (hModule == NULL)
			hModule = LoadLibraryExW(wszuser32, 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
		if (hModule)
			pCommandLineToArgvW = (CommandLineToArgvWFunction)GetProcAddress(hModule, "CommandLineToArgvW");
	}

	__try
	{
		DWORD ExcludeFileType = ParsCommandLineEntry->m_excludeExe;

		// get embedded scripts
		if (ParsCommandLineEntry->m_pParseEmbeddedFunction)
		{
			rtn = ParsCommandLineEntry->m_pParseEmbeddedFunction(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, &lpScriptPath);
		}

		if (rtn)
		{
			LPWSTR lpEmbeddedPath = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (_CMD_MAX_PATH * sizeof(WCHAR)));
			if (lpEmbeddedPath != NULL)
			{
				// if script is file name/path, just return it
				if ((dwFlags & CMD_PARSER_FILE) && ParsCommandLineEntry->m_pIsScriptAFile && ParsCommandLineEntry->m_pIsScriptAFile(lpScriptPath, lpCurrentDirectory, lpEmbeddedPath))
				{
					if (shouldFileTypeBeSkipped(ExcludeFileType, lpEmbeddedPath))
					{
						lpEmbeddedPath[0] = 0;
					}
				}
				else
				{
					lpEmbeddedPath[0] = 0;
				}
			}

			if (lpScriptPath)
			{
				LocalFree(lpScriptPath);
				lpScriptPath = NULL;
			}

			if (lpEmbeddedPath)
			{
				if (lpEmbeddedPath[0] != 0)
					lpScriptPath = lpEmbeddedPath;
				else
					LocalFree(lpEmbeddedPath);
			}
		}

		if ((dwFlags & CMD_PARSER_FILE) && lpScriptPath == NULL && ParsCommandLineEntry->m_pParseFunction)
		{
			rtn = ParsCommandLineEntry->m_pParseFunction(lpFullPath, lpCmdLineWithoutPath, lpCurrentDirectory, &lpScriptPath);
			if (rtn)
			{
				if (shouldFileTypeBeSkipped(ExcludeFileType, lpScriptPath))
				{
					LocalFree(lpScriptPath);
					return FALSE;
				}
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		lpScriptPath = NULL;
	}

	if (lpScriptPath)
	{
		LPWSTR lpLongName = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
			_CMD_MAX_PATH * sizeof(WCHAR) + 1);

		if (lpLongName)
		{
			INT_PTR ret = GetLongPathNameW(lpScriptPath,
				lpLongName,
				_CMD_MAX_PATH);

			if (ret == 0 || ret >= _CMD_MAX_PATH)
			{
				*lppScriptPath = lpScriptPath;
				LocalFree(lpLongName);
			}
			else
			{
				*lppScriptPath = lpLongName;
				LocalFree(lpScriptPath);
			}
		}
		else
			*lppScriptPath = lpScriptPath;
	}
	return lpScriptPath != NULL;
}

BOOL getFullPathFromCommandline(LPCWSTR lpCommandLine, LPWSTR  lpFullPath, LPWSTR* lppCommandline)
{
	LPCWSTR lpApplicationName = NULL;
	BOOL bRet = FALSE;

	if (lpCommandLine == NULL)
		return 0;

	do
	{
		ULONG Length = 0;
		PWCH TempNull = NULL;
		WCHAR TempChar = UNICODE_NULL;
		LPWSTR WhiteScan = NULL;
		BOOLEAN SearchRetry = TRUE;
		DWORD fileattr = 0;

		if (lppCommandline)
			*lppCommandline = NULL;

		lpApplicationName = lpCommandLine;
		TempNull = (PWCH)lpApplicationName;
		WhiteScan = (LPWSTR)lpApplicationName;

		if (*WhiteScan == L'\"')
		{
			SearchRetry = FALSE;
			WhiteScan++;
			lpApplicationName = WhiteScan;
			while (*WhiteScan)
			{
				if (*WhiteScan == (WCHAR)'\"')
				{
					TempNull = (PWCH)WhiteScan;
					break;
				}
				WhiteScan++;
				TempNull = (PWCH)WhiteScan;
			}
		}
		else
		{
		retrywsscan:
			lpApplicationName = lpCommandLine;
			while (*WhiteScan)
			{
				if (*WhiteScan == (WCHAR)' ' ||
					*WhiteScan == (WCHAR)'\t')
				{
					TempNull = (PWCH)WhiteScan;
					break;
				}
				WhiteScan++;
				TempNull = (PWCH)WhiteScan;
			}
		}

		if (lppCommandline && (TempNull + 1))
			*lppCommandline = TempNull + 1;

		TempChar = *TempNull;
		*TempNull = UNICODE_NULL;

		Length = SearchPathW(
			NULL,
			lpApplicationName,
			(PWSTR)L".exe",
			_CMD_MAX_PATH,
			lpFullPath,
			NULL
		) * 2;

		if (Length != 0 && Length < _CMD_MAX_PATH * sizeof(WCHAR))
		{
			fileattr = GetFileAttributesW(lpFullPath);
			if (((fileattr != 0xffffffff) &&
				(fileattr & FILE_ATTRIBUTE_DIRECTORY)) ||
				(fileattr == 0xffffffff))
			{
				Length = 0;
			}
			else
			{
				Length++;
				Length++;
			}
		}

		if (!Length || Length >= _CMD_MAX_PATH << 1)
		{
			*TempNull = TempChar;

			if (*WhiteScan && SearchRetry)
			{
				WhiteScan++;
				TempNull = WhiteScan;
				goto retrywsscan;
			}

			if (lppCommandline)
				*lppCommandline = NULL;

			bRet = FALSE;
			break;
		}

		*TempNull = TempChar;
		bRet = TRUE;
	} while (0);

	return bRet;
}

static int __inline Lower(int c)
{
	if ((c >= L'A') && (c <= L'Z'))
	{
		return(c + (L'a' - L'A'));
	}
	else
	{
		return(c);
	}
}

BOOL strPatternMatch(LPCTSTR pat, LPCTSTR str)
{
	const TCHAR* s = NULL;
	const TCHAR* p = NULL;
	BOOL star = FALSE;

loopStart:
	for (s = str, p = pat; *s; ++s, ++p) {
		switch (*p) {
		case L'?':
			if (*s == L'.') goto starCheck;
			break;
		case L'*':
			star = TRUE;
			str = s, pat = p;
			if (!*++pat) return TRUE;
			goto loopStart;
		default:
			if (Lower(*s) != Lower(*p))
				goto starCheck;
			break;
		}
	}
	if (*p == L'*') ++p;
	return (!*p);

starCheck:
	if (!star) return FALSE;
	str++;
	goto loopStart;
}

BOOL getFileFullPath(IN LPCWSTR szFile, OUT LPWSTR szFullPath)
{
	typedef ULONG(NTAPI* RTLGETFULLPATHNAME_U)(
		const WCHAR* name,
		ULONG size,
		WCHAR* buffer,
		WCHAR** file_part);

	static RTLGETFULLPATHNAME_U funcRtlGetFullPathName_U = (RTLGETFULLPATHNAME_U)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "RtlGetFullPathName_U");

	if (!funcRtlGetFullPathName_U)
		return FALSE;

	ULONG retLen = funcRtlGetFullPathName_U(szFile,
		_CMD_MAX_PATH * sizeof(WCHAR),
		szFullPath,
		NULL);
	if (retLen == 0)
		return FALSE;
	szFullPath[_CMD_MAX_PATH - 1] = 0;
	return TRUE;
}

BOOL getScriptFullPathWithCommandline(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPWSTR  lpFullPath, LPWSTR* lppCommandline)
{
	BOOL bRet = FALSE;

	if (lpFullPath == NULL)
		return FALSE;

	if (lpCommandLine == NULL && lpApplicationName == NULL)
		return FALSE;

	if (lpApplicationName == NULL)
	{
		bRet = getFullPathFromCommandline(lpCommandLine, lpFullPath, lppCommandline);

		if (bRet && lppCommandline)
		{
			LPWSTR WhiteScan = *lppCommandline;

			while (*WhiteScan)
			{
				if ((*WhiteScan != (WCHAR)' ') &&
					(*WhiteScan != (WCHAR)'\t'))
					break;

				WhiteScan++;
			}

			*lppCommandline = WhiteScan;
		}

		return bRet;
	}

	if (lppCommandline)
		*lppCommandline = NULL;

	bRet = getFileFullPath(lpApplicationName, lpFullPath);

	if (bRet && lpCommandLine && lppCommandline)
	{
		LPWSTR lpTemp = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
			(_CMD_MAX_PATH + 1)* sizeof(WCHAR));

		if (lpTemp)
		{
			LPWSTR pCommandLine = NULL;

			if (getFullPathFromCommandline(lpCommandLine, lpTemp, &pCommandLine))
			{
				WCHAR sSystemDir[_MAX_PATH] = { 0 };
				GetWindowsDirectory(sSystemDir, _MAX_PATH);
				wcscat_s(sSystemDir, L"\\syswow64\\");

				if (_wcsnicmp(sSystemDir, lpFullPath, wcslen(sSystemDir)) == 0)
				{
					fixFilePath(lpTemp);
				}

				if (_wcsnicmp(lpTemp, lpFullPath, _CMD_MAX_PATH) == 0)
				{
					*lppCommandline = pCommandLine;
				}
				else
				{
					*lppCommandline = lpCommandLine;
				}
			}
			else
				*lppCommandline = lpCommandLine;


			LPWSTR WhiteScan = *lppCommandline;

			while (*WhiteScan)
			{
				if ((*WhiteScan != (WCHAR)' ') &&
					(*WhiteScan != (WCHAR)'\t'))
					break;

				WhiteScan++;
			}

			*lppCommandline = WhiteScan;

			LocalFree(lpTemp);
		}

		return TRUE;
	}

	return bRet;
}

INT_PTR getFullScriptPath(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPCWSTR lpCurrentDirectory, std::wstring& scriptFullPath)
{
	INT_PTR rtn = 0;
	LPWSTR lpCmdLine = NULL;
	LPWSTR lpCommandLineCopy = NULL;
	WCHAR lpFullPath[_CMD_MAX_PATH] = {0};
	BOOL bCmdLineAllocated = FALSE;

	if (lpCommandLine)
		lpCommandLineCopy = (LPWSTR)LocalAlloc(LMEM_ZEROINIT,
			_CMD_MAX_PATH * sizeof(WCHAR));

	if (lpCommandLineCopy)
		wcsncpy_s(lpCommandLineCopy, _CMD_MAX_PATH, lpCommandLine, _TRUNCATE);

	rtn = getScriptFullPathWithCommandline(lpApplicationName,
		lpCommandLineCopy ? lpCommandLineCopy : lpCommandLine,
		lpFullPath,
		&lpCmdLine);

	if (rtn)
	{
		if (strPatternMatch(L"*.bat", lpFullPath) ||
			strPatternMatch(L"*.cmd", lpFullPath))
		{
			if (!isValidPEFile(lpFullPath))
			{
				LPCWSTR SourcePath = lpCommandLine != NULL ? lpCommandLine : lpFullPath;
				size_t CmdLineBufferLength = wcslen(SourcePath) + 16;
				lpCmdLine = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, CmdLineBufferLength * sizeof(WCHAR));
				if (lpCmdLine)
				{
					bCmdLineAllocated = TRUE;
					wcscpy_s(lpCmdLine, CmdLineBufferLength, L"/c ");
					wcscat_s(lpCmdLine, CmdLineBufferLength, SourcePath);
					GetSystemDirectory(lpFullPath, _CMD_MAX_PATH);
					wcscat_s(lpFullPath, _CMD_MAX_PATH, L"\\cmd.exe");
				}
			}
		}
	}

	if (rtn && lpCmdLine)
	{
		LPWSTR lpCmdLinePath = NULL;

		if (lpCmdLine != NULL && parseScriptPathInCmdLine(lpFullPath, lpCmdLine, lpCurrentDirectory, &lpCmdLinePath))
		{
			rtn = 2;
			wcsncpy_s(lpFullPath, _CMD_MAX_PATH, lpCmdLinePath, _TRUNCATE);
			LocalFree(lpCmdLinePath);
		}
	}

	DWORD fileattr = GetFileAttributesW(lpFullPath);
	if (fileattr & FILE_ATTRIBUTE_REPARSE_POINT)
	{
		WCHAR targetPath[_CMD_MAX_PATH] = { 0 };
		if (resolveReparsePointTarget(lpFullPath, targetPath))
		{
			wcsncpy_s(lpFullPath, _CMD_MAX_PATH, targetPath, _TRUNCATE);
		}
	}

	if (rtn == 2)
	{
		scriptFullPath.append(lpFullPath);
	}
	else
	{
		scriptFullPath = L"";
	}

	if (bCmdLineAllocated)
		LocalFree(lpCmdLine);

	if (lpCommandLineCopy)
		LocalFree(lpCommandLineCopy);

	return rtn;
}

} // namespace win
} // namespace sys
} // namespace cmd
