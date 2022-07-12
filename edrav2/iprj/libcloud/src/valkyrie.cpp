//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (01.04.2019)
// Reviewer: Denis Bogdanov (18.04.2019)
//
///
/// @file Valkyrie Client Service objects implementation
///
/// @addtogroup cloud Cloud communication library
/// @{
#include "pch.h"
#include "valkyrie.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "valkyrie"

namespace cmd {
namespace cloud {
namespace valkyrie {

constexpr Size c_nMaxFileSize = 85 * 1024 * 1024; // Max file size for submit is 85MB.

//
//
//
enum class ErrorCode : uint16_t
{
	Success = 0, //< No error
	BadRequest = 400, //<  Following parameter(s) are required: ...
	Unauthorized = 401, //< Provided api key is not authorized to do this request.
	PageNotFound = 404, //< Requested call or page is not found.
	InternalServerError = 500, //< An error occurred when processing request.
	SubmitTokenExpired = 101, //< This submit token is expired.
	SubmitTokenAlreadyUsed = 102, //< This submit token is used before.
	InvalidSubmitToken = 103, //< This submit token is invalid.
	SubmitTokenMissing = 104, //< Submit token is missing in request parameters.
	ApiKeyMissing = 105, //< API Key is missing in request parameters.
	FileMissing = 106, //< File not found within request content.Please send file with "file_data" parameter.
	InvalidRequestType = 107, //< (NOT USED) Invalid HTTP request type.
	ManualAnalysisNotFound = 108, //< Requested Manual Analysis is not found.
	InvalidApiKey = 109, //< API Key is invalid.
	InvalidFileType = 110, //< Invalid file is uploaded to server.Accepted types : 32 Bit PE, 64 Bit PE
	TimeoutExtractingArchiveFile = 111, //< Given archive file is corrupted or password protected or too big 
										// to extract in 60 seconds...
	NoExeInArchiveFile = 112, //< Given archive file does not contain executable content...
	FileNotUploadedBefore = 113, //< Requested file is not uploaded server before.Please use existing file 
							     // SHA1 or upload file via / fvs_submit_auto request.
	InvalidSha1 = 114, //< SHA1 can not be empty and must be 40 characters length
	InvalidNumericValue = 115, //< Given value for field: 'abc', is not numeric, use numeric value for this field.
	AptNoStartedScanFound = 116, //< No APT Scan started for this Api - Key found.
	ManualAnalysisLimitReached = 117, //< Number of manual analyses reached predefined limit
	UndefinedManualAnalysisLimit = 118, //< Manual analysis limit for corresponding user is not defined
	MasterUserNotFound = 119, //< (NOT USED) Master user could not be found
	InvalidUserForManualAnalysis = 120, //< Please use premium user credentials to make manual analysis request!
	MetadataTokenMissing = 131, //< Metadata token is missing in request parameters.
	MetadataTokenAlreadyUsed = 132, //< Requested metadata token already used.
	InvalidMetadataType = 133, //< Requested metadata_type is invalid.
	InvalidMetadataTokenParameter = 134, //< The value of 'metadata_token' parameter is invalid.
	MetadataTokenExpired = 135, //< (NOT USED) This metadata token is expired.
	MetadataFileMissing = 136, //< Metadata file not found within request content.Please send file with 
							   //"metadata_file_data" parameter.
	MetadataFileIsInvalid = 137, //< Invalid metadata file uploaded.Please check the following error 
								 // message : <ERROR-MESSAGE-WILL-BE-HERE>
	ComponentResponseTimeout = 201, //< Didn't received response from component.
	ManualVerdictNotGiven = 202, //< Cannot succeed objection, manual verdict not given yet.
	AlreadyObjected = 203, //< Cannot succeed objection, given sha1 already objected.
	AnalysisNotFinished = 204, //< Analysis not finished yet.
	AnalysisNotFound = 205, //< Analysis not found for requested analysis id.
	AnalysisFailed = 206, //< Analysis failed with internal error.
	ComponentErrorOccured = 207, //< Didn't received correct response from component(s).
	LicenseAlreadyAssociatedToAnotherApiKey = 301, //< License is already associated with another api key.
	InvalidLicenseId = 302, //< Given license id is invalid.
	EndpointIdMissing = 303, //< Endpoint id is missing in request parameters.
	LicenseExpired = 304, //< This user license is expired.
	NoActiveLicenseFound = 305, //< (NOT USED) This user has no active license.
	CannotConnectToCamLicenseServer = 306, //< Unable to connect CAM License server to validate license.
	InvalidCamCredentials = 307, //<  Provided username and password are not valid.
	AllLicensesAreAlreadyConnectedToAnotherApiKey = 308, //< All licenses are belonging to provided CAM user 
													     // are already connected to another Api Key, please 
														 // remove api_key param.
	CamLicenseRequired = 310, //< (NOT USED) Only CAM Licensed users can do this action.
	EndpointLimitReached = 311, //< Endpoint limit reached for connected license to this api key.
	NoValidLicenseFound = 312, //< This CAM user has no valid license for Valkyrie.
	NonEnterprise = 313, //< Non-enterprise or non-premium accounts can send just 1 request per 1 sec.
	UnexpectedResponseReturnedFromCam = 314, //< (NOT USED) Unexpected response returned from CAM server.

