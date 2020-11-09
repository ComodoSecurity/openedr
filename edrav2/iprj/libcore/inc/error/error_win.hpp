//
// EDRAv2.libcore project
//
// Author: Yury Ermakov (14.12.2018)
// Reviewer: Denis Bogdanov (21.12.2018)
//
///
/// @file WinAPI errors processing declaration
///
/// @addtogroup errors Errors processing
/// @{
#pragma once

#include "error_common.hpp"
#include "string.hpp"

#ifndef _WIN32
#error You should include this file only into windows-specific code
#endif // #ifdef _WIN32

namespace openEdr {
namespace error {
namespace win {

#ifndef DWORD
typedef uint32_t DWORD;
#endif

///
/// Get WinAPI last error's description.
///
/// @param dwErrorCode - WinAPI error code.
/// @return The function returns a text description for the specified error code.
/// 
std::string getWinApiErrorString(DWORD dwErrCode);

///
///
///
inline DWORD getFromHResult(HRESULT hRes)
{
	if ((hRes & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)) 
		return HRESULT_CODE(hRes);

	if (hRes == S_OK)
		return HRESULT_CODE(hRes);

	// otherwise, we got an impossible value
	return ERROR_GEN_FAILURE;
}


///
/// WinAPI error class.
///
/// Some WinAPI errors are translated to a particular error:
///  * `ERROR_FILE_NOT_FOUND` -> `error::NotFound`.
///
class WinApiError : public SystemError
{
private:
	DWORD m_dwErrCode;

protected:
	std::exception_ptr getRealException() override;
	std::string createWhat(DWORD dwErrCode, std::string_view sMsg);

public:
	///
	/// Constructor.
	///
	/// @param dwErrCode - WinAPI error code.
	/// @param sMsg [opt] - an error description (default: "").
	///
	WinApiError(DWORD dwErrCode, std::string_view sMsg = "");

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param sMsg [opt] - an error description (default: "").
	///
	WinApiError(SourceLocation srcLoc, std::string_view sMsg = "");
	
	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param dwErrCode - WinAPI error code.
	/// @param sMsg [opt] - an error description (default: "").
	///
	WinApiError(SourceLocation srcLoc, DWORD dwErrCode, std::string_view sMsg = "");

	///
	/// Destructor.
	///
	virtual ~WinApiError();

	///
	/// Gets WinAPI error code.
	///
	/// @returns The function returns the value of WinAPI error code.
	///
	DWORD getWinErrorCode() const;
};

} // namespace win
} // namespace error
} // namespace openEdr
/// @}
