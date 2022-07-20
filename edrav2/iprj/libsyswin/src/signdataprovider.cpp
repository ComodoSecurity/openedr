//
// edrav2.libsyswin project
//
// Author: Denis Kroshin (14.03.2019)
// Reviewer: Denis Bogdanov (01.04.2019)
//
///
/// @file Signature Data Provider implementation
///
/// @addtogroup syswin System library (Windows)
/// @{
#include "pch.h"
#include "signdataprovider.h"

namespace cmd {
namespace sys {
namespace win {

#undef CMD_COMPONENT
#define CMD_COMPONENT "signdtprv"

//
//
//
SignDataProvider::SignDataProvider()
{
	m_fIsWin8Plus = IsWindows8OrGreater();
}

//
//
//
SignDataProvider::~SignDataProvider()
{
}

//
//
//
void SignDataProvider::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;

	m_nMaxStreamSize = vConfig.get("maxFileSize", g_nMaxStreamSize);
	m_nPurgeTimeout = vConfig.get("purgeTimeout", c_nPurgeTimeout);
	m_fDisableNetworAccess = vConfig.get("disableNetworkAccess", false);

	TRACE_END("Can't create SignDataProvider");
}

//
// FIXME: All timer procedures shall process exceptions
//
bool SignDataProvider::purgeItems()
{
	std::deque<Variant> pPurgeItems;
	Size nMapSizeBefore = 0;
	Size nMapSizeAfter = 0;

	LOGLVL(Debug, "Purge started <SignDataProvider>.");
	CMD_TRY
	{
		std::scoped_lock _lock(m_mtxLock);
		nMapSizeBefore = m_pSignInfoMap.size();
		Time nCurTime = getCurrentTime();
		for (auto itItem = m_pSignInfoMap.begin(); itItem != m_pSignInfoMap.end();)
		{
			// Break on stop
			if (!m_fInitialized)
				break;
			
			Time nItemTime = itItem->second["time"];
			if (nCurTime > nItemTime && nCurTime - nItemTime >= m_nPurgeTimeout)
			{
				// Delete file descriptor
				pPurgeItems.push_back(itItem->second);
				itItem = m_pSignInfoMap.erase(itItem);
			}
			else
				++itItem;
		}
		nMapSizeAfter = m_pSignInfoMap.size();
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, "Can't purge in <SignDataProvider>");
	}