	Unknown = UINT16_MAX
};

///
/// The Valkyrie errors wrapper class.
///
class ValkyrieError : public error::SystemError
{
private:
	ErrorCode m_valkyrieErrorCode;
	
	//
	//
	//
	std::string createWhat(const Variant& v, std::string_view sMsg)
	{
		ErrorCode ec = v.get("return_code", ErrorCode::Unknown);

		std::string sDescription;
		if (v.has("result_message"))
			sDescription = v["result_message"];
		if (v.has("result_description"))
		{
			if (!sDescription.empty())
				sDescription += " - ";
			sDescription += v["result_description"];
		}
		if ((sDescription.empty()) && (ec == ErrorCode::Success))
			sDescription = "Success";

		std::string s(sMsg);
		if (s.empty())
		{
			s += "Valkyrie error ";
			s += string::convertToHex(ec);
			if (!sDescription.empty())
			{
				s += " - ";
				s += string::convertToUtf8(sDescription);
			}
		}
		else
		{
			s += " [Valkyrie error ";
			s += string::convertToHex(ec);
			if (!sDescription.empty())
			{
				s += " - ";
				s += string::convertToUtf8(sDescription);
			}
			s += "]";
		}
		return s;
	}

	//
	//
	//
	std::exception_ptr getRealException() override
	{
		return std::make_exception_ptr(*this);
	};

public:

	///
	/// Constructor.
	///
	/// @param v - Variant value with error's details.
	/// @param sMsg [opt] - the error description (default: "").
	///
	ValkyrieError(const Variant& v, std::string_view sMsg = "") :
		error::SystemError(createWhat(v, sMsg)),
		m_valkyrieErrorCode(v.get("return_code", ErrorCode::Unknown)) {}

	///
	/// Constructor.
	///
	/// @param srcLoc - information about a source file and line.
	/// @param v - Variant value with error's details.
	/// @param sMsg [opt] - the error description (default: "").
	///
	ValkyrieError(SourceLocation srcLoc, const Variant& v, std::string_view sMsg = "") :
		error::SystemError(srcLoc, createWhat(v, sMsg)),
		m_valkyrieErrorCode(v.get("return_code", ErrorCode::Unknown)) {}

	///
	/// Destructor.
	///
	virtual ~ValkyrieError() {}

