//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (19.03.2019)
// Reviewer: Denis Bogdanov (25.03.2019)
//
///
/// @file File Lookup Service (FLS) objects implementation
///
#include "pch.h"
#include <net.hpp>
#include "flsproto4.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "fls4"

namespace openEdr {
namespace cloud {
namespace fls {
namespace v4 {


//
//
//
UdpClient::UdpClient(std::string_view sHost, Size nPort) :
	Client(sHost, nPort)
{
}

//
//
//
Size UdpClient::getNextId()
{
	std::scoped_lock lock(m_mtxId);
	return m_nId++;
}

//
//
//
FileVerdictMap UdpClient::getVerdict(RequestType eRequestType, const HashList& hashList, Size nResponseAnswerSize)
{
	using boost::asio::ip::udp;
	using boost::asio::ip::address;
	
	if (hashList.size() > c_nMaxHashPerRequest)
		error::InvalidArgument(SL, "Too many hashes").throwException();

	boost::array<uint8_t, c_nPacketMaxSize> packet;
	auto pRequest = (Request*)packet.c_array();
	pRequest->requestType = eRequestType;
	pRequest->nId = static_cast<uint32_t>(getNextId());
	pRequest->nNumOfHashes = static_cast<uint8_t>(hashList.size());
	pRequest->hashType = HashType::SHA1;
	pRequest->nProtocol = 3;
	pRequest->applicationId = ApplicationId::CloudAntivirus;
	memset(pRequest->guid, 0, sizeof(pRequest->guid));
	pRequest->callerType = CallerType::FromOnAccess;

	uint8_t* pHash = &pRequest->hashes;
	for (auto& sHash : hashList)
	{
		if (sHash.size() != c_nHashSize * 2)
			error::InvalidArgument(SL, FMT("Invalid hash size. Hash <" << sHash << ">" )).throwException();
		std::string sPlainHash;
		try
		{
			sPlainHash = boost::algorithm::unhex(sHash);
		}
		catch (const std::exception& ex)
		{
			error::InvalidArgument(SL, FMT("Invalid hash (" << ex.what() << ")")).throwException();
		}
		memcpy(pHash, sPlainHash.c_str(), sPlainHash.size());
		pHash += c_nHashSize;
	}	
	Size nPacketSize = sizeof(Request) - 1 + hashList.size() * c_nHashSize;

	// Retrieve host address
	boost::system::error_code ec;
	auto addr = address::from_string(m_sHost, ec);
	if (ec)
		// FIXME: Why do you use only first resolved address?
		addr = address::from_string(*net::getHostByName(m_sHost).begin());

	udp::endpoint remoteReceiver(addr, static_cast<unsigned short>(m_nPort));

	boost::asio::io_context ioContext;
	udp::socket socket(ioContext);
	socket.open(udp::v4());

	// set timeouts
	static constexpr Size nSendTimeoutMs = 2000;
	static constexpr Size nReceiveTimeoutMs = 2000;
	net::setSoketTimeout(socket, nSendTimeoutMs, nReceiveTimeoutMs);

	auto nBytesSent = socket.send_to(boost::asio::buffer(packet, nPacketSize), remoteReceiver, 0);
	if (nBytesSent != nPacketSize)
		error::RuntimeError(SL, "Not all data was sent").throwException();

	boost::array<uint8_t, c_nPacketMaxSize> response;
	udp::endpoint remoteSender;
	Size nBytesReceived = socket.receive_from(boost::asio::buffer(response), remoteSender, 0);
	if (nBytesReceived < sizeof(Response))
		error::NoData(SL, "Response is too small").throwException();

	auto pResponse = (Response*)response.c_array();
	if (pResponse->nId != pRequest->nId)
		error::InvalidFormat(SL, "Wrong request id in the response").throwException();
	if (pResponse->nNumOfAnswers != pRequest->nNumOfHashes)
		error::InvalidFormat(SL, "Number of answers doesn't match requested").throwException();
	if (nBytesReceived < sizeof(Response) - 1 + pResponse->nNumOfAnswers * nResponseAnswerSize)
		error::InvalidFormat(SL, "Not enough data received").throwException();

	FileVerdictMap verdicts;
	uint8_t* pVerdict = &pResponse->answers;
	for (Size i = 0; i < pResponse->nNumOfAnswers; ++i)
	{
		verdicts.insert({ hashList[i], static_cast<FileVerdict>(*pVerdict) });
		pVerdict += nResponseAnswerSize;
	}

	// FIXME: Do we need to use RAII for socket here?
	// FIXME: Do we need close socket after each request?
	socket.close();

	return verdicts;
}

//
//
//
FileVerdictMap UdpClient::getFileVerdict(const HashList& hashList)
{
	TRACE_BEGIN;
	return getVerdict(RequestType::SimpleRequest, hashList, 2);
	TRACE_END("Error during file verdict request");
}

//
//
//
FileVerdictMap UdpClient::getVendorVerdict(const HashList& hashList)
{
	TRACE_BEGIN;
	return getVerdict(RequestType::TrustedVendorLookup, hashList, 1);
	TRACE_END("Error during vendor verdict request");
}

} // namespace v4
} // namespace fls
} // namespace cloud
} // namespace openEdr
