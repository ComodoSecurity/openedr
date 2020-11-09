//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (07.08.2019)
// Reviewer:
//
///
/// @file File Lookup Service (FLS) objects declaration
///
#pragma once
#include "flscommon.h"

namespace openEdr {
namespace cloud {
namespace fls {
namespace v7 {

//
// Possible additional hosts are:
// p10.fls.security.comodo.com
// p10.r11.v7.fls.security.comodo.com
//
constexpr char c_sDefaultHost[] = "p10.r11.v7.fls.security.comodo.com";
//constexpr char c_sDefaultHost[] = "fls.security.comodo.com";
constexpr size_t c_nDefaultUdpPort = 4447;
constexpr size_t c_nDefaultTcpPort = 4448;

//
//
//
enum class RequestType : uint8_t
{
	SimpleRequest = 0, // 1.2.1 Simple Request (Type 0)
	HitCountAndFirstSeenTimestampExtended = 4, // 1.2.2 Hit Count And First Seen Time Stamp Extended (Type 4)
	VendorLookup = 8, // 1.2.3 Vendor Look up (Type 8)
	MalwareDetection = 9, // 1.2.4 Malware Detection (Type 9)
	DomainLookup = 10, // 1.2.5 Domain Look up (Type 10)
	SubmitFileInformation = 11, // 1.2.6 Submit File Information (Type 11)
	FileHashLookupWithVerdictSource = 31, // 1.2.7 File Hash Lookup with verdict source (Type 31)
	FileHashLookupWithSourcesVerdicts = 32, // 1.2.8 File Hash Lookup with sources verdicts (Type 32)
	UrlLookUp = 35, // 1.2.9 URL Look up (Type 35)
	CwfCategorizedUrlLookUp = 36, // 1.2.10 CWF (Comodo Web Filtering) Categorized URL Look up (Type 36)
	CwfInvalidationDomainsUrlsList = 40, // 1.2.11 CWF Invalidation domains/URLs list (Type 40)
};

//
//
//
enum class CallerType : uint8_t
{
	FromOnAccess = 1,
	FromOnDemand = 2,
	FromSandbox = 3,
	Viruscope = 4,
	PresentInsideSandboxPrevInstall = 5,
	PresentOutsideSandboxPrevInstall = 6,
	PresentInsideSandboxAfterInstall = 7,
	PresentOutsideSandboxAfterInstall = 8
};

//
//
//
enum class HashType : uint8_t
{
	SHA1 = 0,
	SHA256 = 1,
	MD5 = 2,
};

//
//
//
enum class ApplicationId : uint8_t
{
	CloudAntivirus = 10 // COMODO Cloud Antivirus
};

#pragma pack(push, 1)
//
//
//
struct Request
{
	uint8_t marker;
	uint8_t nProtocolVersion;
	RequestType requestType;
	uint8_t nRequestTypeRevision;
	uint16_t nPayloadSize;
	// Payload
	uint32_t nId;
	ApplicationId applicationId;
	uint16_t nApplicationVersion;
	CallerType callerType;
	uint8_t guid[c_nGuidSize];
	HashType hashType;
	uint8_t hashes;
};

//
//
//
struct Response
{
	uint32_t nId;
	uint8_t nNumOfAnswers;
	uint8_t answers;
};
#pragma pack(pop)

//
// FLS Protocol v7 UDP client
//
class UdpClient : public Client
{
private:
	static constexpr Size c_nPacketMaxSize = 1024;

	Size m_nId = 0;
	std::mutex m_mtxId;

	Size getNextId();
	FileVerdictMap getVerdict(const HashList& hashList, RequestType eRequestType,
		Size nRequestTypeRevision, Size nResponseAnswerSize);

public:
	static constexpr Size c_nMaxHashPerRequest = (c_nPacketMaxSize - sizeof(Request) + 1) / c_nHashSize;

	//
	// Constructor
	//
	UdpClient(std::string_view sHost, Size nPort);

	//
	// Gets verdicts for a list of files
	//
	FileVerdictMap getFileVerdict(const HashList& hashList) override;

	//
	// Gets verdicts for a list of vendors
	//
	FileVerdictMap getVendorVerdict(const HashList& hashList) override;
};

} // namespace v7
} // namespace fls
} // namespace cloud
} // namespace openEdr
