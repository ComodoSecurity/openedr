//
// EDRAv2.libcore project
//
// Author: Yury Ermakov (17.12.2018)
// Reviewer: Denis Bogdanov (21.12.2018)
//
///
/// @file MinidumpWriter class declaration (for Windows only)
///
#include "pch_win.h"
#include <error\dump\minidumpwriter.hpp>
#include <version.h>

#ifndef _WIN32
#error You should include this file only into windows-specific code
#endif // _WIN32

namespace cmd {
namespace error {
namespace dump {
namespace win {

//
//
//
NTSTATUS getStatusFromExceptionInfo(PEXCEPTION_POINTERS pExInfo)
{
	// 0xC0000001 - STATUS_UNSUCCESSFUL - { Operation Failed } The requested operation was unsuccessful.
	if (NULL == pExInfo)
		return 0xC0000001;
	if (NULL == pExInfo->ExceptionRecord)
		return 0xC0000001;
	return pExInfo->ExceptionRecord->ExceptionCode;
}

namespace detail {

namespace fs = std::filesystem;

//
//
//
MinidumpWriter::MinidumpWriter() :
	callMiniDumpWriteDump{ NULL },
	m_dbhelpLib{ NULL },
#if defined(_DEBUG)
	m_minidumpType{ MiniDumpWithFullMemory }
#else
	m_minidumpType{ MINIDUMP_TYPE(MiniDumpNormal | MiniDumpWithUnloadedModules | 
		MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithProcessThreadData) }
	// May be we need to add also MiniDumpWithFullMemoryInfo and MiniDumpWithThreadInfo flags
#endif // _DEBUG
{
	m_dbhelpLib = sys::win::ScopedLibrary(::LoadLibraryW(L"dbghelp.dll"));
	if (m_dbhelpLib == NULL)
		error::win::WinApiError(SL).throwException();
	callMiniDumpWriteDump = (MiniDumpWriteDumpFunc)::GetProcAddress(m_dbhelpLib, (LPCSTR)"MiniDumpWriteDump");
	if (callMiniDumpWriteDump == NULL)
		error::win::WinApiError(SL).throwException();

	wchar_t buf[0x1000];
	DWORD dwSize = GetModuleFileNameW(NULL, buf, sizeof(buf) / sizeof(buf[0]));
	if (dwSize == 0)
		error::win::WinApiError(SL).throwException();

	std::wstring sModuleFullName(buf);
	auto const nPos = sModuleFullName.find_last_of(L"\\/");
	if (std::wstring::npos != nPos)
	{
		m_sPrefix = sModuleFullName.substr(nPos + 1);
		m_sDumpsPath = sModuleFullName.substr(0, nPos);
	}
	else
	{
		m_sPrefix = sModuleFullName;
		m_sDumpsPath = L".";
	}
	m_sDumpsPath += L"\\dump";
}

//
//
//
MinidumpWriter::~MinidumpWriter()
{
}

//
//
//
std::wstring MinidumpWriter::makeDumpFilename(DWORD dwProcessId, DWORD dwThreadId, NTSTATUS status /* = 0 */) const
{
	SYSTEMTIME now;
	::GetLocalTime(&now);

	// {prefix}-b{build}-{configuration}-{platform}-yymmdd-hhmmss-{pid}-{tid}-{status}.dmp
	std::wostringstream res;
	res << std::setfill(L'0') 
		<< m_sPrefix << "-b"
		<< CMD_VERSION_BUILD << '-'
		<< CMD_VERSION_TYPE << '-'
		<< CPUTYPE_STRING << '-'
		<< std::setw(2) << now.wYear % 100
		<< std::setw(2) << now.wMonth
		<< std::setw(2) << now.wDay << '-'
		<< std::setw(2) << now.wHour
		<< std::setw(2) << now.wMinute
		<< std::setw(2) << now.wSecond << '-'
		<< dwProcessId << '-'
		<< dwThreadId << '-'
		<< std::setw(8) << std::hex << status
		<< L".dmp";
	return res.str();
}

//
//
//
void MinidumpWriter::save(const std::wstring& sDumpFile, PEXCEPTION_POINTERS pExInfo /* = nullptr */) const
{
	// create target directory if it doesn't exist
	fs::path pathDumpDir = sDumpFile;
	pathDumpDir.remove_filename();
	std::error_code ec;
	fs::create_directories(pathDumpDir, ec);
	if (ec)
		error::StdSystemError(SL, ec, FMT("Can't create directory for dump <"
			<< pathDumpDir << ">")).log();

	sys::win::ScopedFileHandle hFile(::CreateFileW(sDumpFile.c_str(), GENERIC_WRITE,
		FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	if (hFile == INVALID_HANDLE_VALUE)
		error::win::WinApiError(SL, "Can't create dump file").throwException();

	_MINIDUMP_EXCEPTION_INFORMATION* pDumpInfo{ NULL };
	_MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
	if (pExInfo != NULL)
	{
		dumpInfo.ThreadId = ::GetCurrentThreadId();
		dumpInfo.ExceptionPointers = pExInfo;
		dumpInfo.ClientPointers = NULL;
		pDumpInfo = &dumpInfo;
	}

	BOOL fResult = (callMiniDumpWriteDump)(::GetCurrentProcess(), ::GetCurrentProcessId(),
		hFile, m_minidumpType, pDumpInfo, NULL, NULL);
	if (!fResult)
	{
		fResult = (callMiniDumpWriteDump)(
			::GetCurrentProcess(), ::GetCurrentProcessId(), hFile,
			MINIDUMP_TYPE(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpIgnoreInaccessibleMemory),
			pDumpInfo, NULL, NULL);
		if (!fResult)
			error::RuntimeError(SL, "Can't write dump").throwException();
	}
}

//
//
//
void MinidumpWriter::save(PEXCEPTION_POINTERS pExInfo /* = nullptr */) const
{
	std::wstring sDumpFile(m_sDumpsPath + L"\\" + makeDumpFilename(
		::GetCurrentProcessId(),
		::GetCurrentThreadId(),
		getStatusFromExceptionInfo(pExInfo)));

	save(sDumpFile, pExInfo);
}

//
//
//
MinidumpWriter& MinidumpWriter::getInstance()
{
	static MinidumpWriter g_MinidumpWriter;
	return g_MinidumpWriter;
}

//
//
//
void MinidumpWriter::init(const Variant& vConfig)
{
	if (vConfig.isNull())
		return;
	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "Dictionary expected").throwException();

	if (vConfig.has("prefix"))
		m_sPrefix = vConfig.get("prefix");
	if (vConfig.has("dumpDirectory"))
		m_sDumpsPath = vConfig.get("dumpDirectory");
}

} // namespace detail
} // namespace win
} // namespace dump
} // namespace error
} // namespace cmd
