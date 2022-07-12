//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file DNS parser
///
#include "pch.h"
#include "parser_dns.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "netmon"

namespace cmd {
namespace netmon {

namespace dns {

enum class Flags1 : uint8_t
{
	QR = 0x01,
	AA = 0x20,
	TC = 0x40,
	RD = 0x80,
};
CMD_DEFINE_ENUM_FLAG_OPERATORS(Flags1)

enum class Flags2 : uint8_t
{
	RA = 0x01,
};
CMD_DEFINE_ENUM_FLAG_OPERATORS(Flags2)


//
// DNS message header
// Names are according to RFC1035.
//
#pragma pack(push, 1)
struct Header
{
	uint16_t id; // identification number

	uint8_t flags1;
	uint8_t flags2;

	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
};
#pragma pack(pop)


//
// Extract bits from byte
//
template<uint8_t c_nOffset, uint8_t c_nBitcount>
inline uint8_t getBits(const uint8_t nVal)
{
	static_assert((size_t(c_nOffset) + c_nBitcount) <= sizeof(nVal)*8, "Invalid offset or bitcount");
	constexpr uint8_t c_nBitMask = (uint8_t) ((1 << c_nBitcount) - 1);

	return (nVal >> c_nOffset) & c_nBitMask;
}

//
// Parse DNS message
//
bool parseMessage(const uint8_t* pData, size_t nDataSize, Message& message)
{
	// Process header
	if (nDataSize < sizeof(Header))
		return false;
	Header* pHeader = (Header*)pData;

	message.m_nId = ntohs(pHeader->id);
	message.m_nAnswersCount = ntohs(pHeader->ancount);
	message.m_nQueriesCount = ntohs(pHeader->qdcount);
	message.m_nAdditionalRecordsCount = ntohs(pHeader->nscount);
	message.m_nAdditionalRecordsCount = ntohs(pHeader->nscount);

	// Parse flags
	const Flags1 flags1 = Flags1(pHeader->flags1);
	message.m_fQuery = testFlag(flags1, Flags1::QR);
	message.m_fAuthoritativeAnswer = testFlag(flags1, Flags1::AA);
	message.m_fTruncated = testFlag(flags1, Flags1::TC);
	message.m_eOpCode = OpCode(getBits<1, 3>(pHeader->flags1));
	const Flags2 flags2 = Flags2(pHeader->flags2);
	// testFlag(flags2, Flags2::RA);
	message.m_eRCode = RCode(getBits<4, 4>(pHeader->flags2));


	const uint8_t* const pDataEnd = pData + nDataSize;

	// Parse queries
	const uint8_t* pCur = pData + sizeof(Header);
	while (true)
	{
		// Check finish
		if (message.m_queries.size() >= message.m_nQueriesCount)
			break;

		Query query;
		bool fAddPrefixDot = false;
		// extract QNAME
		while (true)
		{
			if (pCur >= pDataEnd)
				return false;
			const uint8_t nPartSize = *pCur;
			++pCur;
			// End of name
			if (nPartSize == 0)
				break;

			const char* const psPart = (const char*)pCur;
			pCur += nPartSize;
			if (pCur >= pDataEnd)
				return false;

			if (fAddPrefixDot)
				query.sName.append(1, '.');
			fAddPrefixDot = true;
			query.sName.append(psPart, nPartSize);
		}

		if(pCur + sizeof(uint16_t)*2 > pDataEnd)
			return false;

		query.eType = QType(ntohs(*(uint16_t*)pCur));
		query.eClass = QClass(ntohs(*(uint16_t*)(pCur+2)));
		pCur += 4;

		message.m_queries.push_back(std::move(query));
	}

	return true;
}


} // namespace dns 


//////////////////////////////////////////////////////////////////////////
//
// DnsUdpParserFactory
//

//
//
//
void DnsUdpParserFactory::finalConstruct(Variant vConfig)
{
	m_pNetMonController = queryInterface<INetworkMonitorController>(vConfig.get("controller"));
}

//
//
//
std::string_view DnsUdpParserFactory::getName()
{
	return "UDP-parser";
}

//
//
//
ObjPtr<IUdpParser> DnsUdpParserFactory::createParser(std::shared_ptr<const ConnectionInfo> pConInfo)
{
	return DnsUdpParser::create(m_pNetMonController);
}

//////////////////////////////////////////////////////////////////////////
//
// DnsUdpParser
//

//
//
//
template<typename FnAddData>
void DnsUdpParser::sendEvent(RawEvent eRawEvent, std::shared_ptr<const ConnectionInfo> pConInfo, FnAddData fnAddData)
{
	if (!m_pNetMonController->isEventEnabled(eRawEvent))
		return;

	if(eRawEvent != RawEvent::NETMON_REQUEST_DNS)
		error::InvalidArgument("Invalid raw event type").throwException();

	Dictionary vEvent {
		{"baseType", Event::LLE_NETWORK_REQUEST_DNS},
		{"rawEventId", createRaw(CLSID_NetworkMonitorController, (uint32_t)eRawEvent)},
		{"tickTime", ::GetTickCount64()},
		{"process", Dictionary {{"pid", pConInfo->nPid}} },
		{"connection", pConInfo->vConnection },
	};

	bool fSendEvent = fnAddData(vEvent);

	if (fSendEvent)
		m_pNetMonController->sendEvent(vEvent);
}


//
//
//
ObjPtr<DnsUdpParser> DnsUdpParser::create(ObjPtr<INetworkMonitorController> pNetMonController)
{
	auto pThis = createObject<DnsUdpParser>();
	pThis->m_pNetMonController = pNetMonController;
	return pThis;
}

//
//
//
DataStatus DnsUdpParser::notifyData(bool fSend, std::shared_ptr<const ConnectionInfo> pConInfo, const uint8_t* pBuf, size_t nLen)
{
	// Skip recieve
	if(!fSend)
		return DataStatus::Ignored;

	dns::Message message;
	if(!dns::parseMessage(pBuf, nLen, message))
		return DataStatus::Incompatible;

	if(!message.m_fQuery)
		return DataStatus::Processed;
	if (message.m_eOpCode != dns::OpCode::Query)
		return DataStatus::Processed;

	for (auto& query : message.m_queries)
	{
		if (query.eClass != dns::QClass::IN)
			continue;
		if (query.eType != dns::QType::A /*IPv4*/ && 
			query.eType != dns::QType::AAAA /*IPv6*/)
			continue;

		sendEvent(RawEvent::NETMON_REQUEST_DNS, pConInfo, [&query](Dictionary& vEvent) -> bool
		{
			vEvent.put("name", query.sName);
			return true;
		});
	}

	return DataStatus::Processed;
}

//
//
//
void DnsUdpParser::notifyClose(std::shared_ptr<const ConnectionInfo> pConInfo)
{
}

} // namespace netmon
} // namespace cmd