	pPurgeItems.clear();
	LOGLVL(Debug, "Purge finished <SignDataProvider>. Size before <"
		<< nMapSizeBefore << ">, size after <" << nMapSizeAfter << ">.");
	return true;
}

//
//
//
SignStatus SignDataProvider::convertSignInfo(LONG winStatus)
{
	if (winStatus == ERROR_SUCCESS || winStatus == CERT_E_EXPIRED)
		return SignStatus::Valid;
	// Mask strange result for files like "sha256_D5773FA3900221255222E9EDE678C70F"
// 	if (winStatus == TRUST_E_NOSIGNATURE)
// 		return SignStatus::Unsigned;
	if (winStatus == CERT_E_UNTRUSTEDROOT)
		return SignStatus::Untrusted;
	if (winStatus == CRYPT_E_FILE_ERROR)
		return SignStatus::Undefined;
	return SignStatus::Invalid;
}

//
//
//
std::wstring SignDataProvider::getCertificateProvider(HANDLE hWVTStateData)
{
	CRYPT_PROVIDER_DATA *pCryptProvData = WTHelperProvDataFromStateData(hWVTStateData);
	if (pCryptProvData == NULL)
		return L"";
	CRYPT_PROVIDER_SGNR *pSigner = WTHelperGetProvSignerFromChain(pCryptProvData, 0, FALSE, 0);
	if (pSigner == NULL)
		return L"";
	CRYPT_PROVIDER_CERT *pCert = WTHelperGetProvCertFromChain(pSigner, 0);
	if (pCert == NULL)
		return L"";

	std::wstring sProvider(::CertGetNameStringW(pCert->pCert,
		CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, NULL, 0), 0);
	if (sProvider.size() <= 1 ||
		!::CertGetNameStringW(pCert->pCert, CERT_NAME_SIMPLE_DISPLAY_TYPE,
			0, NULL, sProvider.data(), (DWORD)sProvider.size()))
		return L"";
	sProvider.resize(sProvider.size() - 1);
	return sProvider;
}

//
//
//
SignStatus SignDataProvider::winVerifyTrust(GUID wt_guid, PWINTRUST_DATA_W8 pWtData, std::wstring& sProvider)
{
	if (pWtData == nullptr)
		error::InvalidArgument(SL).throwException();

	pWtData->dwUIChoice = WTD_UI_NONE;
	pWtData->fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
	pWtData->dwProvFlags = WTD_SAFER_FLAG | WTD_DISABLE_MD2_MD4;

	if (m_fDisableNetworAccess)
	{
		pWtData->fdwRevocationChecks = WTD_REVOKE_NONE;
		pWtData->dwProvFlags |= WTD_REVOCATION_CHECK_NONE | WTD_CACHE_ONLY_URL_RETRIEVAL;
	}

	SignStatus status = SignStatus::Undefined;
	do
	{
		pWtData->dwStateAction = WTD_STATEACTION_VERIFY;
		status = convertSignInfo(WinVerifyTrust(NULL, &wt_guid, pWtData));
		if (status == SignStatus::Valid)
			sProvider = getCertificateProvider(pWtData->hWVTStateData);

		pWtData->dwStateAction = WTD_STATEACTION_CLOSE;
		if (WinVerifyTrust(NULL, &wt_guid, pWtData) != ERROR_SUCCESS ||
			status != SignStatus::Valid)
			break;
		pWtData->hWVTStateData = NULL;
	} while (pWtData->pSignatureSettings != nullptr &&
		++pWtData->pSignatureSettings->dwIndex <= pWtData->pSignatureSettings->cSecondarySigs);
	return status;
}

//
//
//
bool hasEmbeddedSignInfo(const HANDLE hFile)
{
	TRACE_BEGIN
	CMD_TRY
	{
		auto pStream = io::win::createFileStream(hFile);
		IMAGE_DOS_HEADER DosHdr;
		pStream->setPosition(0);
		if (pStream->read(&DosHdr, sizeof(DosHdr)) < sizeof(DosHdr) ||
			DosHdr.e_magic != IMAGE_DOS_SIGNATURE)
			return false;

		uint32_t nPeMagic = 0;
		pStream->setPosition(DosHdr.e_lfanew);
		if (pStream->read(&nPeMagic, sizeof(nPeMagic)) < sizeof(nPeMagic) ||
			nPeMagic != IMAGE_NT_SIGNATURE)
			return false;

		uint16_t nMagic = 0;
		pStream->setPosition(io::IoOffset(DosHdr.e_lfanew) + 4 + sizeof(IMAGE_FILE_HEADER));
		if (pStream->read(&nMagic, sizeof(nMagic)) < sizeof(nMagic))
			return false;

		bool fIs64 = FALSE;
		IMAGE_DATA_DIRECTORY securityDir;
		if (nMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		{
			IMAGE_OPTIONAL_HEADER32 peHdr32;
			if (pStream->read(&peHdr32.MajorLinkerVersion, sizeof(peHdr32) - sizeof(nMagic)) < sizeof(peHdr32) - sizeof(nMagic))
				return false;
			securityDir = peHdr32.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
		}
		else if (nMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			fIs64 = TRUE;
			IMAGE_OPTIONAL_HEADER64 peHdr64;
			if (pStream->read(&peHdr64.MajorLinkerVersion, sizeof(peHdr64) - sizeof(nMagic)) < sizeof(peHdr64) - sizeof(nMagic))
				return false;
			securityDir = peHdr64.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
		}
		else
			return false;

		if (securityDir.VirtualAddress == 0)
			return false;

		uint32_t pSignHdr[2];
		pStream->setPosition(securityDir.VirtualAddress);
		if (pStream->read(pSignHdr, sizeof(pSignHdr)) < sizeof(pSignHdr) ||
			pSignHdr[1] != 0x00020200)
			return false;
	}
	CMD_PREPARE_CATCH
	catch (...)
	{
		return false;
	}
	return true;
	TRACE_END("Fail to parse file header")
}

//
//
//
Variant SignDataProvider::getEmbeddedSignInfo(const std::wstring& sFileName, HANDLE hFile)
{
	TRACE_BEGIN
	WINTRUST_FILE_INFO wt_fileinfo = { sizeof(WINTRUST_FILE_INFO) };
	wt_fileinfo.pcwszFilePath = sFileName.c_str();
	wt_fileinfo.hFile = hFile;

	WINTRUST_DATA_W8 wt_data = { m_fIsWin8Plus ? sizeof(WINTRUST_DATA_W8) : sizeof(WINTRUST_DATA) };
	wt_data.dwUnionChoice = WTD_CHOICE_FILE;
	wt_data.pFile = &wt_fileinfo;

	WINTRUST_SIGNATURE_SETTINGS wt_sig_settings = { sizeof(WINTRUST_SIGNATURE_SETTINGS) };
	CERT_STRONG_SIGN_PARA StrongSigPolicy = { sizeof(CERT_STRONG_SIGN_PARA) };
	if (m_fIsWin8Plus)
	{
		wt_sig_settings.dwFlags = WSS_GET_SECONDARY_SIG_COUNT | WSS_VERIFY_SPECIFIC;
		wt_data.pSignatureSettings = &wt_sig_settings;

		bool fStrongSigPolicy = false;
		if (fStrongSigPolicy)
		{
			StrongSigPolicy.dwInfoChoice = CERT_STRONG_SIGN_OID_INFO_CHOICE;
			StrongSigPolicy.pszOID = (LPSTR)szOID_CERT_STRONG_SIGN_OS_CURRENT;
			wt_data.pSignatureSettings->pCryptoPolicy = &StrongSigPolicy;
		}
	}

	std::wstring sVendor;
	GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	auto status = winVerifyTrust(action, &wt_data, sVendor);

	auto vSignInfo = Dictionary({ {"status", status} });
	if (!sVendor.empty())
		vSignInfo.put("vendor", sVendor);
	return vSignInfo;
	TRACE_END("Fail to check embedded signature")
}

// Specialization for windows HCATADMIN
struct CatAdminTraits
{
	using ResourceType = HCATADMIN;
	static inline const ResourceType c_defVal = nullptr;
	static void cleanup(ResourceType& rsrc) noexcept
	{
		if (!::CryptCATAdminReleaseContext(rsrc, 0))
			error::win::WinApiError(SL, "Can't close cat admin handle").log();
	}
};
using ScopedCatAdminHandle = UniqueResource<CatAdminTraits>;

//
//
//
Variant SignDataProvider::getCatalogSignInfo(const std::wstring& sFileName, const HANDLE hFile, std::wstring sCatFile)
{
	TRACE_BEGIN
	auto vSignInfo = Dictionary({ {"status", SignStatus::Unsigned} });

	const GUID guid = DRIVER_ACTION_VERIFY;
	ScopedCatAdminHandle hCatAdmin;
	if (!::CryptCATAdminAcquireContext(&hCatAdmin, &guid, 0))
	{
		error::win::WinApiError(SL, FMT("File: " << string::convertWCharToUtf8(sFileName) << ">")).log();
		return vSignInfo;
	}

	DWORD size = 0;
	if (!::CryptCATAdminCalcHashFromFileHandle(hFile, &size, 0, 0))
	{
		error::win::WinApiError(SL, FMT("File: " << string::convertWCharToUtf8(sFileName) << ">")).log();
		return vSignInfo;
	}
	std::vector<uint8_t> pHash(size);
	if (!::CryptCATAdminCalcHashFromFileHandle(hFile, &size, pHash.data(), 0))
	{
		LOGLVL(Debug, "Fail to calculte hash for file <" << string::convertWCharToUtf8(sFileName) << ">");
		return vSignInfo;
	}

	HCATINFO hCatInfo = NULL;
	do
	{
		if (sCatFile.empty())
		{
			hCatInfo = ::CryptCATAdminEnumCatalogFromHash(hCatAdmin, pHash.data(), static_cast<DWORD>(pHash.size()), 0, &hCatInfo);
			if (hCatInfo == NULL)
				break;

			CATALOG_INFO catinfo = {};
			if (!::CryptCATCatalogInfoFromContext(hCatInfo, &catinfo, 0))
			{
				error::win::WinApiError(SL, FMT("File: " << string::convertWCharToUtf8(sFileName) << ">")).log();
				break;
			}
			
			sCatFile = catinfo.wszCatalogFile;
		}

		WINTRUST_CATALOG_INFO_W8 CatalogData = { m_fIsWin8Plus ? sizeof(WINTRUST_CATALOG_INFO_W8) : sizeof(WINTRUST_CATALOG_INFO) };
		CatalogData.pcwszMemberFilePath = sFileName.c_str();
		CatalogData.hMemberFile = hFile;
		CatalogData.pcwszCatalogFilePath = sCatFile.c_str();
		CatalogData.cbCalculatedFileHash = static_cast<DWORD>(pHash.size());
		CatalogData.pbCalculatedFileHash = pHash.data();
		if (m_fIsWin8Plus)
			CatalogData.hCatAdmin = hCatAdmin;

		WINTRUST_DATA_W8 wt_data = { m_fIsWin8Plus ? sizeof(WINTRUST_DATA_W8) : sizeof(WINTRUST_DATA) };
		wt_data.dwUnionChoice = WTD_CHOICE_CATALOG;
		wt_data.pCatalog = &CatalogData;

		GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;

		DRIVER_VER_INFO verInfo = { sizeof(DRIVER_VER_INFO) };
		if (hCatInfo)
		{
			// TODO: Found this in ProcHacker, need investigation
			// Disable OS version checking by passing in a DRIVER_VER_INFO structure.
// 			wt_data.pPolicyCallbackData = &verInfo;
// 			action = DRIVER_ACTION_VERIFY;
		}

		std::wstring sVendor;
		vSignInfo.put("status", winVerifyTrust(action, &wt_data, sVendor));
		if (!sVendor.empty())
			vSignInfo.put("vendor", sVendor);
#ifdef _DEBUG
		if (!sCatFile.empty())
			vSignInfo.put("catFilePath", sCatFile);
#endif
		
		if (verInfo.pcSignerCertContext && !::CertFreeCertificateContext(verInfo.pcSignerCertContext))
			error::win::WinApiError(SL).log();

		break; // Now we parse first CAT file only
		sCatFile.clear();
	} while (hCatInfo);
	if (hCatInfo && !::CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0))
		error::win::WinApiError(SL).log();

	return vSignInfo;
	TRACE_END("Fail to check catalog signature")
}

typedef HRESULT(WINAPI* fnPackageIdFromFullName)(
	_In_ PCWSTR packageFullName,
	_In_ const UINT32 flags,
	_Inout_ UINT32* bufferLength,
	_Out_writes_bytes_opt_(*bufferLength) BYTE* buffer);

static fnPackageIdFromFullName pPackageIdFromFullName = NULL;

//
//
//
bool SignDataProvider::isUwpApplication(const std::wstring& sFileName)
{
	if (m_sWindowsApps.empty())
	{
		m_sWindowsApps = getKnownPath(FOLDERID_ProgramFiles) + L"\\WindowsApps";
		auto pFileDP = queryInterfaceSafe<IFileInformation>(queryServiceSafe("fileDataProvider"));
		if (pFileDP)
		{
			m_sWindowsAppsNt = pFileDP->normalizePathName(m_sWindowsApps, {}, PathType::NtPath);
			m_sWindowsAppsVl = pFileDP->normalizePathName(m_sWindowsApps, {}, PathType::Unique);
		}
/*
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (!SUCCEEDED(hr))
		{
			error::win::WinApiError(SL, hr, "Fail to initialize COM library").log();
			return false;
		}
*/
		if (IsWindows8OrGreater())
		{
			sys::win::ScopedLibrary pKernel(::LoadLibraryW(L"kernel32.dll"));
			if (pKernel)
				pPackageIdFromFullName = (fnPackageIdFromFullName)::GetProcAddress(pKernel, (LPCSTR)"PackageIdFromFullName");
		}
	}

	if ((!m_sWindowsAppsVl.empty() && string::startsWith(sFileName, m_sWindowsAppsVl)) ||
		(!m_sWindowsAppsNt.empty() && string::startsWith(sFileName, m_sWindowsAppsNt)) ||
		(!m_sWindowsApps.empty() && string::startsWith(sFileName, m_sWindowsApps, true)))
		return true;
	return false;
}

//
//
//
bool SignDataProvider::isUwpAppValid(const std::wstring& sFileName, const std::wstring& sAppPath)
{
	// TODO: need proper per-tread COM initialization
	return false;

	IAppxFactory* appxFactory = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(AppxFactory), NULL, CLSCTX_INPROC_SERVER, __uuidof(IAppxFactory), (LPVOID*)(&appxFactory));
	if (!SUCCEEDED(hr))
		error::win::WinApiError(SL, hr, "Fail to create object <AppxFactory>").throwException();

