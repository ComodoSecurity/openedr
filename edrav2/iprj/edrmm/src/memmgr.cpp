//
// EDRAv2.edrmm project
//
// Autor: Yury Podpruzhnikov (08.04.2019)
// Reviewer:
//
///
/// @file Memory manager
///
/// @addtogroup edrmm
/// 
/// Memory manager provides memory allocation and leak detection functions.
/// 
/// @{
#include "pch.h"

//
// Leak detection way
//
#if defined(_MSC_VER) && defined(_DEBUG)
// Use leaks detection from microsoft allocator
#define USE_MSC_MEM_LEAKS_DETECTION
#elif defined(_MSC_VER) /*&& !defined(_DEBUG)*/
// Disable allocation usage in release
#else
#error Malloc debug is not implemented for this compiler. \
Add implementation or exception (in this #if). \
If you add exception, standard malloc without debug will be used.
#endif


#ifdef USE_MSC_MEM_LEAKS_DETECTION
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif


namespace cmd {
namespace edrmm {

//////////////////////////////////////////////////////////////////////////
//
// Source location
//


// Source location of thread
thread_local SourceLocation g_CurrentSourceLocation;
// shortcut for casting thread_local SourceLocation into auto SourceLocation
inline const SourceLocation getCurSL()
{
	return g_CurrentSourceLocation;
}

//
// Name of file for saving detected memory leaks.
// This variable should not be in dynamical memory.
//
static constexpr Size c_nMexLongPath = 0x8000;
wchar_t g_sMemoryLeaksFilename[c_nMexLongPath];
std::mutex g_mtxMemoryLeaksFilename; //< mutex for sync access to g_sMemoryLeaksFilename
std::atomic<Size> m_nCurrAllocSize = 0;
std::atomic<Size> m_nTotalAllocSize = 0;
std::atomic<Size> m_nCurrAllocCount = 0;
std::atomic<Size> m_nTotalAllocCount = 0;


//
//
//
CMD_EDRMM_API const SourceLocation getSourceLocation()
{
	return getCurSL();
}

//
//
//
CMD_EDRMM_API const SourceLocation setSourceLocation(const SourceLocation& newSrcLoc)
{
	auto old = getCurSL();
	g_CurrentSourceLocation = newSrcLoc;
	return old;
}

//////////////////////////////////////////////////////////////////////////
//
// Memory allocation
//
//////////////////////////////////////////////////////////////////////////

//
//
//
CMD_EDRMM_API void* allocMem(size_t nSize)
{
#ifdef USE_MSC_MEM_LEAKS_DETECTION
	m_nCurrAllocSize += nSize;
	m_nTotalAllocSize += nSize;
	m_nTotalAllocCount++;
	m_nCurrAllocCount++;
	auto srcLoc = getCurSL();
	return _malloc_dbg(nSize, _NORMAL_BLOCK, srcLoc.sFileName,
		(srcLoc.nLine != 0 ? srcLoc.nLine : GetCurrentThreadId()));
#else
	return std::malloc(nSize);
#endif
}

//
//
//
CMD_EDRMM_API void freeMem(void* pBlock)
{
#ifdef USE_MSC_MEM_LEAKS_DETECTION
	if (pBlock)
	{
		m_nCurrAllocSize -= _msize(pBlock);
		m_nCurrAllocCount--;
	}
	return free(pBlock);
#else
	return std::free(pBlock);
#endif
}

//
//
//
CMD_EDRMM_API void* reallocMem(void* pOldBlock, Size nNewSize)
{
#ifdef USE_MSC_MEM_LEAKS_DETECTION
	if (pOldBlock)
		m_nCurrAllocSize -= _msize(pOldBlock);
	m_nCurrAllocSize += nNewSize;
	m_nTotalAllocSize += nNewSize;
	m_nTotalAllocCount++;
	auto srcLoc = getCurSL();
	return _realloc_dbg(pOldBlock, nNewSize, _NORMAL_BLOCK, srcLoc.sFileName, srcLoc.nLine);
#else
	return std::realloc(pOldBlock, nNewSize);
#endif
}

//
//
//
CMD_EDRMM_API void* allocAlignedMem(Size nAlignment, Size nSize)
{
#ifdef USE_MSC_MEM_LEAKS_DETECTION
	auto srcLoc = getCurSL();
	m_nCurrAllocSize += nSize;
	m_nTotalAllocSize += nSize;
	m_nCurrAllocCount++;
	m_nTotalAllocCount++;
	return _aligned_malloc_dbg(nSize, nAlignment, srcLoc.sFileName, srcLoc.nLine);
#else
	return _aligned_malloc(nSize, nAlignment);
#endif
}

//
//
//
CMD_EDRMM_API MemInfo getMemInfo()
{
	MemInfo m;
	m.nCurrCount = m_nCurrAllocCount;
	m.nTotalCount = m_nTotalAllocCount;
	m.nCurrSize = m_nCurrAllocSize;
	m.nTotalSize = m_nTotalAllocSize;
	return m;
}

//
//
//
CMD_EDRMM_API const SourceLocation getMemSourceLocation(void* pBlock)
{
#ifdef USE_MSC_MEM_LEAKS_DETECTION
	char* sFilename = nullptr;
	int nLine;
	Size nSize = _msize_dbg(pBlock, _NORMAL_BLOCK);
	if (_CrtIsMemoryBlock(pBlock, static_cast<int>(nSize), nullptr, &sFilename, &nLine))
		return SourceLocation{ SourceLocationTag{},  sFilename, nLine };
	return SourceLocation{};
#else
	return SourceLocation{};
#endif
}


//////////////////////////////////////////////////////////////////////////
//
// Memory leaks detection
//

//
//
//
CMD_EDRMM_API void setLeaksInfoFilename(const wchar_t* sFileName)
{
	std::scoped_lock lock(g_mtxMemoryLeaksFilename);
	if (sFileName == nullptr)
	{
		g_sMemoryLeaksFilename[0] = '\0';
		return;
	}
	if (wcslen(sFileName) >= std::size(g_sMemoryLeaksFilename))
		return;
	wcscpy_s(g_sMemoryLeaksFilename, sFileName);
}

///
///
///
uint32_t nMemLeaksTimeout = 0;
CMD_EDRMM_API void setMemLeaksTimeout(const uint32_t nTimeout)
{
	nMemLeaksTimeout = nTimeout;
}

#ifdef USE_MSC_MEM_LEAKS_DETECTION

_CrtMemState g_initialMemState = {0};

//
//
//
void startMemoryLeaksMonitoring()
{
	_CrtMemCheckpoint(&g_initialMemState);
}

//
//
//
inline Time getCurrentTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
}

//
//
//
void saveMemoryLeaks()
{
	// startMemoryLeaksMonitoring was not called
	if (g_initialMemState.pBlockHeader == nullptr)
		return;

	// g_sMemoryLeaksFilename is not set
	if (std::wcslen(g_sMemoryLeaksFilename) == 0)
		return;

	// Check leaks
	_CrtMemState curState;
	_CrtMemCheckpoint(&curState);

	// Test and write memory or objects leaks to log file
	_CrtMemState diffMemState;
	if (!_CrtMemDifference(&diffMemState, &g_initialMemState, &curState))
		return;

	// Test known static leaks
	size_t nAllocatedBlockCount = diffMemState.lCounts[_NORMAL_BLOCK] + diffMemState.lCounts[_CLIENT_BLOCK];
	size_t nAllocatedBlockSize = diffMemState.lSizes[_NORMAL_BLOCK] + diffMemState.lSizes[_CLIENT_BLOCK];
	// Mask 3 pointers from GRPC
	if (nAllocatedBlockSize <= 3 * sizeof(UINT_PTR))
		return;

	// Output memory leaks
	HANDLE hLogFile = CreateFileW(g_sMemoryLeaksFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hLogFile == INVALID_HANDLE_VALUE)
		return;

	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, hLogFile);
	_CrtSetReportFile(_CRT_ERROR, hLogFile);
	_CrtSetReportFile(_CRT_WARN, hLogFile); 

	_RPT1(_CRT_WARN, "Memory leaks are detected.\nProcess id: %u.\n", GetCurrentProcessId());
	_RPT0(_CRT_WARN, "\nStatistic:\n");
	_CrtMemDumpStatistics(&diffMemState);
	_RPT0(_CRT_WARN, "\nDetailed information:\n");
	_CrtMemDumpAllObjectsSince(&g_initialMemState);

    nMemLeaksTimeout = 200000; // FIXME: remove me
	if (nMemLeaksTimeout)
		_RPT2(_CRT_WARN, "Sleep for %u sec (current time %u).\n", nMemLeaksTimeout, getCurrentTime());

	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	CloseHandle(hLogFile);

	if (nMemLeaksTimeout)
		::Sleep(nMemLeaksTimeout);
}

#else // USE_MSC_MEM_LEAKS_DETECTION

//
//
//
void startMemoryLeaksMonitoring()
{

}

//
//
//
void saveMemoryLeaks()
{

}

#endif // USE_MSC_MEM_LEAKS_DETECTION

} // namespace edrmm
} // namespace cmd

