//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (14.03.2019)
// Reviewer: Denis Bogdanov (01.04.2019)
//
///
/// @file File Data Provider implementation
///
/// @addtogroup syswin System library (Windows)
/// @{
#include "pch.h"
#include "filedataprovider.h"

namespace cmd {
namespace sys {
namespace win {

#undef CMD_COMPONENT
#define CMD_COMPONENT "filedtprv"

//
//
//
FileDataProvider::FileDataProvider()
{
}

//
//
//
FileDataProvider::~FileDataProvider()
{
}

//
//
//
void FileDataProvider::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;

	m_nMaxStreamSize = vConfig.get("maxFileSize", g_nMaxStreamSize);
	m_nPurgeFileTimeout = vConfig.get("purgeTimeout", m_nPurgeFileTimeout);
	m_nPurgeSheduleTimeout = vConfig.get("purgeSheduleTimeout", m_nPurgeSheduleTimeout);
	m_sEndpointId = getCatalogData("app.config.license.endpointId", "");
	m_pSignDP = queryInterface<ISignatureInformation>(queryService("signatureDataProvider"));

	Dictionary clFmt = vConfig.get("formats", Dictionary());
	for (auto fmt : clFmt)
	{
		std::stringstream ss(std::string(fmt.second));
		std::string item;
		while (std::getline(ss, item, ';')) {
			m_pFormats[std::move(item)] = fmt.first;
		}
	}

	HMODULE hNtDll = ::GetModuleHandleW(L"ntdll.dll");
	if (hNtDll == nullptr)
		error::RuntimeError(SL, "Can't load ntdll.dll").throwException();
	*(FARPROC*)&m_pNtOpenDirectoryObject = ::GetProcAddress(hNtDll, "NtOpenDirectoryObject");
	*(FARPROC*)&m_pNtQueryDirectoryObject = ::GetProcAddress(hNtDll, "NtQueryDirectoryObject");
	*(FARPROC*)&m_pNtOpenSymbolicLinkObject = ::GetProcAddress(hNtDll, "NtOpenSymbolicLinkObject");
	*(FARPROC*)&m_pNtQuerySymbolicLinkObject = ::GetProcAddress(hNtDll, "NtQuerySymbolicLinkObject");

	// Update volumes
	(void)findVolume(L"", true);

	TRACE_END("Can't create FileDataProvider");
}

//
//
//
FileId FileDataProvider::generateId(const std::wstring& sPath)
{
	crypt::xxh::Hasher ctx;
	crypt::updateHash(ctx, sPath);
	crypt::updateHash(ctx, m_sEndpointId);
	return ctx.finalize();
}

//
// FIXME: All timer procedures shall process exceptions
//
bool FileDataProvider::purgeItems()
{
	if (!m_timerPurge)
		return false;

	LOGLVL(Debug, "Purge started <FileDataProvider>.");

	// All deleted items are collected in local variable to free them outside the lock
	// Because destructor can take time
	std::deque<Variant> pPurgeItems;
	std::unordered_set<FileId> pPurgeIds;

	Size nMapSizeBefore = 0;
	Size nMapSizeAfter = 0;
	Time nMinPurgeTimeFromNow = c_nMaxTime;
	CMD_TRY
	{
		std::unique_lock _lock(m_mtxFileData);
		nMapSizeBefore = m_pPathMap.size();

		Time nCurTime = getCurrentTime();
		for (auto itFile = m_pFileMap.begin(); itFile != m_pFileMap.end();)
		{
			// Break on stop
			if (!m_fInitialized)
				break;

			Time nDelTime = itFile->second["ctx"]["accessTime"];
			if (nCurTime > nDelTime && nCurTime - nDelTime >= m_nPurgeFileTimeout)
			{
				const FileId& sFileId = itFile->first;
				//LOGLVL(Debug, "Purge file item <" << sFileId << ")");
				pPurgeIds.insert(sFileId);

				// Delete file descriptor
				pPurgeItems.push_back(itFile->second);
				itFile = m_pFileMap.erase(itFile);
			}
			else
			{
				if (nCurTime > nDelTime && nMinPurgeTimeFromNow > (nCurTime - nDelTime))
					nMinPurgeTimeFromNow = nCurTime - nDelTime;
				++itFile;
			}
		}

		//LOGLVL(Debug, "Purge checkpoint 1 <FileDataProvider>.");

		// Delete all path links
		for (auto itPath = m_pPathMap.begin(); itPath != m_pPathMap.end();)
		{
			if (pPurgeIds.find(itPath->second) != pPurgeIds.end())
				itPath = m_pPathMap.erase(itPath);
			else
				++itPath;
		}

		//LOGLVL(Debug, "Purge checkpoint 2 <FileDataProvider>.");

		nMapSizeAfter = m_pPathMap.size();
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, "Can't purge in <FileDataProvider>");
	}

	// Reschedule to nearest item + m_nMinPurgeCheckTimeout
	if (nMinPurgeTimeFromNow != c_nMaxTime)
	{
		auto nPurgeSheduleTimeout = m_nPurgeSheduleTimeout;
		if (nMapSizeBefore < 100)
			nPurgeSheduleTimeout = m_nPurgeSheduleTimeout * 3;
		else if (nMapSizeBefore < 1000)
			nPurgeSheduleTimeout = m_nPurgeSheduleTimeout * 2;
		m_timerPurge->reschedule(nMinPurgeTimeFromNow + nPurgeSheduleTimeout);
	}
	else
		m_timerPurge->reschedule(m_nPurgeFileTimeout);

	pPurgeItems.clear();
	pPurgeIds.clear();