	auto sBlockMapFile = sAppPath + L"\\AppxBlockMap.xml";
	IStream* pBlockMapStream = NULL;
	hr = SHCreateStreamOnFileEx(sBlockMapFile.c_str(), STGM_READ | STGM_SHARE_DENY_WRITE, 0, FALSE, NULL, &pBlockMapStream);
	if (!SUCCEEDED(hr))
		error::win::WinApiError(SL, hr, "Fail to open <AppxBlockMap.xml> file").throwException();

	auto sSignatureFile = sAppPath + L"\\AppxSignature.p7x";
	IAppxBlockMapReader* reader = NULL;
	if (std::filesystem::exists(sSignatureFile))
		hr = appxFactory->CreateValidatedBlockMapReader(pBlockMapStream, sSignatureFile.c_str(), &reader);
	else
		hr = appxFactory->CreateBlockMapReader(pBlockMapStream, &reader);
	if (!SUCCEEDED(hr))
		error::win::WinApiError(SL, hr, "Fail to create <AppxBlockMapReader>").throwException();

	auto sName = sFileName.substr(sFileName.rfind(L'\\') + 1);
	IAppxBlockMapFile* file = NULL;
	hr = reader->GetFile(sName.c_str(), &file);
	if (!SUCCEEDED(hr))
		error::win::WinApiError(SL, hr, "Fail to query <AppxBlockMapFile>").throwException();

