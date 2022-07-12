//
//  edrav2.libcore project
//
///
/// @file Communication API of erdmm.dll
///
/// @addtogroup edrmm
/// @{
#pragma once
#include "basic.hpp"

// Add import of edrmm.dll
#ifndef EDRMM_EXPORTS
#ifdef _MSC_VER
#pragma comment(lib, "edrmm.lib")
#else
#error Declaration of linking dependences is not implemented for this compiler
#endif // _MSC_VER
#endif // EDRMM_EXPORTS

namespace cmd {
namespace edrmm {

//
// import/export macros
//
#ifdef EDRMM_EXPORTS
#define CMD_EDRMM_API __declspec( dllexport )
#else // EDRMM_EXPORTS
#define CMD_EDRMM_API __declspec( dllimport )
#endif // EDRMM_EXPORTS

///
/// @return The function returns a SourceLocation of current thread.
///
CMD_EDRMM_API const SourceLocation getSourceLocation();

///
/// Sets SourceLocation of current thread.
///
/// @param newSrcLoc - a source location.
/// @return The function returns old SourceLocation.
///
CMD_EDRMM_API const SourceLocation setSourceLocation(const SourceLocation& newSrcLoc);

///
/// Allocates new memory block.
///
/// @param nSize - a size in bytes.
/// @return The function returns a pointer to the allocated memory block.
///
CMD_EDRMM_API void* allocMem(Size nSize);

///
/// Frees allocated memory.
///
/// @param pBlock - a pointer to a memory block.
///
CMD_EDRMM_API void freeMem(void* pBlock);

///
/// Reallocates memory block.
///
/// @param pOldBlock - a pointer to an old block.
/// @param nNewSize - a size of the new block.
/// @return The function returns a pointer to the new memory block.
///
CMD_EDRMM_API void* reallocMem(void* pOldBlock, Size nNewSize);

///
/// Allocates new memory block with specified alignment.
///
/// @param nAlignment - an alignment.
/// @param nSize - a size.
/// @return The function returns a pointer to the allocated memory block.
///
CMD_EDRMM_API void* allocAlignedMem(Size nAlignment, Size nSize);

///
/// Returns SourceLocation of allocated block.
///
/// @param pBlock - a pointer to the memory block.
/// @return block SourceLocation or empty SourceLocation10
///
CMD_EDRMM_API const SourceLocation getMemSourceLocation(void* pBlock);

///
/// Sets name of file for saving detected memory leaks.
///
/// @param sFileName - a name of the file. If sFileName is `nullptr` or "",
///   leaks saving is disabled.
///
/// If the function has never called, leaks saving is disabled.
///
CMD_EDRMM_API void setLeaksInfoFilename(const wchar_t* sFileName);

///
/// Returns allocated memory size.
///
/// Also (optionally) the allocation counters (current and total).
///
/// @return The function returns a structure with requested information.
///
CMD_EDRMM_API MemInfo getMemInfo();

///
/// Set timeout to wait after memory leaks found.
///
/// @param nTimeout - a timeout.
///
CMD_EDRMM_API void setMemLeaksTimeout(const uint32_t nTimeout);

} // namespace edrmm
} // namespace cmd 
/// @}



