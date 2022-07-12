//
// edrav2.libcore project
//
// Memory manager
//
// Author: Yury Podpruzhnikov (15.12.2018)
// Reviewer: Denis Bogdanov (15.01.2019) 
//
///
/// @file Memory manager
///
/// @addtogroup memmgr Memory manager
/// Global memory allocation hook for leak detection
/// It also provides global replacement of operator new and operator delete.
/// @{

#pragma once
#include "basic.hpp"

namespace cmd {

///
/// Returns SourceLocation of current thread 
///
const SourceLocation getSourceLocation();

///
/// Sets SourceLocation of current thread
/// @param newSrcLoc
/// @return old SourceLocation
///
const SourceLocation setSourceLocation(const SourceLocation& newSrcLoc);

///
/// Returns SourceLocation of allocated block
/// @return block SourceLocation or empty SourceLocation10
///
const SourceLocation getMemSourceLocation(void* pBlock);

///
/// Sets filename to save memory leaks.
///
void setMemLeaksInfoFilename(const std::filesystem::path& pathMemoryLeakFilename);

///
/// Get memory information
///
MemInfo getMemoryInfo();

///
/// Set timeout to wait after memory leaks found.
///
void setMemLeaksTimeout(const uint32_t nTimeout);


//
//
//
template<typename T>
void fillMemory(T& data, char s = 0)
{
	memset(&data, s, sizeof(data));
}


namespace detail {

//
//
//
void* allocMem(Size nSize);

//
//
//
void freeMem(void* pBlock);

//
//
//
void* reallocMem(void* pOldBlock, Size nNewSize);

//
//
//
void* allocAlignedMem(Size nAlignment, Size nSize);

//
//
//
class CheckInSourceLocationHelper
{
private:
	SourceLocation m_restoredPos;

public:
	CheckInSourceLocationHelper(const SourceLocation /*curPos*/, const SourceLocation newPos) noexcept
	{
		m_restoredPos = setSourceLocation(newPos);
	}

	CheckInSourceLocationHelper(const SourceLocation curPos, nullptr_t) noexcept
	{
		m_restoredPos = setSourceLocation(curPos);
	}

	~CheckInSourceLocationHelper() noexcept
	{
		setSourceLocation(m_restoredPos);
	}

	static const SourceLocation checkInPlace(const SourceLocation pos) noexcept
	{
		return pos;
	}

	static const SourceLocation checkInPlace(const char* sFile, int nLine) noexcept
	{
		return SourceLocation{ SourceLocationTag(), sFile, nLine };
	}

	static nullptr_t checkInPlace() noexcept
	{
		return nullptr;
	}
};

//
// Deleter for MallocPtr
//
template<typename T>
struct FreeMemDeleter
{
	void operator () (T* pBlock) const
	{
		return freeMem(pBlock);
	};
};

} // namespace detail 


///
/// Check in new source location and restore old at scope exit
/// Usage:
///  * CHECK_IN_SOURCE_LOCATION();  - checking in current position
///  * CHECK_IN_SOURCE_LOCATION(sourceLocation);  - checking in the sourceLocation value
///  * CHECK_IN_SOURCE_LOCATION("fileName", 10);  - checking in "fileName" and line 10.
///
#define CHECK_IN_SOURCE_LOCATION(...)											\
	cmd::detail::CheckInSourceLocationHelper CONCAT_ID(CheckInLoc, __LINE__) \
	(SL, cmd::detail::CheckInSourceLocationHelper::checkInPlace(__VA_ARGS__))

///
/// Smart pointer returned by allocMem() and reallocMem()
///
template<typename T>
using MemPtr = std::unique_ptr<T, detail::FreeMemDeleter<T>>;

///
/// Replacement of malloc() with memory leak detection
/// If can't allocate, returns empty pointer.
///
template<typename T = Byte>
inline MemPtr<T> allocMem(Size nCount)
{
	return MemPtr<T>((T*)detail::allocMem(sizeof(T)*nCount));
}

///
/// Replacement of realloc() with memory leak detection
/// If can't allocate, returns empty pointer.
/// TODO: replace std::uint8_t
///
template<typename T = std::uint8_t>
inline MemPtr<T> reallocMem(MemPtr<T>& pOldBlock, Size nNewCount)
{
	auto pNewData = detail::reallocMem(pOldBlock.release(), sizeof(T)*nNewCount);
	return MemPtr<T>((T*)pNewData);
}


} // namespace cmd
/// @}
