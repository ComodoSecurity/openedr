//
// edrav2.libcore project
//
// Author: Denis Bogdanov (16.04.2019)
// Reviewer: Denis Bogdanov (16.04.2019)
//
///
/// @file Common auxillary system-related functions
/// (implementation)
///
#include "pch_win.h"
#include "system.hpp"
#include "crypt.hpp"

#ifndef PLATFORM_WIN
#error "Please exclude this file from your project"
#endif

namespace cmd {
namespace sys {

namespace win {

//
//
//
void HandleTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::CloseHandle(rsrc))
		error::win::WinApiError(SL, "Can't close handle").log();
}

//
//
//
void FileHandleTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::CloseHandle(rsrc))
		error::win::WinApiError(SL, "Can't close handle").log();
}

//
//
//
void LibraryTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::FreeLibrary(rsrc))
		error::win::WinApiError(SL, "Can't unload library").log();
}

//
//
//
void ServiceHandleTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::CloseServiceHandle(rsrc))
		error::win::WinApiError(SL, "Can't CloseServiceHandle").log();
}

//
//
//
void LocalMemoryTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (::LocalFree(rsrc) != NULL)
		error::win::WinApiError(SL, "Can't free memory").log();
}

//
//
//
void RegKeyTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (::RegCloseKey(rsrc) != ERROR_SUCCESS)
		error::win::WinApiError(SL, "Can't close registry key").log();
}

//
//
//
void EnvBlockTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::DestroyEnvironmentBlock(rsrc))
		error::win::WinApiError(SL, "Can't destroy the environment block").log();
}

//
//
//
void ImpersonationTraits::cleanup(ResourceType&) noexcept
{
	if (!::SetThreadToken(0, 0))
		error::win::WinApiError(SL, "Can't restore impersonation token").log();
}

//
//
//
void FileVolumeTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::FindVolumeClose(rsrc))
		error::win::WinApiError(SL, "Can't close find volume").log();
}


//
//
//
Variant ReadRegistryValue(HKEY rootKey, const std::wstring& sKey, const std::wstring& sValue)
{
	TRACE_BEGIN;
	ScopedRegKey hKey;
	LONG lRes = RegOpenKeyExW(rootKey, sKey.c_str(), 0, KEY_READ, &hKey);
	if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't open registry key").throwException();

	// Get data type and size
	DWORD dwType = REG_NONE;
	DWORD dwDataSize = 0;
	lRes = RegQueryValueExW(hKey, sValue.c_str(), NULL, &dwType, NULL, &dwDataSize);
	if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't read registry value").throwException();
	
	switch (dwType)
	{
		case REG_BINARY:
		{ 
			auto pStream = io::createMemoryStream(dwDataSize);
			auto pBuffer = queryInterface<io::IMemoryBuffer>(pStream);
			lRes = RegQueryValueExW(hKey, sValue.c_str(), NULL, NULL, (LPBYTE)pBuffer->getData().first, &dwDataSize);
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't read registry value").throwException();
			return pStream;
		}
		case REG_DWORD:
		case REG_DWORD_BIG_ENDIAN:
		{
			DWORD dwData = 0;
			if (dwDataSize != sizeof(dwData)) error::InvalidFormat("Bad registry data format").throwException();
			lRes = RegQueryValueExW(hKey, sValue.c_str(), NULL, NULL, (LPBYTE)&dwData, &dwDataSize);
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't read registry value").throwException();
			return dwData;
		}
		case REG_EXPAND_SZ:
		case REG_SZ:
		case REG_LINK:
		{
			std::wstring sData(Size(dwDataSize / sizeof(wchar_t) - 1), 0);
			lRes = RegQueryValueExW(hKey, sValue.c_str(), NULL, NULL, (LPBYTE)sData.data(), &dwDataSize);
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't read registry value").throwException();
			return sData;
		}
		case REG_MULTI_SZ:
		{
			std::wstring sData(Size(dwDataSize / sizeof(wchar_t) - 1), 0);
			lRes = RegQueryValueExW(hKey, sValue.c_str(), NULL, NULL, (LPBYTE)sData.data(), &dwDataSize);
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't read registry value").throwException();

			std::vector<std::wstring> vecStr;
			boost::split(vecStr, sData, boost::algorithm::is_any_of("\0"));
			Sequence seqData;
			for (auto& s : vecStr) seqData.push_back(s);
			return seqData;
		}
		case REG_QWORD:
		{
			UINT64 qwData = 0;
			if (dwDataSize != sizeof(qwData)) error::InvalidFormat(SL, "Bad registry data format").throwException();
			lRes = RegQueryValueExW(hKey, sValue.c_str(), NULL, NULL, (LPBYTE)&qwData, &dwDataSize);
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't read registry value").throwException();
			return qwData;
		}
	}
	return {};
	TRACE_END("Error during reading registry value");
}

