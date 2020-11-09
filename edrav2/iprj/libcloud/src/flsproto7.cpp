//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (07.08.2019)
// Reviewer:
//
///
/// @file File Lookup Service (FLS) objects implementation
///
#include "pch.h"
#include <net.hpp>
#include "flsproto7.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "fls7"

namespace openEdr {
namespace cloud {
namespace fls {
namespace v7 {

static const uint8_t c_guidPrefix[] = { 0x3c, 0xb0, 0x3c, 0x49, 0x16, 0x1e, 0x49, 0xac, 0x99, 0x6e, 0x63, 0x03 };

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
FileVerdictMap UdpClient::getVerdict(const HashList& hashList, RequestType eRequestType,
	Size nRequestTypeRevision, Size nResponseAnswerSize)
{
	using boost::asio::ip::udp;
	using boost::asio::ip::tcp;
	using boost::asio::ip::address;
	
	if (hashList.size() > c_nMaxHashPerRequest)
		error::InvalidArgument(SL, "Too many hashes").throwException();

	boost::array<uint8_t, c_nPacketMaxSize> packet;
	auto pRequest = (Request*)packet.c_array();
	pRequest->marker = 0xEF;
	pRequest->nProtocolVersion = 7;
	pRequest->requestType = eRequestType;
	pRequest->nRequestTypeRevision = static_cast<uint8_t>(nRequestTypeRevision);
	pRequest->nPayloadSize = static_cast<uint16_t>(sizeof(Request) - 
		offsetof(Request, nId) - 1 + (hashList.size() * c_nHashSize));
	pRequest->nId = static_cast<uint32_t>(getNextId());
	pRequest->applicationId = ApplicationId::CloudAntivirus;
	pRequest->nApplicationVersion = 0;
	pRequest->callerType = CallerType::FromOnAccess;
	//// guid = <constant prefix> + <autoincrement identifier>
	//memcpy(pRequest->guid, c_guidPrefix, sizeof(c_guidPrefix));
	//*(uint32_t*)(pRequest->guid + sizeof(c_guidPrefix)) = pRequest->nId;
	// guid = { 0 }
	memset(pRequest->guid, 0, sizeof(pRequest->guid));
	pRequest->hashType = HashType::SHA1;

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

	boost::asio::io_context ioContext;

	//
	// TCP
	//

	//tcp::socket socket(ioContext);
	//tcp::endpoint remoteReceiver(addr, static_cast<unsigned short>(c_nDefaultTcpPort)); // m_nPort
	//socket.connect(remoteReceiver);
	//auto nBytesSent = socket.send(boost::asio::buffer(packet, nPacketSize));
	//if (nBytesSent != nPacketSize)
	//	error::RuntimeError(SL, "Not all data was sent").throwException();
	//boost::array<uint8_t, c_nPacketMaxSize> response;
	//Size nBytesReceived = socket.receive(boost::asio::buffer(response));
	//if (nBytesReceived < sizeof(Response))
	//	error::NoData(SL, "Response is too small").throwException();

	//
	// UDP
	//

	udp::socket socket(ioContext);
	socket.open(udp::v4());

	// set timeouts
	static constexpr Size nSendTimeoutMs = 2000;
	static constexpr Size nReceiveTimeoutMs = 2000;
	net::setSoketTimeout(socket, nSendTimeoutMs, nReceiveTimeoutMs);

	udp::endpoint remoteReceiver(addr, static_cast<unsigned short>(m_nPort));
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
	if (pResponse->nNumOfAnswers != hashList.size())
		error::InvalidFormat(SL, "Number of answers doesn't match requested").throwException();
	if (nBytesReceived < sizeof(Response) - 1 - offsetof(Response, answers) +
		(pResponse->nNumOfAnswers * nResponseAnswerSize))
		error::InvalidFormat(SL, "Not enough data received").throwException();

	FileVerdictMap verdicts;
	uint8_t* pVerdict = &pResponse->answers;
	//for (Size i = 0; i < hashList.size(); ++i)
	for (const auto& hash : hashList)
	{
		//verdicts.insert({ hashList[i], static_cast<FileVerdict>(*pVerdict) });
		verdicts.insert({ hash, static_cast<FileVerdict>(*pVerdict) });
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
	return getVerdict(hashList, RequestType::SimpleRequest, 2, 2);
	TRACE_END("Error during file verdict request");
}

//
//
//
FileVerdictMap UdpClient::getVendorVerdict(const HashList& hashList)
{
	TRACE_BEGIN;
	return getVerdict(hashList, RequestType::VendorLookup, 1, 1);
	TRACE_END("Error during vendor verdict request");
}

} // namespace v7
} // namespace fls
} // namespace cloud
} // namespace openEdr