	LOGLVL(Debug, "Purge finished <FileDataProvider>. Size before <" 
		<< nMapSizeBefore << ">, size after <" << nMapSizeAfter << ">.");
	return true;
}

// URL: https://docs.microsoft.com/ru-ru/windows/desktop/Memory/obtaining-a-file-name-from-a-file-handle
// URL: http://cbloomrants.blogspot.com/2012/12/12-21-12-file-handle-to-file-name-on.html
// URL: http://pdh11.blogspot.com/2009/05/pathcanonicalize-versus-what-it-says-on.html
// URL: https://stackoverflow.com/questions/65170/how-to-get-name-associated-with-open-handle/5286888#5286888

//
// Wrapper for QueryDosDeviceW
//
std::wstring FileDataProvider::queryDosDevice(const std::wstring& sDrv)
{
	static const size_t c_nMaxDeviceSize = 0x10000;

	std::vector<wchar_t> pDynBuffer;
	wchar_t pStaticBuffer[0x1000]; *pStaticBuffer = 0;
	wchar_t* sNtVolume = pStaticBuffer;
	DWORD nSize = (DWORD)std::size(pStaticBuffer);
	// may return multiple strings!
	// returns very weird strings for network shares
	while (!::QueryDosDeviceW(sDrv.c_str(), sNtVolume, nSize))
	{
		auto ec = GetLastError();
		if (ec != ERROR_INSUFFICIENT_BUFFER || nSize >= c_nMaxDeviceSize)
			error::win::WinApiError(SL, ec, "Fail to query Dos device").throwException();
		nSize *= 2;
		pDynBuffer.resize(nSize);
		sNtVolume = pDynBuffer.data();
	}
	return sNtVolume;
}

//
// Wrapper for GetVolumePathNamesForVolumeNameW
//
std::wstring FileDataProvider::getVolumeMountPoint(const std::wstring& sVolumeName)
{
	static const size_t c_nMaxDeviceSize = 0x10000;

	std::vector<wchar_t> pDynBuffer;
	wchar_t pStaticBuffer[0x1000]; *pStaticBuffer = 0;
	wchar_t* sVolumePaths = pStaticBuffer;
	DWORD nSize = (DWORD)std::size(pStaticBuffer);
	DWORD nReadSize = 0;
	// may return multiple strings!
	while (!::GetVolumePathNamesForVolumeNameW(sVolumeName.c_str(), sVolumePaths, nSize, &nReadSize))
	{
		auto ec = GetLastError();
		if (ec != ERROR_INSUFFICIENT_BUFFER || nSize >= c_nMaxDeviceSize)
			error::win::WinApiError(SL, ec, "Fail to query volume mount point").throwException();
		pDynBuffer.resize(nReadSize);
		sVolumePaths = pDynBuffer.data();
		nSize = nReadSize;
	}
	return sVolumePaths;
}

//
// Wrapper for GetFullPathNameW
//
std::wstring FileDataProvider::getFullPathName(const std::wstring& sPathName)
{
	// Note that GetFullPathNameW is a purely textual operation
	// that eliminates /./ and /../ only.
	std::wstring sFullPath(::GetFullPathNameW(sPathName.c_str(), 0, nullptr, nullptr), 0);
	if (sFullPath.size() == 0)
	{
		LOGWRN("Fail to get full path for <" << string::convertWCharToUtf8(sPathName) << ">");
		return sPathName;
	}

	if (0 == GetFullPathNameW(sPathName.c_str(), DWORD(sFullPath.size()),
		LPWSTR(sFullPath.c_str()), nullptr))
		error::win::WinApiError(SL, FMT("Fail to get long path name <"
			<< string::convertWCharToUtf8(sPathName) << ">")).throwException();

	// Trim last zero
	sFullPath.resize(sFullPath.size() - 1);
	return sFullPath;
}

//
// Wrapper for GetLongPathNameW
//
std::wstring FileDataProvider::getLongPathName(const std::wstring& sPathName)
{
	// Convert to "long" form of path
	// GetLongPathName() validate files and dirs on disk
	// MAY BE SLOW!!!
	std::wstring sLongPath(GetLongPathNameW(sPathName.c_str(), nullptr, 0), 0);
	if (sLongPath.size() == 0)
	{
		LOGWRN("Fail to get long path for <" << string::convertWCharToUtf8(sPathName) << ">");
		return sPathName;
	}

	if (0 == GetLongPathName(sPathName.c_str(), LPWSTR(sLongPath.c_str()), DWORD(sLongPath.size())))
		error::win::WinApiError(SL, FMT("Fail to get long path name for <"
			<< string::convertWCharToUtf8(sPathName) << ">")).throwException();

	// Trim last zero
	sLongPath.resize(sLongPath.size() - 1);
	return sLongPath;

}

//
// Converting examples:
// "\Device\HarddiskVolume3"                                -> "E:"
// "\Device\HarddiskVolume3\Temp"                           -> "E:\Temp"
// "\Device\HarddiskVolume3\Temp\transparent.jpeg"          -> "E:\Temp\transparent.jpeg"
// "\Device\Harddisk1\DP(1)0-0+6\foto.jpg"                  -> "I:\foto.jpg"
// "\Device\TrueCryptVolumeP\Data\Passwords.txt"            -> "P:\Data\Passwords.txt"
// "\Device\Floppy0\Autoexec.bat"                           -> "A:\Autoexec.bat"
// "\Device\CdRom1\VIDEO_TS\VTS_01_0.VOB"                   -> "H:\VIDEO_TS\VTS_01_0.VOB"
// "\Device\Serial1"                                        -> "COM1"
// "\Device\USBSER000"                                      -> "COM4"
// "\Device\Mup\ComputerName\C$\Boot.ini"                   -> "\\ComputerName\C$\Boot.ini"
// "\Device\LanmanRedirector\ComputerName\C$\Boot.ini"      -> "\\ComputerName\C$\Boot.ini"
// "\Device\LanmanRedirector\ComputerName\Shares\Dance.m3u" -> "\\ComputerName\Shares\Dance.m3u"
//
// Returns input path for any other device type
//
std::wstring FileDataProvider::convertNtPathToDosPath(std::wstring sPathName)
{
	TRACE_BEGIN
	sPathName = parseObjectDirectory(sPathName);

	// Parse port alliases
	if (string::startsWith(sPathName, L"\\Device\\Serial", true) ||	// e.g. "Serial1"
		string::startsWith(sPathName, L"\\Device\\UsbSer", true))		// e.g. "USBSER000"
	{
		std::wstring sKeyPath(L"Hardware\\DeviceMap\\SerialComm");
		wchar_t sComPort[0x100]; *sComPort = 0;
		DWORD nKeySize = sizeof(sComPort);
		if (::RegGetValueW(HKEY_LOCAL_MACHINE, sKeyPath.c_str(),
			sPathName.c_str(), RRF_RT_REG_SZ, NULL, (PVOID)sComPort, &nKeySize))
			error::win::WinApiError(SL,	"Fail to get registry value").throwException();
		return sComPort;
	}

	if (string::startsWith(sPathName, L"\\Device\\LanmanRedirector\\", true)) // Win XP
		return std::wstring(L"\\\\") + sPathName.substr(25);

	if (string::startsWith(sPathName, L"\\Device\\Mup\\", true)) // Win 7
		return std::wstring(L"\\\\") + std::wstring(sPathName.substr(12));

	static const Size c_nDriveLetterSize = 4; // "c:\" + 0x00
	// 26 letters + 1
	wchar_t sDrives[27 * c_nDriveLetterSize]; *sDrives = 0;
	size_t nRetSize = ::GetLogicalDriveStringsW(DWORD(std::size(sDrives)), sDrives);
	if (nRetSize == 0 || nRetSize > std::size(sDrives))
		error::win::WinApiError(SL, "Fail to get logical drive strings").throwException();

	wchar_t* sDrv = sDrives;
	while (sDrv[0])
	{
		wchar_t* sNextDrv = sDrv + c_nDriveLetterSize;
		sDrv[2] = 0; // the backslash is not allowed for QueryDosDevice()

		std::wstring sDeviceName = queryDosDevice(sDrv);
		if (!sDeviceName.empty() &&
			string::startsWith(sPathName, sDeviceName) &&
			(sPathName[sDeviceName.size()] == L'\\' || sPathName[sDeviceName.size()] == L'\0'))
			return std::wstring(sDrv) + sPathName.substr(sDeviceName.length());

		sDrv = sNextDrv;
	}
	return sPathName;

	TRACE_END(FMT("Fail to normalize file path <" <<
		string::convertWCharToUtf8(sPathName) << ">"));
}

//
// Q: Why I have to prepare PUNICODE_STRING parameter outside?
// A: We already have PUNICODE_STRING returned from previous function
//
std::wstring FileDataProvider::getSymbolicLink(HANDLE hDirectory, PUNICODE_STRING pName)
{
	OBJECT_ATTRIBUTES attrLink;
	InitializeObjectAttributes(&attrLink, pName, 0, hDirectory, nullptr);
	ScopedHandle hLink;
	if (!NT_SUCCESS(m_pNtOpenSymbolicLinkObject(&hLink, GENERIC_READ, &attrLink)))
		error::NotFound(SL, FMT("Can't open symbolic link object")).throwException();
	WCHAR pLinkBuffer[MAX_PATH] = { 0 };
	UNICODE_STRING usTarget;
	RtlInitUnicodeString(&usTarget, pLinkBuffer);
	usTarget.MaximumLength = (MAX_PATH - 1) * 2;
	ULONG len;
	if (!NT_SUCCESS(m_pNtQuerySymbolicLinkObject(hLink, &usTarget, &len)))
		error::NoData(SL, FMT("Can't query symbolic link object")).throwException();
	return usTarget.Buffer;
}

//
//
//
bool FileDataProvider::parseObjectDirectory(const std::wstring& sLink, std::wstring& sOutStr)
{
	// Check cache
	for (auto& itItem : m_pObjectsMap)
	{
		if (!string::startsWith(sLink, itItem.first) ||
			(sLink[itItem.first.size()] != L'\\' && sLink[itItem.first.size()] != L'\0'))
			continue;
		if (itItem.second.empty())
			return false;
		sOutStr = itItem.second;
		return true;
	}

	std::wstring sRoot(L"\\");
	Byte pBuffer[0x1000]; *pBuffer = 0;

	bool fDirectory = true;
	while (fDirectory)
	{
		ScopedHandle hDirectory;
		OBJECT_ATTRIBUTES attr;
		UNICODE_STRING name;
		RtlInitUnicodeString(&name, sRoot.c_str());
		InitializeObjectAttributes(&attr, &name, 0, nullptr, nullptr);
		auto nStatus = m_pNtOpenDirectoryObject(&hDirectory, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &attr);
		if (nStatus < 0)
		{
			error::win::WinApiError(SL, FMT("Fail to open object directory <" << 
				string::convertWCharToUtf8(sRoot) << ">")).log();
			break;
		}
		
		bool fAddSlash = (sRoot.back() != L'\\');

		ULONG nIndex = 0;
		ULONG nStart = 0;
		BOOLEAN firstEntry = TRUE;
		POBJECT_DIRECTORY_INFORMATION pObjDirInfo = nullptr;
		do
		{
			ULONG nBytes = 0; 
			// We do not need to check size, because we have number of elements in nIndex
			// If function fail, nBytes do not say us how many bytes we need to allocate
			// That's why we pass static buffer big enough to get portion of objects 
			nStatus = m_pNtQueryDirectoryObject(hDirectory, pBuffer, ULONG(std::size(pBuffer)), FALSE, firstEntry, &nIndex, &nBytes);
			if (nStatus < 0)
			{
				pObjDirInfo = nullptr;
				// TODO: Need to correctly handle negative NTSTATUS results
				// URL: https://stackoverflow.com/questions/25566234/how-to-convert-specific-ntstatus-value-to-the-hresult
				error::NoData(SL, FMT("Fail to query object directory <" <<
					string::convertWCharToUtf8(sRoot) << ">")).log();
				break;
			}
			firstEntry = FALSE;

			pObjDirInfo = (POBJECT_DIRECTORY_INFORMATION)pBuffer;
			while (nStart < nIndex)
			{
				// Q: Is it necessary to create new local variable?
				// A: Yes. It will be created automaticaly, when we pass concatination of strings to startsWith()
				//    And we use it twice to assign new sRoot value
				std::wstring sNewRoot(sRoot + (fAddSlash ? L"\\" : L"") + pObjDirInfo->Name.Buffer);
// 				if (sNewRoot == L"\\GLOBAL??")
// 					sNewRoot = L"\\??";
				if (string::startsWith(sLink, sNewRoot) &&
					(sLink[sNewRoot.size()] == L'\\' || sLink[sNewRoot.size()] == L'\0'))
				{
					sRoot = sNewRoot;
					break;
				}
				// Q: I'm not sure that we can use simple iterator here
				// A: Yes, pObjDirInfo points to array of OBJECT_DIRECTORY_INFORMATION items
				++pObjDirInfo;
				++nStart;
			}
			if (nStart == nIndex)
				pObjDirInfo = nullptr;
			// Q: Which status can you check here?
			// A: Status of NtQueryDirectoryObject call. Negative - error, zero - no more items,
			//    positive - there is more items to query.
		} while (nStatus != 0 && pObjDirInfo == nullptr);

		if (pObjDirInfo == nullptr)
			break;

		// Q: Why do you think that Directory objects are sorted and first?
		// A: They are not sorted. Here we check not first element, but single item found.
		if (wcscmp(pObjDirInfo->TypeName.Buffer, L"Directory") == 0)
			continue;

		fDirectory = false;

		if (wcscmp(pObjDirInfo->TypeName.Buffer, L"SymbolicLink") == 0)
		{
			auto sLinkTo = getSymbolicLink(hDirectory, &pObjDirInfo->Name);
			sOutStr = sLinkTo + std::wstring(sLink.substr(sRoot.size()));
			m_pObjectsMap[sRoot] = sLinkTo;
			return true;
		}

		m_pObjectsMap[sRoot] = L"";
	}

	return false;
}

//
//
//
std::wstring FileDataProvider::parseObjectDirectory(const std::wstring& sPathName)
{
	std::wstring sNtPath(sPathName);
	std::wstring sOutPath;
	while (parseObjectDirectory(sNtPath, sOutPath))
		sNtPath = sOutPath;
	return sNtPath;
}

//
//
//
std::wstring FileDataProvider::normalizePathManualy(std::wstring sPathName, 
	Variant vSecurity, bool fDos)
{
	TRACE_BEGIN
	// Convert to long Win32 path
	if (string::startsWith(sPathName, L"\\??\\") || string::startsWith(sPathName, L"\\\\.\\"))
	{
		// Q: Very strange changes - you get different results depending on input
		// A: We convert ANY input string to one generic format: "\\?\"
		//    Universal format of long paths that Win32 API understand
		sPathName[1] = L'\\';
		sPathName[2] = L'?';
	}
	else if (string::startsWith(sPathName, L"\\DosDevices\\", true))
	{
		sPathName.erase(1, 8);
		sPathName[1] = L'\\';
		sPathName[2] = L'?';
	}
	else if (iswalpha(sPathName[0]) && sPathName[1] == ':')
		sPathName = std::wstring(L"\\\\?\\") + sPathName;
	
	/** Get rid of \\?\UNC on drive-letter and UNC paths */
	if (string::startsWith(sPathName, L"\\\\?\\UNC\\", true))
	{
		if (sPathName.size() > 9 && isalpha(sPathName[8]) && sPathName[9] == L':')
			sPathName.erase(4, 4);
		else
			// return network UNC path
			// Q: strange code... you transformed it from "\\??\\" format above
			// A: We always has output string in format of "\??\"
			//    All this transformations do in place without resizing.
			//    And it is match faster then do additional comparisons without transformation
			sPathName[1] = L'?';
			return sPathName;
	}

	if (string::startsWith(sPathName, L"\\\\?\\"))
	{
		// Not understood
		if (sPathName.length() < 6 ||
			!iswalpha(sPathName[4]) || sPathName[5] != ':')
			error::InvalidFormat(SL, FMT("Unknown path format <" <<
				string::convertWCharToUtf8(sPathName) << ">")).throwException();

		// QueryDosDevice and WNetGetConnection FORBID the trailing slash; 
		// GetDriveType REQUIRES it.
		wchar_t sDrive[4] = { L"A:\\" };

		// Convert from short 8.3 names to long
		bool fConvert = true;

		// unwind SUBST'ing
		while (true)
		{
			ScopedImpersonation fImpers;
			sDrive[0] = (wchar_t)toupper(sPathName[4]);
			UINT nDT = ::GetDriveTypeW(sDrive);
			if (nDT == DRIVE_NO_ROOT_DIR && !vSecurity.isEmpty())
			{
				fImpers.reset(impersonateThread(vSecurity));
				if (fImpers)
					nDT = ::GetDriveTypeW(sDrive);
			}
			if (nDT == DRIVE_UNKNOWN || nDT == DRIVE_NO_ROOT_DIR)
			{
				sPathName[1] = L'?';
				break;
			}

			sDrive[2] = 0;
			if (nDT == DRIVE_REMOTE)
			{
				DWORD nPathLen = 0;
				DWORD nResult = ::WNetGetConnectionW(sDrive, NULL, &nPathLen);
				std::wstring sNetPath(nPathLen, 0);
				nResult = ::WNetGetConnectionW(sDrive, sNetPath.data(), &nPathLen);
				sNetPath.resize(sNetPath.size() - 1);
				sPathName = sNetPath + sPathName.substr(6);
				break;
			}

			if (fConvert)
			{
				// Convert from relative to absolute
				sPathName = getFullPathName(sPathName);
				// Convert from short 8.3 names to long
				sPathName = getLongPathName(sPathName);
				fConvert = false;
			}

			std::wstring sDosDevice = queryDosDevice(sDrive);
			if (string::startsWith(sDosDevice, L"\\??\\") &&
				iswalpha(sPathName[4]) && sPathName[5] == ':')
			{
				// SUBST drive found
				sDosDevice[1] = L'\\';
				sPathName = sDosDevice + sPathName.substr(6);
				continue;
			}

			if (fDos)
				sPathName[1] = L'?';
			else
				sPathName = sDosDevice + sPathName.substr(6);
			break;
		}

		return sPathName;
	}
	else if (sPathName[0] == L'\\' && sPathName[1] == L'\\')
		// Convert network path to UNC
		return std::wstring(L"\\??\\UNC\\") + sPathName.substr(2);
	else if (string::startsWith(sPathName, L"\\Device\\Mup\\", true))
		return std::wstring(L"\\??\\UNC\\") + sPathName.substr(12);
	else if (string::startsWith(sPathName, L"\\Device\\LanmanRedirector\\", true))
		return std::wstring(L"\\??\\UNC\\") + sPathName.substr(25);

	return sPathName;
	TRACE_END(FMT("Fail to normalize file: " << string::convertWCharToUtf8(sPathName)))
}

//
//
//
std::wstring FileDataProvider::normalizePathNameInt(const std::wstring& sPathName, Variant vSecurity, PathType eType)
{
	TRACE_BEGIN
#ifdef FDP_USE_NORMALIZATION_API
	ScopedFileHandle hFile(getFileHandle(sPathName, FILE_READ_ATTRIBUTES));

	ScopedImpersonation fImpers;
	if (!hFile && !vSecurity.isEmpty())
	{
		fImpers.reset(impersonateThread(vSecurity));
		if (fImpers)
			hFile.reset(getFileHandle(sPathName, FILE_READ_ATTRIBUTES));
	}

	if (hFile)
	{
		DWORD nFlags = VOLUME_NAME_NONE;
		if (eType == PathType::HumanReadable || eType == PathType::Retry)
			nFlags = VOLUME_NAME_DOS;
		else if (eType == PathType::Unique)
			nFlags = VOLUME_NAME_GUID;
		else if (eType == PathType::NtPath)
			nFlags = VOLUME_NAME_NT;

		wchar_t sFileName[0x1000]; *sFileName = 0;
		DWORD nNameLen = DWORD(std::size(sFileName));
		nNameLen = ::GetFinalPathNameByHandleW(hFile, sFileName, nNameLen - 1, nFlags);
		if (nNameLen == 0)
		{
			auto ec = GetLastError();
			if (eType != PathType::Unique)
				LOGWRN("Fail to normalize file (err: " << hex(ec) << ") <" << string::convertWCharToUtf8(sPathName) << ">");
			else
			{
				// Retry to get path for network shares
				auto sNormalName(normalizePathNameInt(sPathName, vSecurity, PathType::Retry));
				if (!sNormalName.empty())
					return sNormalName;
			}
		}
		else if (nNameLen < std::size(sFileName))
			return sFileName;
		else
		{
			std::wstring sFileNameDyn(nNameLen, 0);
			if (::GetFinalPathNameByHandleW(hFile, sFileNameDyn.data(), nNameLen - 1, nFlags) != nNameLen - 1)
				error::win::WinApiError(SL, "Fail to normalize file").throwException();
			sFileNameDyn.resize(nNameLen - 1);
			return sFileNameDyn;
		}
	}
	else
		LOGLVL(Trace, "Fail to open file for normalization <" << string::convertWCharToUtf8(sPathName) << ">");

	fImpers.reset();
	if (eType == PathType::Retry)
		return {};
#endif // FDP_USE_NORMALIZATION_API

	// Convert path manually
	if (eType == PathType::HumanReadable)
	{
		std::wstring sDosPath;
		if (string::startsWith(sPathName, L"\\\\?\\Volume{", true))
			sDosPath = convertNtPathToDosPath(getDeviceByGuid(sPathName) + sPathName.substr(48));
		else if (string::startsWith(sPathName, L"\\Device\\", true))
			sDosPath = convertNtPathToDosPath(sPathName);
		else if (string::startsWith(sPathName, L"\\SystemRoot\\", true))
			sDosPath = convertNtPathToDosPath(parseObjectDirectory(sPathName));
		else
			sDosPath = normalizePathManualy(sPathName, vSecurity, true);
		return sDosPath;
	}
	else
	{
		if (string::startsWith(sPathName, L"\\\\?\\Volume{", true))
			return sPathName;
		std::wstring sNtPath(normalizePathManualy(sPathName, vSecurity, false));
		sNtPath = parseObjectDirectory(sNtPath);
		static const wchar_t c_sHddVol[] = L"\\Device\\HarddiskVolume";
		if (!string::startsWith(sNtPath, c_sHddVol, true) ||
			eType == PathType::NtPath)
			return sNtPath;
		auto nSlashPos = sNtPath.find(L'\\', std::size(c_sHddVol));
		if (nSlashPos == sNtPath.npos)
			return sNtPath;
		Variant vVolume = getVolumeInfoByDevice(sNtPath.substr(0, nSlashPos));
		if (!vVolume.isEmpty() && vVolume.has("guid"))
			sNtPath.replace(0, nSlashPos, vVolume["guid"]);
		return sNtPath;
	}

	TRACE_END(FMT("Fail to normalize file: " << string::convertWCharToUtf8(sPathName)))
}

//
//
//
std::wstring FileDataProvider::normalizePathName(const std::wstring& sPathName, Variant vSecurity, PathType eType)
{
	CMD_TRY
	{
		std::wstring sFullPath = normalizePathNameInt(sPathName, vSecurity, eType);
		if (eType == PathType::HumanReadable)
		{
			// Q: Why do you do additional transformations here again?
			// A: In 'HumanReadable' format we cut any prefixes
			//    And do not transform to upper or lower cases
			if (string::startsWith(sFullPath, L"\\??\\") || string::startsWith(sFullPath, L"\\\\?\\"))
			{
				sFullPath.erase(0, 4);
				if (string::startsWith(sFullPath, L"UNC\\", true))
				{
					sFullPath.erase(0, 2);
					sFullPath[0] = L'\\';
				}
			}
		}
		else
		{
			if (eType == PathType::NtPath &&
				(string::startsWith(sFullPath, L"\\??\\UNC", true) ||
				string::startsWith(sFullPath, L"\\\\?\\UNC", true)))
				sFullPath.replace(0, 7, L"\\device\\mup");

			// Transform to lowercase
			sFullPath = string::convertToLow(sFullPath);
		}
		return sFullPath;
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Fail to normalize path <" << string::convertWCharToUtf8(sPathName) << ">"));
	}

	return sPathName;
}

//
//
//
bool FileDataProvider::hasFileInfo(const FileId& sFileId)
{
	std::shared_lock _lock(m_mtxFileData);
	return m_pFileMap.find(sFileId) != m_pFileMap.end();
}

//
//
//
Variant FileDataProvider::getFileInfoById(FileId sFileId)
{
	std::shared_lock _lock(m_mtxFileData);
	return getFileInfoById_NoLock(sFileId);
}

//
// Always use mutex before calling this function
//
Variant FileDataProvider::getFileInfoById_NoLock(FileId sFileId)
{
	auto itFile = m_pFileMap.find(sFileId);
	if (itFile == m_pFileMap.end())
		error::NotFound(SL, FMT("Can't find file <" << sFileId << ">")).throwException();

	// Update file access
	auto vCtx = itFile->second["ctx"];
	vCtx.put("accessTime", getCurrentTime());
	return itFile->second;
}

//
//
//
Variant FileDataProvider::getFileInfoByPath(const std::wstring& sRawPath, Variant vSecurity)
{
	TRACE_BEGIN
	// Try to find by sRawPath
	{
		std::shared_lock _lock(m_mtxFileData);
		auto itFile = m_pPathMap.find(sRawPath);
		if (itFile != m_pPathMap.end())
			return getFileInfoById_NoLock(itFile->second);
	}

	std::wstring sUniquePath = normalizePathName(sRawPath, vSecurity, PathType::Unique);
	FileId sFileId = generateId(sUniquePath);

	// Try to find by sFileId
	{
		std::unique_lock _lock(m_mtxFileData);
		if (m_pFileMap.find(sFileId) != m_pFileMap.end())
		{
			m_pPathMap[sRawPath] = sFileId;
			//LOGLVL(Trace, "Link file. Path <" << string::convertWCharToUtf8(sRawPath) << ">, id <" << sFileId << ">");
			return getFileInfoById_NoLock(sFileId);
		}
	}

	// Try to add new file descriptor
	auto sViewPath(normalizePathName(sRawPath, vSecurity, PathType::HumanReadable));
	auto vFile = Dictionary({
		{"id", sFileId},
		{"uniquePath", sUniquePath},
		{"path", sViewPath},
		{"type", getFileType(sUniquePath)}
	});

	auto vCtx = Dictionary({
		{"security", vSecurity},
		{"accessTime", getCurrentTime()}
	});

	auto vFileInfo = Dictionary({ 
		{"data", vFile},
		{"ctx", vCtx}
	});

	{
		// Possible other thread already has added the same file
		std::unique_lock _lock(m_mtxFileData);
		auto itFile = m_pFileMap.find(sFileId);
		if (itFile != m_pFileMap.end() && !itFile->second.isNull())
			return itFile->second;

		// Add the file into maps
		m_pFileMap[sFileId] = vFileInfo;
		m_pPathMap[sRawPath] = sFileId;
//		m_pPathMap[sUniquePath] = sFileId;
//		m_pPathMap[sViewPath] = sFileId;
	}

	LOGLVL(Debug, "New file: path <" << string::convertWCharToUtf8(sRawPath) <<	">, id <" << sFileId << ">");

	return vFileInfo;
	TRACE_END(FMT("Fail to get info for file"))
}

//
//
//
Variant FileDataProvider::getFileInfoByParams(Variant vParams)
{
	Variant vSecurity = vParams.get("security", {});
	if (vParams.has("id") && hasFileInfo(vParams["id"]))
	{
		std::shared_lock _lock(m_mtxFileData);
		return getFileInfoById_NoLock(vParams["id"]);
	}
	// uniquePath and rawPath may come from SYSMON events
	if (vParams.has("uniquePath"))
		return getFileInfoByPath(vParams["uniquePath"], vSecurity);
	if (vParams.has("rawPath"))
		return getFileInfoByPath(vParams["rawPath"], vSecurity);
	if (vParams.has("path"))
		return getFileInfoByPath(vParams["path"], vSecurity);
	error::InvalidArgument(SL, "Invalid arguments for <getRawFileInfo>").throwException();
}

//
// Q: In this case we can use string_view
// A: This way we need to split extension manually
//
cmd::Variant FileDataProvider::getFileType(std::filesystem::path sPath)
{
	auto sExt(sPath.extension().u8string());
	auto format = m_pFormats.find(sExt);
	if (format == m_pFormats.end())
		return "OTHER";
	return format->second;
}

//
// Update event depended fields
//
void FileDataProvider::updateFileInfo(Variant vFileInfo, Variant vParams)
{
	auto vFile = vFileInfo["data"];

	// Mark file deleted
	if (vParams.get("cmdRemove", false))
		vFile.put("deleted", true);

	// Set file type
	if (vParams.get("cmdExecute", false))
		vFile.put("type", "EXECUTABLE");

	// Invalidate file hash on change
	if (vParams.get("cmdModify", false) ||
		(vParams.get("cmdExecute", false) && vFile.has("deleted")))
	{
		vFile.erase("deleted");
		auto oldSecurity = vFileInfo["ctx"]["security"];
		vFileInfo.put("ctx", Dictionary({
			{"security", vParams.get("security", oldSecurity)},
			{"accessTime", getCurrentTime()} }));
	}
}

//
//
//
Variant FileDataProvider::getFileInfo(Variant vParams)
{
	Variant vFileInfo;
	std::wstring sRawPath;
	
	Variant vSecurity = vParams.get("security", {});
	if (vParams.has("id") && hasFileInfo(vParams["id"]))
	{
		std::shared_lock _lock(m_mtxFileData);
		vFileInfo = getFileInfoById_NoLock(vParams["id"]);
	}
	else
	{
		// uniquePath and rawPath may come from SYSMON events
		sRawPath = vParams.get("uniquePath", L"");
		if (sRawPath.empty())
			sRawPath = vParams.get("rawPath", L"");
		if (sRawPath.empty())
			sRawPath = vParams.get("path");
		vFileInfo = getFileInfoByPath(sRawPath, vSecurity);
	}

	updateFileInfo(vFileInfo, vParams);

	//////////////////////////////////////////////////////////////////////////
	// Update variable fields

	// Clone descriptor
	Variant vFile = vFileInfo["data"].clone();

	// Copy calculated fields
	auto vCtx = vFileInfo["ctx"];
	if (vCtx.has("hash"))
		vFile.put("hash", vCtx["hash"]);
	else
		vFile.put("@ctx", vCtx);
	if (vCtx.has("signature"))
		vFile.put("signature", vCtx["signature"]);
	else
		vFile.put("@ctx", vCtx);
	if (vCtx.has("size"))
		vFile.put("size", vCtx["size"]);
	else
	{
		auto pThis = getPtrFromThis(this);
		vFile.put("size", variant::createLambdaProxy([pThis, vParams]()->Variant
		{
			return pThis->getFileSize(vParams);
		}, true));
	}

	// Volume information can dynamically change
	Variant vVolume = vParams.has("volume") ? 
		enrichVolumeInfo(vParams["volume"]) : 
		getVolumeInfoByPath(vFile["uniquePath"], vSecurity);
	if (!vVolume.isEmpty())
		vFile.put("volume", vVolume);

	// Abstract path is user dependent. Put it here.
	Variant vUniquePath = vFile["uniquePath"];
	vFile.put("abstractPath", variant::createLambdaProxy([vUniquePath, vSecurity]() -> Variant
	{
		auto pObj = queryInterface<IPathConverter>(queryService("pathConverter"));
		return pObj->getAbstractPath(vUniquePath, vSecurity);
	}, true));

	// Copy rawHash
	if (vParams.has("rawHash"))
		vFile.put("rawHash", vParams["rawHash"]);

	// Add source file path
	if (!sRawPath.empty())
		vFile.put("rawPath", sRawPath);
	return vFile;
}

//
//
//
ObjPtr<io::IReadableStream> FileDataProvider::getFileStream(Variant sFileName, Variant vSecurity)
{
	error::ErrorCode ec = error::ErrorCode::OK;
//	std::filesystem::path sFileName(vFile.get("uniquePath"));
	ObjPtr<io::IReadableStream> pStream = io::win::createFileStreamSafe(sFileName.convertToPath(),
		io::FileMode::Read | io::FileMode::ShareRead | io::FileMode::ShareWrite, &ec);

	if (pStream == nullptr && !vSecurity.isEmpty())
	{
		ScopedImpersonation fImpers(impersonateThread(vSecurity));
		LOGLVL(Debug, "Try to open stream using impersonation <" << (fImpers ? "true" : "false") << ">");
		if (fImpers)
			pStream = io::win::createFileStreamSafe(sFileName.convertToPath(),
				io::FileMode::Read | io::FileMode::ShareRead | io::FileMode::ShareWrite, &ec);
	}

	if (pStream == nullptr)
		LOGLVL(Debug, "Fail to open file <" << sFileName << ">, error <" << ec << ">");
	return pStream;
}

//
//
//
ObjPtr<io::IReadableStream> FileDataProvider::getFileStream(Variant vParams)
{
	TRACE_BEGIN
	auto vFileInfo = getFileInfoByParams(vParams);
	auto vFile = vFileInfo["data"];
	if (vFile.has("deleted"))
	{
		LOGLVL(Debug, "Can't return stream for removed file <" << vFile["id"] << ">");
		return nullptr;
	}

	auto vCtx = vParams.get("@ctx", vFileInfo["ctx"]);
	return getFileStream(vFile["uniquePath"], vParams.get("security", vCtx["security"]));
	TRACE_END("Fail to get file stream")
}

//
//
//
io::IoSize FileDataProvider::getFileSize(Variant sFileName, Variant vSecurity)
{
	auto pStream = getFileStream(sFileName, vSecurity);;
	if (pStream == nullptr)
		return 0;

	return pStream->getSize();
}

//
//
//
io::IoSize FileDataProvider::getFileSize(Variant vParams)
{
	TRACE_BEGIN
	auto vFileInfo = getFileInfoByParams(vParams);
	auto vFile = vFileInfo["data"];
	if (vFile.has("deleted"))
	{
		LOGLVL(Debug, "Can't calculate size for removed file <" << vFile["id"] << ">");
		return 0;
	}

	auto vCtx = vParams.get("@ctx", vFileInfo["ctx"]);
	if (vCtx.has("size"))
		return vCtx["size"];

	auto size = getFileSize(vFile["uniquePath"], vParams.get("security", vCtx["security"]));
	vCtx.put("size", size);
	return size;
	TRACE_END("Fail to get file size")
}

//
//
//
std::string FileDataProvider::getFileHash(Variant vFileName, Variant vSecurity)
{
	auto pStream = getFileStream(vFileName, vSecurity);
	if (pStream == nullptr)
		return "";

	auto nStreamSize = pStream->getSize();
	if (nStreamSize > m_nMaxStreamSize)
	{
		LOGLVL(Debug, "Skip hash calculation");
		return "";
	}

	auto pHash = crypt::sha1::getHash(pStream);
	return string::convertToHex(std::begin(pHash.byte), std::end(pHash.byte));
}

//
//
//
std::string FileDataProvider::getFileHash(Variant vParams)
{
	TRACE_BEGIN
	std::string hash;
	auto vFileInfo = getFileInfoByParams(vParams);
	auto vFile = vFileInfo["data"];
	if (vFile.has("deleted"))
	{
		LOGLVL(Debug, "Can't calculate hash for removed file <" << vFile["id"] << ">");
		return hash;
	}

	Variant vCtx;
	FileId sFileId = vFile["id"];
	{
		std::unique_lock<std::mutex> sync(m_mtxCtxData);
		while (m_pCtxHashQueue.find(sFileId) != m_pCtxHashQueue.end())
			m_cvCtxData.wait(sync);

		vCtx = vParams.get("@ctx", vFileInfo["ctx"]);
		if (vCtx.has("hash"))
			return vCtx["hash"];

		m_pCtxHashQueue.insert(sFileId);
	}
	CMD_TRY
	{
		hash = getFileHash(vFile["uniquePath"], vParams.get("security", vCtx["security"]));
		vCtx.put("hash", hash);
		LOGLVL(Debug, "File hash: id <" << sFileId <<
			">, file <" << vFile.get("path", "") <<
			">, hash <" << std::string(hash) << ">");
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		LOGLVL(Debug, "Fail to calculate hash: id <" << sFileId <<
			">, file <" << vFile.get("path", "") <<
			">, error <" << ex.what() << ">");
	}
	{
		std::unique_lock<std::mutex> sync(m_mtxCtxData);
		m_pCtxHashQueue.erase(sFileId);
		m_cvCtxData.notify_all();
	}
	return hash;
	TRACE_END("Fail to cals file hash")
}

//
//
//
Variant FileDataProvider::enrichFileHash(Variant vFile)
{
	TRACE_BEGIN
	vFile.put("hash", getFileHash(vFile));
	return vFile;
	TRACE_END("Fail to enrich file hash")
}

//
//
//
Variant FileDataProvider::getSignatureInfo(Variant vParams)
{
	TRACE_BEGIN
	if (m_pSignDP == nullptr)
		return Dictionary();
	
	auto signature = Dictionary({ {"status", SignStatus::Undefined} });
	auto vFileInfo = getFileInfoByParams(vParams);
	auto vFile = vFileInfo["data"];
	if (vFile.has("deleted"))
	{
		LOGLVL(Debug, "Skip signature verification for removed file <" << vFile["id"] << ">");
		return signature;
	}

	Variant vCtx;
	FileId sFileId = vFile["id"];
	{
		std::unique_lock<std::mutex> sync(m_mtxCtxData);
		while (m_pCtxSignQueue.find(sFileId) != m_pCtxSignQueue.end())
			m_cvCtxData.wait(sync);

		// Check file context
		vCtx = vParams.get("@ctx", vFileInfo["ctx"]);
		if (vCtx.has("signature"))
			return vCtx["signature"];

		m_pCtxSignQueue.insert(sFileId);
	}
	CMD_TRY
	{
		signature = m_pSignDP->getSignatureInfo(vFile["uniquePath"],
			vParams.get("security", vCtx["security"]), getFileHash(vParams));
		if (!signature.isEmpty())
			vCtx.put("signature", signature);
		LOGLVL(Debug, "File sign: id <" << sFileId <<
			">, file <" << vFile.get("path", "") <<
			">, status <" << signature.get("status", SignStatus::Undefined) <<
			"> , vendor <" << signature.get("vendor", "") << ">");
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.log(SL, FMT("Fail to parse signature: id <" << sFileId <<
			">, file <" << vFile.get("path", "") <<
			">, error <" << ex.what() << ">"));
// 		LOGLVL(Debug, "Fail to parse signature: id <" << sFileId <<
// 			">, file <" << vFile.get("path", "") <<
// 			">, error <" << ex.what() << ">");
	}
	{
		std::unique_lock<std::mutex> sync(m_mtxCtxData);
		m_pCtxSignQueue.erase(sFileId);
		m_cvCtxData.notify_all();
	}
	return signature;
	TRACE_END("Fail to get file signature")
}

//
//
//
Variant FileDataProvider::enrichSignatureInfo(Variant vFile)
{
	TRACE_BEGIN
	if (m_pSignDP == nullptr)
		return {};
	vFile.put("signature", getSignatureInfo(vFile));
	return vFile;
	TRACE_END("Fail to enrich file signature")
}


//
// Should be called with locked m_mtxVolumeData
//
Variant FileDataProvider::addVolume(const std::wstring& sDevice, const std::wstring& sGuid, std::string sType)
{
	Dictionary vVolume;
	auto nId = generateId(sDevice);
	vVolume.put("id", nId);
	vVolume.put("device", sDevice);
	if (!sGuid.empty())
		vVolume.put("guid", sGuid);
	vVolume.put("name", convertNtPathToDosPath(sDevice));
	if (sType.empty())
	{
		// Function GetDriveTypeW require trailing slash 
		switch (GetDriveTypeW((sGuid + L"\\").c_str()))
		{
		case DRIVE_REMOVABLE:
		case DRIVE_CDROM:
		case DRIVE_RAMDISK:
			sType = "REMOVABLE";
			break;
		case DRIVE_FIXED:
			sType = "FIXED";
			break;
		case DRIVE_REMOTE:
			sType = "NETWIRK";
			break;
		default:
			sType = "UNKNOWN";
			break;
		}
	}
	vVolume.put("type", sType);

	{
		std::scoped_lock _lock(m_mtxVolumeData);
		m_pDeviceMap[string::convertToLow(sDevice)] = nId;
		m_pVolumeMap[nId] = vVolume;
	}
	LOGLVL(Debug, "New volume: id <" << nId << ">, device <" << string::convertWCharToUtf8(sDevice) << 
		">, guid <" << string::convertWCharToUtf8(sGuid) << ">, type <" << sType << ">");
	LOGLVL(Trace, FMT("Volume:\n" << vVolume.print()));
	return vVolume;
}

//
// Should be called with locked m_mtxVolumeData
//
Variant FileDataProvider::findVolume(const std::wstring& sFindStr, bool fFindAll)
{
	CMD_TRY
	{
		if (sFindStr.empty() && !fFindAll)
			error::InvalidArgument(SL).throwException();

		wchar_t sVolumeName[MAX_PATH]; *sVolumeName = 0;
		ScopedVolumeHandle hHandle(::FindFirstVolumeW(sVolumeName, (DWORD)std::size(sVolumeName)));
		if (!hHandle)
			error::win::WinApiError(SL, "Fail to find first volume").throwException();

		while (hHandle)
		{
			auto nVolumeLen = wcslen(sVolumeName);
			if (nVolumeLen < 4 || sVolumeName[nVolumeLen - 1] != L'\\')
				error::InvalidFormat(SL, FMT("Incorrect volume <" << string::convertWCharToUtf8(sVolumeName) << "L")).throwException();
			// Function QueryDosDeviceW do not accept trailing slash
			sVolumeName[nVolumeLen - 1] = L'\0';
			auto sDeviceName = queryDosDevice(sVolumeName + 4);

			if (fFindAll)
				(void)addVolume(sDeviceName, sVolumeName, "");
			else if (_wcsnicmp(sFindStr.c_str(), sDeviceName.c_str(), sDeviceName.length()) == 0 &&
				(sFindStr.length() == sDeviceName.length() || sFindStr[sDeviceName.length()] == L'\\'))
					return addVolume(sDeviceName, sVolumeName, "");

			if (!::FindNextVolumeW(hHandle, sVolumeName, (DWORD)std::size(sVolumeName)))
			{
				DWORD ec = GetLastError();
				if (ec != ERROR_NO_MORE_FILES)
					error::win::WinApiError(SL, "Fail to find next volume").throwException();
				if (!fFindAll)
					LOGLVL(Trace, "Fail to find volume for <" << string::convertWCharToUtf8(sFindStr) << ">");
				break;
			}
		}

		if (string::startsWith(sFindStr, L"\\Device\\Mup", true))
			return addVolume(L"\\Device\\Mup", L"", "NETWORK");
	}
	CMD_CATCH("Fail to find volume descriptor")
	return {};
}

//
//
//
Variant FileDataProvider::getVolumeInfoById_NoLock(FileId sVolId)
{
	auto itVolume = m_pVolumeMap.find(sVolId);
	if (itVolume == m_pVolumeMap.end())
		error::NotFound(SL, FMT("Can't find volume <" << sVolId << ">")).throwException();
	return itVolume->second;
}

//
//
//
cmd::Variant FileDataProvider::getVolumeInfoByDevice(const std::wstring& sDevice)
{
	{
		std::scoped_lock _lock(m_mtxVolumeData);
		auto itVolume = m_pDeviceMap.find(string::convertToLow(sDevice));
		if (itVolume != m_pDeviceMap.end())
			return getVolumeInfoById_NoLock(itVolume->second);
	}

	return findVolume(sDevice);
}

//
//
//
Variant FileDataProvider::getVolumeInfoByPath(const std::wstring& sPath, Variant vToken)
{
	static const Size c_nVolumeGuidLen = 48;
	Variant vVolume;
	if (string::startsWith(sPath, L"\\\\?\\volume{", true) &&
		sPath.size() >= c_nVolumeGuidLen && 
		(sPath[c_nVolumeGuidLen] == L'\\' || sPath[c_nVolumeGuidLen] == L'\0'))
	{
		auto sDevice = getDeviceByGuid(sPath.substr(0, c_nVolumeGuidLen));
		return getVolumeInfoByDevice(sDevice);
	}

	std::wstring sNtPath;
	if (string::startsWith(sPath, L"\\Device\\", true))
		sNtPath = sPath;
	else if (string::startsWith(sPath, L"\\SystemRoot\\", true))
		sNtPath = parseObjectDirectory(sPath);
	else
		sNtPath = normalizePathName(sPath, vToken, PathType::NtPath);

	{
		std::scoped_lock _lock(m_mtxVolumeData);
		for (auto itVolume : m_pDeviceMap)
		{
			if (string::startsWith(sNtPath, itVolume.first, true) &&
				(sNtPath.length() == itVolume.first.length() || sNtPath[itVolume.first.length()] == L'\\'))
				return getVolumeInfoById_NoLock(itVolume.second);
		}
	}

	return findVolume(sNtPath);
}

//
//
//
Variant FileDataProvider::enrichVolumeInfo(Variant vVolume)
{
	std::wstring sDevice = vVolume["device"];
	{
		std::scoped_lock _lock(m_mtxVolumeData);
		auto itVolume = m_pDeviceMap.find(string::convertToLow(sDevice));
		if (itVolume != m_pDeviceMap.end())
			return getVolumeInfoById_NoLock(itVolume->second);
	}

	return addVolume(sDevice, vVolume.get("guid", ""), vVolume["type"]);
}

//
//
//
Variant FileDataProvider::getVolumeInfo(Variant vParams)
{
	TRACE_BEGIN
	if (vParams.has("id"))
	{
		std::scoped_lock _lock(m_mtxVolumeData);
		return getVolumeInfoById_NoLock(vParams["id"]).clone();
	}

	if (vParams.has("device"))
		return getVolumeInfoByDevice(vParams["device"]);

	if (vParams.has("guid"))
		return getVolumeInfoByDevice(getDeviceByGuid(vParams["guid"]));

	if (vParams.has("path"))
		return getVolumeInfoByPath(vParams["path"]);

	error::InvalidArgument(SL, "Unknown argument for getVolumeInfo()").throwException();
	TRACE_END("Fail to get volume info");
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * 'objects.fileDataProvider'
///
/// #### Supported commands
///
Variant FileDataProvider::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### getFileInfo()
	/// Get user info.
	///   * **id** [str] - file identifier.
	///  or
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	///   * **volume** [variant] - volume descriptor.
	/// Returns a file descriptor.
	///
	if (vCommand == "getFileInfo")
		return getFileInfo(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### getFileHash()
	/// Get file hash.
	///   * **id** [str] - file identifier.
	///  or
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	/// Returns a file descriptor.
	///
	if (vCommand == "getFileHash")
		return getFileHash(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### enrichFileHash()
	/// Enriches a file descriptor with hash.
	///   * **id** [str] - file identifier.
	///  or
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	/// Returns a file descriptor.
	///
	if (vCommand == "enrichFileHash")
		return enrichFileHash(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### getSignInfo()
	/// Get signature information.
	///   * **id** [str] - file identifier.
	///  or
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	/// Returns a data packet with signature-related information.
	///
	if (vCommand == "getSignInfo")
		return getSignatureInfo(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### enrichSignInfo()
	/// Enriches a file descriptor with signature-related information.
	///   * *id* [str] - file identifier.
	///  or
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	/// Returns a file descriptor.
	///
	if (vCommand == "enrichSignInfo")
		return enrichSignatureInfo(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### getFileStream()
	/// Get file stream.
	///   * **id** [str] - file identifier.
	///  or
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	/// Returns a file stream.
	///
	if (vCommand == "getFileStream")
		return getFileStream(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### getFileInfo()
	/// Get volume informatoin.
	///   * **id** [str] - volume identifier.
	///  or
	///   * **device** [str] - device name.
	/// Returns a data packet with volume-related information.
	///
	if (vCommand == "getVolumeInfo")
		return getVolumeInfo(vParams);

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### normalizePathName()
	/// Normalizes a file path.
	///   * **path** [str] - source path.
	///   * **security** [dict,opt] - security dict.
	///   * **type** [int] - a type of path.
	///
	/// Returns a string with normalized path.
	///
	if (vCommand == "normalizePathName")
	{
		std::wstring sPathName = vParams.get("path");
		Variant vSecurity = vParams.get("security", {});
		PathType eType = vParams.get("type");

		return normalizePathName(sPathName, vSecurity, eType);
	}

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	if (vCommand == "start")
	{
		start();
		return {};
	}

	///
	/// @fn Variant FileDataProvider::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	if (vCommand == "stop")
	{
		stop();
		return {};
	}

	TRACE_END( FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("FileDataProvider doesn't support a command <" 
		<< vCommand << ">")).throwException();
}

//
//
//
void FileDataProvider::loadState(Variant vState)
{
}

//
//
//
cmd::Variant FileDataProvider::saveState()
{
	return {};
}

//
//
//
void FileDataProvider::start()
{
	TRACE_BEGIN
	LOGLVL(Detailed, "File Data Provider is being started");

	std::scoped_lock _lock(m_mtxStartStop);
	if (m_fInitialized)
		return;

	if (m_timerPurge)
	{
		m_timerPurge->reschedule(m_nPurgeFileTimeout);
	}
	else
	{
		// avoid recursive pointing timer <-> FlsService
		ObjWeakPtr<FileDataProvider> weakPtrToThis(getPtrFromThis(this));
		m_timerPurge = runWithDelay(m_nPurgeFileTimeout, [weakPtrToThis]()
		{
			auto pThis = weakPtrToThis.lock();
			if (pThis == nullptr)
				return false;
			return pThis->purgeItems();
		});
	}

	m_fInitialized = true;

	LOGLVL(Detailed, "File Data Provider is started");
	TRACE_END("Fail to start File Data Provider");
}

//
//
//
void FileDataProvider::stop()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "File Data Provider is being stopped");

	std::scoped_lock _lock(m_mtxStartStop);
	if (!m_fInitialized)
		return;
	if (m_timerPurge)
		m_timerPurge->cancel();
	m_fInitialized = false;

	LOGLVL(Detailed, "File Data Provider is stopped");
	TRACE_END("Fail to stop File Data Provider");
}

//
//
//
void FileDataProvider::shutdown()
{
	LOGLVL(Detailed, "File Data Provider is being shutdowned");
	m_pFormats.clear();
	m_pObjectsMap.clear();

	{
		std::unique_lock _lock(m_mtxFileData);
		m_pPathMap.clear();
		m_pFileMap.clear();
	}
	{
		std::scoped_lock _lock(m_mtxVolumeData);
		m_pDeviceMap.clear();
		m_pVolumeMap.clear();
	}
	LOGLVL(Detailed, "File Data Provider is shutdowned");
}

} // namespace win
} // namespace sys
} // namespace cmd 

/// @}
