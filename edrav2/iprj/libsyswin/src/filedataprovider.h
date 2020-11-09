//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (14.03.2019)
// Reviewer: Denis Bogdanov (01.04.2019) 
//
///
/// @file File Data Provider declaration
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once
#include <objects.h>

namespace openEdr {
namespace sys {
namespace win {

#define FDP_USE_NORMALIZATION_API

//
// Object Manager Directory Specific Access Rights.
//
#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)
#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

#define STATUS_NEED_MORE_ENTRIES 0x105

typedef struct _OBJECT_DIRECTORY_INFORMATION {
	UNICODE_STRING Name;
	UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef NTSTATUS(WINAPI * fnNtOpenDirectoryObject)(
	_Out_ PHANDLE            DirectoryHandle,
	_In_  ACCESS_MASK        DesiredAccess,
	_In_  POBJECT_ATTRIBUTES ObjectAttributes
	);

typedef NTSTATUS(WINAPI * fnNtQueryDirectoryObject)(
	_In_      HANDLE  DirectoryHandle,
	_Out_opt_ PVOID   Buffer,
	_In_      ULONG   Length,
	_In_      BOOLEAN ReturnSingleEntry,
	_In_      BOOLEAN RestartScan,
	_Inout_   PULONG  Context,
	_Out_opt_ PULONG  ReturnLength
);

typedef NTSTATUS(NTAPI * fnNtOpenSymbolicLinkObject)(
	_Out_ PHANDLE            LinkHandle,
	_In_  ACCESS_MASK        DesiredAccess,
	_In_  POBJECT_ATTRIBUTES ObjectAttributes
	);

typedef NTSTATUS(NTAPI * fnNtQuerySymbolicLinkObject)(
	_In_ HANDLE LinkHandle,
	_Inout_ PUNICODE_STRING LinkTarget,
	_Out_opt_ PULONG ReturnedLength
	);

///
/// Process Data Provider class.
///
class FileDataProvider : public ObjectBase<CLSID_FileDataProvider>,
	public ICommandProcessor,
	public IFileInformation,
	public IService
{
private:
	// URL: https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/f992ad60-0fe4-4b87-9fed-beb478836861
	static const Size c_nMaxBinSidSize = 68;
	// URL: https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-dtyp/c92a27b1-c772-4fa7-a432-15df5f1b66a1
	static const Size c_nMaxStrSidSize = 164;

	static const Time c_nPurgeTimeout = 5 * 60 * 1000; // 5 min
	// Minimal time between calls of purgeItems()
	static const Time c_nMinPurgeTimeout = 10 * 1000; // 10 sec

	std::shared_mutex m_mtxFileData;
	std::unordered_map<std::wstring, FileId> m_pPathMap;
	std::unordered_map<FileId, Variant> m_pFileMap;

	std::mutex m_mtxCtxData;
	std::condition_variable m_cvCtxData;
	std::unordered_set<FileId> m_pCtxHashQueue;
	std::unordered_set<FileId> m_pCtxSignQueue;

	std::mutex m_mtxVolumeData;
	std::unordered_map<std::wstring, FileId> m_pDeviceMap;
	std::unordered_map<FileId, Variant> m_pVolumeMap;

	ObjPtr<ISignatureInformation> m_pSignDP;
	std::map<std::string, std::string> m_pFormats;

	std::mutex m_mtxStartStop;
	std::atomic_bool m_fInitialized = false;
	std::string m_sEndpointId;
	io::IoSize m_nMaxStreamSize = g_nMaxStreamSize;
	Time m_nPurgeFileTimeout = c_nPurgeTimeout;
	Time m_nPurgeSheduleTimeout = c_nMinPurgeTimeout;
	ThreadPool::TimerContextPtr m_timerPurge;

	std::map<std::wstring, std::wstring> m_pObjectsMap;
	fnNtOpenDirectoryObject m_pNtOpenDirectoryObject = nullptr;
	fnNtQueryDirectoryObject m_pNtQueryDirectoryObject = nullptr;
	fnNtOpenSymbolicLinkObject m_pNtOpenSymbolicLinkObject = nullptr;
	fnNtQuerySymbolicLinkObject m_pNtQuerySymbolicLinkObject = nullptr;

	bool parseObjectDirectory(const std::wstring& sLink, std::wstring& sOutStr);
	std::wstring parseObjectDirectory(const std::wstring& sPathName);
	std::wstring getSymbolicLink(HANDLE hDirectory, PUNICODE_STRING pName);

	//
	// Generate unique user ID 
	//
	FileId generateId(const std::wstring& sSid);

	bool purgeItems();

	std::wstring queryDosDevice(const std::wstring& sDrv);
	std::wstring getVolumeMountPoint(const std::wstring& sVolumeName);
	std::wstring getFullPathName(const std::wstring& sPathName);
	std::wstring getLongPathName(const std::wstring& sPathName);

	std::wstring normalizePathNameInt(const std::wstring& sPathName, Variant vSecurity, PathType eType);
	std::wstring normalizePathManualy(std::wstring sPathName, Variant vSecurity, bool fDos);
	std::wstring convertNtPathToDosPath(std::wstring sPathName);

	Variant getFileType(std::filesystem::path sPath);
	void updateFileInfo(Variant vFile, Variant vParms);

	bool hasFileInfo(const FileId& sFileId);
	Variant getFileInfoById(FileId sFileId);
	Variant getFileInfoById_NoLock(FileId sFileId);
	Variant getFileInfoByPath(const std::wstring& sPath, Variant vSecurity);
	Variant getFileInfoByParams(Variant vParams);

	ObjPtr<io::IReadableStream> getFileStream(Variant vFileName, Variant vSecurity);
	io::IoSize getFileSize(Variant vFileName, Variant vSecurity);
	std::string getFileHash(Variant vFileName, Variant vSecurity);
	Variant enrichFileHash(Variant vFile);

	Variant getSignatureInfo(Variant vParams);
	Variant enrichSignatureInfo(Variant vFile);

	Variant addVolume(const std::wstring& sDevice, const std::wstring& sVolume, std::string sType);
	Variant findVolume(const std::wstring& sFindStr, bool fFindAll = false);
	Variant getVolumeInfoById_NoLock(FileId sId);
	//Variant getVolumeInfoByGuid(const std::wstring& sVolume);
	Variant getVolumeInfoByDevice(const std::wstring& sDevice);
	Variant getVolumeInfoByPath(const std::wstring& sPath, Variant vToken = {});
	Variant enrichVolumeInfo(Variant vVolume);

	inline std::wstring getDeviceByGuid(const std::wstring& sVolume)
	{
		return queryDosDevice(sVolume.substr(4, 44));
	}

protected:
	
	// Constructor
	FileDataProvider();

	// Destructor
	virtual ~FileDataProvider() override;

public:

	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - the object's configuration including the following fields:
	///   **purgeMask** - purge process start every (purgeMask + 1) unique file query.
	///   **purgeTimeout** - guaranteed time file stay in cache after last access.
	///   **minPurgeCheckTimeout** - minimal time between purge actions.
	///   **receiver** - object for pushing data.
	///   **maxFileSize** - max file size to calculate hash.
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IFileInformation

	/// @copydoc IFileInformation::normalizePathName()
	std::wstring normalizePathName(const std::wstring& sPathName, Variant vSecurity, PathType eType) override;
	/// @copydoc IFileInformation::getFileInfo()
	Variant getFileInfo(Variant vFile) override;
	/// @copydoc IFileInformation::getFileSize()
	io::IoSize getFileSize(Variant vFile) override;
	/// @copydoc IFileInformation::getFileHash()
	std::string getFileHash(Variant vFile) override;
	/// @copydoc IFileInformation::getFileStream()
	ObjPtr<io::IReadableStream> getFileStream(Variant vFile) override;
	/// @copydoc IFileInformation::getVolumeInfo()
	Variant getVolumeInfo(Variant vVolume) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IService

	/// @copydoc IService::loadState()
	void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	Variant saveState() override;
	/// @copydoc IService::start()
	void start() override;
	/// @copydoc IService::stop()
	void stop() override;
	/// @copydoc IService::shutdown()
	void shutdown() override;
};

} // namespace win
} // namespace sys
} // namespace openEdr 

/// @}
