//
// edrav2.libcore project
//
// Author: Yury Ermakov (14.12.2018)
// Reviewer: Denis Bogdanov (21.12.2018)
//
///
/// @file WinAPI errors processing declaration
///
#include "pch_win.h"
#include <error/error_win.hpp>

namespace openEdr {
namespace error {
namespace win {

//
//
//
std::string getWinApiErrorString(DWORD dwErrCode)
{
	std::ostringstream res;
	res << hex(dwErrCode);

	wchar_t* pMsgBuf{ nullptr };
	const DWORD dwMsgSize = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, dwErrCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&pMsgBuf), 0, NULL);

	if (dwMsgSize > 0)
	{
		// TODO: Use automatic resource controller class
		auto deleter = [](void* p) { ::HeapFree(::GetProcessHeap(), 0, p); };
		std::unique_ptr<wchar_t, decltype(deleter)> msgBuf(pMsgBuf, deleter);
		try
		{
			std::string s = string::convertWCharToUtf8(msgBuf.get());
			s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
			{ return !std::isspace(ch) && (ch != '.'); }).base(), s.end());
			res << " - " << s;
		}
		catch (const error::StringEncodingError&) {}	// ignore encoding error
	}

	return res.str();
}


//
//
//
std::string WinApiError::createWhat(DWORD dwErrCode, std::string_view sMsg)
{
	std::string s(sMsg);
	if (s.empty())
		s += "WinAPI error " + getWinApiErrorString(dwErrCode);
	else
		s += " [WinAPI error " + getWinApiErrorString(dwErrCode) + "]";

	return s;
}

//
//
//
WinApiError::WinApiError(DWORD dwErrCode, std::string_view sMsg) :
	SystemError(createWhat(dwErrCode, sMsg)),
	m_dwErrCode(dwErrCode)
{
}

//
//
//
WinApiError::WinApiError(SourceLocation srcLoc, std::string_view sMsg) :
	SystemError(srcLoc, createWhat(m_dwErrCode = ::GetLastError(), sMsg))
{
}

//
//
//
WinApiError::WinApiError(SourceLocation srcLoc, DWORD dwErrCode, std::string_view sMsg) :
	SystemError(srcLoc, createWhat(dwErrCode, sMsg)),
	m_dwErrCode(dwErrCode)
{
}

//
//
//
WinApiError::~WinApiError()
{
}

//
//
//
DWORD WinApiError::getWinErrorCode() const
{
	return m_dwErrCode;
}

//
//
//
std::exception_ptr WinApiError::getRealException()
{
	switch (m_dwErrCode)
	{
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return std::make_exception_ptr(NotFound(*this));
		case ERROR_ACCESS_DENIED: 
			return std::make_exception_ptr(AccessDenied(*this));
		case ERROR_NOT_ENOUGH_MEMORY:
			return std::make_exception_ptr(BadAlloc(*this));
	}

	return std::make_exception_ptr(*this);
}

} // namespace win
} // namespace error
} // namespace openEdr
