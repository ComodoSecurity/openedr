//
// EDRAv2.libcore project
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
/// @{
#include "pch.h"
#include "edrmmapi.hpp"

namespace openEdr {

//
//
//
const SourceLocation getSourceLocation()
{
	return edrmm::getSourceLocation();
}

//
//
//
const SourceLocation setSourceLocation(const SourceLocation& newSrcLoc)
{
	return edrmm::setSourceLocation(newSrcLoc);
}


//
//
//
const SourceLocation getMemSourceLocation(void* pBlock)
{
	return edrmm::getMemSourceLocation(pBlock);
}

//
//
//
void setMemLeaksInfoFilename(const std::filesystem::path& pathMemoryLeakFilename)
{
	edrmm::setLeaksInfoFilename(pathMemoryLeakFilename.wstring().c_str());
}

///
/// Set timeout to wait after memory leaks found.
///
void setMemLeaksTimeout(const uint32_t nTimeout)
{
	edrmm::setMemLeaksTimeout(nTimeout);
	LOGLVL(Debug, "Set memory leaks timiout to " << nTimeout << " ms");
}

namespace detail {

//
//
//
void* allocMem(Size nSize)
{
	return edrmm::allocMem(nSize);
}

//
//
//
void freeMem(void* pBlock)
{
	edrmm::freeMem(pBlock);
}

//
//
//
void* reallocMem(void* pOldBlock, Size nNewSize)
{
	return edrmm::reallocMem(pOldBlock, nNewSize);
}


//
//
//
void* allocAlignedMem(Size nAlignment, Size nSize)
{
	return edrmm::allocAlignedMem(nAlignment, nSize);
}


} // namespace detail

//
//
//
MemInfo getMemoryInfo()
{
	return edrmm::getMemInfo();
}

} // namespace openEdr 

//////////////////////////////////////////////////////////////////////////
//
// Global operator new and operator delete replacement
//

//
//
//
void* operator new(std::size_t nSize)
{
	void* ptr = openEdr::detail::allocMem(nSize);
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

//
//
//
void* operator new(std::size_t nSize, std::align_val_t nAlign)
{
	void* ptr = openEdr::detail::allocAlignedMem((openEdr::Size)nAlign, nSize);
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}

//
//
//
void operator delete(void* ptr)
{
	openEdr::detail::freeMem(ptr);
}

//
// 
//
void operator delete(void* ptr, std::align_val_t /* al */)
{
	openEdr::detail::freeMem(ptr);
}

/// @}