	IStream* fileStream = NULL;
	hr = SHCreateStreamOnFileEx(sFileName.c_str(), STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, NULL, &fileStream);
	if (!SUCCEEDED(hr))
		error::win::WinApiError(SL, hr, "Fail to open target file").throwException();

	BOOL fValid = false;
	hr = file->ValidateFileHash(fileStream, &fValid);
	if (!SUCCEEDED(hr))
		error::win::WinApiError(SL, hr, "Fail to validate target file").throwException();

	return fValid;
}

//
//
//
Variant SignDataProvider::getUwpSignInfo(const std::wstring& sFileName, const HANDLE hFile)
{
	auto vSignInfo = Dictionary({ {"status", SignStatus::Unsigned} });

	Size nPkgNameOffset = 0;
	if (!m_sWindowsAppsVl.empty() && string::startsWith(sFileName, m_sWindowsAppsVl))
		nPkgNameOffset = m_sWindowsAppsVl.length() + 1;
	else if (!m_sWindowsAppsNt.empty() && string::startsWith(sFileName, m_sWindowsAppsNt))
		nPkgNameOffset = m_sWindowsAppsNt.length() + 1;
	else if (!m_sWindowsApps.empty() && string::startsWith(sFileName, m_sWindowsApps, true))
		nPkgNameOffset = m_sWindowsApps.length() + 1;
	else
		return vSignInfo;

	auto nSlash = sFileName.find(L'\\', nPkgNameOffset);
	if (nSlash == sFileName.npos)
		return vSignInfo;
	auto sPkgName = sFileName.substr(nPkgNameOffset, nSlash - nPkgNameOffset);
	if (sPkgName.empty())
		return vSignInfo;
	auto sAppPath(sFileName.substr(0, nPkgNameOffset + sPkgName.length()));

	CMD_TRY
	{
		auto sCatName = sAppPath + L"\\AppxMetadata\\CodeIntegrity.cat";
		if (std::filesystem::exists(sCatName))
			vSignInfo = getCatalogSignInfo(sFileName, hFile, sCatName);
		else if (pPackageIdFromFullName && isUwpAppValid(sFileName, sAppPath))
		{
			vSignInfo.put("status", SignStatus::Valid);
			UINT32 nSize = 0;
			auto status = pPackageIdFromFullName(sPkgName.c_str(), PACKAGE_INFORMATION_FULL, &nSize, NULL);
			std::vector<BYTE> pBuffer(nSize); 
			if (status == ERROR_INSUFFICIENT_BUFFER)
				status = pPackageIdFromFullName(sPkgName.c_str(), PACKAGE_INFORMATION_FULL, &nSize, pBuffer.data());
			if (status == ERROR_SUCCESS && ((PACKAGE_ID*)pBuffer.data())->publisher != NULL)
				vSignInfo.put("vendor", ((PACKAGE_ID*)pBuffer.data())->publisher);
		}
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.log(SL, FMT("Fail to validate UWP app <" << string::convertWCharToUtf8(sFileName) << ">"));
		vSignInfo.put("status", SignStatus::Undefined);
	}

	return vSignInfo;
}