	///
	/// Returns the Valkyrie error code.
	///
	/// @returns The function returns the value of `valkyrie::ErrorCode`.
	///
	ErrorCode getValkyrieErrorCode() const
	{
		return m_valkyrieErrorCode;
	}
};

//
//
//
void ValkyrieService::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	m_nSubmitRetryCount = vConfig.get("retryCount", m_nSubmitRetryCount);
	updateSettings(vConfig);
	m_pFileDataProvider = getCatalogData("objects.fileDataProvider");
	m_pHttp = std::make_unique<net::HttpClient>();
	TRACE_END("Error during configuration");
}

//
//
//
void ValkyrieService::updateSettings(Variant vConfig)
{
	std::scoped_lock lock(m_mtxConfig);

	if (vConfig.has("url"))
		m_sUrl = vConfig["url"];
	else if (m_sUrl.empty())
		error::InvalidArgument(SL, "Missing field <url>").throwException();

	if (vConfig.has("apiKey"))
		m_sApiKey = vConfig["apiKey"];
	else if (m_sApiKey.empty())
		error::InvalidArgument(SL, "Missing field <apiKey>").throwException();

	if (vConfig.has("endpointId"))
		m_sEndpointId = vConfig["endpointId"];
	else if (m_sEndpointId.empty())
		error::InvalidArgument(SL, "Missing field <endpointId>").throwException();

	m_fInitialized = false;
}

//
//
//
void ValkyrieService::loadState(Variant vState)
{
	TRACE_BEGIN;
	if (vState.isEmpty())
		return;

	// Load cache
	if (!vState.has("cache"))
		return;

	std::scoped_lock lock(m_mtxCache);
	for (const auto& item : Dictionary(vState.get("cache")))
		m_cache.try_emplace(std::string(item.first), item.second.get("time"));
	TRACE_END("Error during loading state");
}

//
//
//
Variant ValkyrieService::saveState()
{
	TRACE_BEGIN;
	Dictionary dictState;

	// Save cache
	Dictionary dictCache;
	{
		std::scoped_lock lock(m_mtxCache);
		for (const auto& item : m_cache)
			dictCache.put(item.first, Dictionary({{ "time", item.second.time }}));
	}
	if (!dictCache.isEmpty())
		dictState.put("cache", dictCache);

	return dictState;
	TRACE_END("Error during saving state");
}

//
//
//
void ValkyrieService::start()
{
	LOGLVL(Detailed, "ValkyrieService is being started");
	// Restart the client after abort
	m_pHttp->restart();
	m_fStarted = true;
	LOGLVL(Detailed, "ValkyrieService is started");
}

//
//
//
void ValkyrieService::stop()
{
	LOGLVL(Detailed, "ValkyrieService is being stopped");
	// Abort a current operation
	m_pHttp->abort();
	m_fStarted = false;
	LOGLVL(Detailed, "ValkyrieService is stopped");
}

//
//
//
void ValkyrieService::shutdown()
{
	m_fStarted = false;
}

//
//
//
void ValkyrieService::init()
{
	if (m_fInitialized)
		return;

	std::scoped_lock lock(m_mtxHttp, m_mtxConfig);
	m_pHttp->setRootUrl(m_sUrl);
	m_fInitialized = true;
}

//
//
//
bool ValkyrieService::isFileInCache(const Hash& hash)
{
	std::scoped_lock lock(m_mtxCache);
	auto iter = m_cache.find(hash);
	return (iter != m_cache.end());
}

//
//
//
void ValkyrieService::addFileToCache(const Hash& hash)
{
	std::scoped_lock lock(m_mtxCache);
	Time now = getCurrentTime();
	m_cache.try_emplace(hash, now);
}

//
//
//
Variant ValkyrieService::getBasicInfo(std::string_view sHash)
{
	Variant vData;
	{
		std::scoped_lock lock(m_mtxConfig);
		vData = Dictionary({
			{"api_key", m_sApiKey},
			{"endpoint_id", m_sEndpointId},
			{"sha1", sHash}
			});
	}

	std::scoped_lock lock(m_mtxHttp);
	return m_pHttp->post("fvs_basic_info", vData, Dictionary({ {"contentType", "multipart/form-data"} }));
}

//
//
//
Variant ValkyrieService::submitToAutomaticAnalysis(ObjPtr<io::IReadableStream> pStream,
	std::filesystem::path pathFile, std::string_view sSubmitToken, Size nTimeout)
{
	if (pStream->getSize() > c_nMaxFileSize)
		error::InvalidArgument(SL, FMT("Max file size is 85MB")).throwException();

	Variant vData;
	{
		std::scoped_lock lock(m_mtxConfig);
		vData = Dictionary({
			{"api_key", m_sApiKey},
			{"endpoint_id", m_sEndpointId},
			{"file_path", pathFile.native()},
			{"submit_token", sSubmitToken},
			{"file_data", Dictionary({
				{"stream", pStream},
				{"filename", pathFile.filename().c_str()}
				})}
			});
	}

	std::scoped_lock lock(m_mtxHttp);
	return m_pHttp->post("fvs_submit_auto", vData, Dictionary({
		{"contentType", "multipart/form-data"}, 
		{"connectionTimeout", 10000},
		{"operationTimeout", nTimeout},
	}));
}

//
//
//
bool ValkyrieService::submit(Variant vFile)
{
	TRACE_BEGIN;
	// Validate file size
	Size nFileSize = static_cast<Size>(vFile.get("size", 0));
	if (nFileSize > c_nMaxFileSize)
	{
		LOGLVL(Debug, "File is too big for Valkyrie:\n" << vFile);
		return true;
	}

	// Get and validate the file hash
	Hash hash = vFile.get("hash", "");
	if (hash.empty() || (hash == c_sZeroHash) || (hash.size() != c_nHashSize * 2))
	{
		LOGWRN("File has empty or invalid hash <" << vFile << ">");
		return true;
	}

	if (isFileInCache(hash))
	{
		LOGLVL(Debug, "File has been already sent <hash=" << hash << ">");
		return true;
	}

	if (!m_fStarted)
		error::OperationDeclined(SL, "ValkyrieService is not running").throwException();

	// Initialize the HTTP client
	init();

	// Get file basic information from Valkyrie
	auto vInfo = getBasicInfo(hash);
	if (!vInfo.has("return_code"))
		error::InvalidFormat(SL, "Invalid server response").throwException();
	if (vInfo["return_code"] != ErrorCode::Success)
		ValkyrieError(SL, vInfo, "Failed to get basic info").throwException();
	if (vInfo.get("upload", 1) != 1)
	{
		LOGLVL(Debug, "Valkyrie doesn't need a file <hash: " << hash << ">");
		addFileToCache(hash);
		return true;
	}
	if (vInfo.has("submit_token"))
		m_sSubmitToken = vInfo["submit_token"];

	// Get file stream
	auto pStream = m_pFileDataProvider->getFileStream(vFile);
	if (pStream == nullptr)
		return true;

	// Submit file to Valkyrie
	auto pathFile = std::filesystem::path(vFile.get("path", "C:\\file.exe"));

	// Use a timeout 30s for each 1MB or 30s if less or no information about size
	Size nTimeout = std::max<Size>((nFileSize / 1024 / 1024) * 30000, 30000);

	auto vSubmitResult = submitToAutomaticAnalysis(pStream, pathFile, m_sSubmitToken, nTimeout);
	if (!vSubmitResult.has("return_code"))
		error::InvalidFormat(SL, "Invalid server response").throwException();
	if (vSubmitResult["return_code"] != ErrorCode::Success)
		ValkyrieError(SL, vSubmitResult, "Failed to submit a file").throwException();

	addFileToCache(hash);
	return true;
	TRACE_END("Error during a file submit");
}

///
/// #### Supported commands
///
Variant ValkyrieService::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Execute command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command's parameters:\n" << vParams);