//
//
//
void WriteRegistryValue(HKEY rootKey, const std::wstring& sKey, const std::wstring& sValue, Variant vData)
{
	TRACE_BEGIN;
	ScopedRegKey hKey;
	LONG lRes = RegOpenKeyExW(rootKey, sKey.c_str(), 0, KEY_WRITE, &hKey);
	if (lRes != ERROR_SUCCESS)
	{
		if (lRes == ERROR_FILE_NOT_FOUND)
			lRes = RegCreateKeyExW(rootKey, sKey.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
		if (lRes != ERROR_SUCCESS)
			error::win::WinApiError(SL, lRes, "Can't open registry key").throwException();
	}

	switch (vData.getType())
	{
		case variant::ValueType::Boolean:
		{ 
			DWORD dwData = vData ? 1 : 0;
			lRes = RegSetValueExW(hKey, sValue.c_str(), NULL, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData));
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't write registry value").throwException();
			break;
		}
		case variant::ValueType::Integer:
		{
			Variant::IntValue ival = vData;
			ULONG64 qwData = ival.asUnsigned();
			lRes = RegSetValueExW(hKey, sValue.c_str(), NULL, REG_QWORD, (LPBYTE)&qwData, sizeof(qwData));
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't write registry value").throwException();
			break;
		}
		case variant::ValueType::String:
		{
			std::wstring sData = vData;
			lRes = RegSetValueExW(hKey, sValue.c_str(), NULL, REG_SZ, (LPBYTE)sData.c_str(), 
				DWORD((sData.size() + 1) * sizeof(wchar_t)));
			if (lRes != ERROR_SUCCESS) error::win::WinApiError(SL, lRes, "Can't write registry value").throwException();
			break;
		}
		default:
			error::InvalidArgument(SL, "Argument has unsupported data type").throwException();
	}
	TRACE_END("Error during writting registry value");
}

} // namespace win


//
//
//
uint32_t executeApplication(const std::filesystem::path& pathProgram,
	std::wstring_view sParams, bool fElevatePrivileges, Size nTimeout)
{
	if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		error::RuntimeError(SL, "Can't initialize COM").throwException();
	SHELLEXECUTEINFOW sinfo { };
	sinfo.cbSize = sizeof(SHELLEXECUTEINFOA);
	sinfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI | SEE_MASK_NO_CONSOLE;
	sinfo.lpVerb = fElevatePrivileges? L"runas" : L"open";
	sinfo.lpFile = pathProgram.c_str();
	sinfo.lpParameters = sParams.data();
	sinfo.nShow = SW_HIDE;

	if (!::ShellExecuteExW(&sinfo))
		error::win::WinApiError(SL, "Can't start process").throwException();	

	if (nTimeout == 0)
		return 0;

	if (sinfo.hProcess == NULL)
		error::win::WinApiError(SL, "Can't get started process").throwException();

	if (WAIT_FAILED == ::WaitForSingleObject(sinfo.hProcess, (DWORD)nTimeout))
		error::win::WinApiError(SL, "Can't wait started process").throwException();

	DWORD dwExitCode = 0;
	if (!GetExitCodeProcess(sinfo.hProcess, &dwExitCode))
		error::win::WinApiError(SL, "Can't get process exit code").throwException();
	return dwExitCode;
}

//
//
//
std::string getHardwareMachineId()
{
	std::string sResult;
	// Get a handle to physical drive
	//win::ScopedFileHandle hFile(::CreateFileW(L"\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
	//	NULL, OPEN_EXISTING, 0, NULL));
	if (sResult.empty())
		return sResult;
	auto hash = crypt::sha1::getHash(sResult);
	return string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
}

//
//
//
std::string getUniqueMachineId()
{
	auto szRes = getHardwareMachineId();
	if (!szRes.empty())
		return szRes;

	HKEY regKey;

#ifdef CPUTYPE_AMD64
	REGSAM regSam = KEY_READ;
#else
	REGSAM regSam = KEY_READ | KEY_WOW64_64KEY;
#endif
	
	auto dwError = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography",
		0, regSam, &regKey);
	if (dwError != ERROR_SUCCESS)
	{
		error::win::WinApiError(SL, dwError, "Can't open MachineGuid key").log();
		return {};
	}

	DWORD dwType = REG_NONE;
	wchar_t data[0x100]; *data = 0;
	DWORD dwSize = sizeof(data);
	dwError = ::RegQueryValueExW(regKey, L"MachineGuid", 0, &dwType, LPBYTE(data), &dwSize);
	if (dwError != ERROR_SUCCESS)
	{
		error::win::WinApiError(SL, dwError, "Can't read MachineGuid value").log();
		return {};
	}

	if (dwType != REG_SZ)
		return {};

	auto hash = crypt::sha1::getHash(std::wstring(data));
	return string::convertToHex(std::begin(hash.byte), std::end(hash.byte));
}

typedef HRESULT(WINAPI* SetThreadDescriptionFunc)(_In_ HANDLE hThread,
	_In_ PCWSTR lpThreadDescription);

//
//
//
void setThreadName(const std::string& sName)
{
	Size nVersion = getCatalogData("os.osMajorVersion", 0);
	if (nVersion < 10)
		return;
	
	// FIXME: Need syncronization
	static SetThreadDescriptionFunc pSetThreadDescriptionFunc = NULL;
	if (pSetThreadDescriptionFunc == NULL)
	{
		sys::win::ScopedLibrary hLib(sys::win::ScopedLibrary(::LoadLibraryW(L"kernel32.dll")));
		if (hLib == NULL)
		{
			error::win::WinApiError(SL).log();
			return;
		}
		pSetThreadDescriptionFunc = (SetThreadDescriptionFunc)::GetProcAddress(hLib, (LPCSTR)"SetThreadDescription");
		if (pSetThreadDescriptionFunc == NULL)
		{
			error::win::WinApiError(SL).log();
			return;
		}
	}

	if (pSetThreadDescriptionFunc != NULL)
	{
		HRESULT hr = pSetThreadDescriptionFunc(GetCurrentThread(), string::convertUtf8ToWChar(sName).c_str());
		if (FAILED(hr))
			error::win::WinApiError(SL, hr, "Can't set thread name").log();
	}
}

} // namespace sys
} // namespace cmd