//
//
//
Variant SignDataProvider::getSignatureInfo(Variant vFileName, Variant vSecurity)
{
	ScopedFileHandle hFile(getFileHandle(vFileName, FILE_GENERIC_READ));
	if (!hFile && !vSecurity.isEmpty())
	{
		ScopedImpersonation fImpers(impersonateThread(vSecurity));
		if (fImpers)
			hFile.reset(getFileHandle(vFileName, FILE_GENERIC_READ));
	}
	if (!hFile)
		return Dictionary({ {"status", SignStatus::Undefined} });

	auto vSignInfo = Dictionary({ {"status", SignStatus::Unsigned} });
	if (hasEmbeddedSignInfo(hFile))
		vSignInfo = getEmbeddedSignInfo(vFileName, hFile);
	// Check and start CryptSvc service in separate thread
	if (vSignInfo["status"] == SignStatus::Unsigned &&
		!m_fCryptSvcIsRun && m_pStartSvcThread.joinable())
	{
		std::scoped_lock _lock(m_mtxThread);
		while (m_pStartSvcThread.joinable())
			m_pStartSvcThread.join();
	}
	LARGE_INTEGER nSize;
	if (m_fCryptSvcIsRun && vSignInfo["status"] == SignStatus::Unsigned &&
		GetFileSizeEx(hFile, &nSize) && io::IoSize(nSize.QuadPart) <= m_nMaxStreamSize)
	{
		vSignInfo = isUwpApplication(vFileName) ? 
			getUwpSignInfo(vFileName, hFile) : 
			getCatalogSignInfo(vFileName, hFile);
	}

	return vSignInfo;
}

