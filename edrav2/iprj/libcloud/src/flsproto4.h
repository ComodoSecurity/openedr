//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (19.03.2019)
// Reviewer: Denis Bogdanov (25.03.2019)
//
///
/// @file File Lookup Service (FLS) objects declaration
///
#pragma once
#include "flscommon.h"

namespace openEdr {
namespace cloud {
namespace fls {
namespace v4 {

//
// Possible additional hosts are:
// p10.fls.security.comodo.com
// p10.r11.v7.fls.security.comodo.com
//
constexpr char c_sDefaultHost[] = "fls.security.comodo.com";
constexpr size_t c_nDefaultUdpPort = 4447;
constexpr size_t c_nDefaultTcpPort = 4448;

//
//
//
enum class RequestType : uint8_t
{
	SimpleRequest = 0, // 8.2.2 Simple Request (Type 0)
	ExtendedRequest = 1, // 8.2.3 Extended Request (Type 1)
	HitCountAndFirstSeenTimestampSimple = 2, // 8.2.4 Hit Count And First Seen Time Stamp Simple (Type 2)
	// 8.2.1 Backward Compatibility Protocol With CIS/CFP (Type 3)
	HitCountAndFirstTimestampExtended = 4, // 8.2.5 Hit Count And First Seen Time Stamp Extended (Type 4)
	TrustedVendorLookup = 5, // 8.2.6 Trusted Vendor Look up (Type 5)
	MalwareDetection = 6, // 8.2.7 Malware Detection by CIS (Type 6)
	// 8.2.8 Submit File Information (Type 7) DEPRECATED
	VendorLookupExtended = 8, // 8.2.9 Vendor Look up Extended (Type 8)
	MalwareDetectionExtended = 9, // 8.2.10 Malware Detection by CIS Extended (Type 9)
	DomainLookup = 10, // 8.2.11 Domain Look up (Type 10)
	SubmitFileInformationExtended = 11, // 8.2.12 Submit File Information Type Extended (Type 11)
};

//
//
//
enum class CallerType : uint8_t
{
	FromOnAccess = 1,
	FromOnDemand = 2,
	FromSandbox = 3,
	Viruscope = 4
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
	CloudAntivirus = 10, // COMODO Cloud Antivirus
};

#pragma pack(push, 1)
//
//
//
struct Request
{
	RequestType requestType;
	uint32_t nId;
	uint8_t nNumOfHashes;
	HashType hashType;
	uint8_t nProtocol;
	ApplicationId applicationId;
	uint8_t guid[c_nGuidSize];
	CallerType callerType;
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
// FLS Protocol v4 UDP client
//
class UdpClient : public Client
{
private:
	static constexpr Size c_nPacketMaxSize = 1024;

	Size m_nId = 0;
	std::mutex m_mtxId;

	Size getNextId();
	FileVerdictMap getVerdict(RequestType eRequestType, const HashList& hashList, Size nResponseAnswerSize);

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

} // namespace v4
} // namespace fls
} // namespace cloud
} // namespace openEdr
