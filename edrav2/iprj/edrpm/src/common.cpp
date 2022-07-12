//
// edrav2.edrpm project
//
// Author: Denis Kroshin (08.04.2019)
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
#include "injection.h"

namespace cmd {
namespace edrpm {


//
//
//
void HandleTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::CloseHandle(rsrc))
		logError("Can't close handle");
}

//
//
//
void FileHandleTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (!::CloseHandle(rsrc))
		logError("Can't close handle");
}

//
//
//
void LocalMemoryTraits::cleanup(ResourceType& rsrc) noexcept
{
	if (::LocalFree(rsrc) != NULL)
		logError("Can't free memory");
}

//
//
//
std::string getModuleFileName()
{
	std::string sName(0x100, '\0');
	DWORD nSize = ::GetModuleFileNameA(NULL, sName.data(), DWORD(sName.size()));
	if (nSize == 0 && ::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		sName.resize(nSize);
		if (!::GetModuleFileNameA(NULL, sName.data(), DWORD(sName.size())))
			sName.clear();
	}
	return sName.c_str();
}

//
//
//
void logError(const std::string sErr, ErrorType eErr)
{
	static const std::string sModuleName(getModuleFileName());
	sendEvent(RawEvent::PROCMON_LOG_MESSAGE, [eErr, sErr](auto serializer) -> bool
		{
			return (serializer->write(EventField::ErrorMsg, sErr) &&
				serializer->write(EventField::ErrorType, uint8_t(eErr)) &&
				sModuleName.empty() ? true : serializer->write(EventField::Path, sModuleName));
		});
	dbgPrint(sErr + " (pid <" + std::to_string(::GetCurrentProcessId()) + ">");
}

//
//
//
#ifdef _DEBUG
void dbgPrint(const std::string sStr)
{
	std::string sErr("[CMD] Injection: ");
	sErr += sStr + " (pid: " + std::to_string(::GetCurrentProcessId()) + ")\n";
	::OutputDebugStringA(sErr.c_str());
}
#else
void dbgPrint(const std::string sStr) {}
#endif

//
//
//
void UserBreak(const char* sFn /*= NULL*/)
{
	std::string sTmp("Critical situation in trampoline");
	if (sFn != NULL)
		sTmp += std::string(" <") + sFn + ">";
	logError(sTmp);
	::DebugBreak();
}

} // namespace edrpm
} // namespace cmd

