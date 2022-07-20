//
// edrav2.edrflt project
//
// Author: Yury Podpruzhnikov (29.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file Common Process information
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace cmd {

///
///The enum class of additional process flags.
///
enum class ProcessInfoFlags : UINT32
{
	CsrssProcess		= 1 << 0, ///< csrss process
	SystemLikeProcess	= 1 << 1, ///< SYSTEM(4) process or similar
	ThisProductProcess  = 1 << 2, ///< This product process
};

inline static constexpr ULONG_PTR c_nIdleProcessPid = 0;

///
/// Basic Process information.
///
struct CommonProcessInfo
{
	// +FIXME: Use m_ prefix!
	// It is struct!
	HANDLE nPid; ///< Process PID
	HANDLE nParentPid; ///< Parent PID
	UINT32 nFlags; ///< ProcessInfoFlags
	BOOLEAN fIsTerminated; ///< Process was terminated
	PUNICODE_STRING pusImageName; ///< Image filename, possible nullptr


	PS_PROTECTED_TYPE eProtectedType; ///< Protected process info
	PS_PROTECTED_SIGNER eProtectedSigner; ///< Protected process info

	/// 
	/// Initializes data.
	///
	void init()
	{
		RtlZeroMemory(this, sizeof(*this));
	}

	/// 
	/// Free data.
	///
	void free()
	{
		if (pusImageName != nullptr)
		{
			ExFreePoolWithTag(pusImageName, c_nAllocTag);
			pusImageName = nullptr;
		}
		nPid = 0;
		nParentPid = 0;
		nFlags = 0;
	}

	/// 
	/// Return string for log.
	///
	/// @return The function returns a pointer to UNICODE string.
	///
	PUNICODE_STRING getPrintImageName();
};


//////////////////////////////////////////////////////////////////////////
//
// Collect process info
//

// +FIXME: Can we use fillProcessInfoByPid() as method of CommonProcessInfo
///
/// Fills process information by handle.
///
/// @param nPid[in] - process pid.
/// @param hProcess[in] - process handle.
/// @param pProcessInfo[in] - processInfo for filling.
/// @param fPrintDebugInfo[in] - print debug info.
///
_IRQL_requires_max_(PASSIVE_LEVEL)
void fillProcessInfoByHandle(HANDLE nPid, HANDLE hProcess, CommonProcessInfo* ProcessInfo, bool fPrintDebugInfo);

// +FIXME: Can we use fillProcessInfoByPid() as method of CommonProcessInfo
///
/// Fills process information by PID.
///
/// @param nPid[in] - process pid.
/// @param pProcessInfo[in] - processInfo for filling.
/// @param fPrintDebugInfo[in] - print debug info.
///
_IRQL_requires_max_(PASSIVE_LEVEL)
void fillProcessInfoByPid(HANDLE nPid, CommonProcessInfo* ProcessInfo, bool fPrintDebugInfo);

///
/// Returns times of the process.
///
/// @param pProcess[in] - PEPROCESS.
/// @param pTimes[out] - structure for filling.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS getProcessTimes(PEPROCESS pProcess, KERNEL_USER_TIMES* pTimes);

///
/// Returns list of processes.
///
/// @param[out] ppProcessArray - new allocated array. It must be released by ExFreePoolWithTag.
/// @param[out] pnSize - count of elements.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS getProcessList(HANDLE** ppProcessArray, size_t* pnSize);

///
/// Returns process access token.
///
/// @param[in] pProcess - process object.
/// @param[out] pToken - result.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS getProcessToken(PEPROCESS pProcess, HANDLE* phToken);

///
/// Returns process access token.
/// @param[in] hProcess - process handle.
/// @param[out] pToken - result.
/// @return The function returns a NTSTATUS of the operation.
///
NTSTATUS getProcessToken(HANDLE hProcess, HANDLE* phToken);

} // namespace cmd
/// @}
