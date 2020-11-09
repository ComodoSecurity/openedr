//
// edrav2.libnetmon
// 
// Author: Podpruzhnikov Yury (15.10.2019)
// Reviewer:
//
///
/// @file DNS parser
///
/// @addtogroup netmon Network Monitor library
/// @{
#pragma once
#include "objects.h"
#include "netmon.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "libnetmon"

namespace openEdr {
namespace netmon {

//////////////////////////////////////////////////////////////////////////
//
// DNS protocol data
// Data is named according to RFC1035 but in camel-case notation
//
namespace dns {

//
// DNS OPCODE
//
enum class OpCode : uint8_t
{
	Query = 0,
	IQuery = 1,
	Status = 2,
};

//
// DNS RCODE
//
enum class RCode : uint8_t
{
	Success = 0,
	FormatError = 1,
	ServerFailure = 2,
	NameError = 3,
	NotImplemented = 4,
	Refused = 5,
};

//
// DNS TYPE & QTYPE
//
enum class QType {
	A = 1,
	NS = 2,
	CNAME = 5,
	SOA = 6,
	PTR = 12,
	MX = 15,
	TXT = 16,
	AAAA = 28,
	SRV = 33,
	OPT = 41,
	SSHFP = 44,
	SPF = 99,
	ALL = 255
};

//
// DNS CLASS & QCLASS 
//
#undef IN
enum class QClass {
	IN = 1,
	ANY = 255
};

//
// DNS query
//
struct Query
{
	std::string sName; // QNAME
	QType eType;
	QClass eClass;
};



//
// Parsed DNS message 
//
struct Message
{
	uint16_t m_nId; // identification number
	bool m_fQuery; // if true, message is query else message is response
	bool m_fAuthoritativeAnswer;
	bool m_fTruncated; // Message is truncated
	OpCode m_eOpCode;
	RCode m_eRCode;

	uint16_t m_nQueriesCount; //< Queries count
	uint16_t m_nAnswersCount; //< Answers are not parsed (just count)
	uint16_t m_nNameServersCount; //< NS are not parsed (just count)
	uint16_t m_nAdditionalRecordsCount; //< AR are not parsed (just count)

	// Parsed data can be less than count in case of truncated message
	std::vector<Query> m_queries; //< Queries
};

//
// Parse DNS message
//
bool parseMessage(const uint8_t* pData, size_t nDataSize, Message& message);


} // namespace dns 



///
/// DNS parser factory (and Parser).
///
class DnsUdpParserFactory : public ObjectBase<CLSID_DnsUdpParserFactory>,
	public IUdpParserFactory
{
private:
	ObjPtr<INetworkMonitorController> m_pNetMonController;  //< Callback to NetworkMonitorController

protected:
	DnsUdpParserFactory() {}
	~DnsUdpParserFactory() override {}
public:
	///
	/// Implements the object's final construction.
	///
	/// @param vConfig - object's configuration including the following fields:
	/// * controller - callback to controller.
	///
	void finalConstruct(Variant vConfig);

	// Inherited via IUdpParserFactory
	std::string_view getName() override;
	ObjPtr<IUdpParser> createParser(std::shared_ptr<const ConnectionInfo> pConInfo) override;
};


///
/// FTP parser.
///
class DnsUdpParser : public ObjectBase<>,
	public IUdpParser
{
private:
	ObjPtr<INetworkMonitorController> m_pNetMonController;  //< Callback to NetworkMonitorController

	//
	// Send event
	//
	// @param eRawEvent - raw event id
	// @param pConInfo - ConnectionInfo
	// @param fnAddData - function to add all additional event data
	//        Signature: bool fnAddData(Dictionary& vEvent);
	//        if fnAddData() returns false, this function returns null.
	//        fnAddData() should add "connection" field.
	// 
	template<typename FnAddData>
	void sendEvent(RawEvent eRawEvent, std::shared_ptr<const ConnectionInfo> pConInfo, FnAddData fnAddData);

protected:
	DnsUdpParser() {}
	~DnsUdpParser() override {}
public:

	// IUdpParser

	/// @copydoc IUdpParser::notifyData()
	DataStatus notifyData(bool fSend, std::shared_ptr<const ConnectionInfo> pConInfo, const uint8_t* pBuf, size_t nLen) override;
	/// @copydoc IUdpParser::notifyClose()
	void notifyClose(std::shared_ptr<const ConnectionInfo> pConInfo) override;

	static ObjPtr<DnsUdpParser> create(ObjPtr<INetworkMonitorController> pNetMonController);
};

} // namespace netmon
} // namespace openEdr

/// @}