//
//
//
Variant SignDataProvider::getSignatureInfo(Variant vFileName, Variant vSecurity, Variant vHash)
{
	TRACE_BEGIN
	if (!m_fInitialized)
		return Dictionary({ {"status", SignStatus::Undefined} });

	if (!vHash.isEmpty())
	{
		std::unique_lock<std::mutex> sync(m_mtxLock);
		std::string hash = vHash;
		while (m_pLockQueue.find(hash) != m_pLockQueue.end())
			m_cvLock.wait(sync);

		// Check global cash
		auto itSignInfo = m_pSignInfoMap.find(hash);
		if (itSignInfo != m_pSignInfoMap.end())
			return itSignInfo->second["data"];

		m_pLockQueue.insert(hash);
	}
	auto signature = getSignatureInfo(vFileName, vSecurity);
	LOGLVL(Debug, "Parse authenticode sign for file <" << vFileName << ">");
	if (!vHash.isEmpty())
	{
		std::unique_lock<std::mutex> sync(m_mtxLock);
		std::string hash = vHash;
		if (!signature.isEmpty() && 
			signature.get("status", SignStatus::Undefined) != SignStatus::Undefined)
			m_pSignInfoMap[hash] = Dictionary({ {"data", signature}, {"time", getCurrentTime()} });
		m_pLockQueue.erase(hash);
		m_cvLock.notify_all();
	}
	return signature;
	TRACE_END("Fail to get file signature")
}