	///
	/// @fn Variant ValkyrieService::execute()
	///
	/// ##### submit()
	/// Submits a file to the server.
	///   * file [Variant] - a file descriptor.
	///
	/// Returns true if submission is success (or item should be skipped).
	/// Returns false if submission failed and submit retry is required.
	/// Throws an exception if the operation was declined.
	///
	if (vCommand == "submit")
	{
		if (!vParams.has("file"))
		{
			error::InvalidArgument(SL, "Missing field <file>").log();
			return true;
		}
		auto vFile = vParams.get("file");
		if (vFile.isNull())
			return true;

		CMD_TRY
		{
			return submit(vFile);
		}
		CMD_PREPARE_CATCH
		catch (error::OperationDeclined& ex)
		{
			ex.logAsWarning();
			throw;
		}
		catch (ValkyrieError& ex)
		{
			LOGLVL(Debug, ex.what());
			auto ec = ex.getValkyrieErrorCode();
			if ((ec == ErrorCode::InternalServerError) ||
				(ec == ErrorCode::SubmitTokenExpired) ||
				(ec == ErrorCode::InvalidSubmitToken))
				return false;
			return true;
		}
		catch (error::Exception& ex)
		{
			LOGLVL(Debug, ex.what());
			return false;
		}
	}
	///
	/// @fn Variant ValkyrieService::execute()
	///
	/// ##### updateSettings()
	/// Updates the settings.
	///
	else if (vCommand == "updateSettings")
	{
		updateSettings(vParams);
		return true;
	}
	///
	/// @fn Variant ValkyrieService::execute()
	///
	/// ##### start()
	/// Starts the service.
	///
	else if (vCommand == "start")
	{
		start();
		return {};
	}
	///
	/// @fn Variant ValkyrieService::execute()
	///
	/// ##### start()
	/// Stops the service.
	///
	else if (vCommand == "stop")
	{
		stop();
		return {};
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
}

} // namespace valkyrie
} // namespace cloud
} // namespace cmd

/// @}
