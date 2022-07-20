//
// edrav2.edrdrv project
//
// Author: Yury Podpruzhnikov (31.01.2019)
// Reviewer: Denis Bogdanov (21.02.2019)
//
///
/// @file File filter
///
/// @addtogroup edrdrv
/// @{
#pragma once

namespace cmd {
namespace filemon {

///
/// Update file rules.
///
/// @param pData - TBD.
/// @param nDataSize - TBD.
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS updateFileRules(const void* pData, size_t nDataSize);

///
/// Print all attached instances.
///
/// @returns The function returns NTSTATUS of the operation.
///
/// applyForEachInstance sample.
///
NTSTATUS printInstances();

///
/// Initialization.
///
/// @returns The function returns NTSTATUS of the operation.
///
NTSTATUS initialize();

///
/// Finalization.
///
void finalize();

namespace tools
{

__drv_maxIRQL(APC_LEVEL)
__checkReturn
NTSTATUS
getInstanceFromFileObject(
	__in PFILE_OBJECT FileObject,
	__deref_out PFLT_INSTANCE* Instance
);

#define FILE_SHARE_ALL (FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)

__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
openFile(
	__in_opt PFLT_INSTANCE Instance,
	__in const UNICODE_STRING& FilePath,
	__out UniqueFltHandle& FileHandle,
	__out FileObjectPtr& FileObject,
	__in ACCESS_MASK DesiredAccess = FILE_GENERIC_READ,
	__in ULONG ShareAccess = FILE_SHARE_ALL
);

__drv_maxIRQL(PASSIVE_LEVEL)
__checkReturn
NTSTATUS
getSystemRootDirectoryPath(
	__out DynUnicodeString& SystemRootPath
);

} //namespace tools
} // namespace filemon
} // namespace cmd
/// @}