//
//
//
void SignDataProvider::startCryptSvc()
{
	CMD_TRY
	{
		// Check the service is exist
		Variant vResult = execCommand(createObject(CLSID_WinServiceController), "isExist",
			Dictionary({ { "name", g_sServiceName } }));
		if (!vResult)
		{
			LOGLVL(Debug, "CryptSvc not found");
			return;
		}

		// Check the service is run
		vResult = execCommand(createObject(CLSID_WinServiceController), "queryStatus",
			Dictionary({ { "name", g_sServiceName } }));
		if (vResult["state"] == 4 /*SERVICE_RUNNING*/)
		{
			m_fCryptSvcIsRun = true;
			LOGLVL(Debug, "CryptSvc already started");
			return;
		}

		// Start service, function throw on fail
		(void)execCommand(createObject(CLSID_WinServiceController), "start",
			Dictionary({ {"name", g_sServiceName} }));
		m_fCryptSvcIsRun = true;
		LOGLVL(Detailed, "Service <" << g_sServiceName << "> is started");
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		e.log(SL, FMT("Fail to start service <" << g_sServiceName << ">"));
	}
}


//
//
//
void SignDataProvider::loadState(Variant vState)
{
	if (vState.isNull())
		return;
	Dictionary vData = vState.clone();
	{
		std::scoped_lock _lock(m_mtxLock);
		for (auto it : vData)
			m_pSignInfoMap[std::string(it.first)] = it.second;
	}
	purgeItems();
}

//
//
//
cmd::Variant SignDataProvider::saveState()
{
	Dictionary vState;
	std::scoped_lock _lock(m_mtxLock);
	for (auto it : m_pSignInfoMap)
		vState.put(it.first, it.second);
	return vState;
}

//
//
//
void SignDataProvider::start()
{
	TRACE_BEGIN
	LOGLVL(Detailed, "Signature Data Provider is being started");

	std::scoped_lock _lock(m_mtxStartStop);
	if (m_fInitialized)
		return;

	if (m_timerPurge)
	{
		m_timerPurge->reschedule(m_nPurgeCallTimeout);
	}
	else
	{
		// avoid recursive pointing timer <-> FlsService
		ObjWeakPtr<SignDataProvider> weakPtrToThis(getPtrFromThis(this));
		m_timerPurge = runWithDelay(m_nPurgeCallTimeout, [weakPtrToThis]()
		{
			auto pThis = weakPtrToThis.lock();
			if (pThis == nullptr)
				return false;
			return pThis->purgeItems();
		});
	}

	m_fCryptSvcIsRun = false;
	m_pStartSvcThread = std::thread([this]() { this->startCryptSvc(); });

	m_fInitialized = true;

	LOGLVL(Detailed, "Signature Data Provider is started");
	TRACE_END("Fail to start Signature Data Provider");
}

//
//
//
void SignDataProvider::stop()
{
	TRACE_BEGIN;
	LOGLVL(Detailed, "Signature Data Provider is being stopped");

	std::scoped_lock _lock(m_mtxStartStop);
	if (!m_fInitialized)
		return;
	if (m_timerPurge)
		m_timerPurge->cancel();
	m_fInitialized = false;

	LOGLVL(Detailed, "Signature Data Provider is stopped");
	TRACE_END("Fail to stop Signature Data Provider");
}

//
//
//
void SignDataProvider::shutdown()
{
	LOGLVL(Detailed, "Signature Data Provider is being shutdowned");
	{
		std::unique_lock _lock(m_mtxLock);
		m_pSignInfoMap.clear();
		if (!m_pLockQueue.empty())
		{
			error::RuntimeError(SL, "Lock queue not empty").log();
			m_pLockQueue.clear();
			m_cvLock.notify_all();
		}
	}
	if (m_pStartSvcThread.joinable())
		m_pStartSvcThread.join();
	LOGLVL(Detailed, "Signature Data Provider is shutdowned");
}

//
//
//
Variant SignDataProvider::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant SignDataProvider::execute()
	///
	/// ##### getSignInfo()
	/// Get signature information.
	///   * **path** [str] - file path.
	///   * **security** [variant] - security descriptor.
	/// Returns a data packet with signature-related information.
	///
	if (vCommand == "getSignInfo")
		return getSignatureInfo(vParams.get("path", ""), vParams.get("security", {}));

	///
	/// @fn Variant SignDataProvider::execute()
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
	/// @fn Variant SignDataProvider::execute()
	///
	/// ##### stop()
	/// Stops the service.
	///
	if (vCommand == "stop")
	{
		stop();
		return {};
	}

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("SignDataProvider doesn't support a command <"
		<< vCommand << ">")).throwException();
}

} // namespace win
} // namespace sys
} // namespace cmd 

/// @}
